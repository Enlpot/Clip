#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClipCaptureDesktopSessionImpl ClipCaptureDesktopSession;
typedef struct ClipCaptureDesktopLayoutImpl ClipCaptureDesktopLayout;
typedef struct ClipCaptureRegionSessionImpl ClipCaptureRegionSession;
typedef struct ClipCaptureWindowSessionImpl ClipCaptureWindowSession;
typedef struct ClipCaptureMonitorSessionImpl ClipCaptureMonitorSession;
typedef struct ClipCaptureFrameLeaseImpl ClipCaptureFrameLease;
typedef struct ClipCaptureCancellationTokenImpl ClipCaptureCancellationToken;
typedef struct ClipCaptureScreenshotResultImpl ClipCaptureScreenshotResult;
typedef struct ClipCaptureCursorSnapshotImpl ClipCaptureCursorSnapshot;

/* Copies the current global cursor, including its pixels, hotspot, desktop position and
 * visibility (physical pixels on Windows, desktop points on macOS).
 * Immutable and safe to share across capture threads. Null reports a sampling error;
 * a hidden cursor is a valid snapshot. */
ClipCaptureCursorSnapshot* clip_capture_cursor_snapshot_create(void);
void clip_capture_cursor_snapshot_destroy(ClipCaptureCursorSnapshot* snapshot);
/* Apply once to a result captured WITHOUT INCLUDE_CURSOR, before publishing image leases.
 * Uses this exact snapshot for every display, never the backend's cached cursor metadata.
 * Existing leases remain immutable. Returns 1 on success, including a hidden/out-of-frame cursor.
 */
uint8_t clip_capture_screenshot_result_composite_cursor(ClipCaptureScreenshotResult* result,
                                                        const ClipCaptureCursorSnapshot* snapshot);
typedef struct ClipCaptureStreamImpl ClipCaptureStream;
typedef struct ClipCaptureStreamFrameImpl ClipCaptureStreamFrame;

typedef enum ClipCaptureBackendKind {
    CLIP_CAPTURE_BACKEND_AUTO = 0,
    CLIP_CAPTURE_BACKEND_DXGI = 1,
    CLIP_CAPTURE_BACKEND_WGC = 2,
    CLIP_CAPTURE_BACKEND_GDI = 3,
    CLIP_CAPTURE_BACKEND_SCREEN_CAPTURE_KIT = 4,
} ClipCaptureBackendKind;

typedef enum ClipCaptureWgcUpdateMode {
    CLIP_CAPTURE_WGC_UPDATE_MODE_AUTO = 0,
    CLIP_CAPTURE_WGC_UPDATE_MODE_COMPLETE_ONLY = 1,
    CLIP_CAPTURE_WGC_UPDATE_MODE_ORDERED_INCREMENTAL = 2,
} ClipCaptureWgcUpdateMode;

typedef enum ClipCapturePixelFormat {
    CLIP_CAPTURE_PIXEL_FORMAT_RGBA8 = 0,
    CLIP_CAPTURE_PIXEL_FORMAT_BGRA8 = 1,
} ClipCapturePixelFormat;

/* Shared by capture and recording. Lists are copied and deduplicated during creation,
 * limited to 4096 entries each, and use macOS WindowServer IDs / process IDs.
 * Nonempty lists require non-null pointers. Filters are fixed for the session's
 * lifetime and apply to display/region captures; independent window captures
 * reject filters. Windows continues to use native display affinity. */
#ifndef CLIP_CAPTURE_EXCLUSIONS_DEFINED
#define CLIP_CAPTURE_EXCLUSIONS_DEFINED
typedef struct ClipCaptureExclusions {
    const uint32_t* windows;
    size_t window_count;
    const int32_t* processes;
    size_t process_count;
} ClipCaptureExclusions;
#endif

