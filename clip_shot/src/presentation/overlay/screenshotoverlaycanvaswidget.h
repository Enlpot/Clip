#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYCANVASWIDGET_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYCANVASWIDGET_H

#include "clip_draw_engine_qt/clip_canvas_widget.h"

class QPaintEvent;
class ClipCanvasRuntime;

class ScreenshotOverlayCanvasWidget final : public ClipCanvasWidget {
  public:
    explicit ScreenshotOverlayCanvasWidget(ClipCanvasRuntime& runtime, QWidget* parent = nullptr);

  protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYCANVASWIDGET_H
