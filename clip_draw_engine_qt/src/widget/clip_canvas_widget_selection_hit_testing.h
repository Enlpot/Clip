#pragma once

#include <QPointF>

#include <cstdint>

class ClipCanvasDisplayCache;
struct ClipOverlayDisplayItem;

namespace clip_canvas_render_geometry {
struct ViewProjection;
}

namespace clip_canvas_widget_selection_hit_testing {

enum class SelectionInteractionTarget { None, Move, Handle };

SelectionInteractionTarget
selectionInteractionAtItems(const ClipOverlayDisplayItem* items, std::uint32_t itemCount,
                            const clip_canvas_render_geometry::ViewProjection& projection,
                            const QPointF& viewPosition);
SelectionInteractionTarget selectionInteractionAt(const ClipCanvasDisplayCache& displayCache,
                                                  const QPointF& viewPosition);

bool pointerHitsSelectionInteractionItems(
    const ClipOverlayDisplayItem* items, std::uint32_t itemCount,
    const clip_canvas_render_geometry::ViewProjection& projection, const QPointF& viewPosition);
bool pointerHitsSelectionInteraction(const ClipCanvasDisplayCache& displayCache,
                                     const QPointF& viewPosition);

} // namespace clip_canvas_widget_selection_hit_testing
