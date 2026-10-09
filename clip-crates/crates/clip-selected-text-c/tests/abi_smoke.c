#include "clip_selected_text.h"
#include <assert.h>
#include <stddef.h>

#if defined(__cplusplus)
static_assert(sizeof(uintptr_t) == sizeof(void*), "handle width");
static_assert(offsetof(ClipSelectedTextOptions, abi_version) == 4, "ABI prefix");
#else
_Static_assert(sizeof(uintptr_t) == sizeof(void*), "handle width");
_Static_assert(offsetof(ClipSelectedTextOptions, abi_version) == 4, "ABI prefix");
#endif

int main(void) {
    ClipSelectedTextOptions options;
    ClipSelectedTextError error = {0};
    ClipSelectedTextService* service = NULL;
    ClipSelectedTextRequest* request = NULL;
    ClipSelectedTextResult* result = NULL;
    assert(clip_selected_text_options_init(&options) == 1);
    assert(options.struct_size == sizeof(options));
    assert(options.abi_version == 1 && options.timeout_ms == 2000 && options.copy_fallback == 1);
    assert(options.strategy == CLIP_SELECTED_TEXT_STRATEGY_AUTO);
    assert(CLIP_SELECTED_TEXT_ACCESSIBILITY == 1 && CLIP_SELECTED_TEXT_NATIVE_CONTROL == 2 &&
           CLIP_SELECTED_TEXT_CLIPBOARD == 3);
    assert(CLIP_SELECTED_TEXT_STRATEGY_ACCESSIBILITY == 1 &&
           CLIP_SELECTED_TEXT_STRATEGY_NATIVE_CONTROL == 2 &&
           CLIP_SELECTED_TEXT_STRATEGY_CLIPBOARD == 3);
    assert(options.excluded_native_windows == NULL && options.excluded_native_window_count == 0);
    {
        ClipSelectedTextMetadata metadata = {0};
        assert(metadata.native_window == 0 && metadata.native_focus == 0);
    }
    assert(clip_selected_text_service_create(&service, &error) == 0);
    /* Exercise real bundle symbols without reading another application or touching the clipboard.
     */
    options.copy_fallback = 42;
    assert(clip_selected_text_start(service, &options, &request, &error) ==
           CLIP_SELECTED_TEXT_ERROR_INVALID_CONFIGURATION);
    assert(request == NULL);
    assert(error.kind == CLIP_SELECTED_TEXT_ERROR_INVALID_CONFIGURATION);
    options.copy_fallback = 1;
    options.strategy = 42;
    assert(clip_selected_text_start(service, &options, &request, &error) ==
           CLIP_SELECTED_TEXT_ERROR_INVALID_CONFIGURATION);
    assert(request == NULL);
    assert(error.kind == CLIP_SELECTED_TEXT_ERROR_INVALID_CONFIGURATION);
    assert(clip_selected_text_request_poll(NULL) == CLIP_SELECTED_TEXT_INVALID_ARGUMENT);
    assert(clip_selected_text_request_result(NULL, &result) == CLIP_SELECTED_TEXT_INVALID_ARGUMENT);
    assert(result == NULL);
    assert(clip_selected_text_result_status(NULL) == CLIP_SELECTED_TEXT_INVALID_ARGUMENT);
    clip_selected_text_request_cancel(NULL);
    clip_selected_text_request_destroy(NULL);
    clip_selected_text_result_destroy(NULL);
    clip_selected_text_service_destroy(service);
    return 0;
}
