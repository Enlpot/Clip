#include "clip_canvas_text_editor_session.h"

#include "clip_canvas_element_id.h"
#include "clip_canvas_text.h"
#include "clip_canvas_text_edit_geometry.h"
#include "clip_canvas_text_editor_input.h"
#include "clip_canvas_text_editor_view.h"
#include "clip_canvas_text_layout.h"

#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QPainter>

#include <cstdint>

bool ClipCanvasTextEditorSession::FinishedEdit::shouldCommit(bool hasViewport) const {
    return hasViewport && (hasExistingElement || !text.trimmed().isEmpty());
}

bool ClipCanvasTextEditorSession::begin(const ClipTextElementInfo& info,
                                        const ClipSceneDisplayItem* existingSceneItem,
                                        const QFont& baseFont, const ClipTextStyle* newTextStyle,
                                        const QString* completeText) {
    cancel();

    const bool hasExisting = clip_canvas_element_id::hasElementId(info.id);
    const QString initialText =
        completeText != nullptr ? *completeText : clip_canvas_text::textFromElementInfo(info);
    m_arrowId = info.arrow_id;
    m_arrowWidth = info.arrow_width;
    ClipCanvasSceneItem preview = clip_canvas_text::defaultPreviewItem(info);
    if (hasExisting) {
        if (existingSceneItem != nullptr) {
            preview = *existingSceneItem;
        }
        clip_canvas_text::copyTextToSceneItem(preview, initialText);
        // Older saved drafts could commit the resized wrap rectangle together
        // with pre-resize ink bounds. Resolve the ink from the current font and
        // wrap width when opening an edit, without changing the stored frame.
        const auto measured = clip_canvas_text_layout::measureWrappedTextLayout(
            initialText, baseFont, preview, preview.width);
        preview.content_width = measured.content.width();
        preview.content_height = measured.content.height();
    } else {
        if (newTextStyle != nullptr) {
            clip_canvas_text::applyTextStyleToSceneItem(preview, *newTextStyle);
        }
        const clip_canvas_text_layout::TextMeasuredLayout initialMeasured =
            clip_canvas_text_layout::measureNaturalTextLayout(initialText, baseFont, preview);
        preview.width = initialMeasured.layout.width();
        preview.height = initialMeasured.layout.height();
        preview.content_width = initialMeasured.content.width();
        preview.content_height = initialMeasured.content.height();
    }

    if (newTextStyle != nullptr && clip_canvas_element_id::hasElementId(m_arrowId)) {
        clip_canvas_text::applyTextStyleToSceneItem(preview, *newTextStyle);
    }
    const QPointF creationPoint(info.center_x, info.center_y);
    m_canvasAnchor =
        hasExisting
            ? clip_canvas_text_edit_geometry::topAnchorForItem(preview)
            : clip_canvas_text_edit_geometry::topAnchorForCreationPoint(preview, creationPoint);
    const QPointF anchoredCenter =
        clip_canvas_text_edit_geometry::centerForTopAnchor(preview, m_canvasAnchor);
    preview.center_x = anchoredCenter.x();
    preview.center_y = anchoredCenter.y();
    m_previewItem = preview;
    m_hasPreview = true;
    m_previewAutoResize = !hasExisting || info.auto_resize != 0;
    m_hasExistingElement = hasExisting;
    m_elementId = hasExisting ? info.id : ClipElementId{};
    m_draft.begin(initialText);
    if (clip_canvas_element_id::hasElementId(m_arrowId)) {
        m_canvasAnchor = creationPoint;
        updatePreviewLayout(baseFont, true);
    }
    return true;
}

ClipCanvasTextEditorSession::FinishedEdit
ClipCanvasTextEditorSession::finish(const QFont& baseFont) {
    FinishedEdit result;
    if (!m_hasPreview) {
        resetState();
        return result;
    }

    m_draft.clearPreedit();
    updatePreviewFromState(baseFont);
    result.text = m_draft.text();
    result.canvasCenter = QPointF(m_previewItem.center_x, m_previewItem.center_y);
    result.elementId = m_elementId;
    result.arrowId = m_arrowId;
    result.hasExistingElement = m_hasExistingElement;
    result.measuredLayout = ClipTextLayoutSize{
        m_previewItem.width,
        m_previewItem.height,
        m_previewItem.content_width,
        m_previewItem.content_height,
    };
    result.style = clip_canvas_text::textStyleFromSceneItem(m_previewItem);
    result.autoResize = m_previewAutoResize;
    result.styleChanged = m_styleChanged;
    resetState();
    return result;
}

void ClipCanvasTextEditorSession::cancel() {
    resetState();
}

