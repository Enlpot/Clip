#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLCOMMANDWORKFLOWPORTS_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLCOMMANDWORKFLOWPORTS_H

#include <QPoint>
#include <QtGlobal>

#include <functional>
#include <optional>

enum class ClipCanvasTool;
struct ClipCanvasShapeStyle;
struct ClipCanvasTextStyle;
struct ClipCanvasSerialNumberStyle;
struct ClipCanvasFilterStyle;
struct ClipCanvasWatermarkConfig;
struct ClipCanvasSpotlightConfig;
struct ClipCanvasStyleToolbarState;
enum class ClipCanvasShapeKind;

struct ScreenshotToolCommandActions {
    std::function<bool()> selectorReady;
    std::function<void()> startSelectorRefresh;
    std::function<void(const QPoint& physicalPoint)> updateSelectorSelectionAt;
    std::function<void()> clearSelectorSelection;

    std::function<void(bool enabled)> setCanvasInteractionEnabled;
    std::function<void(ClipCanvasTool tool)> setCanvasTool;
    std::function<bool(ClipCanvasShapeStyle* outStyle)> tryCurrentRectangleStyle;
    std::function<void(const ClipCanvasShapeStyle& style, quint32 properties,
                       ClipCanvasShapeKind kind)>
        setShapeStylePatch;
    std::function<void(const ClipCanvasFilterStyle& style, quint32 properties)> setFilterStyle;
    std::function<void(const ClipCanvasWatermarkConfig& config)> setWatermarkConfig;
    std::function<void(const ClipCanvasSpotlightConfig& config)> setSpotlightConfig;
    std::function<void(const ClipCanvasTextStyle& style, quint32 properties)> setTextStyle;
    std::function<void(const ClipCanvasSerialNumberStyle& style, std::optional<quint32> properties)>
        setSerialNumberStyle;
    std::function<void(qint64 delta)> adjustSelectedSerialNumbers;
    std::function<void()> createCanvasTextForSelectedSerialNumber;
    std::function<bool(int direction)> stepToolbarStrokeWidth;

    std::function<void()> updateOverlayState;
    std::function<void()> updateOverlayCursors;
    std::function<void()> raiseToolbarForCanvasInteraction;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTTOOLCOMMANDWORKFLOWPORTS_H
