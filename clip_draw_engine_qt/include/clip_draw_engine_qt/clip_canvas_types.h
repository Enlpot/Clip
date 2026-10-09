#pragma once

#include <QColor>
#include <QRectF>
#include <QString>
#include <QtGlobal>

#include <cstring>
#include <algorithm>
#include <cmath>
#include <optional>
#include <QSet>
#include <QVector>

inline bool clipCanvasExactDoubleEqual(double lhs, double rhs) noexcept {
    return std::memcmp(&lhs, &rhs, sizeof(double)) == 0;
}

// Match the engine contract; controls limit adjustments to 0.5 or greater.
inline double clipCanvasNormalizeArrowRatio(double value) noexcept {
    return std::isfinite(value) && value > 0.0 ? std::min(value, 3.0) : 1.0;
}

enum class ClipCanvasTool {
    Select,
    Shape,
    Arrow,
    Line,
    FreeDraw,
    RectangleHighlight,
    Highlight = RectangleHighlight,
    Eraser,
    RectangleFilter,
    Filter = RectangleFilter,
    Watermark,
    Text,
    SerialNumber,
    PenHighlight,
    PenFilter,
    Spotlight,
    AutoFilter,
    RectangleEraser,
    BrushEraser,
    Distance,
};

enum class ClipCanvasCursorLayer {
    CanvasTool,
    Host,
};

enum class ClipCanvasStyleToolbarSource {
    DefaultRectangle,
    SelectedRectangle,
    DefaultArrow,
    SelectedArrow,
    DefaultLine,
    SelectedLine,
    DefaultFreeDraw,
    SelectedFreeDraw,
    DefaultRectangleHighlight,
    SelectedRectangleHighlight,
    Eraser,
    DefaultRectangleFilter,
    DefaultFilter = DefaultRectangleFilter,
    SelectedRectangleFilter,
    SelectedFilter = SelectedRectangleFilter,
    Watermark,
    DefaultText,
    SelectedText,
    DefaultSerialNumber,
    SelectedSerialNumber,
    DefaultPenHighlight,
    SelectedPenHighlight,
    DefaultPenFilter,
    SelectedPenFilter,
    DefaultSpotlight,
    SelectedSpotlight,
    DefaultRectangleEraser,
    DefaultBrushEraser,
    DefaultDistance,
    SelectedDistance,
};

enum ClipCanvasTextStyleMixedFlag : quint32 {
    ClipCanvasTextStyleMixedColor = 1u << 0,
    ClipCanvasTextStyleMixedFontSize = 1u << 1,
    ClipCanvasTextStyleMixedFontFamily = 1u << 2,
    ClipCanvasTextStyleMixedFill = 1u << 3,
    ClipCanvasTextStyleMixedFillStyle = 1u << 4,
    ClipCanvasTextStyleMixedStroke = 1u << 5,
    ClipCanvasTextStyleMixedStrokeWidth = 1u << 6,
    ClipCanvasTextStyleMixedCornerRadii = 1u << 7,
    ClipCanvasTextStyleMixedHorizontalAlign = 1u << 8,
    ClipCanvasTextStyleMixedVerticalAlign = 1u << 9,
    ClipCanvasTextStyleMixedOpacity = 1u << 10,
};
inline constexpr quint32 ClipCanvasTextStyleAllProperties = (1u << 11) - 1u;

enum ClipCanvasSerialNumberStyleMixedFlag : quint32 {
    ClipCanvasSerialNumberStyleMixedNumber = 1u << 0,
    ClipCanvasSerialNumberStyleMixedColor = 1u << 1,
    ClipCanvasSerialNumberStyleMixedFill = 1u << 2,
    ClipCanvasSerialNumberStyleMixedFillStyle = 1u << 3,
    ClipCanvasSerialNumberStyleMixedFontSize = 1u << 4,
    ClipCanvasSerialNumberStyleMixedFontFamily = 1u << 5,
    ClipCanvasSerialNumberStyleMixedStrokeWidth = 1u << 6,
    ClipCanvasSerialNumberStyleMixedStrokeStyle = 1u << 7,
    ClipCanvasSerialNumberStyleMixedOpacity = 1u << 8,
    ClipCanvasSerialNumberStyleMixedType = 1u << 9,
    ClipCanvasSerialNumberStyleMixedNumericType = 1u << 10,
};

