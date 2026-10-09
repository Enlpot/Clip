#include "clip_draw_engine_qt/clip_canvas_widget.h"
#include "icons/draw_engine_icons.h"
#include "icon_renderer.h"

#include "clip_canvas_commands.h"
#include "clip_canvas_cursor_controller.h"
#include "clip_canvas_changed_viewports.h"
#include "clip_canvas_compositor.h"
#include "clip_canvas_display_cache.h"
#include "clip_canvas_event_flow.h"
#include "clip_canvas_input_adapter.h"
#include "clip_canvas_lifecycle.h"
#include "clip_canvas_pen_mask_atlas.h"
#include "clip_canvas_render_geometry.h"
#include "clip_canvas_reference_scene.h"
#include "clip_canvas_state.h"
#include "clip_canvas_text_editor_input.h"
#include "clip_canvas_text_measurement.h"
#include "clip_canvas_filter_tile_cache.h"
#include "clip_canvas_smart_erase.h"
#include "clip_canvas_type_conversions.h"
#include "clip_canvas_widget_display_state.h"
#include "clip_canvas_widget_input_handler.h"
#include "clip_canvas_widget_keyboard_flow.h"
#include "clip_canvas_widget_paint_frame.h"
#include "clip_canvas_widget_pointer_flow.h"
#include "clip_canvas_widget_repaint.h"
#include "clip_canvas_widget_runtime_binding.h"
#include "clip_canvas_widget_selection_hit_testing.h"
#include "clip_canvas_widget_sync.h"
#include "clip_canvas_widget_text_interaction.h"
#include "clip_draw_engine_qt/clip_canvas_custom_renderer.h"
#include "clip_draw_engine_qt/clip_canvas_runtime.h"

#include <QBitmap>
#include <QByteArray>
#include <QApplication>
#include <QCursor>
#include <QContextMenuEvent>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputMethodEvent>
#include <QImage>
#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPointer>
#include <QPixmap>
#include <QResizeEvent>
#include <QScreen>
#include <QScopedValueRollback>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>
#include <QWindow>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {

std::optional<ClipCursorStyle> baselineCursorForCanvasTool(ClipCanvasTool tool) {
    switch (tool) {
    case ClipCanvasTool::Shape:
    case ClipCanvasTool::Arrow:
    case ClipCanvasTool::Distance:
    case ClipCanvasTool::Line:
    case ClipCanvasTool::RectangleHighlight:
    case ClipCanvasTool::RectangleFilter:
    case ClipCanvasTool::RectangleEraser:
    case ClipCanvasTool::Spotlight:
    case ClipCanvasTool::SerialNumber:
        return CLIP_CURSOR_STYLE_CROSSHAIR;
    case ClipCanvasTool::FreeDraw:
    case ClipCanvasTool::PenHighlight:
    case ClipCanvasTool::PenFilter:
    case ClipCanvasTool::BrushEraser:
        return CLIP_CURSOR_STYLE_STROKE;
    case ClipCanvasTool::Eraser:
        return CLIP_CURSOR_STYLE_ERASER;
    case ClipCanvasTool::Text:
        return CLIP_CURSOR_STYLE_TEXT;
    case ClipCanvasTool::Select:
    case ClipCanvasTool::Watermark:
    default:
        return std::nullopt;
    }
}

bool hasExposedRect(const QRegion& exposedRegion, const QRect& widgetRect) {
    return !exposedRegion.boundingRect().intersected(widgetRect).isEmpty();
}

bool hasFilter(const ClipCanvasSceneItem* sceneItems, std::uint32_t sceneItemCount) {
    if (sceneItems == nullptr) {
        return false;
    }
    for (std::uint32_t index = 0; index < sceneItemCount; ++index) {
        if (sceneItems[index].kind == CLIP_SCENE_DISPLAY_ITEM_FILTER) {
            return true;
        }
    }
    return false;
}

std::uint64_t sceneCacheContentKey(const SceneDisplayInfo& sceneInfo,
                                   const ClipCanvasCustomRenderer* customRenderer,
                                   bool clearBackgroundEnabled, double devicePixelRatio,
                                   const QSize& widgetSize) {
    std::size_t key = 1469598103934665603ull;
    const auto hashCombine = [&key](std::size_t value) {
        key ^= value;
        key *= 1099511628211ull;
    };
    hashCombine(std::hash<double>{}(sceneInfo.surface_width));
    hashCombine(std::hash<double>{}(sceneInfo.surface_height));
    hashCombine(std::hash<double>{}(sceneInfo.camera_center_x));
    hashCombine(std::hash<double>{}(sceneInfo.camera_center_y));
    hashCombine(std::hash<double>{}(sceneInfo.camera_zoom));
    hashCombine(static_cast<std::size_t>(sceneInfo.clear_color.r) |
                (static_cast<std::size_t>(sceneInfo.clear_color.g) << 8) |
                (static_cast<std::size_t>(sceneInfo.clear_color.b) << 16) |
                (static_cast<std::size_t>(sceneInfo.clear_color.a) << 24));
    hashCombine(reinterpret_cast<std::size_t>(customRenderer));
    if (customRenderer != nullptr) {
        hashCombine(std::hash<std::uint64_t>{}(customRenderer->contentRevision()));
    }
    hashCombine(clearBackgroundEnabled ? 1u : 0u);
    hashCombine(std::hash<double>{}(devicePixelRatio));
    hashCombine(static_cast<std::size_t>(widgetSize.width()));
    hashCombine(static_cast<std::size_t>(widgetSize.height()));
    return static_cast<std::uint64_t>(key);
}

bool isPointerInput(const ClipInputEvent& input, ClipPointerEventType eventType) {
    return input.kind == CLIP_INPUT_EVENT_POINTER && input.pointer.event_type == eventType;
}

constexpr int kSerialToolbarButtonSize = 24;
constexpr int kSerialToolbarIconSize = 14;
constexpr int kSerialToolbarWidth = kSerialToolbarButtonSize * 3;
constexpr int kSerialToolbarHeight = kSerialToolbarButtonSize;
constexpr int kSerialToolbarRadius = 10;

bool focusWidgetIsWithin(const QWidget* focusWidget, const QWidget* scope) {
    for (const QWidget* candidate = focusWidget; candidate != nullptr;
         candidate = candidate->parentWidget()) {
        if (candidate == scope) {
            return true;
        }
    }
    return false;
}

enum class SerialToolbarIcon : std::uint8_t {
    Decrease,
    Increase,
    TextFields,
};

QIcon serialToolbarIcon(SerialToolbarIcon icon) {
    adqt::icons::IconRef ref;
    switch (icon) {
    case SerialToolbarIcon::Decrease:
        ref = clip::draw_engine::icons::toolbar::SerialDecrease();
        break;
    case SerialToolbarIcon::Increase:
        ref = clip::draw_engine::icons::toolbar::SerialIncrease();
        break;
    case SerialToolbarIcon::TextFields:
        ref = clip::draw_engine::icons::toolbar::SerialTextFields();
        break;
    }
    adqt::icons::IconStatePalette palette;
    palette.set(QIcon::Normal, QIcon::Off, adqt::icons::IconColors::primary(QColor(29, 27, 32)));
    palette.set(QIcon::Disabled, QIcon::Off,
                adqt::icons::IconColors::primary(QColor(73, 69, 79, 128)));
    return adqt::icons::makeIcon(ref, palette);
}

void applySerialToolbarMask(QWidget* toolbar) {
    if (toolbar == nullptr || toolbar->size().isEmpty()) {
        return;
    }

    QBitmap mask(toolbar->size());
    mask.fill(Qt::color0);

    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::color1);
    painter.drawRoundedRect(QRectF(0.0, 0.0, toolbar->width(), toolbar->height()),
                            kSerialToolbarRadius, kSerialToolbarRadius);
    toolbar->setMask(mask);
}

QToolButton* createSerialToolbarButton(QWidget* parent, const QString& objectName,
                                       SerialToolbarIcon icon, const QString& toolTip) {
    QToolButton* button = new QToolButton(parent);
    button->setObjectName(objectName);
    button->setText(QString());
    button->setIcon(serialToolbarIcon(icon));
    button->setIconSize(QSize(kSerialToolbarIconSize, kSerialToolbarIconSize));
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setToolTip(toolTip);
    button->setToolTipDuration(-1);
    button->setAccessibleName(toolTip);
    button->setAutoRaise(true);
    button->setCursor(Qt::ArrowCursor);
    button->setFixedSize(kSerialToolbarButtonSize, kSerialToolbarButtonSize);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

bool displayCacheHasSynced(const ClipCanvasDisplayCache& displayCache) {
    const ClipPatchCursor& cursor = displayCache.patchCursor();
    return cursor.scene_revision != 0 || cursor.decoration_revision != 0 ||
           cursor.overlay_revision != 0;
}

void applyWatermarkConfig(WatermarkDisplayInfo& displayInfo,
                          const ClipCanvasWatermarkConfig& config) {
    displayInfo.watermark_color = ClipColorRgba8{
        static_cast<std::uint8_t>(config.color.red()),
        static_cast<std::uint8_t>(config.color.green()),
        static_cast<std::uint8_t>(config.color.blue()),
        static_cast<std::uint8_t>(config.color.alpha()),
    };
    // Transient watermark previews currently modify presentation properties only. Keep the
    // engine-resolved template text from the retained display cache so a color preview cannot
    // temporarily expose the raw watermark text.
    displayInfo.watermark_font_size = config.fontSize;
    const QByteArray family =
        config.fontFamily.trimmed().toUtf8().left(CLIP_WATERMARK_FONT_FAMILY_CAPACITY);
    displayInfo.watermark_font_family.fill(0);
    std::copy(family.begin(), family.end(), displayInfo.watermark_font_family.begin());
    displayInfo.watermark_font_family_len = static_cast<std::uint16_t>(family.size());
    displayInfo.watermark_angle = config.angle;
    displayInfo.watermark_gap = config.gap;
    displayInfo.watermark_opacity = config.opacity;
}

bool surfaceSizeMatches(const ClipCanvasDisplayCache& displayCache, const QSize& size) {
    if (!displayCacheHasSynced(displayCache)) {
        return false;
    }

    const SceneDisplayInfo& sceneInfo = displayCache.sceneInfo();
    return sceneInfo.surface_width == static_cast<double>(size.width()) &&
           sceneInfo.surface_height == static_cast<double>(size.height());
}

bool cameraMatches(const ClipCanvasDisplayCache& displayCache, double centerX, double centerY,
                   double zoom) {
    if (!displayCacheHasSynced(displayCache)) {
        return false;
    }

    const SceneDisplayInfo& sceneInfo = displayCache.sceneInfo();
    return sceneInfo.camera_center_x == centerX && sceneInfo.camera_center_y == centerY &&
           sceneInfo.camera_zoom == zoom;
}

qreal painterDevicePixelRatio(const QPainter& painter, const QWidget& widget) {
    if (painter.device() != nullptr && painter.device()->devicePixelRatioF() > 0.0) {
        return painter.device()->devicePixelRatioF();
    }
    return widget.devicePixelRatioF() > 0.0 ? widget.devicePixelRatioF() : 1.0;
}

QTransform canvasToViewTransform(const SceneDisplayInfo& sceneInfo) {
    const qreal zoom = sceneInfo.camera_zoom > 0.0 ? sceneInfo.camera_zoom : 1.0;
    return QTransform(zoom, 0.0, 0.0, zoom,
                      sceneInfo.surface_width / 2.0 - sceneInfo.camera_center_x * zoom,
                      sceneInfo.surface_height / 2.0 - sceneInfo.camera_center_y * zoom);
}

void accept(QKeyEvent& event) {
    event.accept();
}

bool hitsSelectedText(ClipRuntime runtime, ClipViewport viewport, const QPointF& canvasPoint) {
    ClipElementId id{};
    std::uint8_t hit = 0;
    std::uint8_t selected = 0;
    return clip_viewport_hit_text(runtime, viewport, canvasPoint.x(), canvasPoint.y(), &id, &hit) ==
               CLIP_OK &&
           hit != 0 &&
           clip_viewport_is_element_selected(runtime, viewport, id, &selected) == CLIP_OK &&
           selected != 0;
}

} // namespace

struct ClipCanvasWidget::Impl : public clip_canvas_runtime::Client {
    explicit Impl(ClipCanvasWidget& widget)
        : widget(widget), cursorController(widget), inputHandler(widget, cursorController),
          textInteraction(widget, cursorController) {}

    Impl(ClipCanvasWidget& widget, ClipCanvasRuntime& runtime)
        : widget(widget), runtimeBinding(runtime), cursorController(widget),
          inputHandler(widget, cursorController), textInteraction(widget, cursorController) {}

