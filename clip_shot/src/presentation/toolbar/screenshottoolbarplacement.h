#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLBARPLACEMENT_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLBARPLACEMENT_H

#include "clip_shot/presentation/screenshotgeometry.h"
#include "clip_shot/presentation/screenshottoolbarpresenter.h"

namespace clip_shot::presentation {
[[nodiscard]] inline ScreenshotAnchoredToolbarPlacement screenshotToolbarPlacement(
    const ScreenshotToolbarPresentationState& state, const ScreenshotGeometryMapper& geometry,
    const CapturedDisplayModel* display, const ScreenshotToolbarPlacementSnapshot& toolbarGeometry,
    const QRect& logicalBounds, int gap) {
    const ScreenshotHalfOpenRect selectionRect =
        ScreenshotHalfOpenRect::fromRect(state.selectionPixels);
    const auto logicalPosition = [&](const QPointF& point) {
        return display != nullptr
                   ? geometry.logicalPositionForCanvasPoint(*display, point).toPoint()
                   : point.toPoint();
    };
    return ScreenshotGeometryMapper::anchoredToolbarPlacement(
        logicalPosition(selectionRect.bottomRight()),
        logicalPosition(QPointF(selectionRect.right, selectionRect.top)), toolbarGeometry.bottom,
        toolbarGeometry.top, logicalBounds, gap);
}
} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLBARPLACEMENT_H
