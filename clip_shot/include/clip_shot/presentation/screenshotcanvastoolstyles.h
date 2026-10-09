#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTCANVASTOOLSTYLES_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTCANVASTOOLSTYLES_H

#include "clip_draw_engine_qt/clip_canvas_style_edit.h"

class ClipCanvasWidget;

namespace clip_shot::presentation {

[[nodiscard]] ClipCanvasStyleDefaults screenshotCanvasToolStyleDefaults();
[[nodiscard]] bool persistScreenshotCanvasToolStyles(const ClipCanvasStyleDefaults& defaults);
[[nodiscard]] bool persistScreenshotCanvasStyleEdit(const ClipCanvasStyleEdit& edit);
void applyScreenshotCanvasToolStyles(ClipCanvasWidget& canvas,
                                     const ClipCanvasStyleDefaults& defaults);

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTCANVASTOOLSTYLES_H
