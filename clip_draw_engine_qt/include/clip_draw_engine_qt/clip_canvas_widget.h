#pragma once
#include "clip_draw_engine_qt/clip_canvas_style_edit.h"
#include "clip_draw_engine_qt/clip_canvas_smart_erase.h"

#include <QRect>
#include <QByteArray>
#include <QRectF>
#include <QTransform>
#include <QVariant>
#include <QWidget>

#include <cstdint>
#include <memory>
#include <functional>
#include <optional>

#include "clip_draw_engine_qt/clip_canvas_types.h"

class QCursor;
class QRegion;
class QEnterEvent;
class QEvent;
class QFocusEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPointF;
class QResizeEvent;
class QWheelEvent;
class ClipCanvasRuntime;
class ClipCanvasCustomRenderer;

struct ClipCanvasDecorationRenderAreas {
    std::optional<QRectF> watermark;
    std::optional<QRectF> spotlight;
};

class ClipCanvasWidget : public QWidget {
    Q_OBJECT

  public:
    explicit ClipCanvasWidget(QWidget* parent = nullptr);
    explicit ClipCanvasWidget(ClipCanvasRuntime& runtime, QWidget* parent = nullptr);
    ~ClipCanvasWidget() override;

    // Resolve command keys without changing text/IME input. An empty resolver
    // preserves the library's default Qt key semantics.
    using CommandKeyResolver = std::function<Qt::Key(const QKeyEvent&)>;
    void setCommandKeyResolver(CommandKeyResolver resolver);

    ClipCanvasTool canvasTool() const;
    bool setCanvasTool(ClipCanvasTool tool);

    void setCursorForLayer(ClipCanvasCursorLayer layer, const QCursor& cursor);
    void clearCursorForLayer(ClipCanvasCursorLayer layer);

