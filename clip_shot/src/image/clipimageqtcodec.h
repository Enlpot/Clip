#pragma once

#include "clip_shot/presentation/screenshotimagerowsource.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>
#include <QtGlobal>

#include <clip/image/codec.h>
#include <clip/image/format.h>

#include <optional>

class QIODevice;
class QImageReader;

namespace clip_shot::image_codec {

enum class SkinDecodeError {
    None,
    UnsupportedFormat,
    UnreadableFile,
    InputTooLarge,
    InvalidImage,
    ResourceLimit,
};

struct SkinDecodeResult final {
    QImage image;
    SkinDecodeError error = SkinDecodeError::None;
};

// Decodes one static preview, including the first composited animation frame.
// Skin decoding has independent resource limits and never uses Qt image plugins.
[[nodiscard]] SkinDecodeResult decodeSkinFile(const QString& path);

struct EncodeResult final {
    quint64 bytesWritten = 0;
    clip::image::Format format = clip::image::Format::unknown;
    QSize size;
    quint32 emittedFrameCount = 0;
    clip::image::PixelRoundTrip roundTrip = clip::image::PixelRoundTrip::codec_artifact;
    bool finalizedAndFlushed = false;
};

[[nodiscard]] std::optional<clip::image::EncoderInfo> encoderInfo(clip::image::Format format);

[[nodiscard]] bool encodeToDevice(const ScreenshotImageRowSource& source, QIODevice* device,
                                  clip::image::Format format,
                                  const clip::image::EncodeOptions& options,
                                  QString* error = nullptr, EncodeResult* result = nullptr);
[[nodiscard]] bool encodeToDevice(const QImage& image, QIODevice* device,
                                  clip::image::Format format,
                                  const clip::image::EncodeOptions& options,
                                  QString* error = nullptr, EncodeResult* result = nullptr);

[[nodiscard]] bool resizeToRgba8(const ScreenshotImageRowSource& source, const QSize& outputSize,
                                 uchar* destination, qsizetype destinationStride,
                                 qsizetype destinationSize, QString* error = nullptr);

[[nodiscard]] QByteArray encodePng(const QImage& image, int compressionLevel = 0);
[[nodiscard]] QByteArray encodePng(const ScreenshotImageRowSource& source,
                                   int compressionLevel = 0);
[[nodiscard]] ScreenshotImageRowSource srgbRowSource(const QImage& image);
[[nodiscard]] QByteArray encodeWebp(const QImage& image, int quality = 75);
// Decoded RGB images retain their source color space. Convert to sRGB at the
// rendering/encoding boundary; merely assigning an sRGB tag changes their meaning.
[[nodiscard]] QImage decode(const QByteArray& encoded, clip::image::Format expectedFormat,
                            const char* nameHint);
// Retains Qt's decoding, metadata and transformation behavior while putting
// the returned raster in managed storage. Compatible readers decode in place.
[[nodiscard]] QImage readManagedImage(QImageReader& reader);
[[nodiscard]] QImage decodeIconFile(const QString& path, uint32_t preferredExtent);
[[nodiscard]] QSize inspectSize(const QByteArray& encoded, clip::image::Format expectedFormat);
[[nodiscard]] QImage decodeFile(const QString& path, clip::image::Format expectedFormat);
[[nodiscard]] QImage decodeFileBgra(const QString& path, clip::image::Format expectedFormat);
[[nodiscard]] bool inspectFile(const QString& path, clip::image::Format expectedFormat,
                               const QSize& expectedSize);

} // namespace clip_shot::image_codec
