#pragma once

#include "clip_canvas_display_cache.h"
#include "clip_canvas_renderer.h"
#include "clip_canvas_pen_mask_atlas.h"
#include "clip_canvas_viewport.h"
#include "clip_draw_engine_qt/clip_canvas_custom_renderer.h"

#include <QFont>
#include <QPainter>

// Widget-owned, single-resolution surface. The fixed viewport preserves the full
// source scene and its explicit filter passes even when the live viewport crops it.
class ClipCanvasReferenceScene final {
  public:
    ~ClipCanvasReferenceScene();
    bool render(ClipRuntime runtime, const ClipCanvasFilterRenderReference& reference,
                const clip_canvas_renderer::SceneRenderRequest& request,
                const QTransform& canvasToView);
    void clearRenderState();
    void reset();

  private:
    ClipCanvasViewport m_viewport;
    ClipCanvasDisplayCache m_displayCache;
    clip_canvas_pen_mask::PenMaskAtlas m_penMasks;
    ClipCanvasFilterRenderReference m_reference;
    QImage m_image;
    std::uint64_t m_sceneRevision = 0;
    std::uint64_t m_backgroundRevision = 0;
    ClipCanvasCustomRenderer* m_backgroundRenderer = nullptr;
    QFont m_font;
    QPainter::RenderHints m_renderHints;
    bool m_clearBackgroundEnabled = true;
};
