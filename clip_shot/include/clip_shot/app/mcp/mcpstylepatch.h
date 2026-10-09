#ifndef CLIP_SHOT_APP_MCP_MCPSTYLEPATCH_H
#define CLIP_SHOT_APP_MCP_MCPSTYLEPATCH_H
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QColor>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <type_traits>
#include "clip_draw_engine_qt/clip_canvas_widget.h"
#include "clip_draw_engine_qt/clip_canvas_export_types.h"

namespace clip_shot::app::mcp {
// Capture image geometry once per annotation. Rendering scale and monitor DPI are independent.
inline QSizeF mcpDistancePixelScale(const QList<CanvasExportSource>& sources,
                                    const QRectF& selection) {
    QSizeF result(1.0, 1.0);
    bool found = false;
    for (const auto& source : sources) {
        if (source.image.isNull() || source.canvasRect.isEmpty() ||
            !source.canvasRect.intersects(selection))
            continue;
        const QSizeF scale(source.image.width() / source.canvasRect.width(),
                           source.image.height() / source.canvasRect.height());
        if (!std::isfinite(scale.width()) || !std::isfinite(scale.height()) || scale.width() <= 0 ||
            scale.height() <= 0)
            continue;
        if (!found) {
            result = scale;
            found = true;
        } else {
            result.setWidth(std::max(result.width(), scale.width()));
            result.setHeight(std::max(result.height(), scale.height()));
        }
    }
    return result;
}

inline QJsonObject mcpDistanceAnnotationsWithPixelScale(QJsonObject payload, const QSizeF& scale) {
    QJsonArray operations = payload.value(QStringLiteral("operations")).toArray();
    for (qsizetype index = 0; index < operations.size(); ++index) {
        QJsonObject operation = operations.at(index).toObject();
        if (operation.value(QStringLiteral("type")) != QStringLiteral("distance"))
            continue;
        operation.insert(QStringLiteral("pixel_scale"), QJsonArray{scale.width(), scale.height()});
        operations[index] = operation;
    }
    payload.insert(QStringLiteral("operations"), operations);
    return payload;
}

inline const QHash<QString, ClipCanvasTool>& mcpCanvasTools() {
    static const QHash<QString, ClipCanvasTool> tools{
        {QStringLiteral("select"), ClipCanvasTool::Select},
        {QStringLiteral("rectangle"), ClipCanvasTool::Shape},
        {QStringLiteral("arrow"), ClipCanvasTool::Arrow},
        {QStringLiteral("distance"), ClipCanvasTool::Distance},
        {QStringLiteral("line"), ClipCanvasTool::Line},
        {QStringLiteral("freehand"), ClipCanvasTool::FreeDraw},
        {QStringLiteral("rectangle_highlight"), ClipCanvasTool::RectangleHighlight},
        {QStringLiteral("pen_highlight"), ClipCanvasTool::PenHighlight},
        {QStringLiteral("eraser"), ClipCanvasTool::Eraser},
        {QStringLiteral("rectangle_eraser"), ClipCanvasTool::RectangleEraser},
        {QStringLiteral("brush_eraser"), ClipCanvasTool::BrushEraser},
        {QStringLiteral("rectangle_filter"), ClipCanvasTool::RectangleFilter},
        {QStringLiteral("pen_filter"), ClipCanvasTool::PenFilter},
        {QStringLiteral("text"), ClipCanvasTool::Text},
        {QStringLiteral("serial_number"), ClipCanvasTool::SerialNumber},
        {QStringLiteral("watermark"), ClipCanvasTool::Watermark},
        {QStringLiteral("spotlight"), ClipCanvasTool::Spotlight},
        {QStringLiteral("auto_filter"), ClipCanvasTool::AutoFilter}};
    return tools;
}
template <class Commands, class Canvas>
inline bool mcpStylePatch(Commands& commands, Canvas& canvas, const QJsonObject& params) {
    const auto target = params.value(QStringLiteral("target")).toString();
    const auto patch = params.value(QStringLiteral("style")).toObject();
    if (patch.isEmpty())
        return false;
    const QHash<QString, QStringList> enums{
        {QStringLiteral("unit"),
         {QStringLiteral("px"), QStringLiteral("cm"), QStringLiteral("m"), QStringLiteral("km"),
          QStringLiteral("mm")}},
        {QStringLiteral("shape"),
         {QStringLiteral("rectangle"), QStringLiteral("ellipse"), QStringLiteral("diamond")}},
        {QStringLiteral("fill_style"),
         {QStringLiteral("line"), QStringLiteral("cross_line"), QStringLiteral("solid")}},
        {QStringLiteral("stroke_style"),
         {QStringLiteral("solid"), QStringLiteral("dashed"), QStringLiteral("dotted")}},
        {QStringLiteral("arrow_type"),
         {QStringLiteral("straight"), QStringLiteral("curve"), QStringLiteral("elbow")}},
        {QStringLiteral("arrow_shaft_type"), {QStringLiteral("plain"), QStringLiteral("tapered")}},
        {QStringLiteral("horizontal_align"),
         {QStringLiteral("left"), QStringLiteral("center"), QStringLiteral("right")}},
        {QStringLiteral("vertical_align"),
         {QStringLiteral("top"), QStringLiteral("center"), QStringLiteral("bottom")}},
        {QStringLiteral("numeric_type"),
         {QStringLiteral("arabic"), QStringLiteral("roman"), QStringLiteral("lowercase_letters"),
          QStringLiteral("uppercase_letters"), QStringLiteral("chinese")}},
        {QStringLiteral("serial_type"),
         {QStringLiteral("outlined_circle"), QStringLiteral("solid_circle"),
          QStringLiteral("outlined_square"), QStringLiteral("solid_square"),
          QStringLiteral("circle")}},
        {QStringLiteral("filter"),
         {QStringLiteral("mosaic"), QStringLiteral("gaussian_blur"), QStringLiteral("grayscale"),
          QStringLiteral("inversion"), QStringLiteral("emboss"), QStringLiteral("smart_erase"),
          QStringLiteral("brightness")}},
        {QStringLiteral("start_arrowhead"),
         {QStringLiteral("none"), QStringLiteral("arrow"), QStringLiteral("bar"),
          QStringLiteral("dot"), QStringLiteral("circle"), QStringLiteral("circle_outline"),
          QStringLiteral("triangle"), QStringLiteral("triangle_outline"), QStringLiteral("diamond"),
          QStringLiteral("diamond_outline"), QStringLiteral("crowfoot_one"),
          QStringLiteral("crowfoot_many"), QStringLiteral("crowfoot_one_or_many"),
          QStringLiteral("indented_triangle")}}};
    const QStringList colors{QStringLiteral("stroke"), QStringLiteral("fill"),
                             QStringLiteral("color")};
    const QStringList textFields{QStringLiteral("text"), QStringLiteral("font_family")};
    const QStringList numbers{QStringLiteral("stroke_width"),
                              QStringLiteral("font_size"),
                              QStringLiteral("opacity"),
                              QStringLiteral("corner_radius"),
                              QStringLiteral("strength"),
                              QStringLiteral("angle"),
                              QStringLiteral("gap"),
                              QStringLiteral("arrow_ratio"),
                              QStringLiteral("number"),
                              QStringLiteral("factor"),
                              QStringLiteral("decimal_places"),
                              QStringLiteral("endpoint_scale")};
    QStringList allowed;
    if (target == QStringLiteral("text"))
        allowed = {QStringLiteral("color"),
                   QStringLiteral("font_size"),
                   QStringLiteral("font_family"),
                   QStringLiteral("opacity"),
                   QStringLiteral("fill"),
                   QStringLiteral("stroke"),
                   QStringLiteral("stroke_width"),
                   QStringLiteral("fill_style"),
                   QStringLiteral("corner_radius"),
                   QStringLiteral("corner_radii"),
                   QStringLiteral("horizontal_align"),
                   QStringLiteral("vertical_align")};
    else if (target == QStringLiteral("serial_number"))
        allowed = {QStringLiteral("color"),       QStringLiteral("font_size"),
                   QStringLiteral("font_family"), QStringLiteral("opacity"),
                   QStringLiteral("fill"),        QStringLiteral("stroke_width"),
                   QStringLiteral("fill_style"),  QStringLiteral("stroke_style"),
                   QStringLiteral("serial_type"), QStringLiteral("numeric_type"),
                   QStringLiteral("number")};
    else if (target == QStringLiteral("watermark"))
        allowed = {QStringLiteral("color"),       QStringLiteral("font_size"),
                   QStringLiteral("font_family"), QStringLiteral("opacity"),
                   QStringLiteral("text"),        QStringLiteral("angle"),
                   QStringLiteral("gap")};
    else if (target == QStringLiteral("spotlight"))
        allowed = {QStringLiteral("color"), QStringLiteral("opacity")};
    else if (target == QStringLiteral("brush_eraser"))
        allowed = {QStringLiteral("stroke_width")};
    else if (target == QStringLiteral("rectangle_filter") || target == QStringLiteral("pen_filter"))
        allowed = {QStringLiteral("filter"), QStringLiteral("strength"), QStringLiteral("opacity"),
                   QStringLiteral("stroke_width")};
    else if (target == QStringLiteral("rectangle"))
        allowed = {QStringLiteral("fill"),          QStringLiteral("stroke"),
                   QStringLiteral("stroke_width"),  QStringLiteral("fill_style"),
                   QStringLiteral("stroke_style"),  QStringLiteral("shape"),
                   QStringLiteral("corner_radius"), QStringLiteral("corner_radii")};
    else if (target == QStringLiteral("distance"))
        allowed = {QStringLiteral("stroke"),         QStringLiteral("stroke_width"),
                   QStringLiteral("factor"),         QStringLiteral("unit"),
                   QStringLiteral("decimal_places"), QStringLiteral("endpoint_scale"),
                   QStringLiteral("endpoint_style")};
    else if (target == QStringLiteral("arrow"))
        allowed = {QStringLiteral("stroke"),           QStringLiteral("stroke_width"),
                   QStringLiteral("stroke_style"),     QStringLiteral("start_arrowhead"),
                   QStringLiteral("end_arrowhead"),    QStringLiteral("arrow_type"),
                   QStringLiteral("arrow_shaft_type"), QStringLiteral("arrow_ratio")};
    else if (target == QStringLiteral("line") || target == QStringLiteral("freehand")) {
        allowed = {QStringLiteral("fill"),         QStringLiteral("stroke"),
                   QStringLiteral("stroke_width"), QStringLiteral("fill_style"),
                   QStringLiteral("stroke_style"), QStringLiteral("opacity")};
        if (target == QStringLiteral("line"))
            allowed.append(QStringLiteral("arrow_type"));
    } else if (target == QStringLiteral("rectangle_highlight"))
        allowed = {QStringLiteral("fill"), QStringLiteral("stroke"),
                   QStringLiteral("stroke_width")};
    else if (target == QStringLiteral("pen_highlight"))
        allowed = {QStringLiteral("stroke"), QStringLiteral("stroke_width")};
    else
        return false;
    if (patch.contains(QStringLiteral("corner_radius")) &&
        patch.contains(QStringLiteral("corner_radii")))
        return false;
    for (auto it = patch.begin(); it != patch.end(); ++it) {
        if (!allowed.contains(it.key()))
            return false;
        if (colors.contains(it.key())) {
            const auto array = it->toArray();
            if (array.size() != 4)
                return false;
            for (auto v : array)
                if (!v.isDouble() || v.toDouble() < 0 || v.toDouble() > 255 ||
                    std::floor(v.toDouble()) != v.toDouble())
                    return false;
        } else if (it.key() == QStringLiteral("corner_radii")) {
            const auto radii = it->toArray();
            if (radii.size() != 4)
                return false;
            for (const auto radius : radii)
                if (!radius.isDouble() || !std::isfinite(radius.toDouble()) ||
                    radius.toDouble() < 0 || radius.toDouble() > 8192)
                    return false;
        } else if (textFields.contains(it.key())) {
            if (!it->isString() || it->toString().toUtf8().size() > 65536)
                return false;
        } else if (numbers.contains(it.key())) {
            const auto value = it->toDouble(-1);
            const double maximum =
                it.key() == QStringLiteral("opacity") || it.key() == QStringLiteral("strength") ? 1
                : it.key() == QStringLiteral("number") ? 9007199254740991.0
                                                       : 8192;
            if (!it->isDouble() || !std::isfinite(value) || value > maximum ||
                value < (it.key() == QStringLiteral("angle") ? -360 : 0))
                return false;
            if (it.key() == QStringLiteral("number") && std::floor(value) != value)
                return false;
            if ((it.key() == QStringLiteral("arrow_ratio") ||
                 it.key() == QStringLiteral("endpoint_scale")) &&
                (value < 0.5 || value > 3))
                return false;
            if (it.key() == QStringLiteral("factor") && (value < 0.01 || value > 1000))
                return false;
            if (it.key() == QStringLiteral("decimal_places") &&
                (value > 3 || std::floor(value) != value))
                return false;
            if (target == QStringLiteral("distance") &&
                it.key() == QStringLiteral("stroke_width") && (value < 1 || value > 72))
                return false;
        } else {
            const auto key = (it.key() == QStringLiteral("end_arrowhead") ||
                              it.key() == QStringLiteral("endpoint_style"))
                                 ? QStringLiteral("start_arrowhead")
                                 : it.key();
            if (!enums.value(key).contains(it->toString()))
                return false;
        }
    }
    const auto color = [&](const char* name, QColor fallback) {
        const auto v = patch.value(QLatin1String(name));
        if (v.isUndefined())
            return fallback;
        const auto c = v.toArray();
        return QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt());
    };
    const auto number = [&](const char* name, double fallback) {
        return patch.value(QLatin1String(name)).toDouble(fallback);
    };
    const auto enumeration = [&](const char* name, int fallback) {
        const auto value = patch.value(QLatin1String(name));
        const auto key = (QString::fromLatin1(name) == QStringLiteral("end_arrowhead") ||
                          QString::fromLatin1(name) == QStringLiteral("endpoint_style"))
                             ? QStringLiteral("start_arrowhead")
                             : QString::fromLatin1(name);
        return value.isUndefined() ? fallback : enums.value(key).indexOf(value.toString());
    };
    const auto state = canvas.canvasStyleToolbarState();
    const auto dispatch = [](auto&& operation) {
        if constexpr (std::is_void_v<decltype(operation())>) {
            operation();
            return true;
        } else {
            return operation();
        }
    };
    const auto cornerRadii = [&](ClipCanvasCornerRadii fallback) {
        if (patch.contains(QStringLiteral("corner_radius"))) {
            const auto r = number("corner_radius", 0);
            return ClipCanvasCornerRadii{r, r, r, r};
        }
        if (patch.contains(QStringLiteral("corner_radii"))) {
            const auto radii = patch.value(QStringLiteral("corner_radii")).toArray();
            return ClipCanvasCornerRadii{radii[0].toDouble(), radii[1].toDouble(),
                                         radii[2].toDouble(), radii[3].toDouble()};
        }
        return fallback;
    };
    if (target == QStringLiteral("distance")) {
        auto style = state.distanceStyle;
        style.stroke = color("stroke", style.stroke);
        style.strokeWidth = number("stroke_width", style.strokeWidth);
        style.factor = number("factor", style.factor);
        style.unit =
            static_cast<ClipCanvasDistanceUnit>(enumeration("unit", static_cast<int>(style.unit)));
        style.decimalPlaces = static_cast<int>(number("decimal_places", style.decimalPlaces));
        style.endpointScale = number("endpoint_scale", style.endpointScale);
        style.endpointStyle = static_cast<ClipCanvasArrowhead>(
            enumeration("endpoint_style", static_cast<int>(style.endpointStyle)));
        const QHash<QString, quint32> properties{
            {QStringLiteral("stroke"), ClipCanvasDistanceStylePropertyStrokeColor},
            {QStringLiteral("stroke_width"), ClipCanvasDistanceStylePropertyStrokeWidth},
            {QStringLiteral("factor"), ClipCanvasDistanceStylePropertyFactor},
            {QStringLiteral("unit"), ClipCanvasDistanceStylePropertyUnit},
            {QStringLiteral("decimal_places"), ClipCanvasDistanceStylePropertyDecimalPlaces},
            {QStringLiteral("endpoint_scale"), ClipCanvasDistanceStylePropertyEndpointScale},
            {QStringLiteral("endpoint_style"), ClipCanvasDistanceStylePropertyEndpointStyle}};
        quint32 flags = 0;
        for (auto it = patch.begin(); it != patch.end(); ++it)
            flags |= properties.value(it.key());
        if constexpr (requires { canvas.commitStyleEdit(ClipCanvasDistanceEdit{style, flags}); })
            return canvas.commitStyleEdit(ClipCanvasDistanceEdit{style, flags});
        else
            return canvas.setDistanceStyleFromToolbar(style, flags);
    } else if (target == QStringLiteral("brush_eraser")) {
        ClipCanvasBrushEraserStyle style{
            number("stroke_width", state.brushEraserStyle.strokeWidth)};
        if (style.strokeWidth < 1.0 || style.strokeWidth > 72.0)
            return false;
        if constexpr (requires { canvas.setCanvasBrushEraserCreationStyle(style); })
            return canvas.setCanvasBrushEraserCreationStyle(style);
        else
            return canvas.setBrushEraserCreationStyle(style);
    } else if (target == QStringLiteral("watermark")) {
        auto style = canvas.canvasWatermarkConfig();
        style.color = color("color", style.color);
        style.fontSize = number("font_size", style.fontSize);
        style.opacity = number("opacity", style.opacity);
        style.angle = number("angle", style.angle);
        style.gap = number("gap", style.gap);
        if (patch.contains(QStringLiteral("text"))) {
            style.text = patch.value(QStringLiteral("text")).toString();
            style.templateValue = style.text;
        }
        if (patch.contains(QStringLiteral("font_family")))
            style.fontFamily = patch.value(QStringLiteral("font_family")).toString();
        return dispatch([&] { return commands.setWatermarkConfigFromToolbar(style); });
    } else if (target == QStringLiteral("spotlight")) {
        auto style = canvas.canvasSpotlightConfig();
        style.color = color("color", style.color);
        style.opacity = number("opacity", style.opacity);
        return dispatch([&] { return commands.setSpotlightConfigFromToolbar(style); });
    } else if (target == QStringLiteral("text")) {
        auto style = state.textStyle;
        style.color = color("color", style.color);
        style.fill = color("fill", style.fill);
        style.stroke = color("stroke", style.stroke);
        style.fontSize = number("font_size", style.fontSize);
        style.strokeWidth = number("stroke_width", style.strokeWidth);
        style.opacity = number("opacity", style.opacity);
        if (patch.contains(QStringLiteral("font_family")))
            style.fontFamily = patch.value(QStringLiteral("font_family")).toString();
        style.cornerRadii = cornerRadii(style.cornerRadii);
        style.horizontalAlign = static_cast<ClipCanvasTextHorizontalAlign>(
            enumeration("horizontal_align", static_cast<int>(style.horizontalAlign)));
        style.verticalAlign = static_cast<ClipCanvasTextVerticalAlign>(
            enumeration("vertical_align", static_cast<int>(style.verticalAlign)));
        style.fillStyle = static_cast<ClipCanvasFillStyle>(
            enumeration("fill_style", static_cast<int>(style.fillStyle)));
        const QHash<QString, quint32> properties{
            {QStringLiteral("color"), ClipCanvasTextStyleMixedColor},
            {QStringLiteral("font_size"), ClipCanvasTextStyleMixedFontSize},
            {QStringLiteral("font_family"), ClipCanvasTextStyleMixedFontFamily},
            {QStringLiteral("fill"), ClipCanvasTextStyleMixedFill},
            {QStringLiteral("fill_style"), ClipCanvasTextStyleMixedFillStyle},
            {QStringLiteral("stroke"), ClipCanvasTextStyleMixedStroke},
            {QStringLiteral("stroke_width"), ClipCanvasTextStyleMixedStrokeWidth},
            {QStringLiteral("corner_radius"), ClipCanvasTextStyleMixedCornerRadii},
            {QStringLiteral("corner_radii"), ClipCanvasTextStyleMixedCornerRadii},
            {QStringLiteral("horizontal_align"), ClipCanvasTextStyleMixedHorizontalAlign},
            {QStringLiteral("vertical_align"), ClipCanvasTextStyleMixedVerticalAlign},
            {QStringLiteral("opacity"), ClipCanvasTextStyleMixedOpacity}};
        quint32 flags = 0;
        for (auto it = patch.begin(); it != patch.end(); ++it)
            flags |= properties.value(it.key());
        return dispatch([&] { return commands.setTextStyleFromToolbar(style, flags); });
    } else if (target == QStringLiteral("serial_number")) {
        auto style = state.serialNumberStyle;
        if (patch.contains(QStringLiteral("number")))
            style.number = patch.value(QStringLiteral("number")).toInteger();
        style.numericType = static_cast<ClipCanvasSerialNumberNumericType>(
            enumeration("numeric_type", static_cast<int>(style.numericType)));
        style.type = static_cast<ClipCanvasSerialNumberType>(
            enumeration("serial_type", static_cast<int>(style.type)));
        style.color = color("color", style.color);
        style.fill = color("fill", style.fill);
        style.fontSize = number("font_size", style.fontSize);
        style.strokeWidth = number("stroke_width", style.strokeWidth);
        style.opacity = number("opacity", style.opacity);
        if (patch.contains(QStringLiteral("font_family")))
            style.fontFamily = patch.value(QStringLiteral("font_family")).toString();
        style.fillStyle = static_cast<ClipCanvasFillStyle>(
            enumeration("fill_style", static_cast<int>(style.fillStyle)));
        style.strokeStyle = static_cast<ClipCanvasStrokeStyle>(
            enumeration("stroke_style", static_cast<int>(style.strokeStyle)));
        const QHash<QString, quint32> properties{
            {QStringLiteral("number"), ClipCanvasSerialNumberStyleMixedNumber},
            {QStringLiteral("serial_type"), ClipCanvasSerialNumberStyleMixedType},
            {QStringLiteral("numeric_type"), ClipCanvasSerialNumberStyleMixedNumericType},
            {QStringLiteral("color"), ClipCanvasSerialNumberStyleMixedColor},
            {QStringLiteral("fill"), ClipCanvasSerialNumberStyleMixedFill},
            {QStringLiteral("fill_style"), ClipCanvasSerialNumberStyleMixedFillStyle},
            {QStringLiteral("font_size"), ClipCanvasSerialNumberStyleMixedFontSize},
            {QStringLiteral("font_family"), ClipCanvasSerialNumberStyleMixedFontFamily},
            {QStringLiteral("stroke_width"), ClipCanvasSerialNumberStyleMixedStrokeWidth},
            {QStringLiteral("stroke_style"), ClipCanvasSerialNumberStyleMixedStrokeStyle},
            {QStringLiteral("opacity"), ClipCanvasSerialNumberStyleMixedOpacity}};
        quint32 flags = 0;
        for (auto it = patch.begin(); it != patch.end(); ++it)
            flags |= properties.value(it.key());
        if constexpr (requires { canvas.applyStyleEdit(ClipCanvasSerialNumberEdit{style, flags}); })
            return canvas.applyStyleEdit(ClipCanvasSerialNumberEdit{style, flags});
        else
            return dispatch([&] { return commands.setSerialNumberStyleFromToolbar(style, flags); });
    } else if (target.endsWith(QStringLiteral("filter"))) {
        auto style = state.filterStyle;
        style.type =
            static_cast<ClipCanvasFilterType>(enumeration("filter", static_cast<int>(style.type)));
        style.strength = number("strength", style.strength);
        style.opacity = number("opacity", style.opacity);
        style.strokeWidth = number("stroke_width", style.strokeWidth);
        quint32 flags = 0;
        if (patch.contains(QStringLiteral("filter")))
            flags |= ClipCanvasFilterStylePropertyType;
        if (patch.contains(QStringLiteral("strength")))
            flags |= ClipCanvasFilterStylePropertyStrength;
        if (patch.contains(QStringLiteral("opacity")))
            flags |= ClipCanvasFilterStylePropertyOpacity;
        if (patch.contains(QStringLiteral("stroke_width")))
            flags |= ClipCanvasFilterStylePropertyStrokeWidth;
        return dispatch([&] { return commands.setFilterStyleFromToolbar(style, flags); });
    } else {
        auto style = state.shapeStyle;
        style.fill = color("fill", style.fill);
        style.stroke = color("stroke", style.stroke);
        style.strokeWidth = number("stroke_width", style.strokeWidth);
        style.opacity = number("opacity", style.opacity);
        style.shape = static_cast<ClipCanvasRectangleShape>(
            enumeration("shape", static_cast<int>(style.shape)));
        if (target == QStringLiteral("rectangle_highlight")) {
            if (style.shape == ClipCanvasRectangleShape::Diamond)
                return false;
            style.highlightShape = style.shape == ClipCanvasRectangleShape::Ellipse
                                       ? ClipCanvasHighlightShape::Ellipse
                                       : ClipCanvasHighlightShape::Rectangle;
        }
        style.fillStyle = static_cast<ClipCanvasFillStyle>(
            enumeration("fill_style", static_cast<int>(style.fillStyle)));
        style.strokeStyle = static_cast<ClipCanvasStrokeStyle>(
            enumeration("stroke_style", static_cast<int>(style.strokeStyle)));
        style.startArrowhead = static_cast<ClipCanvasArrowhead>(
            enumeration("start_arrowhead", static_cast<int>(style.startArrowhead)));
        style.endArrowhead = static_cast<ClipCanvasArrowhead>(
            enumeration("end_arrowhead", static_cast<int>(style.endArrowhead)));
        style.arrowType = static_cast<ClipCanvasArrowType>(
            enumeration("arrow_type", static_cast<int>(style.arrowType)));
        style.arrowShaftType = static_cast<ClipCanvasArrowShaftType>(
            enumeration("arrow_shaft_type", static_cast<int>(style.arrowShaftType)));
        style.arrowRatio = number("arrow_ratio", style.arrowRatio);
        style.cornerRadii = cornerRadii(style.cornerRadii);
        const QHash<QString, quint32> properties{
            {QStringLiteral("fill"), ClipCanvasShapeStylePropertyFillColor},
            {QStringLiteral("stroke"), ClipCanvasShapeStylePropertyStrokeColor},
            {QStringLiteral("stroke_width"), ClipCanvasShapeStylePropertyStrokeWidth},
            {QStringLiteral("fill_style"), ClipCanvasShapeStylePropertyFillStyle},
            {QStringLiteral("stroke_style"), ClipCanvasShapeStylePropertyStrokeStyle},
            {QStringLiteral("opacity"), ClipCanvasShapeStylePropertyOpacity},
            {QStringLiteral("corner_radius"), ClipCanvasShapeStylePropertyCornerRadius},
            {QStringLiteral("corner_radii"), ClipCanvasShapeStylePropertyCornerRadius},
            {QStringLiteral("shape"), ClipCanvasShapeStylePropertyShape},
            {QStringLiteral("start_arrowhead"), ClipCanvasShapeStylePropertyStartArrowhead},
            {QStringLiteral("end_arrowhead"), ClipCanvasShapeStylePropertyEndArrowhead},
            {QStringLiteral("arrow_type"), ClipCanvasShapeStylePropertyArrowType},
            {QStringLiteral("arrow_shaft_type"), ClipCanvasShapeStylePropertyArrowShaftType},
            {QStringLiteral("arrow_ratio"), ClipCanvasShapeStylePropertyArrowRatio}};
        quint32 flags = 0;
        for (auto it = patch.begin(); it != patch.end(); ++it)
            flags |= properties.value(it.key());
        const QStringList kinds{QStringLiteral("rectangle"),
                                QStringLiteral("arrow"),
                                QStringLiteral("line"),
                                QStringLiteral("freehand"),
                                QStringLiteral("rectangle_highlight"),
                                QStringLiteral("pen_highlight")};
        return dispatch([&] {
            return commands.setShapeStyleFromToolbar(
                style, flags, static_cast<ClipCanvasShapeKind>(kinds.indexOf(target)));
        });
    }
}
} // namespace clip_shot::app::mcp
#endif
