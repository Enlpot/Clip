#include "clip_canvas_widget_runtime_binding.h"

#include "clip_canvas_lifecycle.h"
#include "clip_canvas_widget_display_state.h"
#include "clip_draw_engine_qt/clip_canvas_runtime.h"

ClipCanvasWidgetRuntimeBinding::ClipCanvasWidgetRuntimeBinding()
    : m_ownedRuntime(std::make_unique<ClipCanvasRuntime>()), m_runtimeOwner(m_ownedRuntime.get()) {}

ClipCanvasWidgetRuntimeBinding::ClipCanvasWidgetRuntimeBinding(ClipCanvasRuntime& runtime)
    : m_runtimeOwner(&runtime) {}

ClipCanvasWidgetRuntimeBinding::~ClipCanvasWidgetRuntimeBinding() = default;

ClipRuntime ClipCanvasWidgetRuntimeBinding::engine() const {
    return m_engine;
}

ClipViewport ClipCanvasWidgetRuntimeBinding::viewportHandle() const {
    return m_viewport.get();
}

ClipCanvasViewport& ClipCanvasWidgetRuntimeBinding::viewport() {
    return m_viewport;
}

const ClipCanvasViewport& ClipCanvasWidgetRuntimeBinding::viewport() const {
    return m_viewport;
}

std::uint64_t ClipCanvasWidgetRuntimeBinding::viewportId() const {
    return m_currentViewportId;
}

bool ClipCanvasWidgetRuntimeBinding::hasViewport() const {
    return clip_canvas_lifecycle::hasViewport(m_engine, m_viewport);
}

bool ClipCanvasWidgetRuntimeBinding::clearDocumentPreservingViewports() {
    return m_runtimeOwner != nullptr && m_runtimeOwner->clearDocumentPreservingViewports();
}

void ClipCanvasWidgetRuntimeBinding::registerClient(clip_canvas_runtime::Client& client) {
    if (m_runtimeOwner != nullptr) {
        clip_canvas_runtime::Access::registerClient(*m_runtimeOwner, client);
    }
}

void ClipCanvasWidgetRuntimeBinding::unregisterClient(clip_canvas_runtime::Client& client) {
    if (m_runtimeOwner != nullptr) {
        clip_canvas_runtime::Access::unregisterClient(*m_runtimeOwner, client);
    }
}

void ClipCanvasWidgetRuntimeBinding::initializeAttachment(
    ClipCanvasWidgetDisplayState& displayState) {
    attachRuntime(m_runtimeOwner != nullptr ? clip_canvas_runtime::Access::handle(*m_runtimeOwner)
                                            : nullptr,
                  displayState);
}

void ClipCanvasWidgetRuntimeBinding::detachAttachment() {
    m_viewport.reset();
    m_engine = nullptr;
    m_currentViewportId = 0;
}

void ClipCanvasWidgetRuntimeBinding::attachRuntime(ClipRuntime runtime,
                                                   ClipCanvasWidgetDisplayState& displayState) {
    m_engine = runtime;
    m_currentViewportId = displayState.initializeEngine(m_engine, m_viewport);
}

bool ClipCanvasWidgetRuntimeBinding::isRuntimeOwner(ClipCanvasRuntime* owner) const {
    return m_runtimeOwner == owner;
}

bool ClipCanvasWidgetRuntimeBinding::detachRuntimeOwner(ClipCanvasRuntime* owner) {
    if (m_runtimeOwner != owner) {
        return false;
    }

    m_runtimeOwner = nullptr;
    detachAttachment();
    return true;
}

void ClipCanvasWidgetRuntimeBinding::syncChangedViewports(
    ClipChangedViewportList changedViewports) {
    if (m_runtimeOwner != nullptr) {
        clip_canvas_runtime::Access::syncChangedViewports(*m_runtimeOwner, changedViewports);
    }
}

void ClipCanvasWidgetRuntimeBinding::syncChangedViewportIds(
    const std::vector<std::uint64_t>& changedViewportIds) {
    if (m_runtimeOwner != nullptr) {
        clip_canvas_runtime::Access::syncChangedViewportIds(*m_runtimeOwner, changedViewportIds);
    }
}
