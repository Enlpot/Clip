#pragma once

#include "clip_draw_engine.h"
#include "clip_draw_engine_qt/clip_canvas_types.h"

namespace clip_canvas_types {

QColor toQColor(const ClipColorRgba8& color);
ClipColorRgba8 toEngineColor(const QColor& color);

ClipCanvasTool toCanvasTool(ClipActiveTool tool);
ClipActiveTool toEngineTool(ClipCanvasTool tool);

ClipCanvasStyleToolbarSource toCanvasStyleToolbarSource(ClipStyleToolbarSource source);

ClipShapeKind toEngineShapeKind(ClipCanvasShapeKind kind);

ClipCanvasArrowhead toCanvasArrowhead(ClipArrowhead arrowhead);
ClipArrowhead toEngineArrowhead(ClipCanvasArrowhead arrowhead);

ClipCanvasArrowType toCanvasArrowType(ClipArrowType arrowType);
ClipArrowType toEngineArrowType(ClipCanvasArrowType arrowType);

ClipCanvasTextHorizontalAlign toCanvasTextHorizontalAlign(ClipTextHorizontalAlign align);
ClipTextHorizontalAlign toEngineTextHorizontalAlign(ClipCanvasTextHorizontalAlign align);

ClipCanvasTextVerticalAlign toCanvasTextVerticalAlign(ClipTextVerticalAlign align);
ClipTextVerticalAlign toEngineTextVerticalAlign(ClipCanvasTextVerticalAlign align);

ClipCanvasFillStyle toCanvasFillStyle(ClipFillStyle fillStyle);
ClipFillStyle toEngineFillStyle(ClipCanvasFillStyle fillStyle);

ClipCanvasStrokeStyle toCanvasStrokeStyle(ClipStrokeStyle strokeStyle);
ClipStrokeStyle toEngineStrokeStyle(ClipCanvasStrokeStyle strokeStyle);

ClipCanvasCornerRadii toCanvasCornerRadii(const ClipCornerRadii& cornerRadii);
ClipCornerRadii toEngineCornerRadii(const ClipCanvasCornerRadii& cornerRadii);

ClipCanvasShapeStyle toCanvasShapeStyle(const ClipShapeStyle& style);
ClipShapeStyle toEngineShapeStyle(const ClipCanvasShapeStyle& style);

bool validDistanceStyle(const ClipCanvasDistanceStyle& style);
ClipCanvasDistanceStyle toCanvasDistanceStyle(const ClipDistanceStyle& style);
ClipDistanceStyle toEngineDistanceStyle(const ClipCanvasDistanceStyle& style);

ClipCanvasTextStyle toCanvasTextStyle(const ClipTextStyle& style);
ClipTextStyle toEngineTextStyle(const ClipCanvasTextStyle& style);

ClipCanvasSerialNumberStyle toCanvasSerialNumberStyle(const ClipSerialNumberStyle& style);
ClipSerialNumberStyle toEngineSerialNumberStyle(const ClipCanvasSerialNumberStyle& style);

ClipCanvasStyleToolbarState toCanvasStyleToolbarState(const ClipStyleToolbarState& state);

ClipCanvasHistoryState toCanvasHistoryState(const ClipHistoryState& state);

ClipCanvasSnapConfig toCanvasSnapConfig(const ClipSnapConfig& config);
ClipSnapConfig toEngineSnapConfig(const ClipCanvasSnapConfig& config);

ClipCanvasGridConfig toCanvasGridConfig(const ClipGridConfig& config);
ClipGridConfig toEngineGridConfig(const ClipCanvasGridConfig& config);

ClipCanvasSpotlightConfig toCanvasSpotlightConfig(const ClipSpotlightConfig& config);
ClipSpotlightConfig toEngineSpotlightConfig(const ClipCanvasSpotlightConfig& config);

ClipCanvasWatermarkConfig toCanvasWatermarkConfig(const ClipWatermarkConfig& config);
ClipWatermarkConfig toEngineWatermarkConfig(const ClipCanvasWatermarkConfig& config);

bool toEngineStyleDefaults(const ClipCanvasStyleDefaults& defaults,
                           ClipStyleDefaults& engineDefaults);

} // namespace clip_canvas_types