bool ClipCanvasTextEditorSession::isActive() const {
    return m_hasPreview;
}

bool ClipCanvasTextEditorSession::containsViewPosition(const SceneDisplayInfo& sceneInfo,
                                                       const QPointF& viewPosition) const {
    if (!m_hasPreview) {
        return false;
    }
    return clip_canvas_text_editor_view::itemContainsViewPosition(m_previewItem, sceneInfo,
                                                                  viewPosition);
}

const ClipSceneDisplayItem* ClipCanvasTextEditorSession::previewItem() const {
    return m_hasPreview ? &m_previewItem : nullptr;
}

QString ClipCanvasTextEditorSession::presentationText() const {
    return m_hasPreview ? m_draft.displayText() : QString();
}

bool ClipCanvasTextEditorSession::activeDraftHasExistingElement() const {
    return m_hasPreview && m_hasExistingElement;
}

bool ClipCanvasTextEditorSession::activeDraftAutoResize() const {
    return m_hasPreview && m_previewAutoResize;
}

ClipTextStyle ClipCanvasTextEditorSession::currentTextStyle() const {
    return m_hasPreview ? clip_canvas_text::textStyleFromSceneItem(m_previewItem) : ClipTextStyle{};
}

bool ClipCanvasTextEditorSession::applyActiveDraftPresentation(const ClipTextElementInfo& info,
                                                               const ClipTextStyle& style) {
    if (!m_hasPreview) {
        return false;
    }
    const bool infoHasElement = clip_canvas_element_id::hasElementId(info.id);
    if (m_hasExistingElement != infoHasElement) {
        return false;
    }
    if (m_hasExistingElement && !clip_canvas_element_id::sameElementId(m_elementId, info.id)) {
        return false;
    }

    m_arrowId = info.arrow_id;
    m_arrowWidth = info.arrow_width;
    m_previewItem.center_x = info.center_x;
    m_previewItem.center_y = info.center_y;
    m_previewItem.width = qMax(1.0, info.width);
    m_previewItem.height = qMax(1.0, info.height);
    m_previewItem.content_width = info.content_width;
    m_previewItem.content_height = info.content_height;
    m_previewItem.rotation = info.rotation;
    clip_canvas_text::applyTextStyleToSceneItem(m_previewItem, style);
    clip_canvas_text::copyTextToSceneItem(m_previewItem, m_draft.displayText());
    m_previewAutoResize = info.auto_resize != 0;
    m_canvasAnchor = clip_canvas_element_id::hasElementId(m_arrowId)
                         ? QPointF(m_previewItem.center_x, m_previewItem.center_y)
                         : clip_canvas_text_edit_geometry::topAnchorForItem(m_previewItem);
    return true;
}

bool ClipCanvasTextEditorSession::syncActiveDraftPresentation(ClipRuntime runtime,
                                                              ClipViewport viewport,
                                                              const QFont& baseFont,
                                                              const SceneDisplayInfo& sceneInfo,
                                                              QRegion* outUpdateRegion) {
    if (!m_hasPreview || runtime == nullptr || viewport == nullptr) {
        return false;
    }

    ClipTextElementInfo info{};
    ClipTextStyle style{};
    std::uint8_t active = 0;
    if (clip_viewport_get_active_text_draft_presentation(runtime, viewport, &info, &style,
                                                         &active) != CLIP_OK ||
        active == 0) {
        return false;
    }

    QRegion updateRegion = editingRegion(baseFont, sceneInfo);
    if (!applyActiveDraftPresentation(info, style)) {
        return false;
    }
    updateRegion += editingRegion(baseFont, sceneInfo);
    if (outUpdateRegion != nullptr) {
        *outUpdateRegion += updateRegion;
    }
    return true;
}

QRegion ClipCanvasTextEditorSession::previewRegion(const SceneDisplayInfo& sceneInfo,
                                                   int paddingPx) const {
    if (!m_hasPreview) {
        return {};
    }
    return clip_canvas_text_editor_view::previewRegion(m_previewItem, sceneInfo, paddingPx);
}

QRegion ClipCanvasTextEditorSession::editingRegion(const QFont& baseFont,
                                                   const SceneDisplayInfo& sceneInfo) const {
    if (!m_hasPreview) {
        return {};
    }
    return clip_canvas_text_editor_view::editingRegion(m_previewItem, m_draft, baseFont, sceneInfo);
}

QRegion ClipCanvasTextEditorSession::caretRegion(const QFont& baseFont,
                                                 const SceneDisplayInfo& sceneInfo) const {
    if (!m_hasPreview) {
        return {};
    }
    return clip_canvas_text_editor_view::caretRegion(m_previewItem, m_draft, baseFont, sceneInfo);
}