typedef struct ClipCaptureDesktopSessionConfig {
    size_t capture_retry_count;
    uint8_t wgc_update_mode;
    /* Preferred backend; remaining backends are tried in default order on eligible failures.
       AUTO retains the default priority for each capture target. */
    uint8_t capture_backend;
    uint8_t pixel_format;
    uint8_t reserved[29];
    ClipCaptureExclusions exclusions;
} ClipCaptureDesktopSessionConfig;

/* No complete byte accounting exists for native resources and shared runtimes.
   Check this
 * sentinel before displaying, adding, or comparing resource bytes. */
#define CLIP_CAPTURE_RESOURCE_BYTES_UNKNOWN UINT64_MAX

typedef struct ClipCaptureDesktopSessionState {
    size_t worker_count;
    uint8_t prepared;
    uint8_t reserved0[3];
    uint32_t active_capture_access_count;
    /* CLIP_CAPTURE_RESOURCE_BYTES_UNKNOWN when no complete measurement exists.
       Zero must not
     * be inferred from active_capture_access_count == 0. */
    uint64_t retained_resource_bytes;
    const char* backend_kind;
} ClipCaptureDesktopSessionState;

typedef struct ClipCaptureFrameInfo {
    const char* stable_id;
    const char* name;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint8_t is_primary;
    uint8_t backend_kind;
    uint8_t pixel_format;
    uint8_t reserved0;
    uint32_t stride_bytes;
    const uint8_t* rgba_bytes;
    size_t rgba_len;
} ClipCaptureFrameInfo;

/* Retained cursor replacement pixels in display-local image coordinates. Desktop pixels and
 *
 * existing leases remain immutable. Success with a null lease means hidden/out-of-frame.
 * Release
 * a non-null lease with clip_capture_frame_lease_release. */
uint8_t clip_capture_screenshot_result_cursor_patch(const ClipCaptureScreenshotResult* result,
                                                    const ClipCaptureCursorSnapshot* snapshot,
                                                    size_t index, ClipCaptureFrameInfo* out_info,
                                                    ClipCaptureFrameLease** out_lease);

/* Geometry belongs to the captured frame, not a later desktop enumeration.
 * coordinate_space: 0 = desktop pixels (Windows), 1 = desktop points (macOS).
 * width/height here describe desktop geometry; FrameInfo dimensions are pixels. */
#define CLIP_CAPTURE_FRAME_GEOMETRY_VERSION 1
typedef struct ClipCaptureFrameGeometry {
    uint32_t version;
    uint32_t struct_size;
    uint32_t coordinate_space;
    uint32_t display_id;
    double x;
    double y;
    double width;
    double height;
    double backing_scale;
} ClipCaptureFrameGeometry;

uint8_t clip_capture_screenshot_result_display_geometry(const ClipCaptureScreenshotResult* result,
                                                        size_t index,
                                                        ClipCaptureFrameGeometry* geometry);
uint8_t
clip_capture_screenshot_result_focused_window_geometry(const ClipCaptureScreenshotResult* result,
                                                       ClipCaptureFrameGeometry* geometry);

/* A single monitor identified by its native device name (for example \\.\DISPLAY1).
 * Uses DXGI, WGC, then GDI on eligible failures, independently of desktop sessions. */
typedef struct ClipCaptureMonitorSessionConfig {
    const char* device_name_utf8;
    size_t capture_retry_count;
    uint8_t pixel_format;
    uint8_t reserved[31];
    ClipCaptureExclusions exclusions;
} ClipCaptureMonitorSessionConfig;

ClipCaptureMonitorSession*
clip_capture_monitor_session_create(const ClipCaptureMonitorSessionConfig* config);
void clip_capture_monitor_session_destroy(ClipCaptureMonitorSession* session);
uint8_t clip_capture_monitor_session_capture(ClipCaptureMonitorSession* session,
                                             ClipCaptureFrameInfo* out_info);
