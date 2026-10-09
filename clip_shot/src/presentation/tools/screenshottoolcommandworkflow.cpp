#include "clip_shot/presentation/screenshottoolcommandworkflow.h"

#include "clip_shot/presentation/screenshotcapturestate.h"
#include "clip_shot/presentation/screenshotdisplaysession.h"
#include "clip_shot/presentation/screenshotgeometry.h"
#include "clip_shot/presentation/screenshotintelligentselectionmodel.h"
#include "clip_shot/presentation/screenshotinteractionstate.h"
#include "clip_shot/presentation/screenshotselectionmodel.h"
#include <QCursor>

#include <utility>

ScreenshotToolCommandWorkflow::ScreenshotToolCommandWorkflow(
    ScreenshotToolCommandWorkflowContext context)
    : m_context(std::move(context)) {}

void ScreenshotToolCommandWorkflow::setMoveTool() {
    const QRect selection = m_context.selection.pixelSelection();
    const bool hasSelection = selection.width() >= 1 && selection.height() >= 1;
    const bool selectorReady = m_context.actions.selectorReady();
    m_context.interaction.setMoveTool(hasSelection, selectorReady);
    m_context.actions.setCanvasInteractionEnabled(false);
    if (hasSelection) {
        m_context.actions.updateOverlayState();
        return;
    }

    if (selectorReady) {
        m_context.actions.updateOverlayCursors();
        m_context.actions.updateSelectorSelectionAt(
            m_context.geometry.physicalPositionForLogicalPoint(m_context.displaySession,
                                                               QCursor::pos()));
        return;
    }

    m_context.actions.clearSelectorSelection();
    m_context.actions.updateOverlayState();
    m_context.actions.startSelectorRefresh();
}

void ScreenshotToolCommandWorkflow::setSelectTool() {
    setCanvasTool(ScreenshotActiveTool::Select, ClipCanvasTool::Select);
}

void ScreenshotToolCommandWorkflow::setShapeTool() {
    setCanvasTool(ScreenshotActiveTool::Shape, ClipCanvasTool::Shape);
}

void ScreenshotToolCommandWorkflow::setArrowTool() {
    setCanvasTool(ScreenshotActiveTool::Arrow, ClipCanvasTool::Arrow);
}

void ScreenshotToolCommandWorkflow::setLineTool() {
    setCanvasTool(ScreenshotActiveTool::Line, ClipCanvasTool::Line);
}

void ScreenshotToolCommandWorkflow::setDistanceTool() {
    setCanvasTool(ScreenshotActiveTool::Distance, ClipCanvasTool::Distance);
}

void ScreenshotToolCommandWorkflow::setFreeDrawTool() {
    setCanvasTool(ScreenshotActiveTool::FreeDraw, ClipCanvasTool::FreeDraw);
}

void ScreenshotToolCommandWorkflow::setHighlightTool() {
    setCanvasTool(ScreenshotActiveTool::RectangleHighlight, ClipCanvasTool::RectangleHighlight);
}

void ScreenshotToolCommandWorkflow::setPenHighlightTool() {
    setCanvasTool(ScreenshotActiveTool::PenHighlight, ClipCanvasTool::PenHighlight);
}

void ScreenshotToolCommandWorkflow::setEraserTool() {
    setCanvasTool(ScreenshotActiveTool::Eraser, ClipCanvasTool::Eraser);
}

void ScreenshotToolCommandWorkflow::setRectangleEraserTool() {
    setCanvasTool(ScreenshotActiveTool::RectangleEraser, ClipCanvasTool::RectangleEraser);
}

void ScreenshotToolCommandWorkflow::setBrushEraserTool() {
    setCanvasTool(ScreenshotActiveTool::BrushEraser, ClipCanvasTool::BrushEraser);
}

void ScreenshotToolCommandWorkflow::setFilterTool() {
    setRectangleFilterTool();
}

void ScreenshotToolCommandWorkflow::setSpotlightTool() {
    setCanvasTool(ScreenshotActiveTool::Spotlight, ClipCanvasTool::Spotlight);
}

