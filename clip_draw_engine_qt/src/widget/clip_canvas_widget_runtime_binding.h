#pragma once

#include "clip_canvas_ffi_handles.h"
#include "clip_canvas_runtime_access.h"
#include "clip_canvas_viewport.h"

#include <cstdint>
#include <memory>
#include <vector>

class ClipCanvasRuntime;
class ClipCanvasWidgetDisplayState;

class ClipCanvasWidgetRuntimeBinding final {
  public:
    ClipCanvasWidgetRuntimeBinding();
    explicit ClipCanvasWidgetRuntimeBinding(ClipCanvasRuntime& runtime);
    ~ClipCanvasWidgetRuntimeBinding();

    ClipCanvasWidgetRuntimeBinding(const ClipCanvasWidgetRuntimeBinding&) = delete;
    ClipCanvasWidgetRuntimeBinding& operator=(const ClipCanvasWidgetRuntimeBinding&) = delete;

    ClipRuntime engine() const;
    ClipCanvasRuntime* runtimeOwner() const {
        return m_runtimeOwner;
    }
    ClipViewport viewportHandle() const;
    ClipCanvasViewport& viewport();
    const ClipCanvasViewport& viewport() const;
    std::uint64_t viewportId() const;
    bool hasViewport() const;
    bool clearDocumentPreservingViewports();

    void registerClient(clip_canvas_runtime::Client& client);
    void unregisterClient(clip_canvas_runtime::Client& client);

    void initializeAttachment(ClipCanvasWidgetDisplayState& displayState);
    void detachAttachment();
    void attachRuntime(ClipRuntime runtime, ClipCanvasWidgetDisplayState& displayState);
    bool isRuntimeOwner(ClipCanvasRuntime* owner) const;
    bool detachRuntimeOwner(ClipCanvasRuntime* owner);
    void syncChangedViewports(ClipChangedViewportList changedViewports);
    void syncChangedViewportIds(const std::vector<std::uint64_t>& changedViewportIds);

  private:
    std::unique_ptr<ClipCanvasRuntime> m_ownedRuntime;
    ClipCanvasRuntime* m_runtimeOwner = nullptr;
    ClipRuntime m_engine = nullptr;
    ClipCanvasViewport m_viewport;
    std::uint64_t m_currentViewportId = 0;
};
