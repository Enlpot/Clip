#pragma once

#include "clip_draw_engine_qt/clip_canvas_types.h"

#include <variant>
#include <type_traits>

namespace clip_canvas_style_limits {
inline constexpr double minimumFontSize = 6.0;
inline constexpr double maximumTextFontSize = 256.0;
inline constexpr double maximumBadgeFontSize = 512.0;
inline constexpr double maximumWatermarkFontSize = 512.0;
} // namespace clip_canvas_style_limits

// Explicit user intent. Display synchronization and programmatic setters do not create edits.
enum ClipCanvasWatermarkProperty : quint32 {
    ClipCanvasWatermarkColor = 1u << 0,
    ClipCanvasWatermarkText = 1u << 1,
    ClipCanvasWatermarkTemplate = 1u << 2,
    ClipCanvasWatermarkFontSize = 1u << 3,
    ClipCanvasWatermarkFontFamily = 1u << 4,
    ClipCanvasWatermarkAngle = 1u << 5,
    ClipCanvasWatermarkGap = 1u << 6,
    ClipCanvasWatermarkOpacity = 1u << 7,
};
enum ClipCanvasSpotlightProperty : quint32 {
    ClipCanvasSpotlightColor = 1u << 0,
    ClipCanvasSpotlightOpacity = 1u << 1,
};

struct ClipCanvasShapeEdit {
    ClipCanvasShapeStyle style;
    quint32 properties = 0;
    ClipCanvasShapeKind kind = ClipCanvasShapeKind::Rectangle;
};
struct ClipCanvasTextEdit {
    ClipCanvasTextStyle style;
    quint32 properties = 0;
};
struct ClipCanvasSerialNumberEdit {
    ClipCanvasSerialNumberStyle style;
    quint32 properties = 0;
};
struct ClipCanvasFilterEdit {
    ClipCanvasFilterStyle style;
    quint32 properties = 0;
    bool pen = false;
};
struct ClipCanvasWatermarkEdit {
    ClipCanvasWatermarkConfig style;
    quint32 properties = 0;
};
struct ClipCanvasSpotlightEdit {
    ClipCanvasSpotlightConfig style;
    quint32 properties = 0;
};
struct ClipCanvasBrushEraserEdit {
    ClipCanvasBrushEraserStyle style;
    quint32 properties = ClipCanvasBrushEraserStylePropertyStrokeWidth;
};
struct ClipCanvasDistanceEdit {
    ClipCanvasDistanceStyle style;
    quint32 properties = 0;
};
using ClipCanvasStyleEdit =
    std::variant<ClipCanvasShapeEdit, ClipCanvasTextEdit, ClipCanvasSerialNumberEdit,
                 ClipCanvasFilterEdit, ClipCanvasWatermarkEdit, ClipCanvasSpotlightEdit,
                 ClipCanvasBrushEraserEdit, ClipCanvasDistanceEdit>;

inline void clipCanvasMergeStyle(ClipCanvasDistanceStyle& target,
                                 const ClipCanvasDistanceStyle& value, quint32 properties) {
    if (properties & ClipCanvasDistanceStylePropertyStrokeColor)
        target.stroke = value.stroke;
    if (properties & ClipCanvasDistanceStylePropertyStrokeWidth)
        target.strokeWidth = value.strokeWidth;
    if (properties & ClipCanvasDistanceStylePropertyFactor)
        target.factor = value.factor;
    if (properties & ClipCanvasDistanceStylePropertyUnit)
        target.unit = value.unit;
    if (properties & ClipCanvasDistanceStylePropertyDecimalPlaces)
        target.decimalPlaces = value.decimalPlaces;
    if (properties & ClipCanvasDistanceStylePropertyEndpointScale)
        target.endpointScale = value.endpointScale;
    if (properties & ClipCanvasDistanceStylePropertyEndpointStyle)
        target.endpointStyle = value.endpointStyle;
}

