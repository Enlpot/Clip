#include "editing/worker_core.h"

#include "editing/raster_package.h"
#include "editing/worker_protocol.h"

#include <clip/image/io.h>
#include <clip/image/processing.h>
#include <clip/image/raster_conversion.h>
#include <clip/image/service.h>

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QThread>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>

namespace clip::image_viewer::worker_core {
namespace {

std::filesystem::path nativePath(const QString& path) {
    return std::filesystem::path(path.toStdU16String());
}

QString statusText(const clip::image::Status& status) {
    const QString message = QString::fromStdString(status.message).trimmed();
    return message.isEmpty() ? QStringLiteral("The image operation failed.") : message;
}

clip::image::Result<clip::image::Document> prepareColorForExport(clip::image::Document document,
                                                                 clip::image::Format format,
                                                                 std::stop_token stop) {
    const bool pngNativeSamples = std::all_of(
        document.frames.begin(), document.frames.end(), [](const clip::image::Frame& frame) {
            const clip::image::PixelFormat& pixel = frame.image.format();
            return pixel.sample_type == clip::image::SampleType::unsigned_integer &&
                   (pixel.bits_per_channel == 8 || pixel.bits_per_channel == 16) &&
                   pixel.channels != clip::image::ChannelLayout::cmyk &&
                   pixel.channels != clip::image::ChannelLayout::indexed;
        });
    if (format == clip::image::Format::png && pngNativeSamples)
        return document;
    if (format == clip::image::Format::png || format == clip::image::Format::jpeg ||
        format == clip::image::Format::webp || format == clip::image::Format::heif) {
        return clip::image::convert_to_sdr_srgb(document, true, stop);
    }
    if (format == clip::image::Format::avif) {
        const bool hdr = document.color.dynamic_range == clip::image::DynamicRange::high ||
                         std::any_of(document.frames.begin(), document.frames.end(),
                                     [](const clip::image::Frame& frame) {
                                         return frame.image.format().sample_type ==
                                                clip::image::SampleType::floating_point;
                                     });
        if (hdr)
            return clip::image::convert_to_hdr_rec2020_pq16(document, stop);
    }
    return document;
}

bool sameColor(const clip::image::ColorEncoding& left, const clip::image::ColorEncoding& right) {
    return left.primaries == right.primaries && left.transfer == right.transfer &&
           left.dynamic_range == right.dynamic_range && left.icc_profile == right.icc_profile &&
           left.source_peak_nits == right.source_peak_nits &&
           left.diffuse_white_nits == right.diffuse_white_nits &&
           left.mastering_primaries_and_white == right.mastering_primaries_and_white &&
           left.max_content_light_level == right.max_content_light_level &&
           left.max_frame_average_light_level == right.max_frame_average_light_level;
}

bool previewEquivalent(const clip::image::Document& base, const clip::image::Document& prepared) {
    if (base.canvas_width != prepared.canvas_width ||
        base.canvas_height != prepared.canvas_height || base.loop_count != prepared.loop_count ||
        base.frames.size() != prepared.frames.size() || !sameColor(base.color, prepared.color))
        return false;
    for (std::size_t index = 0; index < base.frames.size(); ++index) {
        const auto& left = base.frames[index];
        const auto& right = prepared.frames[index];
        if (left.x != right.x || left.y != right.y || left.duration != right.duration ||
            left.blend != right.blend || left.disposal != right.disposal ||
            left.image.width() != right.image.width() ||
            left.image.height() != right.image.height() ||
            left.image.format() != right.image.format() ||
            left.image.row_stride() != right.image.row_stride() ||
            left.image.storage().owner() != right.image.storage().owner() ||
            left.image.pixels().data() != right.image.pixels().data() ||
            left.image.pixels().size() != right.image.pixels().size() ||
            !sameColor(left.color, right.color))
            return false;
    }
    return true;
}

bool descriptorHasAlpha(const clip::image::DocumentDescriptor& descriptor) {
    return std::any_of(descriptor.frames.begin(), descriptor.frames.end(),
                       [](const clip::image::RasterFrameDescriptor& frame) {
                           return std::any_of(
                               frame.layout.planes.begin(), frame.layout.planes.end(),
                               [](const clip::image::PlaneDescriptor& plane) {
                                   return plane.semantic == clip::image::PlaneSemantic::alpha ||
                                          plane.format.alpha != clip::image::AlphaMode::none;
                               });
                       });
}

bool jpegPackedCompatible(const clip::image::PixelFormat& pixel) {
    if (pixel.sample_type != clip::image::SampleType::unsigned_integer ||
        pixel.bits_per_channel != 8)
        return false;
    return pixel.channels == clip::image::ChannelLayout::gray ||
           pixel.channels == clip::image::ChannelLayout::rgb ||
           pixel.channels == clip::image::ChannelLayout::rgba ||
           pixel.channels == clip::image::ChannelLayout::bgr ||
           pixel.channels == clip::image::ChannelLayout::bgra;
}

clip::image::Result<clip::image::Document>
materializePackedRaster(const clip::image::RasterSource& source, std::stop_token stop) {
    const clip::image::DocumentDescriptor& descriptor = source.descriptor();
    if (descriptor.frames.size() != 1)
        return clip::image::Status::error(clip::image::ErrorCode::unsupported_feature,
                                          "The native raster fallback requires one frame.");
    const clip::image::RasterFrameDescriptor& frame = descriptor.frames.front();
    auto allocated =
        clip::image::MutableImage::allocate(frame.width, frame.height, clip::image::kRgba8);
    if (!allocated)
        return allocated.error();
    clip::image::MutableImage pixels = std::move(allocated).value();
    clip::image::MutablePlaneView destination{frame.width, frame.height, clip::image::kRgba8,
                                              pixels.row_stride(), pixels.pixels()};
    auto converted = clip::image::read_rgba8_region(source, 0, {0, 0, frame.width, frame.height},
                                                    destination, stop);
    if (!converted)
        return converted.error();
    clip::image::Document document;
    document.format = descriptor.format;
    document.canvas_width = descriptor.canvas_width;
    document.canvas_height = descriptor.canvas_height;
    document.loop_count = descriptor.loop_count;
    document.metadata = descriptor.metadata;
    document.color = descriptor.color;
    clip::image::Frame packed;
    packed.image = std::move(pixels).freeze();
    packed.x = frame.x;
    packed.y = frame.y;
    packed.duration = frame.duration;
    packed.blend = frame.blend;
    packed.disposal = frame.disposal;
    packed.metadata = frame.metadata;
    packed.color = frame.color;
    packed.cursor_hotspot = frame.cursor_hotspot;
    document.frames.push_back(std::move(packed));
    return document;
}

bool validateNativeReceipt(const clip::image::EncodedArtifactReceipt& receipt,
                           const clip::image::DocumentDescriptor& descriptor,
                           const EditExportSettings& settings, QString* error) {
    const auto fail = [&](const QString& message) {
        if (error)
            *error = message;
        return false;
    };
    if (!receipt.encoder_finalized_and_sink_flushed || receipt.format != settings.format ||
        receipt.document_kind != clip::image::DocumentKind::raster ||
        receipt.canvas_width != descriptor.canvas_width ||
        receipt.canvas_height != descriptor.canvas_height || receipt.emitted_frame_count != 1 ||
        receipt.emitted_frame_extents.size() != 1)
        return fail(QStringLiteral("The native raster receipt is invalid."));
    const auto& frame = descriptor.frames.front();
    const auto& extent = receipt.emitted_frame_extents.front();
    if (extent.x != 0 || extent.y != 0 || extent.width != frame.width ||
        extent.height != frame.height)
        return fail(QStringLiteral("The native receipt does not match its raster."));
    if (settings.format == clip::image::Format::jpeg) {
        const bool grayscale = frame.layout.color_model == clip::image::ColorModel::gray;
        const auto expected =
            clip::image::resolve_jpeg_chroma_subsampling(settings.encode, grayscale);
        if (!expected || receipt.jpeg_chroma_subsampling != expected.value())
            return fail(
                QStringLiteral("The native JPEG receipt sampling does not match its raster."));
    } else if (receipt.jpeg_chroma_subsampling) {
        return fail(QStringLiteral("A native non-JPEG receipt contains JPEG sampling metadata."));
    }
    return true;
}

clip::image::Result<clip::image::AlphaContent>
classifyDocumentAlpha(const clip::image::Document& document, std::stop_token stop) {
    for (const clip::image::Frame& frame : document.frames) {
        const auto classified = clip::image::classify_alpha(frame.image, stop);
        if (!classified)
            return classified.error();
        if (classified.value() == clip::image::AlphaContent::non_opaque)
            return clip::image::AlphaContent::non_opaque;
    }
    return clip::image::AlphaContent::opaque;
}

bool validateReceipt(const clip::image::EncodedArtifactReceipt& receipt,
                     const clip::image::Document& document, const EditExportSettings& settings,
                     QString* error) {
    const auto fail = [&](const QString& message) {
        if (error)
            *error = message;
        return false;
    };
    if (!receipt.encoder_finalized_and_sink_flushed || receipt.format != settings.format ||
        receipt.document_kind != clip::image::DocumentKind::raster ||
        receipt.emitted_frame_count != receipt.emitted_frame_extents.size()) {
        return fail(QStringLiteral("The encoder returned an invalid artifact receipt."));
    }
    const bool firstFrame =
        animationPolicyForFormat(settings.format) == clip::image::AnimationPolicy::first_frame;
    if (document.frames.empty())
        return fail(QStringLiteral("The prepared raster has no frames."));
    const std::size_t expectedCount = firstFrame ? 1U : document.frames.size();
    if (receipt.emitted_frame_extents.size() != expectedCount)
        return fail(
            QStringLiteral("The artifact receipt frame count does not match the export policy."));

    std::uint32_t expectedCanvasWidth = document.canvas_width;
    std::uint32_t expectedCanvasHeight = document.canvas_height;
    if (settings.format == clip::image::Format::ico ||
        settings.format == clip::image::Format::cur) {
        expectedCanvasWidth = 0;
        expectedCanvasHeight = 0;
        for (const auto& frame : document.frames) {
            expectedCanvasWidth = std::max(expectedCanvasWidth, frame.image.width());
            expectedCanvasHeight = std::max(expectedCanvasHeight, frame.image.height());
        }
    } else if (firstFrame) {
        expectedCanvasWidth = document.frames.front().image.width();
        expectedCanvasHeight = document.frames.front().image.height();
    }
    if (receipt.canvas_width != expectedCanvasWidth ||
        receipt.canvas_height != expectedCanvasHeight)
        return fail(
            QStringLiteral("The artifact receipt canvas does not match the prepared raster."));

    if (settings.format == clip::image::Format::jpeg) {
        const bool grayscale =
            document.frames.front().image.format().channels == clip::image::ChannelLayout::gray;
        auto expectedSampling =
            clip::image::resolve_jpeg_chroma_subsampling(settings.encode, grayscale);
        if (!expectedSampling || receipt.jpeg_chroma_subsampling != expectedSampling.value()) {
            return fail(QStringLiteral(
                "The JPEG artifact receipt sampling does not match the export settings."));
        }
    } else if (receipt.jpeg_chroma_subsampling) {
        return fail(QStringLiteral("A non-JPEG artifact receipt contains JPEG sampling metadata."));
    }

    for (std::size_t index = 0; index < expectedCount; ++index) {
        const auto& frame = document.frames[firstFrame ? 0U : index];
        const auto& extent = receipt.emitted_frame_extents[index];
        const bool resetOrigin = firstFrame || settings.format == clip::image::Format::ico ||
                                 settings.format == clip::image::Format::cur;
        if (extent.x != (resetOrigin ? 0U : frame.x) || extent.y != (resetOrigin ? 0U : frame.y) ||
            extent.width != frame.image.width() || extent.height != frame.image.height()) {
            return fail(QStringLiteral(
                "The artifact receipt frame extent does not match the prepared raster."));
        }
    }
    return true;
}

class DocumentRasterSource final : public clip::image::RasterSource {
  public:
    static clip::image::Result<DocumentRasterSource> create(const clip::image::Document& document) {
        auto descriptor = clip::image::describe_document(document);
        if (!descriptor)
            return descriptor.error();
        return DocumentRasterSource(document, std::move(descriptor).value());
    }

