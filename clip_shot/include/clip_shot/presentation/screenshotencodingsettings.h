#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTENCODINGSETTINGS_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTENCODINGSETTINGS_H

#include "clip_shot/presentation/screenshotimagefileservice.h"
#include "clip_shot/storage/settingsadapters.h"

namespace clip_shot::presentation {
// Snapshot at the output request boundary; asynchronous exports own this value.
[[nodiscard]] inline ScreenshotImageEncodingOptions
screenshotEncodingOptions(const storage::ScreenshotSettings& settings) {
    return {settings.imageQuality(),
            ScreenshotImageFileService::compressionLevelForKey(settings.compressionLevel())};
}
} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTENCODINGSETTINGS_H