void ScreenshotToolCommandWorkflow::setRectangleFilterTool() {
    setCanvasTool(ScreenshotActiveTool::RectangleFilter, ClipCanvasTool::RectangleFilter);
}

void ScreenshotToolCommandWorkflow::setAutoFilterTool() {
    setCanvasTool(ScreenshotActiveTool::AutoFilter, ClipCanvasTool::AutoFilter);
}

void ScreenshotToolCommandWorkflow::setPenFilterTool() {
    setCanvasTool(ScreenshotActiveTool::PenFilter, ClipCanvasTool::PenFilter);
}

void ScreenshotToolCommandWorkflow::setWatermarkTool() {
    setCanvasTool(ScreenshotActiveTool::Watermark, ClipCanvasTool::Watermark);
}

void ScreenshotToolCommandWorkflow::setWatermarkConfigFromToolbar(
    const ClipCanvasWatermarkConfig& config) {
    m_context.actions.setWatermarkConfig(config);
}

void ScreenshotToolCommandWorkflow::setSpotlightConfigFromToolbar(
    const ClipCanvasSpotlightConfig& config) {
    m_context.actions.setSpotlightConfig(config);
}

void ScreenshotToolCommandWorkflow::setFilterStyleFromToolbar(const ClipCanvasFilterStyle& style,
                                                              quint32 properties) {
    m_context.actions.setFilterStyle(style, properties);
}

void ScreenshotToolCommandWorkflow::setTextTool() {
    setCanvasTool(ScreenshotActiveTool::Text, ClipCanvasTool::Text);
}

void ScreenshotToolCommandWorkflow::setSerialNumberTool() {
    setCanvasTool(ScreenshotActiveTool::SerialNumber, ClipCanvasTool::SerialNumber);
}

void ScreenshotToolCommandWorkflow::decrementSelectedSerialNumbers() {
    m_context.actions.adjustSelectedSerialNumbers(-1);
}

void ScreenshotToolCommandWorkflow::incrementSelectedSerialNumbers() {
    m_context.actions.adjustSelectedSerialNumbers(1);
}

void ScreenshotToolCommandWorkflow::createTextForSelectedSerialNumber() {
    m_context.actions.createCanvasTextForSelectedSerialNumber();
}

ClipCanvasShapeStyle ScreenshotToolCommandWorkflow::currentRectangleStyle() const {
    ClipCanvasShapeStyle style;
    return m_context.actions.tryCurrentRectangleStyle(&style) ? style : ClipCanvasShapeStyle{};
}

void ScreenshotToolCommandWorkflow::setShapeStyleFromToolbar(const ClipCanvasShapeStyle& style,
                                                             quint32 properties,
                                                             ClipCanvasShapeKind kind) {
    m_context.actions.setShapeStylePatch(style, properties, kind);
}

void ScreenshotToolCommandWorkflow::setTextStyleFromToolbar(const ClipCanvasTextStyle& style,
                                                            quint32 properties) {
    m_context.actions.setTextStyle(style, properties);
}

void ScreenshotToolCommandWorkflow::setSerialNumberStyleFromToolbar(
    const ClipCanvasSerialNumberStyle& style, std::optional<quint32> properties) {
    m_context.actions.setSerialNumberStyle(style, properties);
}

bool ScreenshotToolCommandWorkflow::stepStrokeWidth(int delta) {
    return m_context.actions.stepToolbarStrokeWidth(delta);
}

void ScreenshotToolCommandWorkflow::setCanvasTool(ScreenshotActiveTool activeTool,
                                                  ClipCanvasTool canvasTool) {
    m_context.interaction.setCanvasTool(activeTool);
    m_context.captureState.sessionState = ScreenshotSessionState::Editing;
    m_context.intelligentSelection.clearPress();
    m_context.actions.setCanvasInteractionEnabled(true);
    m_context.actions.setCanvasTool(canvasTool);
    m_context.actions.updateOverlayState();
    m_context.actions.updateOverlayCursors();
    m_context.actions.raiseToolbarForCanvasInteraction();
}