enum class ClipCanvasSerialNumberNumericType : quint32 {
    Arabic = 0,
    Roman = 1,
    LowercaseLetters = 2,
    UppercaseLetters = 3,
    Chinese = 4,
};

enum class ClipCanvasSerialNumberType : quint32 {
    OutlinedCircle = 0,
    SolidCircle = 1,
    OutlinedSquare = 2,
    SolidSquare = 3,
    Circle = 4,
};

enum ClipCanvasShapeStyleMixedFlag : quint32 {
    ClipCanvasShapeStyleMixedFill = 1u << 0,
    ClipCanvasShapeStyleMixedFillStyle = 1u << 1,
    ClipCanvasShapeStyleMixedStroke = 1u << 2,
    ClipCanvasShapeStyleMixedStrokeWidth = 1u << 3,
    ClipCanvasShapeStyleMixedCornerRadii = 1u << 4,
    ClipCanvasShapeStyleMixedStartArrowhead = 1u << 5,
    ClipCanvasShapeStyleMixedEndArrowhead = 1u << 6,
    ClipCanvasShapeStyleMixedStrokeStyle = 1u << 7,
    ClipCanvasShapeStyleMixedArrowShaftType = 1u << 12,
    ClipCanvasShapeStyleMixedArrowRatio = 1u << 13,
    ClipCanvasShapeStyleMixedArrowType = 1u << 8,
    ClipCanvasShapeStyleMixedOpacity = 1u << 9,
    ClipCanvasShapeStyleMixedHighlightShape = 1u << 10,
    ClipCanvasShapeStyleMixedShape = 1u << 11,
};

enum class ClipCanvasSelectionOrder {
    SendToBack = 0,
    SendBackward = 1,
    BringForward = 2,
    BringToFront = 3,
};

enum class ClipCanvasSelectionAlignment {
    AlignLeft = 0,
    AlignCenterHorizontally = 1,
    AlignRight = 2,
    AlignTop = 3,
    AlignCenterVertically = 4,
    AlignBottom = 5,
    DistributeHorizontally = 6,
    DistributeVertically = 7,
};

enum ClipCanvasShapeStyleProperty : quint32 {
    ClipCanvasShapeStylePropertyFillColor = 1u << 0,
    ClipCanvasShapeStylePropertyFillStyle = 1u << 1,
    ClipCanvasShapeStylePropertyStrokeColor = 1u << 2,
    ClipCanvasShapeStylePropertyStrokeWidth = 1u << 3,
    ClipCanvasShapeStylePropertyCornerRadius = 1u << 4,
    ClipCanvasShapeStylePropertyStartArrowhead = 1u << 5,
    ClipCanvasShapeStylePropertyEndArrowhead = 1u << 6,
    ClipCanvasShapeStylePropertyStrokeStyle = 1u << 7,
    ClipCanvasShapeStylePropertyArrowShaftType = 1u << 12,
    ClipCanvasShapeStylePropertyArrowRatio = 1u << 13,
    ClipCanvasShapeStylePropertyArrowType = 1u << 8,
    ClipCanvasShapeStylePropertyOpacity = 1u << 9,
    ClipCanvasShapeStylePropertyShape = 1u << 11,
};

enum class ClipCanvasShapeKind {
    Rectangle,
    Arrow,
    Line,
    FreeDraw,
    RectangleHighlight,
    Highlight = RectangleHighlight,
    PenHighlight,
    Spotlight,
};

enum class ClipCanvasHighlightShape {
    Rectangle,
    Ellipse,
};

enum class ClipCanvasRectangleShape {
    Rectangle,
    Ellipse,
    Diamond,
};

enum class ClipCanvasFilterType {
    Mosaic,
    GaussianBlur,
    Grayscale,
    Inversion,
    Emboss = 4,
    SmartErase = 5,
    Brightness = 6,
    RestoreBackground = 7,
};

// Capability flag returned alongside filterStyleMixed property bits.
constexpr quint32 ClipCanvasFilterStyleMixedContainsSmartErase = 1u << 31;

enum ClipCanvasFilterStyleProperty : quint32 {
    ClipCanvasFilterStylePropertyType = 1u << 0,
    ClipCanvasFilterStylePropertyStrength = 1u << 1,
    ClipCanvasFilterStylePropertyOpacity = 1u << 2,
    ClipCanvasFilterStylePropertyStrokeWidth = 1u << 3,
};