    const clip::image::DocumentDescriptor& descriptor() const noexcept override {
        return descriptor_;
    }

    clip::image::RasterAccess access() const noexcept override {
        return clip::image::RasterAccess::sequential_rows | clip::image::RasterAccess::random_rows |
               clip::image::RasterAccess::mapped_planes |
               clip::image::RasterAccess::concurrent_reads;
    }

    clip::image::Result<void> read_rows(std::uint32_t frameIndex, std::uint32_t planeIndex,
                                        std::uint32_t firstRow, std::uint32_t rowCount,
                                        std::size_t destinationStride,
                                        std::span<std::byte> destination,
                                        std::stop_token stop) const override {
        if (planeIndex != 0 || frameIndex >= document_->frames.size()) {
            return clip::image::Status::error(clip::image::ErrorCode::invalid_argument,
                                              "The document raster row request is invalid.");
        }
        const clip::image::Image& image = document_->frames[frameIndex].image;
        const auto bytesPerPixel = image.format().bytes_per_pixel();
        if (!bytesPerPixel || firstRow > image.height() || rowCount == 0 ||
            rowCount > image.height() - firstRow ||
            image.width() > std::numeric_limits<std::size_t>::max() / bytesPerPixel.value()) {
            return clip::image::Status::error(clip::image::ErrorCode::invalid_argument,
                                              "The document raster row range is invalid.");
        }
        const std::size_t rowBytes =
            static_cast<std::size_t>(image.width()) * bytesPerPixel.value();
        if (destinationStride < rowBytes ||
            (rowCount > 1 &&
             destinationStride >
                 (std::numeric_limits<std::size_t>::max() - rowBytes) / (rowCount - 1U)) ||
            destination.size() < destinationStride * (rowCount - 1U) + rowBytes) {
            return clip::image::Status::error(clip::image::ErrorCode::invalid_argument,
                                              "The document raster destination is too small.");
        }
        for (std::uint32_t row = 0; row < rowCount; ++row) {
            if (stop.stop_requested()) {
                return clip::image::Status::error(clip::image::ErrorCode::cancelled,
                                                  "The document raster read was cancelled.");
            }
            std::memcpy(destination.data() + static_cast<std::size_t>(row) * destinationStride,
                        image.pixels().data() +
                            static_cast<std::size_t>(firstRow + row) * image.row_stride(),
                        rowBytes);
        }
        return {};
    }

