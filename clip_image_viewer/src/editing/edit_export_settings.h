#pragma once

#include <clip/image/codec.h>
#include <clip/image/processing.h>
#include <clip/image/service.h>

#include <QMetaType>
#include <QSize>

namespace clip::image_viewer {

enum class EditChangeKind : quint8 { discrete, dimension_typing, continuous };

enum class OutputColorIntent : quint8 { sdr_srgb, hdr_preserve };
enum class OutputAlphaIntent : quint8 { preserve, composite_black };
struct EditExportSettings final {
    QSize sourceSize;
    int width = 0;
    int height = 0;
    clip::image::ResamplingMethod resampling = clip::image::ResamplingMethod::lanczos3;
    bool premultiplyAlpha = true;
    bool linearRgb = true;
    bool maintainAspectRatio = true;
    bool reducePalette = false;
    int paletteColors = 256;
    int ditheringPercent = 100;
    clip::image::Format format = clip::image::Format::png;
    clip::image::EncodeOptions encode;

    bool isValid() const {
        if (!(sourceSize.isValid() && width > 0 && height > 0 && paletteColors >= 2 &&
              paletteColors <= 256 && ditheringPercent >= 0 && ditheringPercent <= 100 &&
              format != clip::image::Format::unknown))
            return false;
        clip::image::Service service;
        const clip::image::EncoderInfo* encoder = service.encoder_info(format);
        return encoder && static_cast<std::uint32_t>(width) <= encoder->limits.maximum_width &&
               static_cast<std::uint32_t>(height) <= encoder->limits.maximum_height;
    }

    friend bool operator==(const EditExportSettings& left, const EditExportSettings& right) {
        return left.sourceSize == right.sourceSize && left.width == right.width &&
               left.height == right.height && left.resampling == right.resampling &&
               left.premultiplyAlpha == right.premultiplyAlpha &&
               left.linearRgb == right.linearRgb &&
               left.maintainAspectRatio == right.maintainAspectRatio &&
               left.reducePalette == right.reducePalette &&
               left.paletteColors == right.paletteColors &&
               left.ditheringPercent == right.ditheringPercent && left.format == right.format &&
               left.encode.format == right.encode.format &&
               left.encode.quality == right.encode.quality &&
               left.encode.effort == right.encode.effort &&
               left.encode.lossless_effort == right.encode.lossless_effort &&
               left.encode.lossless == right.encode.lossless &&
               left.encode.preserve_metadata == right.encode.preserve_metadata &&
               left.encode.progressive == right.encode.progressive &&
               left.encode.interlaced == right.encode.interlaced &&
               left.encode.compression_level == right.encode.compression_level &&
               left.encode.chroma_subsampling == right.encode.chroma_subsampling;
    }
};

struct BaseRasterKey final {
    quint64 sourceGeneration = 0;
    int width = 0;
    int height = 0;
    clip::image::ResamplingMethod resampling = clip::image::ResamplingMethod::lanczos3;
    bool premultiplyAlpha = true;
    bool linearRgb = true;
    clip::image::AnimationPolicy animationPolicy = clip::image::AnimationPolicy::first_frame;

    friend bool operator==(const BaseRasterKey&, const BaseRasterKey&) = default;
};

struct ExportKey final {
    BaseRasterKey base;
    clip::image::Format format = clip::image::Format::unknown;
    clip::image::AnimationPolicy animationPolicy = clip::image::AnimationPolicy::first_frame;
    OutputColorIntent colorIntent = OutputColorIntent::sdr_srgb;
    OutputAlphaIntent alphaIntent = OutputAlphaIntent::preserve;
    bool reducePalette = false;
    int paletteColors = 0;
    int ditheringPercent = 0;
    int quality = 0;
    int effort = 0;
    int losslessEffort = 0;
    int compressionLevel = 0;
    bool lossless = false;
    bool preserveMetadata = false;
    bool progressive = false;
    bool interlaced = false;
    std::optional<clip::image::ChromaSubsampling> chromaSubsampling;

