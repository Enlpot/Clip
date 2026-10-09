#ifndef CLIP_SHOT_PRESENTATION_DIRECTCAPTURENATIVE_H
#define CLIP_SHOT_PRESENTATION_DIRECTCAPTURENATIVE_H
#include "clip_shot/presentation/directcaptureworkflow.h"
namespace clip_shot::presentation {
[[nodiscard]] DirectCaptureFrame captureDirectTarget(const DirectCaptureRequest& request);
}
#endif
