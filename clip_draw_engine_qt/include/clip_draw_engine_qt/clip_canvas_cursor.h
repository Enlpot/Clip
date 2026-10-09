#ifndef CLIP_DRAW_ENGINE_QT_CLIP_CANVAS_CURSOR_H
#define CLIP_DRAW_ENGINE_QT_CLIP_CANVAS_CURSOR_H

#include <QCursor>

// The same cursor is shared by canvas shapes and host-owned selection decorations.
[[nodiscard]] QCursor clipCanvasCornerRadiusCursor(qreal devicePixelRatio = 1.0);

#endif // CLIP_DRAW_ENGINE_QT_CLIP_CANVAS_CURSOR_H
