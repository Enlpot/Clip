#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClipSelectedTextService ClipSelectedTextService;
typedef struct ClipSelectedTextRequest ClipSelectedTextRequest;
typedef struct ClipSelectedTextResult ClipSelectedTextResult;

/* All status/option values use fixed-width integers, not compiler-sized enums. */
enum {
    CLIP_SELECTED_TEXT_PENDING = 0,
    CLIP_SELECTED_TEXT_SELECTED = 1,
    CLIP_SELECTED_TEXT_NO_SELECTION = 2,
    CLIP_SELECTED_TEXT_UNSUPPORTED = 3,
    CLIP_SELECTED_TEXT_FAILED = 4,
    CLIP_SELECTED_TEXT_INVALID_ARGUMENT = 5
};
/* NATIVE_CONTROL is Windows-only in both method and strategy constants. */
enum {
    CLIP_SELECTED_TEXT_ACCESSIBILITY = 1,
    CLIP_SELECTED_TEXT_NATIVE_CONTROL = 2,
    CLIP_SELECTED_TEXT_CLIPBOARD = 3
};
enum {
    CLIP_SELECTED_TEXT_STRATEGY_AUTO = 0,
    CLIP_SELECTED_TEXT_STRATEGY_ACCESSIBILITY = 1,
    CLIP_SELECTED_TEXT_STRATEGY_NATIVE_CONTROL = 2,
    CLIP_SELECTED_TEXT_STRATEGY_CLIPBOARD = 3
};
enum {
    CLIP_SELECTED_TEXT_CLIPBOARD_UNCHANGED = 0,
    CLIP_SELECTED_TEXT_CLIPBOARD_RESTORED = 1,
    CLIP_SELECTED_TEXT_CLIPBOARD_PRESERVATION_INCOMPLETE = 2,
    CLIP_SELECTED_TEXT_CLIPBOARD_SUPERSEDED = 3,
    CLIP_SELECTED_TEXT_CLIPBOARD_RESTORATION_FAILED = 4,
    CLIP_SELECTED_TEXT_CLIPBOARD_UNKNOWN = 5
};
enum {
    CLIP_SELECTED_TEXT_ERROR_INVALID_CONFIGURATION = 1,
    CLIP_SELECTED_TEXT_ERROR_BUSY = 2,
    CLIP_SELECTED_TEXT_ERROR_WORKER_UNAVAILABLE = 3,
    CLIP_SELECTED_TEXT_ERROR_TIMED_OUT = 4,
    CLIP_SELECTED_TEXT_ERROR_CANCELLED = 5,
    CLIP_SELECTED_TEXT_ERROR_NO_FOREGROUND_WINDOW = 6,
    CLIP_SELECTED_TEXT_ERROR_TARGET_CHANGED = 7,
    CLIP_SELECTED_TEXT_ERROR_PROTECTED_CONTENT = 8,
    CLIP_SELECTED_TEXT_ERROR_ACCESS_DENIED = 9,
    CLIP_SELECTED_TEXT_ERROR_CLIPBOARD_BUSY = 10,
    CLIP_SELECTED_TEXT_ERROR_CLIPBOARD_AMBIGUOUS = 11,
    CLIP_SELECTED_TEXT_ERROR_INPUT_INJECTION_FAILED = 12,
    CLIP_SELECTED_TEXT_ERROR_MALFORMED_DATA = 13,
    CLIP_SELECTED_TEXT_ERROR_LIMIT_EXCEEDED = 14,
    CLIP_SELECTED_TEXT_ERROR_UNSUPPORTED_PLATFORM = 15,
    CLIP_SELECTED_TEXT_ERROR_NATIVE_API = 16,
    CLIP_SELECTED_TEXT_ERROR_COPY_BLOCKED = 17,
    CLIP_SELECTED_TEXT_ERROR_TARGET_UNAVAILABLE = 18
};

/* UTF-8 bytes, never implicitly NUL-terminated. A zero length permits a null pointer. */
typedef struct ClipSelectedTextBytes {
    const uint8_t* data;
    size_t length;
} ClipSelectedTextBytes;

