#include "clip_draw_engine_qt/clip_canvas_image.h"
#include "clipimageqtcodec.h"
#include "clipimageqtsrgbrowreader.h"

#include "clipimagecodecbridge.h"

#include <clip/memory/pixel_buffer.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QBuffer>
#include <QColorSpace>
#include <QIODevice>
#include <QImageReader>
#include <QPainter>
#include <QTransform>

namespace clip_shot::image_codec {
namespace {

constexpr std::size_t kBackendErrorCapacity = 1024;

void releaseBackendBuffer(void* rawBuffer) {
    auto* buffer = static_cast<ClipShotImageCodecBuffer*>(rawBuffer);
    if (buffer == nullptr) {
        return;
    }
    clip_shot_image_codec_release_buffer(buffer);
    delete buffer;
}

struct StreamingBridgeContext final {
    const ScreenshotImageRowSource* source = nullptr;
    QIODevice* device = nullptr;
    QString ioError;
};

int32_t CLIP_SHOT_IMAGE_CODEC_CALL readRowsCallback(void* rawContext, uint32_t firstRow,
                                                    uint32_t rowCount, uint64_t destinationStride,
                                                    uint8_t* destination,
                                                    uint64_t destinationSize) {
    auto* context = static_cast<StreamingBridgeContext*>(rawContext);
    if (context == nullptr || context->source == nullptr || destination == nullptr ||
        firstRow > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        rowCount > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        destinationStride > static_cast<uint64_t>(std::numeric_limits<qsizetype>::max()) ||
        destinationSize > static_cast<uint64_t>(std::numeric_limits<qsizetype>::max()) ||
        !context->source->readRows(static_cast<int>(firstRow), static_cast<int>(rowCount),
                                   static_cast<qsizetype>(destinationStride), destination,
                                   static_cast<qsizetype>(destinationSize))) {
        return 0;
    }
    return 1;
}

int32_t CLIP_SHOT_IMAGE_CODEC_CALL writeCallback(void* rawContext, const uint8_t* source,
                                                 uint64_t sourceSize) {
    auto* context = static_cast<StreamingBridgeContext*>(rawContext);
    if (context == nullptr || context->device == nullptr ||
        (source == nullptr && sourceSize != 0)) {
        return 0;
    }
    uint64_t written = 0;
    while (written < sourceSize) {
        const uint64_t remaining = sourceSize - written;
        const qsizetype chunk = static_cast<qsizetype>(std::min<uint64_t>(
            remaining, static_cast<uint64_t>(std::numeric_limits<qsizetype>::max())));
        const qint64 result =
            context->device->write(reinterpret_cast<const char*>(source + written), chunk);
        if (result <= 0) {
            context->ioError = context->device->errorString();
            return 0;
        }
        written += static_cast<uint64_t>(result);
    }
    return 1;
}

int32_t CLIP_SHOT_IMAGE_CODEC_CALL positionCallback(void* rawContext, uint64_t* position) {
    auto* context = static_cast<StreamingBridgeContext*>(rawContext);
    if (context == nullptr || context->device == nullptr || position == nullptr) {
        return 0;
    }
    const qint64 value = context->device->pos();
    if (value < 0) {
        context->ioError = context->device->errorString();
        return 0;
    }
    *position = static_cast<uint64_t>(value);
    return 1;
}

int32_t CLIP_SHOT_IMAGE_CODEC_CALL seekCallback(void* rawContext, uint64_t position) {
    auto* context = static_cast<StreamingBridgeContext*>(rawContext);
    if (context == nullptr || context->device == nullptr ||
        position > static_cast<uint64_t>(std::numeric_limits<qint64>::max()) ||
        !context->device->seek(static_cast<qint64>(position))) {
        if (context != nullptr && context->device != nullptr) {
            context->ioError = context->device->errorString();
        }
        return 0;
    }
    return 1;
}

int32_t CLIP_SHOT_IMAGE_CODEC_CALL flushCallback(void* rawContext) {
    auto* context = static_cast<StreamingBridgeContext*>(rawContext);
    if (context == nullptr || context->device == nullptr) {
        return 0;
    }
    if (auto* file = dynamic_cast<QFileDevice*>(context->device);
        file != nullptr && !file->flush()) {
        context->ioError = file->errorString();
        return 0;
    }
    return 1;
}

int32_t CLIP_SHOT_IMAGE_CODEC_CALL cancelledCallback(void* rawContext) {
    const auto* context = static_cast<const StreamingBridgeContext*>(rawContext);
    return context != nullptr && context->source != nullptr &&
                   context->source->cancellationRequested &&
                   context->source->cancellationRequested()
               ? 1
               : 0;
}

bool backendAbiIsCompatible() noexcept {
    return clip_shot_image_codec_abi_version() == CLIP_SHOT_IMAGE_CODEC_ABI_VERSION;
}

uint32_t bridgeFormat(clip::image::Format format) noexcept {
    switch (format) {
    case clip::image::Format::unknown:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN;
    case clip::image::Format::bmp:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_BMP;
    case clip::image::Format::cur:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_CUR;
    case clip::image::Format::gif:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_GIF;
    case clip::image::Format::ico:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_ICO;
    case clip::image::Format::jpeg:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_JPEG;
    case clip::image::Format::pbm:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_PBM;
    case clip::image::Format::pgm:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_PGM;
    case clip::image::Format::png:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_PNG;
    case clip::image::Format::ppm:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_PPM;
    case clip::image::Format::svg:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_SVG;
    case clip::image::Format::svgz:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_SVGZ;
    case clip::image::Format::xbm:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_XBM;
    case clip::image::Format::xpm:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_XPM;
    case clip::image::Format::heif:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_HEIF;
    case clip::image::Format::avif:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_AVIF;
    case clip::image::Format::jxl:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_JXL;
    case clip::image::Format::exr:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_EXR;
    case clip::image::Format::webp:
        return CLIP_SHOT_IMAGE_CODEC_FORMAT_WEBP;
    }
    return CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN;
}

clip::image::EncoderOptionRange
encoderOptionRange(const ClipShotImageCodecEncoderOptionRange& range) noexcept {
    return {range.minimum, range.maximum, range.default_value};
}

uint8_t bridgeChromaSubsampling(clip::image::ChromaSubsampling value) noexcept {
    switch (value) {
    case clip::image::ChromaSubsampling::none:
        return CLIP_SHOT_IMAGE_CODEC_CHROMA_NONE;
    case clip::image::ChromaSubsampling::yuv444:
        return CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV444;
    case clip::image::ChromaSubsampling::yuv422:
        return CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV422;
    case clip::image::ChromaSubsampling::yuv420:
        return CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV420;
    case clip::image::ChromaSubsampling::yuv440:
        return CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV440;
    case clip::image::ChromaSubsampling::yuv411:
        return CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV411;
    case clip::image::ChromaSubsampling::yuv441:
        return CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV441;
    }
    return CLIP_SHOT_IMAGE_CODEC_CHROMA_NONE;
}

uint8_t bridgeAlphaContent(clip::image::AlphaContent value) noexcept {
    switch (value) {
    case clip::image::AlphaContent::opaque:
        return CLIP_SHOT_IMAGE_CODEC_ALPHA_OPAQUE;
    case clip::image::AlphaContent::non_opaque:
        return CLIP_SHOT_IMAGE_CODEC_ALPHA_NON_OPAQUE;
    }
    return CLIP_SHOT_IMAGE_CODEC_ALPHA_OPAQUE;
}

ClipShotImageCodecEncodeOptions bridgeOptions(clip::image::Format format,
                                              const clip::image::EncodeOptions& options) noexcept {
    ClipShotImageCodecEncodeOptions result{};
    result.struct_size = static_cast<uint32_t>(sizeof(ClipShotImageCodecEncodeOptions));
    result.abi_version = CLIP_SHOT_IMAGE_CODEC_ABI_VERSION;
    result.format = bridgeFormat(format);
    result.quality = options.quality;
    result.effort = options.effort;
    result.lossless_effort = options.lossless_effort;
    result.compression_level = options.compression_level;
    result.lossless = options.lossless ? uint8_t{1} : uint8_t{0};
    result.preserve_metadata = options.preserve_metadata ? uint8_t{1} : uint8_t{0};
    result.progressive = options.progressive ? uint8_t{1} : uint8_t{0};
    result.interlaced = options.interlaced ? uint8_t{1} : uint8_t{0};
    if (options.chroma_subsampling.has_value()) {
        result.has_chroma_subsampling = 1;
        result.chroma_subsampling = bridgeChromaSubsampling(*options.chroma_subsampling);
    }
    if (options.verified_alpha_content.has_value()) {
        result.has_verified_alpha_content = 1;
        result.verified_alpha_content = bridgeAlphaContent(*options.verified_alpha_content);
    }
    return result;
}

void setError(QString* output, const char* backendError, const char* fallback) {
    if (output == nullptr) {
        return;
    }
    *output = backendError != nullptr && backendError[0] != '\0' ? QString::fromUtf8(backendError)
                                                                 : QString::fromLatin1(fallback);
}

QByteArray encodeImage(const QImage& image, clip::image::Format format,
                       const clip::image::EncodeOptions& options, QString* error) {
    QByteArray encoded;
    QBuffer buffer(&encoded);
    if (!buffer.open(QIODevice::WriteOnly) ||
        !encodeToDevice(image, &buffer, format, options, error)) {
        return {};
    }
    return encoded;
}

QColorSpace decodedColorSpace(const ClipShotImageCodecColorEncoding& color) {
    if (color.icc_profile != nullptr && color.icc_profile_size != 0 &&
        color.icc_profile_size <= static_cast<uint64_t>(std::numeric_limits<qsizetype>::max())) {
        // QColorSpace must own the ICC bytes independently of the native pixel buffer.
        const QByteArray profile(reinterpret_cast<const char*>(color.icc_profile),
                                 static_cast<qsizetype>(color.icc_profile_size));
        const QColorSpace space = QColorSpace::fromIccProfile(profile);
        if (space.isValid())
            return space;
    }

    QColorSpace::Primaries primaries;
    switch (color.primaries) {
    case CLIP_SHOT_IMAGE_CODEC_PRIMARIES_SRGB:
        primaries = QColorSpace::Primaries::SRgb;
        break;
    case CLIP_SHOT_IMAGE_CODEC_PRIMARIES_DISPLAY_P3:
        primaries = QColorSpace::Primaries::DciP3D65;
        break;
    case CLIP_SHOT_IMAGE_CODEC_PRIMARIES_ADOBE_RGB:
        primaries = QColorSpace::Primaries::AdobeRgb;
        break;
    case CLIP_SHOT_IMAGE_CODEC_PRIMARIES_REC2020:
        primaries = QColorSpace::Primaries::Bt2020;
        break;
    default:
        return {};
    }
    QColorSpace::TransferFunction transfer;
    switch (color.transfer) {
    case CLIP_SHOT_IMAGE_CODEC_TRANSFER_LINEAR:
        transfer = QColorSpace::TransferFunction::Linear;
        break;
    case CLIP_SHOT_IMAGE_CODEC_TRANSFER_SRGB:
        transfer = QColorSpace::TransferFunction::SRgb;
        break;
    case CLIP_SHOT_IMAGE_CODEC_TRANSFER_GAMMA:
        transfer = QColorSpace::TransferFunction::Gamma;
        break;
    case CLIP_SHOT_IMAGE_CODEC_TRANSFER_PQ:
        transfer = QColorSpace::TransferFunction::St2084;
        break;
    case CLIP_SHOT_IMAGE_CODEC_TRANSFER_HLG:
        transfer = QColorSpace::TransferFunction::Hlg;
        break;
    default:
        return {};
    }
    return QColorSpace(primaries, transfer);
}

using OwnedBackendBuffer =
    std::unique_ptr<ClipShotImageCodecBuffer, decltype(&releaseBackendBuffer)>;

QImage takeDecodedImage(OwnedBackendBuffer output, QImage::Format format) {
    if (!output || output->data == nullptr || output->width == 0 || output->height == 0 ||
        output->width > static_cast<uint32_t>(std::numeric_limits<int>::max() / 4) ||
        output->height > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        return {};
    }
    const uint64_t rowBytes = static_cast<uint64_t>(output->width) * 4U;
    if (output->row_stride != rowBytes ||
        output->height > std::numeric_limits<uint64_t>::max() / rowBytes) {
        return {};
    }
    const uint64_t requiredSize = rowBytes * output->height;
    if (requiredSize > output->size ||
        requiredSize > static_cast<uint64_t>(std::numeric_limits<qsizetype>::max())) {
        return {};
    }

    const QColorSpace space = decodedColorSpace(output->color);
    QImage result(output->data, static_cast<int>(output->width), static_cast<int>(output->height),
                  static_cast<qsizetype>(rowBytes), format, &releaseBackendBuffer, output.get());
    if (result.isNull())
        return {};
    // Both packed formats share this owner. Attach metadata while the QImage is unique
    // so neither wrapping the pixels nor setting the profile introduces an image copy.
    static_cast<void>(output.release());
    result.setColorSpace(space);
    return result;
}

QImage decodeBytes(const QByteArray& encoded, clip::image::Format expectedFormat,
                   uint32_t preferredIconExtent = 0,
                   QImage::Format pixelFormat = QImage::Format_RGBA8888) {
    const uint32_t bridgeExpectedFormat = bridgeFormat(expectedFormat);
    if (!backendAbiIsCompatible() || encoded.isEmpty() ||
        bridgeExpectedFormat == CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN) {
        return {};
    }

    OwnedBackendBuffer output(new (std::nothrow) ClipShotImageCodecBuffer{}, &releaseBackendBuffer);
    if (!output)
        return {};
    std::array<char, kBackendErrorCapacity> backendError{};
    const auto decoder = preferredIconExtent != 0 ? clip_shot_image_codec_decode_icon_rgba8
                                                  : (pixelFormat == QImage::Format_ARGB32
                                                         ? clip_shot_image_codec_decode_bgra8
                                                         : clip_shot_image_codec_decode_rgba8);
    const int32_t succeeded =
        decoder(reinterpret_cast<const uint8_t*>(encoded.constData()),
                static_cast<uint64_t>(encoded.size()),
                preferredIconExtent != 0 ? preferredIconExtent : bridgeExpectedFormat, output.get(),
                backendError.data(), static_cast<uint64_t>(backendError.size()));
    if (succeeded == 0) {
        return {};
    }
    return takeDecodedImage(std::move(output), pixelFormat);
}

QImage decodeBgraBytes(const QByteArray& encoded, clip::image::Format expectedFormat) {
    return decodeBytes(encoded, expectedFormat, 0, QImage::Format_ARGB32);
}

bool inspectBytes(const QByteArray& encoded, clip::image::Format expectedFormat,
                  const QSize& expectedSize) {
    const uint32_t bridgeExpectedFormat = bridgeFormat(expectedFormat);
    if (!backendAbiIsCompatible() || encoded.isEmpty() || !expectedSize.isValid() ||
        bridgeExpectedFormat == CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN) {
        return false;
    }

    ClipShotImageCodecImageInfo information{};
    std::array<char, kBackendErrorCapacity> backendError{};
    const int32_t succeeded = clip_shot_image_codec_inspect(
        reinterpret_cast<const uint8_t*>(encoded.constData()),
        static_cast<uint64_t>(encoded.size()), bridgeExpectedFormat, &information,
        backendError.data(), static_cast<uint64_t>(backendError.size()));
    return succeeded != 0 && information.width == static_cast<uint32_t>(expectedSize.width()) &&
           information.height == static_cast<uint32_t>(expectedSize.height());
}

QByteArray readFile(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}

} // namespace

std::optional<clip::image::EncoderInfo> encoderInfo(clip::image::Format format) {
    const uint32_t requestedFormat = bridgeFormat(format);
    if (!backendAbiIsCompatible() || requestedFormat == CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN)
        return std::nullopt;
    ClipShotImageCodecEncoderInfo bridge{};
    bridge.struct_size = sizeof(bridge);
    bridge.abi_version = CLIP_SHOT_IMAGE_CODEC_ABI_VERSION;
    if (clip_shot_image_codec_encoder_info(requestedFormat, &bridge) == 0 ||
        bridge.format != requestedFormat) {
        return std::nullopt;
    }
    clip::image::EncoderInfo result;
    result.format = format;
    result.features = static_cast<clip::image::EncoderFeature>(bridge.features);
    result.quality = encoderOptionRange(bridge.quality);
    result.effort = encoderOptionRange(bridge.effort);
    result.lossless_effort = encoderOptionRange(bridge.lossless_effort);
    result.compression_level = encoderOptionRange(bridge.compression_level);
    return result;
}

bool encodeToDevice(const ScreenshotImageRowSource& source, QIODevice* device,
                    clip::image::Format format, const clip::image::EncodeOptions& options,
                    QString* error, EncodeResult* result) {
    if (error != nullptr) {
        error->clear();
    }
    if (result != nullptr)
        *result = {};
    if (!backendAbiIsCompatible()) {
        setError(error, nullptr, "The image codec backend is incompatible.");
        return false;
    }
    if (!source.isValid() || device == nullptr || !device->isOpen() || !device->isWritable() ||
        bridgeFormat(format) == CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN) {
        setError(error, nullptr, "The image source, format, or output device is invalid.");
        return false;
    }

    StreamingBridgeContext context{&source, device, {}};
    ClipShotImageCodecRgba8Source bridgeSource{};
    bridgeSource.struct_size = sizeof(bridgeSource);
    bridgeSource.abi_version = CLIP_SHOT_IMAGE_CODEC_ABI_VERSION;
    bridgeSource.context = &context;
    bridgeSource.width = static_cast<uint32_t>(source.size.width());
    bridgeSource.height = static_cast<uint32_t>(source.size.height());
    bridgeSource.read_rows = &readRowsCallback;
    bridgeSource.is_cancelled = &cancelledCallback;

    ClipShotImageCodecByteSink bridgeSink{};
    bridgeSink.struct_size = sizeof(bridgeSink);
    bridgeSink.abi_version = CLIP_SHOT_IMAGE_CODEC_ABI_VERSION;
    bridgeSink.context = &context;
    bridgeSink.write = &writeCallback;
    bridgeSink.position = &positionCallback;
    bridgeSink.seek = &seekCallback;
    bridgeSink.flush = &flushCallback;
    bridgeSink.is_cancelled = &cancelledCallback;
    bridgeSink.seekable = device->isSequential() ? 0 : 1;

    const ClipShotImageCodecEncodeOptions encodedOptions = bridgeOptions(format, options);
    std::array<char, kBackendErrorCapacity> backendError{};
    ClipShotImageCodecEncodeResult bridgeResult{};
    bridgeResult.struct_size = sizeof(bridgeResult);
    bridgeResult.abi_version = CLIP_SHOT_IMAGE_CODEC_ABI_VERSION;
    const int32_t succeeded = clip_shot_image_codec_encode_rgba8_stream(
        &bridgeSource, &bridgeSink, &encodedOptions, &bridgeResult, backendError.data(),
        static_cast<uint64_t>(backendError.size()));
    if (succeeded == 0 || bridgeResult.bytes_written == 0 ||
        bridgeResult.encoded_format != bridgeFormat(format) ||
        bridgeResult.canvas_width != static_cast<uint32_t>(source.size.width()) ||
        bridgeResult.canvas_height != static_cast<uint32_t>(source.size.height()) ||
        bridgeResult.emitted_frame_count == 0 ||
        bridgeResult.pixel_round_trip > CLIP_SHOT_IMAGE_CODEC_PIXEL_ROUND_TRIP_CODEC_ARTIFACT ||
        bridgeResult.encoder_finalized_and_sink_flushed == 0) {
        if (!context.ioError.isEmpty() && error != nullptr) {
            *error = context.ioError;
        } else {
            setError(error, backendError.data(), "Image encoding failed.");
        }
        return false;
    }
    if (result != nullptr) {
        result->bytesWritten = bridgeResult.bytes_written;
        result->format = format;
        result->size = source.size;
        result->emittedFrameCount = bridgeResult.emitted_frame_count;
        result->roundTrip =
            bridgeResult.pixel_round_trip == CLIP_SHOT_IMAGE_CODEC_PIXEL_ROUND_TRIP_EXACT
                ? clip::image::PixelRoundTrip::exact
                : clip::image::PixelRoundTrip::codec_artifact;
        result->finalizedAndFlushed = true;
    }
    return true;
}

bool encodeToDevice(const QImage& image, QIODevice* device, clip::image::Format format,
                    const clip::image::EncodeOptions& options, QString* error,
                    EncodeResult* result) {
    const ScreenshotImageRowSource source = srgbRowSource(image);
    if (!source.isValid()) {
        setError(error, nullptr, "The image could not be converted to RGBA pixels.");
        return false;
    }
    return encodeToDevice(source, device, format, options, error, result);
}

bool resizeToRgba8(const ScreenshotImageRowSource& source, const QSize& outputSize,
                   uchar* destination, qsizetype destinationStride, qsizetype destinationSize,
                   QString* error) {
    if (error != nullptr)
        error->clear();
    const qsizetype outputRowBytes = static_cast<qsizetype>(outputSize.width()) * 4;
    if (!backendAbiIsCompatible() || !source.isValid() || outputSize.isEmpty() ||
        outputSize.width() > std::numeric_limits<int>::max() / 4 || destination == nullptr ||
        destinationStride < outputRowBytes || destinationSize < outputRowBytes ||
        outputSize.height() - 1 > (destinationSize - outputRowBytes) / destinationStride) {
        setError(error, nullptr, "The resize source or destination is invalid.");
        return false;
    }

    StreamingBridgeContext context{&source, nullptr, {}};
    ClipShotImageCodecRgba8Source bridgeSource{};
    bridgeSource.struct_size = sizeof(bridgeSource);
    bridgeSource.abi_version = CLIP_SHOT_IMAGE_CODEC_ABI_VERSION;
    bridgeSource.context = &context;
    bridgeSource.width = static_cast<uint32_t>(source.size.width());
    bridgeSource.height = static_cast<uint32_t>(source.size.height());
    bridgeSource.read_rows = &readRowsCallback;
    bridgeSource.is_cancelled = &cancelledCallback;

    std::array<char, kBackendErrorCapacity> backendError{};
    const int32_t succeeded = clip_shot_image_codec_resize_rgba8(
        &bridgeSource, destination, static_cast<uint64_t>(destinationStride),
        static_cast<uint64_t>(destinationSize), static_cast<uint32_t>(outputSize.width()),
        static_cast<uint32_t>(outputSize.height()), backendError.data(),
        static_cast<uint64_t>(backendError.size()));
    if (succeeded == 0) {
        setError(error, backendError.data(), "Image resizing failed.");
        return false;
    }
    return true;
}

QByteArray encodePng(const QImage& image, int compressionLevel) {
    clip::image::EncodeOptions options;
    options.compression_level = qBound(0, compressionLevel, 9);
    return encodeImage(image, clip::image::Format::png, options, nullptr);
}

ScreenshotImageRowSource srgbRowSource(const QImage& image) {
    if (image.isNull())
        return {};
    const QColorSpace srgb(QColorSpace::SRgb);
    const bool convertColor = image.colorSpace().isValid() && image.colorSpace() != srgb;
    ScreenshotImageRowSource source;
    source.size = image.size();
    // Untagged images retain the existing assumption that their pixels are
    // sRGB. Retagging a shared image here would detach its complete raster.
    if (!convertColor && image.format() == QImage::Format_RGBA8888) {
        source.backingImage = image;
        source.readRows = [image](int first, int count, qsizetype stride, uchar* destination,
                                  qsizetype capacity) {
            return clipCanvasCopyRgba8888Rows(image, first, count, destination, capacity, stride);
        };
    } else {
        source.readRows = detail::BoundedSrgbRowReader(image);
    }
    return source;
}

QByteArray encodePng(const ScreenshotImageRowSource& source, int compressionLevel) {
    QByteArray bytes;
    QBuffer buffer(&bytes);
    clip::image::EncodeOptions options;
    options.compression_level = qBound(0, compressionLevel, 9);
    if (!buffer.open(QIODevice::WriteOnly) ||
        !encodeToDevice(source, &buffer, clip::image::Format::png, options)) {
        return {};
    }
    return bytes;
}

QByteArray encodeWebp(const QImage& image, int quality) {
    clip::image::EncodeOptions options;
    options.quality = quality;
    return encodeImage(image, clip::image::Format::webp, options, nullptr);
}

QImage decode(const QByteArray& encoded, clip::image::Format expectedFormat,
              const char* /*nameHint*/) {
    return decodeBytes(encoded, expectedFormat);
}

QImage readManagedImage(QImageReader& reader) {
    const auto fallback = [&reader] {
        QImage image = reader.read();
        return image.sizeInBytes() < qsizetype(clip::memory::kMappedPixelBufferMinimum)
                   ? image
                   : clipCanvasCopyImage(image);
    };
    const QSize size = reader.size();
    const QImage::Format format = reader.imageFormat();
    if (size.isEmpty() || format <= QImage::Format_Invalid || format >= QImage::NImageFormats)
        return fallback();

    // Qt checks this limit only when its handler allocates a new image. Reusing
    // our allocation must enforce the same effective depth and aligned stride.
    const quint64 depth =
        std::max(quint64{32}, quint64{QImage::toPixelFormat(format).bitsPerPixel()});
    const quint64 stride = ((static_cast<quint64>(size.width()) * depth + 31) / 32) * 4;
    if (stride == 0 || static_cast<quint64>(size.height()) >
                           static_cast<quint64>(std::numeric_limits<qsizetype>::max()) / stride)
        return fallback();
    const quint64 bytes = stride * static_cast<quint64>(size.height());
    const int limit = QImageReader::allocationLimit();
    if (limit > 0 && bytes > static_cast<quint64>(limit) * 1024 * 1024)
        return fallback();

    QImage result = clipCanvasAllocateImage(size, format);
    if (result.isNull())
        return fallback();
    const uchar* pixels = result.constBits();
    if (!reader.read(&result))
        return {};
    // Some plugins cannot reuse storage, and automatic orientation or reader
    // scaling can replace it after decoding. Preserve those results exactly.
    return result.constBits() == pixels ||
                   result.sizeInBytes() < qsizetype(clip::memory::kMappedPixelBufferMinimum)
               ? result
               : clipCanvasCopyImage(result);
}

QImage decodeIconFile(const QString& path, uint32_t preferredExtent) {
    if (preferredExtent == 0)
        return {};
    return decodeBytes(readFile(path), clip::image::Format::ico, preferredExtent);
}

QSize inspectSize(const QByteArray& encoded, clip::image::Format expectedFormat) {
    const auto format = bridgeFormat(expectedFormat);
    if (!backendAbiIsCompatible() || encoded.isEmpty() ||
        format == CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN)
        return {};
    ClipShotImageCodecImageInfo information{};
    std::array<char, kBackendErrorCapacity> backendError{};
    if (clip_shot_image_codec_inspect(reinterpret_cast<const uint8_t*>(encoded.constData()),
                                      static_cast<uint64_t>(encoded.size()), format, &information,
                                      backendError.data(),
                                      static_cast<uint64_t>(backendError.size())) == 0 ||
        information.width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        information.height > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        return {};
    return QSize(static_cast<int>(information.width), static_cast<int>(information.height));
}

QImage decodeFile(const QString& path, clip::image::Format expectedFormat) {
    return decodeBytes(readFile(path), expectedFormat);
}

QImage decodeFileBgra(const QString& path, clip::image::Format expectedFormat) {
    return decodeBgraBytes(readFile(path), expectedFormat);
}

SkinDecodeResult decodeSkinFile(const QString& path) {
    constexpr qint64 inputLimit = 64LL << 20U;
    constexpr int previewExtent = 4096;
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix != QStringLiteral("png") && suffix != QStringLiteral("jpg") &&
        suffix != QStringLiteral("jpeg") && suffix != QStringLiteral("webp"))
        return {{}, SkinDecodeError::UnsupportedFormat};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.isSequential())
        return {{}, SkinDecodeError::UnreadableFile};
    if (file.size() > inputLimit)
        return {{}, SkinDecodeError::InputTooLarge};
    try {
        // One extra byte detects a file growing after size() without allowing
        // readAll() to allocate an unbounded buffer.
        const QByteArray encoded = file.read(inputLimit + 1);
        if (encoded.size() > inputLimit)
            return {{}, SkinDecodeError::InputTooLarge};
        if (file.error() != QFileDevice::NoError)
            return {{}, SkinDecodeError::UnreadableFile};
        if (encoded.isEmpty())
            return {{}, SkinDecodeError::InvalidImage};
        if (!backendAbiIsCompatible())
            return {{}, SkinDecodeError::InvalidImage};
        OwnedBackendBuffer output(new (std::nothrow) ClipShotImageCodecBuffer{},
                                  &releaseBackendBuffer);
        if (!output)
            return {{}, SkinDecodeError::ResourceLimit};
        ClipShotImageCodecSkinInfo info{};
        uint32_t failure = CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_INVALID_IMAGE;
        if (clip_shot_image_codec_decode_skin_rgba8(
                reinterpret_cast<const uint8_t*>(encoded.constData()),
                static_cast<uint64_t>(encoded.size()), output.get(), &info, &failure) == 0) {
            switch (failure) {
            case CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_UNSUPPORTED_FORMAT:
                return {{}, SkinDecodeError::UnsupportedFormat};
            case CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_INPUT_TOO_LARGE:
                return {{}, SkinDecodeError::InputTooLarge};
            case CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_RESOURCE_LIMIT:
                return {{}, SkinDecodeError::ResourceLimit};
            default:
                return {{}, SkinDecodeError::InvalidImage};
            }
        }
        QImage image = takeDecodedImage(std::move(output), QImage::Format_RGBA8888);
        if (image.isNull())
            return {{}, SkinDecodeError::ResourceLimit};
        const QSize decodedSize = image.size();
        const QSize canvasSize(static_cast<int>(info.canvas_width),
                               static_cast<int>(info.canvas_height));
        if (canvasSize.isEmpty())
            return {{}, SkinDecodeError::InvalidImage};
        QSize previewCanvas = canvasSize;
        if (previewCanvas.width() > previewExtent || previewCanvas.height() > previewExtent) {
            const qreal scale =
                static_cast<qreal>(previewExtent) / qMax(canvasSize.width(), canvasSize.height());
            previewCanvas = QSize(qMax(1, qRound(canvasSize.width() * scale)),
                                  qMax(1, qRound(canvasSize.height() * scale)));
        }
        const qreal scaleX = static_cast<qreal>(previewCanvas.width()) / canvasSize.width();
        const qreal scaleY = static_cast<qreal>(previewCanvas.height()) / canvasSize.height();
        const QSize framePreview(qMax(1, qRound(decodedSize.width() * scaleX)),
                                 qMax(1, qRound(decodedSize.height() * scaleY)));
        if (framePreview != image.size())
            image = clipCanvasScaleImage(image, framePreview, Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation);
        if (image.isNull())
            return {{}, SkinDecodeError::ResourceLimit};
        const QColorSpace srgb(QColorSpace::SRgb);
        image =
            image.colorSpace().isValid() && image.colorSpace() != srgb
                ? clipCanvasColorConvertedImage(image, srgb, QImage::Format_ARGB32_Premultiplied)
                : clipCanvasConvertImage(image, QImage::Format_ARGB32_Premultiplied);
        if (image.isNull())
            return {{}, SkinDecodeError::ResourceLimit};
        image.setColorSpace(srgb);
        if (info.frame_x != 0 || info.frame_y != 0 || decodedSize != canvasSize) {
            QImage canvas =
                clipCanvasAllocateZeroedImage(previewCanvas, QImage::Format_ARGB32_Premultiplied);
            if (canvas.isNull())
                return {{}, SkinDecodeError::ResourceLimit};
            canvas.setColorSpace(srgb);
            QPainter painter(&canvas);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.drawImage(QRectF(info.frame_x * scaleX, info.frame_y * scaleY,
                                     decodedSize.width() * scaleX, decodedSize.height() * scaleY),
                              image);
            painter.end();
            image = std::move(canvas);
        }
        QTransform orientation;
        switch (info.orientation) {
        case 2:
            orientation = QTransform(-1, 0, 0, 1, 0, 0);
            break;
        case 3:
            orientation = QTransform(-1, 0, 0, -1, 0, 0);
            break;
        case 4:
            orientation = QTransform(1, 0, 0, -1, 0, 0);
            break;
        case 5:
            orientation = QTransform(0, 1, 1, 0, 0, 0);
            break;
        case 6:
            orientation = QTransform(0, 1, -1, 0, 0, 0);
            break;
        case 7:
            orientation = QTransform(0, -1, -1, 0, 0, 0);
            break;
        case 8:
            orientation = QTransform(0, -1, 1, 0, 0, 0);
            break;
        default:
            break;
        }
        if (!orientation.isIdentity())
            image = clipCanvasTransformImage(image, orientation, Qt::FastTransformation);
        if (image.isNull())
            return {{}, SkinDecodeError::ResourceLimit};
        image.setDevicePixelRatio(1.0);
        return {std::move(image), SkinDecodeError::None};
    } catch (const std::bad_alloc&) {
        return {{}, SkinDecodeError::ResourceLimit};
    } catch (...) {
        return {{}, SkinDecodeError::InvalidImage};
    }
}

bool inspectFile(const QString& path, clip::image::Format expectedFormat,
                 const QSize& expectedSize) {
    return inspectBytes(readFile(path), expectedFormat, expectedSize);
}

} // namespace clip_shot::image_codec