/* Retained pixels survive the next capture and session destruction. */
ClipCaptureFrameLease*
clip_capture_monitor_session_frame_retain(const ClipCaptureMonitorSession* session);

typedef struct ClipCaptureRegionSessionConfig {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    size_t capture_retry_count;
    uint8_t wgc_update_mode;
    uint8_t capture_backend;
    uint8_t pixel_format;
    uint8_t reserved[29];
    ClipCaptureExclusions exclusions;
} ClipCaptureRegionSessionConfig;

typedef struct ClipCaptureRegionFrameInfo {
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    uint8_t is_duplicate;
    uint8_t pixel_format;
    uint8_t reserved0[2];
    const uint8_t* rgba_bytes;
    size_t rgba_len;
} ClipCaptureRegionFrameInfo;

#define CLIP_CAPTURE_STREAM_CONFIG_VERSION 2u
#define CLIP_CAPTURE_STREAM_FRAME_INFO_VERSION 1u

typedef enum ClipCaptureStreamEventKind {
    CLIP_CAPTURE_STREAM_EVENT_TIMEOUT = 0,
    CLIP_CAPTURE_STREAM_EVENT_FRAME = 1,
    CLIP_CAPTURE_STREAM_EVENT_FRAMES_DROPPED = 2,
    CLIP_CAPTURE_STREAM_EVENT_RESOLUTION_CHANGED = 3,
    CLIP_CAPTURE_STREAM_EVENT_PAUSED = 4,
    CLIP_CAPTURE_STREAM_EVENT_RESUMED = 5,
    CLIP_CAPTURE_STREAM_EVENT_ENDED = 6,
    CLIP_CAPTURE_STREAM_EVENT_ERROR = 7,
} ClipCaptureStreamEventKind;

typedef struct ClipCaptureStreamConfig {
    uint32_t version;
    uint32_t struct_size;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t target_fps;
    uint32_t min_fps;
    uint32_t buffer_depth;
    uint32_t max_consecutive_errors;
    size_t capture_retry_count;
    uint8_t wgc_update_mode;
    uint8_t capture_backend;
    uint8_t pixel_format;
    uint8_t adaptive_fps;
    /* Set to zero for scrolling captures to avoid cursor compositing work. */
    uint8_t include_cursor;
    /* Opt in to reversal of supported full-screen Magnifier effects.
       The effect is sampled once when the stream is created. */
    uint8_t restore_original_colors;
    uint8_t reserved[26];
    ClipCaptureExclusions exclusions;
} ClipCaptureStreamConfig;

typedef struct ClipCaptureStreamEvent {
    ClipCaptureStreamEventKind kind;
    ClipCaptureStreamFrame* frame;
    uint64_t dropped_count;
    uint32_t old_width;
    uint32_t old_height;
    uint32_t new_width;
    uint32_t new_height;
    uint8_t reserved[32];
} ClipCaptureStreamEvent;

typedef struct ClipCaptureStreamFrameInfo {
    uint32_t version;
    uint32_t struct_size;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    uint8_t is_duplicate;
    uint8_t pixel_format;
    /* Actual frame backend; consumes a reserved byte without changing ABI layout. */
    uint8_t backend_kind;
    uint8_t reserved0[1];
    uint64_t sequence;
    const uint8_t* rgba_bytes;
    size_t rgba_len;
} ClipCaptureStreamFrameInfo;

typedef struct ClipCaptureStreamStats {
    uint64_t frames_captured;
    uint64_t frames_dropped;
    uint64_t errors_recovered;
    double current_fps;
    uint32_t target_fps;
    uint32_t buffer_fill;
    uint64_t capture_latency_ns;
} ClipCaptureStreamStats;

/* A native top-level window capture backed by Windows Graphics Capture when
 * the platform supports it. The returned pixel pointer remains valid until
 * the next capture, explicit frame release, or destroy. */
