#pragma once

#include "clip_canvas_compositor.h"
#include "clip_canvas_display_cache.h"

class QWidget;

namespace clip_canvas_widget_paint_frame {

struct Request {
    const ClipCanvasDisplayCache* displayCache = nullptr;
    const QWidget* widget = nullptr;
    bool clearBackgroundEnabled = true;
    bool showDirtyRects = false;
    clip_canvas_filter_render::RenderWorkspace* workspace = nullptr;
};

clip_canvas_compositor::Frame build(const Request& request);

} // namespace clip_canvas_widget_paint_frame