    bool stepFontSize(int direction);
    bool applyStyleEdit(const ClipCanvasStyleEdit& edit);
    bool rememberDraftStyle(const ClipCanvasStyleEdit& edit);
    void initializeWidget();
    void initializeViewport();
    std::uint64_t runtimeViewportId() const override;
    void detachRuntimeForReplacement() override;
    void detachRuntime();
    void attachRuntime(ClipRuntime runtime) override;
    void detachRuntimeOwner(ClipCanvasRuntime* owner) override;
    void clearRenderState() override;
    void resetDocumentRetainedState() override;
    void setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources,
                             const QRegion& damage = {}, bool partialDamage = false);
    void smartEraseChanged() override {
        clearRenderState();
        widget.update();
    }
    void clearRetainedDisplayState();
    bool hasViewport() const;
    void syncAfterEngineMutation() override;
    void syncAfterEngineMutation(bool emitSignals);
    quint64 autoFilterGeneration = 0;
    void refreshStateFromEngine(bool emitSignals) override;
    void syncChangedViewports(ClipChangedViewportList changedViewports);
    bool measurePresentationText();
    bool withPresentationUpdate(const std::function<bool()>& update);
    bool applyMutation(const std::function<clip_canvas_commands::MutationResult()>& mutate);
    bool
    applyPairedMutation(const std::function<clip_canvas_commands::PairedMutationResult()>& mutate);
    unsigned presentationUpdateDepth = 0;
    void refreshArrowTextMetrics();
    bool setSurfaceSizeAndSync(const QSize& size, bool emitSignals);
    bool setCameraAndSync(double centerX, double centerY, double zoom);
    void shutdown();
    void initializeSerialNumberToolbar();
    void refreshSerialNumberToolbar();

    ClipCanvasTool canvasTool() const;
    bool setCanvasTool(ClipCanvasTool tool);
    void setCursorForLayer(ClipCanvasCursorLayer layer, const QCursor& cursor);
    void clearCursorForLayer(ClipCanvasCursorLayer layer);
    void refreshCursorDevicePixelRatio() {
        cursorController.refreshDevicePixelRatio();
    }
    ClipCanvasStyleToolbarState canvasStyleToolbarState() const;
    bool setCanvasDistanceStyle(const ClipCanvasDistanceStyle& style, quint32 properties);
    bool setDistanceCreationPixelScale(const QSizeF& scale);
    ClipCanvasSerialNumberToolbarState serialNumberToolbarState() const;
    ClipCanvasWatermarkConfig canvasWatermarkConfig() const;
    bool setCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config);
    void previewCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config);
    void applyPendingWatermarkPreview();
    int watermarkPreviewFrameInterval() const;
    void updateWatermarkPresentationInfo() const;
    ClipCanvasSpotlightConfig canvasSpotlightConfig() const;
    bool setCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config);
    void previewCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config);
    void applyPendingSpotlightPreview();
    bool setCanvasShapeStylePatch(const ClipCanvasShapeStyle& style, quint32 properties,
                                  ClipCanvasShapeKind kind);
    bool setCanvasFilterStyle(const ClipCanvasFilterStyle& style, quint32 properties);
    bool setCanvasFilterCreationStyle(const ClipCanvasFilterStyle& style, quint32 properties,
                                      ClipCanvasTool filterTool);
    bool setCanvasBrushEraserCreationStyle(const ClipCanvasBrushEraserStyle& style,
                                           quint32 properties);
    quint64 readAutoFilterGeneration() const;
    std::optional<ClipCanvasAutoFilterRecord> autoFilterRegions() const;
    bool setAutoFilterRegions(const std::optional<ClipCanvasAutoFilterRecord>& record);
    bool fillAutoFilterCategory(const QString& category);
    bool setCanvasTextStyle(const ClipCanvasTextStyle& style, quint32 properties);
    bool setCanvasSerialNumberStyle(const ClipCanvasSerialNumberStyle& style);
    ClipCanvasHistoryState canvasHistoryState() const;
    ClipCanvasSnapConfig canvasSnapConfig() const;
    bool setCanvasSnapConfig(const ClipCanvasSnapConfig& config);
    bool setCanvasSnapGuideTargets(const ClipCanvasSnapGuideTargets& targets);
    ClipCanvasGridConfig canvasGridConfig() const;
    bool setCanvasGridConfig(const ClipCanvasGridConfig& config);
    bool interactionEnabled() const;
    void setInteractionEnabled(bool enabled);
    bool hasQuickSelectionTargetAt(const QPointF& viewPosition, Qt::MouseButton button) const;
    void updateMouseContextMenuOwnership(const QMouseEvent* event, bool handled);
    bool handleContextMenu(QContextMenuEvent* event);
    bool wheelZoomEnabled() const;
    void setWheelZoomEnabled(bool enabled);
    bool canvasContentVisible() const;
    void setCanvasContentVisible(bool visible);
    bool clearBackgroundEnabled() const;
    void setClearBackgroundEnabled(bool enabled);
    bool showDirtyRects() const;
    void setShowDirtyRects(bool show);
    std::uint64_t viewportId() const;
    bool setViewportCamera(double centerX, double centerY, double zoom);
    bool hasWatermarkRenderArea() const;
    QRectF watermarkRenderArea() const;
    void setWatermarkRenderArea(const QRectF& canvasRect);
    void clearWatermarkRenderArea();
    void setDecorationRenderAreas(const ClipCanvasDecorationRenderAreas& areas);
    QRectF watermarkViewRenderArea() const;
    QRegion watermarkViewRenderRegion() const;
    bool hasSpotlightRenderArea() const;
    QRectF spotlightRenderArea() const;
    void setSpotlightRenderArea(const QRectF& canvasRect);
    void clearSpotlightRenderArea();
    QRectF spotlightViewRenderArea() const;
    QRegion spotlightViewRenderRegion() const;
    bool watermarkEffectivelyVisible() const;
    bool spotlightEffectivelyVisible() const;
    ClipCanvasCustomRenderer* customRenderer() const;
    void setCustomRenderer(ClipCanvasCustomRenderer* renderer);
    QTransform canvasToViewTransform() const;
    QRect viewRectForCanvasRect(const QRectF& canvasRect, int paddingPx) const;

    bool undo();
    bool redo();
    bool deleteSelected();
    bool deleteAllElements();
    bool clearDocument();
    bool duplicateSelected(const QPointF& offset);
    bool insertDrawTemplate(const QByteArray& payload, const QPointF& center);
    bool reorderSelected(ClipCanvasSelectionOrder order);
    bool alignSelected(ClipCanvasSelectionAlignment alignment);
    bool setSelectedOpacity(double opacity);
    bool adjustSelectedSerialNumbers(qint64 delta);
    bool createSerialNumberText();
    bool beginArrowText(const QPointF& viewPosition, bool selected);
    bool resetEditingState(bool restoreSelectTool) override;
    bool cancelActiveTextEditing();
    bool hasActiveTextEditing() const;
    ClipCanvasWidgetTextInteraction::CommitResult commitText(bool refocusWidget = true,
                                                             bool restoreExistingSelection = true);
    void applyTextCommitResult(ClipCanvasWidgetTextInteraction::CommitResult& result,
                               bool restoreExistingSelection);
    void applyTextEditorEventResult(ClipCanvasWidgetTextInteraction::EditorEventResult& result);
    bool restoreFinishedExistingTextSelection(
        const ClipCanvasWidgetTextInteraction::FinishedExistingEdit& edit);
    bool beginText(const QPointF& viewPosition, bool allowCreate);
    bool beginSelectedText(const QPointF& viewPosition, bool requireSerialBoundText);

    void paintEvent(QPaintEvent* event);
    bool paint(QPainter& painter, const QRegion& exposedRegion);
    clip_canvas_compositor::Frame buildPaintFrame() const;
    ClipCanvasRenderContext renderContext(QPainter& painter, const QRegion& exposedRegion) const;
    void renderBeforeCanvas(QPainter& painter, const ClipCanvasRenderContext& context) const;
    void renderAfterCanvas(QPainter& painter, const ClipCanvasRenderContext& context) const;
    bool handleMousePress(QMouseEvent* event);
    bool handleMouseDoubleClick(QMouseEvent* event);
    bool handleMouseMove(QMouseEvent* event);
    bool handleMouseRelease(QMouseEvent* event);
    void beginMiddleClickTracking(const QPointF& position);
    void updateMiddleClickTracking(const QPointF& position);
    bool finishMiddleClickTracking();
    void cancelMiddleClickTracking();
    bool handleEnter(QEnterEvent* event);
    bool handleLeave(QEvent* event);
    bool handleWheel(QWheelEvent* event);
    enum class FontWheelTarget { Text, SerialNumber };
    FontWheelTarget fontWheelTarget() const;
    bool stepSerialNumberFontSize(bool increase);
    bool stepTextFontSize(bool increase);
    CommandKeyResolver commandKeyResolver;
    bool handleKeyPress(QKeyEvent* event);
    bool handleKeyRelease(QKeyEvent* event);
    bool handleInputMethodEvent(QInputMethodEvent* event);
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
    void handleFocusOut();
    void handleResize(const QSize& size);
    void beginTextStylePopupInteraction();
    void endTextStylePopupInteraction(QWidget* focusScope);
    void clearTextStylePopupInteraction();
    void handleApplicationStateChanged(Qt::ApplicationState state);

  private:
    ClipCanvasWidgetInputHandler::Context inputHandlerContext();
    void applyInputProcessResult(const ClipCanvasWidgetInputHandler::ProcessResult& result);
    bool applyActiveTextResizeMeasurementIfNeeded();
    bool prepareActiveTextResizeMeasurementForPointerUp(const ClipInputEvent& input);
    bool dispatchInput(QEvent* event, const ClipInputEvent& input);
    bool processInput(const ClipInputEvent& input);
    bool queueLiveStrokeMove(QMouseEvent* event, const ClipInputEvent& input,
                             bool preserveEverySample);
    void flushLiveStrokeMoves();
    bool queueEraserMove(QMouseEvent* event, const ClipInputEvent& input);
    void flushEraserMove();

    bool m_wheelZoomEnabled = true;
    bool applyMutationResult(const clip_canvas_commands::MutationResult& result);
    void emitChangedStateSignals(const clip_canvas_state::Changes& changes);
    void applyCanvasToolCursor(ClipCanvasTool tool);
    void refreshToolCursorStyle();
    void refocusWidget();

    ClipCanvasWidget& widget;
    ClipCanvasWidgetRuntimeBinding runtimeBinding;
    ClipCanvasCursorController cursorController;
    ClipCanvasWidgetInputHandler inputHandler;
    ClipCanvasWidgetDisplayState displayState;
    QWidget* serialNumberToolbar = nullptr;
    QToolButton* serialNumberDecreaseButton = nullptr;
    QToolButton* serialNumberIncreaseButton = nullptr;
    QToolButton* serialNumberCreateTextButton = nullptr;
    bool dirtyRectsVisible = false;
    bool canvasContentIsVisible = true;
    bool canvasClearBackgroundEnabled = true;
    bool suppressNextTextToolCreate = false;
    bool restoredSelectionForNextTextToolPress = false;
    bool suppressMouseContextMenu = false;
    ClipCanvasTool requestedCanvasTool = ClipCanvasTool::Select;
    ClipCanvasCustomRenderer* installedCustomRenderer = nullptr;
    std::optional<ClipCanvasSnapGuideTargets> snapGuideTargets;
    std::optional<QRectF> configuredWatermarkRenderArea;
    std::optional<QRectF> configuredSpotlightRenderArea;
    clip_canvas_filter_render::RenderWorkspace filterWorkspace;
    clip_canvas_pen_mask::PenMaskAtlas penMaskAtlas;
    ClipCanvasReferenceScene referenceScene;
    ClipCanvasWidgetTextInteraction textInteraction;
    std::optional<QPointF> middleClickPressPosition;
    bool middleClickCandidate = false;
    std::vector<ClipInputEvent> pendingLiveStrokeMoves;
    bool liveStrokeMoveFlushScheduled = false;
    quint64 liveStrokeMoveFlushGeneration = 0;
    bool pendingLiveStrokePreservesEverySample = false;
    std::optional<ClipInputEvent> pendingEraserMove;
    bool eraserMoveFlushScheduled = false;
    std::optional<ClipCanvasWatermarkConfig> pendingWatermarkPreview;
    std::optional<ClipCanvasWatermarkConfig> watermarkPreview;
    QTimer watermarkPreviewTimer;
    bool watermarkPreviewScheduled = false;
    mutable WatermarkDisplayInfo watermarkPresentationInfo;
    std::optional<ClipCanvasSpotlightConfig> spotlightPreview;
    std::optional<ClipCanvasSpotlightConfig> pendingSpotlightPreview;
    QTimer spotlightPreviewTimer;
    bool spotlightPreviewScheduled = false;
    mutable SpotlightDisplayInfo spotlightPresentationInfo;
    int textStylePopupInteractionDepth = 0;
    quint64 textStylePopupFocusRestoreGeneration = 0;
    QPointer<QWidget> textStylePopupFocusScope;
};

ClipCanvasWidget::ClipCanvasWidget(QWidget* parent)
    : QWidget(parent), m_impl(std::make_unique<Impl>(*this)) {
    m_impl->initializeWidget();
    m_impl->initializeViewport();
}

ClipCanvasWidget::ClipCanvasWidget(ClipCanvasRuntime& runtime, QWidget* parent)
    : QWidget(parent), m_impl(std::make_unique<Impl>(*this, runtime)) {
    m_impl->initializeWidget();
    m_impl->initializeViewport();
}

ClipCanvasWidgetInputHandler::Context ClipCanvasWidget::Impl::inputHandlerContext() {
    ClipCanvasWidgetInputHandler::Context context;
    context.hasViewport = hasViewport();
    context.processInput = [this](const ClipInputEvent& input) {
        return clip_canvas_commands::processInput(runtimeBinding.engine(),
                                                  runtimeBinding.viewportHandle(), input);
    };
    context.processInputBatch = [this](const std::vector<ClipInputEvent>& inputs) {
        return clip_canvas_commands::processPointerMoveBatch(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), inputs);
    };
    return context;
}

void ClipCanvasWidget::Impl::applyInputProcessResult(
    const ClipCanvasWidgetInputHandler::ProcessResult& result) {
    if (!result.success) {
        return;
    }
    syncChangedViewports(result.changedViewports.get());
}

bool ClipCanvasWidget::Impl::applyActiveTextResizeMeasurementIfNeeded() {
    ClipCanvasWidgetTextInteraction::ActiveResizeMeasurementResult result =
        textInteraction.applyActiveResizeMeasurementIfNeeded(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), displayState.displayCache());
    if (!result.success) {
        return false;
    }
    if (result.active) {
        syncChangedViewports(result.changedViewports.get());
    }
    return true;
}

bool ClipCanvasWidget::Impl::prepareActiveTextResizeMeasurementForPointerUp(
    const ClipInputEvent& input) {
    if (!isPointerInput(input, CLIP_POINTER_EVENT_UP)) {
        return true;
    }

    const ClipCanvasWidgetTextInteraction::ActiveResizeMeasurementState state =
        textInteraction.activeResizeMeasurementState(runtimeBinding.engine(),
                                                     runtimeBinding.viewportHandle());
    if (!state.success) {
        return false;
    }
    const auto tool = canvasTool();
    const bool geometryTool = tool == ClipCanvasTool::Select || tool == ClipCanvasTool::Arrow ||
                              tool == ClipCanvasTool::Distance || tool == ClipCanvasTool::Shape;
    if (!state.active &&
        (!geometryTool || (tool != ClipCanvasTool::Distance &&
                           clip_runtime_arrow_text_count(runtimeBinding.engine()) == 0))) {
        return true;
    }

    ClipInputEvent moveInput = input;
    moveInput.pointer.event_type = CLIP_POINTER_EVENT_MOVE;
    moveInput.pointer.button = CLIP_POINTER_BUTTON_NONE;
    moveInput.pointer.buttons |= 0b0000'0001;
    ClipCanvasWidgetInputHandler::ProcessResult moveResult =
        inputHandler.process(inputHandlerContext(), moveInput);
    applyInputProcessResult(moveResult);
    if (!moveResult.success) {
        return false;
    }
    // Persist the final pointer position's label metrics in the originating
    // undo command, while keeping the intermediate move unpublished.
    return measurePresentationText() && applyActiveTextResizeMeasurementIfNeeded();
}

bool ClipCanvasWidget::Impl::dispatchInput(QEvent* event, const ClipInputEvent& input) {
    if (!isPointerInput(input, CLIP_POINTER_EVENT_MOVE)) {
        flushLiveStrokeMoves();
        flushEraserMove();
    }

    return withPresentationUpdate([&]() {
        if (!prepareActiveTextResizeMeasurementForPointerUp(input)) {
            return false;
        }
        ClipCanvasWidgetInputHandler::DispatchResult result =
            inputHandler.dispatch(event, inputHandlerContext(), input);
        applyInputProcessResult(result.process);
        if (result.process.success && isPointerInput(input, CLIP_POINTER_EVENT_MOVE)) {
            if (!applyActiveTextResizeMeasurementIfNeeded()) {
                return false;
            }
        }
        return result.accepted;
    });
}

bool ClipCanvasWidget::Impl::processInput(const ClipInputEvent& input) {
    return withPresentationUpdate([&]() {
        if (!prepareActiveTextResizeMeasurementForPointerUp(input)) {
            return false;
        }
        ClipCanvasWidgetInputHandler::ProcessResult result =
            inputHandler.process(inputHandlerContext(), input);
        applyInputProcessResult(result);
        if (result.success && isPointerInput(input, CLIP_POINTER_EVENT_MOVE)) {
            if (!applyActiveTextResizeMeasurementIfNeeded()) {
                return false;
            }
        }
        return result.success;
    });
}

void ClipCanvasWidget::Impl::initializeWidget() {
    QObject::connect(qGuiApp, &QGuiApplication::fontDatabaseChanged, &widget,
                     [this]() { refreshArrowTextMetrics(); });
    widget.setAttribute(Qt::WA_OpaquePaintEvent);
    widget.setAttribute(Qt::WA_InputMethodEnabled, false);
    widget.setMouseTracking(true);
    widget.setFocusPolicy(Qt::StrongFocus);
    watermarkPreviewTimer.setSingleShot(true);
    QObject::connect(&watermarkPreviewTimer, &QTimer::timeout, &widget, [this]() {
        if (!pendingWatermarkPreview.has_value()) {
            watermarkPreviewScheduled = false;
            return;
        }
        applyPendingWatermarkPreview();
        watermarkPreviewTimer.start(watermarkPreviewFrameInterval());
    });
    spotlightPreviewTimer.setSingleShot(true);
    QObject::connect(&spotlightPreviewTimer, &QTimer::timeout, &widget, [this]() {
        if (!pendingSpotlightPreview.has_value()) {
            spotlightPreviewScheduled = false;
            return;
        }
        applyPendingSpotlightPreview();
        if (pendingSpotlightPreview.has_value()) {
            spotlightPreviewTimer.start(watermarkPreviewFrameInterval());
        } else {
            spotlightPreviewScheduled = false;
        }
    });
    initializeSerialNumberToolbar();
    QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged, &widget,
                     [this](Qt::ApplicationState state) { handleApplicationStateChanged(state); });
}

void ClipCanvasWidget::Impl::initializeViewport() {
    displayState.resetRetainedState();
    runtimeBinding.registerClient(*this);
    runtimeBinding.initializeAttachment(displayState);
    setSurfaceSizeAndSync(widget.size(), false);
    refreshSerialNumberToolbar();
}

