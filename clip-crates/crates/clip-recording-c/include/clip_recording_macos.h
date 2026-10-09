#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* macOS 15+. Drive create/step/pause/finish/destroy on one worker while the
 * main run loop runs. Destroy before finish cancels the staging recording.
 * Errors use clip_recording_last_error_message() from clip_recording.h. */
typedef struct ClipMacRecording ClipMacRecording;
typedef enum ClipMacRecordingStatus {
    CLIP_MAC_RECORDING_OK = 0,
    CLIP_MAC_RECORDING_INVALID_ARGUMENT = 1,
    CLIP_MAC_RECORDING_PERMISSION_DENIED = 2,
    CLIP_MAC_RECORDING_TARGET_UNAVAILABLE = 3,
    CLIP_MAC_RECORDING_UNSUPPORTED = 4,
    CLIP_MAC_RECORDING_TIMEOUT = 5,
    CLIP_MAC_RECORDING_CANCELED = 6,
    CLIP_MAC_RECORDING_FAILED = 255
} ClipMacRecordingStatus;
/* Cancel from any thread; keep the handle alive during cancel. Sessions copy
 * its state. Cancellation aborts recording; finish returns CANCELED. */
typedef struct ClipMacRecordingCancellation ClipMacRecordingCancellation;
ClipMacRecordingCancellation* clip_recording_macos_cancellation_create(void);
void clip_recording_macos_cancellation_cancel(const ClipMacRecordingCancellation*);
void clip_recording_macos_cancellation_release(ClipMacRecordingCancellation*);
typedef struct ClipMacRecordingConfig {
    uint32_t struct_size;
    const ClipMacRecordingCancellation* cancellation;
    uint32_t target_kind; /* 0 primary, 1 display ID, 2 window ID, 3 desktop region */
    uint32_t target_id;
    uint32_t hdr;      /* 0 SDR, 1 HDR (requires HEVC and Apple Silicon) */
    uint32_t editable; /* 0 direct MP4 + sidecar, 1 version-2 editable bundle at output_path */
    double x, y, width, height;           /* desktop points for target_kind = 3 */
    uint32_t output_width, output_height; /* fixed pixel dimensions, even and nonzero */
    uint32_t fps;                         /* 1..240 */
    uint32_t codec;                       /* 0 H.264, 1 HEVC */
    uint32_t policy; /* 0 hardware preferred, 1 hardware only, 2 software only */
    uint32_t cursor; /* 0 hidden, 1 embedded, 2 separate (fails when public shape unavailable) */
    uint32_t system_audio, microphone;       /* 0 disabled, 1 required */
    uint32_t click_effects, trail, keyboard; /* 0 disabled, 1 enabled; require Input Monitoring */
    const char* microphone_id; /* NULL selects default device; otherwise CoreAudio device UID */
    const char* output_path;   /* UTF-8 */
    /* Optional appended fields, selected by struct_size; legacy callers default off. */
    uint32_t highlight_rgba;      /* 0xRRGGBBAA multiply tint, visible cursor only */
    uint32_t record_mouse_clicks; /* independent of keyboard */
} ClipMacRecordingConfig;
typedef struct ClipMacRecordingEvent {
    uint32_t kind; /* 0 idle, 1 configuration, 2 encoded frame, 3 source interrupted */
    uint32_t width, height;
    uint64_t generation;
    uint64_t pts; /* output-frame time base for kind 2; active milliseconds for kind 3 */
    double x, y, desktop_width, desktop_height;
} ClipMacRecordingEvent;
typedef struct ClipMacRecordingReport {
    uint64_t encoded_frames, cpu_readbacks, interruptions, configuration_changes;
} ClipMacRecordingReport;
uint8_t clip_recording_macos_input_permission_check(void);
uint8_t clip_recording_macos_input_permission_request(void);
uint8_t clip_recording_macos_microphone_permission_check(void);
/* The completion runs once on an arbitrary OS queue. Keep callback/context
 * alive until completion. The host main run loop must remain active. */
ClipMacRecordingStatus
clip_recording_macos_microphone_permission_request(void (*completion)(uint8_t granted, void*),
                                                   void* context);
typedef struct ClipMacInputDevice {
    uint32_t id, is_default;
    const char* uid;
    const char* name;
} ClipMacInputDevice;
/* Synchronous visitor; copy strings before returning if retaining them. */
ClipMacRecordingStatus
clip_recording_macos_enumerate_inputs(void (*visitor)(const ClipMacInputDevice*, void*),
                                      void* context);
ClipMacRecordingStatus clip_recording_macos_create(const ClipMacRecordingConfig*,
                                                   ClipMacRecording**);
ClipMacRecordingStatus clip_recording_macos_step(ClipMacRecording*, uint32_t timeout_ms,
                                                 ClipMacRecordingEvent*);
ClipMacRecordingStatus clip_recording_macos_pause(ClipMacRecording*, uint8_t paused);
ClipMacRecordingStatus clip_recording_macos_finish(ClipMacRecording*, ClipMacRecordingReport*);
void clip_recording_macos_destroy(ClipMacRecording*);
#ifdef __cplusplus
}
#endif