void ClipCanvasTextEditorSession::updatePreviewFromState(const QFont& baseFont) {
    updatePreviewLayout(baseFont, false);
}

QRegion ClipCanvasTextEditorSession::updatePreviewFromState(const QFont& baseFont,
                                                            const SceneDisplayInfo& sceneInfo) {
    if (!m_hasPreview) {
        return {};
    }

    QRegion updateRegion = editingRegion(baseFont, sceneInfo);
    updatePreviewFromState(baseFont);
    updateRegion += editingRegion(baseFont, sceneInfo);
    return updateRegion;
}

QRegion ClipCanvasTextEditorSession::applyTextStyle(const ClipTextStyle& style,
                                                    const QFont& baseFont,
                                                    const SceneDisplayInfo& sceneInfo) {
    if (!m_hasPreview) {
        return {};
    }

    QRegion updateRegion = editingRegion(baseFont, sceneInfo);
    const double previousFontSize = m_previewItem.font_size;
    const QString previousFontFamily = clip_canvas_text::fontFamilyFromSceneItem(m_previewItem);
    clip_canvas_text::applyTextStyleToSceneItem(m_previewItem, style);
    m_styleChanged = true;
    m_canvasAnchor = clip_canvas_element_id::hasElementId(m_arrowId)
                         ? QPointF(m_previewItem.center_x, m_previewItem.center_y)
                         : clip_canvas_text_edit_geometry::topAnchorForItem(m_previewItem);
    const bool textLayoutChanged =
        previousFontSize != m_previewItem.font_size ||
        previousFontFamily != clip_canvas_text::fontFamilyFromSceneItem(m_previewItem);
    updatePreviewLayout(baseFont, textLayoutChanged);
    updateRegion += editingRegion(baseFont, sceneInfo);
    return updateRegion;
}

void ClipCanvasTextEditorSession::updateGeometry(const QFont& baseFont,
                                                 const SceneDisplayInfo& sceneInfo) {
    Q_UNUSED(sceneInfo);
    updatePreviewFromState(baseFont);
}

bool ClipCanvasTextEditorSession::setCursorFromViewPosition(const QFont& baseFont,
                                                            const SceneDisplayInfo& sceneInfo,
                                                            const QPointF& viewPosition,
                                                            bool keepSelection) {
    if (!m_hasPreview) {
        return false;
    }

    const bool clearedPreedit = m_draft.ensureNoPreeditForDirectEdit();
    const int position = clip_canvas_text_editor_view::cursorPositionForViewPosition(
        m_previewItem, baseFont, sceneInfo, viewPosition, m_draft.text());
    const bool cursorChanged = m_draft.setCursorPosition(position, keepSelection);
    if (clearedPreedit) {
        updatePreviewFromState(baseFont);
    }
    return clearedPreedit || cursorChanged;
}

ClipCanvasTextEditorSession::KeyResult
ClipCanvasTextEditorSession::handleKeyPress(QKeyEvent* event, const QFont& baseFont,
                                            const SceneDisplayInfo& sceneInfo) {
    KeyResult result;
    if (!m_hasPreview || event == nullptr) {
        return result;
    }

    const clip_canvas_text_editor_input::KeyResult commandResult =
        clip_canvas_text_editor_input::handleKeyPress(
            event, m_draft,
            [this, &baseFont, &sceneInfo](QTextCursor::MoveOperation operation,
                                          QTextCursor::MoveMode mode) {
                return moveCursor(operation, mode, baseFont, sceneInfo);
            });
    result.handled = commandResult.handled;
    result.changed = commandResult.changed;
    switch (commandResult.command) {
    case clip_canvas_text_editor_input::EventCommand::Commit:
        result.command = EventCommand::Commit;
        break;
    case clip_canvas_text_editor_input::EventCommand::DeleteElement:
        result.command = EventCommand::DeleteElement;
        break;
    case clip_canvas_text_editor_input::EventCommand::None:
    default:
        break;
    }

    if (result.changed) {
        updatePreviewFromState(baseFont);
    }
    return result;
}

bool ClipCanvasTextEditorSession::handleInputMethodEvent(QInputMethodEvent* event,
                                                         const QFont& baseFont) {
    if (!m_hasPreview || event == nullptr) {
        return false;
    }

    const bool changed = m_draft.handleInputMethodEvent(*event);
    if (changed) {
        updatePreviewFromState(baseFont);
    }
    return changed;
}

