#include "decoding/clip_image_decoder.h"

#include "core/image_raster_store.h"
#include "core/image_tile_store.h"

#include <clip/image/service.h>
#include <clip/image/processing.h>

#include <QFileInfo>
#include <QPainter>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace clip::image_viewer {
namespace {

constexpr std::uint64_t kMaximumResidentBytes = std::uint64_t{384} << 20U;
constexpr std::uint64_t kMaximumTileStripeBytes = std::uint64_t{128} << 20U;
constexpr std::uint64_t kMaximumTileCacheBytes = std::uint64_t{16} << 30U;
constexpr int kTileWidth = 512;
constexpr int kPreviewExtent = 4096;

QString statusText(const clip::image::Status& status) {
    QString message = QString::fromStdString(status.message).trimmed();
    if (message.isEmpty()) {
        message = QStringLiteral("clip_image could not decode this image.");
    }
    if (!status.codec.empty()) {
        message += QStringLiteral(" (%1)").arg(QString::fromStdString(status.codec));
    }
    return message;
}

clip::image::Result<clip::image::RasterAnalysis>
analyzeDocument(const clip::image::Document& document, std::stop_token stop) {
    clip::image::RasterAnalysis analysis;
    analysis.alpha_content = clip::image::AlphaContent::opaque;
    for (const clip::image::Frame& frame : document.frames) {
        const auto content = clip::image::classify_alpha(frame.image, stop);
        if (!content)
            return content.error();
        if (content.value() == clip::image::AlphaContent::non_opaque) {
            analysis.alpha_content = content.value();
            break;
        }
    }
    return analysis;
}

std::filesystem::path nativePath(const QString& filePath) {
    return std::filesystem::path(filePath.toStdU16String());
}

QColorSpace::Primaries qtPrimaries(clip::image::ColorPrimaries primaries) {
    switch (primaries) {
    case clip::image::ColorPrimaries::display_p3:
        return QColorSpace::Primaries::DciP3D65;
    case clip::image::ColorPrimaries::adobe_rgb:
        return QColorSpace::Primaries::AdobeRgb;
    case clip::image::ColorPrimaries::rec2020:
        return QColorSpace::Primaries::Bt2020;
    case clip::image::ColorPrimaries::srgb:
    case clip::image::ColorPrimaries::custom:
    case clip::image::ColorPrimaries::unknown:
        return QColorSpace::Primaries::SRgb;
    }
    return QColorSpace::Primaries::SRgb;
}

QColorSpace::TransferFunction qtTransfer(clip::image::TransferFunction transfer) {
    switch (transfer) {
    case clip::image::TransferFunction::linear:
        return QColorSpace::TransferFunction::Linear;
    case clip::image::TransferFunction::pq:
        return QColorSpace::TransferFunction::St2084;
    case clip::image::TransferFunction::hlg:
        return QColorSpace::TransferFunction::Hlg;
    case clip::image::TransferFunction::gamma:
        return QColorSpace::TransferFunction::Gamma;
    case clip::image::TransferFunction::srgb:
    case clip::image::TransferFunction::unknown:
        return QColorSpace::TransferFunction::SRgb;
    }
    return QColorSpace::TransferFunction::SRgb;
}

ColorMetadata colorMetadata(const clip::image::ColorEncoding& color) {
    ColorMetadata metadata;
    if (!color.icc_profile.empty() &&
        color.icc_profile.size() <=
            static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        const QByteArray profile(reinterpret_cast<const char*>(color.icc_profile.data()),
                                 static_cast<qsizetype>(color.icc_profile.size()));
        metadata.sourceColorSpace = QColorSpace::fromIccProfile(profile);
    }
    if (!metadata.sourceColorSpace.isValid()) {
        metadata.sourceColorSpace =
            QColorSpace(qtPrimaries(color.primaries), qtTransfer(color.transfer));
    }
    metadata.dynamicRange = color.dynamic_range == clip::image::DynamicRange::high
                                ? DynamicRange::High
                                : DynamicRange::Standard;
    metadata.sourcePeakNits = color.source_peak_nits;
    metadata.diffuseWhiteNits = color.diffuse_white_nits;
    return metadata;
}

void releaseWrappedImage(void* owner) {
    delete static_cast<std::shared_ptr<const void>*>(owner);
}

struct ResidentImageOwner final {
    // Destruction is reverse declaration order: release the mapping before the
    // backing-store owner attempts to remove its file on Windows.
    std::shared_ptr<ImageRasterStore> backingStore;
    std::shared_ptr<const void> mapping;
};

void releaseResidentImage(void* owner) {
    delete static_cast<ResidentImageOwner*>(owner);
}

QImage wrapImage(const clip::image::Image& source, QString* error) {
    if (source.width() == 0 || source.height() == 0 ||
        source.width() > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        source.height() > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        source.row_stride() > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        *error = QStringLiteral("The decoded image dimensions exceed Qt's raster limits.");
        return {};
    }
    const clip::image::PixelFormat& format = source.format();
    QImage::Format qtFormat = QImage::Format_Invalid;
    if (format == clip::image::kGray8)
        qtFormat = QImage::Format_Grayscale8;
    else if (format == clip::image::kGray16)
        qtFormat = QImage::Format_Grayscale16;
    else if (format == clip::image::kRgb8)
        qtFormat = QImage::Format_RGB888;
    else if (format == clip::image::kRgba8)
        qtFormat = QImage::Format_RGBA8888;
    else if (format == clip::image::kBgra8)
        qtFormat = QImage::Format_ARGB32;
    else if (format == clip::image::kRgba16)
        qtFormat = QImage::Format_RGBA64;
    else if (format == clip::image::kRgba16Float)
        qtFormat = QImage::Format_RGBA16FPx4;
    else if (format == clip::image::kRgba32Float)
        qtFormat = QImage::Format_RGBA32FPx4;
    if (qtFormat == QImage::Format_Invalid) {
        *error = QStringLiteral("The decoded pixel layout is not supported by the viewer.");
        return {};
    }

    if (!source.storage().owner()) {
        *error = QStringLiteral("The decoded image storage has no lifetime owner.");
        return {};
    }
    auto* owner = new std::shared_ptr<const void>(source.storage().owner());
    QImage image(reinterpret_cast<const uchar*>(source.pixels().data()),
                 static_cast<int>(source.width()), static_cast<int>(source.height()),
                 static_cast<qsizetype>(source.row_stride()), qtFormat, &releaseWrappedImage,
                 owner);
    if (image.isNull()) {
        delete owner;
        *error = QStringLiteral("The decoded image could not be wrapped for display.");
    }
    return image;
}

class TileSink final : public clip::image::PixelSink {
  public:
    explicit TileSink(const DecodeCancellation& cancellation) : cancellation_(cancellation) {}

