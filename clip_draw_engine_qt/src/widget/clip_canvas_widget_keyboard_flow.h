#pragma once

#include "clip_canvas_event_flow.h"

class QKeyEvent;

namespace clip_canvas_widget_keyboard_flow {

enum class KeyEffect {
    None,
    ToggleDirtyRects,
    AcceptOnly,
    DispatchToEngine,
};

struct KeyPlan {
    bool hasEvent = false;
    KeyEffect effect = KeyEffect::None;
};

KeyPlan planPress(const QKeyEvent* event);
KeyPlan planRelease(const QKeyEvent* event);

} // namespace clip_canvas_widget_keyboard_flow
