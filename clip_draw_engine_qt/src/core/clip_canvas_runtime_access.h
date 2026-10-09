#pragma once

#include "clip_draw_engine.h"

#include <cstdint>
#include <vector>

class ClipCanvasRuntime;
namespace clip_canvas_smart_erase {
class Coordinator;
}

namespace clip_canvas_runtime {

class Client {
  public:
    virtual ~Client() = default;

    virtual std::uint64_t runtimeViewportId() const = 0;
    virtual void detachRuntimeForReplacement() = 0;
    virtual void attachRuntime(ClipRuntime runtime) = 0;
    virtual void detachRuntimeOwner(ClipCanvasRuntime* runtimeOwner) = 0;
    virtual void clearRenderState() = 0;
    // Called after the old document is discarded, before syncing the empty viewports.
    virtual void resetDocumentRetainedState() = 0;
    virtual void smartEraseChanged() {
        clearRenderState();
    }
    virtual bool resetEditingState(bool restoreSelectTool) = 0;
    virtual void syncAfterEngineMutation() = 0;
    virtual void refreshStateFromEngine(bool emitSignals) = 0;
};

struct Access {
    static clip_canvas_smart_erase::Coordinator& smartErase(ClipCanvasRuntime& runtime);
    static ClipRuntime handle(const ClipCanvasRuntime& runtime);
    static void registerClient(ClipCanvasRuntime& runtime, Client& client);
    static void unregisterClient(ClipCanvasRuntime& runtime, Client& client);
    static void syncChangedViewports(ClipCanvasRuntime& runtime,
                                     ClipChangedViewportList changedViewports);
    static void syncChangedViewportIds(ClipCanvasRuntime& runtime,
                                       const std::vector<std::uint64_t>& changedViewportIds);
};

} // namespace clip_canvas_runtime
