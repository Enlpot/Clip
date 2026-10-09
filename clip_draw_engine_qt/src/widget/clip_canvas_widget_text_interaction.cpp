#include "clip_canvas_widget_text_interaction.h"

#include "clip_canvas_cursor_controller.h"
#include "clip_canvas_element_id.h"
#include "clip_canvas_render_geometry.h"
#include "clip_canvas_text.h"
#include "clip_canvas_text_layout.h"
#include "clip_canvas_text_render.h"
#include "clip_canvas_text_edit_target.h"
#include "clip_canvas_text_editor_connector.h"
#include "clip_canvas_text_measurement.h"
#include "clip_canvas_widget_repaint.h"

#include <QByteArray>
#include <QFont>
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QStyleHints>
#include <QWidget>

#include <cmath>
#include <limits>
#include <optional>
#include <utility>

namespace {

int caretBlinkPhaseDuration(int flashTimeMs) {
    // Qt reports the complete visible-plus-hidden flash cycle.
    return qMax(1, flashTimeMs / 2);
}

void updateInputMethod() {
    if (QInputMethod* inputMethod = QGuiApplication::inputMethod()) {
        inputMethod->update(Qt::ImEnabled | Qt::ImCursorRectangle | Qt::ImCursorPosition |
                            Qt::ImAnchorPosition | Qt::ImSurroundingText | Qt::ImCurrentSelection);
    }
}

void resetInputMethod() {
    if (QInputMethod* inputMethod = QGuiApplication::inputMethod()) {
        inputMethod->reset();
    }
}

void commitInputMethod() {
    if (QInputMethod* inputMethod = QGuiApplication::inputMethod()) {
        inputMethod->commit();
    }
}

QRegion committedTextRegion(const QRegion& editorRegion, const ClipCanvasDisplayCache& displayCache,
                            const QRect& widgetRect) {
    return editorRegion +
           clip_canvas_display::dirtyRectsToRegion(displayCache.sceneDirtyRects(),
                                                   displayCache.sceneDirtyRectCount(), widgetRect) +
           clip_canvas_display::dirtyRectsToRegion(
               displayCache.overlayDirtyRects(), displayCache.overlayDirtyRectCount(), widgetRect);
}

} // namespace

ClipCanvasWidgetTextInteraction::ClipCanvasWidgetTextInteraction(
    QWidget& widget, ClipCanvasCursorController& cursorController)
    : m_widget(widget), m_cursorController(cursorController) {
    m_caretBlinkTimer.setTimerType(Qt::CoarseTimer);
    QObject::connect(&m_caretBlinkTimer, &QTimer::timeout, &m_caretBlinkTimer,
                     [this]() { handleCaretBlinkTimeout(); });
    QObject::connect(qGuiApp, &QGuiApplication::fontDatabaseChanged, &m_caretBlinkTimer,
                     [this]() { invalidateArrowTextMetrics(); });

    if (QStyleHints* styleHints = QGuiApplication::styleHints()) {
        setCaretFlashTime(styleHints->cursorFlashTime());
        QObject::connect(styleHints, &QStyleHints::cursorFlashTimeChanged, &m_caretBlinkTimer,
                         [this](int flashTimeMs) { setCaretFlashTime(flashTimeMs); });
    }
}

ClipCanvasTextEditorSession& ClipCanvasWidgetTextInteraction::session() {
    return m_session;
}

const ClipCanvasTextEditorSession& ClipCanvasWidgetTextInteraction::session() const {
    return m_session;
}

void ClipCanvasWidgetTextInteraction::invalidateArrowTextMetrics() {
    m_arrowNaturalLayouts.clear();
    clip_canvas_text_render::resetLayoutCacheForCurrentThread();
    m_arrowMetricsInvalid = true;
}

void ClipCanvasWidgetTextInteraction::resetDocumentRetainedState() {
    m_session.releaseRetainedState();
    invalidateArrowTextMetrics();
}

