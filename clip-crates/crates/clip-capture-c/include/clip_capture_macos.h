#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* macOS 15+. Blocking calls belong on a worker thread while the host runs its
 * main run loop. They fail explicitly if invoked on the main thread.
 * Every output lease must be released. Lease clones survive stream destruction.
 * Serialize calls on each stream; never destroy a handle while it is in use.
 * Permission requests are explicit and belong on the host's main thread. */
typedef struct ClipMacFrame ClipMacFrame;
typedef struct ClipMacStream ClipMacStream;
typedef enum ClipMacStatus {
    CLIP_MAC_OK = 0,
    CLIP_MAC_INVALID_ARGUMENT = 1,
    CLIP_MAC_PERMISSION_DENIED = 2,
    CLIP_MAC_TARGET_UNAVAILABLE = 3,
    CLIP_MAC_UNSUPPORTED = 4,
    CLIP_MAC_TIMEOUT = 5,
    CLIP_MAC_CANCELED = 6,
    CLIP_MAC_INTERRUPTED = 7,
    CLIP_MAC_INTERNAL = 255
} ClipMacStatus;

/* Cancellation may be requested from another thread. Keep this handle alive
 * through the cancel call. Configs copy its state; release after create returns
 * is safe and does not cancel the session. NULL selects no external token. */
typedef struct ClipMacCancellation ClipMacCancellation;
ClipMacCancellation* clip_capture_macos_cancellation_create(void);
void clip_capture_macos_cancellation_cancel(const ClipMacCancellation*);
void clip_capture_macos_cancellation_release(ClipMacCancellation*);
typedef struct ClipMacCaptureConfig {
    uint32_t struct_size; /* sizeof(ClipMacCaptureConfig) */
    const ClipMacCancellation* cancellation;
    uint32_t target_kind; /* 0 primary display, 1 display ID, 2 session-scoped window ID, 3 desktop
                             region */
    uint32_t target_id;
    uint32_t dynamic_range; /* 0 SDR BGRA8, 1 canonical HDR RGBA16Float */
    uint32_t cursor_mode;   /* 0 hidden, 1 embedded, 2 separate (no embedded pixels) */
    uint32_t fps;           /* 1..240 */
    uint32_t timeout_ms;    /* 1..60000 */
    double source_x, source_y, source_width,
        source_height;                    /* desktop points, top-left; region only */
    uint32_t output_width, output_height; /* pixels; both zero selects native resolution */
    const uint32_t* excluded_windows;
    size_t excluded_window_count;
    const int32_t* excluded_processes;
    size_t excluded_process_count;
} ClipMacCaptureConfig;

/* Capability queries enumerate current target geometry, require screen permission,
 * and run on a worker thread. They do not start streams or acquire pixels.
 * Unsupported explicit requests return UNSUPPORTED. A result is not a target
 * reservation; configuration changes may invalidate it immediately. */
typedef struct ClipMacCapabilities {
    uint32_t struct_size;
    uint32_t hdr_supported, native_format;
    uint32_t cpu_format_mask; /* bit (1 << pixel_format), same values as FrameInfo */
    uint32_t exclusion_filters, source_count; /* multiple sources are NOT atomic */
    uint32_t output_width, output_height;
    double x, y, width, height; /* desktop points */
} ClipMacCapabilities;
ClipMacStatus clip_capture_macos_query_capabilities(const ClipMacCaptureConfig*,
                                                    ClipMacCapabilities*);

typedef struct ClipMacFrameInfo {
    uint32_t width, height;
    uint32_t pixel_format; /* 0 RGBA8, 1 BGRA8, 2 RGBA16Float, 3 NV12, 4 P010 */
    uint32_t plane_count;
    int64_t timestamp_value;
    uint32_t timestamp_scale;   /* Mac host clock rational timescale */
    uint32_t color_primaries;   /* 0 BT.709, 1 Display P3, 2 BT.2020 */
    uint32_t transfer_function; /* 0 sRGB, 1 linear, 2 PQ, 3 HLG, 4 BT.709 */
    uint32_t color_range;       /* 0 full, 1 video */
    uint32_t alpha_mode;        /* 0 opaque, 1 straight, 2 premultiplied */
    int64_t timestamp_epoch;
    uint64_t generation;
    uint32_t source_count, duplicate;
    double desktop_x, desktop_y, desktop_width, desktop_height;
} ClipMacFrameInfo;
typedef struct ClipMacCaptureEvent {
    uint32_t kind; /* 1 configuration, 2 frame; configuration precedes its first frame */
    uint32_t width, height;
    uint64_t generation;
    double x, y, desktop_width, desktop_height;
    ClipMacFrame* frame; /* caller owns this lease for kind=2 */
} ClipMacCaptureEvent;
typedef struct ClipMacSourceTime {
    int64_t value;
    uint32_t timescale;
    int64_t epoch;
} ClipMacSourceTime;
typedef struct ClipMacPlane {
    const uint8_t* data;
    size_t len, width, height, stride;
} ClipMacPlane;

typedef struct ClipMacTargetInfo {
    uint32_t kind, id;
    int32_t process_id;
    uint32_t primary, available, pixel_width, pixel_height;
    double x, y, width, height; /* desktop points */
} ClipMacTargetInfo;
/* Synchronous visitor on caller's worker. Copy info if retaining it; IDs for windows
 * belong to the current WindowServer session. kind=1 displays, kind=2 windows. */
ClipMacStatus clip_capture_macos_enumerate(uint32_t kind, uint32_t timeout_ms,
                                           void (*visitor)(const ClipMacTargetInfo*, void*),
                                           void* context);
uint8_t clip_capture_macos_permission_check(void);
uint8_t clip_capture_macos_permission_request(void);
uint8_t clip_capture_macos_hdr_supported(void);
ClipMacStatus clip_capture_macos_snapshot(const ClipMacCaptureConfig*, ClipMacFrame**);
ClipMacStatus clip_capture_macos_stream_create(const ClipMacCaptureConfig*, ClipMacStream**);
ClipMacStatus clip_capture_macos_stream_next(ClipMacStream*, uint32_t timeout_ms,
                                             ClipMacCaptureEvent*);
void clip_capture_macos_stream_destroy(ClipMacStream*);
ClipMacFrame* clip_capture_macos_frame_retain(const ClipMacFrame*);
void clip_capture_macos_frame_release(ClipMacFrame*);
ClipMacStatus clip_capture_macos_frame_info(const ClipMacFrame*, ClipMacFrameInfo*);
/* Source timestamps are independent observations, not an atomic desktop acquisition.
 * FrameInfo's timestamp is the first source only. Use source_count to enumerate. */
ClipMacStatus clip_capture_macos_frame_source_time(const ClipMacFrame*, uint32_t index,
                                                   ClipMacSourceTime*);
/* Explicit CPU readback, cached for the lease lifetime. Plane pointers are read-only. */
/* Explicit CPU conversion: 0 RGBA8, 1 BGRA8, 2 RGBA16Float. HDR-to-SDR
 * conversion is not implicit. Pointer lifetime matches map_plane. */
ClipMacStatus clip_capture_macos_frame_map_format(const ClipMacFrame*, uint32_t format,
                                                  uint32_t plane, ClipMacPlane*);
ClipMacStatus clip_capture_macos_frame_map_plane(const ClipMacFrame*, uint32_t index,
                                                 ClipMacPlane*);
/* Borrowed, immutable CVPixelBufferRef. Keep the lease alive through GPU completion. */
const void* clip_capture_macos_frame_pixel_buffer(const ClipMacFrame*);
#ifdef __cplusplus
}
#endif
