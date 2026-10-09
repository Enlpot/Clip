#include "clip_canvas_interaction.h"

#include "clip_canvas_cursor_controller.h"
#include "clip_canvas_input_adapter.h"

#include <QWidget>

namespace clip_canvas_interaction {

bool Controller::isEnabled() const {
    return m_enabled;
}

void Controller::setEnabled(QWidget& widget, ClipCanvasCursorController& cursorController,
                            bool enabled) {
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    if (!m_enabled) {
        clearTransientState(widget, cursorController);
    }
}

void Controller::clearTransientState(QWidget& widget,
                                     ClipCanvasCursorController& cursorController) {
    if (m_capturedPointerId != 0) {
        widget.releaseMouse();
        m_capturedPointerId = 0;
    }
    cursorController.clearCursor(ClipCanvasCursorLayer::CanvasTool);
}

void Controller::applyOutput(QWidget& widget, ClipCanvasCursorController& cursorController,
                             const ClipInteractionOutput& output) {
    if (!m_enabled) {
        return;
    }

    switch (output.capture_kind) {
    case CLIP_POINTER_CAPTURE_CAPTURE:
        if (m_capturedPointerId == 0) {
            widget.grabMouse();
        }
        m_capturedPointerId = output.capture_pointer_id;
        break;
    case CLIP_POINTER_CAPTURE_RELEASE:
        if (m_capturedPointerId != 0) {
            widget.releaseMouse();
        }
        m_capturedPointerId = 0;
        break;
    case CLIP_POINTER_CAPTURE_NO_CHANGE:
    default:
        break;
    }

    if (output.cursor_kind == CLIP_CURSOR_SET) {
        cursorController.setEngineCursor(output.cursor_style);
    }
}

} // namespace clip_canvas_interaction