    ~TileSink() override {
        rasterStore_.reset();
        backingStore_.reset();
    }

    clip::image::Result<void> begin(const clip::image::DocumentInfo& document) override {
        if (document.canvas_width == 0 || document.canvas_height == 0 ||
            document.canvas_width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
            document.canvas_height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
            return failure(clip::image::ErrorCode::limit_exceeded,
                           "The tiled image dimensions exceed viewer limits.");
        }
        sourceSize_ = QSize(static_cast<int>(document.canvas_width),
                            static_cast<int>(document.canvas_height));
        if (document.frames.size() != 1 || document.frames.front().width != document.canvas_width ||
            document.frames.front().height != document.canvas_height ||
            document.frames.front().x != 0 || document.frames.front().y != 0 ||
            document.frames.front().native_format != clip::image::kRgba8) {
            return failure(clip::image::ErrorCode::unsupported_feature,
                           "Oversized tiled decoding requires one full-canvas RGBA8 frame.");
        }
        rowBytes_ = static_cast<std::size_t>(document.canvas_width) * 4U;
        if (rowBytes_ == 0 || rowBytes_ > kMaximumTileStripeBytes) {
            return failure(clip::image::ErrorCode::limit_exceeded,
                           "One image row exceeds the tile working-memory budget.");
        }
        stripeRows_ = static_cast<int>(std::max<std::uint64_t>(
            1, std::min<std::uint64_t>(512, kMaximumTileStripeBytes / rowBytes_)));
        stripe_.resize(rowBytes_ * static_cast<std::size_t>(stripeRows_));

        const QSize previewSize =
            sourceSize_.scaled(kPreviewExtent, kPreviewExtent, Qt::KeepAspectRatio);
        preview_ = QImage(previewSize, QImage::Format_RGBA8888_Premultiplied);
        if (preview_.isNull()) {
            return failure(clip::image::ErrorCode::io_error,
                           "Could not create the temporary image tile cache.");
        }
        clip::image::Result<clip::image::DocumentDescriptor> descriptor =
            clip::image::describe_document(document);
        if (!descriptor)
            return descriptor.error();
        descriptor.value().frames.front().layout.alpha = clip::image::AlphaMode::premultiplied;
        clip::image::RasterStoreOptions options;
        options.chunk_rows = static_cast<std::uint32_t>(stripeRows_);
        options.limits.maximum_pixel_bytes = kMaximumTileCacheBytes;
        QString error;
        backingStore_ = ImageRasterStore::create(std::move(descriptor).value(), options, &error);
        if (!backingStore_) {
            return failure(clip::image::ErrorCode::io_error, error.toUtf8().constData());
        }
        rasterStore_ = backingStore_->store();
        preview_.fill(Qt::transparent);
        preview_.setColorSpace(QColorSpace(QColorSpace::SRgb));
        return {};
    }