clip_canvas_commands::MutationResult
ClipCanvasWidgetTextInteraction::measureArrowText(ClipRuntime runtime, ClipViewport viewport) {
    clip_canvas_commands::MutationResult result;
    if (runtime == nullptr || viewport == nullptr) {
        return result;
    }
    if (m_arrowMetricsInvalid || m_arrowLayoutFont != m_widget.font()) {
        if (clip_viewport_invalidate_arrow_text_layouts(runtime, viewport) != CLIP_OK) {
            return result;
        }
        m_arrowLayoutFont = m_widget.font();
        m_arrowMetricsInvalid = false;
    }
    std::uint32_t count = 0;
    if (clip_viewport_get_arrow_text_layout_requests(runtime, viewport, nullptr, 0, &count) !=
        CLIP_OK) {
        return result;
    }
    result.success = true;
    if (count == 0) {
        return result;
    }
    std::vector<ClipArrowTextLayoutRequest> requests(count);
    if (clip_viewport_get_arrow_text_layout_requests(runtime, viewport, requests.data(), count,
                                                     &count) != CLIP_OK) {
        result.success = false;
        return result;
    }
    std::vector<ClipArrowTextLayoutMetrics> layouts;
    layouts.reserve(requests.size());
    for (const auto& request : requests) {
        ClipCanvasSceneItem item = clip_canvas_text::defaultPreviewItem(request.info);
        clip_canvas_text::applyTextStyleToSceneItem(item, request.style);
        QString text = clip_canvas_text::textFromElementInfo(request.info);
        if (request.info.text_truncated != 0) {
            std::uint32_t length = 0;
            if (clip_runtime_get_text_utf8(runtime, request.info.id, nullptr, 0, &length) !=
                CLIP_OK) {
                result.success = false;
                return result;
            }
            QByteArray utf8(static_cast<qsizetype>(length), Qt::Uninitialized);
            if (clip_runtime_get_text_utf8(runtime, request.info.id,
                                           reinterpret_cast<std::uint8_t*>(utf8.data()), length,
                                           &length) != CLIP_OK) {
                result.success = false;
                return result;
            }
            text = QString::fromUtf8(utf8);
        }
        const clip_canvas_text_layout::TextMeasuredLayout natural =
            m_arrowNaturalLayouts.measure(text, m_widget.font(), item);
        const double width = qMin(natural.layout.width(), request.max_width);
        const clip_canvas_text_layout::TextMeasuredLayout wrapped =
            width == natural.layout.width() ? natural
                                            : clip_canvas_text_layout::measureWrappedTextLayout(
                                                  text, m_widget.font(), item, width);
        layouts.push_back(ClipArrowTextLayoutMetrics{
            request.info.id,
            request.key,
            {width, wrapped.layout.height(), wrapped.content.width(), wrapped.content.height()},
            natural.layout.width(),
        });
    }
    result.success =
        clip_viewport_apply_arrow_text_layout_metrics_ex(
            runtime, viewport, layouts.data(), static_cast<std::uint32_t>(layouts.size()),
            result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

clip_canvas_commands::MutationResult
ClipCanvasWidgetTextInteraction::measureSerialLabelLayout(ClipRuntime runtime,
                                                          ClipViewport viewport) {
    clip_canvas_commands::MutationResult result;
    if (runtime == nullptr || viewport == nullptr) {
        return result;
    }
    ClipSerialLabelLayoutRequest request{};
    std::uint8_t hasRequest = 0;
    if (clip_viewport_get_serial_label_layout_request(runtime, viewport, &request, &hasRequest) !=
        CLIP_OK) {
        return result;
    }
    result.success = true;
    if (hasRequest == 0) {
        return result;
    }
    const ClipTextLayoutSize layout =
        clip_canvas_text_measurement::measureSerialLabelLayout(request, m_widget.font());
    result.success =
        clip_viewport_apply_serial_label_layout_ex(runtime, viewport, request.text_id, layout,
                                                   result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

bool ClipCanvasWidgetTextInteraction::isActive() const {
    return m_session.isActive();
}

bool ClipCanvasWidgetTextInteraction::editorContains(const ClipCanvasDisplayCache& displayCache,
                                                     const QPointF& position) const {
    return m_session.containsViewPosition(displayCache.sceneInfo(), position);
}

bool ClipCanvasWidgetTextInteraction::hasSelectionInteraction() const {
    return m_session.isActive() && m_session.arrowId().generation == 0;
}

const ClipSceneDisplayItem* ClipCanvasWidgetTextInteraction::previewItem() const {
    return m_session.previewItem();
}

ClipTextStyle ClipCanvasWidgetTextInteraction::currentTextStyle() const {
    return m_session.currentTextStyle();
}

clip_canvas_commands::MutationResult
ClipCanvasWidgetTextInteraction::publishActiveDraftPresentation(ClipRuntime runtime,
                                                                ClipViewport viewport) const {
    clip_canvas_commands::MutationResult result;
    const ClipSceneDisplayItem* preview = m_session.previewItem();
    if (preview == nullptr || preview->kind != CLIP_SCENE_DISPLAY_ITEM_TEXT) {
        result.success = true;
        return result;
    }

    const QByteArray utf8 = m_session.presentationText().toUtf8();
    return clip_canvas_commands::setActiveTextDraftPresentation(
        runtime, viewport,
        clip_canvas_commands::ActiveTextDraftPresentationRequest{
            preview->element_id,
            m_session.activeDraftHasExistingElement(),
            preview->center_x,
            preview->center_y,
            preview->width,
            preview->height,
            preview->rotation,
            preview->content_width,
            preview->content_height,
            utf8.constData(),
            static_cast<std::uint32_t>(utf8.size()),
            clip_canvas_text::textStyleFromSceneItem(*preview),
            m_session.activeDraftAutoResize(),
            m_session.arrowId(),
        });
}

clip_canvas_commands::MutationResult
ClipCanvasWidgetTextInteraction::clearActiveDraftPresentation(ClipRuntime runtime,
                                                              ClipViewport viewport) const {
    return clip_canvas_commands::clearActiveTextDraftPresentation(runtime, viewport);
}

void ClipCanvasWidgetTextInteraction::renderEditorOverlay(
    QPainter& painter, const QFont& baseFont, const ClipCanvasDisplayCache& displayCache) {
    m_caretUpdateRegion = m_session.caretRegion(baseFont, displayCache.sceneInfo());
    m_session.renderEditorOverlay(painter, baseFont, displayCache.sceneInfo(), m_caretVisible);
}

ClipCanvasWidgetTextInteraction::StyleChangeResult ClipCanvasWidgetTextInteraction::applyTextStyle(
    ClipRuntime runtime, ClipViewport viewport, ClipCanvasDisplayCache& displayCache,
    const ClipTextStyle& style, std::uint32_t properties) {
    StyleChangeResult result;
    if ((properties & ~CLIP_TEXT_STYLE_ALL_PROPERTIES) != 0) {
        return result;
    }
    if (properties == 0) {
        result.success = true;
        return result;
    }
    if (m_session.isActive()) {
        const ClipTextStyle patched =
            clip_canvas_text::patchedTextStyle(m_session.currentTextStyle(), style, properties);
        const QRegion updateRegion = applyEditorTextStyle(patched, displayCache, m_widget.font());
        clip_canvas_widget_repaint::updateCoalesced(m_widget, updateRegion);
        clip_canvas_commands::MutationResult draftResult =
            publishActiveDraftPresentation(runtime, viewport);
        result.success = draftResult.success;
        result.changedViewports = std::move(draftResult.changedViewports);
        result.toolbarStateChanged = true;
        return result;
    }

    const clip_canvas_text_measurement::TextLayoutOverrideMeasurement layoutOverrides =
        clip_canvas_text_measurement::measureSelectedAutoResizeLayoutOverrides(
            clip_canvas_text_measurement::SelectedTextLayoutMeasurementRequest{
                runtime,
                viewport,
                style,
                m_widget.font(),
                properties,
            });
    if (!layoutOverrides.success) {
        return result;
    }

    clip_canvas_commands::MutationResult mutation = clip_canvas_commands::setTextStyle(
        runtime, viewport, style, properties, layoutOverrides.layouts);
    if (!mutation.success) {
        return result;
    }

    result.success = true;
    result.changedViewports = std::move(mutation.changedViewports);
    return result;
}

QRegion ClipCanvasWidgetTextInteraction::applyEditorTextStyle(
    const ClipTextStyle& style, const ClipCanvasDisplayCache& displayCache, const QFont& baseFont) {
    QRegion region = editingRegion(displayCache, baseFont);
    region += m_session.applyTextStyle(style, baseFont, displayCache.sceneInfo());
    region += editingRegion(displayCache, baseFont);
    region += resetCaretBlink(displayCache, baseFont);
    updateInputMethod();
    return region;
}

ClipCanvasWidgetTextInteraction::BeginResult
ClipCanvasWidgetTextInteraction::beginArrow(ClipRuntime runtime, ClipViewport viewport,
                                            ClipCanvasDisplayCache& displayCache,
                                            const QPointF& viewPosition, bool selected) {
    BeginResult result;
    ClipTextElementInfo info{};
    ClipTextStyle style{};
    std::uint8_t found = 0;
    const QPointF point = viewToCanvasPoint(displayCache, viewPosition);
    if (clip_viewport_get_arrow_text_target(runtime, viewport, selected ? 0 : 1, point.x(),
                                            point.y(), &info, &style, &found) != CLIP_OK ||
        found == 0) {
        return result;
    }
    result.started = beginForElement(
        info, displayCache, canvasToViewPoint(displayCache, QPointF(info.center_x, info.center_y)),
        &style, false, runtime);
    if (!result.started) {
        return result;
    }
    auto selection = clip_canvas_commands::selectElement(runtime, viewport, info.arrow_id);
    result.firstChangedViewports = std::move(selection.changedViewports);
    auto draft = publishActiveDraftPresentation(runtime, viewport);
    result.secondChangedViewports = std::move(draft.changedViewports);
    return result;
}

ClipCanvasWidgetTextInteraction::BeginResult ClipCanvasWidgetTextInteraction::beginAt(
    ClipRuntime runtime, ClipViewport viewport, ClipCanvasDisplayCache& displayCache,
    const QPointF& viewPosition, const ClipTextStyle& newTextStyle, bool allowCreate) {
    BeginResult result;
    if (runtime == nullptr || viewport == nullptr) {
        return result;
    }

    if (allowCreate) {
        BeginResult arrow = beginArrow(runtime, viewport, displayCache, viewPosition);
        if (arrow.started) {
            return arrow;
        }
    }
    const std::optional<ClipTextElementInfo> target =
        clip_canvas_text_edit_target::resolveTextEditTarget(
            runtime, viewport, viewToCanvasPoint(displayCache, viewPosition), m_widget.font(),
            newTextStyle, allowCreate);
    if (!target.has_value()) {
        return result;
    }

    const bool existingText = clip_canvas_element_id::hasElementId(target->id);
    result.started = beginForElement(*target, displayCache, viewPosition,
                                     existingText ? nullptr : &newTextStyle, true, runtime);
    if (!result.started || !existingText) {
        if (result.started) {
            clip_canvas_commands::MutationResult draftResult =
                publishActiveDraftPresentation(runtime, viewport);
            result.firstChangedViewports = std::move(draftResult.changedViewports);
        }
        return result;
    }

    clip_canvas_commands::MutationResult selectResult =
        clip_canvas_commands::selectElement(runtime, viewport, target->id);
    if (selectResult.success) {
        result.firstChangedViewports = std::move(selectResult.changedViewports);
    }
    clip_canvas_commands::MutationResult draftResult =
        publishActiveDraftPresentation(runtime, viewport);
    if (draftResult.success) {
        result.secondChangedViewports = std::move(draftResult.changedViewports);
    }
    return result;
}

ClipCanvasWidgetTextInteraction::BeginResult ClipCanvasWidgetTextInteraction::beginSelectedAt(
    ClipRuntime runtime, ClipViewport viewport, ClipCanvasDisplayCache& displayCache,
    const QPointF& viewPosition, bool requireSerialBoundText) {
    BeginResult result;
    if (runtime == nullptr || viewport == nullptr) {
        return result;
    }

    const std::optional<ClipTextElementInfo> target =
        clip_canvas_text_edit_target::resolveSelectedTextEditTarget(
            runtime, viewport, viewToCanvasPoint(displayCache, viewPosition),
            requireSerialBoundText);
    if (!target.has_value()) {
        return result;
    }

    result.started = beginForElement(*target, displayCache, viewPosition, nullptr, true, runtime);
    if (result.started) {
        clip_canvas_commands::MutationResult draftResult =
            publishActiveDraftPresentation(runtime, viewport);
        result.firstChangedViewports = std::move(draftResult.changedViewports);
    }
    return result;
}

bool ClipCanvasWidgetTextInteraction::beginForElement(const ClipTextElementInfo& info,
                                                      const ClipCanvasDisplayCache& displayCache,
                                                      const QPointF& fallbackViewPosition,
                                                      const ClipTextStyle* newTextStyle,
                                                      bool placeCursorFromViewPosition,
                                                      ClipRuntime runtime) {
    const ClipSceneDisplayItem* existingSceneItem = nullptr;
    if (clip_canvas_element_id::hasElementId(info.id)) {
        existingSceneItem = findTextSceneItem(displayCache, info.id);
    }

    QString completeText;
    if (info.text_truncated != 0 && clip_canvas_element_id::hasElementId(info.id)) {
        if (runtime != nullptr) {
            std::uint32_t length = 0;
            if (clip_runtime_get_text_utf8(runtime, info.id, nullptr, 0, &length) != CLIP_OK) {
                return false;
            }
            QByteArray utf8(static_cast<qsizetype>(length), Qt::Uninitialized);
            if (clip_runtime_get_text_utf8(runtime, info.id,
                                           reinterpret_cast<std::uint8_t*>(utf8.data()), length,
                                           &length) != CLIP_OK) {
                return false;
            }
            completeText = QString::fromUtf8(utf8);
        } else if (existingSceneItem != nullptr) {
            completeText = clip_canvas_text::textFromSceneItem(*existingSceneItem);
        } else {
            return false;
        }
    }
    if (!m_session.begin(info, existingSceneItem, m_widget.font(), newTextStyle,
                         info.text_truncated != 0 ? &completeText : nullptr)) {
        return false;
    }
    const bool attached =
        attachEditor(displayCache, fallbackViewPosition, placeCursorFromViewPosition);
    if (!attached) {
        m_session.cancel();
        setInputMethodEnabled(false);
    }
    return attached;
}

ClipCanvasWidgetTextInteraction::BeginResult ClipCanvasWidgetTextInteraction::beginCreatedText(
    ClipRuntime runtime, ClipViewport viewport,
    const clip_canvas_commands::CreateSerialNumberTextResult& result,
    const ClipCanvasDisplayCache& displayCache) {
    BeginResult beginResult;
    if (!result.success) {
        return beginResult;
    }

    if (!result.hasTextId) {
        beginResult.started = true;
        return beginResult;
    }

    if (runtime == nullptr) {
        return beginResult;
    }

    ClipTextElementInfo textInfo{};
    if (clip_runtime_get_text_element(runtime, result.textId, &textInfo) == CLIP_OK) {
        const bool started = beginForElement(
            textInfo, displayCache,
            canvasToViewPoint(displayCache, QPointF(textInfo.center_x, textInfo.center_y)), nullptr,
            false);
        if (started) {
            clip_canvas_commands::MutationResult draftResult =
                publishActiveDraftPresentation(runtime, viewport);
            beginResult.started = true;
            if (draftResult.success) {
                beginResult.firstChangedViewports = std::move(draftResult.changedViewports);
            }
            return beginResult;
        }
    }
    return beginResult;
}

clip_canvas_commands::CreateSerialNumberTextResult
ClipCanvasWidgetTextInteraction::createSerialNumberText(ClipRuntime runtime, ClipViewport viewport,
                                                        const ClipTextStyle& textStyle) {
    const ClipTextLayoutSize layout =
        clip_canvas_text_measurement::measureSerialNumberBoundTextLayout(textStyle,
                                                                         m_widget.font());
    return clip_canvas_commands::createSerialNumberText(runtime, viewport, layout);
}

ClipCanvasWidgetTextInteraction::BeginResult
ClipCanvasWidgetTextInteraction::beginRequestedTextEdit(
    ClipRuntime runtime, ClipViewport viewport, const ClipCanvasDisplayCache& displayCache) {
    clip_canvas_commands::CreateSerialNumberTextResult request;
    std::uint8_t hasTextId = 0;
    if (clip_viewport_take_text_edit_request(runtime, viewport, &request.textId, &hasTextId) !=
            CLIP_OK ||
        hasTextId == 0) {
        return {};
    }
    request.success = true;
    request.hasTextId = true;
    return beginCreatedText(runtime, viewport, request, displayCache);
}

ClipCanvasWidgetTextInteraction::BeginResult
ClipCanvasWidgetTextInteraction::beginRequestedNewTextDraft(ClipRuntime runtime,
                                                            ClipViewport viewport,
                                                            ClipCanvasDisplayCache& displayCache,
                                                            const QPointF& viewPosition,
                                                            const ClipTextStyle& newTextStyle) {
    BeginResult result;
    if (runtime == nullptr || viewport == nullptr) {
        return result;
    }
    std::uint8_t requested = 0;
    if (clip_viewport_take_new_text_draft_request(runtime, viewport, &requested) != CLIP_OK ||
        requested == 0) {
        return result;
    }
    const ClipTextElementInfo target = clip_canvas_text::newTextInfoAt(
        viewToCanvasPoint(displayCache, viewPosition), m_widget.font(), newTextStyle);
    result.started =
        beginForElement(target, displayCache, viewPosition, &newTextStyle, true, runtime);
    if (result.started) {
        clip_canvas_commands::MutationResult draftResult =
            publishActiveDraftPresentation(runtime, viewport);
        result.firstChangedViewports = std::move(draftResult.changedViewports);
    }
    return result;
}

ClipCanvasWidgetTextInteraction::ActiveResizeMeasurementState
ClipCanvasWidgetTextInteraction::activeResizeMeasurementState(ClipRuntime runtime,
                                                              ClipViewport viewport) const {
    ActiveResizeMeasurementState state;
    const clip_canvas_commands::TextResizeMeasurementResult measurement =
        clip_canvas_commands::activeTextResizeMeasurement(runtime, viewport);
    if (!measurement.success) {
        return state;
    }

    state.success = true;
    state.active = measurement.active;
    return state;
}

ClipCanvasWidgetTextInteraction::ActiveResizeMeasurementResult
ClipCanvasWidgetTextInteraction::applyActiveResizeMeasurementIfNeeded(
    ClipRuntime runtime, ClipViewport viewport, const ClipCanvasDisplayCache& displayCache) {
    ActiveResizeMeasurementResult result;
    const clip_canvas_commands::TextResizeMeasurementResult measurement =
        clip_canvas_commands::activeTextResizeMeasurement(runtime, viewport);
    if (!measurement.success) {
        return result;
    }
    if (!measurement.active) {
        result.success = true;
        return result;
    }

    const ClipTextLayoutSize layout = clip_canvas_text_measurement::measureResizeLayout(
        clip_canvas_text_measurement::ResizeLayoutMeasurementRequest{
            measurement.info,
            m_widget.font(),
            displayCache.sceneInfo().camera_zoom,
        });
    clip_canvas_commands::MutationResult mutation =
        clip_canvas_commands::applyActiveTextResizeMeasurement(runtime, viewport, layout);
    if (!mutation.success) {
        return result;
    }

    result.success = true;
    result.active = true;
    result.changedViewports = std::move(mutation.changedViewports);
    return result;
}

ClipCanvasWidgetTextInteraction::CommitResult
ClipCanvasWidgetTextInteraction::commit(ClipRuntime runtime, ClipViewport viewport,
                                        bool hasViewport, ClipCanvasDisplayCache& displayCache,
                                        bool refocusWidget, bool deleteElement) {
    CommitResult commitResult;
    if (!m_session.isActive()) {
        return commitResult;
    }

    if (!deleteElement) {
        commitInputMethod();
    }
    QRegion updateRegion = editingRegion(displayCache, m_widget.font());
    ClipCanvasTextEditorSession::FinishedEdit edit = m_session.finish(m_widget.font());
    if (deleteElement) {
        edit.text.clear();
    }
    setInputMethodEnabled(false);
    stopCaretBlink();
    m_selectionDragging = false;
    commitResult.sessionEnded = true;
    commitResult.finishedExistingEdit.elementId =
        clip_canvas_element_id::hasElementId(edit.arrowId) ? edit.arrowId : edit.elementId;
    commitResult.finishedExistingEdit.hasElement =
        edit.hasExistingElement || clip_canvas_element_id::hasElementId(edit.arrowId);

    if (edit.shouldCommit(hasViewport)) {
        const QByteArray utf8 = edit.text.toUtf8();
        clip_canvas_commands::MutationResult result =
            clip_canvas_commands::commitText(runtime, viewport,
                                             clip_canvas_commands::CommitTextRequest{
                                                 edit.elementId,
                                                 edit.hasExistingElement,
                                                 edit.canvasCenter.x(),
                                                 edit.canvasCenter.y(),
                                                 utf8.constData(),
                                                 static_cast<std::uint32_t>(utf8.size()),
                                                 edit.measuredLayout,
                                                 edit.style,
                                                 edit.autoResize,
                                                 edit.styleChanged,
                                                 edit.arrowId,
                                             });
        if (result.success) {
            commitResult.changedViewports = std::move(result.changedViewports);
        } else {
            clip_canvas_commands::MutationResult clearResult =
                clearActiveDraftPresentation(runtime, viewport);
            if (clearResult.success) {
                commitResult.changedViewports = std::move(clearResult.changedViewports);
            }
        }
        updateRegion = committedTextRegion(updateRegion, displayCache, m_widget.rect());
    } else {
        clip_canvas_commands::MutationResult clearResult =
            clearActiveDraftPresentation(runtime, viewport);
        if (clearResult.success) {
            commitResult.changedViewports = std::move(clearResult.changedViewports);
        }
    }

    resetInputMethod();
    if (refocusWidget) {
        m_widget.setFocus(Qt::OtherFocusReason);
    }
    clip_canvas_widget_repaint::updateCoalesced(m_widget, updateRegion);
    return commitResult;
}

ClipCanvasWidgetTextInteraction::SelectionRestoreResult
ClipCanvasWidgetTextInteraction::restoreFinishedExistingSelection(
    ClipRuntime runtime, ClipViewport viewport, const FinishedExistingEdit& edit) {
    SelectionRestoreResult result;
    if (!edit.hasElement) {
        return result;
    }

    clip_canvas_commands::MutationResult selectResult =
        clip_canvas_commands::selectElement(runtime, viewport, edit.elementId);
    if (!selectResult.success) {
        return result;
    }

    result.restored = true;
    result.changedViewports = std::move(selectResult.changedViewports);
    return result;
}

ClipCanvasWidgetTextInteraction::CancelResult
ClipCanvasWidgetTextInteraction::cancel(ClipRuntime runtime, ClipViewport viewport,
                                        const ClipCanvasDisplayCache& displayCache) {
    CancelResult result;
    if (!m_session.isActive()) {
        return result;
    }

    const QRegion updateRegion = editingRegion(displayCache, m_widget.font());
    m_session.cancel();
    setInputMethodEnabled(false);
    stopCaretBlink();
    m_selectionDragging = false;
    clip_canvas_commands::MutationResult clearResult =
        clearActiveDraftPresentation(runtime, viewport);
    result.changedViewports = std::move(clearResult.changedViewports);
    result.sessionEnded = true;
    resetInputMethod();
    m_widget.setFocus(Qt::OtherFocusReason);
    clip_canvas_widget_repaint::updateCoalesced(m_widget, updateRegion);
    return result;
}

bool ClipCanvasWidgetTextInteraction::handleEditorMousePress(
    QMouseEvent* event, const ClipCanvasDisplayCache& displayCache, const QFont& baseFont) {
    m_selectionDragging = false;
    if (event == nullptr || !m_session.isActive() || event->button() != Qt::LeftButton) {
        return false;
    }
    if (!editorContains(displayCache, event->position())) {
        return false;
    }

    m_widget.setFocus(Qt::MouseFocusReason);
    m_selectionDragging = true;
    setCursorFromViewPosition(displayCache, baseFont, event->position(),
                              (event->modifiers() & Qt::ShiftModifier) != 0);
    event->accept();
    return true;
}

bool ClipCanvasWidgetTextInteraction::handleEditorMouseMove(
    QMouseEvent* event, const ClipCanvasDisplayCache& displayCache, const QFont& baseFont) {
    if (event == nullptr || !m_session.isActive()) {
        return false;
    }

    if (!m_selectionDragging) {
        if (event->buttons() != Qt::NoButton) {
            return false;
        }

        if (editorContains(displayCache, event->position())) {
            m_cursorController.setCursor(ClipCanvasCursorLayer::CanvasTool,
                                         QCursor(Qt::IBeamCursor));
            event->accept();
            return true;
        }
        return false;
    }

    setCursorFromViewPosition(displayCache, baseFont, event->position(), true);
    event->accept();
    return true;
}

bool ClipCanvasWidgetTextInteraction::handleEditorMouseRelease(QMouseEvent* event) {
    if (event == nullptr || !m_selectionDragging || event->button() != Qt::LeftButton) {
        return false;
    }

    m_selectionDragging = false;
    event->accept();
    return true;
}

ClipCanvasWidgetTextInteraction::EditorEventResult
ClipCanvasWidgetTextInteraction::handleKeyPress(QKeyEvent* event, ClipRuntime runtime,
                                                ClipViewport viewport, bool hasViewport,
                                                ClipCanvasDisplayCache& displayCache) {
    EditorEventResult result;
    if (!m_session.isActive()) {
        return result;
    }

    const QRegion beforeRegion = editingRegion(displayCache, m_widget.font());
    const ClipCanvasTextEditorSession::KeyResult keyResult =
        m_session.handleKeyPress(event, m_widget.font(), displayCache.sceneInfo());
    if (!keyResult.handled) {
        return result;
    }

    result.handled = true;
    result.consume = true;
    if (event != nullptr) {
        event->accept();
    }
    switch (keyResult.command) {
    case ClipCanvasTextEditorSession::EventCommand::DeleteElement:
    case ClipCanvasTextEditorSession::EventCommand::Commit: {
        CommitResult commitResult =
            commit(runtime, viewport, hasViewport, displayCache, true,
                   keyResult.command == ClipCanvasTextEditorSession::EventCommand::DeleteElement);
        result.finishedExistingEdit = commitResult.finishedExistingEdit;
        result.changedViewports = std::move(commitResult.changedViewports);
        result.sessionEnded = commitResult.sessionEnded;
        return result;
    }
    case ClipCanvasTextEditorSession::EventCommand::None:
    default:
        break;
    }

    const QRegion caretResetRegion = resetCaretBlink(displayCache, m_widget.font());
    if (keyResult.changed) {
        clip_canvas_commands::MutationResult draftResult =
            publishActiveDraftPresentation(runtime, viewport);
        result.changedViewports = std::move(draftResult.changedViewports);
        QRegion updateRegion = beforeRegion;
        updateRegion += editingRegion(displayCache, m_widget.font());
        updateRegion += caretResetRegion;
        clip_canvas_widget_repaint::updateCoalesced(m_widget, updateRegion);
        updateInputMethod();
    } else {
        clip_canvas_widget_repaint::updateClipped(m_widget, caretResetRegion);
    }
    return result;
}

bool ClipCanvasWidgetTextInteraction::handleKeyRelease(QKeyEvent* event) {
    if (!m_session.isActive()) {
        return false;
    }
    if (event != nullptr) {
        event->accept();
    }
    return true;
}

ClipCanvasWidgetTextInteraction::InputMethodEventResult
ClipCanvasWidgetTextInteraction::handleInputMethodEvent(QInputMethodEvent* event,
                                                        ClipRuntime runtime, ClipViewport viewport,
                                                        const ClipCanvasDisplayCache& displayCache,
                                                        const QFont& baseFont) {
    InputMethodEventResult result;
    if (!m_session.isActive()) {
        return result;
    }

    QRegion updateRegion = editingRegion(displayCache, baseFont);
    const bool changed = m_session.handleInputMethodEvent(event, baseFont);
    updateRegion += editingRegion(displayCache, baseFont);
    const QRegion caretResetRegion = resetCaretBlink(displayCache, baseFont);
    if (changed) {
        clip_canvas_commands::MutationResult draftResult =
            publishActiveDraftPresentation(runtime, viewport);
        result.changedViewports = std::move(draftResult.changedViewports);
        updateRegion += caretResetRegion;
        clip_canvas_widget_repaint::updateCoalesced(m_widget, updateRegion);
        updateInputMethod();
    } else {
        clip_canvas_widget_repaint::updateClipped(m_widget, caretResetRegion);
    }
    if (event != nullptr) {
        event->accept();
    }
    result.handled = true;
    return result;
}

QVariant
ClipCanvasWidgetTextInteraction::inputMethodQuery(Qt::InputMethodQuery query,
                                                  const ClipCanvasDisplayCache& displayCache,
                                                  const QFont& baseFont) const {
    if (!m_session.isActive()) {
        return {};
    }
    return m_session.inputMethodQuery(query, baseFont, displayCache.sceneInfo());
}

QRegion ClipCanvasWidgetTextInteraction::setCursorFromViewPosition(
    const ClipCanvasDisplayCache& displayCache, const QFont& baseFont, const QPointF& viewPosition,
    bool keepSelection) {
    if (!m_session.isActive()) {
        return {};
    }

    QRegion updateRegion = editingRegion(displayCache, baseFont);
    const bool changed = m_session.setCursorFromViewPosition(baseFont, displayCache.sceneInfo(),
                                                             viewPosition, keepSelection);
    const QRegion caretResetRegion = resetCaretBlink(displayCache, baseFont);
    if (!changed && caretResetRegion.isEmpty()) {
        return {};
    }
    if (changed) {
        updateRegion += editingRegion(displayCache, baseFont);
    } else {
        updateRegion = {};
    }
    updateRegion += caretResetRegion;
    clip_canvas_widget_repaint::updateCoalesced(m_widget, updateRegion);
    updateInputMethod();
    return updateRegion;
}

QRegion ClipCanvasWidgetTextInteraction::editingRegion(const ClipCanvasDisplayCache& displayCache,
                                                       const QFont& baseFont) const {
    QRegion region = m_session.editingRegion(baseFont, displayCache.sceneInfo());
    region += clip_canvas_text_editor_connector::connectorRegion(
        displayCache.sceneInfo(), displayCache.sceneItems(), displayCache.sceneItemCount(),
        m_session.previewItem());
    return region;
}

const ClipSceneDisplayItem*
ClipCanvasWidgetTextInteraction::findTextSceneItem(const ClipCanvasDisplayCache& displayCache,
                                                   ClipElementId id) const {
    if (!clip_canvas_element_id::hasElementId(id)) {
        return nullptr;
    }
    const ClipCanvasSceneItem* sceneItems = displayCache.sceneItems();
    const std::uint32_t sceneItemCount = displayCache.sceneItemCount();
    for (std::uint32_t index = 0; index < sceneItemCount; ++index) {
        const ClipSceneDisplayItem& item = sceneItems[index];
        if (item.kind == CLIP_SCENE_DISPLAY_ITEM_TEXT &&
            clip_canvas_element_id::sameElementId(item.element_id, id)) {
            return &item;
        }
    }
    return nullptr;
}

QPointF
ClipCanvasWidgetTextInteraction::viewToCanvasPoint(const ClipCanvasDisplayCache& displayCache,
                                                   const QPointF& viewPosition) const {
    const SceneDisplayInfo& sceneInfo = displayCache.sceneInfo();
    return clip_canvas_render_geometry::viewToCanvas(
        sceneInfo.camera_center_x, sceneInfo.camera_center_y, sceneInfo.camera_zoom,
        sceneInfo.surface_width, sceneInfo.surface_height, viewPosition);
}

QPointF
ClipCanvasWidgetTextInteraction::canvasToViewPoint(const ClipCanvasDisplayCache& displayCache,
                                                   const QPointF& canvasPosition) const {
    const SceneDisplayInfo& sceneInfo = displayCache.sceneInfo();
    return clip_canvas_render_geometry::canvasToView(
        sceneInfo.camera_center_x, sceneInfo.camera_center_y, sceneInfo.camera_zoom,
        sceneInfo.surface_width, sceneInfo.surface_height, canvasPosition.x(), canvasPosition.y());
}

bool ClipCanvasWidgetTextInteraction::attachEditor(const ClipCanvasDisplayCache& displayCache,
                                                   const QPointF& fallbackViewPosition,
                                                   bool placeCursorFromViewPosition) {
    if (!m_session.isActive()) {
        return false;
    }

    setInputMethodEnabled(true);
    resetInputMethod();
    QRegion updateRegion = editingRegion(displayCache, m_widget.font());
    m_session.updatePreviewFromState(m_widget.font());
    updateRegion += editingRegion(displayCache, m_widget.font());
    if (placeCursorFromViewPosition) {
        if (m_session.setCursorFromViewPosition(m_widget.font(), displayCache.sceneInfo(),
                                                fallbackViewPosition)) {
            updateRegion += editingRegion(displayCache, m_widget.font());
        }
    }
    updateRegion += resetCaretBlink(displayCache, m_widget.font());

    m_widget.setFocus(Qt::MouseFocusReason);
    clip_canvas_widget_repaint::updateCoalesced(m_widget, updateRegion);
    updateInputMethod();
    return true;
}

void ClipCanvasWidgetTextInteraction::setInputMethodEnabled(bool enabled) {
    if (m_widget.testAttribute(Qt::WA_InputMethodEnabled) == enabled) {
        return;
    }

    m_widget.setAttribute(Qt::WA_InputMethodEnabled, enabled);
    if (m_widget.hasFocus()) {
        updateInputMethod();
    }
}

QRegion ClipCanvasWidgetTextInteraction::resetCaretBlink(const ClipCanvasDisplayCache& displayCache,
                                                         const QFont& baseFont) {
    const QRegion previousCaretRegion = m_caretUpdateRegion;
    m_caretUpdateRegion = m_session.caretRegion(baseFont, displayCache.sceneInfo());

    QRegion repaintRegion;
    if (!m_caretVisible) {
        repaintRegion = previousCaretRegion + m_caretUpdateRegion;
    }
    m_caretVisible = true;
    if (m_caretFlashTimeMs > 0 && m_session.isActive()) {
        m_caretBlinkTimer.start(caretBlinkPhaseDuration(m_caretFlashTimeMs));
    } else {
        m_caretBlinkTimer.stop();
    }
    return repaintRegion;
}

void ClipCanvasWidgetTextInteraction::setCaretFlashTime(int flashTimeMs) {
    m_caretFlashTimeMs = qMax(0, flashTimeMs);
    if (!m_session.isActive()) {
        m_caretBlinkTimer.stop();
        m_caretVisible = true;
        return;
    }

    const bool needsRepaint = !m_caretVisible;
    m_caretVisible = true;
    if (m_caretFlashTimeMs > 0) {
        m_caretBlinkTimer.start(caretBlinkPhaseDuration(m_caretFlashTimeMs));
    } else {
        m_caretBlinkTimer.stop();
    }
    if (needsRepaint) {
        clip_canvas_widget_repaint::updateClipped(m_widget, m_caretUpdateRegion);
    }
}

void ClipCanvasWidgetTextInteraction::stopCaretBlink() {
    m_caretBlinkTimer.stop();
    m_caretUpdateRegion = {};
    m_caretVisible = true;
}

void ClipCanvasWidgetTextInteraction::handleCaretBlinkTimeout() {
    if (!m_session.isActive() || m_caretFlashTimeMs <= 0) {
        stopCaretBlink();
        return;
    }

    m_caretVisible = !m_caretVisible;
    clip_canvas_widget_repaint::updateClipped(m_widget, m_caretUpdateRegion);
}