struct ClipCanvasFilterStyle {
    ClipCanvasFilterType type = ClipCanvasFilterType::Mosaic;
    double strength = 0.5;
    double opacity = 1.0;
    double strokeWidth = 2.0;
};

enum ClipCanvasBrushEraserStyleProperty : quint32 {
    ClipCanvasBrushEraserStylePropertyStrokeWidth = 1u << 0,
};

struct ClipCanvasBrushEraserStyle {
    double strokeWidth = 30.0;
};

inline bool operator==(const ClipCanvasBrushEraserStyle& lhs,
                       const ClipCanvasBrushEraserStyle& rhs) {
    return clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth);
}

inline bool operator!=(const ClipCanvasBrushEraserStyle& lhs,
                       const ClipCanvasBrushEraserStyle& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const ClipCanvasFilterStyle& lhs, const ClipCanvasFilterStyle& rhs) {
    return lhs.type == rhs.type && clipCanvasExactDoubleEqual(lhs.strength, rhs.strength) &&
           clipCanvasExactDoubleEqual(lhs.opacity, rhs.opacity) &&
           clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth);
}

inline bool operator!=(const ClipCanvasFilterStyle& lhs, const ClipCanvasFilterStyle& rhs) {
    return !(lhs == rhs);
}

enum class ClipCanvasArrowhead {
    None,
    Arrow,
    Bar,
    Dot,
    Circle,
    CircleOutline,
    Triangle,
    TriangleOutline,
    Diamond,
    DiamondOutline,
    CrowfootOne,
    CrowfootMany,
    CrowfootOneOrMany,
    IndentedTriangle,
};

enum class ClipCanvasArrowShaftType { Plain, Tapered };

enum class ClipCanvasArrowType {
    Straight,
    Curve,
    Elbow,
};

enum class ClipCanvasTextHorizontalAlign {
    Left,
    Center,
    Right,
};

enum class ClipCanvasTextVerticalAlign {
    Top,
    Center,
    Bottom,
};

enum class ClipCanvasFillStyle {
    Line,
    CrossLine,
    Solid,
};

enum class ClipCanvasStrokeStyle {
    Solid,
    Dashed,
    Dotted,
};

struct ClipCanvasCornerRadii {
    double topLeft = 0.0;
    double topRight = 0.0;
    double bottomRight = 0.0;
    double bottomLeft = 0.0;
};

inline bool operator==(const ClipCanvasCornerRadii& lhs, const ClipCanvasCornerRadii& rhs) {
    return clipCanvasExactDoubleEqual(lhs.topLeft, rhs.topLeft) &&
           clipCanvasExactDoubleEqual(lhs.topRight, rhs.topRight) &&
           clipCanvasExactDoubleEqual(lhs.bottomRight, rhs.bottomRight) &&
           clipCanvasExactDoubleEqual(lhs.bottomLeft, rhs.bottomLeft);
}

inline bool operator!=(const ClipCanvasCornerRadii& lhs, const ClipCanvasCornerRadii& rhs) {
    return !(lhs == rhs);
}

enum class ClipCanvasDistanceUnit { Px, Cm, M, Km, Mm };

enum ClipCanvasDistanceStyleProperty : quint32 {
    ClipCanvasDistanceStylePropertyStrokeColor = 1u << 0,
    ClipCanvasDistanceStylePropertyStrokeWidth = 1u << 1,
    ClipCanvasDistanceStylePropertyFactor = 1u << 2,
    ClipCanvasDistanceStylePropertyUnit = 1u << 3,
    ClipCanvasDistanceStylePropertyDecimalPlaces = 1u << 4,
    ClipCanvasDistanceStylePropertyEndpointScale = 1u << 5,
    ClipCanvasDistanceStylePropertyEndpointStyle = 1u << 6,
};
inline constexpr quint32 ClipCanvasDistanceStyleAllProperties = (1u << 7) - 1u;
enum ClipCanvasDistanceStyleMixedFlag : quint32 {
    ClipCanvasDistanceStyleMixedStrokeColor = 1u << 0,
    ClipCanvasDistanceStyleMixedStrokeWidth = 1u << 1,
    ClipCanvasDistanceStyleMixedFactor = 1u << 2,
    ClipCanvasDistanceStyleMixedUnit = 1u << 3,
    ClipCanvasDistanceStyleMixedDecimalPlaces = 1u << 4,
    ClipCanvasDistanceStyleMixedEndpointScale = 1u << 5,
    ClipCanvasDistanceStyleMixedEndpointStyle = 1u << 6,
};

