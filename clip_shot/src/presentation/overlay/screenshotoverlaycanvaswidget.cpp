#include "clip_shot/shortcuts/shortcutbinding.h"
#include "screenshotoverlaycanvaswidget.h"

#include "../capture/screenshotcaptureperfinstrumentation.h"

#include <QPaintEvent>
#include <QRegion>

namespace {
#if defined(CLIP_SHOT_CAPTURE_PERF_INSTRUMENTATION)
qint64 paintRegionArea(const QRegion& region) {
    qint64 area = 0;
    for (const QRect& rect : region) {
        area += static_cast<qint64>(rect.width()) * static_cast<qint64>(rect.height());
    }
    return area;
}
#endif
} // namespace

ScreenshotOverlayCanvasWidget::ScreenshotOverlayCanvasWidget(ClipCanvasRuntime& runtime,
                                                             QWidget* parent)
    : ClipCanvasWidget(runtime, parent) {
#ifdef Q_OS_MACOS
    setCommandKeyResolver(
        [](const QKeyEvent& event) { return clip_shot::shortcuts::commandKey(event); });
#endif
}

void ScreenshotOverlayCanvasWidget::paintEvent(QPaintEvent* event) {
    CLIP_SHOT_CAPTURE_PERF_SCOPE("presentation.window.canvas.paint_event");
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.canvas.paint_events", 1);
#if defined(CLIP_SHOT_CAPTURE_PERF_INSTRUMENTATION)
    const QRegion region = event != nullptr ? event->region().intersected(rect()) : QRegion(rect());
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.canvas.paint_rects", region.rectCount());
    CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.canvas.paint_logical_pixels",
                                   paintRegionArea(region));
    if (region.contains(rect())) {
        CLIP_SHOT_CAPTURE_PERF_COUNTER("presentation.window.canvas.full_paint_events", 1);
    }
#endif
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.canvas.paint_begin");
    ClipCanvasWidget::paintEvent(event);
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("presentation.window.canvas.paint_end");
}
