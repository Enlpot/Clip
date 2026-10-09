#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLCOMMANDWORKFLOW_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLCOMMANDWORKFLOW_H

#include "clip_draw_engine_qt/clip_canvas_types.h"
#include "clip_shot/presentation/screenshotinteractionstate.h"
#include "clip_shot/presentation/screenshottoolcommandworkflowports.h"

class ScreenshotDisplaySession;
class ScreenshotGeometryMapper;
class ScreenshotInteractionState;
class ScreenshotIntelligentSelectionModel;
class ScreenshotSelectionModel;
struct ScreenshotCaptureState;

struct ScreenshotToolCommandWorkflowContext {
    ScreenshotCaptureState& captureState;
    ScreenshotToolCommandActions actions;
    ScreenshotDisplaySession& displaySession;
    const ScreenshotGeometryMapper& geometry;
    ScreenshotInteractionState& interaction;
    ScreenshotSelectionModel& selection;
    ScreenshotIntelligentSelectionModel& intelligentSelection;
};

class ScreenshotToolCommandWorkflow final {
  public:
    explicit ScreenshotToolCommandWorkflow(ScreenshotToolCommandWorkflowContext context);

    void setMoveTool();
    void setSelectTool();
    void setShapeTool();
    void setArrowTool();
    void setLineTool();
    void setDistanceTool();
    void setFreeDrawTool();
    void setHighlightTool();
    void setPenHighlightTool();
    void setSpotlightTool();
    void setEraserTool();
    void setRectangleEraserTool();
    void setBrushEraserTool();
    void setFilterTool();
    void setRectangleFilterTool();
    void setPenFilterTool();
    void setAutoFilterTool();
    void setWatermarkTool();
    void setWatermarkConfigFromToolbar(const ClipCanvasWatermarkConfig& config);
    void setSpotlightConfigFromToolbar(const ClipCanvasSpotlightConfig& config);
    void setFilterStyleFromToolbar(const ClipCanvasFilterStyle& style, quint32 properties);
    void setTextTool();
    void setSerialNumberTool();

    void decrementSelectedSerialNumbers();
    void incrementSelectedSerialNumbers();
    void createTextForSelectedSerialNumber();

    [[nodiscard]] ClipCanvasShapeStyle currentRectangleStyle() const;
    void setShapeStyleFromToolbar(const ClipCanvasShapeStyle& style, quint32 properties,
                                  ClipCanvasShapeKind kind);
    void setTextStyleFromToolbar(const ClipCanvasTextStyle& style, quint32 properties);
    void setSerialNumberStyleFromToolbar(const ClipCanvasSerialNumberStyle& style,
                                         std::optional<quint32> properties = std::nullopt);
    [[nodiscard]] bool stepStrokeWidth(int delta);

  private:
    void setCanvasTool(ScreenshotActiveTool activeTool, ClipCanvasTool canvasTool);

    ScreenshotToolCommandWorkflowContext m_context;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLCOMMANDWORKFLOW_H
