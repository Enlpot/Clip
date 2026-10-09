#pragma once

#include "clip_canvas_display_cache.h"
#include "clip_canvas_text_draft.h"
#include "clip_draw_engine.h"

#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QRegion>
#include <QTextCursor>

class QPainter;

namespace clip_canvas_text_editor_view {

QPointF canvasToViewPoint(const SceneDisplayInfo& sceneInfo, const QPointF& canvasPosition);
bool itemContainsViewPosition(const ClipSceneDisplayItem& item, const SceneDisplayInfo& sceneInfo,
                              const QPointF& viewPosition);
QRegion previewRegion(const ClipSceneDisplayItem& item, const SceneDisplayInfo& sceneInfo,
                      int paddingPx = 2);
QRectF cursorRect(const ClipSceneDisplayItem& item, const QFont& baseFont,
                  const SceneDisplayInfo& sceneInfo, const QString& text, int cursorPosition);
QRegion caretRegion(const ClipSceneDisplayItem& item, const ClipCanvasTextDraft& draft,
                    const QFont& baseFont, const SceneDisplayInfo& sceneInfo, int paddingPx = 3);
QRegion editingRegion(const ClipSceneDisplayItem& item, const ClipCanvasTextDraft& draft,
                      const QFont& baseFont, const SceneDisplayInfo& sceneInfo);
int cursorPositionForViewPosition(const ClipSceneDisplayItem& item, const QFont& baseFont,
                                  const SceneDisplayInfo& sceneInfo, const QPointF& viewPosition,
                                  const QString& text);
int movedCursorPosition(const ClipSceneDisplayItem& item, const QFont& baseFont,
                        const SceneDisplayInfo& sceneInfo, const QString& text, int cursorPosition,
                        QTextCursor::MoveOperation operation,
                        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
QFont inputMethodFont(const ClipSceneDisplayItem& item, const QFont& baseFont,
                      const SceneDisplayInfo& sceneInfo);
void renderOverlay(QPainter& painter, const ClipSceneDisplayItem& item,
                   const ClipCanvasTextDraft& draft, const QFont& baseFont,
                   const SceneDisplayInfo& sceneInfo, bool caretVisible = true);

} // namespace clip_canvas_text_editor_view