void ClipCanvasWidget::Impl::initializeSerialNumberToolbar() {
    serialNumberToolbar = new QWidget(&widget);
    serialNumberToolbar->setObjectName(QStringLiteral("clipSerialNumberToolbar"));
    serialNumberToolbar->setAttribute(Qt::WA_StyledBackground, true);
    serialNumberToolbar->setFixedSize(kSerialToolbarWidth, kSerialToolbarHeight);
    applySerialToolbarMask(serialNumberToolbar);
    serialNumberToolbar->setStyleSheet(
        QStringLiteral("#clipSerialNumberToolbar {"
                       "background: white;"
                       "border: none;"
                       "border-radius: 10px;"
                       "}"
                       "#clipSerialNumberToolbar QToolButton {"
                       "border: 0;"
                       "border-radius: 0;"
                       "padding: 0;"
                       "}"
                       "#clipSerialNumberToolbarDecreaseButton {"
                       "border-top-left-radius: 10px;"
                       "border-bottom-left-radius: 10px;"
                       "}"
                       "#clipSerialNumberToolbarCreateTextButton {"
                       "border-top-right-radius: 10px;"
                       "border-bottom-right-radius: 10px;"
                       "}"
                       "#clipSerialNumberToolbar QToolButton:hover:enabled {"
                       "background: rgba(30, 30, 30, 18);"
                       "}"
                       "#clipSerialNumberToolbar QToolButton:pressed:enabled {"
                       "background: rgba(30, 30, 30, 32);"
                       "}"
                       "#clipSerialNumberToolbar QToolButton:disabled {"
                       "color: rgba(30, 30, 30, 76);"
                       "}"));

    auto* shadow = new QGraphicsDropShadowEffect(serialNumberToolbar);
    shadow->setBlurRadius(10.0);
    shadow->setOffset(0.0, 2.0);
    shadow->setColor(QColor(0, 0, 0, 48));
    serialNumberToolbar->setGraphicsEffect(shadow);

    auto* layout = new QHBoxLayout(serialNumberToolbar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    serialNumberDecreaseButton = createSerialToolbarButton(
        serialNumberToolbar, QStringLiteral("clipSerialNumberToolbarDecreaseButton"),
        SerialToolbarIcon::Decrease, QStringLiteral("Decrease"));
    serialNumberIncreaseButton = createSerialToolbarButton(
        serialNumberToolbar, QStringLiteral("clipSerialNumberToolbarIncreaseButton"),
        SerialToolbarIcon::Increase, QStringLiteral("Increase"));
    serialNumberCreateTextButton = createSerialToolbarButton(
        serialNumberToolbar, QStringLiteral("clipSerialNumberToolbarCreateTextButton"),
        SerialToolbarIcon::TextFields, QStringLiteral("Create Text"));

    layout->addWidget(serialNumberDecreaseButton);
    layout->addWidget(serialNumberIncreaseButton);
    layout->addWidget(serialNumberCreateTextButton);

    QObject::connect(serialNumberDecreaseButton, &QToolButton::clicked, &widget, [this]() {
        adjustSelectedSerialNumbers(-1);
        refreshSerialNumberToolbar();
    });
    QObject::connect(serialNumberIncreaseButton, &QToolButton::clicked, &widget, [this]() {
        adjustSelectedSerialNumbers(1);
        refreshSerialNumberToolbar();
    });
    QObject::connect(serialNumberCreateTextButton, &QToolButton::clicked, &widget, [this]() {
        createSerialNumberText();
        refreshSerialNumberToolbar();
    });

    serialNumberToolbar->hide();
}

void ClipCanvasWidget::Impl::refreshSerialNumberToolbar() {
    if (serialNumberToolbar == nullptr) {
        return;
    }

    const ClipCanvasSerialNumberToolbarState state = serialNumberToolbarState();
    if (!canvasContentIsVisible || !state.visible) {
        serialNumberToolbar->hide();
        return;
    }

    const bool enabled = interactionEnabled();
    serialNumberDecreaseButton->setEnabled(enabled && state.canDecrease);
    serialNumberIncreaseButton->setEnabled(enabled && state.canIncrease);
    serialNumberCreateTextButton->setEnabled(enabled && state.canCreateText);
    serialNumberToolbar->setGeometry(state.geometry.toAlignedRect());
    applySerialToolbarMask(serialNumberToolbar);
    serialNumberToolbar->raise();
    serialNumberToolbar->show();
}

void ClipCanvasWidget::Impl::shutdown() {
    if (pendingLiveStrokePreservesEverySample) {
        flushLiveStrokeMoves();
    }
    clearTextStylePopupInteraction();
    detachRuntime();
    runtimeBinding.unregisterClient(*this);
}

ClipCanvasWidget::~ClipCanvasWidget() {
    m_impl->shutdown();
}

ClipCanvasTool ClipCanvasWidget::Impl::canvasTool() const {
    const ClipCanvasTool engineTool =
        clip_canvas_types::toCanvasTool(displayState.snapshot().activeTool);
    return requestedCanvasTool == ClipCanvasTool::Line && engineTool == ClipCanvasTool::Arrow
               ? ClipCanvasTool::Line
               : engineTool;
}

ClipCanvasTool ClipCanvasWidget::canvasTool() const {
    return m_impl->canvasTool();
}

bool ClipCanvasWidget::Impl::setCanvasTool(ClipCanvasTool tool) {
    const ClipCanvasTool previousTool = canvasTool();
    if (previousTool != tool) {
        if (pendingLiveStrokePreservesEverySample) {
            flushLiveStrokeMoves();
        } else {
            pendingLiveStrokeMoves.clear();
        }
        pendingEraserMove.reset();
    }
    if (textInteraction.isActive() && previousTool != tool) {
        clearTextStylePopupInteraction();
        commitText(false, false);
    }

    const bool engineToolChanged =
        clip_canvas_types::toEngineTool(previousTool) != clip_canvas_types::toEngineTool(tool);
    if (!applyMutation([&]() {
            return clip_canvas_commands::setActiveTool(runtimeBinding.engine(),
                                                       runtimeBinding.viewportHandle(),
                                                       clip_canvas_types::toEngineTool(tool));
        })) {
        return false;
    }

    requestedCanvasTool = tool;
    if (previousTool != tool && !engineToolChanged) {
        emit widget.activeToolChanged();
    }
    applyCanvasToolCursor(tool);
    return true;
}

bool ClipCanvasWidget::setCanvasTool(ClipCanvasTool tool) {
    return m_impl->setCanvasTool(tool);
}

void ClipCanvasWidget::Impl::setCursorForLayer(ClipCanvasCursorLayer layer, const QCursor& cursor) {
    cursorController.setCursor(layer, cursor);
}

void ClipCanvasWidget::setCursorForLayer(ClipCanvasCursorLayer layer, const QCursor& cursor) {
    m_impl->setCursorForLayer(layer, cursor);
}

void ClipCanvasWidget::Impl::clearCursorForLayer(ClipCanvasCursorLayer layer) {
    cursorController.clearCursor(layer);
}

void ClipCanvasWidget::clearCursorForLayer(ClipCanvasCursorLayer layer) {
    m_impl->clearCursorForLayer(layer);
}

ClipCanvasStyleToolbarState ClipCanvasWidget::Impl::canvasStyleToolbarState() const {
    ClipCanvasStyleToolbarState state =
        clip_canvas_types::toCanvasStyleToolbarState(displayState.snapshot().styleToolbarState);
    ClipTextElementInfo arrowText{};
    ClipTextStyle arrowTextStyle{};
    std::uint8_t foundArrow = 0;
    state.canEditArrowText = hasViewport() && interactionEnabled() && !textInteraction.isActive() &&
                             clip_viewport_get_arrow_text_target(
                                 runtimeBinding.engine(), runtimeBinding.viewportHandle(), 0, 0.0,
                                 0.0, &arrowText, &arrowTextStyle, &foundArrow) == CLIP_OK &&
                             foundArrow != 0;
    if (textInteraction.isActive()) {
        if (textInteraction.session().arrowId().generation != 0) {
            state.source = ClipCanvasStyleToolbarSource::SelectedText;
        }
        state.textStyle = clip_canvas_types::toCanvasTextStyle(textInteraction.currentTextStyle());
        state.textStyleMixed = 0;
    }
    return state;
}

ClipCanvasStyleToolbarState ClipCanvasWidget::canvasStyleToolbarState() const {
    return m_impl->canvasStyleToolbarState();
}

ClipCanvasSerialNumberToolbarState ClipCanvasWidget::Impl::serialNumberToolbarState() const {
    ClipCanvasSerialNumberToolbarState state;
    if (!hasViewport()) {
        return state;
    }

    ClipSerialNumberToolbarState engineState{};
    if (clip_viewport_get_serial_number_toolbar_state(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), &engineState) != CLIP_OK) {
        return state;
    }

    state.visible = engineState.visible != 0;
    state.geometry =
        QRectF(engineState.left, engineState.top, engineState.width, engineState.height);
    state.canDecrease = engineState.can_decrease != 0;
    state.canIncrease = engineState.can_increase != 0;
    state.canCreateText = engineState.can_create_text != 0;
    return state;
}

ClipCanvasSerialNumberToolbarState ClipCanvasWidget::serialNumberToolbarState() const {
    return m_impl->serialNumberToolbarState();
}

ClipCanvasDistanceStyle ClipCanvasWidget::canvasDistanceStyle() const {
    return canvasStyleToolbarState().distanceStyle;
}

bool ClipCanvasWidget::setCanvasDistanceStyle(const ClipCanvasDistanceStyle& style,
                                              quint32 properties) {
    return m_impl->setCanvasDistanceStyle(style, properties);
}

bool ClipCanvasWidget::Impl::setCanvasDistanceStyle(const ClipCanvasDistanceStyle& style,
                                                    quint32 properties) {
    if (!clip_canvas_types::validDistanceStyle(style))
        return false;
    return applyMutation([&]() {
        return clip_canvas_commands::setDistanceStylePatch(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(),
            clip_canvas_types::toEngineDistanceStyle(style), properties);
    });
}

bool ClipCanvasWidget::setDistanceCreationPixelScale(const QSizeF& scale) {
    return m_impl->setDistanceCreationPixelScale(scale);
}

bool ClipCanvasWidget::Impl::setDistanceCreationPixelScale(const QSizeF& scale) {
    // Calibration is captured when creation starts; changing it needs no
    // presentation update, label measurement, or focus transfer.
    return clip_canvas_commands::setDistancePixelScale(runtimeBinding.engine(),
                                                       runtimeBinding.viewportHandle(),
                                                       scale.width(), scale.height())
        .success;
}

bool ClipCanvasWidget::Impl::setCanvasShapeStylePatch(const ClipCanvasShapeStyle& style,
                                                      quint32 properties,
                                                      ClipCanvasShapeKind kind) {
    return applyMutation([&]() {
        return clip_canvas_commands::setShapeStylePatch(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(),
            clip_canvas_types::toEngineShapeStyle(style), properties,
            clip_canvas_types::toEngineShapeKind(kind));
    });
}

ClipCanvasWatermarkConfig ClipCanvasWidget::Impl::canvasWatermarkConfig() const {
    ClipWatermarkConfig engineConfig{};
    if (clip_viewport_get_watermark_config(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                                           &engineConfig) != CLIP_OK) {
        return {};
    }
    return clip_canvas_types::toCanvasWatermarkConfig(engineConfig);
}

ClipCanvasWatermarkConfig ClipCanvasWidget::canvasWatermarkConfig() const {
    return m_impl->canvasWatermarkConfig();
}

bool ClipCanvasWidget::Impl::setCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config) {
    // A persistent update owns the display state from this point onward. Any
    // queued transient value must not be allowed to overwrite the synced store.
    const QRegion previousPreviewRegion = watermarkViewRenderRegion();
    pendingWatermarkPreview.reset();
    watermarkPreview.reset();
    watermarkPreviewTimer.stop();
    watermarkPreviewScheduled = false;
    const ClipWatermarkConfig engineConfig = clip_canvas_types::toEngineWatermarkConfig(config);
    const bool applied = applyMutation([&]() {
        return clip_canvas_commands::setWatermarkConfig(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), engineConfig);
    });
    if (applied && !previousPreviewRegion.isEmpty()) {
        widget.update(previousPreviewRegion);
    }
    return applied;
}

bool ClipCanvasWidget::setCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config) {
    return m_impl->setCanvasWatermarkConfig(config);
}

void ClipCanvasWidget::Impl::previewCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config) {
    pendingWatermarkPreview = config;
    if (watermarkPreviewScheduled) {
        return;
    }
    watermarkPreviewScheduled = true;
    applyPendingWatermarkPreview();
    watermarkPreviewTimer.start(watermarkPreviewFrameInterval());
}

void ClipCanvasWidget::Impl::applyPendingWatermarkPreview() {
    if (!pendingWatermarkPreview.has_value()) {
        return;
    }
    const QRegion previousRegion = watermarkViewRenderRegion();
    watermarkPreview = *pendingWatermarkPreview;
    pendingWatermarkPreview.reset();
    updateWatermarkPresentationInfo();
    const QRegion changedRegion = previousRegion.united(watermarkViewRenderRegion());
    if (!changedRegion.isEmpty()) {
        widget.update(changedRegion);
    }
    emit widget.watermarkPreviewApplied();
}

int ClipCanvasWidget::Impl::watermarkPreviewFrameInterval() const {
    QScreen* screen = widget.windowHandle() != nullptr ? widget.windowHandle()->screen()
                                                       : QGuiApplication::primaryScreen();
    const qreal refreshRate = screen != nullptr ? screen->refreshRate() : 60.0;
    const int interval =
        refreshRate > 1.0 && std::isfinite(refreshRate) ? qRound(1000.0 / refreshRate) : 16;
    return std::clamp(interval, 4, 16);
}

void ClipCanvasWidget::previewCanvasWatermarkConfig(const ClipCanvasWatermarkConfig& config) {
    m_impl->previewCanvasWatermarkConfig(config);
}

ClipCanvasSpotlightConfig ClipCanvasWidget::Impl::canvasSpotlightConfig() const {
    ClipSpotlightConfig config{};
    if (clip_viewport_get_spotlight_config(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                                           &config) != CLIP_OK) {
        return {};
    }
    return clip_canvas_types::toCanvasSpotlightConfig(config);
}

ClipCanvasSpotlightConfig ClipCanvasWidget::canvasSpotlightConfig() const {
    return m_impl->canvasSpotlightConfig();
}

bool ClipCanvasWidget::Impl::setCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config) {
    const QRegion previousPreviewRegion = spotlightViewRenderRegion();
    pendingSpotlightPreview.reset();
    spotlightPreview.reset();
    spotlightPreviewTimer.stop();
    spotlightPreviewScheduled = false;
    const bool applied = applyMutation([&]() {
        return clip_canvas_commands::setSpotlightConfig(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(),
            clip_canvas_types::toEngineSpotlightConfig(config));
    });
    if (applied && !previousPreviewRegion.isEmpty()) {
        widget.update(previousPreviewRegion);
    }
    return applied;
}

bool ClipCanvasWidget::setCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config) {
    return m_impl->setCanvasSpotlightConfig(config);
}

void ClipCanvasWidget::Impl::previewCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config) {
    const ClipCanvasSpotlightConfig current =
        spotlightPreview.has_value() ? *spotlightPreview : canvasSpotlightConfig();
    if (config == current) {
        pendingSpotlightPreview.reset();
        // Keep an already scheduled refresh boundary alive so a later value
        // in the same burst remains coalesced instead of applying immediately.
        if (!spotlightPreviewScheduled) {
            spotlightPreviewTimer.stop();
        }
        return;
    }
    if (pendingSpotlightPreview.has_value() && *pendingSpotlightPreview == config) {
        return;
    }
    pendingSpotlightPreview = config;
    if (spotlightPreviewScheduled) {
        return;
    }
    spotlightPreviewScheduled = true;
    applyPendingSpotlightPreview();
    spotlightPreviewTimer.start(watermarkPreviewFrameInterval());
}

void ClipCanvasWidget::Impl::applyPendingSpotlightPreview() {
    if (!pendingSpotlightPreview.has_value()) {
        return;
    }
    const QRegion previousRegion = spotlightViewRenderRegion();
    spotlightPreview = *pendingSpotlightPreview;
    pendingSpotlightPreview.reset();
    const QRegion changedRegion = previousRegion.united(spotlightViewRenderRegion());
    if (!changedRegion.isEmpty()) {
        widget.update(changedRegion);
    }
    emit widget.spotlightPreviewApplied();
}

void ClipCanvasWidget::previewCanvasSpotlightConfig(const ClipCanvasSpotlightConfig& config) {
    m_impl->previewCanvasSpotlightConfig(config);
}

bool ClipCanvasWidget::setCanvasShapeStylePatch(const ClipCanvasShapeStyle& style,
                                                quint32 properties, ClipCanvasShapeKind kind) {
    return m_impl->setCanvasShapeStylePatch(style, properties, kind);
}

bool ClipCanvasWidget::Impl::setCanvasFilterStyle(const ClipCanvasFilterStyle& style,
                                                  quint32 properties) {
    ClipFilterStyle engineStyle{};
    engineStyle.filter_type = static_cast<ClipFilterType>(style.type);
    engineStyle.strength = style.strength;
    engineStyle.opacity = style.opacity;
    engineStyle.stroke_width = style.strokeWidth;
    return applyMutation([&]() {
        return clip_canvas_commands::setFilterStyle(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), engineStyle, properties);
    });
}

bool ClipCanvasWidget::setCanvasFilterStyle(const ClipCanvasFilterStyle& style,
                                            quint32 properties) {
    return m_impl->setCanvasFilterStyle(style, properties);
}

bool ClipCanvasWidget::Impl::setCanvasFilterCreationStyle(const ClipCanvasFilterStyle& style,
                                                          quint32 properties,
                                                          ClipCanvasTool filterTool) {
    if (filterTool != ClipCanvasTool::RectangleFilter && filterTool != ClipCanvasTool::PenFilter) {
        return false;
    }
    const ClipFilterStyle engineStyle{static_cast<ClipFilterType>(style.type), style.strength,
                                      style.opacity, style.strokeWidth};
    const auto result = clip_canvas_commands::setFilterCreationStyle(
        runtimeBinding.engine(), runtimeBinding.viewportHandle(), engineStyle, properties,
        clip_canvas_types::toEngineTool(filterTool));
    if (!result.success) {
        return false;
    }
    syncChangedViewports(result.changedViewports.get());
    return true;
}

