#pragma once

#include <QSize>

#include <cstdint>

#include "clip_draw_engine.h"

class ClipCanvasDisplayCache;
class ClipCanvasViewport;

namespace clip_canvas_state {
class Store;
}

namespace clip_canvas_lifecycle {

struct InitializeEngineResult {
    bool hasViewport = false;
    std::uint64_t viewportId = 0;
};

bool hasViewport(ClipRuntime runtime, const ClipCanvasViewport& viewport);
InitializeEngineResult initializeEngine(ClipRuntime runtime, ClipCanvasViewport& viewport,
                                        ClipCanvasDisplayCache& displayCache,
                                        clip_canvas_state::Store& state);
bool setSurfaceSize(ClipRuntime runtime, const ClipCanvasViewport& viewport, const QSize& size);
bool setCamera(ClipRuntime runtime, const ClipCanvasViewport& viewport, double centerX,
               double centerY, double zoom);

} // namespace clip_canvas_lifecycle
