#pragma once

#include "clip_draw_engine_qt/clip_canvas_style_edit.h"
#include <QObject>
#include <functional>
#include <optional>

class ClipCanvasWidget;
class ScreenshotToolPalette;

namespace clip_shot::presentation {

// One binding per editor; only successful user commits are remembered and saved.
class ScreenshotStyleBinding final : public QObject {
  public:
    using Save = std::function<bool(const ClipCanvasStyleEdit&)>;
    using Replicate = std::function<void(const ClipCanvasStyleEdit&)>;
    ScreenshotStyleBinding(ScreenshotToolPalette& palette, ClipCanvasWidget& canvas,
                           QObject* parent, Replicate replicate = {}, Save save = {});
    [[nodiscard]] std::optional<bool> lastSaveSucceeded() const {
        return m_lastSaveSucceeded;
    }

  private:
    std::optional<bool> m_lastSaveSucceeded;
};

void replicateScreenshotStyleEdit(ClipCanvasWidget& peer, const ClipCanvasStyleEdit& edit);

// Returns false for font input, which belongs to the canvas text/draft handler.
[[nodiscard]] bool stepScreenshotStyle(ScreenshotToolPalette& palette, ClipCanvasWidget& canvas,
                                       int direction);
} // namespace clip_shot::presentation
