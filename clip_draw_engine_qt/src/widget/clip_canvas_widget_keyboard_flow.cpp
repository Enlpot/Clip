#include "clip_canvas_widget_keyboard_flow.h"

#include <QKeyEvent>

namespace clip_canvas_widget_keyboard_flow {
namespace {

KeyEffect effectForShortcut(clip_canvas_event_flow::KeyShortcutAction action) {
    switch (action) {
    case clip_canvas_event_flow::KeyShortcutAction::ToggleDirtyRects:
        return KeyEffect::ToggleDirtyRects;
    case clip_canvas_event_flow::KeyShortcutAction::Swallow:
        return KeyEffect::AcceptOnly;
    case clip_canvas_event_flow::KeyShortcutAction::None:
    default:
        return KeyEffect::DispatchToEngine;
    }
}

} // namespace

KeyPlan planPress(const QKeyEvent* event) {
    KeyPlan plan;
    if (event == nullptr) {
        return plan;
    }
    plan.hasEvent = true;
    plan.effect = effectForShortcut(clip_canvas_event_flow::keyPressShortcutAction(*event));
    return plan;
}

KeyPlan planRelease(const QKeyEvent* event) {
    KeyPlan plan;
    if (event == nullptr) {
        return plan;
    }
    plan.hasEvent = true;
    plan.effect = effectForShortcut(clip_canvas_event_flow::keyReleaseShortcutAction(*event));
    return plan;
}

} // namespace clip_canvas_widget_keyboard_flow
