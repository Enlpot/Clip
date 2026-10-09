#pragma once

#include "clip_draw_engine.h"

class QKeyEvent;

namespace clip_canvas_event_flow {

enum class KeyShortcutAction {
    None,
    ToggleDirtyRects,
    Swallow,
};

bool shouldAcceptInputResult(bool processed, const ClipInteractionOutput& output);
ClipInputEvent focusLostInput();
KeyShortcutAction keyPressShortcutAction(const QKeyEvent& event);
KeyShortcutAction keyReleaseShortcutAction(const QKeyEvent& event);

} // namespace clip_canvas_event_flow
