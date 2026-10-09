#pragma once

#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTextCursor>

#include "clip_draw_engine.h"

namespace clip_canvas_text_edit_geometry {

QPointF topAnchorForItem(const ClipSceneDisplayItem& item);
QPointF topAnchorForCreationPoint(const ClipSceneDisplayItem& item, const QPointF& creationPoint);
QPointF centerForTopAnchor(const ClipSceneDisplayItem& item, const QPointF& anchor);

int cursorPositionForViewPoint(const ClipSceneDisplayItem& item, const QFont& baseFont,
                               const QPointF& centerView, double zoom, const QPointF& viewPosition,
                               const QString& text);
int movedCursorPosition(const ClipSceneDisplayItem& item, const QFont& baseFont, double zoom,
                        const QString& text, int cursorPosition,
                        QTextCursor::MoveOperation operation,
                        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
QRectF cursorRectForTextPosition(const ClipSceneDisplayItem& item, const QFont& baseFont,
                                 const QPointF& centerView, double zoom, const QString& text,
                                 int cursorPosition);
bool viewPointInsideTextItem(const ClipSceneDisplayItem& item, const QPointF& centerView,
                             double zoom, const QPointF& viewPosition);

} // namespace clip_canvas_text_edit_geometry
