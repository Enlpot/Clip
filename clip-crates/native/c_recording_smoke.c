#include "clip_recording_macos.h"
#include "clip_recording.h"
#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition))                                                                          \
            return __LINE__;                                                                       \
    } while (0)
#include <stddef.h>
int main(void) {
    REQUIRE(clip_recording_macos_create(NULL, NULL) == CLIP_MAC_RECORDING_INVALID_ARGUMENT);
    REQUIRE(clip_recording_macos_pause(NULL, 0) == CLIP_MAC_RECORDING_INVALID_ARGUMENT);
    REQUIRE(clip_recording_macos_enumerate_inputs(NULL, NULL) ==
            CLIP_MAC_RECORDING_INVALID_ARGUMENT);
    REQUIRE(clip_recording_macos_microphone_permission_request(NULL, NULL) ==
            CLIP_MAC_RECORDING_INVALID_ARGUMENT);
    ClipMacRecordingCancellation* cancellation = clip_recording_macos_cancellation_create();
    REQUIRE(cancellation != NULL);
    ClipMacRecordingConfig config = {0};
    config.struct_size = sizeof(config);
    config.cancellation = cancellation;
    config.fps = 30;
    config.output_width = 64;
    config.output_height = 64;
    config.output_path = "/nonexistent/canceled-recording.mp4";
    clip_recording_macos_cancellation_cancel(cancellation);
    ClipMacRecording* recording = NULL;
    REQUIRE(clip_recording_macos_create(&config, &recording) == CLIP_MAC_RECORDING_CANCELED);
    REQUIRE(recording == NULL);
    clip_recording_macos_cancellation_release(cancellation);
    clip_recording_macos_destroy(NULL);
    REQUIRE(clip_recording_last_error_message() != NULL);
    return 0;
}