    // Apply a property patch without recording a user preference (restoration/replication).
    bool applyStyleEdit(const ClipCanvasStyleEdit& edit);
    // Apply explicit user intent and publish it only on success.
    bool commitStyleEdit(const ClipCanvasStyleEdit& edit);
    bool stepFontSize(int direction);
    ClipCanvasStyleToolbarState canvasStyleToolbarState() const;
    ClipCanvasSerialNumberToolbarState serialNumberToolbarState() const;
    ClipCanvasWatermarkConfig canvasWatermarkConfig() const;
    bool setCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config);
    void previewCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config);
    ClipCanvasSpotlightConfig canvasSpotlightConfig() const;
    bool setCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config);
    void previewCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config);
    ClipCanvasDistanceStyle canvasDistanceStyle() const;
    bool setCanvasDistanceStyle(const ClipCanvasDistanceStyle& style,
                                quint32 properties = ClipCanvasDistanceStyleAllProperties);
    // Source-image pixels per canvas unit, captured when a distance draft begins.
    bool setDistanceCreationPixelScale(const QSizeF& scale);
    bool setCanvasShapeStylePatch(const ClipCanvasShapeStyle& style, quint32 properties,
                                  ClipCanvasShapeKind kind);
    bool setCanvasFilterStyle(const ClipCanvasFilterStyle& style, quint32 properties);
    // Updates shared creation defaults for RectangleFilter or PenFilter without
    // changing this widget's active tool, selection, or existing elements.
    // Strength remains shared by both filter families.
    bool setCanvasFilterCreationStyle(const ClipCanvasFilterStyle& style, quint32 properties,
                                      ClipCanvasTool filterTool);
    // Changes only future brush erasers, preserving selection, tool and history.
    bool setCanvasBrushEraserCreationStyle(
        const ClipCanvasBrushEraserStyle& style,
        quint32 properties = ClipCanvasBrushEraserStylePropertyStrokeWidth);
    bool setCanvasTextStyle(const ClipCanvasTextStyle& style,
                            quint32 properties = ClipCanvasTextStyleAllProperties);
    bool setCanvasSerialNumberStyle(const ClipCanvasSerialNumberStyle& style);

    ClipCanvasHistoryState canvasHistoryState() const;
    quint64 autoFilterGeneration() const;
    std::optional<ClipCanvasAutoFilterRecord> autoFilterRegions() const;
    bool setAutoFilterRegions(const std::optional<ClipCanvasAutoFilterRecord>& record);
    bool fillAutoFilterCategory(const QString& category);

    ClipCanvasSnapConfig canvasSnapConfig() const;
    bool setCanvasSnapConfig(const ClipCanvasSnapConfig& config);
    bool setCanvasSnapGuideTargets(const ClipCanvasSnapGuideTargets& targets);

    ClipCanvasGridConfig canvasGridConfig() const;
    bool setCanvasGridConfig(const ClipCanvasGridConfig& config);

    bool undo();
    bool redo();
    bool deleteSelected();
    // Deletes every document element as one undoable history entry, preserving viewports,
    // creation styles, history, and document-wide configuration.
    bool deleteAllElements();
    // Clears all document elements and history, preserving viewports and creation styles.
    bool clearDocument();
    bool duplicateSelected(const QPointF& offset = QPointF(12.0, 12.0));
    bool insertDrawTemplate(const QByteArray& payload, const QPointF& center);
    bool reorderSelected(ClipCanvasSelectionOrder order);
    // Aligns or distributes the selected elements to their shared bounds as one
    // undoable history entry. Requires at least two selected elements for the
    // align modes and three for the distribute modes.
    bool alignSelected(ClipCanvasSelectionAlignment alignment);
    bool setSelectedOpacity(double opacity);
    bool adjustSelectedSerialNumbers(qint64 delta);
    bool createSerialNumberText();
    // Opens the label of one selected arrow, or starts an uncommitted attached draft.
    bool editSelectedArrowText();
    // Commits active text, clears transient editing state and selection, and
    // restores the select tool.
    bool resetEditingState();
    // Commits active text and clears transient editing state and selection while
    // preserving the currently active canvas tool.
    bool resetEditingStatePreservingTool();
    // Discards an uncommitted inline text draft without adding it to history.
    bool cancelActiveTextEditing();
    // Releases renderer cache and scratch memory without changing document, view, styles, or
    // selection.
    void clearRenderState();
    [[nodiscard]] bool hasActiveTextEditing() const;
    // Keeps an active inline text draft alive while a text-style popup owns focus.
    void beginTextStylePopupInteraction();
    // Ends a text-style popup interaction and restores text input when appropriate.
    void endTextStylePopupInteraction(QWidget* focusScope);

    bool interactionEnabled() const;
    void setInteractionEnabled(bool enabled);
    // Tests element bodies using the active tool and the requested button's policy.
    [[nodiscard]] bool hasQuickSelectionTargetAt(const QPointF& viewPosition,
                                                 Qt::MouseButton button) const;
    bool wheelZoomEnabled() const;
    void setWheelZoomEnabled(bool enabled);
    // Controls engine-owned scene, overlay, editor, and auxiliary content.
    // Custom renderer passes and background clearing remain active.
    [[nodiscard]] bool canvasContentVisible() const;
    void setCanvasContentVisible(bool visible);
    bool clearBackgroundEnabled() const;
    void setClearBackgroundEnabled(bool enabled);

    bool showDirtyRects() const;
    std::uint64_t viewportId() const;

    bool setViewportCamera(double centerX, double centerY, double zoom);
    // Limits the viewport-anchored watermark to a canvas-space area. An empty
    // area renders no watermark; clearWatermarkRenderArea() restores the full
    // viewport behavior.
    bool hasWatermarkRenderArea() const;
    QRectF watermarkRenderArea() const;
    void setWatermarkRenderArea(const QRectF& canvasRect);
    void clearWatermarkRenderArea();
    void setDecorationRenderAreas(const ClipCanvasDecorationRenderAreas& areas);
    bool hasSpotlightRenderArea() const;
    QRectF spotlightRenderArea() const;
    void setSpotlightRenderArea(const QRectF& canvasRect);
    void clearSpotlightRenderArea();
    // The renderer is borrowed and must be detached before it is destroyed.
    ClipCanvasCustomRenderer* customRenderer() const;
    void setCustomRenderer(ClipCanvasCustomRenderer* renderer);
    void setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources);
    void setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources,
                             const QRegion& damage);
    [[nodiscard]] QTransform canvasToViewTransform() const;
    QRect viewRectForCanvasRect(const QRectF& canvasRect, int paddingPx = 0) const;

  public slots:
    void setShowDirtyRects(bool show);

  signals:
    void autoFilterRegionsChanged();
    void autoFilterInteractionStarting();
    void activeToolChanged();
    void styleToolbarStateChanged();
    void styleEditCommitted(const ClipCanvasStyleEdit& edit);
    void historyStateChanged();
    void snapConfigChanged();
    void gridConfigChanged();
    void watermarkPreviewApplied();
    void spotlightPreviewApplied();
    void freeDrawMoveBatchProcessed(quint32 inputCount, quint32 dispatchedCount);
    void eraserMoveFrameProcessed();
    void unhandledLeftDoubleClick();
    void unhandledMiddleClick();
    void showDirtyRectsChanged();

  protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void focusOutEvent(QFocusEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
