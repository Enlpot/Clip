#include "clip_shot/presentation/screenshotstylebinding.h"

#include "clip_draw_engine_qt/clip_canvas_widget.h"
#include "clip_shot/presentation/screenshottoolpalette.h"
#include "clip_shot/presentation/screenshotcanvastoolstyles.h"
#include <QDebug>
#include <QPointer>

namespace clip_shot::presentation {
ScreenshotStyleBinding::ScreenshotStyleBinding(ScreenshotToolPalette& palette,
                                               ClipCanvasWidget& canvas, QObject* parent,
                                               Replicate replicate, Save save)
    : QObject(parent) {
    palette.setStyleEditHandler(
        [guard = QPointer<QObject>(this), target = QPointer<ClipCanvasWidget>(&canvas),
         source = QPointer<ScreenshotToolPalette>(&palette)](const ClipCanvasStyleEdit& edit) {
            if (guard == nullptr || target == nullptr)
                return false;
            if (target->commitStyleEdit(edit))
                return true;
            if (source != nullptr) {
                source->setStyleToolbarState(target->canvasStyleToolbarState());
                source->setWatermarkConfig(target->canvasWatermarkConfig());
                source->setSpotlightConfig(target->canvasSpotlightConfig());
            }
            qWarning() << "Unable to apply canvas style edit";
            return false;
        });
    connect(&canvas, &ClipCanvasWidget::styleEditCommitted, this,
            [this, source = QPointer<ScreenshotToolPalette>(&palette), replicate,
             save](const ClipCanvasStyleEdit& edit) {
                if (source == nullptr)
                    return;
                source->rememberStyleEdit(edit);
                if (replicate)
                    replicate(edit);
                m_lastSaveSucceeded = save ? save(edit) : persistScreenshotCanvasStyleEdit(edit);
                if (!*m_lastSaveSucceeded) {
                    qWarning() << "Unable to persist canvas style edit";
                }
            });
}

void replicateScreenshotStyleEdit(ClipCanvasWidget& peer, const ClipCanvasStyleEdit& edit) {
    // Document and creation styles are already shared by the screenshot runtime. Applying
    // a text patch again through an idle viewport would bypass an active draft's ownership.
    if (std::holds_alternative<ClipCanvasWatermarkEdit>(edit) ||
        std::holds_alternative<ClipCanvasSpotlightEdit>(edit)) {
        static_cast<void>(peer.applyStyleEdit(edit));
    }
}

bool stepScreenshotStyle(ScreenshotToolPalette& palette, ClipCanvasWidget& canvas, int direction) {
    if (direction == 0 || !canvas.interactionEnabled() || canvas.hasActiveTextEditing())
        return false;
    switch (canvas.canvasTool()) {
    case ClipCanvasTool::Shape:
    case ClipCanvasTool::Distance:
    case ClipCanvasTool::Arrow:
    case ClipCanvasTool::Line:
    case ClipCanvasTool::FreeDraw:
    case ClipCanvasTool::RectangleHighlight:
    case ClipCanvasTool::PenHighlight:
        return palette.stepStrokeWidth(direction);
    case ClipCanvasTool::Select:
        return palette.stepSelectionOpacity(direction);
    case ClipCanvasTool::Spotlight:
        return palette.stepSpotlightOpacity(direction);
    case ClipCanvasTool::RectangleFilter:
    case ClipCanvasTool::AutoFilter:
        return palette.stepFilterIntensity(direction);
    case ClipCanvasTool::PenFilter:
        return palette.stepPenFilterStrokeWidth(direction);
    case ClipCanvasTool::BrushEraser:
        return palette.stepBrushEraserStrokeWidth(direction);
    case ClipCanvasTool::Watermark:
        return palette.stepWatermarkFontSize(direction);
    default:
        return false;
    }
}
} // namespace clip_shot::presentation
