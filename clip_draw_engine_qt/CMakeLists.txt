cmake_minimum_required(VERSION 3.22)

project(clip_draw_engine_qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()
option(CLIP_DRAW_ENGINE_QT_BUILD_DEMO "Build the Clip Draw Engine Qt demo executable." ON)
option(CLIP_DRAW_ENGINE_QT_BUILD_TESTS "Build the Clip Draw Engine Qt test executables." ON)
option(CLIP_DRAW_ENGINE_QT_BUILD_BENCHMARKS "Build opt-in Clip Draw Engine Qt benchmarks." OFF)
option(CLIP_DRAW_ENGINE_QT_STRICT_COMPILE "Treat Clip Draw Engine Qt warnings as errors." ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(RustStaticLibrary)
clip_configure_windows_architecture()
include(ProjectWarnings)
include(ClipPixelBuffer)
include(ClipQtOffscreenTest)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ClipCanvasQtSources.cmake")
if(CLIP_DRAW_ENGINE_QT_STRICT_COMPILE)
    clip_enable_strict_warnings()
endif()
clip_enable_clang_tidy()

set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)
include(ClipQt)
clip_find_qt(COMPONENTS Widgets Svg)
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4" CACHE PATH "OpenCV package directory" FORCE)
endif()
find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgproc)
include(ClipTestCliGuard)
include(ClipQtOffscreenTest)
qt_standard_project_setup()

