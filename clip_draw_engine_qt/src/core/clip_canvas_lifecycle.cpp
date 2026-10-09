#include "clip_canvas_lifecycle.h"

#include "clip_canvas_display_cache.h"
#include "clip_canvas_state.h"
#include "clip_canvas_viewport.h"

namespace clip_canvas_lifecycle {

bool hasViewport(ClipRuntime runtime, const ClipCanvasViewport& viewport) {
    return runtime != nullptr && viewport.isValid() && viewport.runtime() == runtime;
}

InitializeEngineResult initializeEngine(ClipRuntime runtime, ClipCanvasViewport& viewport,
                                        ClipCanvasDisplayCache& displayCache,
                                        clip_canvas_state::Store& state) {
    InitializeEngineResult result;
    if (runtime == nullptr) {
        viewport.reset();
        return result;
    }

    const ClipEngineConfig config = clip_canvas_viewport::defaultEngineConfig();
    state.applyEngineConfigDefaults(config);
    displayCache.setClearColor(config.clear_color);

    if (!viewport.create(runtime, config)) {
        return result;
    }

    result.hasViewport = true;
    viewport.id(&result.viewportId);
    return result;
}

bool setSurfaceSize(ClipRuntime runtime, const ClipCanvasViewport& viewport, const QSize& size) {
    if (!hasViewport(runtime, viewport)) {
        return false;
    }
    if (size.width() < 0 || size.height() < 0) {
        return false;
    }

    return clip_viewport_set_surface_size(runtime, viewport.get(),
                                          static_cast<std::uint32_t>(size.width()),
                                          static_cast<std::uint32_t>(size.height())) == CLIP_OK;
}

bool setCamera(ClipRuntime runtime, const ClipCanvasViewport& viewport, double centerX,
               double centerY, double zoom) {
    if (!hasViewport(runtime, viewport)) {
        return false;
    }

    return clip_viewport_set_camera(runtime, viewport.get(), centerX, centerY, zoom) == CLIP_OK;
}

} // namespace clip_canvas_lifecycle
