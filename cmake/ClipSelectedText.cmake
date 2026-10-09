if(NOT TARGET clip_selected_text_c)
    if(TARGET clip_shot_rust_ffi_bundle)
        # Fat LTO can internalize runtime functions in one archive while another
        # archive still needs the original std object. Compile Full's Rust code
        # together so Rust resolves its runtime once. Separate Cargo outputs
        # keep Mini's default archive free of selected-text dependencies.
        function(_clip_add_full_rust_bundle)
            set(CLIP_RUST_CARGO_TARGET_DIR "${CLIP_RUST_CARGO_TARGET_DIR}/full")
            clip_add_rust_static_library(clip_shot_full_rust_ffi_bundle
                PACKAGE clip-shot-rust-ffi
                MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}/../clip_rust_ffi"
                OUTPUT_NAME clip_shot_rust_ffi
                FEATURES selected-text ${_clip_scrolling_perf_features}
                STRIP_MSVC_DIRECTIVES)
        endfunction()
        _clip_add_full_rust_bundle()
        clip_add_rust_bundle_selector(clip_shot_rust_ffi_selector clip_shot_rust_ffi_bundle)
        add_library(clip_selected_text_c INTERFACE)
        target_link_libraries(clip_selected_text_c INTERFACE
            clip_shot_full_rust_ffi_bundle)
        # Full hosts select this superset for every shared C ABI dependency;
        # Mini and canvas-only hosts retain the default archive.
        cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
            CALL clip_select_rust_bundle_consumers
                clip_selected_text_c clip_shot_full_rust_ffi_bundle)
    else()
        clip_add_rust_static_library(clip_selected_text_c
            PACKAGE clip-selected-text-c
            MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
            OUTPUT_NAME clip_selected_text_c
            STRIP_MSVC_DIRECTIVES)
    endif()
    target_include_directories(clip_selected_text_c INTERFACE
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-selected-text-c/include")
endif()
