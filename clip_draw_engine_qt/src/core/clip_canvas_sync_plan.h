#pragma once

#include <QRect>
#include <QRegion>

class ClipCanvasDisplayCache;

namespace clip_canvas_sync_plan {

struct DirtyRegions {
    QRegion scene;
    QRegion decoration;
    QRegion overlay;
    QRegion visualization;
};

struct Request {
    DirtyRegions display;
    QRegion previousDirtyVisualization;
    QRegion textEditorRegionBefore;
    QRegion textEditorRegionAfter;
    bool textEditorActive = false;
    bool showDirtyRects = false;
};

struct Plan {
    QRegion updateRegion;
    QRegion dirtyVisualizationRegion;
};

DirtyRegions dirtyRegionsForCache(const ClipCanvasDisplayCache& cache, const QRect& clip);
Plan build(const Request& request);

} // namespace clip_canvas_sync_plan
