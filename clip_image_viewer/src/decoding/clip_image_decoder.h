#pragma once

#include "decoding/image_decoder.h"

#include <clip/image/document.h>

namespace clip::image_viewer {

DecodeResult prepareClipDocument(const QString& filePath, clip::image::Document document,
                                 const QString& decoderName,
                                 const DecodeCancellation& cancellation);
DecodeResult prepareMappedClipDocument(const QString& filePath,
                                       const clip::image::Document& document,
                                       const QString& decoderName,
                                       const DecodeCancellation& cancellation);

class ClipImageDecoder final : public ImageDecoder {
  public:
    bool canDecode(const QString& filePath, const QByteArray& header) const override;
    DecodeResult decode(const QString& filePath,
                        const DecodeCancellation& cancellation) const override;
};

} // namespace clip::image_viewer
