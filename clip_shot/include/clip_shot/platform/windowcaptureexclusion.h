#ifndef CLIP_SHOT_PLATFORM_WINDOWCAPTUREEXCLUSION_H
#define CLIP_SHOT_PLATFORM_WINDOWCAPTUREEXCLUSION_H

#include <QWidget>

#include <cstdint>
#include <optional>

namespace clip_shot::platform {

[[nodiscard]] bool setWindowExcludedFromCapture(QWidget* window, bool excluded);
[[nodiscard]] std::optional<std::uint32_t> captureWindowId(QWidget* window);

} // namespace clip_shot::platform

#endif // CLIP_SHOT_PLATFORM_WINDOWCAPTUREEXCLUSION_H
