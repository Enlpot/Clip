#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTDEFAULTSTYLES_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTDEFAULTSTYLES_H

#include "clip_draw_engine_qt/clip_canvas_types.h"

#include <QColor>
#include <QHash>
#include <QSet>
#include <QStringList>

namespace clip_shot::presentation {
inline ClipCanvasStyleDefaults screenshotCanvasStyleDefaults() {
    const QColor red(0xf5, 0x22, 0x2d, 255);
    const QColor redAccent(0xf4, 0x21, 0x2c, 255);
    const QColor transparent(0xff, 0xff, 0xff, 0);

    ClipCanvasStyleDefaults defaults;
    defaults.rectangle.fill = transparent;
    defaults.rectangle.fillStyle = ClipCanvasFillStyle::Solid;
    defaults.rectangle.stroke = red;
    defaults.rectangle.strokeWidth = 2.0;
    defaults.rectangle.strokeStyle = ClipCanvasStrokeStyle::Solid;
    defaults.rectangle.cornerRadii = {6.0, 6.0, 6.0, 6.0};

    defaults.arrow = defaults.rectangle;
    defaults.arrow.startArrowhead = ClipCanvasArrowhead::None;
    defaults.arrow.endArrowhead = ClipCanvasArrowhead::Arrow;
    defaults.arrow.arrowType = ClipCanvasArrowType::Curve;

    defaults.distance.stroke = red;
    defaults.distance.strokeWidth = 2.0;

    defaults.line.fill = transparent;
    defaults.line.fillStyle = ClipCanvasFillStyle::Solid;
    defaults.line.stroke = red;
    defaults.line.strokeWidth = 2.0;
    defaults.line.strokeStyle = ClipCanvasStrokeStyle::Solid;
    defaults.line.startArrowhead = ClipCanvasArrowhead::None;
    defaults.line.endArrowhead = ClipCanvasArrowhead::None;
    defaults.line.opacity = 1.0;

    defaults.freeDraw = defaults.line;
    defaults.freeDraw.stroke = red;
    defaults.line.arrowType = ClipCanvasArrowType::Curve;

    defaults.rectangleHighlight.fill = red;
    defaults.rectangleHighlight.fillStyle = ClipCanvasFillStyle::Solid;
    defaults.rectangleHighlight.stroke = redAccent;
    defaults.rectangleHighlight.strokeWidth = 0.0;
    defaults.rectangleHighlight.strokeStyle = ClipCanvasStrokeStyle::Solid;
    defaults.rectangleHighlight.opacity = 1.0;
    defaults.rectangleHighlight.highlightShape = ClipCanvasHighlightShape::Rectangle;

    defaults.penHighlight.fill = transparent;
    defaults.penHighlight.fillStyle = ClipCanvasFillStyle::Solid;
    defaults.penHighlight.stroke = red;
    defaults.penHighlight.strokeWidth = 30.0;
    defaults.penHighlight.strokeStyle = ClipCanvasStrokeStyle::Solid;
    defaults.penHighlight.opacity = 1.0;

    defaults.rectangleFilter = {
        ClipCanvasFilterType::Mosaic,
        0.5,
        1.0,
        2.0,
    };
    defaults.penFilter = defaults.rectangleFilter;
    defaults.penFilter.strokeWidth = 30.0;
    defaults.brushEraser.strokeWidth = 30.0;

    defaults.text.color = red;
    defaults.text.fontSize = 30.0;
    defaults.text.fontFamily.clear();
    defaults.text.fill = transparent;
    defaults.text.fillStyle = ClipCanvasFillStyle::Solid;
    defaults.text.stroke = QColor(0xff, 0xcc, 0xc7, 255);
    defaults.text.strokeWidth = 0.0;
    defaults.text.cornerRadii = {6.0, 6.0, 6.0, 6.0};
    defaults.text.horizontalAlign = ClipCanvasTextHorizontalAlign::Left;
    defaults.text.verticalAlign = ClipCanvasTextVerticalAlign::Center;
    defaults.text.opacity = 1.0;

    defaults.serialNumber.number = 1;
    defaults.serialNumber.type = ClipCanvasSerialNumberType::OutlinedCircle;
    defaults.serialNumber.color = red;
    defaults.serialNumber.fill = transparent;
    defaults.serialNumber.fillStyle = ClipCanvasFillStyle::Solid;
    defaults.serialNumber.fontSize = 24.0;
    defaults.serialNumber.fontFamily.clear();
    defaults.serialNumber.strokeWidth = 2.0;
    defaults.serialNumber.strokeStyle = ClipCanvasStrokeStyle::Solid;
    defaults.serialNumber.opacity = 1.0;

    defaults.watermark.color = QColor(0, 0, 0, 255);
    defaults.watermark.text.clear();
    defaults.watermark.fontSize = 16.0;
    defaults.watermark.fontFamily.clear();
    defaults.watermark.angle = 30.0;
    defaults.watermark.gap = 56.0;
    defaults.watermark.opacity = 0.16;

    defaults.spotlight.color = QColor(0, 0, 0, 255);
    defaults.spotlight.opacity = 0.64;
    return defaults;
}

inline QSet<ClipCanvasTool> screenshotQuickSelectionDisabledTools(const QStringList& toolIds) {
    QSet<ClipCanvasTool> tools;
    const QHash<QString, ClipCanvasTool> toolsById{
        {QStringLiteral("shape"), ClipCanvasTool::Shape},
        {QStringLiteral("arrow"), ClipCanvasTool::Arrow},
        {QStringLiteral("distance"), ClipCanvasTool::Distance},
        {QStringLiteral("line"), ClipCanvasTool::Line},
        {QStringLiteral("free-draw"), ClipCanvasTool::FreeDraw},
        {QStringLiteral("rectangle-highlight"), ClipCanvasTool::RectangleHighlight},
        {QStringLiteral("pen-highlight"), ClipCanvasTool::PenHighlight},
        {QStringLiteral("spotlight"), ClipCanvasTool::Spotlight},
        {QStringLiteral("rectangle-filter"), ClipCanvasTool::RectangleFilter},
        {QStringLiteral("pen-filter"), ClipCanvasTool::PenFilter},
        {QStringLiteral("text"), ClipCanvasTool::Text},
        {QStringLiteral("serial-number"), ClipCanvasTool::SerialNumber},
        {QStringLiteral("eraser"), ClipCanvasTool::Eraser},
        {QStringLiteral("watermark"), ClipCanvasTool::Watermark},
    };
    for (const QString& toolId : toolIds) {
        const auto it = toolsById.constFind(toolId);
        if (it != toolsById.cend()) {
            tools.insert(*it);
        }
    }
    return tools;
}
} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTDEFAULTSTYLES_H
