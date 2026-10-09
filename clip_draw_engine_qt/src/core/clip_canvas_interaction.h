#pragma once

#include "clip_draw_engine.h"

#include <cstdint>

class QWidget;
class ClipCanvasCursorController;

namespace clip_canvas_interaction {

class Controller final {
  public:
    bool isEnabled() const;

    void setEnabled(QWidget& widget, ClipCanvasCursorController& cursorController, bool enabled);
    void clearTransientState(QWidget& widget, ClipCanvasCursorController& cursorController);
    void applyOutput(QWidget& widget, ClipCanvasCursorController& cursorController,
                     const ClipInteractionOutput& output);

  private:
    bool m_enabled = true;
    std::uint32_t m_capturedPointerId = 0;
};

} // namespace clip_canvas_interaction