inline void clipCanvasMergeStyle(ClipCanvasShapeStyle& target, const ClipCanvasShapeStyle& value,
                                 quint32 properties) {
    if ((properties & ClipCanvasShapeStylePropertyFillColor) != 0)
        target.fill = value.fill;
    if ((properties & ClipCanvasShapeStylePropertyFillStyle) != 0)
        target.fillStyle = value.fillStyle;
    if ((properties & ClipCanvasShapeStylePropertyStrokeColor) != 0)
        target.stroke = value.stroke;
    if ((properties & ClipCanvasShapeStylePropertyStrokeWidth) != 0)
        target.strokeWidth = value.strokeWidth;
    if ((properties & ClipCanvasShapeStylePropertyCornerRadius) != 0)
        target.cornerRadii = value.cornerRadii;
    if ((properties & ClipCanvasShapeStylePropertyStartArrowhead) != 0)
        target.startArrowhead = value.startArrowhead;
    if ((properties & ClipCanvasShapeStylePropertyEndArrowhead) != 0)
        target.endArrowhead = value.endArrowhead;
    if ((properties & ClipCanvasShapeStylePropertyStrokeStyle) != 0)
        target.strokeStyle = value.strokeStyle;
    if ((properties & ClipCanvasShapeStylePropertyArrowShaftType) != 0)
        target.arrowShaftType = value.arrowShaftType;
    if ((properties & ClipCanvasShapeStylePropertyArrowRatio) != 0)
        target.arrowRatio = value.arrowRatio;
    if ((properties & ClipCanvasShapeStylePropertyArrowType) != 0)
        target.arrowType = value.arrowType;
    if ((properties & ClipCanvasShapeStylePropertyOpacity) != 0)
        target.opacity = value.opacity;
    if ((properties & ClipCanvasShapeStylePropertyShape) != 0)
        target.shape = value.shape;
}

inline void clipCanvasMergeStyle(ClipCanvasTextStyle& target, const ClipCanvasTextStyle& value,
                                 quint32 properties) {
    if ((properties & ClipCanvasTextStyleMixedColor) != 0)
        target.color = value.color;
    if ((properties & ClipCanvasTextStyleMixedFontSize) != 0)
        target.fontSize = value.fontSize;
    if ((properties & ClipCanvasTextStyleMixedFontFamily) != 0)
        target.fontFamily = value.fontFamily;
    if ((properties & ClipCanvasTextStyleMixedFill) != 0)
        target.fill = value.fill;
    if ((properties & ClipCanvasTextStyleMixedFillStyle) != 0)
        target.fillStyle = value.fillStyle;
    if ((properties & ClipCanvasTextStyleMixedStroke) != 0)
        target.stroke = value.stroke;
    if ((properties & ClipCanvasTextStyleMixedStrokeWidth) != 0)
        target.strokeWidth = value.strokeWidth;
    if ((properties & ClipCanvasTextStyleMixedCornerRadii) != 0)
        target.cornerRadii = value.cornerRadii;
    if ((properties & ClipCanvasTextStyleMixedHorizontalAlign) != 0)
        target.horizontalAlign = value.horizontalAlign;
    if ((properties & ClipCanvasTextStyleMixedVerticalAlign) != 0)
        target.verticalAlign = value.verticalAlign;
    if ((properties & ClipCanvasTextStyleMixedOpacity) != 0)
        target.opacity = value.opacity;
}

inline void clipCanvasMergeStyle(ClipCanvasSerialNumberStyle& target,
                                 const ClipCanvasSerialNumberStyle& value, quint32 properties) {
    if ((properties & ClipCanvasSerialNumberStyleMixedNumber) != 0)
        target.number = value.number;
    if ((properties & ClipCanvasSerialNumberStyleMixedColor) != 0)
        target.color = value.color;
    if ((properties & ClipCanvasSerialNumberStyleMixedFill) != 0)
        target.fill = value.fill;
    if ((properties & ClipCanvasSerialNumberStyleMixedFillStyle) != 0)
        target.fillStyle = value.fillStyle;
    if ((properties & ClipCanvasSerialNumberStyleMixedFontSize) != 0)
        target.fontSize = value.fontSize;
    if ((properties & ClipCanvasSerialNumberStyleMixedFontFamily) != 0)
        target.fontFamily = value.fontFamily;
    if ((properties & ClipCanvasSerialNumberStyleMixedStrokeWidth) != 0)
        target.strokeWidth = value.strokeWidth;
    if ((properties & ClipCanvasSerialNumberStyleMixedStrokeStyle) != 0)
        target.strokeStyle = value.strokeStyle;
    if ((properties & ClipCanvasSerialNumberStyleMixedOpacity) != 0)
        target.opacity = value.opacity;
    if ((properties & ClipCanvasSerialNumberStyleMixedNumericType) != 0)
        target.numericType = value.numericType;
    if ((properties & ClipCanvasSerialNumberStyleMixedType) != 0)
        target.type = value.type;
}

inline void clipCanvasMergeStyle(ClipCanvasFilterStyle& target, const ClipCanvasFilterStyle& value,
                                 quint32 properties) {
    if ((properties & ClipCanvasFilterStylePropertyType) != 0)
        target.type = value.type;
    if ((properties & ClipCanvasFilterStylePropertyStrength) != 0)
        target.strength = value.strength;
    if ((properties & ClipCanvasFilterStylePropertyOpacity) != 0)
        target.opacity = value.opacity;
    if ((properties & ClipCanvasFilterStylePropertyStrokeWidth) != 0)
        target.strokeWidth = value.strokeWidth;
}

