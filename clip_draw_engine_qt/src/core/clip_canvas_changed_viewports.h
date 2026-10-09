#pragma once

#include "clip_draw_engine.h"

#include <cstdint>
#include <vector>

namespace clip_canvas_changed_viewports {

using ViewportIds = std::vector<std::uint64_t>;

ViewportIds deduplicate(const ViewportIds& viewportIds);
ViewportIds idsFromList(ClipChangedViewportList changedViewports);

} // namespace clip_canvas_changed_viewports