bool ClipCanvasWidget::setCanvasFilterCreationStyle(const ClipCanvasFilterStyle& style,
                                                    quint32 properties, ClipCanvasTool filterTool) {
    return m_impl->setCanvasFilterCreationStyle(style, properties, filterTool);
}

bool ClipCanvasWidget::Impl::setCanvasBrushEraserCreationStyle(
    const ClipCanvasBrushEraserStyle& style, quint32 properties) {
    const auto result = clip_canvas_commands::setBrushEraserCreationStyle(
        runtimeBinding.engine(), runtimeBinding.viewportHandle(),
        ClipBrushEraserStyle{style.strokeWidth}, properties);
    if (!result.success) {
        return false;
    }
    syncChangedViewports(result.changedViewports.get());
    return true;
}

bool ClipCanvasWidget::setCanvasBrushEraserCreationStyle(const ClipCanvasBrushEraserStyle& style,
                                                         quint32 properties) {
    return m_impl->setCanvasBrushEraserCreationStyle(style, properties);
}

bool ClipCanvasWidget::Impl::setCanvasTextStyle(const ClipCanvasTextStyle& style,
                                                quint32 properties) {
    bool toolbarStateChanged = false;
    const bool applied = withPresentationUpdate([&]() {
        const ClipTextStyle engineStyle = clip_canvas_types::toEngineTextStyle(style);
        ClipCanvasWidgetTextInteraction::StyleChangeResult result =
            textInteraction.applyTextStyle(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                                           displayState.displayCache(), engineStyle, properties);
        if (!result.success) {
            return false;
        }

        syncChangedViewports(result.changedViewports.get());
        toolbarStateChanged = result.toolbarStateChanged;
        return true;
    });
    if (applied && toolbarStateChanged) {
        emit widget.styleToolbarStateChanged();
    }
    return applied;
}

bool ClipCanvasWidget::setCanvasTextStyle(const ClipCanvasTextStyle& style, quint32 properties) {
    return m_impl->setCanvasTextStyle(style, properties);
}

bool ClipCanvasWidget::Impl::setCanvasSerialNumberStyle(const ClipCanvasSerialNumberStyle& style) {
    return applyMutation([&]() {
        return clip_canvas_commands::setSerialNumberStyle(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(),
            clip_canvas_types::toEngineSerialNumberStyle(style));
    });
}

bool ClipCanvasWidget::setCanvasSerialNumberStyle(const ClipCanvasSerialNumberStyle& style) {
    return m_impl->setCanvasSerialNumberStyle(style);
}

ClipCanvasHistoryState ClipCanvasWidget::Impl::canvasHistoryState() const {
    return clip_canvas_types::toCanvasHistoryState(displayState.snapshot().historyState);
}

ClipCanvasHistoryState ClipCanvasWidget::canvasHistoryState() const {
    return m_impl->canvasHistoryState();
}

ClipCanvasSnapConfig ClipCanvasWidget::Impl::canvasSnapConfig() const {
    return clip_canvas_types::toCanvasSnapConfig(displayState.snapshot().snapConfig);
}

ClipCanvasSnapConfig ClipCanvasWidget::canvasSnapConfig() const {
    return m_impl->canvasSnapConfig();
}

bool ClipCanvasWidget::Impl::setCanvasSnapConfig(const ClipCanvasSnapConfig& config) {
    return applyPairedMutation([&]() {
        return clip_canvas_commands::setSnapConfig(runtimeBinding.engine(),
                                                   runtimeBinding.viewportHandle(),
                                                   clip_canvas_types::toEngineSnapConfig(config));
    });
}

bool ClipCanvasWidget::setCanvasSnapConfig(const ClipCanvasSnapConfig& config) {
    return m_impl->setCanvasSnapConfig(config);
}

bool ClipCanvasWidget::Impl::setCanvasSnapGuideTargets(const ClipCanvasSnapGuideTargets& targets) {
    const auto finite = [](qreal value) { return std::isfinite(value); };
    if (!hasViewport() || targets.verticalXs.size() > 2 || targets.horizontalYs.size() > 2 ||
        !std::all_of(targets.verticalXs.cbegin(), targets.verticalXs.cend(), finite) ||
        !std::all_of(targets.horizontalYs.cbegin(), targets.horizontalYs.cend(), finite)) {
        return false;
    }
    if (snapGuideTargets && snapGuideTargets->verticalXs == targets.verticalXs &&
        snapGuideTargets->horizontalYs == targets.horizontalYs) {
        return true;
    }

    const auto result = clip_canvas_commands::setSnapGuideTargets(
        runtimeBinding.engine(), runtimeBinding.viewportHandle(), targets.verticalXs.constData(),
        static_cast<size_t>(targets.verticalXs.size()), targets.horizontalYs.constData(),
        static_cast<size_t>(targets.horizontalYs.size()));
    if (!result.success) {
        return false;
    }
    snapGuideTargets = targets;
    // Guides are viewport metadata. They cannot change text layout and must not take
    // keyboard focus from a host control whenever the pointer moves.
    if (clip_changed_viewports_count(result.changedViewports.get()) > 0) {
        runtimeBinding.syncChangedViewports(result.changedViewports.get());
    }
    return true;
}

bool ClipCanvasWidget::setCanvasSnapGuideTargets(const ClipCanvasSnapGuideTargets& targets) {
    return m_impl->setCanvasSnapGuideTargets(targets);
}

ClipCanvasGridConfig ClipCanvasWidget::Impl::canvasGridConfig() const {
    return clip_canvas_types::toCanvasGridConfig(displayState.snapshot().gridConfig);
}

ClipCanvasGridConfig ClipCanvasWidget::canvasGridConfig() const {
    return m_impl->canvasGridConfig();
}

bool ClipCanvasWidget::Impl::setCanvasGridConfig(const ClipCanvasGridConfig& config) {
    return applyPairedMutation([&]() {
        return clip_canvas_commands::setGridConfig(runtimeBinding.engine(),
                                                   runtimeBinding.viewportHandle(),
                                                   clip_canvas_types::toEngineGridConfig(config));
    });
}

bool ClipCanvasWidget::setCanvasGridConfig(const ClipCanvasGridConfig& config) {
    return m_impl->setCanvasGridConfig(config);
}

bool ClipCanvasWidget::Impl::interactionEnabled() const {
    return inputHandler.interactionEnabled();
}

bool ClipCanvasWidget::interactionEnabled() const {
    return m_impl->interactionEnabled();
}

bool ClipCanvasWidget::hasQuickSelectionTargetAt(const QPointF& viewPosition,
                                                 Qt::MouseButton button) const {
    return m_impl->hasQuickSelectionTargetAt(viewPosition, button);
}

bool ClipCanvasWidget::Impl::hasQuickSelectionTargetAt(const QPointF& viewPosition,
                                                       Qt::MouseButton button) const {
    if (!interactionEnabled() || !hasViewport() ||
        (button != Qt::LeftButton && button != Qt::RightButton)) {
        return false;
    }
    const QPointF canvasPoint = canvasToViewTransform().inverted().map(viewPosition);
    ClipElementId id{};
    std::uint8_t hit = 0;
    return clip_viewport_hit_quick_selection(
               runtimeBinding.engine(), runtimeBinding.viewportHandle(), canvasPoint.x(),
               canvasPoint.y(), clip_canvas_input::mapButton(button), &id, &hit) == CLIP_OK &&
           hit != 0;
}

void ClipCanvasWidget::Impl::updateMouseContextMenuOwnership(const QMouseEvent* event,
                                                             bool handled) {
    if (event != nullptr && event->button() == Qt::RightButton) {
        // Creation and text-edit completion can consume a right press without a selection hit.
        // Keep ownership through its release, even when the operation has already finished.
        suppressMouseContextMenu = handled;
    }
}

bool ClipCanvasWidget::Impl::handleContextMenu(QContextMenuEvent* event) {
    if (event->reason() != QContextMenuEvent::Mouse || !suppressMouseContextMenu) {
        return false;
    }
    suppressMouseContextMenu = false;
    event->accept();
    return true;
}

void ClipCanvasWidget::Impl::setInteractionEnabled(bool enabled) {
    if (!enabled) {
        ++liveStrokeMoveFlushGeneration;
        liveStrokeMoveFlushScheduled = false;
        pendingLiveStrokeMoves.clear();
        pendingLiveStrokePreservesEverySample = false;
        pendingEraserMove.reset();
    }
    inputHandler.setInteractionEnabled(enabled);
    refreshSerialNumberToolbar();
}

void ClipCanvasWidget::setInteractionEnabled(bool enabled) {
    m_impl->setInteractionEnabled(enabled);
}

bool ClipCanvasWidget::Impl::wheelZoomEnabled() const {
    return m_wheelZoomEnabled;
}

bool ClipCanvasWidget::wheelZoomEnabled() const {
    return m_impl->wheelZoomEnabled();
}

void ClipCanvasWidget::Impl::setWheelZoomEnabled(bool enabled) {
    m_wheelZoomEnabled = enabled;
}

void ClipCanvasWidget::setWheelZoomEnabled(bool enabled) {
    m_impl->setWheelZoomEnabled(enabled);
}

bool ClipCanvasWidget::Impl::canvasContentVisible() const {
    return canvasContentIsVisible;
}

bool ClipCanvasWidget::canvasContentVisible() const {
    return m_impl->canvasContentVisible();
}

void ClipCanvasWidget::Impl::setCanvasContentVisible(bool visible) {
    if (canvasContentIsVisible == visible) {
        return;
    }
    canvasContentIsVisible = visible;
    clip_canvas_filter_tile_cache::invalidateNamespace(&widget);
    refreshSerialNumberToolbar();
    widget.update();
}

void ClipCanvasWidget::setCanvasContentVisible(bool visible) {
    m_impl->setCanvasContentVisible(visible);
}

bool ClipCanvasWidget::Impl::clearBackgroundEnabled() const {
    return canvasClearBackgroundEnabled;
}

bool ClipCanvasWidget::clearBackgroundEnabled() const {
    return m_impl->clearBackgroundEnabled();
}

void ClipCanvasWidget::Impl::setClearBackgroundEnabled(bool enabled) {
    if (canvasClearBackgroundEnabled == enabled) {
        return;
    }
    canvasClearBackgroundEnabled = enabled;
    widget.update();
}

void ClipCanvasWidget::setClearBackgroundEnabled(bool enabled) {
    m_impl->setClearBackgroundEnabled(enabled);
}

bool ClipCanvasWidget::Impl::showDirtyRects() const {
    return dirtyRectsVisible;
}

bool ClipCanvasWidget::showDirtyRects() const {
    return m_impl->showDirtyRects();
}

std::uint64_t ClipCanvasWidget::Impl::viewportId() const {
    return runtimeBinding.viewportId();
}

std::uint64_t ClipCanvasWidget::Impl::runtimeViewportId() const {
    return viewportId();
}

std::uint64_t ClipCanvasWidget::viewportId() const {
    return m_impl->viewportId();
}

bool ClipCanvasWidget::Impl::setViewportCamera(double centerX, double centerY, double zoom) {
    return setCameraAndSync(centerX, centerY, zoom);
}

bool ClipCanvasWidget::setViewportCamera(double centerX, double centerY, double zoom) {
    return m_impl->setViewportCamera(centerX, centerY, zoom);
}

bool ClipCanvasWidget::Impl::hasWatermarkRenderArea() const {
    return configuredWatermarkRenderArea.has_value();
}

bool ClipCanvasWidget::hasWatermarkRenderArea() const {
    return m_impl->hasWatermarkRenderArea();
}

QRectF ClipCanvasWidget::Impl::watermarkRenderArea() const {
    return configuredWatermarkRenderArea.value_or(QRectF());
}

QRectF ClipCanvasWidget::watermarkRenderArea() const {
    return m_impl->watermarkRenderArea();
}

QRectF ClipCanvasWidget::Impl::watermarkViewRenderArea() const {
    if (!configuredWatermarkRenderArea.has_value()) {
        return QRectF(widget.rect());
    }
    const QRectF canvasArea = configuredWatermarkRenderArea->normalized();
    if (!canvasArea.isValid() || canvasArea.isEmpty()) {
        return {};
    }
    const ClipCanvasDisplayCache& cache = displayState.displayCache();
    if (!displayCacheHasSynced(cache)) {
        return {};
    }
    return ::canvasToViewTransform(cache.sceneInfo()).mapRect(canvasArea);
}

QRegion ClipCanvasWidget::Impl::watermarkViewRenderRegion() const {
    if (!configuredWatermarkRenderArea.has_value()) {
        return QRegion(widget.rect());
    }
    const QRectF viewArea = watermarkViewRenderArea();
    return viewArea.isValid() && !viewArea.isEmpty()
               ? QRegion(viewArea.toAlignedRect().intersected(widget.rect()))
               : QRegion();
}

bool ClipCanvasWidget::Impl::watermarkEffectivelyVisible() const {
    updateWatermarkPresentationInfo();
    const WatermarkDisplayInfo& info = watermarkPresentationInfo;
    return info.watermark_text_len != 0 && info.watermark_color.a != 0 &&
           std::isfinite(info.watermark_font_size) && info.watermark_font_size > 0.0 &&
           std::isfinite(info.watermark_opacity) && info.watermark_opacity > 0.0;
}

bool ClipCanvasWidget::Impl::spotlightEffectivelyVisible() const {
    spotlightPresentationInfo = displayState.displayCache().spotlightInfo();
    if (spotlightPreview.has_value()) {
        spotlightPresentationInfo.color = clip_canvas_types::toEngineColor(spotlightPreview->color);
        spotlightPresentationInfo.opacity = spotlightPreview->opacity;
    }
    const SpotlightDisplayInfo& info = spotlightPresentationInfo;
    return info.active && info.color.a != 0 && std::isfinite(info.opacity) && info.opacity > 0.0;
}

void ClipCanvasWidget::Impl::setDecorationRenderAreas(
    const ClipCanvasDecorationRenderAreas& areas) {
    const std::optional<QRectF> nextWatermarkArea =
        areas.watermark.has_value() ? std::optional<QRectF>(areas.watermark->normalized())
                                    : std::nullopt;
    const std::optional<QRectF> nextSpotlightArea =
        areas.spotlight.has_value() ? std::optional<QRectF>(areas.spotlight->normalized())
                                    : std::nullopt;
    if (configuredWatermarkRenderArea == nextWatermarkArea &&
        configuredSpotlightRenderArea == nextSpotlightArea) {
        return;
    }

    const QRegion previousWatermark = watermarkViewRenderRegion();
    const QRegion previousSpotlight = spotlightViewRenderRegion();
    const bool watermarkWasVisible = watermarkEffectivelyVisible();
    const bool spotlightWasVisible = spotlightEffectivelyVisible();

    configuredWatermarkRenderArea = nextWatermarkArea;
    configuredSpotlightRenderArea = nextSpotlightArea;

    const QRegion nextWatermark = watermarkViewRenderRegion();
    const QRegion nextSpotlight = spotlightViewRenderRegion();
    const bool watermarkIsVisible = watermarkEffectivelyVisible();
    const bool spotlightIsVisible = spotlightEffectivelyVisible();
    QRegion changed;
    if (watermarkWasVisible || watermarkIsVisible) {
        changed += previousWatermark.united(nextWatermark);
    }
    if (spotlightWasVisible || spotlightIsVisible) {
        changed += previousSpotlight.xored(nextSpotlight);
    }
    if (!changed.isEmpty()) {
        widget.update(changed);
    }
}

void ClipCanvasWidget::Impl::setWatermarkRenderArea(const QRectF& canvasRect) {
    setDecorationRenderAreas(ClipCanvasDecorationRenderAreas{
        std::optional<QRectF>(canvasRect.normalized()),
        configuredSpotlightRenderArea,
    });
}

void ClipCanvasWidget::setWatermarkRenderArea(const QRectF& canvasRect) {
    m_impl->setWatermarkRenderArea(canvasRect);
}

void ClipCanvasWidget::setDecorationRenderAreas(const ClipCanvasDecorationRenderAreas& areas) {
    m_impl->setDecorationRenderAreas(areas);
}

void ClipCanvasWidget::Impl::clearWatermarkRenderArea() {
    setDecorationRenderAreas(ClipCanvasDecorationRenderAreas{
        std::nullopt,
        configuredSpotlightRenderArea,
    });
}

void ClipCanvasWidget::clearWatermarkRenderArea() {
    m_impl->clearWatermarkRenderArea();
}

bool ClipCanvasWidget::Impl::hasSpotlightRenderArea() const {
    return configuredSpotlightRenderArea.has_value();
}

bool ClipCanvasWidget::hasSpotlightRenderArea() const {
    return m_impl->hasSpotlightRenderArea();
}

QRectF ClipCanvasWidget::Impl::spotlightRenderArea() const {
    return configuredSpotlightRenderArea.value_or(QRectF());
}

QRectF ClipCanvasWidget::spotlightRenderArea() const {
    return m_impl->spotlightRenderArea();
}