typedef struct ClipCaptureWindowSessionConfig {
    intptr_t hwnd;
    size_t capture_retry_count;
    uint8_t wgc_update_mode;
    uint8_t capture_backend;
    uint8_t pixel_format;
    uint8_t reserved[29];
} ClipCaptureWindowSessionConfig;

#define CLIP_CAPTURE_WINDOW_FRAME_INFO_VERSION 1u

typedef struct ClipCaptureWindowFrameInfo {
    uint32_t version;
    uint32_t struct_size;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    const uint8_t* rgba_bytes;
    size_t rgba_len;
    uint8_t backend_kind;
    uint8_t pixel_format;
    uint8_t reserved[6];
} ClipCaptureWindowFrameInfo;

#define CLIP_CAPTURE_SCREENSHOT_REQUEST_VERSION 1u
#define CLIP_CAPTURE_SCREENSHOT_REQUEST_REFRESH_LAYOUT (1u << 0)
#define CLIP_CAPTURE_SCREENSHOT_REQUEST_RESTORE_ORIGINAL_COLORS (1u << 1)
#define CLIP_CAPTURE_SCREENSHOT_REQUEST_INCLUDE_CURSOR (1u << 2)

typedef struct ClipCaptureScreenshotRequest {
    uint32_t version;
    uint32_t struct_size;
    uint32_t flags;
    uint32_t reserved0;
    intptr_t focused_window;
    const ClipCaptureCancellationToken* cancellation_token;
    uint8_t reserved[32];
} ClipCaptureScreenshotRequest;

ClipCaptureDesktopSession*
clip_capture_desktop_session_create(const ClipCaptureDesktopSessionConfig* config);
void clip_capture_desktop_session_destroy(ClipCaptureDesktopSession* session);

uint8_t clip_capture_desktop_session_prepare(ClipCaptureDesktopSession* session);
uint8_t clip_capture_desktop_session_state(ClipCaptureDesktopSession* session,
                                           ClipCaptureDesktopSessionState* out_state);
/* Immutable owned snapshot. Descriptor strings live until snapshot destruction. Snapshot calls
 * run on the session's owning thread; descriptor reads may run on any thread. */
typedef struct ClipCaptureDisplayDescriptor {
    uint32_t version, struct_size;
    const char* stable_id;
    const char* name;
    uint32_t display_id, coordinate_space;
    double x, y, width, height, backing_scale;
    uint32_t pixel_width, pixel_height, is_primary;
} ClipCaptureDisplayDescriptor;
ClipCaptureDesktopLayout* clip_capture_desktop_session_layout_snapshot(ClipCaptureDesktopSession*,
                                                                       uint8_t refresh);
void clip_capture_desktop_layout_destroy(ClipCaptureDesktopLayout*);
size_t clip_capture_desktop_layout_count(const ClipCaptureDesktopLayout*);
uint8_t clip_capture_desktop_layout_display(const ClipCaptureDesktopLayout*, size_t,
                                            ClipCaptureDisplayDescriptor*);
ClipCaptureScreenshotResult*
clip_capture_desktop_session_capture_with_layout(ClipCaptureDesktopSession*,
                                                 const ClipCaptureScreenshotRequest*,
                                                 const ClipCaptureDesktopLayout*);

uint8_t clip_capture_desktop_session_refresh_layout(ClipCaptureDesktopSession* session);
uint8_t clip_capture_desktop_session_reset_to_prepared(ClipCaptureDesktopSession* session);
/* Captures every display and, when focused_window is nonzero, the requested
 * window as one all-or-nothing transaction. The returned result owns its
 * frame buffers and must be destroyed by the caller. */
ClipCaptureScreenshotResult*
clip_capture_desktop_session_capture(ClipCaptureDesktopSession* session,
                                     const ClipCaptureScreenshotRequest* request);

/* Cancellation may be signaled from another thread. The token must remain
 * alive until every capture call that references it has returned. */