    clip::image::Result<void> begin_frame(std::uint32_t frameIndex,
                                          const clip::image::FrameInfo& frame) override {
        if (frameIndex != 0 || frame.width != static_cast<std::uint32_t>(sourceSize_.width()) ||
            frame.height != static_cast<std::uint32_t>(sourceSize_.height()) || frame.x != 0 ||
            frame.y != 0 || frame.native_format != clip::image::kRgba8) {
            return failure(clip::image::ErrorCode::unsupported_feature,
                           "Oversized tiled decoding requires one full-canvas RGBA8 frame.");
        }
        if (!rasterStore_) {
            return failure(clip::image::ErrorCode::internal_error,
                           "The temporary raster store was not initialized.");
        }
        return {};
    }

    clip::image::Result<void> write_rows(std::uint32_t firstRow, std::uint32_t rowCount,
                                         std::size_t rowStride,
                                         std::span<const std::byte> pixels) override {
        if (firstRow != expectedRow_ || rowStride < rowBytes_ ||
            rowCount > (pixels.size() / rowStride)) {
            return failure(clip::image::ErrorCode::corrupt_data,
                           "The streaming decoder emitted an invalid row sequence.");
        }
        for (std::uint32_t relative = 0; relative < rowCount; ++relative) {
            if (cancellation_.isCancelled()) {
                return failure(clip::image::ErrorCode::cancelled, "Image decoding was cancelled.");
            }
            const std::byte* source =
                pixels.data() + static_cast<std::size_t>(relative) * rowStride;
            std::byte* destination =
                stripe_.data() + static_cast<std::size_t>(stripeRowsFilled_) * rowBytes_;
            for (int x = 0; x < sourceSize_.width(); ++x) {
                const std::size_t offset = static_cast<std::size_t>(x) * 4U;
                const std::uint32_t alpha = std::to_integer<std::uint8_t>(source[offset + 3U]);
                if (alpha != 0xFFU)
                    alphaOpaque_ = false;
                destination[offset] = static_cast<std::byte>(
                    (std::to_integer<std::uint8_t>(source[offset]) * alpha + 127U) / 255U);
                destination[offset + 1U] = static_cast<std::byte>(
                    (std::to_integer<std::uint8_t>(source[offset + 1U]) * alpha + 127U) / 255U);
                destination[offset + 2U] = static_cast<std::byte>(
                    (std::to_integer<std::uint8_t>(source[offset + 2U]) * alpha + 127U) / 255U);
                destination[offset + 3U] = source[offset + 3U];
            }
            writePreviewRow(expectedRow_, destination);
            ++expectedRow_;
            ++stripeRowsFilled_;
            if (stripeRowsFilled_ == stripeRows_ ||
                expectedRow_ == static_cast<std::uint32_t>(sourceSize_.height())) {
                clip::image::Result<void> status = flushStripe();
                if (!status)
                    return status;
            }
        }
        return {};
    }

    clip::image::Result<void> end_frame(std::uint32_t frameIndex) override {
        if (frameIndex != 0 || expectedRow_ != static_cast<std::uint32_t>(sourceSize_.height()) ||
            stripeRowsFilled_ != 0) {
            return failure(clip::image::ErrorCode::truncated_data,
                           "The streaming decoder did not emit a complete image.");
        }
        return {};
    }

    clip::image::Result<void> end() override {
        if (!rasterStore_) {
            return failure(clip::image::ErrorCode::internal_error,
                           "The temporary raster store was not initialized.");
        }
        backingStore_->setVerifiedAlphaContent(alphaOpaque_
                                                   ? clip::image::AlphaContent::opaque
                                                   : clip::image::AlphaContent::non_opaque);
        clip::image::Result<void> committed = rasterStore_->commit();
        if (!committed)
            return committed;
        store_ = std::make_shared<ImageTileStore>(backingStore_, sourceSize_, preview_,
                                                  QSize(kTileWidth, stripeRows_));
        rasterStore_.reset();
        backingStore_.reset();
        return {};
    }

    std::shared_ptr<ImageTileStore> takeStore() {
        return std::move(store_);
    }

  private:
    clip::image::Status failure(clip::image::ErrorCode code, const char* message) const {
        return clip::image::Status::error(code, message, "clip_image_viewer tile cache");
    }

    void writePreviewRow(std::uint32_t sourceY, const std::byte* source) {
        while (nextPreviewRow_ < preview_.height() &&
               (static_cast<qint64>(nextPreviewRow_) * sourceSize_.height()) / preview_.height() ==
                   sourceY) {
            auto* destination = reinterpret_cast<std::byte*>(preview_.scanLine(nextPreviewRow_));
            for (int x = 0; x < preview_.width(); ++x) {
                const int sourceX = static_cast<int>(
                    (static_cast<qint64>(x) * sourceSize_.width()) / preview_.width());
                std::memcpy(destination + static_cast<std::size_t>(x) * 4U,
                            source + static_cast<std::size_t>(sourceX) * 4U, 4U);
            }
            ++nextPreviewRow_;
        }
    }

