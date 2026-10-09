#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClipStitchFramePoolImpl ClipStitchFramePool;
typedef struct ClipStitchFrameBufferImpl ClipStitchFrameBuffer;
typedef struct ClipStitchSessionImpl ClipStitchSession;
typedef struct ClipStitchSnapshotImpl ClipStitchSnapshot;
typedef struct ClipStitchOwnedImageImpl ClipStitchOwnedImage;
typedef struct ClipStitchExportTaskImpl ClipStitchExportTask;

#if defined(CLIP_STITCH_PERF_INSTRUMENTATION)
typedef enum ClipStitchPerfStage {
    CLIP_STITCH_PERF_FRAME_FREEZE = 0,
    CLIP_STITCH_PERF_DUPLICATE_CHECK = 1,
    CLIP_STITCH_PERF_REFERENCE_PREPARATION = 2,
    CLIP_STITCH_PERF_GRAYSCALE = 3,
    CLIP_STITCH_PERF_SIMILARITY_MAPS = 4,
    CLIP_STITCH_PERF_FEATURE_EXTRACTION = 5,
    CLIP_STITCH_PERF_DESCRIPTOR_MATCHING = 6,
    CLIP_STITCH_PERF_CANDIDATE_SCORING = 7,
    CLIP_STITCH_PERF_REFINEMENT = 8,
    CLIP_STITCH_PERF_REGION_UPDATE = 9,
    CLIP_STITCH_PERF_CANVAS_COMPOSITION = 10,
    CLIP_STITCH_PERF_REFERENCE_SYNTHESIS = 11,
    CLIP_STITCH_PERF_PREVIEW_SCALING = 12,
    CLIP_STITCH_PERF_PUSH_TOTAL = 13,
    CLIP_STITCH_PERF_INITIALIZATION = 14,
    CLIP_STITCH_PERF_RESERVED = 15,
    CLIP_STITCH_PERF_STAGE_COUNT = 16,
} ClipStitchPerfStage;
typedef struct ClipStitchPerfSnapshot {
    uint64_t elapsed_ns[CLIP_STITCH_PERF_STAGE_COUNT];
    uint64_t calls[CLIP_STITCH_PERF_STAGE_COUNT];
} ClipStitchPerfSnapshot;
/* Reset/read the calling thread only. Read preserves the counters. */
void clip_stitch_perf_reset_thread(void);
uint8_t clip_stitch_perf_read_thread(ClipStitchPerfSnapshot* output, size_t output_size);
#endif

typedef enum ClipStitchFrameEvent {
    CLIP_STITCH_FRAME_EVENT_INITIAL = 0,
    CLIP_STITCH_FRAME_EVENT_EXTENDED_TOP = 1,
    CLIP_STITCH_FRAME_EVENT_EXTENDED_BOTTOM = 2,
    CLIP_STITCH_FRAME_EVENT_COVERED = 3,
    CLIP_STITCH_FRAME_EVENT_DUPLICATE = 4,
    CLIP_STITCH_FRAME_EVENT_UNMATCHED = 5,
    CLIP_STITCH_FRAME_EVENT_EXTENDED_LEFT = 6,
    CLIP_STITCH_FRAME_EVENT_EXTENDED_RIGHT = 7,
} ClipStitchFrameEvent;

typedef enum ClipStitchAxis {
    CLIP_STITCH_AXIS_VERTICAL = 0,
    CLIP_STITCH_AXIS_HORIZONTAL = 1,
} ClipStitchAxis;

typedef enum ClipStitchUnmatchedReason {
    CLIP_STITCH_UNMATCHED_REASON_NONE = 0,
    CLIP_STITCH_UNMATCHED_REASON_INSUFFICIENT_OVERLAP = 1,
    CLIP_STITCH_UNMATCHED_REASON_LOW_INFORMATION = 2,
    CLIP_STITCH_UNMATCHED_REASON_AMBIGUOUS = 3,
    CLIP_STITCH_UNMATCHED_REASON_CONFLICTING_REFERENCES = 4,
    CLIP_STITCH_UNMATCHED_REASON_FIXED_CONTENT_DOMINATED = 5,
    CLIP_STITCH_UNMATCHED_REASON_VERIFICATION_FAILED = 6,
} ClipStitchUnmatchedReason;

