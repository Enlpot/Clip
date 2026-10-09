#include "clip_canvas_cursor_controller.h"

#include <QWidget>
#include "clip_canvas_input_adapter.h"

#include <algorithm>
#include <cmath>

ClipCanvasCursorController::ClipCanvasCursorController(QWidget& widget) : m_widget(widget) {}

void ClipCanvasCursorController::setCursor(ClipCanvasCursorLayer layer, const QCursor& cursor) {
    if (layer == ClipCanvasCursorLayer::CanvasTool) {
        m_engineCursorStyle.reset();
    }
    cursorForLayer(layer) = cursor;
    applyResolvedCursor();
}

void ClipCanvasCursorController::clearCursor(ClipCanvasCursorLayer layer) {
    if (layer == ClipCanvasCursorLayer::CanvasTool) {
        m_engineCursorStyle.reset();
    }
    cursorForLayer(layer).reset();
    applyResolvedCursor();
}

std::optional<QCursor>& ClipCanvasCursorController::cursorForLayer(ClipCanvasCursorLayer layer) {
    switch (layer) {
    case ClipCanvasCursorLayer::Host:
        return m_hostCursor;
    case ClipCanvasCursorLayer::CanvasTool:
    default:
        return m_canvasToolCursor;
    }
}

void ClipCanvasCursorController::applyResolvedCursor() {
    if (m_hostCursor.has_value()) {
        applyResolvedCursorToWidget(*m_hostCursor);
        return;
    }
    if (m_canvasToolCursor.has_value()) {
        applyResolvedCursorToWidget(*m_canvasToolCursor);
        return;
    }
    if (m_widget.testAttribute(Qt::WA_SetCursor)) {
        m_widget.unsetCursor();
    }
}

void ClipCanvasCursorController::applyResolvedCursorToWidget(const QCursor& cursor) {
    // QWidget forwards every cursor change to the platform window, and on
    // Windows each changed-shape transition reaches the native sprite
    // immediately. Re-applying the cursor a widget already shows would flash
    // it, so layered updates must resolve to a no-op here.
    if (m_widget.testAttribute(Qt::WA_SetCursor) && m_widget.cursor() == cursor) {
        return;
    }
    m_widget.setCursor(cursor);
}

void ClipCanvasCursorController::setEngineCursor(ClipCursorStyle style) {
    const qreal dpr = m_widget.devicePixelRatioF();
    if (dpr != m_cursorDevicePixelRatio) {
        m_cursorDevicePixelRatio = dpr;
        m_strokeCursor.reset();
        m_eraserCursor.reset();
    }
    if (style == CLIP_CURSOR_STYLE_STROKE || style == CLIP_CURSOR_STYLE_ERASER) {
        auto& cached = style == CLIP_CURSOR_STYLE_STROKE ? m_strokeCursor : m_eraserCursor;
        if (!cached) {
            cached = clip_canvas_input::strokeCursor(
                style == CLIP_CURSOR_STYLE_STROKE ? m_strokeDiameter : 16.0,
                style == CLIP_CURSOR_STYLE_STROKE ? m_strokeColor : std::nullopt,
                style == CLIP_CURSOR_STYLE_STROKE, dpr);
        }
        m_canvasToolCursor = *cached;
    } else {
        m_canvasToolCursor = clip_canvas_input::cursorForClipCursor(style, dpr);
    }
    m_engineCursorStyle = style;
    applyResolvedCursor();
}

void ClipCanvasCursorController::configureStrokeCursor(double diameter,
                                                       const std::optional<QColor>& color) {
    // Bound native bitmap allocation even at extreme canvas zoom levels.
    diameter = std::isfinite(diameter) ? std::clamp(diameter, 1.0, 1024.0) : 1.0;
    if (m_strokeDiameter == diameter && m_strokeColor == color &&
        m_cursorDevicePixelRatio == m_widget.devicePixelRatioF()) {
        return;
    }
    m_strokeDiameter = diameter;
    m_strokeColor = color;
    m_strokeCursor.reset();
    refreshDevicePixelRatio();
}

void ClipCanvasCursorController::refreshDevicePixelRatio() {
    if (m_engineCursorStyle) {
        setEngineCursor(*m_engineCursorStyle);
    }
}
