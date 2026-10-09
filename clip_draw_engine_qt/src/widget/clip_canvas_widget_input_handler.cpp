#include "clip_canvas_widget_input_handler.h"

#include "clip_canvas_event_flow.h"
#include "clip_canvas_interaction.h"
#include "clip_canvas_cursor_controller.h"

#include <QEvent>
#include <QWidget>

#include <utility>

ClipCanvasWidgetInputHandler::ClipCanvasWidgetInputHandler(
    QWidget& widget, ClipCanvasCursorController& cursorController)
    : m_widget(widget), m_cursorController(cursorController) {}

bool ClipCanvasWidgetInputHandler::interactionEnabled() const {
    return m_interaction.isEnabled();
}

void ClipCanvasWidgetInputHandler::setInteractionEnabled(bool enabled) {
    m_interaction.setEnabled(m_widget, m_cursorController, enabled);
}

void ClipCanvasWidgetInputHandler::clearTransientState() {
    m_interaction.clearTransientState(m_widget, m_cursorController);
}

ClipCanvasWidgetInputHandler::ProcessResult
ClipCanvasWidgetInputHandler::process(const Context& context, const ClipInputEvent& event) {
    ProcessResult output;
    if (!m_interaction.isEnabled() || !context.hasViewport || !context.processInput) {
        return output;
    }

    clip_canvas_commands::ProcessInputResult result = context.processInput(event);
    if (!result.success) {
        return output;
    }

    m_interaction.applyOutput(m_widget, m_cursorController, result.output);

    output.success = true;
    output.output = result.output;
    output.changedViewports = std::move(result.changedViewports);
    return output;
}

ClipCanvasWidgetInputHandler::ProcessResult
ClipCanvasWidgetInputHandler::processBatch(const Context& context,
                                           const std::vector<ClipInputEvent>& events) {
    ProcessResult output;
    if (!m_interaction.isEnabled() || !context.hasViewport || !context.processInputBatch ||
        events.empty()) {
        return output;
    }
    clip_canvas_commands::ProcessInputResult result = context.processInputBatch(events);
    if (!result.success) {
        return output;
    }
    m_interaction.applyOutput(m_widget, m_cursorController, result.output);
    output.success = true;
    output.output = result.output;
    output.changedViewports = std::move(result.changedViewports);
    return output;
}

ClipCanvasWidgetInputHandler::DispatchResult
ClipCanvasWidgetInputHandler::dispatch(QEvent* event, const Context& context,
                                       const ClipInputEvent& input) {
    DispatchResult result;
    result.process = process(context, input);
    if (!clip_canvas_event_flow::shouldAcceptInputResult(result.process.success,
                                                         result.process.output)) {
        return result;
    }
    if (event != nullptr) {
        event->accept();
    }
    result.accepted = true;
    return result;
}
