#include "clip_canvas_event_flow.h"

#include <QKeyEvent>

namespace clip_canvas_event_flow {

bool shouldAcceptInputResult(bool processed, const ClipInteractionOutput& output) {
    return processed && output.consumed != 0;
}

ClipInputEvent focusLostInput() {
    ClipInputEvent input{};
    input.kind = CLIP_INPUT_EVENT_FOCUS_LOST;
    return input;
}

KeyShortcutAction keyPressShortcutAction(const QKeyEvent& event) {
    if (event.isAutoRepeat()) {
        return KeyShortcutAction::None;
    }
    if (event.key() == Qt::Key_F4) {
        return KeyShortcutAction::ToggleDirtyRects;
    }
    return KeyShortcutAction::None;
}

KeyShortcutAction keyReleaseShortcutAction(const QKeyEvent& event) {
    if (event.isAutoRepeat()) {
        return KeyShortcutAction::None;
    }
    if (event.key() == Qt::Key_F4) {
        return KeyShortcutAction::Swallow;
    }
    return KeyShortcutAction::None;
}

} // namespace clip_canvas_event_flow
