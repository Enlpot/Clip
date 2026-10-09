#include "clip_capture_macos.h"
#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition))                                                                          \
            return __LINE__;                                                                       \
    } while (0)
#include <stdio.h>

int main(void) {
    REQUIRE(clip_capture_macos_snapshot(NULL, NULL) == CLIP_MAC_INVALID_ARGUMENT);
    REQUIRE(clip_capture_macos_frame_retain(NULL) == NULL);
    clip_capture_macos_frame_release(NULL);
    clip_capture_macos_stream_destroy(NULL);
    ClipMacCancellation* cancellation = clip_capture_macos_cancellation_create();
    REQUIRE(cancellation != NULL);
    ClipMacCaptureConfig config = {0};
    config.struct_size = sizeof(config);
    config.cancellation = cancellation;
    config.fps = 30;
    config.timeout_ms = 100;
    clip_capture_macos_cancellation_cancel(cancellation);
    ClipMacFrame* frame = NULL;
    REQUIRE(clip_capture_macos_snapshot(&config, &frame) == CLIP_MAC_CANCELED);
    REQUIRE(frame == NULL);
    clip_capture_macos_cancellation_release(cancellation);
    printf("capture-only C ABI is usable; HDR supported=%u\n", clip_capture_macos_hdr_supported());
    return 0;
}
