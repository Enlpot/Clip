#pragma once

#include "clip_draw_engine_qt/clip_canvas_types.h"
#include "clip_canvas_widget_selection_hit_testing.h"

#include <Qt>

namespace clip_canvas_widget_pointer_flow {

struct PressRequest {
    bool hasEvent = false;
    bool textEditorActive = false;
    bool pointerInsideTextEditor = false;
    ClipCanvasTool canvasTool = ClipCanvasTool::Select;
    Qt::MouseButton button = Qt::NoButton;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    bool pointerOverSelectionInteraction = false;
    bool suppressNextTextToolCreate = false;
    bool restoredSelectionForNextTextToolPress = false;
    bool pointerHitsSelectedText = false;
    clip_canvas_widget_selection_hit_testing::SelectionInteractionTarget selectionTarget =
        clip_canvas_widget_selection_hit_testing::SelectionInteractionTarget::None;
    bool rightQuickSelectionEligible = false;
};

struct PressPlan {
    bool shouldFocusWidget = false;
    bool shouldHandleEditorPress = false;
    bool shouldCommitTextEditor = false;
    bool shouldRestoreSelectionOnCommit = false;
    bool shouldBeginText = false;
    bool shouldBeginSelectedText = false;
    bool allowCreateText = false;
    bool shouldAcceptIfTextBeginFails = false;
    bool suppressedTextCreateForPress = false;
    bool suppressedTextCreateRestoredSelection = false;
    bool shouldAcceptSuppressedTextCreate = false;
    bool dispatchAfterCommitRequiresRestoredSelection = false;
    bool shouldDispatchToEngine = false;
};

bool isSelectedTextCopyGesture(Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
PressPlan planPress(const PressRequest& request);

} // namespace clip_canvas_widget_pointer_flow