typedef struct ClipSelectedTextOptions {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t timeout_ms;    /* 1..60000; default 2000 */
    uint32_t copy_fallback; /* 0 or 1; default 1; only applies to AUTO */
    uint32_t strategy;      /* STRATEGY_*; default AUTO; explicit strategies never fall back */
    size_t max_text_bytes;  /* 1..64 MiB; default 1 MiB */
    /* Platform-native window/control IDs. macOS currently exposes none in results. */
    const uintptr_t* excluded_native_windows;
    size_t excluded_native_window_count;
    const ClipSelectedTextBytes* excluded_executables; /* UTF-8 basenames */
    size_t excluded_executable_count;                  /* Both lists are limited to 1024 entries. */
} ClipSelectedTextOptions;

typedef struct ClipSelectedTextError {
    uint32_t kind;
    int32_t native_code;
    uint32_t has_native_code;
    uint32_t clipboard_status;
    ClipSelectedTextBytes operation; /* Static lifetime; contains no captured text. */
} ClipSelectedTextError;

typedef struct ClipSelectedTextMetadata {
    uintptr_t native_window; /* Zero when the platform exposes no stable native identifier. */
    uint32_t process_id;
    uintptr_t native_focus; /* Zero when unavailable. */
    uint32_t method;
    uint32_t clipboard_status;
    ClipSelectedTextBytes executable;
    size_t range_count;
} ClipSelectedTextMetadata;

typedef struct ClipSelectedTextRect {
    /* Screen coordinates: physical pixels on Windows, Accessibility points on macOS. */
    double left;
    double top;
    double width;
    double height;
} ClipSelectedTextRect;

/* Non-null pointers must be live and aligned; destruction must not race another call.
   No API activates a window. Submit BEFORE activating the host's translation popup.
   One process-wide request may run at a time. A timed-out provider can keep it busy.
   Handles can be used on different threads; cancellation and polling are thread-safe.
   Request/result handles remain valid after service destruction. */
uint8_t clip_selected_text_options_init(ClipSelectedTextOptions* output);
/* Creation/submission return 0 on success, an ERROR_* kind otherwise. Error is optional.
   Output handles are cleared on failure. Input options/arrays are copied by start. */
uint32_t clip_selected_text_service_create(ClipSelectedTextService** output,
                                           ClipSelectedTextError* error);
void clip_selected_text_service_destroy(ClipSelectedTextService* service);
uint32_t clip_selected_text_start(const ClipSelectedTextService* service,
                                  const ClipSelectedTextOptions* options,
                                  ClipSelectedTextRequest** output, ClipSelectedTextError* error);
uint32_t clip_selected_text_request_poll(const ClipSelectedTextRequest* request);
/* Returns a status. Each terminal call creates an independent result; destroy each one.
   Pending or invalid arguments produce a null result. Prefer poll until terminal. */
uint32_t clip_selected_text_request_result(const ClipSelectedTextRequest* request,
                                           ClipSelectedTextResult** output);
void clip_selected_text_request_cancel(const ClipSelectedTextRequest* request);
void clip_selected_text_request_destroy(ClipSelectedTextRequest* request);
void clip_selected_text_result_destroy(ClipSelectedTextResult* result);
uint32_t clip_selected_text_result_status(const ClipSelectedTextResult* result);
/* Accessors return 1 on success, 0 for null/wrong outcome/out-of-range arguments.
   Outputs are cleared on failure. Text/executable views last until result destruction. */
uint8_t clip_selected_text_result_error(const ClipSelectedTextResult* result,
                                        ClipSelectedTextError* output);
uint8_t clip_selected_text_result_text(const ClipSelectedTextResult* result,
                                       ClipSelectedTextBytes* output);
uint8_t clip_selected_text_result_metadata(const ClipSelectedTextResult* result,
                                           ClipSelectedTextMetadata* output);
uint8_t clip_selected_text_result_range(const ClipSelectedTextResult* result, size_t index,
                                        ClipSelectedTextBytes* output, size_t* rectangle_count);
uint8_t clip_selected_text_result_rect(const ClipSelectedTextResult* result, size_t range,
                                       size_t index, ClipSelectedTextRect* output);

#ifdef __cplusplus
}
#endif
