#pragma once

#include "clip_canvas_display_cache.h"
#include "clip_canvas_renderer.h"

#include <QPointF>
#include <QImage>
#include <QRect>
#include <QRectF>
#include <QRegion>

class QPainter;
class QWidget;
class ClipCanvasCustomRenderer;
struct ClipCanvasRenderContext;

namespace clip_canvas_compositor {

struct Frame {
    const ClipCanvasDisplayCache* displayCache = nullptr;
    const SceneDisplayInfo* sceneInfo = nullptr;
    const WatermarkDisplayInfo* watermarkInfo = nullptr;
    const SpotlightDisplayInfo* spotlightInfo = nullptr;
    const ClipSpotlightCutout* spotlightCutouts = nullptr;
    std::uint32_t spotlightCutoutCount = 0;
    const OverlayDisplayInfo* overlayInfo = nullptr;
    const ClipCanvasSceneItem* sceneItems = nullptr;
    std::uint32_t sceneItemCount = 0;
    const ClipCanvasOverlayItem* overlayItems = nullptr;
    std::uint32_t overlayItemCount = 0;
    const ClipDirtyRect* sceneDirtyRects = nullptr;
    std::uint32_t sceneDirtyRectCount = 0;
    const ClipDirtyRect* decorationDirtyRects = nullptr;
    std::uint32_t decorationDirtyRectCount = 0;
    const ClipDirtyRect* overlayDirtyRects = nullptr;
    std::uint32_t overlayDirtyRectCount = 0;
    const QImage* backgroundImage = nullptr;
    ClipCanvasCustomRenderer* backgroundRenderer = nullptr;
    const ClipCanvasRenderContext* backgroundContext = nullptr;
    clip_canvas_filter_render::RenderWorkspace* workspace = nullptr;
    clip_canvas_pen_mask::PenMaskAtlas* penMaskAtlas = nullptr;
    const QWidget* widget = nullptr;
    QRectF watermarkRenderArea;
    bool hasWatermarkRenderArea = false;
    QRectF spotlightRenderArea;
    bool hasSpotlightRenderArea = false;
    bool clearBackgroundEnabled = true;
    bool showDirtyRects = false;
};

void clearSurface(QPainter& painter, const Frame& frame);
void renderDocumentDecorations(QPainter& painter, const Frame& frame,
                               clip_canvas_renderer::WatermarkRenderPurpose purpose =
                                   clip_canvas_renderer::WatermarkRenderPurpose::Widget);
void renderEditorOverlays(QPainter& painter, const Frame& frame);

} // namespace clip_canvas_compositor
