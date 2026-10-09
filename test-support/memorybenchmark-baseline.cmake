# Add current benchmark sources to an older checkout without modifying its
# tracked files. Include at the named child projects through CMake's
# CMAKE_PROJECT_<project-name>_INCLUDE cache options (see PERFORMANCE-BENCHMARKS.md).
if(NOT CLIP_APPS_BUILD_BENCHMARKS)
    message(FATAL_ERROR "Memory comparisons require a performance preset.")
endif()
if(NOT IS_DIRECTORY "${CLIP_MEMORY_BENCHMARK_SOURCE_ROOT}/clip_memory")
    message(FATAL_ERROR
        "Set CLIP_MEMORY_BENCHMARK_SOURCE_ROOT to the checkout containing the benchmark sources.")
endif()

include("${CLIP_MEMORY_BENCHMARK_SOURCE_ROOT}/cmake/ProjectWarnings.cmake")
if(PROJECT_NAME STREQUAL "clip_shot")
    # The project include runs before Clip declares its codec target and
    # source groups. Defer creation until those production declarations exist.
    function(clip_add_baseline_screenshot_memory_benchmark)
        add_executable(clip-shot-memory-performance-benchmark-baseline
            "${CLIP_MEMORY_BENCHMARK_SOURCE_ROOT}/clip_shot/tests/screenshot_memory_performance_benchmark.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/services/screenshotclipboardcontent.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp"
            ${CLIP_SHOT_RECOGNITION_IMAGE_RENDER_SOURCES})
        target_include_directories(clip-shot-memory-performance-benchmark-baseline PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_compile_definitions(clip-shot-memory-performance-benchmark-baseline PRIVATE
            CLIP_SHOT_BENCHMARK_MANAGED_READER=0)
        target_link_libraries(clip-shot-memory-performance-benchmark-baseline PRIVATE
            adqt::widgets clip_shot_image_codec clip_shot_diagnostics
            clip_shot_clipboard_placement Qt6::Widgets)
        if(WIN32)
            target_link_libraries(clip-shot-memory-performance-benchmark-baseline PRIVATE
                user32 psapi)
        endif()
        clip_shot_import_offscreen_platform(clip-shot-memory-performance-benchmark-baseline)
        clip_apply_strict_warnings(clip-shot-memory-performance-benchmark-baseline)
    endfunction()
    cmake_language(DEFER CALL clip_add_baseline_screenshot_memory_benchmark)
    return()
elseif(PROJECT_NAME STREQUAL "clip_image")
    add_executable(clip-image-memory-performance-benchmark-baseline
        "${CLIP_MEMORY_BENCHMARK_SOURCE_ROOT}/clip_image/tests/memory_performance_benchmark.cpp")
    target_link_libraries(clip-image-memory-performance-benchmark-baseline
        PRIVATE clip_image::clip_image)
    if(CLIP_IMAGE_CODEC_PNG AND NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
        target_link_libraries(clip-image-memory-performance-benchmark-baseline PRIVATE PNG::PNG)
    endif()
    set(_clip_memory_comparison_target clip-image-memory-performance-benchmark-baseline)
    set(_clip_memory_comparison_standard 20)
elseif(PROJECT_NAME STREQUAL "clip_draw_engine_qt")
    function(clip_use_current_render_memory_benchmark_sources)
        # Keep workload lifetimes, output checksums, and timing code identical
        # on both revisions under strict warnings.
        foreach(_kind filter watermark smart_erase)
            string(REPLACE "_" "-" _target_kind "${_kind}")
            set(_target "clip-canvas-${_target_kind}-benchmark")
            if(TARGET "${_target}")
                get_target_property(_sources "${_target}" SOURCES)
                list(REMOVE_ITEM _sources "tests/clip_canvas_${_kind}_benchmark.cpp")
                list(APPEND _sources
                    "${CLIP_MEMORY_BENCHMARK_SOURCE_ROOT}/clip_draw_engine_qt/tests/clip_canvas_${_kind}_benchmark.cpp")
                set_property(TARGET "${_target}" PROPERTY SOURCES "${_sources}")
            endif()
        endforeach()
        # The old standalone geometry source has an unused constexpr removed
        # by the branch. Suppress only that diagnostic without changing code.
        if(APPLE AND TARGET clip-canvas-smart-erase-benchmark)
            target_compile_options(clip-canvas-smart-erase-benchmark PRIVATE
                -Wno-unused-const-variable)
        endif()
    endfunction()
    cmake_language(DEFER CALL clip_use_current_render_memory_benchmark_sources)
    add_executable(clip-canvas-image-memory-benchmark-baseline
        "${CLIP_MEMORY_BENCHMARK_SOURCE_ROOT}/clip_draw_engine_qt/tests/clip_canvas_image_memory_benchmark.cpp")
    target_compile_definitions(clip-canvas-image-memory-benchmark-baseline
        PRIVATE CLIP_MEMORY_BENCHMARK_BASELINE=1)
    target_link_libraries(clip-canvas-image-memory-benchmark-baseline PRIVATE Qt6::Gui)
    set(_clip_memory_comparison_target clip-canvas-image-memory-benchmark-baseline)
    set(_clip_memory_comparison_standard 17)
else()
    message(FATAL_ERROR
        "Include this hook only for clip_image, clip_draw_engine_qt and clip_shot.")
endif()

set_target_properties(${_clip_memory_comparison_target} PROPERTIES
    CXX_STANDARD ${_clip_memory_comparison_standard}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
if(WIN32)
    target_link_libraries(${_clip_memory_comparison_target} PRIVATE psapi)
endif()
clip_apply_strict_warnings(${_clip_memory_comparison_target})
# Inherit the performance preset's flags and IPO policy. Adding another release
# policy here would give the comparison binary different LTO/section flags.
unset(_clip_memory_comparison_target)
unset(_clip_memory_comparison_standard)
