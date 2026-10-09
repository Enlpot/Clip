#include "clip_canvas_text_edit_target.h"

#include "clip_canvas_text.h"

#include <QFont>

#include <cstdint>

namespace clip_canvas_text_edit_target {
namespace {

bool hasViewport(ClipRuntime runtime, ClipViewport viewport) {
    return runtime != nullptr && viewport != nullptr;
}

std::optional<ClipElementId> hitTextElementId(ClipRuntime runtime, ClipViewport viewport,
                                              const QPointF& canvasPoint) {
    ClipElementId hitId{};
    std::uint8_t hit = 0;
    if (clip_viewport_hit_text(runtime, viewport, canvasPoint.x(), canvasPoint.y(), &hitId, &hit) !=
            CLIP_OK ||
        hit == 0) {
        return std::nullopt;
    }
    return hitId;
}

std::optional<ClipTextElementInfo> textElementInfo(ClipRuntime runtime, const ClipElementId& id) {
    ClipTextElementInfo textInfo{};
    if (clip_runtime_get_text_element(runtime, id, &textInfo) != CLIP_OK) {
        return std::nullopt;
    }
    return textInfo;
}

} // namespace

std::optional<ClipTextElementInfo>
resolveTextEditTarget(ClipRuntime runtime, ClipViewport viewport, const QPointF& canvasPoint,
                      const QFont& baseFont, const ClipTextStyle& newTextStyle, bool allowCreate) {
    if (!hasViewport(runtime, viewport)) {
        return std::nullopt;
    }

    if (const std::optional<ClipElementId> hitId =
            hitTextElementId(runtime, viewport, canvasPoint)) {
        return textElementInfo(runtime, *hitId);
    }

    if (!allowCreate) {
        return std::nullopt;
    }

    return clip_canvas_text::newTextInfoAt(canvasPoint, baseFont, newTextStyle);
}

std::optional<ClipTextElementInfo> resolveSelectedTextEditTarget(ClipRuntime runtime,
                                                                 ClipViewport viewport,
                                                                 const QPointF& canvasPoint,
                                                                 bool requireSerialBoundText) {
    if (!hasViewport(runtime, viewport)) {
        return std::nullopt;
    }

    const std::optional<ClipElementId> hitId = hitTextElementId(runtime, viewport, canvasPoint);
    if (!hitId.has_value()) {
        return std::nullopt;
    }

    std::uint8_t selected = 0;
    if (clip_viewport_is_element_selected(runtime, viewport, *hitId, &selected) != CLIP_OK ||
        selected == 0) {
        return std::nullopt;
    }

    std::uint32_t selectedTextCount = 0;
    if (clip_viewport_selected_text_count(runtime, viewport, &selectedTextCount) != CLIP_OK ||
        selectedTextCount != 1) {
        return std::nullopt;
    }

    if (requireSerialBoundText) {
        std::uint8_t bound = 0;
        if (clip_viewport_is_text_bound_to_serial_number(runtime, viewport, *hitId, &bound) !=
                CLIP_OK ||
            bound == 0) {
            return std::nullopt;
        }
    }

    return textElementInfo(runtime, *hitId);
}

} // namespace clip_canvas_text_edit_target
