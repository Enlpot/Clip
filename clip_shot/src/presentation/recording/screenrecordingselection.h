#ifndef CLIP_SHOT_PRESENTATION_RECORDING_SCREENRECORDINGSELECTION_H
#define CLIP_SHOT_PRESENTATION_RECORDING_SCREENRECORDINGSELECTION_H

#include "clip_shot/presentation/screenrecordingareawindow.h"
#include "clip_shot/presentation/screenshottoolpalette.h"
#include "clip_draw_engine_qt/clip_canvas_widget.h"

namespace clip_shot::presentation::recording {

inline void connectScreenRecordingSelection(ScreenshotToolPalette& palette,
                                            ScreenRecordingAreaWindow& area, QObject& context) {
    auto* canvas = area.canvas();
    QObject::connect(
        &palette, &ScreenshotToolPalette::selectRequested, &context, [&palette, &area, canvas]() {
            if (palette.activeTool() == ScreenshotToolPalette::Tool::Select) {
                canvas->setCanvasTool(ClipCanvasTool::Select);
                area.setInputMode(ScreenRecordingAreaWindow::InputMode::Drawing);
            } else {
                area.setInputMode(palette.recordingExportSettingsVisible()
                                      ? ScreenRecordingAreaWindow::InputMode::RegionEditing
                                      : ScreenRecordingAreaWindow::InputMode::PassThrough);
            }
        });
    QObject::connect(&palette, &ScreenshotToolPalette::recordingExportSettingsVisibleChanged,
                     &context, [&area](bool visible) {
                         area.setInputMode(visible
                                               ? ScreenRecordingAreaWindow::InputMode::RegionEditing
                                               : ScreenRecordingAreaWindow::InputMode::PassThrough);
                     });
    QObject::connect(
        canvas, &ClipCanvasWidget::historyStateChanged, &context,
        [&palette, canvas]() { palette.setHistoryState(canvas->canvasHistoryState()); });
    QObject::connect(
        canvas, &ClipCanvasWidget::styleToolbarStateChanged, &context,
        [&palette, canvas]() { palette.setStyleToolbarState(canvas->canvasStyleToolbarState()); });
    QObject::connect(&palette, &ScreenshotToolPalette::sendSelectionToBackRequested, &context,
                     [canvas]() { canvas->reorderSelected(ClipCanvasSelectionOrder::SendToBack); });
    QObject::connect(
        &palette, &ScreenshotToolPalette::sendSelectionBackwardRequested, &context,
        [canvas]() { canvas->reorderSelected(ClipCanvasSelectionOrder::SendBackward); });
    QObject::connect(
        &palette, &ScreenshotToolPalette::bringSelectionForwardRequested, &context,
        [canvas]() { canvas->reorderSelected(ClipCanvasSelectionOrder::BringForward); });
    QObject::connect(
        &palette, &ScreenshotToolPalette::bringSelectionToFrontRequested, &context,
        [canvas]() { canvas->reorderSelected(ClipCanvasSelectionOrder::BringToFront); });
    QObject::connect(
        &palette, &ScreenshotToolPalette::alignSelectionLeftRequested, &context,
        [canvas]() { canvas->alignSelected(ClipCanvasSelectionAlignment::AlignLeft); });
    QObject::connect(&palette, &ScreenshotToolPalette::alignSelectionCenterHorizontallyRequested,
                     &context, [canvas]() {
                         canvas->alignSelected(
                             ClipCanvasSelectionAlignment::AlignCenterHorizontally);
                     });
    QObject::connect(
        &palette, &ScreenshotToolPalette::alignSelectionRightRequested, &context,
        [canvas]() { canvas->alignSelected(ClipCanvasSelectionAlignment::AlignRight); });
    QObject::connect(&palette, &ScreenshotToolPalette::alignSelectionTopRequested, &context,
                     [canvas]() { canvas->alignSelected(ClipCanvasSelectionAlignment::AlignTop); });
    QObject::connect(
        &palette, &ScreenshotToolPalette::alignSelectionCenterVerticallyRequested, &context,
        [canvas]() { canvas->alignSelected(ClipCanvasSelectionAlignment::AlignCenterVertically); });
    QObject::connect(
        &palette, &ScreenshotToolPalette::alignSelectionBottomRequested, &context,
        [canvas]() { canvas->alignSelected(ClipCanvasSelectionAlignment::AlignBottom); });
    QObject::connect(&palette, &ScreenshotToolPalette::distributeSelectionHorizontallyRequested,
                     &context, [canvas]() {
                         canvas->alignSelected(
                             ClipCanvasSelectionAlignment::DistributeHorizontally);
                     });
    QObject::connect(
        &palette, &ScreenshotToolPalette::distributeSelectionVerticallyRequested, &context,
        [canvas]() { canvas->alignSelected(ClipCanvasSelectionAlignment::DistributeVertically); });
    QObject::connect(&palette, &ScreenshotToolPalette::selectionOpacityChanged, &context,
                     [canvas](qreal opacity) { canvas->setSelectedOpacity(opacity); });
    QObject::connect(&palette, &ScreenshotToolPalette::duplicateSelectionRequested, &context,
                     [canvas]() { canvas->duplicateSelected(); });
    QObject::connect(&palette, &ScreenshotToolPalette::deleteSelectionRequested, &context,
                     [canvas]() { canvas->deleteSelected(); });
    QObject::connect(&palette, &ScreenshotToolPalette::resetCanvasRequested, &context,
                     [canvas]() { canvas->deleteAllElements(); });
}

} // namespace clip_shot::presentation::recording

#endif // CLIP_SHOT_PRESENTATION_RECORDING_SCREENRECORDINGSELECTION_H