    clip::image::Result<clip::image::MappedPlane>
    map_plane(std::uint32_t frameIndex, std::uint32_t planeIndex) const override {
        if (planeIndex != 0 || frameIndex >= document_->frames.size()) {
            return clip::image::Status::error(clip::image::ErrorCode::invalid_argument,
                                              "The document raster mapping request is invalid.");
        }
        const clip::image::Image& image = document_->frames[frameIndex].image;
        return clip::image::MappedPlane{image.storage().owner(), image.pixels(),
                                        image.row_stride()};
    }

  private:
    DocumentRasterSource(const clip::image::Document& document,
                         clip::image::DocumentDescriptor descriptor)
        : document_(&document), descriptor_(std::move(descriptor)) {}

    const clip::image::Document* document_ = nullptr;
    clip::image::DocumentDescriptor descriptor_;
};

} // namespace

clip::image::Result<PreparationResult>
prepareForExport(clip::image::Document document, const EditExportSettings& settings,
                 const clip::image::EncoderInfo& encoder, QString* warning, std::stop_token stop,
                 std::optional<clip::image::AlphaContent> verifiedAlphaContent) {
    const clip::image::Document base = document;
    if (verifiedAlphaContent && *verifiedAlphaContent == clip::image::AlphaContent::non_opaque) {
        clip::image::Result<clip::image::DocumentDescriptor> descriptor =
            clip::image::describe_document(document);
        if (!descriptor)
            return descriptor.error();
        if (!descriptorHasAlpha(descriptor.value())) {
            return clip::image::Status::error(
                clip::image::ErrorCode::invalid_argument,
                "Verified non-opaque alpha metadata contradicts the prepared raster.");
        }
    }
    if (document.frames.size() > 1 &&
        !clip::image::has_feature(encoder.features, clip::image::EncoderFeature::animation)) {
        return clip::image::Status::error(
            clip::image::ErrorCode::invalid_argument,
            "The base raster animation policy does not match the selected encoder.");
    }
    Q_UNUSED(warning);
    std::optional<clip::image::AlphaContent> alphaContent = verifiedAlphaContent;
    std::chrono::nanoseconds alphaDuration{0};
    const auto classify = [&]() -> clip::image::Result<void> {
        if (alphaContent)
            return {};
        const auto alphaStart = std::chrono::steady_clock::now();
        const auto classified = classifyDocumentAlpha(document, stop);
        if (!classified)
            return classified.error();
        alphaContent = classified.value();
        alphaDuration += std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - alphaStart);
        return {};
    };

