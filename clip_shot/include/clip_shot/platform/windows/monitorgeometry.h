#ifndef CLIP_SHOT_PLATFORM_WINDOWS_MONITORGEOMETRY_H
#define CLIP_SHOT_PLATFORM_WINDOWS_MONITORGEOMETRY_H

#include <QRect>

class QScreen;

namespace clip_shot::platform::windows {

// Physical monitor rectangle for screen in the native desktop coordinates the capture backend
// reports. Empty when the monitor cannot be queried; callers must not fall back to Qt logical
// geometry, because the two spaces disagree under mixed-DPI scaling. Returns an empty rectangle
// on platforms without a native monitor space (macOS uses its point-based capture canvas).
[[nodiscard]] QRect nativeMonitorRect(const QScreen& screen);

} // namespace clip_shot::platform::windows

#endif // CLIP_SHOT_PLATFORM_WINDOWS_MONITORGEOMETRY_H
