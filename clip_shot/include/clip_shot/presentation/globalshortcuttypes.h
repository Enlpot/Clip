#ifndef CLIP_SHOT_PRESENTATION_GLOBALSHORTCUTTYPES_H
#define CLIP_SHOT_PRESENTATION_GLOBALSHORTCUTTYPES_H

#include "clip_shot/shortcuts/shortcutbinding.h"

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

namespace clip_shot::presentation {
enum class GlobalShortcutAction {
    Screenshot,
    ScreenshotDelay,
    ScreenshotFixed,
    ScreenshotOcr,
    ScreenshotTranslation,
    ScreenshotCopy,
    ScreenshotFullScreen,
    ScreenshotFocusedWindow,
    ScreenRecord,
    ScreenRecordCopy,
    OpenScreenRecordingFolder,
    OpenCaptureHistory,
    OpenSettings,
    PinClipboardContent,
    TranslateSelectedText,
    PinSelectedFiles,
    RestoreLastClosedWindows,
    ToggleGlobalHotkeys,
    ToggleDisableOnFocusedFullscreenWindow,
    OpenPinToScreenManagement,
    GlobalCanvas,
    SwitchWindowGroup,
    ScreenshotSave,
    ScreenshotQuickSave,
};

[[nodiscard]] constexpr bool controlsGlobalHotkeyGates(GlobalShortcutAction action) {
    return action == GlobalShortcutAction::ToggleGlobalHotkeys ||
           action == GlobalShortcutAction::ToggleDisableOnFocusedFullscreenWindow;
}

enum class GlobalShortcutStatus {
    Unset,
    Registered,
    PartiallyRegistered,
    Failed,
};

enum class GlobalShortcutFailureReason {
    None,
    InvalidShortcut,
    AlreadyInUse,
    UnsupportedPlatform,
    SystemError,
};

struct GlobalShortcutBindingResult {
    QString shortcut;
    bool registered = false;
    GlobalShortcutFailureReason failureReason = GlobalShortcutFailureReason::None;
    qint64 nativeErrorCode = 0;
    clip_shot::shortcuts::ShortcutBinding binding;
};

struct GlobalShortcutRegistrationState {
    GlobalShortcutAction action = GlobalShortcutAction::Screenshot;
    clip_shot::shortcuts::ShortcutBindingList shortcuts;
    GlobalShortcutStatus status = GlobalShortcutStatus::Unset;
    QVector<GlobalShortcutBindingResult> bindings;
};

// A missing value means the native input state cannot be observed safely.
struct GlobalShortcutInputState {
    bool anyShortcutKeyDown = false;
    bool escapeDown = false;
    bool escapeIsShortcutKey = false;
};

struct GlobalShortcutBackendResult {
    bool registered = false;
    GlobalShortcutFailureReason failureReason = GlobalShortcutFailureReason::None;
    qint64 nativeErrorCode = 0;
};

struct GlobalShortcutValidationResult {
    QString shortcut;
    bool supported = false;
    GlobalShortcutFailureReason failureReason = GlobalShortcutFailureReason::InvalidShortcut;
    clip_shot::shortcuts::ShortcutBinding binding;
};
} // namespace clip_shot::presentation

Q_DECLARE_METATYPE(clip_shot::presentation::GlobalShortcutAction)
Q_DECLARE_METATYPE(clip_shot::presentation::GlobalShortcutRegistrationState)

#endif // CLIP_SHOT_PRESENTATION_GLOBALSHORTCUTTYPES_H