ClipCaptureCancellationToken* clip_capture_cancellation_token_create(void);
void clip_capture_cancellation_token_cancel(ClipCaptureCancellationToken* token);
void clip_capture_cancellation_token_destroy(ClipCaptureCancellationToken* token);

/* Pointers returned through frame-info structures remain valid until the
 * result is destroyed. Retaining the corresponding frame lease extends the
 * pixel-buffer lifetime beyond result destruction. */
size_t clip_capture_screenshot_result_display_count(const ClipCaptureScreenshotResult* result);
uint8_t clip_capture_screenshot_result_display_info(const ClipCaptureScreenshotResult* result,
                                                    size_t index, ClipCaptureFrameInfo* out_info);
ClipCaptureFrameLease*
clip_capture_screenshot_result_display_retain(const ClipCaptureScreenshotResult* result,
                                              size_t index);
uint8_t
clip_capture_screenshot_result_focused_window_info(const ClipCaptureScreenshotResult* result,
                                                   ClipCaptureWindowFrameInfo* out_info);
ClipCaptureFrameLease*
clip_capture_screenshot_result_focused_window_retain(const ClipCaptureScreenshotResult* result);
void clip_capture_screenshot_result_destroy(ClipCaptureScreenshotResult* result);

ClipCaptureRegionSession*
clip_capture_region_session_create(const ClipCaptureRegionSessionConfig* config);
void clip_capture_region_session_destroy(ClipCaptureRegionSession* session);
uint8_t clip_capture_region_session_prepare(ClipCaptureRegionSession* session);
/* The returned pixel pointer remains valid until the next capture or destroy. */
uint8_t clip_capture_region_session_capture(ClipCaptureRegionSession* session,
                                            ClipCaptureRegionFrameInfo* out_info);

/* Starts a continuous region stream. Frames are delivered as explicit leases;
 * a frame event must be released with clip_capture_stream_frame_release(). */
ClipCaptureStream* clip_capture_stream_create_region(const ClipCaptureStreamConfig* config);
void clip_capture_stream_destroy(ClipCaptureStream* stream);
uint8_t clip_capture_stream_stop(ClipCaptureStream* stream);
uint8_t clip_capture_stream_set_target_fps(ClipCaptureStream* stream, uint32_t target_fps);
/* A timeout is a successful receive with kind TIMEOUT. The function only
 * returns zero for invalid arguments or an internal API error. */
uint8_t clip_capture_stream_receive(ClipCaptureStream* stream, uint32_t timeout_ms,
                                    ClipCaptureStreamEvent* out_event);
uint8_t clip_capture_stream_frame_info(const ClipCaptureStreamFrame* frame,
                                       ClipCaptureStreamFrameInfo* out_info);
void clip_capture_stream_frame_release(ClipCaptureStreamFrame* frame);
uint8_t clip_capture_stream_stats(const ClipCaptureStream* stream,
                                  ClipCaptureStreamStats* out_stats);

ClipCaptureWindowSession*
clip_capture_window_session_create(const ClipCaptureWindowSessionConfig* config);
void clip_capture_window_session_destroy(ClipCaptureWindowSession* session);
uint8_t clip_capture_window_session_prepare(ClipCaptureWindowSession* session);
uint8_t clip_capture_window_session_capture(ClipCaptureWindowSession* session,
                                            ClipCaptureWindowFrameInfo* out_info);
ClipCaptureFrameLease*
clip_capture_window_session_frame_retain(const ClipCaptureWindowSession* session);
/* Clears the session-owned frame and restores the prepared state. Retained
 * frame leases remain valid; pointers obtained from frame-info calls are
 * invalid after this function returns. */
uint8_t clip_capture_window_session_release_frame(ClipCaptureWindowSession* session);

void clip_capture_frame_lease_release(ClipCaptureFrameLease* lease);

const char* clip_capture_last_error_message(void);

#ifdef __cplusplus
}
#endif
