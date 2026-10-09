#pragma once

#include "clip_canvas_ffi_handles.h"
#include "clip_canvas_state.h"

#include <QFont>
#include <QRect>
#include <QRegion>

class ClipCanvasTextEditorSession;
class ClipCanvasWidgetDisplayState;

namespace clip_canvas_widget_sync {

struct Request {
    ClipCanvasWidgetDisplayState* displayState = nullptr;
    ClipCanvasTextEditorSession* textEditorSession = nullptr;
    ClipRuntime runtime = nullptr;
    ClipViewport viewport = nullptr;
    bool hasViewport = false;
    QRect widgetRect;
    QFont font;
    bool showDirtyRects = false;
    bool emitStateSignals = true;
    QRegion extraRepaintRegion;
};

struct Result {
    QRegion repaintRegion;
    clip_canvas_state::Changes stateChanges;
    bool displaySyncAttempted = false;
    bool stateRefreshed = false;
    bool shouldEmitStateSignals = false;
};

Result syncAfterEngineMutation(const Request& request);

} // namespace clip_canvas_widget_sync