QRectF ClipCanvasWidget::Impl::spotlightViewRenderArea() const {
    if (!configuredSpotlightRenderArea.has_value()) {
        return QRectF(widget.rect());
    }
    const QRectF canvasArea = configuredSpotlightRenderArea->normalized();
    if (!canvasArea.isValid() || canvasArea.isEmpty()) {
        return {};
    }
    const ClipCanvasDisplayCache& cache = displayState.displayCache();
    return displayCacheHasSynced(cache)
               ? ::canvasToViewTransform(cache.sceneInfo()).mapRect(canvasArea)
               : QRectF();
}

QRegion ClipCanvasWidget::Impl::spotlightViewRenderRegion() const {
    const QRectF area = spotlightViewRenderArea();
    return area.isValid() && !area.isEmpty()
               ? QRegion(area.toAlignedRect().intersected(widget.rect()))
               : QRegion();
}

void ClipCanvasWidget::Impl::setSpotlightRenderArea(const QRectF& canvasRect) {
    setDecorationRenderAreas(ClipCanvasDecorationRenderAreas{
        configuredWatermarkRenderArea,
        std::optional<QRectF>(canvasRect.normalized()),
    });
}

void ClipCanvasWidget::setSpotlightRenderArea(const QRectF& canvasRect) {
    m_impl->setSpotlightRenderArea(canvasRect);
}

void ClipCanvasWidget::Impl::clearSpotlightRenderArea() {
    setDecorationRenderAreas(ClipCanvasDecorationRenderAreas{
        configuredWatermarkRenderArea,
        std::nullopt,
    });
}

void ClipCanvasWidget::clearSpotlightRenderArea() {
    m_impl->clearSpotlightRenderArea();
}

ClipCanvasCustomRenderer* ClipCanvasWidget::Impl::customRenderer() const {
    return installedCustomRenderer;
}

ClipCanvasCustomRenderer* ClipCanvasWidget::customRenderer() const {
    return m_impl->customRenderer();
}

void ClipCanvasWidget::Impl::setCustomRenderer(ClipCanvasCustomRenderer* renderer) {
    if (installedCustomRenderer == renderer) {
        return;
    }
    referenceScene.reset();
    installedCustomRenderer = renderer;
    clip_canvas_filter_tile_cache::invalidateNamespace(&widget);
    widget.update();
}

void ClipCanvasWidget::setCustomRenderer(ClipCanvasCustomRenderer* renderer) {
    m_impl->setCustomRenderer(renderer);
}

QTransform ClipCanvasWidget::Impl::canvasToViewTransform() const {
    const ClipCanvasDisplayCache& cache = displayState.displayCache();
    return displayCacheHasSynced(cache) ? ::canvasToViewTransform(cache.sceneInfo())
                                        : QTransform(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}

QTransform ClipCanvasWidget::canvasToViewTransform() const {
    return m_impl->canvasToViewTransform();
}

QRect ClipCanvasWidget::Impl::viewRectForCanvasRect(const QRectF& canvasRect, int paddingPx) const {
    const QRectF normalized = canvasRect.normalized();
    if (!normalized.isValid() || normalized.isEmpty()) {
        return {};
    }

    const ClipCanvasDisplayCache& cache = displayState.displayCache();
    if (!displayCacheHasSynced(cache)) {
        return {};
    }
    return clip_canvas_render_geometry::alignedRectForBounds(
        ::canvasToViewTransform(cache.sceneInfo()).mapRect(normalized), paddingPx);
}

QRect ClipCanvasWidget::viewRectForCanvasRect(const QRectF& canvasRect, int paddingPx) const {
    return m_impl->viewRectForCanvasRect(canvasRect, paddingPx);
}

void ClipCanvasWidget::Impl::detachRuntime() {
    clearRetainedDisplayState();
    runtimeBinding.detachAttachment();
}

void ClipCanvasWidget::Impl::detachRuntimeForReplacement() {
    detachRuntime();
}

void ClipCanvasWidget::Impl::detachRuntimeOwner(ClipCanvasRuntime* owner) {
    if (!runtimeBinding.isRuntimeOwner(owner)) {
        return;
    }

    clearRetainedDisplayState();
    runtimeBinding.detachRuntimeOwner(owner);
}

void ClipCanvasWidget::Impl::attachRuntime(ClipRuntime runtime) {
    snapGuideTargets.reset();
    runtimeBinding.attachRuntime(runtime, displayState);
    setSurfaceSizeAndSync(widget.size(), false);
    refreshSerialNumberToolbar();
    widget.update();
}

void ClipCanvasWidget::Impl::clearRetainedDisplayState() {
    snapGuideTargets.reset();
    referenceScene.reset();
    if (pendingLiveStrokePreservesEverySample && hasViewport()) {
        flushLiveStrokeMoves();
    }
    clearTextStylePopupInteraction();
    std::vector<ClipInputEvent>().swap(pendingLiveStrokeMoves);
    pendingLiveStrokePreservesEverySample = false;
    pendingEraserMove.reset();
    pendingWatermarkPreview.reset();
    watermarkPreview.reset();
    watermarkPreviewTimer.stop();
    watermarkPreviewScheduled = false;
    pendingSpotlightPreview.reset();
    spotlightPreview.reset();
    spotlightPreviewTimer.stop();
    spotlightPreviewScheduled = false;
    const bool textSessionWasActive = textInteraction.isActive();
    const ClipCanvasWidgetTextInteraction::CancelResult cancelResult = textInteraction.cancel(
        runtimeBinding.engine(), runtimeBinding.viewportHandle(), displayState.displayCache());
    if (cancelResult.sessionEnded) {
        syncChangedViewports(cancelResult.changedViewports.get());
    }
    displayState.resetRetainedState();
    textInteraction.resetDocumentRetainedState();
    if (textSessionWasActive) {
        emit widget.styleToolbarStateChanged();
    }
    inputHandler.clearTransientState();
    refreshSerialNumberToolbar();
    clearRenderState();
}

void ClipCanvasWidget::Impl::clearRenderState() {
    referenceScene.clearRenderState();
    displayState.displayCache().clearRenderState();
    clip_canvas_filter_tile_cache::invalidateNamespace(&widget);
    filterWorkspace.clear();
    penMaskAtlas.clear();
    if (installedCustomRenderer != nullptr) {
        installedCustomRenderer->clearRenderState();
    }
}

void ClipCanvasWidget::Impl::resetDocumentRetainedState() {
    snapGuideTargets.reset();
    displayState.resetDocumentRetainedState();
    textInteraction.resetDocumentRetainedState();
    std::vector<ClipInputEvent>().swap(pendingLiveStrokeMoves);
    clearRenderState();
}

bool ClipCanvasWidget::Impl::hasViewport() const {
    return runtimeBinding.hasViewport();
}

bool ClipCanvasWidget::Impl::undo() {
    return applyMutation([&]() { return clip_canvas_commands::undo(runtimeBinding.engine()); });
}

bool ClipCanvasWidget::undo() {
    return m_impl->undo();
}

bool ClipCanvasWidget::Impl::redo() {
    return applyMutation([&]() { return clip_canvas_commands::redo(runtimeBinding.engine()); });
}

bool ClipCanvasWidget::redo() {
    return m_impl->redo();
}

bool ClipCanvasWidget::Impl::deleteSelected() {
    return applyMutation([&]() {
        return clip_canvas_commands::deleteSelected(runtimeBinding.engine(),
                                                    runtimeBinding.viewportHandle());
    });
}

bool ClipCanvasWidget::deleteSelected() {
    return m_impl->deleteSelected();
}

bool ClipCanvasWidget::Impl::deleteAllElements() {
    return applyMutation([&]() {
        return clip_canvas_commands::deleteAllElements(runtimeBinding.engine(),
                                                       runtimeBinding.viewportHandle());
    });
}

bool ClipCanvasWidget::deleteAllElements() {
    return m_impl->deleteAllElements();
}

bool ClipCanvasWidget::Impl::clearDocument() {
    const ClipCanvasTool previousTool = canvasTool();
    return runtimeBinding.clearDocumentPreservingViewports() && setCanvasTool(previousTool);
}

bool ClipCanvasWidget::clearDocument() {
    return m_impl->clearDocument();
}

bool ClipCanvasWidget::Impl::duplicateSelected(const QPointF& offset) {
    return applyMutation([&]() {
        return clip_canvas_commands::duplicateSelected(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), offset.x(), offset.y());
    });
}

bool ClipCanvasWidget::duplicateSelected(const QPointF& offset) {
    return m_impl->duplicateSelected(offset);
}

bool ClipCanvasWidget::insertDrawTemplate(const QByteArray& payload, const QPointF& center) {
    return m_impl->insertDrawTemplate(payload, center);
}

bool ClipCanvasWidget::Impl::insertDrawTemplate(const QByteArray& payload, const QPointF& center) {
    return applyMutation([&]() {
        return clip_canvas_commands::insertDrawTemplate(runtimeBinding.engine(),
                                                        runtimeBinding.viewportHandle(), payload,
                                                        center.x(), center.y());
    });
}

bool ClipCanvasWidget::Impl::reorderSelected(ClipCanvasSelectionOrder order) {
    return applyMutation([&]() {
        return clip_canvas_commands::reorderSelected(runtimeBinding.engine(),
                                                     runtimeBinding.viewportHandle(),
                                                     static_cast<std::uint32_t>(order));
    });
}

bool ClipCanvasWidget::reorderSelected(ClipCanvasSelectionOrder order) {
    return m_impl->reorderSelected(order);
}

bool ClipCanvasWidget::Impl::alignSelected(ClipCanvasSelectionAlignment alignment) {
    return applyMutation([&]() {
        return clip_canvas_commands::alignSelected(runtimeBinding.engine(),
                                                   runtimeBinding.viewportHandle(),
                                                   static_cast<std::uint32_t>(alignment));
    });
}

bool ClipCanvasWidget::alignSelected(ClipCanvasSelectionAlignment alignment) {
    return m_impl->alignSelected(alignment);
}

bool ClipCanvasWidget::Impl::setSelectedOpacity(double opacity) {
    return applyMutation([&]() {
        return clip_canvas_commands::setSelectedOpacity(runtimeBinding.engine(),
                                                        runtimeBinding.viewportHandle(), opacity);
    });
}

bool ClipCanvasWidget::setSelectedOpacity(double opacity) {
    return m_impl->setSelectedOpacity(opacity);
}

bool ClipCanvasWidget::Impl::adjustSelectedSerialNumbers(qint64 delta) {
    return applyMutation([&]() {
        return clip_canvas_commands::adjustSelectedSerialNumbers(runtimeBinding.engine(),
                                                                 runtimeBinding.viewportHandle(),
                                                                 static_cast<std::int64_t>(delta));
    });
}

bool ClipCanvasWidget::adjustSelectedSerialNumbers(qint64 delta) {
    return m_impl->adjustSelectedSerialNumbers(delta);
}

bool ClipCanvasWidget::Impl::createSerialNumberText() {
    const ClipStyleToolbarState& styleState = displayState.snapshot().styleToolbarState;
    clip_canvas_commands::CreateSerialNumberTextResult createResult =
        textInteraction.createSerialNumberText(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), styleState.text_style);
    if (!createResult.success) {
        return false;
    }

    // The editor session must begin from the label's styled scene item, the
    // same authority the serial drag's release path uses. Sync the creation
    // into the display cache first; beginning before the sync leaves the
    // editor a style-less preview whose commit strips the label's fill,
    // color, and stroke.
    syncChangedViewports(createResult.changedViewports.get());
    ClipCanvasWidgetTextInteraction::BeginResult beginResult =
        textInteraction.beginCreatedText(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                                         createResult, displayState.displayCache());
    syncChangedViewports(beginResult.firstChangedViewports.get());
    if (beginResult.started) {
        refocusWidget();
    }
    return true;
}

bool ClipCanvasWidget::Impl::beginArrowText(const QPointF& viewPosition, bool selected) {
    if (!interactionEnabled() || textInteraction.isActive()) {
        return false;
    }
    auto result =
        textInteraction.beginArrow(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                                   displayState.displayCache(), viewPosition, selected);
    syncChangedViewports(result.firstChangedViewports.get());
    syncChangedViewports(result.secondChangedViewports.get());
    if (result.started) {
        emit widget.styleToolbarStateChanged();
    }
    return result.started;
}

bool ClipCanvasWidget::editSelectedArrowText() {
    return m_impl->beginArrowText(QPointF(), true);
}

bool ClipCanvasWidget::createSerialNumberText() {
    return m_impl->createSerialNumberText();
}

bool ClipCanvasWidget::Impl::resetEditingState(bool restoreSelectTool) {
    const ClipCanvasTool previousTool = canvasTool();
    if (pendingLiveStrokePreservesEverySample) {
        flushLiveStrokeMoves();
    } else {
        pendingLiveStrokeMoves.clear();
    }
    pendingEraserMove.reset();

    clearTextStylePopupInteraction();
    if (textInteraction.isActive()) {
        commitText(false, false);
    }

    const QRegion previewRegion = watermarkViewRenderRegion().united(spotlightViewRenderRegion());
    pendingWatermarkPreview.reset();
    watermarkPreview.reset();
    watermarkPreviewTimer.stop();
    watermarkPreviewScheduled = false;
    pendingSpotlightPreview.reset();
    spotlightPreviewTimer.stop();
    spotlightPreviewScheduled = false;
    spotlightPreview.reset();

    if (!applyMutation([&]() {
            return clip_canvas_commands::resetEditingState(runtimeBinding.engine(),
                                                           runtimeBinding.viewportHandle());
        })) {
        return false;
    }

    clearRenderState();
    inputHandler.clearTransientState();
    if (restoreSelectTool) {
        requestedCanvasTool = ClipCanvasTool::Select;
        applyCanvasToolCursor(ClipCanvasTool::Select);
    } else if (previousTool != ClipCanvasTool::Select) {
        // The engine reset also selects the Select tool. Export callers need
        // the editing cleanup while preserving the user's active tool.
        if (!setCanvasTool(previousTool)) {
            return false;
        }
    }
    refreshSerialNumberToolbar();
    if (!previewRegion.isEmpty()) {
        widget.update(previewRegion);
    }
    return true;
}

bool ClipCanvasWidget::resetEditingState() {
    return m_impl->resetEditingState(true);
}

bool ClipCanvasWidget::resetEditingStatePreservingTool() {
    return m_impl->resetEditingState(false);
}

void ClipCanvasWidget::clearRenderState() {
    m_impl->clearRenderState();
}

bool ClipCanvasWidget::Impl::cancelActiveTextEditing() {
    ClipCanvasWidgetTextInteraction::CancelResult result = textInteraction.cancel(
        runtimeBinding.engine(), runtimeBinding.viewportHandle(), displayState.displayCache());
    if (!result.sessionEnded) {
        return false;
    }

    clearTextStylePopupInteraction();
    syncChangedViewports(result.changedViewports.get());
    emit widget.styleToolbarStateChanged();
    return true;
}

bool ClipCanvasWidget::cancelActiveTextEditing() {
    return m_impl->cancelActiveTextEditing();
}

bool ClipCanvasWidget::Impl::hasActiveTextEditing() const {
    return textInteraction.isActive();
}

bool ClipCanvasWidget::hasActiveTextEditing() const {
    return m_impl->hasActiveTextEditing();
}

void ClipCanvasWidget::Impl::beginTextStylePopupInteraction() {
    ++textStylePopupInteractionDepth;
    ++textStylePopupFocusRestoreGeneration;
}

void ClipCanvasWidget::beginTextStylePopupInteraction() {
    m_impl->beginTextStylePopupInteraction();
}

void ClipCanvasWidget::Impl::endTextStylePopupInteraction(QWidget* focusScope) {
    if (textStylePopupInteractionDepth <= 0) {
        return;
    }

    --textStylePopupInteractionDepth;
    if (textStylePopupInteractionDepth != 0) {
        return;
    }

    textStylePopupFocusScope = focusScope;
    const quint64 generation = ++textStylePopupFocusRestoreGeneration;
    QTimer::singleShot(0, &widget, [this, generation]() {
        if (generation != textStylePopupFocusRestoreGeneration ||
            textStylePopupInteractionDepth != 0 || !textInteraction.isActive() ||
            QGuiApplication::applicationState() != Qt::ApplicationActive) {
            return;
        }

        QWidget* focusWidget = QApplication::focusWidget();
        const bool focusMayBeRestored =
            focusWidget == nullptr || !focusWidget->isVisible() ||
            focusWidgetIsWithin(focusWidget, textStylePopupFocusScope.data());
        if (focusMayBeRestored) {
            widget.setFocus(Qt::OtherFocusReason);
        }
    });
}

void ClipCanvasWidget::endTextStylePopupInteraction(QWidget* focusScope) {
    m_impl->endTextStylePopupInteraction(focusScope);
}

void ClipCanvasWidget::Impl::clearTextStylePopupInteraction() {
    textStylePopupInteractionDepth = 0;
    textStylePopupFocusScope = nullptr;
    ++textStylePopupFocusRestoreGeneration;
}

