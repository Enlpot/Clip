#ifndef CLIP_SHOT_PLATFORM_MACOS_WINDOWCURSORCOORDINATOR_H
#define CLIP_SHOT_PLATFORM_MACOS_WINDOWCURSORCOORDINATOR_H

class QWidget;

namespace clip_shot::platform::macos {
// Register a window family for its QWidget lifetime without creating a native surface.
// Current transient descendants are included; repeated registration is harmless.
void configureWindowCursorUpdates(QWidget* window);
} // namespace clip_shot::platform::macos

#endif
