#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLBARWINDOW_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLBARWINDOW_H

#include "clip_shot/presentation/screenshotselectiondisplayunit.h"
#include "clip_shot/presentation/screenshotfloatingtoolpalettewindow.h"
#include "clip_shot/presentation/screenshottoolpalette.h"
#include "clip_shot/storage/settingsadapters.h"

#include <QRect>

class QEnterEvent;
class QScreen;
class ScreenshotToolbarCommandSink;

class ScreenshotToolbarWindow final : public ScreenshotFloatingToolPaletteWindow {
    Q_OBJECT

  public:
    explicit ScreenshotToolbarWindow(ScreenshotToolbarCommandSink& commands,
                                     QWidget* parent = nullptr);
    void resetForNewCapture();
    void setToolbarSize(const QString& size);
    void setToolbarLayout(const clip_shot::storage::ScreenshotToolbarLayout& layout);
    void setActionToolsLayout(const clip_shot::storage::ScreenshotToolbarLayout& layout);
    void setScrollingScreenshotMode(bool enabled);
    void setActiveTool(ScreenshotToolPalette::Tool tool);
    void setScreenshotRegionType(ScreenshotRegionType type);
    void setSelectionDisplayUnit(ScreenshotSelectionDisplayUnit unit);
    void setRecaptureBusy(bool busy);
    void synchronizeCursorState();
    [[nodiscard]] bool activateDrawingShortcut(const QString& toolId);
    void restoreRememberedDrawingTool();
    void suppressRememberedDrawingTool();
    void setHistoryState(const ClipCanvasHistoryState& state);
    void setStyleToolbarState(const ClipCanvasStyleToolbarState& state);
    void setWatermarkConfig(const ClipCanvasWatermarkConfig& config);
    void setSpotlightConfig(const ClipCanvasSpotlightConfig& config);
    void setRecognitionEnabled(bool enabled);
    void setOcrEnabled(bool enabled);
    void setOcrBusy(bool busy);
    void setTableEnabled(bool enabled);
    void setTableBusy(bool busy);
    void setQrEnabled(bool enabled);
    void setQrBusy(bool busy);
    void setImageConversionBusy(bool markdownBusy, bool htmlBusy);
    void setTableEditingState(bool available, bool canUndo, bool canRedo, bool canMerge,
                              bool canSplit, bool canReset);
    void setShowOriginalImage(bool show);
    void setTextEditingState(bool available, bool editing, bool canUndo = false,
                             bool canRedo = false);
    void setTextTranslationState(bool available, bool translating, bool streaming,
                                 bool canUndo = false, bool canRedo = false, bool canReset = false,
                                 bool originalImage = false);
    void setTextTransformSelections(const QString& formatting, const QString& punctuation);
    void setTextTargetLanguage(const QString& language);
    void setPlacementContext(QScreen* screen, const QRect& logicalBounds,
                             const QRect& physicalBounds = QRect());
    void setMovementBounds(const QRect& logicalBounds, const QRect& physicalBounds = QRect());
    void resetPositionForSelection(const QPoint& position);

  protected:
    void enterEvent(QEnterEvent* event) override;

  private:
    void initializePalette();
    void connectToolCommands(ScreenshotToolPalette& toolPalette);
    void connectActionCommands(ScreenshotToolPalette& toolPalette);
    void connectStyleCommands(ScreenshotToolPalette& toolPalette);
    void connectSerialNumberCommands(ScreenshotToolPalette& toolPalette);
    void connectScrollingScreenshotCommands(ScreenshotToolPalette& toolPalette);

    void synchronizeJumpToTranslationPageSetting();
    void setActiveToolAndReposition(ScreenshotToolPalette::Tool tool);

    ScreenshotToolbarCommandSink& m_commands;
    QRect m_movementLogicalBounds;
    QRect m_movementPhysicalBounds;
    QScreen* m_placementScreen = nullptr;
    bool m_manuallyDragged = false;
    bool m_rememberedDrawingToolRestorePending = true;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLBARWINDOW_H