struct ClipCanvasDistanceStyle {
    QColor stroke = QColor(0xf5, 0x22, 0x2d);
    double strokeWidth = 2.0;
    double factor = 1.0;
    ClipCanvasDistanceUnit unit = ClipCanvasDistanceUnit::Cm;
    int decimalPlaces = 0;
    double endpointScale = 1.0;
    ClipCanvasArrowhead endpointStyle = ClipCanvasArrowhead::Bar;
};
inline bool operator==(const ClipCanvasDistanceStyle& lhs, const ClipCanvasDistanceStyle& rhs) {
    return lhs.stroke == rhs.stroke &&
           clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth) &&
           clipCanvasExactDoubleEqual(lhs.factor, rhs.factor) && lhs.unit == rhs.unit &&
           lhs.decimalPlaces == rhs.decimalPlaces &&
           clipCanvasExactDoubleEqual(lhs.endpointScale, rhs.endpointScale) &&
           lhs.endpointStyle == rhs.endpointStyle;
}
inline bool operator!=(const ClipCanvasDistanceStyle& lhs, const ClipCanvasDistanceStyle& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasShapeStyle {
    QColor fill;
    ClipCanvasFillStyle fillStyle = ClipCanvasFillStyle::Solid;
    QColor stroke;
    double strokeWidth = 0.0;
    ClipCanvasCornerRadii cornerRadii;
    ClipCanvasArrowhead startArrowhead = ClipCanvasArrowhead::None;
    ClipCanvasArrowhead endArrowhead = ClipCanvasArrowhead::None;
    ClipCanvasStrokeStyle strokeStyle = ClipCanvasStrokeStyle::Solid;
    ClipCanvasArrowType arrowType = ClipCanvasArrowType::Straight;
    ClipCanvasArrowShaftType arrowShaftType = ClipCanvasArrowShaftType::Plain;
    double arrowRatio = 1.0;
    double opacity = 1.0;
    ClipCanvasHighlightShape highlightShape = ClipCanvasHighlightShape::Rectangle;
    ClipCanvasRectangleShape shape = ClipCanvasRectangleShape::Rectangle;
};

inline bool operator==(const ClipCanvasShapeStyle& lhs, const ClipCanvasShapeStyle& rhs) {
    return lhs.fill == rhs.fill && lhs.fillStyle == rhs.fillStyle && lhs.stroke == rhs.stroke &&
           clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth) &&
           lhs.cornerRadii == rhs.cornerRadii && lhs.startArrowhead == rhs.startArrowhead &&
           lhs.endArrowhead == rhs.endArrowhead && lhs.strokeStyle == rhs.strokeStyle &&
           lhs.arrowType == rhs.arrowType &&
           clipCanvasExactDoubleEqual(lhs.arrowRatio, rhs.arrowRatio) &&
           lhs.arrowShaftType == rhs.arrowShaftType &&
           clipCanvasExactDoubleEqual(lhs.opacity, rhs.opacity) &&
           lhs.highlightShape == rhs.highlightShape && lhs.shape == rhs.shape;
}

