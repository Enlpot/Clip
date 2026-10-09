#include "clip_canvas_sync_plan.h"

#include "clip_canvas_display_cache.h"

namespace clip_canvas_sync_plan {

DirtyRegions dirtyRegionsForCache(const ClipCanvasDisplayCache& cache, const QRect& clip) {
    DirtyRegions regions;
    regions.scene = clip_canvas_display::dirtyRectsToRegion(cache.sceneDirtyRects(),
                                                            cache.sceneDirtyRectCount(), clip);
    regions.decoration = clip_canvas_display::dirtyRectsToRegion(
        cache.decorationDirtyRects(), cache.decorationDirtyRectCount(), clip);
    regions.overlay = clip_canvas_display::dirtyRectsToRegion(cache.overlayDirtyRects(),
                                                              cache.overlayDirtyRectCount(), clip);
    regions.visualization = regions.scene + regions.decoration + regions.overlay;
    return regions;
}

Plan build(const Request& request) {
    Plan plan;
    plan.dirtyVisualizationRegion = request.display.visualization;
    plan.updateRegion += request.display.scene;
    plan.updateRegion += request.display.decoration;
    plan.updateRegion += request.display.overlay;

    if (request.textEditorActive) {
        plan.updateRegion += request.textEditorRegionBefore;
        plan.updateRegion += request.textEditorRegionAfter;
    }

    if (request.showDirtyRects) {
        plan.updateRegion += request.previousDirtyVisualization;
        plan.updateRegion += plan.dirtyVisualizationRegion;
    }

    return plan;
}

} // namespace clip_canvas_sync_plan
