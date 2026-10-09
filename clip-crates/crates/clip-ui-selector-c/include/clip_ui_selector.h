#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClipUiSelectorServiceImpl ClipUiSelectorService;
typedef enum ClipUiSelectorBackend {
    CLIP_UI_SELECTOR_BACKEND_UIA,
    CLIP_UI_SELECTOR_BACKEND_MSAA,
    CLIP_UI_SELECTOR_BACKEND_ACCESSIBILITY
} ClipUiSelectorBackend;
typedef enum ClipUiSelectorHitTestMode {
    CLIP_UI_SELECTOR_HIT_TEST_MODE_UI_ELEMENT,
    CLIP_UI_SELECTOR_HIT_TEST_MODE_WINDOW
} ClipUiSelectorHitTestMode;
typedef enum ClipUiSelectorPhase {
    CLIP_UI_SELECTOR_INITIAL,
    CLIP_UI_SELECTOR_REFINEMENT,
    CLIP_UI_SELECTOR_FINISHED
} ClipUiSelectorPhase;
typedef enum ClipUiSelectorStopReason {
    CLIP_UI_SELECTOR_COMPLETE,
    CLIP_UI_SELECTOR_BUDGET_EXHAUSTED,
    CLIP_UI_SELECTOR_DECODING_PENDING,
    CLIP_UI_SELECTOR_PROVIDER_TIMEOUT,
    CLIP_UI_SELECTOR_PROVIDER_FAILURE,
    CLIP_UI_SELECTOR_CANCELLED,
    CLIP_UI_SELECTOR_TRAVERSAL_LIMIT,
    CLIP_UI_SELECTOR_PERMISSION_REQUIRED,
    CLIP_UI_SELECTOR_ACCESSIBILITY_PENDING
} ClipUiSelectorStopReason;
typedef struct ClipUiSelectorRect {
    int32_t left, top, right, bottom;
} ClipUiSelectorRect;
typedef struct ClipUiSelectorQuery {
    uint64_t epoch, request_id, generation;
    int32_t x, y;
    ClipUiSelectorHitTestMode mode;
    /* CGDirectDisplayID on macOS; zero selects the first matching display. Ignored on Windows. */
    uint32_t display_id;
    /* macOS UI-thread mouse hit, preserved for refinement. When window_hit_tested is 1,
       window_id=0 means desktop. Only matching snapshot windows may be selected.
       Set window_hit_tested=0 for rectangle-based selection. Ignored on Windows. */
    uintptr_t window_id;
    uint8_t window_hit_tested;
} ClipUiSelectorQuery;
typedef struct ClipUiSelectorEvent {
    ClipUiSelectorQuery query;
    ClipUiSelectorPhase phase;
    ClipUiSelectorStopReason reason;
    uint8_t ok;
    uint64_t elapsed_us;
    const ClipUiSelectorRect* rects;
    size_t count;
} ClipUiSelectorEvent;
/* Events and rectangles are borrowed for the callback duration. Callbacks may run on
   either worker or the submitting thread, are serialized, must copy/queue only, and must not
   re-enter the service. Destroy closes delivery synchronously but joins provider workers in the
   background. */
typedef void (*ClipUiSelectorEventCallback)(const ClipUiSelectorEvent*, void*);
typedef void (*ClipUiSelectorRefreshCallback)(uint64_t epoch, uint8_t ok, void*);
ClipUiSelectorService* clip_ui_selector_service_create(ClipUiSelectorEventCallback,
                                                       ClipUiSelectorRefreshCallback, void*);
void clip_ui_selector_service_destroy(ClipUiSelectorService*);
uint8_t clip_ui_selector_service_release_cache(ClipUiSelectorService*);
uint8_t clip_ui_selector_service_refresh(ClipUiSelectorService*, uint64_t epoch,
                                         ClipUiSelectorBackend, const uintptr_t*, size_t);
/* Geometry is copied before returning; count must be 1..128. Coordinates are native desktop
 * pixels on Windows (space=0), Quartz points plus oriented pixel dimensions on macOS (space=1). */
typedef struct ClipUiSelectorDisplay {
    uint32_t version, struct_size, display_id, coordinate_space;
    double x, y, width, height;
    uint32_t pixel_width, pixel_height;
} ClipUiSelectorDisplay;
uint8_t clip_ui_selector_service_refresh_with_displays(ClipUiSelectorService*, uint64_t epoch,
                                                       ClipUiSelectorBackend, const uintptr_t*,
                                                       size_t, const ClipUiSelectorDisplay*,
                                                       size_t);
uint8_t clip_ui_selector_service_query(ClipUiSelectorService*, const ClipUiSelectorQuery*);
uint8_t clip_ui_selector_service_refine(ClipUiSelectorService*, const ClipUiSelectorQuery*);
/* Pass prompt=1 only for an explicit user action. This never prompts when prompt=0. */
uint8_t clip_ui_selector_accessibility_permission(uint8_t prompt);
void clip_ui_selector_service_invalidate_refinement(ClipUiSelectorService*);
#ifdef __cplusplus
}
#endif