    if (settings.format == clip::image::Format::jpeg && !settings.reducePalette) {
        auto classified = classify();
        if (!classified)
            return classified.error();
        clip::image::SdrConversionOptions options;
        options.verified_alpha_content = *alphaContent;
        if (*alphaContent == clip::image::AlphaContent::non_opaque) {
            options.background = std::array<std::uint8_t, 3>{0, 0, 0};
            options.output_format = clip::image::kRgb8;
        } else if (!document.frames.empty() &&
                   jpegPackedCompatible(document.frames.front().image.format()) &&
                   std::all_of(document.frames.begin(), document.frames.end(),
                               [&](const clip::image::Frame& frame) {
                                   return frame.image.format() ==
                                          document.frames.front().image.format();
                               })) {
            options.output_format = document.frames.front().image.format();
        } else {
            options.output_format = clip::image::kRgb8;
        }
        auto converted = clip::image::convert_to_sdr_srgb(document, options, stop);
        if (!converted)
            return converted.error();
        document = std::move(converted).value();
        if (options.background)
            alphaContent = clip::image::AlphaContent::opaque;
    } else {
        auto converted = prepareColorForExport(std::move(document), settings.format, stop);
        if (!converted)
            return converted.error();
        document = std::move(converted).value();
    }
    if (settings.reducePalette) {
        clip::image::TransformOptions options;
        options.palette =
            clip::image::PaletteOptions{static_cast<std::uint16_t>(settings.paletteColors),
                                        static_cast<float>(settings.ditheringPercent) / 100.0F};
        auto reduced = clip::image::transform(document, options, stop);
        if (!reduced)
            return reduced.error();
        document = std::move(reduced).value();
        alphaContent.reset();
    }
    if (!settings.encode.preserve_metadata) {
        document.metadata = {};
        for (auto& frame : document.frames)
            frame.metadata = {};
    }
    const bool equivalent = previewEquivalent(base, document);
    auto classified = classify();
    if (!classified)
        return classified.error();

    if (!clip::image::has_feature(encoder.features, clip::image::EncoderFeature::alpha) &&
        *alphaContent == clip::image::AlphaContent::non_opaque) {
        clip::image::SdrConversionOptions options;
        options.verified_alpha_content = *alphaContent;
        options.background = std::array<std::uint8_t, 3>{0, 0, 0};
        options.output_format = clip::image::kRgb8;
        auto composited = clip::image::convert_to_sdr_srgb(document, options, stop);
        if (!composited)
            return composited.error();
        document = std::move(composited).value();
        alphaContent = clip::image::AlphaContent::opaque;
    }
    return PreparationResult{std::move(document), equivalent, *alphaContent, alphaDuration};
}

