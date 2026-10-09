#ifndef CLIP_SHOT_PRESENTATION_SETTINGS_APPLICATIONPRIORITY_H
#define CLIP_SHOT_PRESENTATION_SETTINGS_APPLICATIONPRIORITY_H

#include <QString>

#include <optional>

namespace clip_shot::presentation::settings {

enum class ApplicationPriority {
    Normal,
    AboveNormal,
    High,
    Realtime,
};

[[nodiscard]] QString applicationPriorityValue(ApplicationPriority priority);
[[nodiscard]] std::optional<ApplicationPriority> applicationPriorityForValue(const QString& value);

// Applies the saved preference to the current process. This is intentionally a no-op failure
// when the operating system rejects a requested class (for example, Realtime without privilege).
[[nodiscard]] bool applyConfiguredApplicationPriority();
[[nodiscard]] bool applyApplicationPriority(ApplicationPriority priority);

} // namespace clip_shot::presentation::settings

#endif // CLIP_SHOT_PRESENTATION_SETTINGS_APPLICATIONPRIORITY_H