void ClipCanvasWidget::Impl::handleApplicationStateChanged(Qt::ApplicationState state) {
    if (state == Qt::ApplicationActive) {
        return;
    }

    clearTextStylePopupInteraction();
    if (textInteraction.isActive()) {
        commitText(false);
    }
}

ClipCanvasWidgetTextInteraction::CommitResult
ClipCanvasWidget::Impl::commitText(bool refocusWidget, bool restoreExistingSelection) {
    ClipCanvasWidgetTextInteraction::CommitResult result =
        textInteraction.commit(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                               hasViewport(), displayState.displayCache(), refocusWidget);
    applyTextCommitResult(result, restoreExistingSelection);
    return result;
}

void ClipCanvasWidget::Impl::applyTextCommitResult(
    ClipCanvasWidgetTextInteraction::CommitResult& result, bool restoreExistingSelection) {
    syncChangedViewports(result.changedViewports.get());
    if (restoreExistingSelection) {
        result.restoredExistingSelection =
            restoreFinishedExistingTextSelection(result.finishedExistingEdit);
    }
    if (result.sessionEnded) {
        clearTextStylePopupInteraction();
        emit widget.styleToolbarStateChanged();
    }
}

void ClipCanvasWidget::Impl::applyTextEditorEventResult(
    ClipCanvasWidgetTextInteraction::EditorEventResult& result) {
    syncChangedViewports(result.changedViewports.get());
    restoreFinishedExistingTextSelection(result.finishedExistingEdit);
    if (result.sessionEnded) {
        clearTextStylePopupInteraction();
        emit widget.styleToolbarStateChanged();
    }
}

bool ClipCanvasWidget::Impl::restoreFinishedExistingTextSelection(
    const ClipCanvasWidgetTextInteraction::FinishedExistingEdit& edit) {
    ClipCanvasWidgetTextInteraction::SelectionRestoreResult result =
        textInteraction.restoreFinishedExistingSelection(runtimeBinding.engine(),
                                                         runtimeBinding.viewportHandle(), edit);
    if (result.restored) {
        syncChangedViewports(result.changedViewports.get());
    }
    return result.restored;
}

bool ClipCanvasWidget::Impl::beginText(const QPointF& viewPosition, bool allowCreate) {
    commitText();

    const ClipTextStyle textStyle = displayState.snapshot().styleToolbarState.text_style;
    ClipCanvasWidgetTextInteraction::BeginResult result =
        textInteraction.beginAt(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                                displayState.displayCache(), viewPosition, textStyle, allowCreate);
    syncChangedViewports(result.firstChangedViewports.get());
    syncChangedViewports(result.secondChangedViewports.get());
    return result.started;
}

bool ClipCanvasWidget::Impl::beginSelectedText(const QPointF& viewPosition,
                                               bool requireSerialBoundText) {
    commitText();
    ClipCanvasWidgetTextInteraction::BeginResult result = textInteraction.beginSelectedAt(
        runtimeBinding.engine(), runtimeBinding.viewportHandle(), displayState.displayCache(),
        viewPosition, requireSerialBoundText);
    syncChangedViewports(result.firstChangedViewports.get());
    syncChangedViewports(result.secondChangedViewports.get());
    return result.started;
}

void ClipCanvasWidget::Impl::refreshStateFromEngine(bool emitSignals) {
    if (!hasViewport()) {
        return;
    }

    const quint64 generation = widget.autoFilterGeneration();
    if (autoFilterGeneration != generation) {
        autoFilterGeneration = generation;
        if (emitSignals) {
            emit widget.autoFilterRegionsChanged();
        }
    }
    clip_canvas_state::Changes changes;
    if (!displayState.refreshState(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                                   &changes)) {
        return;
    }

    refreshToolCursorStyle();
    if (emitSignals) {
        emitChangedStateSignals(changes);
    }
    refreshSerialNumberToolbar();
}

void ClipCanvasWidget::Impl::syncAfterEngineMutation() {
    syncAfterEngineMutation(true);
}

void ClipCanvasWidget::Impl::syncAfterEngineMutation(bool emitSignals) {
    if (auto* owner = runtimeBinding.runtimeOwner())
        clip_canvas_runtime::Access::smartErase(*owner).sync(runtimeBinding.engine());
    const std::uint64_t previousSceneRevision =
        displayState.displayCache().patchCursor().scene_revision;
    const clip_canvas_widget_sync::Result result =
        clip_canvas_widget_sync::syncAfterEngineMutation(clip_canvas_widget_sync::Request{
            &displayState,
            &textInteraction.session(),
            runtimeBinding.engine(),
            runtimeBinding.viewportHandle(),
            hasViewport(),
            widget.rect(),
            widget.font(),
            dirtyRectsVisible,
            emitSignals,
            QRegion(),
        });

    const ClipCanvasDisplayCache& cache = displayState.displayCache();
    refreshToolCursorStyle();
    if (cache.patchCursor().scene_revision != previousSceneRevision) {
        // Paint events can coalesce patches, so consume each patch's dirty region
        // before the next display-cache sync replaces it.
        clip_canvas_filter_tile_cache::invalidateRegion(
            &widget,
            clip_canvas_display::dirtyRectsToRegion(cache.sceneDirtyRects(),
                                                    cache.sceneDirtyRectCount(), widget.rect()),
            widget.devicePixelRatioF());
    }
    if (result.shouldEmitStateSignals) {
        emitChangedStateSignals(result.stateChanges);
    }
    clip_canvas_widget_repaint::updateCoalesced(widget, result.repaintRegion);
    refreshSerialNumberToolbar();
}

bool ClipCanvasWidget::Impl::measurePresentationText() {
    const auto labels =
        textInteraction.measureArrowText(runtimeBinding.engine(), runtimeBinding.viewportHandle());
    const auto serial = textInteraction.measureSerialLabelLayout(runtimeBinding.engine(),
                                                                 runtimeBinding.viewportHandle());
    return labels.success && serial.success;
}

bool ClipCanvasWidget::Impl::withPresentationUpdate(const std::function<bool()>& update) {
    ScopedPresentationUpdate scope(runtimeBinding.engine());
    if (!scope.isActive()) {
        return false;
    }
    ScopedChangedViewportList changedViewports;
    bool succeeded = false;
    {
        QScopedValueRollback depth(presentationUpdateDepth, presentationUpdateDepth + 1);
        const bool updated = update();
        const bool measured = presentationUpdateDepth != 1 || measurePresentationText();
        const bool published = scope.finish(changedViewports);
        succeeded = updated && measured && published;
    }
    runtimeBinding.syncChangedViewports(changedViewports.get());
    return succeeded;
}

bool ClipCanvasWidget::Impl::applyMutation(
    const std::function<clip_canvas_commands::MutationResult()>& mutate) {
    const bool applied = withPresentationUpdate([&]() { return mutate().success; });
    if (applied) {
        refocusWidget();
    }
    return applied;
}

bool ClipCanvasWidget::Impl::applyPairedMutation(
    const std::function<clip_canvas_commands::PairedMutationResult()>& mutate) {
    const bool applied = withPresentationUpdate([&]() { return mutate().success; });
    if (applied) {
        refocusWidget();
    }
    return applied;
}

void ClipCanvasWidget::Impl::syncChangedViewports(ClipChangedViewportList changedViewports) {
    if (presentationUpdateDepth != 0) {
        return;
    }
    // External runtime callers retain their immediate publication contract.
    // Group all additional host measurements before synchronizing the widget.
    withPresentationUpdate([]() { return true; });
    runtimeBinding.syncChangedViewports(changedViewports);
}

void ClipCanvasWidget::Impl::refreshArrowTextMetrics() {
    textInteraction.invalidateArrowTextMetrics();
    if (hasViewport()) {
        syncChangedViewports(nullptr);
    }
}

bool ClipCanvasWidget::Impl::applyMutationResult(
    const clip_canvas_commands::MutationResult& result) {
    if (!result.success) {
        return false;
    }

    syncChangedViewports(result.changedViewports.get());
    refocusWidget();
    return true;
}

void ClipCanvasWidget::Impl::emitChangedStateSignals(const clip_canvas_state::Changes& changes) {
    if (changes.activeToolChanged) {
        emit widget.activeToolChanged();
    }
    if (changes.styleToolbarChanged) {
        emit widget.styleToolbarStateChanged();
    }
    if (changes.historyChanged) {
        emit widget.historyStateChanged();
    }
    if (changes.snapConfigChanged) {
        emit widget.snapConfigChanged();
    }
    if (changes.gridConfigChanged) {
        emit widget.gridConfigChanged();
    }
}

void ClipCanvasWidget::Impl::refreshToolCursorStyle() {
    const auto& cursorStyle = displayState.snapshot().styleToolbarState;
    const bool filterCursor = displayState.snapshot().activeTool == CLIP_ACTIVE_TOOL_PEN_FILTER;
    const bool eraserCursor = displayState.snapshot().activeTool == CLIP_ACTIVE_TOOL_BRUSH_ERASER;
    const auto& stroke = cursorStyle.shape_style.stroke;
    QColor cursorColor(stroke.r, stroke.g, stroke.b, stroke.a);
    if (displayState.snapshot().activeTool == CLIP_ACTIVE_TOOL_PEN_HIGHLIGHT) {
        cursorColor.setAlphaF(cursorColor.alphaF() * 0.5F);
    }
    const double strokeWidth = eraserCursor   ? cursorStyle.brush_eraser_style.stroke_width
                               : filterCursor ? cursorStyle.filter_style.stroke_width
                                              : cursorStyle.shape_style.stroke_width;
    cursorController.configureStrokeCursor(
        strokeWidth * displayState.displayCache().sceneInfo().camera_zoom,
        filterCursor || eraserCursor ? std::nullopt : std::optional<QColor>(cursorColor));
}

void ClipCanvasWidget::Impl::applyCanvasToolCursor(ClipCanvasTool tool) {
    const auto cursor = baselineCursorForCanvasTool(tool);
    if (cursor.has_value()) {
        cursorController.setEngineCursor(*cursor);
        return;
    }
    cursorController.clearCursor(ClipCanvasCursorLayer::CanvasTool);
}

void ClipCanvasWidget::Impl::refocusWidget() {
    widget.setFocus(Qt::OtherFocusReason);
}

bool ClipCanvasWidget::Impl::setSurfaceSizeAndSync(const QSize& size, bool emitSignals) {
    if (!hasViewport()) {
        return false;
    }
    if (surfaceSizeMatches(displayState.displayCache(), size)) {
        return true;
    }
    if (!clip_canvas_lifecycle::setSurfaceSize(runtimeBinding.engine(), runtimeBinding.viewport(),
                                               size)) {
        return false;
    }

    syncAfterEngineMutation(emitSignals);
    return true;
}

bool ClipCanvasWidget::Impl::setCameraAndSync(double centerX, double centerY, double zoom) {
    if (!hasViewport()) {
        return false;
    }
    if (cameraMatches(displayState.displayCache(), centerX, centerY, zoom)) {
        return true;
    }
    if (!clip_canvas_lifecycle::setCamera(runtimeBinding.engine(), runtimeBinding.viewport(),
                                          centerX, centerY, zoom)) {
        return false;
    }

    syncAfterEngineMutation(true);
    return true;
}

void ClipCanvasWidget::Impl::paintEvent(QPaintEvent* event) {
    const QRegion exposedRegion = event != nullptr ? event->region() : QRegion(widget.rect());
    if (!hasExposedRect(exposedRegion, widget.rect())) {
        return;
    }

    QPainter painter(&widget);
    paint(painter, exposedRegion);
}

bool ClipCanvasWidget::Impl::paint(QPainter& painter, const QRegion& exposedRegion) {
    if (!hasExposedRect(exposedRegion, widget.rect())) {
        return false;
    }

    const ClipCanvasDisplayCache& cache = displayState.displayCache();

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setClipRegion(exposedRegion);
    clip_canvas_compositor::Frame frame = buildPaintFrame();
    if (canvasContentIsVisible) {
        const ClipCanvasRenderContext tileContext = renderContext(painter, exposedRegion);
        const auto reference = installedCustomRenderer != nullptr
                                   ? installedCustomRenderer->filterRenderReference()
                                   : std::nullopt;
        clip_canvas_renderer::SceneRenderRequest sceneRequest{
            &painter,
            &cache.sceneInfo(),
            frame.sceneItems,
            frame.sceneItemCount,
            exposedRegion,
            nullptr,
            0,
            frame.backgroundImage,
            installedCustomRenderer,
            &tileContext,
            frame.displayCache,
            frame.workspace,
            {},
            nullptr,
            &widget,
            frame.penMaskAtlas,
            true,
            0,
            QPoint(),
            canvasClearBackgroundEnabled,
        };
        if (auto* owner = runtimeBinding.runtimeOwner())
            sceneRequest.smartErase = owner->smartEraseSnapshot();
        if (!reference.has_value())
            referenceScene.reset();
        const bool referenceRendered =
            reference.has_value() &&
            referenceScene.render(runtimeBinding.engine(), *reference, sceneRequest,
                                  tileContext.canvasToViewTransform);
        if (!referenceRendered) {
            sceneRequest.filterTileContentKey = sceneCacheContentKey(
                cache.sceneInfo(), installedCustomRenderer, canvasClearBackgroundEnabled,
                painterDevicePixelRatio(painter, widget), widget.size());
            if (!hasFilter(frame.sceneItems, frame.sceneItemCount)) {
                clip_canvas_compositor::clearSurface(painter, frame);
                renderBeforeCanvas(painter, tileContext);
                sceneRequest.backgroundRenderer = nullptr;
                sceneRequest.backgroundContext = nullptr;
            }
            clip_canvas_renderer::renderSceneItemsTiled(sceneRequest);
        }
        painter.save();
        clip_canvas_compositor::renderDocumentDecorations(painter, frame);
        clip_canvas_compositor::renderEditorOverlays(painter, frame);
        textInteraction.renderEditorOverlay(painter, widget.font(), cache);
        painter.restore();
    } else {
        const ClipCanvasRenderContext context = renderContext(painter, exposedRegion);
        clip_canvas_compositor::clearSurface(painter, frame);
        renderBeforeCanvas(painter, context);
    }
    const ClipCanvasRenderContext context = renderContext(painter, exposedRegion);
    renderAfterCanvas(painter, context);

    return true;
}

clip_canvas_compositor::Frame ClipCanvasWidget::Impl::buildPaintFrame() const {
    const ClipCanvasDisplayCache& cache = displayState.displayCache();
    clip_canvas_compositor::Frame frame =
        clip_canvas_widget_paint_frame::build(clip_canvas_widget_paint_frame::Request{
            &cache,
            &widget,
            canvasClearBackgroundEnabled,
            dirtyRectsVisible,
            const_cast<clip_canvas_filter_render::RenderWorkspace*>(&filterWorkspace),
        });
    frame.penMaskAtlas = &const_cast<ClipCanvasWidget::Impl*>(this)->penMaskAtlas;
    updateWatermarkPresentationInfo();
    frame.watermarkInfo = &watermarkPresentationInfo;
    frame.watermarkRenderArea = watermarkViewRenderArea();
    frame.hasWatermarkRenderArea = configuredWatermarkRenderArea.has_value();
    spotlightPresentationInfo = cache.spotlightInfo();
    if (spotlightPreview.has_value()) {
        spotlightPresentationInfo.color = clip_canvas_types::toEngineColor(spotlightPreview->color);
        spotlightPresentationInfo.opacity = spotlightPreview->opacity;
    }
    frame.spotlightInfo = &spotlightPresentationInfo;
    frame.spotlightRenderArea = spotlightViewRenderArea();
    frame.hasSpotlightRenderArea = configuredSpotlightRenderArea.has_value();
    return frame;
}

void ClipCanvasWidget::Impl::updateWatermarkPresentationInfo() const {
    watermarkPresentationInfo = displayState.displayCache().watermarkInfo();
    if (watermarkPreview.has_value()) {
        applyWatermarkConfig(watermarkPresentationInfo, *watermarkPreview);
    }
}

ClipCanvasRenderContext ClipCanvasWidget::Impl::renderContext(QPainter& painter,
                                                              const QRegion& exposedRegion) const {
    const SceneDisplayInfo& sceneInfo = displayState.displayCache().sceneInfo();
    return ClipCanvasRenderContext{
        widget.rect(),
        exposedRegion.intersected(widget.rect()),
        ::canvasToViewTransform(sceneInfo),
        painterDevicePixelRatio(painter, widget),
    };
}

void ClipCanvasWidget::Impl::renderBeforeCanvas(QPainter& painter,
                                                const ClipCanvasRenderContext& context) const {
    if (installedCustomRenderer == nullptr) {
        return;
    }
    painter.save();
    installedCustomRenderer->renderBeforeCanvas(painter, context);
    painter.restore();
}