if(NOT TARGET adqt::icons)
    set(CLIP_DRAW_ENGINE_ADQT_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../ant_design_qt"
        CACHE PATH "Path to the ant_design_qt repository")
    if(NOT EXISTS "${CLIP_DRAW_ENGINE_ADQT_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "Unable to locate ant_design_qt relative to this project. Set CLIP_DRAW_ENGINE_ADQT_DIR for an external checkout.")
    endif()
    add_subdirectory("${CLIP_DRAW_ENGINE_ADQT_DIR}"
                     "${CMAKE_CURRENT_BINARY_DIR}/ant_design_qt")
endif()

if(NOT TARGET clip_draw_engine_c)
    clip_add_rust_static_library(clip_draw_engine_c
        PACKAGE clip-draw-engine-c
        MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_NAME clip_draw_engine_c
        STRIP_MSVC_DIRECTIVES
    )
endif()

add_library(clip_canvas_image STATIC
    src/rendering/clip_canvas_image.cpp
    include/clip_draw_engine_qt/clip_canvas_image.h)
target_include_directories(clip_canvas_image PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_canvas_image PUBLIC Qt6::Gui $<BUILD_INTERFACE:clip_pixel_buffer>)

qt_add_library(clip_draw_engine_qt_canvas STATIC
    ${CLIP_CANVAS_QT_SOURCES}
    src/icons/draw_engine_icons.cpp
    src/icons/draw_engine_icons.h
)

include(ClipSimd)
clip_configure_avx2_sources(
    src/rendering/clip_canvas_filter_avx2.cpp
    src/rendering/clip_canvas_pen_mask_avx2.cpp)

target_include_directories(clip_draw_engine_qt_canvas
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
)

target_link_libraries(clip_draw_engine_qt_canvas
    PUBLIC
        Qt6::Widgets
        $<BUILD_INTERFACE:clip_pixel_buffer>
    PRIVATE
        clip_canvas_image
        Qt6::Svg
        opencv_core
        opencv_imgproc
        adqt::icons
        clip_draw_engine_c
)
if(WIN32)
    target_link_libraries(clip_draw_engine_qt_canvas PRIVATE
        advapi32
        bcrypt
        ntdll
        shell32
        userenv
        ws2_32
    )
endif()
target_precompile_headers(clip_draw_engine_qt_canvas PRIVATE
    <algorithm>
    <memory>
    <optional>
    <string>
    <vector>
    <QtCore/QObject>
    <QtCore/QString>
    <QtGui/QColor>
    <QtGui/QImage>
    <QtGui/QPainter>
    <QtWidgets/QWidget>
)
clip_enable_unity_build(clip_draw_engine_qt_canvas)

if(CLIP_DRAW_ENGINE_QT_BUILD_DEMO)
    add_subdirectory(qt-demo)
endif()

if(CLIP_DRAW_ENGINE_QT_BUILD_BENCHMARKS)
    add_executable(clip-canvas-image-memory-benchmark
        tests/clip_canvas_image_memory_benchmark.cpp)
    target_link_libraries(clip-canvas-image-memory-benchmark PRIVATE clip_canvas_image)
    if(WIN32)
        target_link_libraries(clip-canvas-image-memory-benchmark PRIVATE psapi)
    endif()
    add_executable(clip-canvas-eraser-benchmark tests/clip_canvas_eraser_benchmark.cpp)
    target_include_directories(clip-canvas-eraser-benchmark PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-eraser-benchmark PRIVATE clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-canvas-eraser-benchmark)
    add_executable(clip-canvas-reference-scene-benchmark
        tests/clip_canvas_reference_scene_benchmark.cpp
    )
    target_include_directories(clip-canvas-reference-scene-benchmark PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-reference-scene-benchmark PRIVATE clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-canvas-reference-scene-benchmark)

    add_executable(clip-canvas-stroke-filter-benchmark tests/clip_canvas_stroke_filter_benchmark.cpp)
    target_link_libraries(clip-canvas-stroke-filter-benchmark PRIVATE clip_draw_engine_qt_canvas)

    add_executable(clip-canvas-arrow-text-benchmark tests/clip_canvas_arrow_text_benchmark.cpp)
    target_link_libraries(clip-canvas-arrow-text-benchmark PRIVATE clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-canvas-arrow-text-benchmark)

    add_executable(clip-canvas-distance-benchmark tests/clip_canvas_distance_benchmark.cpp)
    target_link_libraries(clip-canvas-distance-benchmark PRIVATE clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-canvas-distance-benchmark)

    clip_add_rust_executable(clip-scene-order-benchmark
        PACKAGE clip-draw-engine-scene
        MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_NAME clip-scene-order-benchmark
    )
    add_executable(clip-canvas-smart-erase-benchmark
        tests/clip_canvas_smart_erase_benchmark.cpp
        src/rendering/clip_canvas_smart_erase_algorithm.cpp
        src/rendering/clip_canvas_display_item.cpp
        src/rendering/clip_canvas_render_geometry.cpp)
    target_include_directories(clip-canvas-smart-erase-benchmark PRIVATE
        include
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-smart-erase-benchmark PRIVATE
        clip_draw_engine_c clip_canvas_image $<BUILD_INTERFACE:clip_pixel_buffer>
        Qt6::Widgets opencv_core opencv_imgproc
    )
    add_executable(clip-canvas-rectangle-benchmark
        tests/clip_canvas_rectangle_benchmark.cpp
    )
    target_include_directories(clip-canvas-rectangle-benchmark PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-rectangle-benchmark PRIVATE
        clip_draw_engine_qt_canvas
    )

    add_executable(clip-canvas-filter-benchmark
        tests/clip_canvas_filter_benchmark.cpp
    )
    target_include_directories(clip-canvas-filter-benchmark PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-filter-benchmark PRIVATE
        clip_draw_engine_qt_canvas
    )
    clip_shot_import_offscreen_platform(clip-canvas-filter-benchmark)

    add_executable(clip-canvas-pen-filter-pipeline-benchmark
        tests/clip_canvas_pen_filter_pipeline_benchmark.cpp
    )
    target_include_directories(clip-canvas-pen-filter-pipeline-benchmark PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-pen-filter-pipeline-benchmark PRIVATE
        clip_draw_engine_qt_canvas
    )

    add_executable(clip-canvas-watermark-benchmark
        tests/clip_canvas_watermark_benchmark.cpp
    )
    target_include_directories(clip-canvas-watermark-benchmark PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-watermark-benchmark PRIVATE
        clip_draw_engine_qt_canvas
    )

    add_executable(clip-canvas-spotlight-benchmark
        tests/clip_canvas_spotlight_benchmark.cpp
    )
    target_include_directories(clip-canvas-spotlight-benchmark PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-spotlight-benchmark PRIVATE
        clip_draw_engine_qt_canvas
    )
endif()

if(CLIP_DRAW_ENGINE_QT_BUILD_TESTS)
    enable_testing()
    add_executable(clip-canvas-image-tests tests/clip_canvas_image_tests.cpp)
    target_link_libraries(clip-canvas-image-tests PRIVATE clip_canvas_image clip_pixel_buffer)
    add_test(NAME clip-canvas-image-tests COMMAND clip-canvas-image-tests)
    set_tests_properties(clip-canvas-image-tests PROPERTIES LABELS "unit" TIMEOUT 30)
    # Test binaries must report failures as stderr output plus a non-zero
    # exit code instead of blocking on modal dialogs.
    clip_link_test_cli_guard(qt)

    add_executable(clip-canvas-fill-render-tests tests/clip_canvas_fill_render_tests.cpp)
    target_include_directories(clip-canvas-fill-render-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-fill-render-tests PRIVATE clip_draw_engine_qt_canvas)
    qt_import_plugins(clip-canvas-fill-render-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-fill-render-tests COMMAND clip-canvas-fill-render-tests)
    set_tests_properties(clip-canvas-fill-render-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )

    add_executable(clip-canvas-quick-selection-tests tests/clip_canvas_quick_selection_tests.cpp)
    target_link_libraries(clip-canvas-quick-selection-tests PRIVATE clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-canvas-quick-selection-tests)
    add_test(NAME clip-canvas-quick-selection-tests COMMAND clip-canvas-quick-selection-tests -platform offscreen)
    set_tests_properties(clip-canvas-quick-selection-tests PROPERTIES LABELS "unit" TIMEOUT 60)

    add_executable(clip-canvas-text-draft-tests
        tests/clip_canvas_text_draft_tests.cpp
    )
    target_include_directories(clip-canvas-text-draft-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/qt-demo"
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-text-draft-tests PRIVATE
        clip_draw_engine_qt_canvas
    )
    qt_import_plugins(clip-canvas-text-draft-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-text-draft-tests COMMAND clip-canvas-text-draft-tests)
    add_test(NAME clip-canvas-serial-number-render-tests
        COMMAND clip-canvas-text-draft-tests --serial-number-only -platform offscreen)
    set_tests_properties(clip-canvas-serial-number-render-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-canvas-text-render-cache-tests
        COMMAND clip-canvas-text-draft-tests --text-render-cache-only -platform offscreen)
    set_tests_properties(clip-canvas-text-render-cache-tests PROPERTIES LABELS "unit" TIMEOUT 60)

    set_tests_properties(clip-canvas-text-draft-tests clip-canvas-text-render-cache-tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-canvas-text-overlay-tests tests/clip_canvas_text_overlay_tests.cpp)
    target_include_directories(clip-canvas-text-overlay-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-text-overlay-tests PRIVATE clip_draw_engine_qt_canvas)
    qt_import_plugins(clip-canvas-text-overlay-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-text-overlay-tests COMMAND clip-canvas-text-overlay-tests)
    set_tests_properties(clip-canvas-text-overlay-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )

    add_executable(clip-canvas-arrow-text-tests tests/clip_canvas_arrow_text_tests.cpp)
    target_include_directories(clip-canvas-arrow-text-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-arrow-text-tests PRIVATE clip_draw_engine_qt_canvas)
    qt_import_plugins(clip-canvas-arrow-text-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-arrow-text-tests COMMAND clip-canvas-arrow-text-tests)
    set_tests_properties(clip-canvas-arrow-text-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )

    add_executable(clip-canvas-distance-tests tests/clip_canvas_distance_tests.cpp)
    target_include_directories(clip-canvas-distance-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-distance-tests PRIVATE clip_draw_engine_qt_canvas)
    qt_import_plugins(clip-canvas-distance-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-distance-tests COMMAND clip-canvas-distance-tests)
    set_tests_properties(clip-canvas-distance-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )

    add_executable(clip-canvas-serial-number-drag-tests tests/clip_canvas_serial_number_drag_tests.cpp)
    target_include_directories(clip-canvas-serial-number-drag-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-serial-number-drag-tests PRIVATE clip_draw_engine_qt_canvas)
    qt_import_plugins(clip-canvas-serial-number-drag-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-serial-number-drag-tests COMMAND clip-canvas-serial-number-drag-tests)
    set_tests_properties(clip-canvas-serial-number-drag-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )

    add_executable(clip-canvas-duplicate-drag-tests tests/clip_canvas_duplicate_drag_tests.cpp)
    target_include_directories(clip-canvas-duplicate-drag-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-duplicate-drag-tests PRIVATE clip_draw_engine_qt_canvas)
    qt_import_plugins(clip-canvas-duplicate-drag-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-duplicate-drag-tests COMMAND clip-canvas-duplicate-drag-tests)
    set_tests_properties(clip-canvas-duplicate-drag-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )

    add_executable(clip-canvas-align-tests tests/clip_canvas_align_tests.cpp)
    target_include_directories(clip-canvas-align-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-align-tests PRIVATE clip_draw_engine_qt_canvas)
    qt_import_plugins(clip-canvas-align-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-canvas-align-tests COMMAND clip-canvas-align-tests)
    set_tests_properties(clip-canvas-align-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )

    add_executable(clip-canvas-custom-renderer-tests
        tests/clip_canvas_custom_renderer_tests.cpp
    )
    target_include_directories(clip-canvas-custom-renderer-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-custom-renderer-tests PRIVATE
        clip_draw_engine_qt_canvas
        adqt::icons
        Qt6::Svg
    )
    add_test(
        NAME clip-canvas-custom-renderer-tests
        COMMAND clip-canvas-custom-renderer-tests
    )
    add_test(
        NAME clip-canvas-document-reset-tests
        COMMAND clip-canvas-custom-renderer-tests --document-reset-only
    )
    add_test(
        NAME clip-canvas-render-state-tests
        COMMAND clip-canvas-custom-renderer-tests --render-state-only
    )
    add_test(
        NAME clip-canvas-filter-creation-tests
        COMMAND clip-canvas-custom-renderer-tests --filter-creation-only
    )
    add_test(NAME clip-canvas-eraser-widget-tests
        COMMAND clip-canvas-custom-renderer-tests --eraser-only)
    clip_shot_import_offscreen_platform(clip-canvas-custom-renderer-tests)
    clip_shot_offscreen_qpa_environment(_clip_canvas_reset_qpa_environment)
    set_tests_properties(clip-canvas-document-reset-tests clip-canvas-render-state-tests
        clip-canvas-filter-creation-tests clip-canvas-eraser-widget-tests PROPERTIES
        ENVIRONMENT "${_clip_canvas_reset_qpa_environment}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        set_tests_properties(clip-canvas-custom-renderer-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-canvas-custom-renderer-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-canvas-state-tests
        tests/clip_canvas_state_tests.cpp
    )
    target_include_directories(clip-canvas-state-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-state-tests PRIVATE
        clip_draw_engine_qt_canvas
    )
    add_test(NAME clip-canvas-state-tests COMMAND clip-canvas-state-tests)

    add_executable(clip-canvas-filter-safety-tests
        tests/clip_canvas_filter_safety_tests.cpp
        src/rendering/clip_canvas_filter_avx2.cpp
        src/rendering/clip_canvas_render_diagnostics.cpp
    )
    target_link_libraries(clip-canvas-filter-safety-tests PRIVATE
        clip_canvas_image clip_pixel_buffer Qt6::Gui)
    add_test(NAME clip-canvas-filter-barrier-safety-tests
        COMMAND clip-canvas-filter-safety-tests barrier)
    add_test(NAME clip-canvas-filter-cache-safety-tests
        COMMAND clip-canvas-filter-safety-tests cache)
    add_test(NAME clip-canvas-filter-caller-failure-safety-tests
        COMMAND clip-canvas-filter-safety-tests caller-failure)
    add_test(NAME clip-canvas-filter-worker-failure-safety-tests
        COMMAND clip-canvas-filter-safety-tests worker-failure)
    set_tests_properties(
        clip-canvas-filter-barrier-safety-tests clip-canvas-filter-cache-safety-tests
        clip-canvas-filter-caller-failure-safety-tests
        clip-canvas-filter-worker-failure-safety-tests
        PROPERTIES TIMEOUT 30 LABELS unit
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-canvas-smart-erase-tests tests/clip_canvas_smart_erase_tests.cpp)
    target_include_directories(clip-canvas-smart-erase-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-smart-erase-tests PRIVATE
        clip_draw_engine_qt_canvas clip_canvas_image opencv_core opencv_imgproc)
    clip_shot_import_offscreen_platform(clip-canvas-smart-erase-tests)
    add_test(NAME clip-canvas-smart-erase-tests COMMAND clip-canvas-smart-erase-tests)
    set_tests_properties(clip-canvas-smart-erase-tests PROPERTIES TIMEOUT 90 LABELS unit
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:opencv_core>")

    add_executable(clip-canvas-smart-erase-quality-tests
        tests/clip_canvas_smart_erase_quality_tests.cpp
        src/rendering/clip_canvas_smart_erase_algorithm.cpp
        src/rendering/clip_canvas_display_item.cpp
        src/rendering/clip_canvas_render_geometry.cpp)
    target_include_directories(clip-canvas-smart-erase-quality-tests PRIVATE
        include ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include")
    target_link_libraries(clip-canvas-smart-erase-quality-tests PRIVATE
        # clip_canvas_render_geometry resolves text paint bounds through the
        # engine's C ABI, so the Rust static library must be linked in.
        clip_draw_engine_c clip_canvas_image $<BUILD_INTERFACE:clip_pixel_buffer>
        Qt6::Widgets opencv_core opencv_imgproc)
    add_test(NAME clip-canvas-smart-erase-quality-tests COMMAND clip-canvas-smart-erase-quality-tests)
    set_tests_properties(clip-canvas-smart-erase-quality-tests PROPERTIES TIMEOUT 180 LABELS unit
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:opencv_core>")

    add_executable(clip-canvas-reference-scene-tests tests/clip_canvas_reference_scene_tests.cpp)
    target_include_directories(clip-canvas-reference-scene-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-reference-scene-tests PRIVATE clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-canvas-reference-scene-tests)
    add_test(NAME clip-canvas-reference-scene-tests COMMAND clip-canvas-reference-scene-tests)
    set_tests_properties(clip-canvas-reference-scene-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-canvas-eraser-render-tests tests/clip_canvas_eraser_render_tests.cpp)
    target_include_directories(clip-canvas-eraser-render-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-eraser-render-tests PRIVATE clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-canvas-eraser-render-tests)
    add_test(NAME clip-canvas-eraser-render-tests COMMAND clip-canvas-eraser-render-tests)
    clip_shot_offscreen_qpa_environment(_clip_canvas_eraser_qpa_environment)
    set_tests_properties(clip-canvas-eraser-render-tests PROPERTIES
        ENVIRONMENT "${_clip_canvas_eraser_qpa_environment}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-canvas-filter-render-tests
        tests/clip_canvas_filter_render_tests.cpp
    )
    target_include_directories(clip-canvas-filter-render-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-filter-render-tests PRIVATE
        clip_pixel_buffer clip_canvas_image
        clip_draw_engine_qt_canvas
    )
    clip_shot_import_offscreen_platform(clip-canvas-filter-render-tests)
    add_test(NAME clip-canvas-filter-render-tests COMMAND clip-canvas-filter-render-tests)
    add_test(NAME clip-canvas-retained-filter-paint-tests
        COMMAND clip-canvas-filter-render-tests --retained-filter-paint-only)
    add_test(NAME clip-canvas-sparse-filter-damage-tests
        COMMAND clip-canvas-filter-render-tests --sparse-filter-damage-only)
    set_tests_properties(
        clip-canvas-filter-render-tests clip-canvas-retained-filter-paint-tests
        clip-canvas-sparse-filter-damage-tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-canvas-sparse-filter-damage-tests PROPERTIES LABELS "unit")

    add_executable(clip-canvas-spotlight-render-tests
        tests/clip_canvas_spotlight_render_tests.cpp
    )
    target_include_directories(clip-canvas-spotlight-render-tests PRIVATE
        ${CLIP_CANVAS_QT_PRIVATE_SOURCE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-canvas-spotlight-render-tests PRIVATE
        clip_draw_engine_qt_canvas
    )
    clip_shot_import_offscreen_platform(clip-canvas-spotlight-render-tests)
    add_test(
        NAME clip-canvas-spotlight-render-tests
        COMMAND clip-canvas-spotlight-render-tests
    )
    set_tests_properties(clip-canvas-spotlight-render-tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
endif()
