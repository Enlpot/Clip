#include "clip_canvas_widget_sync.h"

#include "clip_canvas_widget_display_state.h"
#include "clip_canvas_text_editor_session.h"

namespace clip_canvas_widget_sync {

Result syncAfterEngineMutation(const Request& request) {
    Result result;
    result.repaintRegion = request.extraRepaintRegion;

    if (!request.hasViewport || request.displayState == nullptr ||
        request.textEditorSession == nullptr) {
        return result;
    }

    const bool hadActiveTextEditor = request.textEditorSession->isActive();
    const ClipTextStyle previousTextStyle =
        hadActiveTextEditor ? request.textEditorSession->currentTextStyle() : ClipTextStyle{};
    result.displaySyncAttempted = true;
    result.repaintRegion += request.displayState->syncDisplayCache(
        request.runtime, request.viewport, request.widgetRect, request.font,
        *request.textEditorSession, request.showDirtyRects);

    result.stateRefreshed =
        request.displayState->refreshState(request.runtime, request.viewport, &result.stateChanges);
    if (result.stateRefreshed && hadActiveTextEditor && request.textEditorSession->isActive() &&
        !clip_canvas_state::textStylesEqual(previousTextStyle,
                                            request.textEditorSession->currentTextStyle())) {
        result.stateChanges.styleToolbarChanged = true;
    }
    result.shouldEmitStateSignals = request.emitStateSignals && result.stateRefreshed;
    return result;
}

} // namespace clip_canvas_widget_sync
