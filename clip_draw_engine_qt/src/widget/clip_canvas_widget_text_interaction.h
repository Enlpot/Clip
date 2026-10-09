#pragma once

#include "clip_canvas_commands.h"
#include "clip_canvas_display_cache.h"
#include "clip_canvas_ffi_handles.h"
#include "clip_canvas_text_editor_session.h"
#include "clip_canvas_text_measurement.h"
#include "clip_draw_engine.h"
#include "clip_draw_engine_qt/clip_canvas_types.h"

#include <QPoint>
#include <QPointF>
#include <QRegion>
#include <QTimer>
#include <QVariant>

class QFont;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QWidget;
class ClipCanvasCursorController;

class ClipCanvasWidgetTextInteraction final {
  public:
    struct FinishedExistingEdit {
        ClipElementId elementId{};
        bool hasElement = false;
    };
    struct EditorEventResult {
        bool handled = false;
        bool consume = false;
        ScopedChangedViewportList changedViewports;
        FinishedExistingEdit finishedExistingEdit;
        bool sessionEnded = false;
    };
    struct CommitResult {
        ScopedChangedViewportList changedViewports;
        FinishedExistingEdit finishedExistingEdit;
        bool restoredExistingSelection = false;
        bool sessionEnded = false;
    };
    struct CancelResult {
        ScopedChangedViewportList changedViewports;
        bool sessionEnded = false;
    };
    struct InputMethodEventResult {
        bool handled = false;
        ScopedChangedViewportList changedViewports;
    };
    struct SelectionRestoreResult {
        bool restored = false;
        ScopedChangedViewportList changedViewports;
    };
    struct BeginResult {
        bool started = false;
        ScopedChangedViewportList firstChangedViewports;
        ScopedChangedViewportList secondChangedViewports;
    };
    struct StyleChangeResult {
        bool success = false;
        bool toolbarStateChanged = false;
        ScopedChangedViewportList changedViewports;
    };
    struct ActiveResizeMeasurementState {
        bool success = false;
        bool active = false;
    };
    struct ActiveResizeMeasurementResult {
        bool success = false;
        bool active = false;
        ScopedChangedViewportList changedViewports;
    };

    ClipCanvasWidgetTextInteraction(QWidget& widget, ClipCanvasCursorController& cursorController);
    void invalidateArrowTextMetrics();
    void resetDocumentRetainedState();

    ClipCanvasTextEditorSession& session();
    const ClipCanvasTextEditorSession& session() const;

    clip_canvas_commands::MutationResult measureArrowText(ClipRuntime runtime,
                                                          ClipViewport viewport);
    clip_canvas_commands::MutationResult measureSerialLabelLayout(ClipRuntime runtime,
                                                                  ClipViewport viewport);
    bool isActive() const;
    bool editorContains(const ClipCanvasDisplayCache& displayCache, const QPointF& position) const;
    bool hasSelectionInteraction() const;
    const ClipSceneDisplayItem* previewItem() const;
    ClipTextStyle currentTextStyle() const;
    void renderEditorOverlay(QPainter& painter, const QFont& baseFont,
                             const ClipCanvasDisplayCache& displayCache);
    StyleChangeResult applyTextStyle(ClipRuntime runtime, ClipViewport viewport,
                                     ClipCanvasDisplayCache& displayCache,
                                     const ClipTextStyle& style, std::uint32_t properties);

    BeginResult beginArrow(ClipRuntime runtime, ClipViewport viewport,
                           ClipCanvasDisplayCache& displayCache, const QPointF& viewPosition,
                           bool selected = false);
    BeginResult beginAt(ClipRuntime runtime, ClipViewport viewport,
                        ClipCanvasDisplayCache& displayCache, const QPointF& viewPosition,
                        const ClipTextStyle& newTextStyle, bool allowCreate = true);
    BeginResult beginSelectedAt(ClipRuntime runtime, ClipViewport viewport,
                                ClipCanvasDisplayCache& displayCache, const QPointF& viewPosition,
                                bool requireSerialBoundText = false);
    bool beginForElement(const ClipTextElementInfo& info,
                         const ClipCanvasDisplayCache& displayCache,
                         const QPointF& fallbackViewPosition,
                         const ClipTextStyle* newTextStyle = nullptr,
                         bool placeCursorFromViewPosition = true, ClipRuntime runtime = nullptr);
    // Creates the bound label only. The caller must sync the returned changed
    // viewports into its display cache before beginning the editor: the editor
    // session needs the label's styled scene item, or its commit strips the
    // styling the label was created with.
    clip_canvas_commands::CreateSerialNumberTextResult
    createSerialNumberText(ClipRuntime runtime, ClipViewport viewport,
                           const ClipTextStyle& textStyle);
    BeginResult beginRequestedTextEdit(ClipRuntime runtime, ClipViewport viewport,
                                       const ClipCanvasDisplayCache& displayCache);
    // Starts a blank-canvas draft only after the engine authorized it by
    // `clip_viewport_take_new_text_draft_request` during this press.
    BeginResult beginRequestedNewTextDraft(ClipRuntime runtime, ClipViewport viewport,
                                           ClipCanvasDisplayCache& displayCache,
                                           const QPointF& viewPosition,
                                           const ClipTextStyle& newTextStyle);
    // Begins editing a freshly created bound label; requires the creation's
    // changed viewports to have been synced so the styled scene item exists.
    BeginResult beginCreatedText(ClipRuntime runtime, ClipViewport viewport,
                                 const clip_canvas_commands::CreateSerialNumberTextResult& result,
                                 const ClipCanvasDisplayCache& displayCache);
    ActiveResizeMeasurementState activeResizeMeasurementState(ClipRuntime runtime,
                                                              ClipViewport viewport) const;
    ActiveResizeMeasurementResult
    applyActiveResizeMeasurementIfNeeded(ClipRuntime runtime, ClipViewport viewport,
                                         const ClipCanvasDisplayCache& displayCache);