typedef struct ClipStitchConfig {
    uint32_t struct_size;
    uint32_t max_output_height;
    uint64_t max_output_pixels;
    uint32_t min_overlap_rows;
    float min_overlap_ratio;
    uint32_t accepted_history_capacity;
    /* Consumes one formerly reserved slot; the structure size is unchanged. */
    uint32_t axis;
    uint32_t reserved[8];
} ClipStitchConfig;

typedef struct ClipStitchMatchMetrics {
    float score;
    float second_score;
    float content_coverage;
    float fixed_coverage;
    float inlier_ratio;
    uint32_t feature_support;
    uint32_t reference_count;
} ClipStitchMatchMetrics;

typedef struct ClipStitchFrameOutcome {
    ClipStitchFrameEvent event;
    ClipStitchUnmatchedReason unmatched_reason;
    int32_t matched_reference_offset_y;
    uint8_t has_matched_reference_offset_y;
    uint8_t reserved[3];
    ClipStitchMatchMetrics metrics;
    uint32_t added_rows;
    uint32_t output_width;
    uint32_t output_height;
    /* Full edge band rewritten by this frame, including splice overlap. */
    uint32_t delta_top;
    uint32_t delta_rows;
} ClipStitchFrameOutcome;

typedef struct ClipStitchMutableImageInfo {
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    uint8_t* rgba_bytes;
    size_t rgba_len;
} ClipStitchMutableImageInfo;

typedef struct ClipStitchImageInfo {
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    const uint8_t* rgba_bytes;
    size_t rgba_len;
} ClipStitchImageInfo;

typedef enum ClipStitchPngCompression {
    CLIP_STITCH_PNG_COMPRESSION_FAST = 0,
    CLIP_STITCH_PNG_COMPRESSION_BALANCED = 1,
    CLIP_STITCH_PNG_COMPRESSION_BEST = 2,
} ClipStitchPngCompression;

typedef enum ClipStitchExportStage {
    CLIP_STITCH_EXPORT_STAGE_PREPARING = 0,
    CLIP_STITCH_EXPORT_STAGE_ENCODING = 1,
    CLIP_STITCH_EXPORT_STAGE_COMMITTING = 2,
} ClipStitchExportStage;

typedef enum ClipStitchExportStatus {
    CLIP_STITCH_EXPORT_STATUS_RUNNING = 0,
    CLIP_STITCH_EXPORT_STATUS_COMPLETE = 1,
    CLIP_STITCH_EXPORT_STATUS_FAILED = 2,
    CLIP_STITCH_EXPORT_STATUS_CANCELED = 3,
} ClipStitchExportStatus;

typedef struct ClipStitchPngExportConfig {
    const char* output_path_utf8;
    ClipStitchPngCompression compression;
    uint8_t overwrite;
    uint8_t reserved[31];
} ClipStitchPngExportConfig;

typedef struct ClipStitchExportProgress {
    ClipStitchExportStage stage;
    uint32_t rows_written;
    uint32_t total_rows;
    float percent;
} ClipStitchExportProgress;

ClipStitchFramePool* clip_stitch_frame_pool_create(uint32_t width, uint32_t height,
                                                   size_t capacity);
void clip_stitch_frame_pool_destroy(ClipStitchFramePool* pool);
/* Returns NULL while all bounded slots are owned by the pipeline. */
ClipStitchFrameBuffer* clip_stitch_frame_pool_acquire(ClipStitchFramePool* pool);
/* Same bounded acquisition, with initialized but unspecified pixels. The caller must
 * overwrite every packed RGBA byte before submitting the frame. */
ClipStitchFrameBuffer* clip_stitch_frame_pool_acquire_for_overwrite(ClipStitchFramePool* pool);
/* The writable pixel pointer remains valid until the frame is consumed/destroyed. */
uint8_t clip_stitch_frame_buffer_info(ClipStitchFrameBuffer* frame,
                                      ClipStitchMutableImageInfo* out_info);
void clip_stitch_frame_buffer_destroy(ClipStitchFrameBuffer* frame);

uint8_t clip_stitch_config_default(ClipStitchConfig* out_config);
ClipStitchSession* clip_stitch_session_create(const ClipStitchConfig* config);
void clip_stitch_session_destroy(ClipStitchSession* session);
uint8_t clip_stitch_session_reset(ClipStitchSession* session);
/* Always consumes *inout_frame and sets the caller's slot to NULL. */
uint8_t clip_stitch_session_push_owned(ClipStitchSession* session,
                                       ClipStitchFrameBuffer** inout_frame,
                                       ClipStitchFrameOutcome* out_outcome);
