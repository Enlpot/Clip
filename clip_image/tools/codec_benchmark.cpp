#include <clip/image/service.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

double median_milliseconds(std::vector<double> samples) {
    std::sort(samples.begin(), samples.end());
    const std::size_t middle = samples.size() / 2U;
    return samples.size() % 2U == 0 ? (samples[middle - 1U] + samples[middle]) * 0.5
                                    : samples[middle];
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: clip_image_benchmark <image> [iterations]\n";
        return 2;
    }
    const int iterations = argc > 2 ? std::max(1, std::atoi(argv[2])) : 3;
    clip::image::Result<clip::image::Input> input =
        clip::image::file_input(std::filesystem::path(argv[1]));
    if (!input) {
        std::cerr << input.error().message << '\n';
        return 1;
    }
    clip::image::Service service;
    clip::image::Result<clip::image::Document> source = service.decode(input.value());
    if (!source) {
        std::cerr << source.error().message << '\n';
        return 1;
    }
    const auto started = std::chrono::steady_clock::now();
    std::uint64_t decodedBytes = 0;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        clip::image::Result<clip::image::Document> document = service.decode(input.value());
        if (!document) {
            std::cerr << document.error().message << '\n';
            return 1;
        }
        for (const clip::image::Frame& frame : document.value().frames) {
            decodedBytes += frame.image.pixels().size();
        }
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started);
    const double mebibytes = static_cast<double>(decodedBytes) / (1024.0 * 1024.0);
    std::cout << iterations << " decode(s), " << elapsed.count() << " s, "
              << mebibytes / elapsed.count() << " MiB/s\n";

    if (source.value().format == clip::image::Format::jpeg) {
        for (const auto [name, layout] :
             {std::pair{"packed", clip::image::RasterLayoutPolicy::packed},
              std::pair{"native", clip::image::RasterLayoutPolicy::native}}) {
            std::vector<double> samples;
            samples.reserve(static_cast<std::size_t>(iterations));
            std::uint64_t storeBytes = 0;
            for (int iteration = 0; iteration < iterations; ++iteration) {
                const std::filesystem::path path =
                    std::filesystem::temp_directory_path() /
                    ("clip-image-benchmark-" + std::string(name) + "-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                     ".srs");
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
                clip::image::DecodeOptions options;
                options.raster_layout = layout;
                const auto decodeStarted = std::chrono::steady_clock::now();
                auto store = service.decode_to_store(input.value(), path, options);
                const auto decodeElapsed = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - decodeStarted);
                if (!store) {
                    std::cerr << store.error().message << '\n';
                    return 1;
                }
                samples.push_back(decodeElapsed.count());
                storeBytes = store.value()->file_bytes();
                store.value().reset();
                std::filesystem::remove(path, ignored);
            }
            std::cout << "JPEG raster decode: layout=" << name << ", iterations=" << iterations
                      << ", median=" << std::fixed << std::setprecision(3)
                      << median_milliseconds(samples) << " ms, store-bytes=" << storeBytes << '\n';
        }
    }

    if (service.encoder_info(clip::image::Format::png)) {
        const std::string_view backend =
            clip::image::compression_backend_version(clip::image::Format::png);
        std::cout << "PNG compression backend: " << backend << '\n';
        for (const int compression : {1, 6, 9}) {
            std::vector<double> samples;
            samples.reserve(static_cast<std::size_t>(iterations));
            std::size_t outputBytes = 0;
            for (int iteration = 0; iteration < iterations; ++iteration) {
                auto encoded = std::make_shared<std::vector<std::byte>>();
                clip::image::EncodeOptions options;
                options.format = clip::image::Format::png;
                options.compression_level = compression;
                const auto encodeStarted = std::chrono::steady_clock::now();
                clip::image::Result<clip::image::EncodeResult> status =
                    service.encode(source.value(), clip::image::memory_output(encoded), options);
                const auto encodeElapsed = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - encodeStarted);
                if (!status) {
                    std::cerr << status.error().message << '\n';
                    return 1;
                }
                samples.push_back(encodeElapsed.count());
                outputBytes = encoded->size();
            }
            std::cout << "PNG encode: compression=" << compression
                      << ", filter=libpng-adaptive, zlib=" << backend
                      << ", iterations=" << iterations << ", median=" << std::fixed
                      << std::setprecision(3) << median_milliseconds(samples)
                      << " ms, bytes=" << outputBytes << '\n';
        }
    }
    return 0;
}
