#ifndef CLIP_SHOT_PLATFORM_SCREENSHOTNATIVE_H
#define CLIP_SHOT_PLATFORM_SCREENSHOTNATIVE_H
#include "clip_shot/platform/windows/scrollinput.h"
class QWidget;
namespace clip_shot::platform {
using ScrollInputResult = windows::ScrollInputResult;
#ifdef Q_OS_MACOS
// Give Qt exclusive drag ownership, including after native surface recreation.
void configureControlledWindowDragging(QWidget* widget, bool controlResizing = false);
void configureScreenshotOverlayWindow(QWidget* widget);
void configureScreenshotColorPickerWindow(QWidget* widget);
void configureGlobalCanvasWindow(QWidget* widget);
void configureScreenRecordingAreaWindow(QWidget* widget);
void configureScreenRecordingToolbarWindow(QWidget* widget);
void configureScreenshotRecognitionWindow(QWidget* widget);
// Order an already-visible passive tool without activating the application or changing levels.
[[nodiscard]] bool stackScreenshotWindowBelow(QWidget* widget, QWidget* sibling);
// Cocoa masks clip drawing, but do not route input to windows underneath.
void setScreenshotInputTransparent(QWidget* widget, bool transparent);
void configureScreenshotToolbarWindow(QWidget* widget);
// Desktop tools receive content drags; an explicitly visible capture tool must
// temporarily stay above its capture overlay instead.
void configureFloatingToolbarWindow(QWidget* widget, bool captureActive = false);
quint32 screenshotDisplayAtCursor();
quint32 screenshotFocusedWindow();
QRectF screenshotFocusedWindowBounds();
bool screenshotScrollPermission();
ScrollInputResult sendScreenshotScroll(const QRect& desktopSelection, const QPoint& delta);
#else
inline bool screenshotScrollPermission() {
    return true;
}
inline ScrollInputResult sendScreenshotScroll(const QRect& desktopSelection, const QPoint& delta) {
    return windows::sendScrollingWheelStep(desktopSelection, delta);
}
#endif
} // namespace clip_shot::platform
#endif
