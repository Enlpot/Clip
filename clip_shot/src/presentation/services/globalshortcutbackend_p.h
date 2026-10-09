#ifndef CLIP_SHOT_PRESENTATION_GLOBALSHORTCUTBACKEND_P_H
#define CLIP_SHOT_PRESENTATION_GLOBALSHORTCUTBACKEND_P_H

#include "clip_shot/presentation/globalshortcutmanager.h"

#include <memory>

namespace clip_shot::presentation {

[[nodiscard]] std::unique_ptr<GlobalShortcutBackend> createPlatformGlobalShortcutBackend();

#ifdef Q_OS_WIN
[[nodiscard]] std::unique_ptr<GlobalShortcutBackend> createWindowsGlobalShortcutBackend();
#endif
#ifdef Q_OS_MACOS
[[nodiscard]] std::unique_ptr<GlobalShortcutBackend> createMacOSGlobalShortcutBackend();
#endif

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_GLOBALSHORTCUTBACKEND_P_H