QJsonObject executeEncodeJob(const QJsonObject& job, const EditExportSettings& settings,
                             const ArtifactReadyCallback& artifactReady, std::stop_token stop,
                             bool* artifactPublished) {
    const QString artifactPath = job.value(QStringLiteral("artifactPath")).toString();
    const QString previewPath = job.value(QStringLiteral("previewPath")).toString();
    QJsonObject result{{QStringLiteral("success"), false}};
    QString error;
    QElapsedTimer total;
    QElapsedTimer stage;
    total.start();
    stage.start();
    std::shared_ptr<MappedRasterPackage> base;
    const QJsonObject transport = job.value(QStringLiteral("baseRaster")).toObject();
    const QString transportKind = transport.value(QStringLiteral("kind")).toString();
    if (transportKind == QStringLiteral("verified_file")) {
        base =
            MappedRasterPackage::open(transport.value(QStringLiteral("path")).toString(), &error);
    } else if (transportKind == QStringLiteral("shared_memory")) {
        bool sizeValid = false;
        const std::uint64_t size =
            transport.value(QStringLiteral("size")).toString().toULongLong(&sizeValid);
        std::array<std::byte, 16> nonce{};
        const QJsonArray nonceJson = transport.value(QStringLiteral("nonce")).toArray();
        if (nonceJson.size() != static_cast<qsizetype>(nonce.size())) {
            error = QStringLiteral("The shared raster nonce is malformed.");
        } else {
            bool nonceValid = true;
            for (qsizetype index = 0; index < nonceJson.size(); ++index) {
                const int value = nonceJson.at(index).toInt(-1);
                if (value < 0 || value > 255) {
                    nonceValid = false;
                    break;
                }
                nonce[static_cast<std::size_t>(index)] = static_cast<std::byte>(value);
            }
            if (!nonceValid)
                error = QStringLiteral("The shared raster nonce is malformed.");
            else if (!sizeValid || size == 0)
                error = QStringLiteral("The shared raster size is malformed.");
            else {
                base = MappedRasterPackage::openShared(
                    transport.value(QStringLiteral("key")).toString(), size, nonce, &error);
            }
        }
    } else {
        error = QStringLiteral("The base raster transport is malformed.");
    }
    if (!base) {
        result.insert(QStringLiteral("error"), error);
        if (transportKind == QStringLiteral("shared_memory") &&
            error.startsWith(QStringLiteral("Could not attach shared raster storage:"))) {
            result.insert(QStringLiteral("retriableSharedMemoryAttach"), true);
        }
        return result;
    }
    result.insert(QStringLiteral("openBaseNs"), QString::number(stage.nsecsElapsed()));
    stage.restart();
    clip::image::Service service;
    const clip::image::EncoderInfo* encoder = service.encoder_info(settings.format);
    if (!encoder) {
        result.insert(QStringLiteral("error"),
                      QStringLiteral("The selected encoder is unavailable."));
        return result;
    }
    QString warning;
    if (job.value(QStringLiteral("sourceFrameCount")).toInt(1) > 1 &&
        animationPolicyForFormat(settings.format) == clip::image::AnimationPolicy::first_frame) {
        warning = QStringLiteral("Only the first frame will be exported.");
    }
    std::optional<clip::image::AlphaContent> verifiedAlphaContent;
    const QJsonValue verifiedAlphaValue = job.value(QStringLiteral("verifiedAlphaContent"));
    if (!verifiedAlphaValue.isUndefined()) {
        const int value = verifiedAlphaValue.toInt(-1);
        if (value < static_cast<int>(clip::image::AlphaContent::opaque) ||
            value > static_cast<int>(clip::image::AlphaContent::non_opaque)) {
            result.insert(
                QStringLiteral("error"),
                QStringLiteral("The encode job contains invalid verified alpha metadata."));
            return result;
        }
        verifiedAlphaContent = static_cast<clip::image::AlphaContent>(value);
    }
    const std::optional<clip::image::AlphaContent> packageAlpha = base->verifiedAlphaContent();
    if (verifiedAlphaContent && packageAlpha && *verifiedAlphaContent != *packageAlpha) {
        result.insert(
            QStringLiteral("error"),
            QStringLiteral("The encode job alpha metadata does not match the raster package."));
        return result;
    }
    if (!verifiedAlphaContent)
        verifiedAlphaContent = packageAlpha;
    clip::image::Result<clip::image::EncodeOptions> normalized =
        clip::image::normalize_encode_options(*encoder, settings.encode);
    if (!normalized) {
        result.insert(QStringLiteral("error"), statusText(normalized.error()));
        return result;
    }
    clip::image::Result<clip::image::RasterEncodeRoute> selectedRoute =
        service.raster_encode_route(base->source().descriptor(), normalized.value());
    if (!selectedRoute) {
        result.insert(QStringLiteral("error"), statusText(selectedRoute.error()));
        return result;
    }
    const bool directNative =
        !settings.reducePalette && selectedRoute.value() == clip::image::RasterEncodeRoute::native;
    clip::image::AlphaContent directAlpha = clip::image::AlphaContent::opaque;
    if (directNative) {
        directAlpha = verifiedAlphaContent.value_or(descriptorHasAlpha(base->source().descriptor())
                                                        ? clip::image::AlphaContent::non_opaque
                                                        : clip::image::AlphaContent::opaque);
    }
    std::optional<PreparationResult> prepared;
    std::shared_ptr<MappedRasterPackage> preparedPackage;
    const auto cachedPreparation = job.value(QStringLiteral("preparedRaster")).toObject();
    if (!directNative && !cachedPreparation.isEmpty()) {
        preparedPackage = MappedRasterPackage::open(
            cachedPreparation.value(QStringLiteral("path")).toString(), &error);
        const auto* document = preparedPackage ? preparedPackage->document() : nullptr;
        const auto alpha = preparedPackage ? preparedPackage->verifiedAlphaContent() : std::nullopt;
        if (!document || !alpha || document->frames.empty() ||
            document->frames.size() != base->source().descriptor().frames.size() ||
            document->frames.front().image.width() != static_cast<std::uint32_t>(settings.width) ||
            document->frames.front().image.height() !=
                static_cast<std::uint32_t>(settings.height)) {
            result.insert(QStringLiteral("error"),
                          error.isEmpty()
                              ? QStringLiteral("The prepared preview document is unavailable.")
                              : error);
            return result;
        }
        prepared = PreparationResult{
            *document, job.value(QStringLiteral("preparedPreviewEquivalentToBase")).toBool(),
            *alpha, std::chrono::nanoseconds{0}};
        result.insert(QStringLiteral("preparationCacheHit"), true);
    } else if (!directNative) {
        std::optional<clip::image::Document> materialized;
        const clip::image::Document* packed = base->document();
        if (!packed) {
            auto fallback = materializePackedRaster(base->source(), stop);
            if (!fallback) {
                result.insert(QStringLiteral("error"), statusText(fallback.error()));
                return result;
            }
            materialized = std::move(fallback).value();
            packed = &*materialized;
        }
        auto preparedResult =
            prepareForExport(*packed, settings, *encoder, &warning, stop, verifiedAlphaContent);
        if (!preparedResult) {
            result.insert(QStringLiteral("error"), statusText(preparedResult.error()));
            return result;
        }
        prepared = std::move(preparedResult).value();
        const QString preparedPath = job.value(QStringLiteral("preparedPath")).toString();
        bool limitValid = false;
        const auto limit =
            job.value(QStringLiteral("preparedCacheLimit")).toString().toULongLong(&limitValid);
        std::uint64_t pixels = 0;
        bool withinLimit = limitValid;
        for (const auto& frame : prepared->document.frames) {
            if (frame.image.pixels().size() > limit - pixels) {
                withinLimit = false;
                break;
            }
            pixels += frame.image.pixels().size();
        }
        if (!preparedPath.isEmpty() && withinLimit) {
            const QString partial = preparedPath + QStringLiteral(".partial");
            preparedPackage = MappedRasterPackage::create(partial, prepared->document, &error, {},
                                                          prepared->alphaContent);
            if (preparedPackage && preparedPackage->mappedBytes() <= limit) {
                preparedPackage.reset();
                if (QFile::rename(partial, preparedPath)) {
                    preparedPackage = MappedRasterPackage::open(preparedPath, &error);
                    if (preparedPackage && preparedPackage->document()) {
                        prepared->document = *preparedPackage->document();
                        result.insert(QStringLiteral("preparedPath"), preparedPath);
                        result.insert(QStringLiteral("preparedPreviewEquivalentToBase"),
                                      prepared->previewEquivalentToBase);
                    } else {
                        preparedPackage.reset();
                    }
                }
            } else {
                preparedPackage.reset();
            }
            QFile::remove(partial);
            if (!preparedPackage)
                QFile::remove(preparedPath);
            // A cache publication failure does not invalidate the prepared export.
            error.clear();
        }
    }
    result.insert(QStringLiteral("alphaContent"),
                  static_cast<int>(directNative ? directAlpha : prepared->alphaContent));
    result.insert(
        QStringLiteral("alphaClassificationNs"),
        QString::number(directNative ? 0 : prepared->alphaClassificationDuration.count()));
    result.insert(QStringLiteral("prepareNs"), QString::number(stage.nsecsElapsed()));
    stage.restart();

    const QString partialArtifact = artifactPath + QStringLiteral(".partial");
    auto output = clip::image::file_output(nativePath(partialArtifact));
    if (!output) {
        result.insert(QStringLiteral("error"), statusText(output.error()));
        return result;
    }
    clip::image::EncodeOptions options = std::move(normalized).value();
    options.format = settings.format;
    options.verified_alpha_content = directNative ? directAlpha : prepared->alphaContent;
    auto encoded = [&]() -> clip::image::Result<clip::image::EncodeResult> {
        if (directNative)
            return service.encode(base->source(), output.value(), options, stop);
        auto preparedSource = DocumentRasterSource::create(prepared->document);
        if (!preparedSource)
            return preparedSource.error();
        return service.encode(preparedSource.value(), output.value(), options, stop);
    }();
    output.value().sink.reset();
    if (!encoded) {
        QFile::remove(partialArtifact);
        result.insert(QStringLiteral("error"), statusText(encoded.error()));
        return result;
    }
    QString receiptError;
    const bool validReceipt =
        directNative
            ? validateNativeReceipt(encoded.value().receipt, base->source().descriptor(), settings,
                                    &receiptError)
            : validateReceipt(encoded.value().receipt, prepared->document, settings, &receiptError);
    if (!validReceipt) {
        QFile::remove(partialArtifact);
        result.insert(QStringLiteral("error"), receiptError);
        return result;
    }
    const QFileInfo partialInfo(partialArtifact);
    if (!partialInfo.isFile() || partialInfo.size() <= 0 ||
        static_cast<std::uint64_t>(partialInfo.size()) != encoded.value().bytes_written) {
        QFile::remove(partialArtifact);
        result.insert(
            QStringLiteral("error"),
            QStringLiteral("The encoded partial artifact size does not match the receipt."));
        return result;
    }
    if (!QFile::rename(partialArtifact, artifactPath)) {
        QFile::remove(partialArtifact);
        result.insert(QStringLiteral("error"),
                      QStringLiteral("The worker could not publish the encoded artifact."));
        return result;
    }
    QFile::setPermissions(artifactPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    result.insert(QStringLiteral("encodeNs"), QString::number(stage.nsecsElapsed()));
    if (directNative) {
        result.insert(QStringLiteral("directNativeEncodeNs"),
                      result.value(QStringLiteral("encodeNs")));
    }
    stage.restart();

    auto artifactInput = clip::image::file_input(nativePath(artifactPath));
    if (!artifactInput) {
        result.insert(QStringLiteral("error"), statusText(artifactInput.error()));
        return result;
    }
    const QString partialPreview = previewPath + QStringLiteral(".partial");
    const clip::image::Document* preparedDocument = prepared ? &prepared->document : nullptr;
    result.insert(QStringLiteral("validateReceiptNs"), QString::number(stage.nsecsElapsed()));
    stage.restart();

    const bool pixelExact = encoded.value().round_trip == clip::image::PixelRoundTrip::exact;
    const bool basePreview = pixelExact && (!prepared || prepared->previewEquivalentToBase);
    const QString previewKind = basePreview  ? QStringLiteral("base")
                                : pixelExact ? QStringLiteral("prepared")
                                             : QStringLiteral("codec");
    QJsonObject publication{
        {QStringLiteral("artifactPath"), artifactPath},
        {QStringLiteral("artifactBytes"), QString::number(encoded.value().bytes_written)},
        {QStringLiteral("warning"), warning},
        {QStringLiteral("previewKind"), previewKind},
        {QStringLiteral("pixelExact"), pixelExact},
        {QStringLiteral("receipt"), worker_protocol::receiptToJson(encoded.value().receipt)},
        {QStringLiteral("alphaContent"), result.value(QStringLiteral("alphaContent"))},
        {QStringLiteral("alphaClassificationNs"),
         result.value(QStringLiteral("alphaClassificationNs"))},
        {QStringLiteral("prepareNs"), result.value(QStringLiteral("prepareNs"))},
        {QStringLiteral("encodeNs"), result.value(QStringLiteral("encodeNs"))},
        {QStringLiteral("validateReceiptNs"), result.value(QStringLiteral("validateReceiptNs"))}};
    if (result.contains(QStringLiteral("preparedPath"))) {
        publication.insert(QStringLiteral("preparedPath"),
                           result.value(QStringLiteral("preparedPath")));
        publication.insert(QStringLiteral("preparedPreviewEquivalentToBase"),
                           result.value(QStringLiteral("preparedPreviewEquivalentToBase")));
    }
    if (result.value(QStringLiteral("preparationCacheHit")).toBool())
        publication.insert(QStringLiteral("preparationCacheHit"), true);
    const bool previewUsesPreparedRaster =
        previewKind == QStringLiteral("prepared") && preparedPackage;
    if (previewUsesPreparedRaster)
        publication.insert(QStringLiteral("previewUsesPreparedRaster"), true);
    if (directNative) {
        publication.insert(QStringLiteral("directNativeEncodeNs"),
                           result.value(QStringLiteral("directNativeEncodeNs")));
    }
    artifactReady(std::move(publication));
    if (artifactPublished)
        *artifactPublished = true;
    if (job.value(QStringLiteral("testMode")).toString() ==
        QStringLiteral("block-preview-cooperative")) {
        while (!stop.stop_requested())
            QThread::msleep(1);
        result.insert(QStringLiteral("error"), QStringLiteral("The preview was cancelled."));
        return result;
    }
    if (job.value(QStringLiteral("testMode")).toString() == QStringLiteral("preview-failure")) {
        result.insert(QStringLiteral("error"), QStringLiteral("Simulated exact preview failure."));
        return result;
    }
    if (previewKind == QStringLiteral("prepared") && !previewUsesPreparedRaster) {
        if (!prepared || preparedDocument == nullptr) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("The prepared preview document is unavailable."));
            return result;
        }
        auto preview = MappedRasterPackage::create(partialPreview, *preparedDocument, &error, {},
                                                   prepared->alphaContent);
        if (!preview) {
            result.insert(QStringLiteral("error"), error);
            return result;
        }
        preview.reset();
    } else if (previewKind == QStringLiteral("codec")) {
        base.reset();
        if (prepared)
            prepared->document = {};
        preparedPackage.reset();
        clip::image::Result<void> streamed;
        if (settings.format == clip::image::Format::jpeg) {
            clip::image::DecodeOptions decodeOptions;
            decodeOptions.raster_layout = clip::image::RasterLayoutPolicy::native;
            decodeOptions.preserve_metadata = false;
            clip::image::RasterStoreOptions storeOptions;
            storeOptions.analysis.alpha_content = clip::image::AlphaContent::opaque;
            auto decoded =
                service.decode_to_store(artifactInput.value(), nativePath(partialPreview),
                                        decodeOptions, storeOptions, stop);
            if (decoded)
                decoded.value().reset();
            else
                streamed = decoded.error();
        } else {
            auto sink = std::make_unique<MappedRasterSink>(partialPreview);
            streamed = service.decode_to_sink(artifactInput.value(), *sink, {}, stop);
            sink.reset();
        }
        if (!streamed) {
            QFile::remove(partialPreview);
            result.insert(QStringLiteral("error"), statusText(streamed.error()));
            return result;
        }
        result.insert(QStringLiteral("artifactDecodeNs"), QString::number(stage.nsecsElapsed()));
        stage.restart();
    }
    if (previewKind != QStringLiteral("base") && !previewUsesPreparedRaster) {
        if (!QFile::rename(partialPreview, previewPath)) {
            QFile::remove(partialPreview);
            result.insert(QStringLiteral("error"),
                          QStringLiteral("The worker could not publish the preview package."));
            return result;
        }
        QFile::setPermissions(previewPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        result.insert(QStringLiteral("previewPath"), previewPath);
        result.insert(QStringLiteral("previewPackageNs"), QString::number(stage.nsecsElapsed()));
    }
    if (stop.stop_requested()) {
        result.insert(QStringLiteral("error"), QStringLiteral("The export was cancelled."));
        return result;
    }
    result.insert(QStringLiteral("success"), true);
    result.insert(QStringLiteral("warning"), warning);
    result.insert(QStringLiteral("previewKind"), previewKind);
    result.insert(QStringLiteral("pixelExact"), pixelExact);
    result.insert(QStringLiteral("artifactBytes"), QString::number(encoded.value().bytes_written));
    result.insert(QStringLiteral("totalNs"), QString::number(total.nsecsElapsed()));
    return result;
}

