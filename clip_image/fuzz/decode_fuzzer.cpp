#include <clip/image/service.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    static clip::image::Service service;
    const auto bytes = std::as_bytes(std::span(data, size));
    const clip::image::Input input = clip::image::memory_input(bytes);
    clip::image::DecodeOptions options;
    options.limits.maximum_input_bytes = std::min<std::uint64_t>(size, 16U << 20U);
    options.limits.maximum_width = 4096;
    options.limits.maximum_height = 4096;
    options.limits.maximum_pixels = std::uint64_t{16} << 20U;
    options.limits.maximum_frames = 64;
    options.limits.maximum_metadata_bytes = std::uint64_t{1} << 20U;
    options.limits.maximum_owned_output_bytes = std::uint64_t{64} << 20U;
    options.limits.maximum_working_bytes = std::uint64_t{64} << 20U;
    options.limits.maximum_deep_samples = std::uint64_t{1} << 20U;
    (void)service.decode(input, options);
    return 0;
}
