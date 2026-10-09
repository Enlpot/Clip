#pragma once

#include "clip_draw_engine_qt/clip_canvas_types.h"

#include <QCursor>
#include <QColor>
#include "clip_draw_engine.h"

#include <optional>

class QWidget;

class ClipCanvasCursorController final {
  public:
    explicit ClipCanvasCursorController(QWidget& widget);

    void setCursor(ClipCanvasCursorLayer layer, const QCursor& cursor);
    void clearCursor(ClipCanvasCursorLayer layer);
    void setEngineCursor(ClipCursorStyle style);
    void configureStrokeCursor(double diameter, const std::optional<QColor>& color);
    void refreshDevicePixelRatio();

  private:
    std::optional<QCursor>& cursorForLayer(ClipCanvasCursorLayer layer);
    void applyResolvedCursor();
    void applyResolvedCursorToWidget(const QCursor& cursor);

    QWidget& m_widget;
    std::optional<ClipCursorStyle> m_engineCursorStyle;
    double m_strokeDiameter = 2.0;
    std::optional<QColor> m_strokeColor;
    qreal m_cursorDevicePixelRatio = 0.0;
    std::optional<QCursor> m_strokeCursor;
    std::optional<QCursor> m_eraserCursor;
    std::optional<QCursor> m_canvasToolCursor;
    std::optional<QCursor> m_hostCursor;
};
