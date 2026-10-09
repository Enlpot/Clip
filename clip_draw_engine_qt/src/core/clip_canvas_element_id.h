#pragma once

#include "clip_draw_engine.h"

namespace clip_canvas_element_id {

inline bool hasElementId(const ClipElementId& id) {
    return id.generation != 0;
}

inline bool sameElementId(const ClipElementId& lhs, const ClipElementId& rhs) {
    return lhs.index == rhs.index && lhs.generation == rhs.generation;
}

} // namespace clip_canvas_element_id
