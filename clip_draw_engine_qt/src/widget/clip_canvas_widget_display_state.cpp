#include "clip_canvas_widget_display_state.h"

#include "clip_canvas_lifecycle.h"
#include "clip_canvas_sync_plan.h"
#include "clip_canvas_text_editor_session.h"

ClipCanvasWidgetDisplayState::ClipCanvasWidgetDisplayState() {
    resetRetainedState();
}

ClipCanvasDisplayCache& ClipCanvasWidgetDisplayState::displayCache() {
    return m_displayCache;
}

const ClipCanvasDisplayCache& ClipCanvasWidgetDisplayState::displayCache() const {
    return m_displayCache;
}

const clip_canvas_state::Snapshot& ClipCanvasWidgetDisplayState::snapshot() const {
    return m_engineState.snapshot();
}

void ClipCanvasWidgetDisplayState::resetRetainedState() {
    resetDocumentRetainedState();
    m_engineState.reset();
}

void ClipCanvasWidgetDisplayState::resetDocumentRetainedState() {
    m_displayCache.reset(ClipColorRgba8{255, 255, 255, 255});
    m_dirtyVisualizationRegion = QRegion();
}

std::uint64_t ClipCanvasWidgetDisplayState::initializeEngine(ClipRuntime runtime,
                                                             ClipCanvasViewport& viewport) {
    const clip_canvas_lifecycle::InitializeEngineResult result =
        clip_canvas_lifecycle::initializeEngine(runtime, viewport, m_displayCache, m_engineState);
    return result.viewportId;
}

bool ClipCanvasWidgetDisplayState::refreshState(ClipRuntime runtime, ClipViewport viewport,
                                                clip_canvas_state::Changes* outChanges) {
    return m_engineState.refresh(runtime, viewport, outChanges);
}

QRegion ClipCanvasWidgetDisplayState::syncDisplayCache(
    ClipRuntime runtime, ClipViewport viewport, const QRect& widgetRect, const QFont& font,
    ClipCanvasTextEditorSession& textEditorSession, bool showDirtyRects) {
    const QRegion previousVisualizationRegion = m_dirtyVisualizationRegion;
    const bool textEditorActive = textEditorSession.isActive();
    const QRegion textEditorRegionBeforeSync =
        textEditorSession.editingRegion(font, m_displayCache.sceneInfo());
    if (!m_displayCache.sync(runtime, viewport)) {
        return {};
    }

    QRegion textEditorRegionAfterSync;
    QRegion activeDraftPresentationRegion;
    if (textEditorActive) {
        const bool syncedActiveDraft = textEditorSession.syncActiveDraftPresentation(
            runtime, viewport, font, m_displayCache.sceneInfo(), &activeDraftPresentationRegion);
        if (!syncedActiveDraft) {
            textEditorSession.updateGeometry(font, m_displayCache.sceneInfo());
        }
        textEditorRegionAfterSync =
            textEditorSession.editingRegion(font, m_displayCache.sceneInfo());
    }

    const clip_canvas_sync_plan::Plan plan =
        clip_canvas_sync_plan::build(clip_canvas_sync_plan::Request{
            clip_canvas_sync_plan::dirtyRegionsForCache(m_displayCache, widgetRect),
            previousVisualizationRegion,
            textEditorRegionBeforeSync,
            textEditorRegionAfterSync + activeDraftPresentationRegion,
            textEditorActive,
            showDirtyRects,
        });
    m_dirtyVisualizationRegion = plan.dirtyVisualizationRegion;
    return plan.updateRegion;
}

const QRegion& ClipCanvasWidgetDisplayState::dirtyVisualizationRegion() const {
    return m_dirtyVisualizationRegion;
}