/* Retains tightly packed immutable RGBA pixels without copying them.
 * A non-NULL release callback
 * transfers context even on failure. It runs exactly
 * once after the last input lease, possibly
 * on another thread. Pixels must remain
 * immutable and valid until that call. A NULL callback
 * rejects the call without
 * taking context. */
uint8_t clip_stitch_session_push_external_rgba(ClipStitchSession* session, uint32_t width,
                                               uint32_t height, const uint8_t* rgba_bytes,
                                               size_t rgba_len, void (*release)(void*),
                                               void* context, ClipStitchFrameOutcome* out_outcome);
uint8_t clip_stitch_session_copy_rows(const ClipStitchSession* session, uint32_t top, uint32_t rows,
                                      uint8_t* destination, size_t destination_len);
ClipStitchOwnedImage* clip_stitch_session_materialize_rows(const ClipStitchSession* session,
                                                           uint32_t top, uint32_t bottom);
ClipStitchOwnedImage* clip_stitch_session_materialize_axis(const ClipStitchSession* session,
                                                           uint32_t start, uint32_t end);
ClipStitchOwnedImage* clip_stitch_session_render_scaled_rows(const ClipStitchSession* session,
                                                             uint32_t top, uint32_t rows,
                                                             uint32_t width, uint32_t height);
ClipStitchOwnedImage* clip_stitch_session_render_scaled_axis(const ClipStitchSession* session,
                                                             uint32_t start, uint32_t span,
                                                             uint32_t width, uint32_t height);
ClipStitchSnapshot* clip_stitch_session_snapshot(const ClipStitchSession* session,
                                                 uint32_t trim_top, uint32_t trim_bottom);
ClipStitchSnapshot* clip_stitch_session_snapshot_axis(const ClipStitchSession* session,
                                                      uint32_t start, uint32_t end);

/* Snapshots retain shared immutable canvas tiles and do not depend on session lifetime.
 * Creating
 * or slicing a snapshot does not copy canvas pixels; materialization does. */
void clip_stitch_snapshot_destroy(ClipStitchSnapshot* snapshot);
uint8_t clip_stitch_snapshot_info(const ClipStitchSnapshot* snapshot,
                                  ClipStitchImageInfo* out_info);
/* Copies tightly packed RGBA rows into a caller-owned strided destination. */
uint8_t clip_stitch_snapshot_copy_rows(const ClipStitchSnapshot* snapshot, uint32_t top,
                                       uint32_t rows, size_t destination_stride,
                                       uint8_t* destination, size_t destination_len);
ClipStitchSnapshot* clip_stitch_snapshot_slice_rows(const ClipStitchSnapshot* snapshot,
                                                    uint32_t top, uint32_t bottom);
ClipStitchSnapshot* clip_stitch_snapshot_slice_axis(const ClipStitchSnapshot* snapshot,
                                                    uint32_t start, uint32_t end);
ClipStitchOwnedImage* clip_stitch_snapshot_materialize(const ClipStitchSnapshot* snapshot);
ClipStitchOwnedImage* clip_stitch_snapshot_render_scaled(const ClipStitchSnapshot* snapshot,
                                                         uint32_t width, uint32_t height);

uint8_t clip_stitch_owned_image_info(const ClipStitchOwnedImage* image,
                                     ClipStitchImageInfo* out_info);
/* Invalidates the pixel pointer returned by clip_stitch_owned_image_info. */
void clip_stitch_owned_image_destroy(ClipStitchOwnedImage* image);

ClipStitchExportTask* clip_stitch_snapshot_export_png(const ClipStitchSnapshot* snapshot,
                                                      const ClipStitchPngExportConfig* config);
void clip_stitch_export_task_cancel(ClipStitchExportTask* task);
ClipStitchExportStatus clip_stitch_export_task_poll(ClipStitchExportTask* task,
                                                    ClipStitchExportProgress* out_progress,
                                                    uint8_t* out_has_progress);
ClipStitchExportStatus clip_stitch_export_task_wait(ClipStitchExportTask* task);
const char* clip_stitch_export_task_error_message(const ClipStitchExportTask* task);
/* Cancels and joins a still-running background export before returning. */
void clip_stitch_export_task_destroy(ClipStitchExportTask* task);

const char* clip_stitch_last_error_message(void);

#ifdef __cplusplus
}
#endif
