#pragma once

#include "clip_canvas_commands.h"
#include "clip_canvas_ffi_handles.h"
#include "clip_canvas_interaction.h"
#include "clip_draw_engine.h"

#include <functional>

class QEvent;
class QWidget;
class ClipCanvasCursorController;

class ClipCanvasWidgetInputHandler final {
  public:
    using ProcessInputCommand =
        std::function<clip_canvas_commands::ProcessInputResult(const ClipInputEvent&)>;
    using ProcessInputBatchCommand =
        std::function<clip_canvas_commands::ProcessInputResult(const std::vector<ClipInputEvent>&)>;

    struct Context {
        bool hasViewport = false;
        ProcessInputCommand processInput;
        ProcessInputBatchCommand processInputBatch;
    };

    struct ProcessResult {
        bool success = false;
        ClipInteractionOutput output{};
        ScopedChangedViewportList changedViewports;
    };

    struct DispatchResult {
        bool accepted = false;
        ProcessResult process;
    };

    ClipCanvasWidgetInputHandler(QWidget& widget, ClipCanvasCursorController& cursorController);

    bool interactionEnabled() const;
    void setInteractionEnabled(bool enabled);
    void clearTransientState();

    ProcessResult process(const Context& context, const ClipInputEvent& event);
    ProcessResult processBatch(const Context& context, const std::vector<ClipInputEvent>& events);
    DispatchResult dispatch(QEvent* event, const Context& context, const ClipInputEvent& input);

  private:
    QWidget& m_widget;
    ClipCanvasCursorController& m_cursorController;
    clip_canvas_interaction::Controller m_interaction;
};
