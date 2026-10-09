#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYCANVASPRESENTER_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYCANVASPRESENTER_H

#include "clip_draw_engine_qt/clip_canvas_types.h"
#include "clip_shot/presentation/screenshotselectioneffectgeometry.h"
#include "clip_shot/presentation/screenshotselectiongeometry.h"
#include "clip_shot/presentation/screenshottypes.h"

#include <QColor>
#include <QPointer>
#include <QPoint>
#include <QRectF>

#include <functional>
#include <optional>

class ScreenshotDisplaySession;
class ScreenshotOverlayWindow;
struct ScreenshotSelectionVisualState;

class ScreenshotOverlayCanvasPresenter final {
  public:
    using OverlayFactory = std::function<ScreenshotOverlayWindow*(ScreenshotOverlayWindow*)>;

    explicit ScreenshotOverlayCanvasPresenter(OverlayFactory ensureOverlay);

    void clearOverlayCanvas(ScreenshotOverlayWindow* overlay) const;
    void prepareDisplayModels(ScreenshotDisplaySession& displaySession) const;
    void applyDisplayModels(ScreenshotDisplaySession& displaySession) const;
    void showOverlayWindows(const ScreenshotDisplaySession& displaySession,
                            ScreenshotOverlayShowMode mode) const;
    void updateOverlayState(const ScreenshotDisplaySession& displaySession,
                            const ScreenshotSelectionVisualState& selectionState,
                            bool intelligentSelecting, bool manualSelecting, bool dragging) const;
    void updateOverlayCursors(const ScreenshotDisplaySession& displaySession, bool selecting,
                              bool dragging) const;
    void updateGuideLines(const ScreenshotDisplaySession& displaySession,
                          ScreenshotOverlayWindow* owner, const QPointF& localPosition,
                          bool selecting, const QColor& cursorColor,
                          const QColor& monitorCenterColor,
                          const QColor& selectionCenterColor = Qt::transparent) const;
    void
    updateGuideLinesAtGlobalPosition(const ScreenshotDisplaySession& displaySession,
                                     const QPoint& globalPosition, bool selecting,
                                     const QColor& cursorColor, const QColor& monitorCenterColor,
                                     const QColor& selectionCenterColor = Qt::transparent) const;
    void clearGuideLines(const ScreenshotDisplaySession& displaySession) const;
    void setOverlayCursor(ScreenshotOverlayWindow* overlay,
                          ScreenshotSelectionDragMode dragMode) const;
    static void setOverlayEffectCursor(ScreenshotOverlayWindow* overlay,
                                       ScreenshotSelectionEffectHandle handle);
    void setCanvasInteractionEnabled(const ScreenshotDisplaySession& displaySession,
                                     bool enabled) const;
    void setCanvasTool(const ScreenshotDisplaySession& displaySession, ClipCanvasTool tool) const;
    void refreshCanvasCreationStyles(const ScreenshotDisplaySession& displaySession,
                                     const ClipCanvasStyleDefaults& defaults) const;
    [[nodiscard]] bool resetEditingState(const ScreenshotDisplaySession& displaySession) const;
    [[nodiscard]] bool tryCurrentRectangleStyle(const ScreenshotDisplaySession& displaySession,
                                                ClipCanvasShapeStyle* outStyle) const;
    [[nodiscard]] ClipCanvasShapeStyle
    currentRectangleStyle(const ScreenshotDisplaySession& displaySession) const;
    void setShapeStylePatch(const ScreenshotDisplaySession& displaySession,
                            const ClipCanvasShapeStyle& style, quint32 properties,
                            ClipCanvasShapeKind kind) const;
    void setFilterStyle(const ScreenshotDisplaySession& displaySession,
                        const ClipCanvasFilterStyle& style, quint32 properties) const;
    void setWatermarkConfig(const ScreenshotDisplaySession& displaySession,
                            const ClipCanvasWatermarkConfig& config) const;
    void previewWatermarkConfig(const ScreenshotDisplaySession& displaySession,
                                const ClipCanvasWatermarkConfig& config) const;
    void setSpotlightConfig(ScreenshotDisplaySession& displaySession,
                            const ClipCanvasSpotlightConfig& config) const;
    void previewSpotlightConfig(ScreenshotDisplaySession& displaySession,
                                const ClipCanvasSpotlightConfig& config) const;
    void setTextStyle(const ScreenshotDisplaySession& displaySession,
                      const ClipCanvasTextStyle& style, quint32 properties) const;
    void setSerialNumberStyle(const ScreenshotDisplaySession& displaySession,
                              const ClipCanvasSerialNumberStyle& style,
                              std::optional<quint32> properties = std::nullopt) const;
    void adjustSelectedSerialNumbers(const ScreenshotDisplaySession& displaySession,
                                     qint64 delta) const;
    void createTextForSelectedSerialNumber(const ScreenshotDisplaySession& displaySession) const;
    void reorderSelectedElements(const ScreenshotDisplaySession& displaySession,
                                 ClipCanvasSelectionOrder order) const;
    void alignSelectedElements(const ScreenshotDisplaySession& displaySession,
                               ClipCanvasSelectionAlignment alignment) const;
    void setSelectedElementsOpacity(const ScreenshotDisplaySession& displaySession,
                                    qreal opacity) const;
    void duplicateSelectedElements(const ScreenshotDisplaySession& displaySession) const;
    void deleteSelectedElements(const ScreenshotDisplaySession& displaySession) const;
    void deleteAllElements(const ScreenshotDisplaySession& displaySession) const;

  private:
    OverlayFactory m_ensureOverlay;
    mutable QPointer<ScreenshotOverlayWindow> m_guideLineOwner;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYCANVASPRESENTER_H