inline void clipCanvasMergeStyle(ClipCanvasWatermarkConfig& target,
                                 const ClipCanvasWatermarkConfig& value, quint32 properties) {
    if ((properties & ClipCanvasWatermarkColor) != 0)
        target.color = value.color;
    if ((properties & ClipCanvasWatermarkText) != 0)
        target.text = value.text;
    if ((properties & ClipCanvasWatermarkTemplate) != 0)
        target.templateValue = value.templateValue;
    if ((properties & ClipCanvasWatermarkTemplate) != 0)
        target.templateApplicationTime = value.templateApplicationTime;
    if ((properties & ClipCanvasWatermarkFontSize) != 0)
        target.fontSize = value.fontSize;
    if ((properties & ClipCanvasWatermarkFontFamily) != 0)
        target.fontFamily = value.fontFamily;
    if ((properties & ClipCanvasWatermarkAngle) != 0)
        target.angle = value.angle;
    if ((properties & ClipCanvasWatermarkGap) != 0)
        target.gap = value.gap;
    if ((properties & ClipCanvasWatermarkOpacity) != 0)
        target.opacity = value.opacity;
}

inline void clipCanvasMergeStyle(ClipCanvasSpotlightConfig& target,
                                 const ClipCanvasSpotlightConfig& value, quint32 properties) {
    if ((properties & ClipCanvasSpotlightColor) != 0)
        target.color = value.color;
    if ((properties & ClipCanvasSpotlightOpacity) != 0)
        target.opacity = value.opacity;
}

inline void clipCanvasMergeStyle(ClipCanvasBrushEraserStyle& target,
                                 const ClipCanvasBrushEraserStyle& value, quint32 properties) {
    if ((properties & ClipCanvasBrushEraserStylePropertyStrokeWidth) != 0)
        target.strokeWidth = value.strokeWidth;
}

inline void clipCanvasMergeStyleEdit(ClipCanvasStyleDefaults& defaults,
                                     const ClipCanvasStyleEdit& edit) {
    std::visit(
        [&](const auto& patch) {
            using T = std::decay_t<decltype(patch)>;
            if constexpr (std::is_same_v<T, ClipCanvasShapeEdit>) {
                ClipCanvasShapeStyle* target = nullptr;
                switch (patch.kind) {
                case ClipCanvasShapeKind::Rectangle:
                    target = &defaults.rectangle;
                    break;
                case ClipCanvasShapeKind::Arrow:
                    target = &defaults.arrow;
                    break;
                case ClipCanvasShapeKind::Line:
                    target = &defaults.line;
                    break;
                case ClipCanvasShapeKind::FreeDraw:
                    target = &defaults.freeDraw;
                    break;
                case ClipCanvasShapeKind::RectangleHighlight:
                    target = &defaults.rectangleHighlight;
                    break;
                case ClipCanvasShapeKind::PenHighlight:
                    target = &defaults.penHighlight;
                    break;
                case ClipCanvasShapeKind::Spotlight:
                    if ((patch.properties & ClipCanvasShapeStylePropertyShape) != 0)
                        defaults.spotlightShape = patch.style.shape;
                    break;
                }
                if (target != nullptr)
                    clipCanvasMergeStyle(*target, patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasDistanceEdit>) {
                clipCanvasMergeStyle(defaults.distance, patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasTextEdit>) {
                clipCanvasMergeStyle(defaults.text, patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasSerialNumberEdit>) {
                clipCanvasMergeStyle(defaults.serialNumber, patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasFilterEdit>) {
                clipCanvasMergeStyle(patch.pen ? defaults.penFilter : defaults.rectangleFilter,
                                     patch.style, patch.properties);
                if ((patch.properties & ClipCanvasFilterStylePropertyStrength) != 0) {
                    defaults.penFilter.strength = patch.style.strength;
                    defaults.rectangleFilter.strength = patch.style.strength;
                }
            } else if constexpr (std::is_same_v<T, ClipCanvasWatermarkEdit>) {
                clipCanvasMergeStyle(defaults.watermark, patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasSpotlightEdit>) {
                clipCanvasMergeStyle(defaults.spotlight, patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasBrushEraserEdit>) {
                clipCanvasMergeStyle(defaults.brushEraser, patch.style, patch.properties);
            }
        },
        edit);
}
