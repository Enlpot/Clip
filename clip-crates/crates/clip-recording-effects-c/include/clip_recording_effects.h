#ifndef CLIP_RECORDING_EFFECTS_H
#define CLIP_RECORDING_EFFECTS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClipRecordingEffects ClipRecordingEffects;
typedef struct ClipRecordingEffectsFrame ClipRecordingEffectsFrame;
typedef void (*ClipRecordingEffectsNotify)(void* context);
typedef struct ClipRecordingEffectsKeyLabel {
    uint32_t virtual_key;
    const char* label_utf8;
} ClipRecordingEffectsKeyLabel;

#define CLIP_RECORDING_EFFECTS_CONFIG_VERSION 6u
typedef struct ClipRecordingEffectsConfig {
    uint32_t version;
    uint32_t struct_size;
    int32_t x, y;
    uint32_t width, height;
    /* Export dimensions, independent of the preview canvas. */
    uint32_t output_width, output_height;
    uint32_t trail_rgba, click_rgba;
    uint32_t show_keyboard;
    uint32_t keyboard_background_rgba, keyboard_text_rgba, keyboard_border_rgba;
    const ClipRecordingEffectsKeyLabel* labels;
    uint32_t label_count;
    /* Version 1 reserved this field as zero (500 ms lifetime). */
    uint32_t trail_duration_ms;
    uint64_t generation;
    /* Version 3: keycap height in pixels (32..128). */
    uint32_t keyboard_size;
    uint32_t reserved_v3;
    /* Version 4: live alpha approximation of the saved multiply highlight. */
    uint32_t highlight_rgba;
    uint32_t record_mouse_clicks;
    /* Version 5: optional application font; strings are copied during configuration.
       Null family selects the system UI font. Weight uses OpenType values (1..999). */
    const char* keyboard_font_family_utf8;
    const char* keyboard_cjk_font_family_utf8;
    uint32_t keyboard_font_weight;
    /* Version 6: physical display pixels per native desktop unit.
       Use the display DPR for macOS points and 1 for Windows physical pixels. */
    double canvas_scale;
} ClipRecordingEffectsConfig;

typedef struct ClipRecordingEffectsTile {
    uint32_t x, y;
    uint32_t width, height;
    uint32_t stride;
    const uint8_t* rgba_premultiplied;
} ClipRecordingEffectsTile;

typedef struct ClipRecordingEffectsFrameInfo {
    uint64_t generation, revision;
    uint32_t width, height;
    const ClipRecordingEffectsTile* tiles;
    uint32_t tile_count;
    const char* error_utf8;
} ClipRecordingEffectsFrameInfo;

// Config strings are copied. Control calls belong to one owner thread.
// Notification runs on the worker; enqueue work and return without calling control APIs.
ClipRecordingEffects* clip_recording_effects_create(const ClipRecordingEffectsConfig* config,
                                                    ClipRecordingEffectsNotify notify,
                                                    void* context);
int32_t clip_recording_effects_configure(ClipRecordingEffects* effects,
                                         const ClipRecordingEffectsConfig* config);
int32_t clip_recording_effects_set_active(ClipRecordingEffects* effects, int32_t active);
// Stops and joins all observers before returning. No subsequent notification can occur.
void clip_recording_effects_stop(ClipRecordingEffects* effects);
void clip_recording_effects_destroy(ClipRecordingEffects* effects);
// A complete immutable snapshot. All tile pointers remain valid until release, including after
// stop.
ClipRecordingEffectsFrame* clip_recording_effects_acquire_frame(ClipRecordingEffects* effects);
int32_t clip_recording_effects_frame_info(const ClipRecordingEffectsFrame* frame,
                                          ClipRecordingEffectsFrameInfo* info);
// Both layers use physical display pixels and fixed pixel styles. FrameInfo width/height
// describe that coordinate space independently of the configured export dimensions.
// Keyboard layer, drawn above the mouse layer returned by
// frame_info. This is a complete snapshot, including an empty tile array when keys expire. Same
// frame lease.
int32_t clip_recording_effects_frame_keyboard_info(const ClipRecordingEffectsFrame* frame,
                                                   ClipRecordingEffectsFrameInfo* info);
void clip_recording_effects_release_frame(ClipRecordingEffectsFrame* frame);
const char* clip_recording_effects_last_error(void);
// Live handles created and not yet destroyed; for leak diagnostics in tests.
size_t clip_recording_effects_live_handle_count(void);
#ifdef __cplusplus
}
#endif
#endif