inline bool operator!=(const ClipCanvasShapeStyle& lhs, const ClipCanvasShapeStyle& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasWatermarkTemplateApplicationTime {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

inline bool operator==(const ClipCanvasWatermarkTemplateApplicationTime& lhs,
                       const ClipCanvasWatermarkTemplateApplicationTime& rhs) {
    return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day &&
           lhs.hour == rhs.hour && lhs.minute == rhs.minute && lhs.second == rhs.second;
}

inline bool operator!=(const ClipCanvasWatermarkTemplateApplicationTime& lhs,
                       const ClipCanvasWatermarkTemplateApplicationTime& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasWatermarkConfig {
    QColor color = QColor(0, 0, 0, 255);
    QString text;
    QString templateValue;
    std::optional<ClipCanvasWatermarkTemplateApplicationTime> templateApplicationTime;
    double fontSize = 16.0;
    QString fontFamily;
    double angle = 30.0;
    double gap = 56.0;
    double opacity = 0.16;
};

struct ClipCanvasSpotlightConfig {
    QColor color = QColor(0, 0, 0, 255);
    double opacity = 0.64;
};

inline bool operator==(const ClipCanvasSpotlightConfig& lhs, const ClipCanvasSpotlightConfig& rhs) {
    return lhs.color == rhs.color && clipCanvasExactDoubleEqual(lhs.opacity, rhs.opacity);
}

inline bool operator!=(const ClipCanvasSpotlightConfig& lhs, const ClipCanvasSpotlightConfig& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const ClipCanvasWatermarkConfig& lhs, const ClipCanvasWatermarkConfig& rhs) {
    return lhs.color == rhs.color && lhs.text == rhs.text &&
           lhs.templateValue == rhs.templateValue &&
           lhs.templateApplicationTime == rhs.templateApplicationTime &&
           clipCanvasExactDoubleEqual(lhs.fontSize, rhs.fontSize) &&
           lhs.fontFamily == rhs.fontFamily && clipCanvasExactDoubleEqual(lhs.angle, rhs.angle) &&
           clipCanvasExactDoubleEqual(lhs.gap, rhs.gap) &&
           clipCanvasExactDoubleEqual(lhs.opacity, rhs.opacity);
}

inline bool operator!=(const ClipCanvasWatermarkConfig& lhs, const ClipCanvasWatermarkConfig& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasRectangleShapeStyle {
    QColor fill;
    ClipCanvasFillStyle fillStyle = ClipCanvasFillStyle::Solid;
    QColor stroke;
    double strokeWidth = 0.0;
    ClipCanvasStrokeStyle strokeStyle = ClipCanvasStrokeStyle::Solid;
    ClipCanvasCornerRadii cornerRadii{6.0, 6.0, 6.0, 6.0};
};

inline bool operator==(const ClipCanvasRectangleShapeStyle& lhs,
                       const ClipCanvasRectangleShapeStyle& rhs) {
    return lhs.fill == rhs.fill && lhs.fillStyle == rhs.fillStyle && lhs.stroke == rhs.stroke &&
           clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth) &&
           lhs.strokeStyle == rhs.strokeStyle && lhs.cornerRadii == rhs.cornerRadii;
}

inline bool operator!=(const ClipCanvasRectangleShapeStyle& lhs,
                       const ClipCanvasRectangleShapeStyle& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasArrowStyle {
    QColor stroke;
    double strokeWidth = 0.0;
    ClipCanvasArrowhead startArrowhead = ClipCanvasArrowhead::None;
    ClipCanvasArrowhead endArrowhead = ClipCanvasArrowhead::None;
    ClipCanvasStrokeStyle strokeStyle = ClipCanvasStrokeStyle::Solid;
    ClipCanvasArrowType arrowType = ClipCanvasArrowType::Straight;
    ClipCanvasArrowShaftType arrowShaftType = ClipCanvasArrowShaftType::Plain;
    double arrowRatio = 1.0;
};

inline bool operator==(const ClipCanvasArrowStyle& lhs, const ClipCanvasArrowStyle& rhs) {
    return lhs.stroke == rhs.stroke &&
           clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth) &&
           lhs.startArrowhead == rhs.startArrowhead && lhs.endArrowhead == rhs.endArrowhead &&
           lhs.strokeStyle == rhs.strokeStyle && lhs.arrowType == rhs.arrowType &&
           clipCanvasExactDoubleEqual(lhs.arrowRatio, rhs.arrowRatio) &&
           lhs.arrowShaftType == rhs.arrowShaftType;
}

inline bool operator!=(const ClipCanvasArrowStyle& lhs, const ClipCanvasArrowStyle& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasTextStyle {
    QColor color{0xf4, 0x21, 0x2c};
    double fontSize = 30.0;
    QString fontFamily;
    QColor fill;
    ClipCanvasFillStyle fillStyle = ClipCanvasFillStyle::Solid;
    QColor stroke{0xff, 0xcc, 0xc7};
    double strokeWidth = 0.0;
    ClipCanvasCornerRadii cornerRadii{6.0, 6.0, 6.0, 6.0};
    ClipCanvasTextHorizontalAlign horizontalAlign = ClipCanvasTextHorizontalAlign::Left;
    ClipCanvasTextVerticalAlign verticalAlign = ClipCanvasTextVerticalAlign::Center;
    double opacity = 1.0;
};

inline bool operator==(const ClipCanvasTextStyle& lhs, const ClipCanvasTextStyle& rhs) {
    return lhs.color == rhs.color && clipCanvasExactDoubleEqual(lhs.fontSize, rhs.fontSize) &&
           lhs.fontFamily == rhs.fontFamily && lhs.fill == rhs.fill &&
           lhs.fillStyle == rhs.fillStyle && lhs.stroke == rhs.stroke &&
           clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth) &&
           lhs.cornerRadii == rhs.cornerRadii && lhs.horizontalAlign == rhs.horizontalAlign &&
           lhs.verticalAlign == rhs.verticalAlign &&
           clipCanvasExactDoubleEqual(lhs.opacity, rhs.opacity);
}

inline bool operator!=(const ClipCanvasTextStyle& lhs, const ClipCanvasTextStyle& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasSerialNumberStyle {
    qint64 number = 1;
    ClipCanvasSerialNumberType type = ClipCanvasSerialNumberType::OutlinedCircle;
    ClipCanvasSerialNumberNumericType numericType = ClipCanvasSerialNumberNumericType::Arabic;
    QColor color{0xf4, 0x21, 0x2c};
    QColor fill;
    ClipCanvasFillStyle fillStyle = ClipCanvasFillStyle::Solid;
    double fontSize = 24.0;
    QString fontFamily;
    double strokeWidth = 2.0;
    ClipCanvasStrokeStyle strokeStyle = ClipCanvasStrokeStyle::Solid;
    double opacity = 1.0;
};

inline bool operator==(const ClipCanvasSerialNumberStyle& lhs,
                       const ClipCanvasSerialNumberStyle& rhs) {
    return lhs.number == rhs.number && lhs.type == rhs.type && lhs.numericType == rhs.numericType &&
           lhs.color == rhs.color && lhs.fill == rhs.fill && lhs.fillStyle == rhs.fillStyle &&
           clipCanvasExactDoubleEqual(lhs.fontSize, rhs.fontSize) &&
           lhs.fontFamily == rhs.fontFamily &&
           clipCanvasExactDoubleEqual(lhs.strokeWidth, rhs.strokeWidth) &&
           lhs.strokeStyle == rhs.strokeStyle &&
           clipCanvasExactDoubleEqual(lhs.opacity, rhs.opacity);
}

inline bool operator!=(const ClipCanvasSerialNumberStyle& lhs,
                       const ClipCanvasSerialNumberStyle& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasStyleDefaults {
    ClipCanvasShapeStyle rectangle;
    ClipCanvasShapeStyle arrow;
    ClipCanvasShapeStyle line;
    ClipCanvasShapeStyle freeDraw;
    ClipCanvasShapeStyle rectangleHighlight;
    ClipCanvasShapeStyle penHighlight;
    ClipCanvasFilterStyle rectangleFilter;
    ClipCanvasFilterStyle penFilter;
    ClipCanvasTextStyle text;
    ClipCanvasSerialNumberStyle serialNumber;
    ClipCanvasWatermarkConfig watermark;
    ClipCanvasSpotlightConfig spotlight;
    ClipCanvasBrushEraserStyle brushEraser;
    ClipCanvasRectangleShape spotlightShape = ClipCanvasRectangleShape::Rectangle;
    ClipCanvasDistanceStyle distance;
};

inline bool operator==(const ClipCanvasStyleDefaults& lhs, const ClipCanvasStyleDefaults& rhs) {
    return lhs.rectangle == rhs.rectangle && lhs.arrow == rhs.arrow && lhs.line == rhs.line &&
           lhs.freeDraw == rhs.freeDraw && lhs.rectangleHighlight == rhs.rectangleHighlight &&
           lhs.penHighlight == rhs.penHighlight && lhs.rectangleFilter == rhs.rectangleFilter &&
           lhs.penFilter == rhs.penFilter && lhs.text == rhs.text &&
           lhs.serialNumber == rhs.serialNumber && lhs.watermark == rhs.watermark &&
           lhs.spotlight == rhs.spotlight && lhs.brushEraser == rhs.brushEraser &&
           lhs.spotlightShape == rhs.spotlightShape && lhs.distance == rhs.distance;
}

inline bool operator!=(const ClipCanvasStyleDefaults& lhs, const ClipCanvasStyleDefaults& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasRuntimeConfig {
    std::optional<ClipCanvasStyleDefaults> styleDefaults;
    QSet<ClipCanvasTool> quickSelectionDisabledTools{};
};

struct ClipCanvasStyleToolbarState {
    ClipCanvasStyleToolbarSource source = ClipCanvasStyleToolbarSource::DefaultRectangle;
    quint32 selectedElementCount = 0;
    ClipCanvasShapeStyle shapeStyle;
    ClipCanvasTextStyle textStyle;
    ClipCanvasSerialNumberStyle serialNumberStyle;
    quint32 textStyleMixed = 0;
    quint32 serialNumberStyleMixed = 0;
    quint32 shapeStyleMixed = 0;
    ClipCanvasFilterStyle filterStyle;
    quint32 filterStyleMixed = 0;
    bool canEditArrowText = false;
    ClipCanvasBrushEraserStyle brushEraserStyle;
    ClipCanvasDistanceStyle distanceStyle;
    quint32 distanceStyleMixed = 0;
};

inline bool operator==(const ClipCanvasStyleToolbarState& lhs,
                       const ClipCanvasStyleToolbarState& rhs) {
    return lhs.canEditArrowText == rhs.canEditArrowText && lhs.source == rhs.source &&
           lhs.selectedElementCount == rhs.selectedElementCount &&
           lhs.shapeStyle == rhs.shapeStyle && lhs.textStyle == rhs.textStyle &&
           lhs.serialNumberStyle == rhs.serialNumberStyle &&
           lhs.textStyleMixed == rhs.textStyleMixed &&
           lhs.serialNumberStyleMixed == rhs.serialNumberStyleMixed &&
           lhs.shapeStyleMixed == rhs.shapeStyleMixed && lhs.filterStyle == rhs.filterStyle &&
           lhs.filterStyleMixed == rhs.filterStyleMixed &&
           lhs.brushEraserStyle == rhs.brushEraserStyle && lhs.distanceStyle == rhs.distanceStyle &&
           lhs.distanceStyleMixed == rhs.distanceStyleMixed;
}

inline bool operator!=(const ClipCanvasStyleToolbarState& lhs,
                       const ClipCanvasStyleToolbarState& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasSerialNumberToolbarState {
    bool visible = false;
    QRectF geometry;
    bool canDecrease = false;
    bool canIncrease = false;
    bool canCreateText = false;
};

inline bool operator==(const ClipCanvasSerialNumberToolbarState& lhs,
                       const ClipCanvasSerialNumberToolbarState& rhs) {
    return lhs.visible == rhs.visible && lhs.geometry == rhs.geometry &&
           lhs.canDecrease == rhs.canDecrease && lhs.canIncrease == rhs.canIncrease &&
           lhs.canCreateText == rhs.canCreateText;
}

inline bool operator!=(const ClipCanvasSerialNumberToolbarState& lhs,
                       const ClipCanvasSerialNumberToolbarState& rhs) {
    return !(lhs == rhs);
}

struct ClipCanvasHistoryState {
    bool canUndo = false;
    bool canRedo = false;
};

struct ClipCanvasSnapConfig {
    bool enabled = false;
    bool enablePointSnaps = true;
    bool enableGapSnaps = true;
    bool showGuides = true;
    bool showGapSize = false;
    double distance = 8.0;
    QColor lineColor = QColor(255, 107, 107, 255);
    double lineWidth = 1.0;
    double markerSize = 8.0;
    double gapDashLength = 4.0;
    double gapDashGap = 4.0;
};

struct ClipCanvasSnapGuideTargets {
    QVector<qreal> verticalXs;
    QVector<qreal> horizontalYs;
};

struct ClipCanvasGridConfig {
    bool enabled = false;
    double size = 20.0;
};

struct ClipCanvasAutoFilterRegion {
    quint64 id = 0;
    QRectF bounds;
    QString category;
};
struct ClipCanvasAutoFilterRecord {
    QRectF sourceBounds;
    QList<ClipCanvasAutoFilterRegion> regions;
};
