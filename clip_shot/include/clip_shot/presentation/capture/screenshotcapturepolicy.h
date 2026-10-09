#ifndef CLIP_SHOT_PRESENTATION_CAPTURE_SCREENSHOTCAPTUREPOLICY_H
#define CLIP_SHOT_PRESENTATION_CAPTURE_SCREENSHOTCAPTUREPOLICY_H

#include <cstdint>

namespace clip_shot::presentation::capture {

enum class ScreenshotApiMode : std::uint8_t { Auto, Dxgi, Wgc, Gdi };

[[nodiscard]] ScreenshotApiMode screenshotApiModeFromValue(const char* value) noexcept;
[[nodiscard]] std::uint8_t nativeBackendForNormalScreenshot(ScreenshotApiMode mode) noexcept;
[[nodiscard]] ScreenshotApiMode resolveAutoScreenshotApiMode() noexcept;

} // namespace clip_shot::presentation::capture

#endif