    friend bool operator==(const ExportKey&, const ExportKey&) = default;
};

inline clip::image::AnimationPolicy animationPolicyForFormat(clip::image::Format format) {
    switch (format) {
    case clip::image::Format::gif:
    case clip::image::Format::webp:
    case clip::image::Format::avif:
    case clip::image::Format::heif:
    case clip::image::Format::jxl:
        return clip::image::AnimationPolicy::preserve;
    default:
        return clip::image::AnimationPolicy::first_frame;
    }
}

inline BaseRasterKey baseRasterKey(const EditExportSettings& settings, quint64 sourceGeneration) {
    return {sourceGeneration,
            settings.width,
            settings.height,
            settings.resampling,
            settings.premultiplyAlpha,
            settings.linearRgb,
            animationPolicyForFormat(settings.format)};
}

inline ExportKey exportKey(const EditExportSettings& settings, quint64 sourceGeneration) {
    clip::image::EncodeOptions normalized = settings.encode;
    clip::image::Service service;
    if (const clip::image::EncoderInfo* encoder = service.encoder_info(settings.format)) {
        auto result = clip::image::normalize_encode_options(*encoder, settings.encode);
        if (result)
            normalized = std::move(result).value();
    }
    const bool hdr =
        settings.format == clip::image::Format::avif || settings.format == clip::image::Format::jxl;
    const bool noAlpha = settings.format == clip::image::Format::jpeg;
    const auto format = settings.format;
    const bool quality = format == clip::image::Format::jpeg ||
                         format == clip::image::Format::webp ||
                         format == clip::image::Format::avif ||
                         format == clip::image::Format::heif || format == clip::image::Format::jxl;
    const bool effort = format == clip::image::Format::webp ||
                        format == clip::image::Format::avif ||
                        format == clip::image::Format::heif || format == clip::image::Format::jxl;
    const bool lossless = format == clip::image::Format::webp ||
                          format == clip::image::Format::avif ||
                          format == clip::image::Format::heif || format == clip::image::Format::jxl;
    std::optional<clip::image::ChromaSubsampling> normalizedSampling;
    if (format == clip::image::Format::jpeg) {
        auto resolved = clip::image::resolve_jpeg_chroma_subsampling(normalized, false);
        if (resolved)
            normalizedSampling = resolved.value();
    }
    return {baseRasterKey(settings, sourceGeneration),
            format,
            animationPolicyForFormat(format),
            hdr ? OutputColorIntent::hdr_preserve : OutputColorIntent::sdr_srgb,
            noAlpha ? OutputAlphaIntent::composite_black : OutputAlphaIntent::preserve,
            settings.reducePalette,
            settings.reducePalette ? settings.paletteColors : 0,
            settings.reducePalette ? settings.ditheringPercent : 0,
            quality ? normalized.quality : 0,
            effort ? normalized.effort : 0,
            format == clip::image::Format::webp && normalized.lossless ? normalized.lossless_effort
                                                                       : 0,
            format == clip::image::Format::png ? normalized.compression_level : 0,
            lossless && normalized.lossless,
            normalized.preserve_metadata,
            format == clip::image::Format::jpeg && normalized.progressive,
            format == clip::image::Format::png && normalized.interlaced,
            normalizedSampling};
}

inline ExportKey preparationKey(const EditExportSettings& settings, quint64 sourceGeneration) {
    ExportKey key = exportKey(settings, sourceGeneration);
    key.quality = 0;
    key.effort = 0;
    key.losslessEffort = 0;
    key.compressionLevel = 0;
    key.lossless = false;
    key.progressive = false;
    key.interlaced = false;
    key.chromaSubsampling.reset();
    return key;
}

} // namespace clip::image_viewer

Q_DECLARE_METATYPE(clip::image_viewer::EditExportSettings)
Q_DECLARE_METATYPE(clip::image_viewer::EditChangeKind)
