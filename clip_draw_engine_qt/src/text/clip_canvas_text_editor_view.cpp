#include "clip_canvas_text_editor_view.h"

#include "clip_canvas_render_geometry.h"
#include "clip_canvas_text_edit_geometry.h"
#include "clip_canvas_text_editor_overlay.h"
#include "clip_canvas_text_layout.h"

#include <QPainter>

namespace clip_canvas_text_editor_view {

QPointF canvasToViewPoint(const SceneDisplayInfo& sceneInfo, const QPointF& canvasPosition) {
    return clip_canvas_render_geometry::canvasToView(
        sceneInfo.camera_center_x, sceneInfo.camera_center_y, sceneInfo.camera_zoom,
        sceneInfo.surface_width, sceneInfo.surface_height, canvasPosition.x(), canvasPosition.y());
}

bool itemContainsViewPosition(const ClipSceneDisplayItem& item, const SceneDisplayInfo& sceneInfo,
                              const QPointF& viewPosition) {
    return clip_canvas_text_edit_geometry::viewPointInsideTextItem(
        item, canvasToViewPoint(sceneInfo, QPointF(item.center_x, item.center_y)),
        sceneInfo.camera_zoom, viewPosition);
}

QRegion previewRegion(const ClipSceneDisplayItem& item, const SceneDisplayInfo& sceneInfo,
                      int paddingPx) {
    return QRegion(clip_canvas_render_geometry::alignedRectForBounds(
        clip_canvas_render_geometry::sceneItemBounds(sceneInfo, item), paddingPx));
}

QRectF cursorRect(const ClipSceneDisplayItem& item, const QFont& baseFont,
                  const SceneDisplayInfo& sceneInfo, const QString& text, int cursorPosition) {
    return clip_canvas_text_edit_geometry::cursorRectForTextPosition(
        item, baseFont, canvasToViewPoint(sceneInfo, QPointF(item.center_x, item.center_y)),
        sceneInfo.camera_zoom, text, cursorPosition);
}

QRegion caretRegion(const ClipSceneDisplayItem& item, const ClipCanvasTextDraft& draft,
                    const QFont& baseFont, const SceneDisplayInfo& sceneInfo, int paddingPx) {
    const QRectF caretBounds =
        cursorRect(item, baseFont, sceneInfo, draft.displayText(), draft.displayCursorPosition());
    if (caretBounds.isEmpty()) {
        return {};
    }
    return QRegion(clip_canvas_render_geometry::alignedRectForBounds(caretBounds, paddingPx));
}

QRegion editingRegion(const ClipSceneDisplayItem& item, const ClipCanvasTextDraft& draft,
                      const QFont& baseFont, const SceneDisplayInfo& sceneInfo) {
    QRegion region = previewRegion(item, sceneInfo, 2);
    region += caretRegion(item, draft, baseFont, sceneInfo);
    return region;
}

int cursorPositionForViewPosition(const ClipSceneDisplayItem& item, const QFont& baseFont,
                                  const SceneDisplayInfo& sceneInfo, const QPointF& viewPosition,
                                  const QString& text) {
    return clip_canvas_text_edit_geometry::cursorPositionForViewPoint(
        item, baseFont, canvasToViewPoint(sceneInfo, QPointF(item.center_x, item.center_y)),
        sceneInfo.camera_zoom, viewPosition, text);
}

int movedCursorPosition(const ClipSceneDisplayItem& item, const QFont& baseFont,
                        const SceneDisplayInfo& sceneInfo, const QString& text, int cursorPosition,
                        QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode) {
    return clip_canvas_text_edit_geometry::movedCursorPosition(
        item, baseFont, sceneInfo.camera_zoom, text, cursorPosition, operation, mode);
}

QFont inputMethodFont(const ClipSceneDisplayItem& item, const QFont& baseFont,
                      const SceneDisplayInfo& sceneInfo) {
    return clip_canvas_text_layout::fontForItem(baseFont, item,
                                                qMax(0.0001, sceneInfo.camera_zoom));
}

void renderOverlay(QPainter& painter, const ClipSceneDisplayItem& item,
                   const ClipCanvasTextDraft& draft, const QFont& baseFont,
                   const SceneDisplayInfo& sceneInfo, bool caretVisible) {
    const QString renderedText = draft.displayText();
    const QPointF centerView = canvasToViewPoint(sceneInfo, QPointF(item.center_x, item.center_y));
    if (draft.hasSelection() && !draft.hasPreedit()) {
        clip_canvas_text_editor_overlay::renderSelection(
            painter, item, renderedText, draft.selectionStart(), draft.selectionEnd(), baseFont,
            centerView, sceneInfo.camera_zoom);
        clip_canvas_text_editor_overlay::renderSelectedText(
            painter, item, renderedText, draft.selectionStart(), draft.selectionEnd(), baseFont,
            centerView, sceneInfo.camera_zoom);
    }
    if (draft.hasPreedit()) {
        clip_canvas_text_editor_overlay::renderPreeditUnderline(
            painter, item, renderedText, draft.preeditStart(), draft.preeditLength(), baseFont,
            centerView, sceneInfo.camera_zoom);
    }
    if (caretVisible) {
        clip_canvas_text_editor_overlay::renderCaret(painter, item, renderedText,
                                                     draft.displayCursorPosition(), baseFont,
                                                     centerView, sceneInfo.camera_zoom);
    }
}

} // namespace clip_canvas_text_editor_view
