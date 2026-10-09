#ifndef CLIP_SHOT_PRESENTATION_DIRECTCAPTUREHISTORY_H
#define CLIP_SHOT_PRESENTATION_DIRECTCAPTUREHISTORY_H

#include "clip_shot/presentation/directcaptureworkflow.h"
#include "clip_shot/storage/capturehistorytypes.h"

namespace clip_shot::presentation {
[[nodiscard]] storage::CaptureHistoryDraft
directCaptureHistoryDraft(const DirectCaptureRequest& request, const DirectCaptureFrame& frame,
                          std::optional<storage::PreparedPngImage> png = {});
}
#endif