void ClipCanvasWidget::Impl::renderAfterCanvas(QPainter& painter,
                                               const ClipCanvasRenderContext& context) const {
    if (installedCustomRenderer == nullptr) {
        return;
    }
    painter.save();
    installedCustomRenderer->renderAfterCanvas(painter, context);
    painter.restore();
}

void ClipCanvasWidget::paintEvent(QPaintEvent* event) {
    m_impl->paintEvent(event);
}

bool ClipCanvasWidget::event(QEvent* event) {
    if (m_impl && event->type() == QEvent::ContextMenu &&
        m_impl->handleContextMenu(static_cast<QContextMenuEvent*>(event))) {
        return true;
    }
    const bool handled = QWidget::event(event);
    if (m_impl && event->type() == QEvent::DevicePixelRatioChange) {
        m_impl->refreshCursorDevicePixelRatio();
    }
    if (m_impl && m_impl->hasViewport() &&
        (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange ||
         event->type() == QEvent::DevicePixelRatioChange ||
         event->type() == QEvent::ScreenChangeInternal)) {
        m_impl->refreshArrowTextMetrics();
    }
    return handled;
}

bool ClipCanvasWidget::eventFilter(QObject* watched, QEvent* event) {
    return QWidget::eventFilter(watched, event);
}

bool ClipCanvasWidget::Impl::handleMousePress(QMouseEvent* event) {
    if (event == nullptr || !interactionEnabled()) {
        return false;
    }

    clearTextStylePopupInteraction();
    const bool rightQuickSelectionEligible =
        event->button() == Qt::RightButton &&
        hasQuickSelectionTargetAt(event->position(), Qt::RightButton);

    const bool textEditorActive = textInteraction.isActive();
    const bool pointerInsideTextEditor =
        textInteraction.editorContains(displayState.displayCache(), event->position());
    const auto selectionTarget = clip_canvas_widget_selection_hit_testing::selectionInteractionAt(
        displayState.displayCache(), event->position());
    using clip_canvas_widget_selection_hit_testing::SelectionInteractionTarget;
    const bool pointerOverSelectionInteraction =
        selectionTarget != SelectionInteractionTarget::None &&
        (!textEditorActive || textInteraction.hasSelectionInteraction());
    const bool pointerHitsSelectedText =
        clip_canvas_widget_pointer_flow::isSelectedTextCopyGesture(event->button(),
                                                                   event->modifiers()) &&
        selectionTarget != SelectionInteractionTarget::Handle &&
        hitsSelectedText(runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                         widget.canvasToViewTransform().inverted().map(event->position()));
    const clip_canvas_widget_pointer_flow::PressPlan plan =
        clip_canvas_widget_pointer_flow::planPress(clip_canvas_widget_pointer_flow::PressRequest{
            true,
            textEditorActive,
            pointerInsideTextEditor,
            canvasTool(),
            event->button(),
            event->modifiers(),
            pointerOverSelectionInteraction,
            suppressNextTextToolCreate,
            restoredSelectionForNextTextToolPress,
            pointerHitsSelectedText,
            selectionTarget,
            rightQuickSelectionEligible,
        });
    if (plan.shouldHandleEditorPress) {
        return textInteraction.handleEditorMousePress(event, displayState.displayCache(),
                                                      widget.font());
    }
    if (!plan.shouldFocusWidget) {
        return false;
    }

    widget.setFocus(Qt::MouseFocusReason);
    suppressNextTextToolCreate = false;
    restoredSelectionForNextTextToolPress = false;
    bool canDispatchAfterTextCommit = true;
    if (plan.shouldCommitTextEditor) {
        const ClipCanvasWidgetTextInteraction::CommitResult commitResult =
            commitText(true, plan.shouldRestoreSelectionOnCommit);
        if (plan.dispatchAfterCommitRequiresRestoredSelection) {
            canDispatchAfterTextCommit = commitResult.restoredExistingSelection ||
                                         plan.suppressedTextCreateRestoredSelection;
        }
    }

    if (plan.shouldBeginText) {
        if (plan.allowCreateText && beginArrowText(event->position(), false)) {
            event->accept();
            return true;
        }
        if (beginText(event->position(), false)) {
            event->accept();
            return true;
        }
    }
    if (plan.shouldBeginSelectedText && !pointerOverSelectionInteraction &&
        beginSelectedText(event->position(), canvasTool() == ClipCanvasTool::SerialNumber)) {
        event->accept();
        return true;
    }
    if (plan.shouldAcceptIfTextBeginFails) {
        if (canvasTool() == ClipCanvasTool::SerialNumber && event->button() == Qt::RightButton) {
            // Committing a label must still let the same blank press reset its sequence.
            static_cast<void>(dispatchInput(
                event, clip_canvas_input::makePointerInput(*event, CLIP_POINTER_EVENT_DOWN)));
        }
        event->accept();
        return true;
    }
    if (plan.shouldAcceptSuppressedTextCreate) {
        event->accept();
        return true;
    }
    if (!plan.shouldDispatchToEngine) {
        return false;
    }
    if (!canDispatchAfterTextCommit) {
        event->accept();
        return true;
    }
    const bool handled =
        dispatchInput(event, clip_canvas_input::makePointerInput(*event, CLIP_POINTER_EVENT_DOWN));
    if (handled && plan.shouldBeginText && plan.allowCreateText) {
        const ClipTextStyle textStyle = displayState.snapshot().styleToolbarState.text_style;
        ClipCanvasWidgetTextInteraction::BeginResult draft =
            textInteraction.beginRequestedNewTextDraft(
                runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                displayState.displayCache(), event->position(), textStyle);
        syncChangedViewports(draft.firstChangedViewports.get());
        syncChangedViewports(draft.secondChangedViewports.get());
    }
    return handled;
}

void ClipCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (canvasTool() == ClipCanvasTool::AutoFilter && event->button() == Qt::LeftButton) {
        emit autoFilterInteractionStarting();
    }
    if (event != nullptr && event->button() == Qt::MiddleButton) {
        m_impl->beginMiddleClickTracking(event->position());
    }
    const bool handled = m_impl->handleMousePress(event);
    m_impl->updateMouseContextMenuOwnership(event, handled);
    if (event != nullptr && event->button() == Qt::MiddleButton && handled) {
        m_impl->cancelMiddleClickTracking();
    }
    if (handled) {
        return;
    }
    QWidget::mousePressEvent(event);
}

bool ClipCanvasWidget::Impl::handleMouseMove(QMouseEvent* event) {
    if (event == nullptr || !interactionEnabled()) {
        return false;
    }
    if (textInteraction.handleEditorMouseMove(event, displayState.displayCache(), widget.font())) {
        return true;
    }
    const ClipInputEvent input =
        clip_canvas_input::makePointerInput(*event, CLIP_POINTER_EVENT_MOVE);
    if ((canvasTool() == ClipCanvasTool::FreeDraw || canvasTool() == ClipCanvasTool::PenFilter ||
         canvasTool() == ClipCanvasTool::BrushEraser) &&
        (event->buttons() & Qt::LeftButton) != 0) {
        return queueLiveStrokeMove(event, input, canvasTool() != ClipCanvasTool::FreeDraw);
    }
    if (canvasTool() == ClipCanvasTool::Eraser && (event->buttons() & Qt::LeftButton) != 0) {
        return queueEraserMove(event, input);
    }
    flushLiveStrokeMoves();
    flushEraserMove();
    return dispatchInput(event, input);
}

bool ClipCanvasWidget::Impl::queueEraserMove(QMouseEvent* event, const ClipInputEvent& input) {
    pendingEraserMove = input;
    if (!eraserMoveFlushScheduled) {
        eraserMoveFlushScheduled = true;
        QTimer::singleShot(0, &widget, [this]() { flushEraserMove(); });
    }
    event->accept();
    return true;
}

void ClipCanvasWidget::Impl::flushEraserMove() {
    eraserMoveFlushScheduled = false;
    if (!pendingEraserMove.has_value()) {
        return;
    }
    const ClipInputEvent input = *pendingEraserMove;
    pendingEraserMove.reset();
    static_cast<void>(processInput(input));
    emit widget.eraserMoveFrameProcessed();
}

bool ClipCanvasWidget::Impl::handleMouseDoubleClick(QMouseEvent* event) {
    if (event == nullptr || !interactionEnabled()) {
        return false;
    }
    if (event->button() == Qt::RightButton &&
        hasQuickSelectionTargetAt(event->position(), Qt::RightButton)) {
        return handleMousePress(event);
    }
    if (textInteraction.isActive()) {
        event->accept();
        return true;
    }
    flushLiveStrokeMoves();
    flushEraserMove();
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier &&
        beginArrowText(event->position(), false)) {
        event->accept();
        return true;
    }
    return dispatchInput(
        event, clip_canvas_input::makePointerInput(*event, CLIP_POINTER_EVENT_DOUBLE_CLICK));
}

void ClipCanvasWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    const bool handled = m_impl->handleMouseDoubleClick(event);
    m_impl->updateMouseContextMenuOwnership(event, handled);
    if (handled) {
        return;
    }
    if (event != nullptr && event->button() == Qt::LeftButton) {
        event->accept();
        emit unhandledLeftDoubleClick();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool ClipCanvasWidget::Impl::queueLiveStrokeMove(QMouseEvent* event, const ClipInputEvent& input,
                                                 bool preserveEverySample) {
    if (!pendingLiveStrokeMoves.empty() &&
        pendingLiveStrokePreservesEverySample != preserveEverySample) {
        flushLiveStrokeMoves();
    }
    pendingLiveStrokePreservesEverySample = preserveEverySample;
    pendingLiveStrokeMoves.push_back(input);
    if (!liveStrokeMoveFlushScheduled) {
        liveStrokeMoveFlushScheduled = true;
        const quint64 generation = ++liveStrokeMoveFlushGeneration;
        QTimer::singleShot(8, &widget, [this, generation]() {
            if (generation == liveStrokeMoveFlushGeneration) {
                flushLiveStrokeMoves();
            }
        });
    }
    event->accept();
    return true;
}

void ClipCanvasWidget::Impl::flushLiveStrokeMoves() {
    ++liveStrokeMoveFlushGeneration;
    liveStrokeMoveFlushScheduled = false;
    if (pendingLiveStrokeMoves.empty()) {
        return;
    }
    const bool preserveEverySample = pendingLiveStrokePreservesEverySample;
    pendingLiveStrokePreservesEverySample = false;
    constexpr std::size_t kMaximumSamplesPerFrame = 96;
    const std::size_t pendingCount = pendingLiveStrokeMoves.size();
    const std::size_t dispatchCount =
        preserveEverySample ? pendingCount : std::min(pendingCount, kMaximumSamplesPerFrame);
    if (dispatchCount < pendingCount) {
        for (std::size_t outputIndex = 0; outputIndex < dispatchCount; ++outputIndex) {
            const std::size_t inputIndex =
                dispatchCount == 1 ? pendingCount - 1
                                   : outputIndex * (pendingCount - 1) / (dispatchCount - 1);
            if (outputIndex != inputIndex) {
                pendingLiveStrokeMoves[outputIndex] = pendingLiveStrokeMoves[inputIndex];
            }
        }
        pendingLiveStrokeMoves.resize(dispatchCount);
    }
    const ClipCanvasWidgetInputHandler::ProcessResult result =
        inputHandler.processBatch(inputHandlerContext(), pendingLiveStrokeMoves);
    if (result.success) {
        runtimeBinding.syncChangedViewports(result.changedViewports.get());
    }
    if (!preserveEverySample) {
        emit widget.freeDrawMoveBatchProcessed(static_cast<quint32>(pendingCount),
                                               static_cast<quint32>(dispatchCount));
    }
    pendingLiveStrokeMoves.clear();
}

void ClipCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event != nullptr) {
        m_impl->updateMiddleClickTracking(event->position());
    }
    if (m_impl->handleMouseMove(event)) {
        return;
    }
    QWidget::mouseMoveEvent(event);
}

bool ClipCanvasWidget::Impl::handleMouseRelease(QMouseEvent* event) {
    if (event == nullptr || !interactionEnabled()) {
        return false;
    }
    if (textInteraction.handleEditorMouseRelease(event)) {
        return true;
    }
    flushLiveStrokeMoves();
    flushEraserMove();
    const bool handled =
        dispatchInput(event, clip_canvas_input::makePointerInput(*event, CLIP_POINTER_EVENT_UP));
    if (handled && event->button() == Qt::LeftButton) {
        auto result = textInteraction.beginRequestedTextEdit(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), displayState.displayCache());
        syncChangedViewports(result.firstChangedViewports.get());
        if (result.started) {
            refocusWidget();
            emit widget.styleToolbarStateChanged();
        }
    }
    return handled;
}

void ClipCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    const bool middleClick = event != nullptr && event->button() == Qt::MiddleButton &&
                             m_impl->finishMiddleClickTracking();
    const bool handled = m_impl->handleMouseRelease(event);
    if (middleClick && !handled) {
        event->accept();
        emit unhandledMiddleClick();
        return;
    }
    if (handled) {
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ClipCanvasWidget::Impl::beginMiddleClickTracking(const QPointF& position) {
    middleClickPressPosition = position;
    middleClickCandidate = true;
}

void ClipCanvasWidget::Impl::updateMiddleClickTracking(const QPointF& position) {
    if (middleClickCandidate && middleClickPressPosition.has_value() &&
        QLineF(*middleClickPressPosition, position).length() > QApplication::startDragDistance()) {
        middleClickCandidate = false;
    }
}

bool ClipCanvasWidget::Impl::finishMiddleClickTracking() {
    const bool candidate = middleClickCandidate;
    middleClickCandidate = false;
    middleClickPressPosition.reset();
    return candidate;
}

void ClipCanvasWidget::Impl::cancelMiddleClickTracking() {
    middleClickCandidate = false;
    middleClickPressPosition.reset();
}

bool ClipCanvasWidget::Impl::handleEnter(QEnterEvent* event) {
    if (event == nullptr || !interactionEnabled()) {
        return false;
    }
    return dispatchInput(event,
                         clip_canvas_input::makePointerInput(
                             event->position(), Qt::NoButton, QGuiApplication::mouseButtons(),
                             QGuiApplication::keyboardModifiers(), CLIP_POINTER_EVENT_ENTER));
}

void ClipCanvasWidget::enterEvent(QEnterEvent* event) {
    if (m_impl->handleEnter(event)) {
        return;
    }
    QWidget::enterEvent(event);
}

bool ClipCanvasWidget::Impl::handleLeave(QEvent* event) {
    if (!interactionEnabled()) {
        return false;
    }
    return dispatchInput(
        event, clip_canvas_input::makePointerInput(widget.mapFromGlobal(QCursor::pos()),
                                                   Qt::NoButton, QGuiApplication::mouseButtons(),
                                                   QGuiApplication::keyboardModifiers(),
                                                   CLIP_POINTER_EVENT_LEAVE));
}

void ClipCanvasWidget::leaveEvent(QEvent* event) {
    if (m_impl->handleLeave(event)) {
        return;
    }
    QWidget::leaveEvent(event);
}

bool ClipCanvasWidget::Impl::handleWheel(QWheelEvent* event) {
    if (event == nullptr || !interactionEnabled()) {
        return false;
    }

    const clip_canvas_text_editor_input::FontSizeWheelPlan plan =
        clip_canvas_text_editor_input::planFontSizeWheel(
            clip_canvas_text_editor_input::FontSizeWheelRequest{
                true,
                textInteraction.isActive() ? ClipCanvasTool::Text : canvasTool(),
                event->modifiers(),
                event->pixelDelta().y(),
                event->angleDelta().y(),
            });
    if (plan.matchedToolWheel) {
        if (!plan.shouldStepFontSize) {
            return false;
        }
        return fontWheelTarget() == FontWheelTarget::SerialNumber
                   ? stepSerialNumberFontSize(plan.increase)
                   : stepTextFontSize(plan.increase);
    }

    if (!wheelZoomEnabled()) {
        event->accept();
        return true;
    }
    return dispatchInput(event, clip_canvas_input::makeWheelInput(*event));
}

void ClipCanvasWidget::wheelEvent(QWheelEvent* event) {
    if (m_impl->handleWheel(event)) {
        return;
    }
    QWidget::wheelEvent(event);
}

// Font-size wheel steps follow the style toolbar source so the wheel and the
// toolbar agree: an active text draft, selected text, or selected serial
// badges. With no text or badge selected, that source is the active tool's
// default style. Other selected kinds fall back to the active font tool.
ClipCanvasWidget::Impl::FontWheelTarget ClipCanvasWidget::Impl::fontWheelTarget() const {
    if (textInteraction.isActive()) {
        return FontWheelTarget::Text;
    }
    switch (displayState.snapshot().styleToolbarState.source) {
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_TEXT:
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_TEXT:
        return FontWheelTarget::Text;
    case CLIP_STYLE_TOOLBAR_SOURCE_SELECTED_SERIAL_NUMBER:
    case CLIP_STYLE_TOOLBAR_SOURCE_DEFAULT_SERIAL_NUMBER:
        return FontWheelTarget::SerialNumber;
    default:
        break;
    }
    return canvasTool() == ClipCanvasTool::SerialNumber ? FontWheelTarget::SerialNumber
                                                        : FontWheelTarget::Text;
}

bool ClipCanvasWidget::Impl::stepSerialNumberFontSize(bool increase) {
    auto style = widget.canvasStyleToolbarState().serialNumberStyle;
    const double next = std::clamp(style.fontSize + (increase ? 1.0 : -1.0),
                                   clip_canvas_style_limits::minimumFontSize,
                                   clip_canvas_style_limits::maximumBadgeFontSize);
    if (next == style.fontSize && (widget.canvasStyleToolbarState().serialNumberStyleMixed &
                                   ClipCanvasSerialNumberStyleMixedFontSize) == 0)
        return true;
    style.fontSize = next;
    return widget.commitStyleEdit(
        ClipCanvasSerialNumberEdit{style, ClipCanvasSerialNumberStyleMixedFontSize});
}

bool ClipCanvasWidget::Impl::stepTextFontSize(bool increase) {
    auto style = widget.canvasStyleToolbarState().textStyle;
    const double next = clip_canvas_text_measurement::steppedFontSize(style.fontSize, increase);
    if (next == style.fontSize &&
        (widget.canvasStyleToolbarState().textStyleMixed & ClipCanvasTextStyleMixedFontSize) == 0)
        return true;
    style.fontSize = next;
    return widget.commitStyleEdit(ClipCanvasTextEdit{style, ClipCanvasTextStyleMixedFontSize});
}

bool ClipCanvasWidget::stepFontSize(int direction) {
    return m_impl->stepFontSize(direction);
}

bool ClipCanvasWidget::applyStyleEdit(const ClipCanvasStyleEdit& edit) {
    return m_impl->applyStyleEdit(edit);
}

bool ClipCanvasWidget::Impl::stepFontSize(int direction) {
    if (direction == 0 || !interactionEnabled())
        return false;
    return fontWheelTarget() == FontWheelTarget::SerialNumber
               ? stepSerialNumberFontSize(direction > 0)
               : stepTextFontSize(direction > 0);
}

bool ClipCanvasWidget::Impl::applyStyleEdit(const ClipCanvasStyleEdit& edit) {
    return std::visit(
        [this](const auto& patch) -> bool {
            if (patch.properties == 0)
                return false;
            using T = std::decay_t<decltype(patch)>;
            if constexpr (std::is_same_v<T, ClipCanvasShapeEdit>) {
                return setCanvasShapeStylePatch(patch.style, patch.properties, patch.kind);
            } else if constexpr (std::is_same_v<T, ClipCanvasDistanceEdit>) {
                return widget.setCanvasDistanceStyle(patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasTextEdit>) {
                return setCanvasTextStyle(patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasSerialNumberEdit>) {
                auto style = widget.canvasStyleToolbarState().serialNumberStyle;
                clipCanvasMergeStyle(style, patch.style, patch.properties);
                return applyMutation([&]() {
                    return clip_canvas_commands::setSerialNumberStylePatch(
                        runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                        clip_canvas_types::toEngineSerialNumberStyle(style), patch.properties);
                });
            } else if constexpr (std::is_same_v<T, ClipCanvasFilterEdit>) {
                return setCanvasFilterStyle(patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasBrushEraserEdit>) {
                return setCanvasBrushEraserCreationStyle(patch.style, patch.properties);
            } else if constexpr (std::is_same_v<T, ClipCanvasWatermarkEdit>) {
                auto style = canvasWatermarkConfig();
                clipCanvasMergeStyle(style, patch.style, patch.properties);
                return setCanvasWatermarkConfig(style);
            } else {
                auto style = canvasSpotlightConfig();
                clipCanvasMergeStyle(style, patch.style, patch.properties);
                return setCanvasSpotlightConfig(style);
            }
        },
        edit);
}

bool ClipCanvasWidget::Impl::rememberDraftStyle(const ClipCanvasStyleEdit& edit) {
    const auto* text = std::get_if<ClipCanvasTextEdit>(&edit);
    if (text == nullptr || !textInteraction.isActive())
        return true;
    return applyMutation([&]() {
        return clip_canvas_commands::setTextCreationStyle(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(),
            clip_canvas_types::toEngineTextStyle(text->style), text->properties);
    });
}

bool ClipCanvasWidget::commitStyleEdit(const ClipCanvasStyleEdit& edit) {
    if (!interactionEnabled() || !applyStyleEdit(edit) || !m_impl->rememberDraftStyle(edit))
        return false;
    emit styleEditCommitted(edit);
    return true;
}

bool ClipCanvasWidget::Impl::handleKeyPress(QKeyEvent* event) {
    if (!interactionEnabled()) {
        return false;
    }
    if (textInteraction.isActive()) {
        ClipCanvasWidgetTextInteraction::EditorEventResult result = textInteraction.handleKeyPress(
            event, runtimeBinding.engine(), runtimeBinding.viewportHandle(), hasViewport(),
            displayState.displayCache());
        if (result.handled) {
            applyTextEditorEventResult(result);
            return true;
        }
    }

    if (event != nullptr && !event->isAutoRepeat() && event->modifiers() == Qt::NoModifier &&
        (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        beginArrowText(QPointF(), true)) {
        event->accept();
        return true;
    }
    const clip_canvas_widget_keyboard_flow::KeyPlan plan =
        clip_canvas_widget_keyboard_flow::planPress(event);
    if (!plan.hasEvent) {
        return false;
    }

    switch (plan.effect) {
    case clip_canvas_widget_keyboard_flow::KeyEffect::ToggleDirtyRects:
        setShowDirtyRects(!dirtyRectsVisible);
        accept(*event);
        return true;
    case clip_canvas_widget_keyboard_flow::KeyEffect::AcceptOnly:
        accept(*event);
        return true;
    case clip_canvas_widget_keyboard_flow::KeyEffect::DispatchToEngine:
    default:
        break;
    }

    auto input = clip_canvas_input::makeKeyInput(*event, CLIP_KEY_EVENT_DOWN);
    if (commandKeyResolver) {
        // Engine characters represent commands here; text editing was handled
        // above with the original text. Never dispatch a layout character as Z.
        const QString commandText = event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z
                                        ? QString(QChar(static_cast<ushort>(event->key())))
                                        : QString();
        QKeyEvent command(event->type(), event->key(), event->modifiers(), commandText,
                          event->isAutoRepeat(), static_cast<quint16>(event->count()));
        input = clip_canvas_input::makeKeyInput(command, CLIP_KEY_EVENT_DOWN);
    }
    return dispatchInput(event, input);
}

void ClipCanvasWidget::setCommandKeyResolver(CommandKeyResolver resolver) {
    m_impl->commandKeyResolver = std::move(resolver);
}

void ClipCanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (m_impl->commandKeyResolver) {
        QKeyEvent command(event->type(), m_impl->commandKeyResolver(*event), event->modifiers(),
                          event->nativeScanCode(), event->nativeVirtualKey(),
                          event->nativeModifiers(), event->text(), event->isAutoRepeat(),
                          static_cast<quint16>(event->count()));
        if (m_impl->handleKeyPress(&command)) {
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
        return;
    }
    if (m_impl->handleKeyPress(event)) {
        return;
    }
    QWidget::keyPressEvent(event);
}

bool ClipCanvasWidget::Impl::handleKeyRelease(QKeyEvent* event) {
    if (!interactionEnabled()) {
        return false;
    }
    if (textInteraction.handleKeyRelease(event)) {
        return true;
    }

    const clip_canvas_widget_keyboard_flow::KeyPlan plan =
        clip_canvas_widget_keyboard_flow::planRelease(event);
    if (!plan.hasEvent) {
        return false;
    }

    if (plan.effect == clip_canvas_widget_keyboard_flow::KeyEffect::AcceptOnly) {
        accept(*event);
        return true;
    }
    auto input = clip_canvas_input::makeKeyInput(*event, CLIP_KEY_EVENT_UP);
    if (commandKeyResolver) {
        // Engine characters represent commands here; text editing was handled
        // above with the original text. Never dispatch a layout character as Z.
        const QString commandText = event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z
                                        ? QString(QChar(static_cast<ushort>(event->key())))
                                        : QString();
        QKeyEvent command(event->type(), event->key(), event->modifiers(), commandText,
                          event->isAutoRepeat(), static_cast<quint16>(event->count()));
        input = clip_canvas_input::makeKeyInput(command, CLIP_KEY_EVENT_UP);
    }
    return dispatchInput(event, input);
}

void ClipCanvasWidget::keyReleaseEvent(QKeyEvent* event) {
    if (m_impl->commandKeyResolver) {
        QKeyEvent command(event->type(), m_impl->commandKeyResolver(*event), event->modifiers(),
                          event->nativeScanCode(), event->nativeVirtualKey(),
                          event->nativeModifiers(), event->text(), event->isAutoRepeat(),
                          static_cast<quint16>(event->count()));
        if (m_impl->handleKeyRelease(&command)) {
            event->accept();
            return;
        }
        QWidget::keyReleaseEvent(event);
        return;
    }
    if (m_impl->handleKeyRelease(event)) {
        return;
    }
    QWidget::keyReleaseEvent(event);
}

bool ClipCanvasWidget::Impl::handleInputMethodEvent(QInputMethodEvent* event) {
    if (!interactionEnabled()) {
        return false;
    }
    ClipCanvasWidgetTextInteraction::InputMethodEventResult result =
        textInteraction.handleInputMethodEvent(event, runtimeBinding.engine(),
                                               runtimeBinding.viewportHandle(),
                                               displayState.displayCache(), widget.font());
    syncChangedViewports(result.changedViewports.get());
    if (result.handled) {
        return true;
    }
    return false;
}

void ClipCanvasWidget::inputMethodEvent(QInputMethodEvent* event) {
    if (m_impl->handleInputMethodEvent(event)) {
        return;
    }
    QWidget::inputMethodEvent(event);
}

QVariant ClipCanvasWidget::Impl::inputMethodQuery(Qt::InputMethodQuery query) const {
    if (!interactionEnabled()) {
        return widget.QWidget::inputMethodQuery(query);
    }
    QVariant value =
        textInteraction.inputMethodQuery(query, displayState.displayCache(), widget.font());
    if (value.isValid()) {
        return value;
    }
    return widget.QWidget::inputMethodQuery(query);
}

QVariant ClipCanvasWidget::inputMethodQuery(Qt::InputMethodQuery query) const {
    return m_impl->inputMethodQuery(query);
}

void ClipCanvasWidget::Impl::handleFocusOut() {
    if (pendingLiveStrokePreservesEverySample) {
        flushLiveStrokeMoves();
    } else {
        pendingLiveStrokeMoves.clear();
    }
    pendingEraserMove.reset();
    const bool retainTextEditing = textStylePopupInteractionDepth > 0;
    if (textInteraction.isActive() && !retainTextEditing) {
        commitText(false);
    }
    if (!retainTextEditing) {
        processInput(clip_canvas_event_flow::focusLostInput());
    }
}

void ClipCanvasWidget::focusOutEvent(QFocusEvent* event) {
    m_impl->handleFocusOut();
    QWidget::focusOutEvent(event);
}

void ClipCanvasWidget::Impl::handleResize(const QSize& size) {
    clip_canvas_filter_tile_cache::invalidateNamespace(&widget);
    setSurfaceSizeAndSync(size, true);
}

void ClipCanvasWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    m_impl->handleResize(event->size());
}

void ClipCanvasWidget::Impl::setShowDirtyRects(bool show) {
    if (dirtyRectsVisible == show) {
        return;
    }
    dirtyRectsVisible = show;

    emit widget.showDirtyRectsChanged();

    const QRegion repaintRegion = displayState.dirtyVisualizationRegion();
    clip_canvas_widget_repaint::updateCoalesced(widget, repaintRegion);
}

void ClipCanvasWidget::setShowDirtyRects(bool show) {
    m_impl->setShowDirtyRects(show);
}

namespace {
ClipAutoFilterBounds autoFilterBounds(const QRectF& r) {
    return {r.left(), r.top(), r.right(), r.bottom()};
}
QRectF autoFilterRect(const ClipAutoFilterBounds& r) {
    return QRectF(QPointF(r.left, r.top), QPointF(r.right, r.bottom));
}
} // namespace
quint64 ClipCanvasWidget::Impl::readAutoFilterGeneration() const {
    uint64_t generation = 0;
    uint8_t identified = 0;
    ClipAutoFilterBounds bounds{};
    size_t count = 0;
    clip_runtime_get_auto_filter_regions(runtimeBinding.engine(), &generation, &identified, &bounds,
                                         nullptr, 0, &count);
    return generation;
}
std::optional<ClipCanvasAutoFilterRecord> ClipCanvasWidget::Impl::autoFilterRegions() const {
    uint64_t generation = 0;
    uint8_t identified = 0;
    ClipAutoFilterBounds bounds{};
    size_t count = 0;
    const auto runtime = runtimeBinding.engine();
    if (clip_runtime_get_auto_filter_regions(runtime, &generation, &identified, &bounds, nullptr, 0,
                                             &count) != CLIP_OK ||
        !identified) {
        return std::nullopt;
    }
    std::vector<ClipAutoFilterRegion> regions(count);
    if (clip_runtime_get_auto_filter_regions(runtime, &generation, &identified, &bounds,
                                             regions.data(), regions.size(), &count) != CLIP_OK) {
        return std::nullopt;
    }
    ClipCanvasAutoFilterRecord record;
    record.sourceBounds = autoFilterRect(bounds);
    for (const auto& region : regions) {
        record.regions.append(
            {region.id, autoFilterRect(region.bounds),
             QString::fromUtf8(reinterpret_cast<const char*>(region.category),
                               static_cast<qsizetype>(
                                   strnlen(reinterpret_cast<const char*>(region.category), 128)))});
    }
    return record;
}
bool ClipCanvasWidget::Impl::setAutoFilterRegions(
    const std::optional<ClipCanvasAutoFilterRecord>& record) {
    std::vector<ClipAutoFilterRegion> regions;
    ClipAutoFilterBounds bounds{};
    if (record) {
        bounds = autoFilterBounds(record->sourceBounds);
        for (const auto& region : record->regions) {
            ClipAutoFilterRegion value{};
            value.id = region.id;
            value.bounds = autoFilterBounds(region.bounds);
            const QByteArray category = region.category.toUtf8();
            if (category.size() > 127) {
                return false;
            }
            std::memcpy(value.category, category.constData(), static_cast<size_t>(category.size()));
            regions.push_back(value);
        }
    }
    ClipChangedViewportList changed = nullptr;
    clip_canvas_commands::MutationResult result;
    result.success =
        clip_viewport_set_auto_filter_regions(
            runtimeBinding.engine(), runtimeBinding.viewportHandle(), record ? &bounds : nullptr,
            regions.data(), regions.size(), &changed) == CLIP_OK;
    result.changedViewports.reset(changed);
    return applyMutationResult(std::move(result));
}
bool ClipCanvasWidget::Impl::fillAutoFilterCategory(const QString& category) {
    const QByteArray bytes = category.toUtf8();
    ClipChangedViewportList changed = nullptr;
    clip_canvas_commands::MutationResult result;
    result.success = clip_viewport_fill_auto_filter_category(
                         runtimeBinding.engine(), runtimeBinding.viewportHandle(),
                         reinterpret_cast<const uint8_t*>(bytes.constData()),
                         static_cast<size_t>(bytes.size()), &changed) == CLIP_OK;
    result.changedViewports.reset(changed);
    return applyMutationResult(std::move(result));
}

quint64 ClipCanvasWidget::autoFilterGeneration() const {
    return m_impl->readAutoFilterGeneration();
}
std::optional<ClipCanvasAutoFilterRecord> ClipCanvasWidget::autoFilterRegions() const {
    return m_impl->autoFilterRegions();
}
bool ClipCanvasWidget::setAutoFilterRegions(
    const std::optional<ClipCanvasAutoFilterRecord>& record) {
    return m_impl->setAutoFilterRegions(record);
}
bool ClipCanvasWidget::fillAutoFilterCategory(const QString& category) {
    return m_impl->fillAutoFilterCategory(category);
}

void ClipCanvasWidget::Impl::setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources,
                                                 const QRegion& damage, bool partialDamage) {
    if (auto* owner = runtimeBinding.runtimeOwner()) {
        auto& coordinator = clip_canvas_runtime::Access::smartErase(*owner);
        coordinator.setSources(this, sources, !partialDamage);
        coordinator.sync(runtimeBinding.engine());
        if (!partialDamage)
            smartEraseChanged();
        else {
            clearRenderState();
            widget.update(damage);
        }
    }
}

void ClipCanvasWidget::setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources) {
    m_impl->setBaseImageSources(sources);
}

void ClipCanvasWidget::setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources,
                                           const QRegion& damage) {
    m_impl->setBaseImageSources(sources, damage, true);
}