    CommitResult commit(ClipRuntime runtime, ClipViewport viewport, bool hasViewport,
                        ClipCanvasDisplayCache& displayCache, bool refocusWidget = true,
                        bool deleteElement = false);
    SelectionRestoreResult restoreFinishedExistingSelection(ClipRuntime runtime,
                                                            ClipViewport viewport,
                                                            const FinishedExistingEdit& edit);
    CancelResult cancel(ClipRuntime runtime, ClipViewport viewport,
                        const ClipCanvasDisplayCache& displayCache);

    bool handleEditorMousePress(QMouseEvent* event, const ClipCanvasDisplayCache& displayCache,
                                const QFont& baseFont);
    bool handleEditorMouseMove(QMouseEvent* event, const ClipCanvasDisplayCache& displayCache,
                               const QFont& baseFont);
    bool handleEditorMouseRelease(QMouseEvent* event);
    EditorEventResult handleKeyPress(QKeyEvent* event, ClipRuntime runtime, ClipViewport viewport,
                                     bool hasViewport, ClipCanvasDisplayCache& displayCache);
    bool handleKeyRelease(QKeyEvent* event);
    InputMethodEventResult handleInputMethodEvent(QInputMethodEvent* event, ClipRuntime runtime,
                                                  ClipViewport viewport,
                                                  const ClipCanvasDisplayCache& displayCache,
                                                  const QFont& baseFont);
    QVariant inputMethodQuery(Qt::InputMethodQuery query,
                              const ClipCanvasDisplayCache& displayCache,
                              const QFont& baseFont) const;
    QRegion setCursorFromViewPosition(const ClipCanvasDisplayCache& displayCache,
                                      const QFont& baseFont, const QPointF& viewPosition,
                                      bool keepSelection);

  private:
    QRegion applyEditorTextStyle(const ClipTextStyle& style,
                                 const ClipCanvasDisplayCache& displayCache, const QFont& baseFont);
    QRegion editingRegion(const ClipCanvasDisplayCache& displayCache, const QFont& baseFont) const;
    const ClipSceneDisplayItem* findTextSceneItem(const ClipCanvasDisplayCache& displayCache,
                                                  ClipElementId id) const;
    QPointF viewToCanvasPoint(const ClipCanvasDisplayCache& displayCache,
                              const QPointF& viewPosition) const;
    QPointF canvasToViewPoint(const ClipCanvasDisplayCache& displayCache,
                              const QPointF& canvasPosition) const;
    bool attachEditor(const ClipCanvasDisplayCache& displayCache,
                      const QPointF& fallbackViewPosition, bool placeCursorFromViewPosition);
    void setInputMethodEnabled(bool enabled);
    clip_canvas_commands::MutationResult
    publishActiveDraftPresentation(ClipRuntime runtime, ClipViewport viewport) const;
    clip_canvas_commands::MutationResult clearActiveDraftPresentation(ClipRuntime runtime,
                                                                      ClipViewport viewport) const;
    QRegion resetCaretBlink(const ClipCanvasDisplayCache& displayCache, const QFont& baseFont);
    void setCaretFlashTime(int flashTimeMs);
    void stopCaretBlink();
    void handleCaretBlinkTimeout();

    QWidget& m_widget;
    ClipCanvasCursorController& m_cursorController;
    ClipCanvasTextEditorSession m_session;
    clip_canvas_text_measurement::NaturalTextLayoutCache m_arrowNaturalLayouts;
    QFont m_arrowLayoutFont;
    bool m_arrowMetricsInvalid = true;
    QTimer m_caretBlinkTimer;
    QRegion m_caretUpdateRegion;
    int m_caretFlashTimeMs = 0;
    bool m_caretVisible = true;
    bool m_selectionDragging = false;
};
