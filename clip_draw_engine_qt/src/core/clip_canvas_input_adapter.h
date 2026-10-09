#pragma once

#include <QColor>
#include <optional>

#include <QCursor>
#include <QPointF>
#include <Qt>

#include <cstdint>

#include "clip_draw_engine.h"

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

namespace clip_canvas_input {

QCursor strokeCursor(double diameter, const std::optional<QColor>& color, bool crosshair,
                     qreal devicePixelRatio = 1.0);
QCursor cursorForClipCursor(ClipCursorStyle style, qreal devicePixelRatio = 1.0);

ClipInputEvent makePointerInput(const QMouseEvent& event, ClipPointerEventType eventType);
ClipInputEvent makePointerInput(const QPointF& position, Qt::MouseButton button,
                                Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
                                ClipPointerEventType eventType);
ClipInputEvent makeWheelInput(const QWheelEvent& event);
ClipInputEvent makeKeyInput(const QKeyEvent& event, ClipKeyEventType eventType);

ClipPointerButton mapButton(Qt::MouseButton button);
std::uint8_t mapButtons(Qt::MouseButtons buttons);
ClipModifiers mapModifiers(Qt::KeyboardModifiers modifiers);
ClipKeyCode mapKeyCode(const QKeyEvent& event, std::uint32_t* outCodepoint);

} // namespace clip_canvas_input