QVariant ClipCanvasTextEditorSession::inputMethodQuery(Qt::InputMethodQuery query,
                                                       const QFont& baseFont,
                                                       const SceneDisplayInfo& sceneInfo) const {
    if (!m_hasPreview) {
        return {};
    }

    switch (query) {
    case Qt::ImEnabled:
        return true;
    case Qt::ImHints:
        return static_cast<int>(Qt::ImhMultiLine);
    case Qt::ImCursorRectangle:
        return clip_canvas_text_editor_view::cursorRect(m_previewItem, baseFont, sceneInfo,
                                                        m_draft.displayText(),
                                                        m_draft.inputMethodCursorPosition());
    case Qt::ImCursorPosition:
        return m_draft.inputMethodCursorPosition();
    case Qt::ImAnchorPosition:
        return m_draft.inputMethodAnchorPosition();
    case Qt::ImSurroundingText:
        return m_draft.displayText();
    case Qt::ImCurrentSelection:
        return m_draft.inputMethodCurrentSelection();
    case Qt::ImFont:
        return clip_canvas_text_editor_view::inputMethodFont(m_previewItem, baseFont, sceneInfo);
    default:
        return {};
    }
}

void ClipCanvasTextEditorSession::renderEditorOverlay(QPainter& painter, const QFont& baseFont,
                                                      const SceneDisplayInfo& sceneInfo,
                                                      bool caretVisible) const {
    if (!m_hasPreview) {
        return;
    }

    clip_canvas_text_editor_view::renderOverlay(painter, m_previewItem, m_draft, baseFont,
                                                sceneInfo, caretVisible);
}

void ClipCanvasTextEditorSession::resetState() {
    m_canvasAnchor = {};
    m_elementId = {};
    m_arrowId = {};
    m_arrowWidth = 0.0;
    m_hasExistingElement = false;
    m_previewItem = ClipCanvasSceneItem{};
    m_hasPreview = false;
    m_previewAutoResize = false;
    m_styleChanged = false;
    m_draft.reset();
}

void ClipCanvasTextEditorSession::releaseRetainedState() {
    resetState();
    m_draft.releaseRetainedState();
}

void ClipCanvasTextEditorSession::updatePreviewLayout(const QFont& baseFont, bool forceLayout) {
    if (!m_hasPreview) {
        return;
    }

    const QString text = m_draft.displayText();
    // Persisted engine geometry remains authoritative until a text or font input changes.
    if (!forceLayout && clip_canvas_text::textFromSceneItem(m_previewItem) == text) {
        return;
    }

    if (clip_canvas_element_id::hasElementId(m_arrowId)) {
        clip_canvas_text::copyTextToSceneItem(m_previewItem, text);
        const clip_canvas_text_layout::TextMeasuredLayout natural =
            clip_canvas_text_layout::measureNaturalTextLayout(text, baseFont, m_previewItem);
        const double maximum = qMax(m_arrowWidth * 0.7, m_previewItem.font_size * 11.0);
        m_previewItem.width = qMin(natural.layout.width(), maximum);
        const clip_canvas_text_layout::TextMeasuredLayout wrapped =
            clip_canvas_text_layout::measureWrappedTextLayout(text, baseFont, m_previewItem,
                                                              m_previewItem.width);
        m_previewItem.height = wrapped.layout.height();
        m_previewItem.content_width = wrapped.content.width();
        m_previewItem.content_height = wrapped.content.height();
        m_previewItem.rotation = 0.0;
        m_previewItem.center_x = m_canvasAnchor.x();
        m_previewItem.center_y = m_canvasAnchor.y();
        return;
    }
    clip_canvas_text::updatePreviewFromEditorText(m_previewItem, text, m_previewAutoResize,
                                                  baseFont);
    updatePreviewAnchor();
}

void ClipCanvasTextEditorSession::updatePreviewAnchor() {
    if (!m_hasPreview) {
        return;
    }

    const QPointF center =
        clip_canvas_text_edit_geometry::centerForTopAnchor(m_previewItem, m_canvasAnchor);
    m_previewItem.center_x = center.x();
    m_previewItem.center_y = center.y();
}

bool ClipCanvasTextEditorSession::moveCursor(QTextCursor::MoveOperation operation,
                                             QTextCursor::MoveMode mode, const QFont& baseFont,
                                             const SceneDisplayInfo& sceneInfo) {
    const bool clearedPreedit = m_draft.ensureNoPreeditForDirectEdit();
    const int position = clip_canvas_text_editor_view::movedCursorPosition(
        m_previewItem, baseFont, sceneInfo, m_draft.text(), m_draft.cursorPosition(), operation,
        mode);
    return m_draft.setCursorPosition(position, mode == QTextCursor::KeepAnchor) || clearedPreedit;
}
