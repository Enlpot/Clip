#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTCLIPBOARDCONTENTSNAPSHOT_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTCLIPBOARDCONTENTSNAPSHOT_H

#include "clip_shot/presentation/screenshotclipboardcontent.h"

namespace clip_shot::presentation::detail {
// Merge MIME content with formats already copied by the native clipboard reader.
[[nodiscard]] std::optional<ScreenshotClipboardContentSnapshot>
snapshotClipboardMimeData(const QMimeData* mimeData, qreal devicePixelRatio,
                          const QColor& baseColor, bool includeDetachedImage,
                          const ScreenshotClipboardContentReader::AllocationCheck& allocate = {},
                          std::optional<ScreenshotClipboardContentSnapshot> nativeSnapshot = {});
} // namespace clip_shot::presentation::detail

#endif
