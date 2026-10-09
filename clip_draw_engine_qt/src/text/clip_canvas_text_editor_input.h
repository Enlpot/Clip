#pragma once

#include "clip_canvas_text_draft.h"
#include "clip_draw_engine_qt/clip_canvas_types.h"

#include <QTextCursor>
#include <Qt>

#include <functional>

class QKeyEvent;

namespace clip_canvas_text_editor_input {

enum class EventCommand {
    None,
    Commit,
    DeleteElement,
};

struct KeyResult {
    EventCommand command = EventCommand::None;
    bool handled = false;
    bool changed = false;
};

using MoveCursor = std::function<bool(QTextCursor::MoveOperation, QTextCursor::MoveMode)>;

KeyResult handleKeyPress(QKeyEvent* event, ClipCanvasTextDraft& draft,
                         const MoveCursor& moveCursor);

struct FontSizeWheelRequest {
    bool hasEvent = false;
    ClipCanvasTool canvasTool = ClipCanvasTool::Select;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    int pixelDeltaY = 0;
    int angleDeltaY = 0;
};

struct FontSizeWheelPlan {
    bool matchedToolWheel = false;
    bool shouldStepFontSize = false;
    bool increase = false;
};

FontSizeWheelPlan planFontSizeWheel(const FontSizeWheelRequest& request);

} // namespace clip_canvas_text_editor_input
