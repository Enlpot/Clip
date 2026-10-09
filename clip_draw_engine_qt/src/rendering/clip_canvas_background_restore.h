#pragma once

#include "clip_canvas_filter_tile_cache.h"

#include <QImage>
#include <QRect>
#include <memory>

class ClipCanvasSceneItem;
struct SceneDisplayInfo;
namespace clip_canvas_pen_mask {
class PenMaskAtlas;
}
namespace clip_canvas_filter_render {
class RenderWorkspace;
struct ExecutionOptions;
} // namespace clip_canvas_filter_render
namespace clip_canvas_renderer {
struct SceneRenderRequest;
struct FilterFrameInfo;
struct FilterRenderDiagnostics;
} // namespace clip_canvas_renderer

namespace clip_canvas_background_restore {

bool hasCoverage(const ClipCanvasSceneItem& item,
                 const clip_canvas_renderer::FilterFrameInfo& geometry,
                 const SceneDisplayInfo& info, const QRect& physicalBounds, qreal dpr,
                 clip_canvas_pen_mask::PenMaskAtlas& atlas,
                 const clip_canvas_filter_render::ExecutionOptions& execution);

const QImage*
originalBackground(const clip_canvas_renderer::SceneRenderRequest& request,
                   const QRect& physicalBounds, qreal dpr,
                   clip_canvas_filter_render::RenderWorkspace& workspace,
                   std::shared_ptr<const clip_canvas_filter_tile_cache::Entry>& retained,
                   clip_canvas_renderer::FilterRenderDiagnostics& diagnostics);

bool apply(const QImage& original, QImage& destination, const ClipCanvasSceneItem& item,
           const clip_canvas_renderer::FilterFrameInfo& geometry, const SceneDisplayInfo& info,
           const QRect& physicalBounds, qreal dpr, clip_canvas_pen_mask::PenMaskAtlas& atlas,
           clip_canvas_filter_render::RenderWorkspace& workspace,
           const clip_canvas_filter_render::ExecutionOptions& execution,
           clip_canvas_renderer::FilterRenderDiagnostics& diagnostics);

} // namespace clip_canvas_background_restore