    clip::image::Result<void> flushStripe() {
        const int stripeStartY = static_cast<int>(expectedRow_) - stripeRowsFilled_;
        if (!rasterStore_) {
            return failure(clip::image::ErrorCode::internal_error,
                           "The temporary raster store was not initialized.");
        }
        clip::image::Result<void> written = rasterStore_->write_rows(
            0, 0, static_cast<std::uint32_t>(stripeStartY),
            static_cast<std::uint32_t>(stripeRowsFilled_), rowBytes_,
            std::span<const std::byte>(stripe_.data(),
                                       rowBytes_ * static_cast<std::size_t>(stripeRowsFilled_)),
            cancellation_.token());
        if (!written)
            return written;
        stripeRowsFilled_ = 0;
        return {};
    }

    const DecodeCancellation& cancellation_;
    std::shared_ptr<ImageRasterStore> backingStore_;
    std::shared_ptr<clip::image::RasterStore> rasterStore_;
    QSize sourceSize_;
    QImage preview_;
    std::vector<std::byte> stripe_;
    std::shared_ptr<ImageTileStore> store_;
    std::size_t rowBytes_ = 0;
    std::uint32_t expectedRow_ = 0;
    bool alphaOpaque_ = true;
    int stripeRows_ = 0;
    int stripeRowsFilled_ = 0;
    int nextPreviewRow_ = 0;
};

std::uint64_t residentEstimate(const clip::image::DocumentInfo& info) {
    std::uint64_t total = 0;
    const bool highPrecisionWorking =
        info.color.dynamic_range == clip::image::DynamicRange::high ||
        (info.color.primaries != clip::image::ColorPrimaries::unknown &&
         info.color.primaries != clip::image::ColorPrimaries::srgb) ||
        (info.color.transfer != clip::image::TransferFunction::unknown &&
         info.color.transfer != clip::image::TransferFunction::srgb);
    for (const clip::image::FrameInfo& frame : info.frames) {
        clip::image::Result<std::size_t> bytesPerPixel = frame.native_format.bytes_per_pixel();
        std::uint64_t pixelBytes = bytesPerPixel ? bytesPerPixel.value() : 16U;
        const bool directSrgbRgba =
            !highPrecisionWorking && frame.native_format == clip::image::kRgba8;
        if (!directSrgbRgba) {
            const std::uint64_t workingBytes = highPrecisionWorking ? 8U : 4U;
            if (pixelBytes > std::numeric_limits<std::uint64_t>::max() - workingBytes)
                return std::numeric_limits<std::uint64_t>::max();
            pixelBytes += workingBytes;
        }
        const std::uint64_t pixels = static_cast<std::uint64_t>(frame.width) * frame.height;
        if (pixels > (std::numeric_limits<std::uint64_t>::max() - total) / pixelBytes) {
            return std::numeric_limits<std::uint64_t>::max();
        }
        total += pixels * pixelBytes;
    }
    return total;
}

bool hasStreamingCapability(const clip::image::Service& service, clip::image::Format format) {
    for (const clip::image::FormatCapability& capability : service.formats()) {
        if (capability.format == format) {
            return clip::image::has_capability(capability.capabilities,
                                               clip::image::CodecCapability::streaming_decode);
        }
    }
    return false;
}

DecodeResult tiledDecode(const QString& filePath, clip::image::DecodeSession& session,
                         const clip::image::DocumentInfo& info,
                         const DecodeCancellation& cancellation) {
    TileSink sink(cancellation);
    clip::image::DecodeOptions options;
    options.orientation = clip::image::OrientationPolicy::apply;
    options.output_format = clip::image::kRgba8;
    options.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
    options.limits.maximum_working_bytes = std::uint64_t{256} << 20U;
    clip::image::Result<void> decoded = session.decode_to_sink(sink, options, cancellation.token());
    if (!decoded)
        return DecodeResult::failure(statusText(decoded.error()));
    std::shared_ptr<ImageTileStore> store = sink.takeStore();
    if (!store) {
        return DecodeResult::failure(QStringLiteral("The tiled decoder returned no image cache."));
    }

    DecodedImage result;
    result.filePath = QFileInfo(filePath).absoluteFilePath();
    result.sourceSize = store->sourceSize();
    result.pixels = store->preview();
    result.pixelEncoding = PixelEncoding::Srgb8;
    result.color = colorMetadata(info.color);
    result.color.sourceColorSpace = QColorSpace(QColorSpace::SRgb);
    result.color.description = QStringLiteral("sRGB tiled image");
    result.color.transferDescription = QStringLiteral("sRGB");
    result.decoderName = QString::fromUtf8(session.codec_name().data(),
                                           static_cast<qsizetype>(session.codec_name().size()));
    result.rasterStore = store->rasterStore();
    if (result.rasterStore) {
        result.analysis = result.rasterStore->analysis();
    }
    result.tileStore = std::move(store);
    return {std::move(result), {}};
}

bool supportsNativeTileStore(const clip::image::DocumentDescriptor& descriptor) {
    if (descriptor.frames.size() != 1 || descriptor.canvas_width == 0 ||
        descriptor.canvas_height == 0 ||
        descriptor.canvas_width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        descriptor.canvas_height > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
        return false;
    const clip::image::RasterFrameDescriptor& frame = descriptor.frames.front();
    if (frame.width != descriptor.canvas_width || frame.height != descriptor.canvas_height ||
        frame.x != 0 || frame.y != 0 || frame.layout.planes.empty() ||
        frame.layout.planes.front().semantic == clip::image::PlaneSemantic::packed)
        return false;
    return frame.layout.color_model == clip::image::ColorModel::gray ||
           frame.layout.color_model == clip::image::ColorModel::ycbcr;
}

DecodeResult nativeTiledDecode(const QString& filePath, clip::image::DecodeSession& session,
                               clip::image::DocumentDescriptor descriptor,
                               const DecodeCancellation& cancellation) {
    clip::image::RasterStoreOptions storeOptions;
    storeOptions.chunk_rows = 256;
    storeOptions.limits.maximum_pixel_bytes = kMaximumTileCacheBytes;
    bool alphaCapable = false;
    for (const auto& plane : descriptor.frames.front().layout.planes) {
        alphaCapable = alphaCapable || plane.format.alpha != clip::image::AlphaMode::none;
    }
    if (!alphaCapable)
        storeOptions.analysis.alpha_content = clip::image::AlphaContent::opaque;
    QString backingError;
    std::shared_ptr<ImageRasterStore> backingStore =
        ImageRasterStore::create(descriptor, storeOptions, &backingError);
    if (!backingStore)
        return DecodeResult::failure(backingError);
    const std::shared_ptr<clip::image::RasterStore>& rasterStore = backingStore->store();

    clip::image::DecodeOptions nativeOptions;
    nativeOptions.orientation = clip::image::OrientationPolicy::apply;
    nativeOptions.raster_layout = clip::image::RasterLayoutPolicy::native;
    nativeOptions.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
    nativeOptions.limits.maximum_working_bytes = std::uint64_t{256} << 20U;
    clip::image::Result<void> decoded =
        session.decode_into(*rasterStore, nativeOptions, cancellation.token());
    if (!decoded)
        return DecodeResult::failure(statusText(decoded.error()));

    clip::image::DecodeOptions previewOptions;
    previewOptions.orientation = clip::image::OrientationPolicy::apply;
    previewOptions.output_format = clip::image::kRgba8;
    previewOptions.maximum_extent = kPreviewExtent;
    previewOptions.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
    previewOptions.limits.maximum_working_bytes = std::uint64_t{256} << 20U;
    clip::image::Result<clip::image::Document> previewDocument =
        session.decode(previewOptions, cancellation.token());
    if (!previewDocument)
        return DecodeResult::failure(statusText(previewDocument.error()));
    const QString decoderName = QString::fromUtf8(
        session.codec_name().data(), static_cast<qsizetype>(session.codec_name().size()));
    DecodeResult preview = prepareClipDocument(filePath, std::move(previewDocument).value(),
                                               decoderName, cancellation);
    if (!preview.succeeded())
        return preview;
    if (preview.image.analysis.alpha_content)
        backingStore->setAnalysis(preview.image.analysis);

    const QSize sourceSize(static_cast<int>(descriptor.canvas_width),
                           static_cast<int>(descriptor.canvas_height));
    std::shared_ptr<ImageTileStore> store = std::make_shared<ImageTileStore>(
        backingStore, sourceSize, preview.image.pixels, QSize(kTileWidth, 256));
    preview.image.sourceSize = sourceSize;
    preview.image.rasterStore = std::move(backingStore);
    preview.image.tileStore = std::move(store);
    return preview;
}

qint64 frameDurationMilliseconds(std::chrono::nanoseconds duration) {
    if (duration.count() <= 0)
        return 100;
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return std::max<qint64>(1, milliseconds.count());
}

bool supportsPackedResidentStore(const clip::image::DocumentDescriptor& descriptor) {
    if (descriptor.frames.size() != 1 || descriptor.canvas_width == 0 ||
        descriptor.canvas_height == 0 ||
        descriptor.canvas_width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        descriptor.canvas_height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    const clip::image::RasterFrameDescriptor& frame = descriptor.frames.front();
    if (frame.layout.planes.size() != 1)
        return false;
    const clip::image::PixelFormat& format = frame.layout.planes.front().format;
    return frame.width == descriptor.canvas_width && frame.height == descriptor.canvas_height &&
           frame.x == 0 && frame.y == 0 &&
           frame.layout.planes.front().semantic == clip::image::PlaneSemantic::packed &&
           (format == clip::image::kRgba8 || format == clip::image::kRgba16);
}

DecodeResult residentRasterDecode(const QString& filePath, clip::image::DecodeSession& session,
                                  clip::image::DocumentDescriptor descriptor,
                                  const DecodeCancellation& cancellation) {
    clip::image::RasterStoreOptions storeOptions;
    storeOptions.chunk_rows = 256;
    storeOptions.limits.maximum_pixel_bytes = kMaximumTileCacheBytes;
    QString backingError;
    std::shared_ptr<ImageRasterStore> backingStore =
        ImageRasterStore::create(descriptor, storeOptions, &backingError);
    if (!backingStore)
        return DecodeResult::failure(backingError);

    clip::image::DecodeOptions options;
    options.orientation = clip::image::OrientationPolicy::apply;
    options.output_format = descriptor.frames.front().layout.planes.front().format;
    options.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
    options.limits.maximum_working_bytes = std::uint64_t{256} << 20U;
    const auto decoded = session.decode_into(*backingStore->store(), options, cancellation.token());
    if (!decoded)
        return DecodeResult::failure(statusText(decoded.error()));

    const clip::image::RasterFrameDescriptor& frame = descriptor.frames.front();
    auto mapped = backingStore->store()->map_plane(0, 0);
    if (!mapped)
        return DecodeResult::failure(statusText(mapped.error()));
    if (mapped.value().row_stride >
        static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        return DecodeResult::failure(
            QStringLiteral("The decoded raster stride exceeds Qt's image limit."));
    }
    const clip::image::PixelFormat& format = frame.layout.planes.front().format;
    const auto alphaContent = clip::image::classify_alpha(
        clip::image::ImageView{frame.width, frame.height, format, mapped.value().row_stride,
                               mapped.value().pixels},
        cancellation.token());
    if (!alphaContent)
        return DecodeResult::failure(statusText(alphaContent.error()));
    backingStore->setVerifiedAlphaContent(alphaContent.value());
    clip::image::RasterAnalysis analysis;
    analysis.alpha_content = alphaContent.value();
    const QImage::Format qtFormat =
        format == clip::image::kRgba16 ? QImage::Format_RGBA64 : QImage::Format_RGBA8888;
    auto* owner = new ResidentImageOwner{backingStore, mapped.value().owner};
    QImage pixels(reinterpret_cast<const uchar*>(mapped.value().pixels.data()),
                  static_cast<int>(frame.width), static_cast<int>(frame.height),
                  static_cast<qsizetype>(mapped.value().row_stride), qtFormat,
                  &releaseResidentImage, owner);
    if (pixels.isNull()) {
        delete owner;
        return DecodeResult::failure(
            QStringLiteral("The decoded raster could not be mapped for display."));
    }
    ColorMetadata metadata = colorMetadata(descriptor.color);
    pixels.setColorSpace(metadata.sourceColorSpace);
    const QString decoderName = QString::fromUtf8(
        session.codec_name().data(), static_cast<qsizetype>(session.codec_name().size()));
    DecodeResult prepared = prepareDecodedImage(filePath, std::move(pixels), std::move(metadata),
                                                decoderName, analysis);
    if (!prepared.succeeded())
        return prepared;
    prepared.image.rasterStore = std::move(backingStore);
    return prepared;
}

DecodeResult residentDecode(const QString& filePath, clip::image::DecodeSession& session,
                            const DecodeCancellation& cancellation) {
    clip::image::DecodeOptions options;
    options.orientation = clip::image::OrientationPolicy::apply;
    options.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
    clip::image::Result<clip::image::Document> decoded =
        session.decode(options, cancellation.token());
    if (!decoded)
        return DecodeResult::failure(statusText(decoded.error()));
    clip::image::Document document = std::move(decoded).value();
    const QString decoderName = QString::fromUtf8(
        session.codec_name().data(), static_cast<qsizetype>(session.codec_name().size()));
    return prepareClipDocument(filePath, std::move(document), decoderName, cancellation);
}

} // namespace

DecodeResult prepareClipDocument(const QString& filePath, clip::image::Document document,
                                 const QString& decoderName,
                                 const DecodeCancellation& cancellation) {
    if (document.frames.empty()) {
        return DecodeResult::failure(
            QStringLiteral("The image document contains no display frame."));
    }
    const auto documentAnalysis = analyzeDocument(document, cancellation.token());
    if (!documentAnalysis)
        return DecodeResult::failure(statusText(documentAnalysis.error()));
    const ColorMetadata metadata = colorMetadata(document.color);
    const bool animated =
        document.frames.size() > 1 &&
        std::any_of(document.frames.begin(), document.frames.end(),
                    [](const clip::image::Frame& frame) { return frame.duration.count() > 0; });
    if (!animated) {
        QString error;
        QImage image = wrapImage(document.frames.front().image, &error);
        if (image.isNull())
            return DecodeResult::failure(error);
        image.setColorSpace(metadata.sourceColorSpace);
        DecodeResult result = prepareDecodedImage(filePath, std::move(image), metadata, decoderName,
                                                  documentAnalysis.value());
        if (result.succeeded() && document.canvas_width > 0 && document.canvas_height > 0 &&
            document.canvas_width <= static_cast<std::uint32_t>(std::numeric_limits<int>::max()) &&
            document.canvas_height <= static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
            result.image.sourceSize = QSize(static_cast<int>(document.canvas_width),
                                            static_cast<int>(document.canvas_height));
        }
        return result;
    }

    if (document.canvas_width == 0 || document.canvas_height == 0 ||
        document.canvas_width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        document.canvas_height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return DecodeResult::failure(
            QStringLiteral("The animation canvas dimensions are invalid."));
    }
    const QSize canvasSize(static_cast<int>(document.canvas_width),
                           static_cast<int>(document.canvas_height));
    QImage canvas(canvasSize, QImage::Format_RGBA8888_Premultiplied);
    if (canvas.isNull()) {
        return DecodeResult::failure(
            QStringLiteral("The animation canvas could not be allocated."));
    }
    canvas.fill(Qt::transparent);

    DecodedImage resultImage;
    for (const clip::image::Frame& frame : document.frames) {
        if (cancellation.isCancelled()) {
            return DecodeResult::failure(QStringLiteral("Image decoding was cancelled."));
        }
        QString error;
        QImage source = wrapImage(frame.image, &error);
        if (source.isNull())
            return DecodeResult::failure(error);
        source = source.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
        const QImage previous =
            frame.disposal == clip::image::FrameDisposal::previous ? canvas.copy() : QImage{};
        {
            QPainter painter(&canvas);
            painter.setCompositionMode(frame.blend == clip::image::FrameBlend::source
                                           ? QPainter::CompositionMode_Source
                                           : QPainter::CompositionMode_SourceOver);
            painter.drawImage(QPoint(static_cast<int>(frame.x), static_cast<int>(frame.y)), source);
        }
        DecodeResult prepared = prepareDecodedImage(filePath, canvas.copy(), metadata, decoderName,
                                                    documentAnalysis.value());
        if (!prepared.succeeded())
            return prepared;
        prepared.image.sourceSize = canvasSize;
        if (resultImage.filePath.isEmpty())
            resultImage = prepared.image;
        resultImage.animationFrames.push_back(
            {prepared.image.pixels, frameDurationMilliseconds(frame.duration)});

        if (frame.disposal == clip::image::FrameDisposal::background) {
            QPainter painter(&canvas);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(QRect(static_cast<int>(frame.x), static_cast<int>(frame.y),
                                   source.width(), source.height()),
                             Qt::transparent);
        } else if (frame.disposal == clip::image::FrameDisposal::previous) {
            canvas = previous;
        }
    }
    resultImage.loopCount =
        document.loop_count > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
            ? std::numeric_limits<int>::max()
            : static_cast<int>(document.loop_count);
    resultImage.pixels = resultImage.animationFrames.front().pixels;
    resultImage.analysis = documentAnalysis.value();
    return {std::move(resultImage), {}};
}

DecodeResult prepareMappedClipDocument(const QString& filePath,
                                       const clip::image::Document& document,
                                       const QString& decoderName,
                                       const DecodeCancellation& cancellation) {
    const bool animated =
        document.frames.size() > 1 &&
        std::any_of(document.frames.begin(), document.frames.end(),
                    [](const clip::image::Frame& frame) { return frame.duration.count() > 0; });
    if (animated) {
        return prepareClipDocument(filePath, document, decoderName, cancellation);
    }
    if (document.frames.empty()) {
        return DecodeResult::failure(QStringLiteral("The preview package has no display frame."));
    }
    const auto documentAnalysis = analyzeDocument(document, cancellation.token());
    if (!documentAnalysis)
        return DecodeResult::failure(statusText(documentAnalysis.error()));
    QString error;
    QImage pixels = wrapImage(document.frames.front().image, &error);
    if (pixels.isNull())
        return DecodeResult::failure(error);
    ColorMetadata metadata = colorMetadata(document.color);
    if (!metadata.sourceColorSpace.isValid())
        metadata.sourceColorSpace = QColorSpace(QColorSpace::SRgb);
    pixels.setColorSpace(metadata.sourceColorSpace);
    DecodedImage image;
    image.filePath = QFileInfo(filePath).absoluteFilePath();
    image.sourceSize =
        QSize(static_cast<int>(document.canvas_width), static_cast<int>(document.canvas_height));
    image.pixels = std::move(pixels);
    image.pixelEncoding = document.frames.front().image.format().sample_type ==
                                  clip::image::SampleType::floating_point
                              ? PixelEncoding::LinearScRgb16F
                              : PixelEncoding::Srgb8;
    image.pixelsPremultiplied = false;
    image.color = std::move(metadata);
    image.decoderName = decoderName;
    image.analysis = documentAnalysis.value();
    return {std::move(image), {}};
}

bool ClipImageDecoder::canDecode(const QString& filePath, const QByteArray& header) const {
    static const clip::image::Service service;
    const auto bytes =
        std::as_bytes(std::span(header.constData(), static_cast<std::size_t>(header.size())));
    const std::string hint = QFileInfo(filePath).fileName().toUtf8().toStdString();
    const clip::image::Input input = clip::image::memory_input(bytes, hint);
    if (service.detect(input))
        return true;
    return clip::image::format_from_extension(hint) != clip::image::Format::unknown;
}

DecodeResult ClipImageDecoder::decode(const QString& filePath,
                                      const DecodeCancellation& cancellation) const {
    if (cancellation.isCancelled()) {
        return DecodeResult::failure(QStringLiteral("Image decoding was cancelled."));
    }
    clip::image::Result<clip::image::Input> input = clip::image::file_input(nativePath(filePath));
    if (!input)
        return DecodeResult::failure(statusText(input.error()));
    clip::image::Service service;
    clip::image::Result<clip::image::DecodeSession> opened =
        service.open_decoder(input.value(), cancellation.token());
    if (!opened)
        return DecodeResult::failure(statusText(opened.error()));
    clip::image::DecodeSession session = std::move(opened).value();
    clip::image::DecodeOptions inspectOptions;
    inspectOptions.orientation = clip::image::OrientationPolicy::apply;
    inspectOptions.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
    clip::image::Result<clip::image::DocumentInfo> inspected =
        session.inspect(inspectOptions, cancellation.token());
    if (!inspected)
        return DecodeResult::failure(statusText(inspected.error()));

    if (residentEstimate(inspected.value()) > kMaximumResidentBytes &&
        hasStreamingCapability(service, session.format())) {
        clip::image::DecodeOptions nativeOptions;
        nativeOptions.orientation = clip::image::OrientationPolicy::apply;
        nativeOptions.raster_layout = clip::image::RasterLayoutPolicy::native;
        nativeOptions.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
        nativeOptions.limits.maximum_working_bytes = std::uint64_t{256} << 20U;
        clip::image::Result<clip::image::DocumentDescriptor> nativeDescriptor =
            session.inspect_raster(nativeOptions, cancellation.token());
        if (!nativeDescriptor)
            return DecodeResult::failure(statusText(nativeDescriptor.error()));
        if (supportsNativeTileStore(nativeDescriptor.value())) {
            return nativeTiledDecode(filePath, session, std::move(nativeDescriptor).value(),
                                     cancellation);
        }
        return tiledDecode(filePath, session, inspected.value(), cancellation);
    }
    if (session.format() == clip::image::Format::jpeg ||
        session.format() == clip::image::Format::webp) {
        clip::image::DecodeOptions nativeOptions;
        nativeOptions.orientation = clip::image::OrientationPolicy::apply;
        nativeOptions.raster_layout = clip::image::RasterLayoutPolicy::native;
        nativeOptions.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
        nativeOptions.limits.maximum_working_bytes = std::uint64_t{256} << 20U;
        clip::image::Result<clip::image::DocumentDescriptor> nativeDescriptor =
            session.inspect_raster(nativeOptions, cancellation.token());
        if (nativeDescriptor && supportsNativeTileStore(nativeDescriptor.value())) {
            return nativeTiledDecode(filePath, session, std::move(nativeDescriptor).value(),
                                     cancellation);
        }
    }
    if (hasStreamingCapability(service, session.format())) {
        clip::image::DecodeOptions packedOptions;
        packedOptions.orientation = clip::image::OrientationPolicy::apply;
        packedOptions.limits.maximum_owned_output_bytes = kMaximumResidentBytes;
        packedOptions.limits.maximum_working_bytes = std::uint64_t{256} << 20U;
        auto descriptor = session.inspect_raster(packedOptions, cancellation.token());
        if (descriptor && supportsPackedResidentStore(descriptor.value())) {
            return residentRasterDecode(filePath, session, std::move(descriptor).value(),
                                        cancellation);
        }
    }
    return residentDecode(filePath, session, cancellation);
}

} // namespace clip::image_viewer
