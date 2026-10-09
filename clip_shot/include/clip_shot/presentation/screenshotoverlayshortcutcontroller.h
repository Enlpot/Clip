#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYSHORTCUTCONTROLLER_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYSHORTCUTCONTROLLER_H

#include "clip_shot/presentation/screenshotoverlayinputhandler.h"

#include <QObject>

#include <memory>

class ScreenshotIntelligentSelectionModel;
class ScreenshotInteractionState;
class ScreenshotOverlayInputHandler;

namespace clip_shot::presentation {
class WindowShortcutManager;
enum class GlobalShortcutAction;
} // namespace clip_shot::presentation

class ScreenshotOverlayShortcutController final : public QObject {
  public:
    ScreenshotOverlayShortcutController(
        clip_shot::presentation::WindowShortcutManager& shortcutManager,
        ScreenshotOverlayInputHandler& inputHandler, ScreenshotInteractionState& interaction,
        ScreenshotIntelligentSelectionModel& intelligentSelection,
        ScreenshotOverlayInputActions actions, QObject* parent = nullptr);
    ~ScreenshotOverlayShortcutController() override;

    void reloadConfiguredShortcuts();
    // Returns true for mapped actions even when capture readiness or local eligibility rejects
    // them.
    [[nodiscard]] bool
    handleGlobalScreenshotShortcut(clip_shot::presentation::GlobalShortcutAction action,
                                   bool captureReady);

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTOVERLAYSHORTCUTCONTROLLER_H