QJsonObject executePreviewJob(const QString& artifactPath, const QString& previewPath,
                              clip::image::Format format, std::stop_token stop) {
    QJsonObject result{{QStringLiteral("success"), false},
                       {QStringLiteral("artifactPublished"), true}};
    const QString partialPreview = previewPath + QStringLiteral(".partial");
    auto input = clip::image::file_input(nativePath(artifactPath));
    if (!input) {
        result.insert(QStringLiteral("error"), statusText(input.error()));
        return result;
    }
    QElapsedTimer timer;
    timer.start();
    clip::image::Service service;
    clip::image::Result<void> decoded;
    if (format == clip::image::Format::jpeg) {
        clip::image::DecodeOptions decodeOptions;
        decodeOptions.raster_layout = clip::image::RasterLayoutPolicy::native;
        decodeOptions.preserve_metadata = false;
        clip::image::RasterStoreOptions storeOptions;
        storeOptions.analysis.alpha_content = clip::image::AlphaContent::opaque;
        auto store = service.decode_to_store(input.value(), nativePath(partialPreview),
                                             decodeOptions, storeOptions, stop);
        if (store)
            store.value().reset();
        else
            decoded = store.error();
    } else {
        auto sink = std::make_unique<MappedRasterSink>(partialPreview);
        decoded = service.decode_to_sink(input.value(), *sink, {}, stop);
        sink.reset();
    }
    if (!decoded) {
        QFile::remove(partialPreview);
        result.insert(QStringLiteral("error"), statusText(decoded.error()));
        return result;
    }
    if (!QFile::rename(partialPreview, previewPath)) {
        QFile::remove(partialPreview);
        result.insert(QStringLiteral("error"),
                      QStringLiteral("The worker could not publish the preview package."));
        return result;
    }
    QFile::setPermissions(previewPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    result.insert(QStringLiteral("success"), true);
    result.insert(QStringLiteral("previewKind"), QStringLiteral("codec"));
    result.insert(QStringLiteral("previewPath"), previewPath);
    result.insert(QStringLiteral("artifactDecodeNs"), QString::number(timer.nsecsElapsed()));
    return result;
}

} // namespace clip::image_viewer::worker_core
