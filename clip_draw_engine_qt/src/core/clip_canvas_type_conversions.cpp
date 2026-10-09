#include "clip_canvas_type_conversions.h"

#include "clip_canvas_utf8.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace clip_canvas_types {
namespace {

std::uint8_t toFlag(bool value) {
    return value ? 1 : 0;
}

bool fromFlag(std::uint8_t value) {
    return value != 0;
}

QString fontFamilyFromEngine(const char* utf8, std::uint32_t length, std::uint32_t capacity) {
    return clip_canvas_utf8::stringFromField(utf8, length, capacity);
}

void copyFontFamilyToEngine(const QString& fontFamily, char* target, std::uint32_t& length,
                            std::uint8_t& truncated, std::uint32_t capacity) {
    clip_canvas_utf8::copyStringToField(fontFamily.trimmed(), target, length, truncated, capacity);
}

template <typename Enum> bool enumInRange(Enum value, Enum first, Enum last) {
    using Underlying = std::underlying_type_t<Enum>;
    const Underlying raw = static_cast<Underlying>(value);
    return raw >= static_cast<Underlying>(first) && raw <= static_cast<Underlying>(last);
}

bool validShapeStyleEnums(const ClipCanvasShapeStyle& style) {
    return enumInRange(style.fillStyle, ClipCanvasFillStyle::Line, ClipCanvasFillStyle::Solid) &&
           enumInRange(style.startArrowhead, ClipCanvasArrowhead::None,
                       ClipCanvasArrowhead::IndentedTriangle) &&
           enumInRange(style.endArrowhead, ClipCanvasArrowhead::None,
                       ClipCanvasArrowhead::IndentedTriangle) &&
           enumInRange(style.strokeStyle, ClipCanvasStrokeStyle::Solid,
                       ClipCanvasStrokeStyle::Dotted) &&
           enumInRange(style.arrowShaftType, ClipCanvasArrowShaftType::Plain,
                       ClipCanvasArrowShaftType::Tapered) &&
           enumInRange(style.arrowType, ClipCanvasArrowType::Straight,
                       ClipCanvasArrowType::Elbow) &&
           enumInRange(style.highlightShape, ClipCanvasHighlightShape::Rectangle,
                       ClipCanvasHighlightShape::Ellipse) &&
           enumInRange(style.shape, ClipCanvasRectangleShape::Rectangle,
                       ClipCanvasRectangleShape::Diamond);
}

} // namespace

QColor toQColor(const ClipColorRgba8& color) {
    return QColor(color.r, color.g, color.b, color.a);
}

ClipColorRgba8 toEngineColor(const QColor& color) {
    const QColor rgba = color.isValid() ? color.toRgb() : QColor(0, 0, 0, 0);
    return ClipColorRgba8{
        static_cast<std::uint8_t>(rgba.red()),
        static_cast<std::uint8_t>(rgba.green()),
        static_cast<std::uint8_t>(rgba.blue()),
        static_cast<std::uint8_t>(rgba.alpha()),
    };
}

ClipCanvasTool toCanvasTool(ClipActiveTool tool) {
    switch (tool) {
    case CLIP_ACTIVE_TOOL_SELECT:
        return ClipCanvasTool::Select;
    case CLIP_ACTIVE_TOOL_SHAPE:
        return ClipCanvasTool::Shape;
    case CLIP_ACTIVE_TOOL_ARROW:
        return ClipCanvasTool::Arrow;
    case CLIP_ACTIVE_TOOL_LINE:
        return ClipCanvasTool::Line;
    case CLIP_ACTIVE_TOOL_FREE_DRAW:
        return ClipCanvasTool::FreeDraw;
    case CLIP_ACTIVE_TOOL_RECTANGLE_HIGHLIGHT:
        return ClipCanvasTool::RectangleHighlight;
    case CLIP_ACTIVE_TOOL_PEN_HIGHLIGHT:
        return ClipCanvasTool::PenHighlight;
    case CLIP_ACTIVE_TOOL_ERASER:
        return ClipCanvasTool::Eraser;
    case CLIP_ACTIVE_TOOL_RECTANGLE_FILTER:
        return ClipCanvasTool::RectangleFilter;
    case CLIP_ACTIVE_TOOL_PEN_FILTER:
        return ClipCanvasTool::PenFilter;
    case CLIP_ACTIVE_TOOL_WATERMARK:
        return ClipCanvasTool::Watermark;
    case CLIP_ACTIVE_TOOL_TEXT:
        return ClipCanvasTool::Text;
    case CLIP_ACTIVE_TOOL_SERIAL_NUMBER:
        return ClipCanvasTool::SerialNumber;
    case CLIP_ACTIVE_TOOL_AUTO_FILTER:
        return ClipCanvasTool::AutoFilter;
    case CLIP_ACTIVE_TOOL_SPOTLIGHT:
        return ClipCanvasTool::Spotlight;
    case CLIP_ACTIVE_TOOL_RECTANGLE_ERASER:
        return ClipCanvasTool::RectangleEraser;
    case CLIP_ACTIVE_TOOL_BRUSH_ERASER:
        return ClipCanvasTool::BrushEraser;
    case CLIP_ACTIVE_TOOL_DISTANCE:
        return ClipCanvasTool::Distance;
    }
    return ClipCanvasTool::Select;
}

ClipActiveTool toEngineTool(ClipCanvasTool tool) {
    switch (tool) {
    case ClipCanvasTool::Select:
        return CLIP_ACTIVE_TOOL_SELECT;
    case ClipCanvasTool::Shape:
        return CLIP_ACTIVE_TOOL_SHAPE;
    case ClipCanvasTool::Arrow:
        return CLIP_ACTIVE_TOOL_ARROW;
    case ClipCanvasTool::Line:
        return CLIP_ACTIVE_TOOL_LINE;
    case ClipCanvasTool::FreeDraw:
        return CLIP_ACTIVE_TOOL_FREE_DRAW;
    case ClipCanvasTool::RectangleHighlight:
        return CLIP_ACTIVE_TOOL_RECTANGLE_HIGHLIGHT;
    case ClipCanvasTool::PenHighlight:
        return CLIP_ACTIVE_TOOL_PEN_HIGHLIGHT;
    case ClipCanvasTool::Eraser:
        return CLIP_ACTIVE_TOOL_ERASER;
    case ClipCanvasTool::RectangleFilter:
        return CLIP_ACTIVE_TOOL_RECTANGLE_FILTER;
    case ClipCanvasTool::PenFilter:
        return CLIP_ACTIVE_TOOL_PEN_FILTER;
    case ClipCanvasTool::Watermark:
        return CLIP_ACTIVE_TOOL_WATERMARK;
    case ClipCanvasTool::Text:
        return CLIP_ACTIVE_TOOL_TEXT;
    case ClipCanvasTool::SerialNumber:
        return CLIP_ACTIVE_TOOL_SERIAL_NUMBER;
    case ClipCanvasTool::AutoFilter:
        return CLIP_ACTIVE_TOOL_AUTO_FILTER;
    case ClipCanvasTool::Spotlight:
        return CLIP_ACTIVE_TOOL_SPOTLIGHT;
    case ClipCanvasTool::RectangleEraser:
        return CLIP_ACTIVE_TOOL_RECTANGLE_ERASER;
    case ClipCanvasTool::BrushEraser:
        return CLIP_ACTIVE_TOOL_BRUSH_ERASER;
    case ClipCanvasTool::Distance:
        return CLIP_ACTIVE_TOOL_DISTANCE;
    }
    return CLIP_ACTIVE_TOOL_SELECT;
}

ClipCanvasStyleToolbarSource toCanvasStyleToolbarSource(ClipStyleToolbarSource source) {
    switch (source) {
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE:
        return ClipCanvasStyleToolbarSource::DefaultRectangle;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE:
        return ClipCanvasStyleToolbarSource::SelectedRectangle;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_ARROW:
        return ClipCanvasStyleToolbarSource::DefaultArrow;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_ARROW:
        return ClipCanvasStyleToolbarSource::SelectedArrow;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_LINE:
        return ClipCanvasStyleToolbarSource::DefaultLine;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_LINE:
        return ClipCanvasStyleToolbarSource::SelectedLine;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_FREE_DRAW:
        return ClipCanvasStyleToolbarSource::DefaultFreeDraw;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_FREE_DRAW:
        return ClipCanvasStyleToolbarSource::SelectedFreeDraw;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_HIGHLIGHT:
        return ClipCanvasStyleToolbarSource::DefaultRectangleHighlight;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE_HIGHLIGHT:
        return ClipCanvasStyleToolbarSource::SelectedRectangleHighlight;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_PEN_HIGHLIGHT:
        return ClipCanvasStyleToolbarSource::DefaultPenHighlight;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_PEN_HIGHLIGHT:
        return ClipCanvasStyleToolbarSource::SelectedPenHighlight;
    case CLIP_STYLE_TOOLBAR_SOURCE_ERASER:
        return ClipCanvasStyleToolbarSource::Eraser;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_FILTER:
        return ClipCanvasStyleToolbarSource::DefaultRectangleFilter;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_RECTANGLE_FILTER:
        return ClipCanvasStyleToolbarSource::SelectedRectangleFilter;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_PEN_FILTER:
        return ClipCanvasStyleToolbarSource::DefaultPenFilter;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_PEN_FILTER:
        return ClipCanvasStyleToolbarSource::SelectedPenFilter;
    case CLIP_STYLE_TOOLBAR_SOURCE_WATERMARK:
        return ClipCanvasStyleToolbarSource::Watermark;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_TEXT:
        return ClipCanvasStyleToolbarSource::DefaultText;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_TEXT:
        return ClipCanvasStyleToolbarSource::SelectedText;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_SERIAL_NUMBER:
        return ClipCanvasStyleToolbarSource::DefaultSerialNumber;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_SERIAL_NUMBER:
        return ClipCanvasStyleToolbarSource::SelectedSerialNumber;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_SPOTLIGHT:
        return ClipCanvasStyleToolbarSource::DefaultSpotlight;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_SPOTLIGHT:
        return ClipCanvasStyleToolbarSource::SelectedSpotlight;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_RECTANGLE_ERASER:
        return ClipCanvasStyleToolbarSource::DefaultRectangleEraser;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_BRUSH_ERASER:
        return ClipCanvasStyleToolbarSource::DefaultBrushEraser;
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_DISTANCE:
        return ClipCanvasStyleToolbarSource::DefaultDistance;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_DISTANCE:
        return ClipCanvasStyleToolbarSource::SelectedDistance;
    }
    return ClipCanvasStyleToolbarSource::DefaultRectangle;
}

ClipShapeKind toEngineShapeKind(ClipCanvasShapeKind kind) {
    switch (kind) {
    case ClipCanvasShapeKind::Rectangle:
        return CLIP_SHAPE_KIND_RECTANGLE;
    case ClipCanvasShapeKind::Arrow:
        return CLIP_SHAPE_KIND_ARROW;
    case ClipCanvasShapeKind::Line:
        return CLIP_SHAPE_KIND_LINE;
    case ClipCanvasShapeKind::FreeDraw:
        return CLIP_SHAPE_KIND_FREE_DRAW;
    case ClipCanvasShapeKind::RectangleHighlight:
        return CLIP_SHAPE_KIND_RECTANGLE_HIGHLIGHT;
    case ClipCanvasShapeKind::PenHighlight:
        return CLIP_SHAPE_KIND_PEN_HIGHLIGHT;
    case ClipCanvasShapeKind::Spotlight:
        return CLIP_SHAPE_KIND_SPOTLIGHT;
    }
    return CLIP_SHAPE_KIND_RECTANGLE;
}

ClipCanvasArrowhead toCanvasArrowhead(ClipArrowhead arrowhead) {
    switch (arrowhead) {
    case CLIP_ARROWHEAD_SQUARE:
    case CLIP_ARROWHEAD_INVERTED_TRIANGLE:
    case CLIP_ARROWHEAD_NONE:
        return ClipCanvasArrowhead::None;
    case CLIP_ARROWHEAD_ARROW:
        return ClipCanvasArrowhead::Arrow;
    case CLIP_ARROWHEAD_BAR:
        return ClipCanvasArrowhead::Bar;
    case CLIP_ARROWHEAD_DOT:
        return ClipCanvasArrowhead::Dot;
    case CLIP_ARROWHEAD_CIRCLE:
        return ClipCanvasArrowhead::Circle;
    case CLIP_ARROWHEAD_CIRCLE_OUTLINE:
        return ClipCanvasArrowhead::CircleOutline;
    case CLIP_ARROWHEAD_INDENTED_TRIANGLE:
        return ClipCanvasArrowhead::IndentedTriangle;
    case CLIP_ARROWHEAD_TRIANGLE:
        return ClipCanvasArrowhead::Triangle;
    case CLIP_ARROWHEAD_TRIANGLE_OUTLINE:
        return ClipCanvasArrowhead::TriangleOutline;
    case CLIP_ARROWHEAD_DIAMOND:
        return ClipCanvasArrowhead::Diamond;
    case CLIP_ARROWHEAD_DIAMOND_OUTLINE:
        return ClipCanvasArrowhead::DiamondOutline;
    case CLIP_ARROWHEAD_CROWFOOT_ONE:
        return ClipCanvasArrowhead::CrowfootOne;
    case CLIP_ARROWHEAD_CROWFOOT_MANY:
        return ClipCanvasArrowhead::CrowfootMany;
    case CLIP_ARROWHEAD_CROWFOOT_ONE_OR_MANY:
        return ClipCanvasArrowhead::CrowfootOneOrMany;
    }
    return ClipCanvasArrowhead::None;
}

ClipArrowhead toEngineArrowhead(ClipCanvasArrowhead arrowhead) {
    switch (arrowhead) {
    case ClipCanvasArrowhead::None:
        return CLIP_ARROWHEAD_NONE;
    case ClipCanvasArrowhead::Arrow:
        return CLIP_ARROWHEAD_ARROW;
    case ClipCanvasArrowhead::Bar:
        return CLIP_ARROWHEAD_BAR;
    case ClipCanvasArrowhead::Dot:
        return CLIP_ARROWHEAD_DOT;
    case ClipCanvasArrowhead::Circle:
        return CLIP_ARROWHEAD_CIRCLE;
    case ClipCanvasArrowhead::CircleOutline:
        return CLIP_ARROWHEAD_CIRCLE_OUTLINE;
    case ClipCanvasArrowhead::IndentedTriangle:
        return CLIP_ARROWHEAD_INDENTED_TRIANGLE;
    case ClipCanvasArrowhead::Triangle:
        return CLIP_ARROWHEAD_TRIANGLE;
    case ClipCanvasArrowhead::TriangleOutline:
        return CLIP_ARROWHEAD_TRIANGLE_OUTLINE;
    case ClipCanvasArrowhead::Diamond:
        return CLIP_ARROWHEAD_DIAMOND;
    case ClipCanvasArrowhead::DiamondOutline:
        return CLIP_ARROWHEAD_DIAMOND_OUTLINE;
    case ClipCanvasArrowhead::CrowfootOne:
        return CLIP_ARROWHEAD_CROWFOOT_ONE;
    case ClipCanvasArrowhead::CrowfootMany:
        return CLIP_ARROWHEAD_CROWFOOT_MANY;
    case ClipCanvasArrowhead::CrowfootOneOrMany:
        return CLIP_ARROWHEAD_CROWFOOT_ONE_OR_MANY;
    }
    return CLIP_ARROWHEAD_NONE;
}

ClipCanvasArrowType toCanvasArrowType(ClipArrowType arrowType) {
    switch (arrowType) {
    case CLIP_ARROW_TYPE_STRAIGHT:
        return ClipCanvasArrowType::Straight;
    case CLIP_ARROW_TYPE_CURVE:
        return ClipCanvasArrowType::Curve;
    case CLIP_ARROW_TYPE_ELBOW:
        return ClipCanvasArrowType::Elbow;
    }
    return ClipCanvasArrowType::Straight;
}

ClipArrowType toEngineArrowType(ClipCanvasArrowType arrowType) {
    switch (arrowType) {
    case ClipCanvasArrowType::Straight:
        return CLIP_ARROW_TYPE_STRAIGHT;
    case ClipCanvasArrowType::Curve:
        return CLIP_ARROW_TYPE_CURVE;
    case ClipCanvasArrowType::Elbow:
        return CLIP_ARROW_TYPE_ELBOW;
    }
    return CLIP_ARROW_TYPE_STRAIGHT;
}

ClipCanvasTextHorizontalAlign toCanvasTextHorizontalAlign(ClipTextHorizontalAlign align) {
    switch (align) {
    case CLIP_TEXT_HORIZONTAL_ALIGN_LEFT:
        return ClipCanvasTextHorizontalAlign::Left;
    case CLIP_TEXT_HORIZONTAL_ALIGN_CENTER:
        return ClipCanvasTextHorizontalAlign::Center;
    case CLIP_TEXT_HORIZONTAL_ALIGN_RIGHT:
        return ClipCanvasTextHorizontalAlign::Right;
    }
    return ClipCanvasTextHorizontalAlign::Left;
}

ClipTextHorizontalAlign toEngineTextHorizontalAlign(ClipCanvasTextHorizontalAlign align) {
    switch (align) {
    case ClipCanvasTextHorizontalAlign::Left:
        return CLIP_TEXT_HORIZONTAL_ALIGN_LEFT;
    case ClipCanvasTextHorizontalAlign::Center:
        return CLIP_TEXT_HORIZONTAL_ALIGN_CENTER;
    case ClipCanvasTextHorizontalAlign::Right:
        return CLIP_TEXT_HORIZONTAL_ALIGN_RIGHT;
    }
    return CLIP_TEXT_HORIZONTAL_ALIGN_LEFT;
}

ClipCanvasTextVerticalAlign toCanvasTextVerticalAlign(ClipTextVerticalAlign align) {
    switch (align) {
    case CLIP_TEXT_VERTICAL_ALIGN_TOP:
        return ClipCanvasTextVerticalAlign::Top;
    case CLIP_TEXT_VERTICAL_ALIGN_CENTER:
        return ClipCanvasTextVerticalAlign::Center;
    case CLIP_TEXT_VERTICAL_ALIGN_BOTTOM:
        return ClipCanvasTextVerticalAlign::Bottom;
    }
    return ClipCanvasTextVerticalAlign::Center;
}

ClipTextVerticalAlign toEngineTextVerticalAlign(ClipCanvasTextVerticalAlign align) {
    switch (align) {
    case ClipCanvasTextVerticalAlign::Top:
        return CLIP_TEXT_VERTICAL_ALIGN_TOP;
    case ClipCanvasTextVerticalAlign::Center:
        return CLIP_TEXT_VERTICAL_ALIGN_CENTER;
    case ClipCanvasTextVerticalAlign::Bottom:
        return CLIP_TEXT_VERTICAL_ALIGN_BOTTOM;
    }
    return CLIP_TEXT_VERTICAL_ALIGN_CENTER;
}

ClipCanvasFillStyle toCanvasFillStyle(ClipFillStyle fillStyle) {
    switch (fillStyle) {
    case CLIP_FILL_STYLE_LINE:
        return ClipCanvasFillStyle::Line;
    case CLIP_FILL_STYLE_CROSS_LINE:
        return ClipCanvasFillStyle::CrossLine;
    case CLIP_FILL_STYLE_SOLID:
        return ClipCanvasFillStyle::Solid;
    }
    return ClipCanvasFillStyle::Solid;
}

ClipFillStyle toEngineFillStyle(ClipCanvasFillStyle fillStyle) {
    switch (fillStyle) {
    case ClipCanvasFillStyle::Line:
        return CLIP_FILL_STYLE_LINE;
    case ClipCanvasFillStyle::CrossLine:
        return CLIP_FILL_STYLE_CROSS_LINE;
    case ClipCanvasFillStyle::Solid:
        return CLIP_FILL_STYLE_SOLID;
    }
    return CLIP_FILL_STYLE_SOLID;
}

ClipCanvasStrokeStyle toCanvasStrokeStyle(ClipStrokeStyle strokeStyle) {
    switch (strokeStyle) {
    case CLIP_STROKE_STYLE_SOLID:
        return ClipCanvasStrokeStyle::Solid;
    case CLIP_STROKE_STYLE_DASHED:
        return ClipCanvasStrokeStyle::Dashed;
    case CLIP_STROKE_STYLE_DOTTED:
        return ClipCanvasStrokeStyle::Dotted;
    }
    return ClipCanvasStrokeStyle::Solid;
}

ClipStrokeStyle toEngineStrokeStyle(ClipCanvasStrokeStyle strokeStyle) {
    switch (strokeStyle) {
    case ClipCanvasStrokeStyle::Solid:
        return CLIP_STROKE_STYLE_SOLID;
    case ClipCanvasStrokeStyle::Dashed:
        return CLIP_STROKE_STYLE_DASHED;
    case ClipCanvasStrokeStyle::Dotted:
        return CLIP_STROKE_STYLE_DOTTED;
    }
    return CLIP_STROKE_STYLE_SOLID;
}

ClipCanvasCornerRadii toCanvasCornerRadii(const ClipCornerRadii& cornerRadii) {
    return ClipCanvasCornerRadii{
        cornerRadii.top_left,
        cornerRadii.top_right,
        cornerRadii.bottom_right,
        cornerRadii.bottom_left,
    };
}

ClipCornerRadii toEngineCornerRadii(const ClipCanvasCornerRadii& cornerRadii) {
    return ClipCornerRadii{
        cornerRadii.topLeft,
        cornerRadii.topRight,
        cornerRadii.bottomRight,
        cornerRadii.bottomLeft,
    };
}

ClipCanvasShapeStyle toCanvasShapeStyle(const ClipShapeStyle& style) {
    return ClipCanvasShapeStyle{
        toQColor(style.fill),
        toCanvasFillStyle(style.fill_style),
        toQColor(style.stroke),
        style.stroke_width,
        toCanvasCornerRadii(style.corner_radii),
        toCanvasArrowhead(style.start_arrowhead),
        toCanvasArrowhead(style.end_arrowhead),
        toCanvasStrokeStyle(style.stroke_style),
        toCanvasArrowType(style.arrow_type),
        static_cast<ClipCanvasArrowShaftType>(style.arrow_shaft_type),
        style.arrow_ratio,
        style.opacity,
        style.highlight_shape == CLIP_HIGHLIGHT_SHAPE_ELLIPSE ? ClipCanvasHighlightShape::Ellipse
                                                              : ClipCanvasHighlightShape::Rectangle,
        style.shape == CLIP_RECTANGLE_SHAPE_ELLIPSE   ? ClipCanvasRectangleShape::Ellipse
        : style.shape == CLIP_RECTANGLE_SHAPE_DIAMOND ? ClipCanvasRectangleShape::Diamond
                                                      : ClipCanvasRectangleShape::Rectangle,
    };
}

ClipShapeStyle toEngineShapeStyle(const ClipCanvasShapeStyle& style) {
    ClipShapeStyle engineStyle{};
    engineStyle.fill = toEngineColor(style.fill);
    engineStyle.fill_style = toEngineFillStyle(style.fillStyle);
    engineStyle.stroke = toEngineColor(style.stroke);
    engineStyle.stroke_width = style.strokeWidth;
    engineStyle.corner_radii = toEngineCornerRadii(style.cornerRadii);
    engineStyle.start_arrowhead = toEngineArrowhead(style.startArrowhead);
    engineStyle.end_arrowhead = toEngineArrowhead(style.endArrowhead);
    engineStyle.stroke_style = toEngineStrokeStyle(style.strokeStyle);
    engineStyle.arrow_type = toEngineArrowType(style.arrowType);
    engineStyle.arrow_shaft_type = static_cast<ClipArrowShaftType>(style.arrowShaftType);
    engineStyle.arrow_ratio = clipCanvasNormalizeArrowRatio(style.arrowRatio);
    engineStyle.opacity = style.opacity;
    engineStyle.highlight_shape = style.highlightShape == ClipCanvasHighlightShape::Ellipse
                                      ? CLIP_HIGHLIGHT_SHAPE_ELLIPSE
                                      : CLIP_HIGHLIGHT_SHAPE_RECTANGLE;
    engineStyle.shape =
        style.shape == ClipCanvasRectangleShape::Ellipse   ? CLIP_RECTANGLE_SHAPE_ELLIPSE
        : style.shape == ClipCanvasRectangleShape::Diamond ? CLIP_RECTANGLE_SHAPE_DIAMOND
                                                           : CLIP_RECTANGLE_SHAPE_RECTANGLE;
    return engineStyle;
}

bool validDistanceStyle(const ClipCanvasDistanceStyle& style) {
    return style.stroke.isValid() && std::isfinite(style.strokeWidth) && style.strokeWidth >= 1.0 &&
           style.strokeWidth <= 72.0 && std::isfinite(style.factor) && style.factor >= 0.01 &&
           style.factor <= 1000.0 &&
           enumInRange(style.unit, ClipCanvasDistanceUnit::Px, ClipCanvasDistanceUnit::Mm) &&
           style.decimalPlaces >= 0 && style.decimalPlaces <= 3 &&
           std::isfinite(style.endpointScale) && style.endpointScale >= 0.5 &&
           style.endpointScale <= 3.0 &&
           enumInRange(style.endpointStyle, ClipCanvasArrowhead::None,
                       ClipCanvasArrowhead::IndentedTriangle);
}

ClipCanvasDistanceStyle toCanvasDistanceStyle(const ClipDistanceStyle& style) {
    return {toQColor(style.stroke),
            style.stroke_width,
            style.factor,
            static_cast<ClipCanvasDistanceUnit>(style.unit),
            static_cast<int>(style.decimal_places),
            style.endpoint_scale,
            toCanvasArrowhead(style.endpoint_style)};
}

ClipDistanceStyle toEngineDistanceStyle(const ClipCanvasDistanceStyle& style) {
    ClipDistanceStyle result{};
    result.stroke = toEngineColor(style.stroke);
    result.stroke_width = style.strokeWidth;
    result.factor = style.factor;
    result.unit = static_cast<ClipDistanceUnit>(style.unit);
    result.decimal_places = static_cast<std::uint32_t>(style.decimalPlaces);
    result.endpoint_scale = style.endpointScale;
    result.endpoint_style = toEngineArrowhead(style.endpointStyle);
    return result;
}

ClipCanvasTextStyle toCanvasTextStyle(const ClipTextStyle& style) {
    return ClipCanvasTextStyle{
        toQColor(style.color),
        style.font_size,
        fontFamilyFromEngine(style.font_family_utf8, style.font_family_utf8_len,
                             CLIP_FONT_FAMILY_UTF8_CAPACITY),
        toQColor(style.fill),
        toCanvasFillStyle(style.fill_style),
        toQColor(style.stroke),
        style.stroke_width,
        toCanvasCornerRadii(style.corner_radii),
        toCanvasTextHorizontalAlign(style.horizontal_align),
        toCanvasTextVerticalAlign(style.vertical_align),
        style.opacity,
    };
}

ClipTextStyle toEngineTextStyle(const ClipCanvasTextStyle& style) {
    ClipTextStyle engineStyle{};
    engineStyle.color = toEngineColor(style.color);
    engineStyle.font_size = style.fontSize;
    copyFontFamilyToEngine(style.fontFamily, engineStyle.font_family_utf8,
                           engineStyle.font_family_utf8_len, engineStyle.font_family_truncated,
                           CLIP_FONT_FAMILY_UTF8_CAPACITY);
    engineStyle.fill = toEngineColor(style.fill);
    engineStyle.fill_style = toEngineFillStyle(style.fillStyle);
    engineStyle.stroke = toEngineColor(style.stroke);
    engineStyle.stroke_width = style.strokeWidth;
    engineStyle.corner_radii = toEngineCornerRadii(style.cornerRadii);
    engineStyle.horizontal_align = toEngineTextHorizontalAlign(style.horizontalAlign);
    engineStyle.vertical_align = toEngineTextVerticalAlign(style.verticalAlign);
    engineStyle.opacity = style.opacity;
    return engineStyle;
}

ClipCanvasSerialNumberStyle toCanvasSerialNumberStyle(const ClipSerialNumberStyle& style) {
    return ClipCanvasSerialNumberStyle{
        static_cast<qint64>(style.number),
        static_cast<ClipCanvasSerialNumberType>(style.serial_number_type),
        static_cast<ClipCanvasSerialNumberNumericType>(style.numeric_type),
        toQColor(style.color),
        toQColor(style.fill),
        toCanvasFillStyle(style.fill_style),
        style.font_size,
        fontFamilyFromEngine(style.font_family_utf8, style.font_family_utf8_len,
                             CLIP_FONT_FAMILY_UTF8_CAPACITY),
        style.stroke_width,
        toCanvasStrokeStyle(style.stroke_style),
        style.opacity,
    };
}

ClipSerialNumberStyle toEngineSerialNumberStyle(const ClipCanvasSerialNumberStyle& style) {
    ClipSerialNumberStyle engineStyle{};
    engineStyle.number = static_cast<std::int64_t>(style.number);
    engineStyle.serial_number_type = static_cast<ClipSerialNumberType>(style.type);
    engineStyle.numeric_type = static_cast<ClipSerialNumberNumericType>(style.numericType);
    engineStyle.color = toEngineColor(style.color);
    engineStyle.fill = toEngineColor(style.fill);
    engineStyle.fill_style = toEngineFillStyle(style.fillStyle);
    engineStyle.font_size = style.fontSize;
    copyFontFamilyToEngine(style.fontFamily, engineStyle.font_family_utf8,
                           engineStyle.font_family_utf8_len, engineStyle.font_family_truncated,
                           CLIP_FONT_FAMILY_UTF8_CAPACITY);
    engineStyle.stroke_width = style.strokeWidth;
    engineStyle.stroke_style = toEngineStrokeStyle(style.strokeStyle);
    engineStyle.opacity = style.opacity;
    return engineStyle;
}

ClipCanvasStyleToolbarState toCanvasStyleToolbarState(const ClipStyleToolbarState& state) {
    return ClipCanvasStyleToolbarState{
        toCanvasStyleToolbarSource(state.source),
        state.selected_element_count,
        toCanvasShapeStyle(state.shape_style),
        toCanvasTextStyle(state.text_style),
        toCanvasSerialNumberStyle(state.serial_number_style),
        state.text_style_mixed,
        state.serial_number_style_mixed,
        state.shape_style_mixed,
        ClipCanvasFilterStyle{
            static_cast<ClipCanvasFilterType>(state.filter_style.filter_type),
            state.filter_style.strength,
            state.filter_style.opacity,
            state.filter_style.stroke_width,
        },
        state.filter_style_mixed,
        false,
        ClipCanvasBrushEraserStyle{state.brush_eraser_style.stroke_width},
        toCanvasDistanceStyle(state.distance_style),
        state.distance_style_mixed,
    };
}

ClipCanvasHistoryState toCanvasHistoryState(const ClipHistoryState& state) {
    return ClipCanvasHistoryState{
        fromFlag(state.can_undo),
        fromFlag(state.can_redo),
    };
}

ClipCanvasSnapConfig toCanvasSnapConfig(const ClipSnapConfig& config) {
    return ClipCanvasSnapConfig{
        fromFlag(config.enabled),
        fromFlag(config.enable_point_snaps),
        fromFlag(config.enable_gap_snaps),
        fromFlag(config.show_guides),
        fromFlag(config.show_gap_size),
        config.distance,
        toQColor(config.line_color),
        config.line_width,
        config.marker_size,
        config.gap_dash_length,
        config.gap_dash_gap,
    };
}

ClipSnapConfig toEngineSnapConfig(const ClipCanvasSnapConfig& config) {
    ClipSnapConfig engineConfig{};
    engineConfig.enabled = toFlag(config.enabled);
    engineConfig.enable_point_snaps = toFlag(config.enablePointSnaps);
    engineConfig.enable_gap_snaps = toFlag(config.enableGapSnaps);
    engineConfig.show_guides = toFlag(config.showGuides);
    engineConfig.show_gap_size = toFlag(config.showGapSize);
    engineConfig.distance = config.distance;
    engineConfig.line_color = toEngineColor(config.lineColor);
    engineConfig.line_width = config.lineWidth;
    engineConfig.marker_size = config.markerSize;
    engineConfig.gap_dash_length = config.gapDashLength;
    engineConfig.gap_dash_gap = config.gapDashGap;
    return engineConfig;
}

ClipCanvasGridConfig toCanvasGridConfig(const ClipGridConfig& config) {
    return ClipCanvasGridConfig{
        fromFlag(config.enabled),
        config.size,
    };
}

ClipGridConfig toEngineGridConfig(const ClipCanvasGridConfig& config) {
    ClipGridConfig engineConfig{};
    engineConfig.enabled = toFlag(config.enabled);
    engineConfig.size = config.size;
    return engineConfig;
}

ClipCanvasSpotlightConfig toCanvasSpotlightConfig(const ClipSpotlightConfig& config) {
    return ClipCanvasSpotlightConfig{toQColor(config.color), config.opacity};
}

ClipSpotlightConfig toEngineSpotlightConfig(const ClipCanvasSpotlightConfig& config) {
    return ClipSpotlightConfig{toEngineColor(config.color), config.opacity};
}

ClipCanvasWatermarkConfig toCanvasWatermarkConfig(const ClipWatermarkConfig& config) {
    ClipCanvasWatermarkConfig canvasConfig;
    canvasConfig.color = toQColor(config.color);
    canvasConfig.text = clip_canvas_utf8::stringFromField(config.text_utf8, config.text_utf8_len,
                                                          sizeof(config.text_utf8));
    canvasConfig.templateValue = clip_canvas_utf8::stringFromField(
        config.template_value_utf8, config.template_value_utf8_len,
        sizeof(config.template_value_utf8));
    if (config.has_template_application_time != 0) {
        canvasConfig.templateApplicationTime = ClipCanvasWatermarkTemplateApplicationTime{
            config.template_application_time.year,   config.template_application_time.month,
            config.template_application_time.day,    config.template_application_time.hour,
            config.template_application_time.minute, config.template_application_time.second,
        };
    }
    canvasConfig.fontSize = config.font_size;
    canvasConfig.fontFamily = clip_canvas_utf8::stringFromField(
        config.font_family_utf8, config.font_family_utf8_len, sizeof(config.font_family_utf8));
    canvasConfig.angle = config.angle;
    canvasConfig.gap = config.gap;
    canvasConfig.opacity = config.opacity;
    return canvasConfig;
}

ClipWatermarkConfig toEngineWatermarkConfig(const ClipCanvasWatermarkConfig& config) {
    ClipWatermarkConfig engineConfig{};
    engineConfig.color = toEngineColor(config.color);
    std::uint8_t truncated = 0;
    clip_canvas_utf8::copyStringToField(config.text.trimmed(), engineConfig.text_utf8,
                                        engineConfig.text_utf8_len, truncated,
                                        sizeof(engineConfig.text_utf8));
    clip_canvas_utf8::copyStringToField(config.templateValue, engineConfig.template_value_utf8,
                                        engineConfig.template_value_utf8_len, truncated,
                                        sizeof(engineConfig.template_value_utf8));
    engineConfig.font_size = config.fontSize;
    copyFontFamilyToEngine(config.fontFamily, engineConfig.font_family_utf8,
                           engineConfig.font_family_utf8_len, truncated,
                           sizeof(engineConfig.font_family_utf8));
    engineConfig.angle = config.angle;
    engineConfig.gap = config.gap;
    engineConfig.opacity = config.opacity;
    if (config.templateApplicationTime.has_value()) {
        const ClipCanvasWatermarkTemplateApplicationTime& time = *config.templateApplicationTime;
        engineConfig.has_template_application_time = 1;
        engineConfig.template_application_time = ClipWatermarkTemplateApplicationTime{
            time.year,
            static_cast<std::uint8_t>(time.month),
            static_cast<std::uint8_t>(time.day),
            static_cast<std::uint8_t>(time.hour),
            static_cast<std::uint8_t>(time.minute),
            static_cast<std::uint8_t>(time.second),
            {0, 0, 0},
        };
    }
    return engineConfig;
}

bool toEngineStyleDefaults(const ClipCanvasStyleDefaults& defaults,
                           ClipStyleDefaults& engineDefaults) {
    const ClipCanvasShapeStyle* shapes[] = {
        &defaults.rectangle,          &defaults.arrow,        &defaults.line, &defaults.freeDraw,
        &defaults.rectangleHighlight, &defaults.penHighlight,
    };
    if (std::any_of(
            std::begin(shapes), std::end(shapes),
            [](const ClipCanvasShapeStyle* style) { return !validShapeStyleEnums(*style); }) ||
        !enumInRange(defaults.spotlightShape, ClipCanvasRectangleShape::Rectangle,
                     ClipCanvasRectangleShape::Diamond) ||
        !std::isfinite(defaults.brushEraser.strokeWidth) ||
        defaults.brushEraser.strokeWidth < 1.0 || defaults.brushEraser.strokeWidth > 72.0 ||
        !enumInRange(defaults.rectangleFilter.type, ClipCanvasFilterType::Mosaic,
                     ClipCanvasFilterType::Brightness) ||
        !enumInRange(defaults.penFilter.type, ClipCanvasFilterType::Mosaic,
                     ClipCanvasFilterType::Brightness) ||
        !enumInRange(defaults.text.fillStyle, ClipCanvasFillStyle::Line,
                     ClipCanvasFillStyle::Solid) ||
        !enumInRange(defaults.text.horizontalAlign, ClipCanvasTextHorizontalAlign::Left,
                     ClipCanvasTextHorizontalAlign::Right) ||
        !enumInRange(defaults.text.verticalAlign, ClipCanvasTextVerticalAlign::Top,
                     ClipCanvasTextVerticalAlign::Bottom) ||
        !enumInRange(defaults.serialNumber.fillStyle, ClipCanvasFillStyle::Line,
                     ClipCanvasFillStyle::Solid) ||
        !enumInRange(defaults.serialNumber.strokeStyle, ClipCanvasStrokeStyle::Solid,
                     ClipCanvasStrokeStyle::Dotted) ||
        !enumInRange(defaults.serialNumber.type, ClipCanvasSerialNumberType::OutlinedCircle,
                     ClipCanvasSerialNumberType::Circle)) {
        return false;
    }

    const QColor colors[] = {
        defaults.rectangle.fill,
        defaults.rectangle.stroke,
        defaults.arrow.fill,
        defaults.arrow.stroke,
        defaults.line.fill,
        defaults.line.stroke,
        defaults.freeDraw.fill,
        defaults.freeDraw.stroke,
        defaults.rectangleHighlight.fill,
        defaults.rectangleHighlight.stroke,
        defaults.penHighlight.fill,
        defaults.penHighlight.stroke,
        defaults.text.color,
        defaults.text.fill,
        defaults.text.stroke,
        defaults.serialNumber.color,
        defaults.serialNumber.fill,
        defaults.watermark.color,
        defaults.spotlight.color,
    };
    for (const QColor& color : colors) {
        if (!color.isValid()) {
            return false;
        }
    }

    if (!validDistanceStyle(defaults.distance))
        return false;
    engineDefaults = ClipStyleDefaults{};
    engineDefaults.rectangle = toEngineShapeStyle(defaults.rectangle);
    engineDefaults.arrow = toEngineShapeStyle(defaults.arrow);
    engineDefaults.line = toEngineShapeStyle(defaults.line);
    engineDefaults.free_draw = toEngineShapeStyle(defaults.freeDraw);
    engineDefaults.rectangle_highlight = toEngineShapeStyle(defaults.rectangleHighlight);
    engineDefaults.pen_highlight = toEngineShapeStyle(defaults.penHighlight);
    engineDefaults.rectangle_filter = ClipFilterStyle{
        static_cast<ClipFilterType>(defaults.rectangleFilter.type),
        defaults.rectangleFilter.strength,
        defaults.rectangleFilter.opacity,
        defaults.rectangleFilter.strokeWidth,
    };
    engineDefaults.pen_filter = ClipFilterStyle{
        static_cast<ClipFilterType>(defaults.penFilter.type),
        defaults.penFilter.strength,
        defaults.penFilter.opacity,
        defaults.penFilter.strokeWidth,
    };
    engineDefaults.text = toEngineTextStyle(defaults.text);
    engineDefaults.serial_number = toEngineSerialNumberStyle(defaults.serialNumber);
    engineDefaults.watermark = toEngineWatermarkConfig(defaults.watermark);
    engineDefaults.spotlight = toEngineSpotlightConfig(defaults.spotlight);
    engineDefaults.brush_eraser = ClipBrushEraserStyle{defaults.brushEraser.strokeWidth};
    engineDefaults.spotlight_shape = static_cast<ClipRectangleShape>(defaults.spotlightShape);
    engineDefaults.distance = toEngineDistanceStyle(defaults.distance);
    return true;
}

} // namespace clip_canvas_types
