#include "clip_shot/presentation/screenshotimagefileservice.h"
#include "clipimageqtcodec.h"

namespace {
int compressionValue(const clip::image::EncoderOptionRange& range,
                     ScreenshotCompressionLevel level) {
    switch (level) {
    case ScreenshotCompressionLevel::Low:
        return range.minimum;
    case ScreenshotCompressionLevel::Medium:
        return range.default_value;
    case ScreenshotCompressionLevel::High:
        return range.maximum;
    }
    return range.minimum;
}
} // namespace

clip::image::Format ScreenshotImageFileService::clipImageFormat(ScreenshotImageFileFormat format) {
    switch (format) {
    case ScreenshotImageFileFormat::Pdf:
        return clip::image::Format::unknown;
    case ScreenshotImageFileFormat::Png:
        return clip::image::Format::png;
    case ScreenshotImageFileFormat::Jpeg:
        return clip::image::Format::jpeg;
    case ScreenshotImageFileFormat::Bmp:
        return clip::image::Format::bmp;
    case ScreenshotImageFileFormat::Webp:
        return clip::image::Format::webp;
    case ScreenshotImageFileFormat::Jxl:
        return clip::image::Format::jxl;
    case ScreenshotImageFileFormat::Avif:
        return clip::image::Format::avif;
    }
    return clip::image::Format::unknown;
}

bool ScreenshotImageFileService::supportsQuality(ScreenshotImageFileFormat format) {
    if (format == ScreenshotImageFileFormat::Pdf)
        return true;
    const auto encoder = clip_shot::image_codec::encoderInfo(clipImageFormat(format));
    return encoder.has_value() &&
           clip::image::has_feature(encoder->features, clip::image::EncoderFeature::quality);
}

bool ScreenshotImageFileService::supportsCompressionLevel(ScreenshotImageFileFormat format) {
    if (format == ScreenshotImageFileFormat::Pdf)
        return false;
    const auto encoder = clip_shot::image_codec::encoderInfo(clipImageFormat(format));
    if (!encoder.has_value())
        return false;
    return clip::image::has_feature(encoder->features,
                                    clip::image::EncoderFeature::compression_level) ||
           clip::image::has_feature(encoder->features, clip::image::EncoderFeature::effort) ||
           (clip::image::has_feature(encoder->features, clip::image::EncoderFeature::lossless) &&
            encoder->lossless_effort.maximum > encoder->lossless_effort.minimum);
}

QString ScreenshotImageFileService::compressionLevelKey(ScreenshotCompressionLevel level) {
    switch (level) {
    case ScreenshotCompressionLevel::Low:
        return QStringLiteral("low");
    case ScreenshotCompressionLevel::Medium:
        return QStringLiteral("medium");
    case ScreenshotCompressionLevel::High:
        return QStringLiteral("high");
    }
    return QStringLiteral("low");
}

ScreenshotCompressionLevel ScreenshotImageFileService::compressionLevelForKey(const QString& key) {
    const QString normalized = key.trimmed().toLower();
    if (normalized == QStringLiteral("medium"))
        return ScreenshotCompressionLevel::Medium;
    if (normalized == QStringLiteral("high"))
        return ScreenshotCompressionLevel::High;
    if (normalized == QStringLiteral("low"))
        return ScreenshotCompressionLevel::Low;
    return ScreenshotCompressionLevel::Medium;
}

clip::image::EncodeOptions
ScreenshotImageFileService::encodeOptions(ScreenshotImageFileFormat format,
                                          ScreenshotImageEncodingOptions encoding) {
    clip::image::EncodeOptions options;
    options.format = clipImageFormat(format);
    // The codec bridge supplies only the canonical sRGB color description, not
    // imported EXIF or other source metadata. Retain it to interpret the pixels.
    options.preserve_metadata = true;
    options.quality = qBound(0, encoding.quality, 100);
    const auto encoder = clip_shot::image_codec::encoderInfo(options.format);
    switch (format) {
    case ScreenshotImageFileFormat::Png:
        if (encoder.has_value())
            options.compression_level =
                compressionValue(encoder->compression_level, encoding.compressionLevel);
        break;
    case ScreenshotImageFileFormat::Pdf:
    case ScreenshotImageFileFormat::Jpeg:
    case ScreenshotImageFileFormat::Bmp:
        break;
    case ScreenshotImageFileFormat::Webp:
        options.lossless = options.quality == 100;
        if (encoder.has_value()) {
            if (options.lossless) {
                options.lossless_effort =
                    compressionValue(encoder->lossless_effort, encoding.compressionLevel);
            } else {
                options.effort = compressionValue(encoder->effort, encoding.compressionLevel);
            }
        }
        break;
    case ScreenshotImageFileFormat::Jxl:
    case ScreenshotImageFileFormat::Avif:
        options.lossless = options.quality == 100;
        if (encoder.has_value())
            options.effort = compressionValue(encoder->effort, encoding.compressionLevel);
        break;
    }
    return options;
}

clip::image::EncodeOptions
ScreenshotImageFileService::encodeOptions(ScreenshotImageFileFormat format, int quality) {
    return encodeOptions(format, ScreenshotImageEncodingOptions{quality});
}
