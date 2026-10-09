#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTOCRLAYOUT_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTOCRLAYOUT_H

#include "clip_shot/presentation/screenshotocrpresentation.h"

namespace clip_shot::presentation {
// STranslate Smart layout adapted to image-coordinate OCR lines. The input is never modified.
[[nodiscard]] QVector<ScreenshotOcrLine> mergeOcrLayout(const QVector<ScreenshotOcrLine>& lines,
                                                        const QPointF& imageOrigin = {});
} // namespace clip_shot::presentation

#endif
