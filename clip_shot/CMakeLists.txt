if(APPLE)
    set_source_files_properties(src/presentation/core/screenshotgeometry.cpp
        src/presentation/selector/screenshotselectorserviceclient.cpp
        src/presentation/recording/screenrecordingareawindow.cpp
        src/presentation/globalcanvas/globalcanvasplatform.cpp
        tests/global_canvas_tests.cpp
        tests/screen_recording_shortcut_tests.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

cmake_minimum_required(VERSION 3.30)

project(clip_shot VERSION ${CLIP_SHOT_VERSION_NUMERIC} LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(RustStaticLibrary)
clip_configure_windows_architecture()
clip_detect_rust_target(CLIP_SHOT_RUST_TARGET)

if(WIN32)
    if(CLIP_WINDOWS_ARCHITECTURE STREQUAL "arm64")
        set(_clip_ocr_manifest_default
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/clip-shot-ocr-asset-manifest-arm64.json")
    else()
        set(_clip_ocr_manifest_default
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/clip-shot-ocr-asset-manifest.json")
    endif()
    set(CLIP_SHOT_OCR_ASSET_MANIFEST "${_clip_ocr_manifest_default}" CACHE FILEPATH
        "Verified OCR asset manifest for the selected Windows architecture.")
    set(_clip_ocr_artifacts_directory "${CMAKE_CURRENT_SOURCE_DIR}/../artifacts")
    if(CLIP_WINDOWS_ARCHITECTURE STREQUAL "arm64")
        string(APPEND _clip_ocr_artifacts_directory "/windows-arm64")
    endif()
    if(EXISTS "${CLIP_SHOT_OCR_ASSET_MANIFEST}")
        file(READ "${CLIP_SHOT_OCR_ASSET_MANIFEST}" _clip_ocr_manifest)
        string(JSON _clip_ocr_manifest_platform GET "${_clip_ocr_manifest}" runtime platform)
        if(NOT _clip_ocr_manifest_platform STREQUAL CLIP_WINDOWS_PLATFORM)
            message(FATAL_ERROR
                "OCR asset manifest platform ${_clip_ocr_manifest_platform} disagrees with ${CLIP_WINDOWS_PLATFORM}")
        endif()
    else()
        message(WARNING
            "Verified ${CLIP_WINDOWS_PLATFORM} OCR assets are not prepared. Build the release OCR runtime, "
            "generate its audited asset manifest, then configure CLIP_SHOT_OCR_ASSET_MANIFEST to stage managed OCR assets.")
    endif()
endif()

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ClipShot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_EXTENSIONS OFF)

if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

if(APPLE)
    set(CLIP_SELECTED_FILES_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos/selectedfiles.mm)
    set(CLIP_SELECTED_FILES_LIBRARIES "-framework AppKit" "-framework ApplicationServices")
else()
    set(CLIP_SELECTED_FILES_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/selectedfiles.cpp)
endif()

set(CLIP_PINNED_PLATFORM_SOURCES)
if(NOT APPLE)
    list(APPEND CLIP_PINNED_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/pinned/pinnedwindowplatform.cpp)
endif()
if(WIN32)
    list(APPEND CLIP_PINNED_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/pinnedwindowevents.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/pinnedwindownative.cpp)
endif()

option(CLIP_SHOT_ENABLE_STRICT_COMPILE "Enable strict compiler warnings and treat warnings as errors." ON)
option(CLIP_SHOT_BUILD_TESTS "Build Clip test executables." OFF)
option(CLIP_SHOT_BUILD_RUNTIME_TESTS
    "Build focused OCR and crash diagnostics tests without unrelated test targets." OFF)
option(CLIP_SHOT_BUILD_IMAGE_CODEC_SMOKE_TEST
    "Build the Clip image-codec smoke test without the full QtTest suite." OFF)
option(CLIP_SHOT_BUILD_BENCHMARKS "Build opt-in native Clip benchmarks." OFF)
option(CLIP_SHOT_SCROLLING_PERF_DETAIL "Compile detailed scrolling C++ and Rust timings." OFF)
option(CLIP_SHOT_RELEASE_STATIC "Prefer static Clip release linkage." OFF)
option(CLIP_SHOT_QT_STATIC "Link Clip against a static Qt build." OFF)
set(CLIP_SHOT_WEBSITE_URL "https://clipshot.top" CACHE STRING "Public Clip website URL.")
set(CLIP_SHOT_PROJECT_URL "https://github.com/mg-chao/clip-apps" CACHE STRING
    "Public Clip source repository URL.")
set(CLIP_SHOT_QQ_GROUP_2_URL "https://qm.qq.com/q/hRIRFED9Ze" CACHE STRING
    "QQ group 2 join URL.")
set(CLIP_SHOT_QQ_GROUP_3_URL "https://qm.qq.com/q/Ts3cwNdykW" CACHE STRING
    "QQ group 3 join URL.")
unset(CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE CACHE)
set(
    CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/audios/camera_shutter.mp3"
)
option(
    CLIP_SHOT_ENABLE_AGGRESSIVE_RELEASE_OPTIMIZATION
    "Enable aggressive compiler and linker optimizations for Release."
    ON
)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ProjectWarnings.cmake")
if(CLIP_SHOT_ENABLE_STRICT_COMPILE)
    clip_enable_strict_warnings()
endif()
clip_enable_clang_tidy()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipQt.cmake")
clip_find_qt(COMPONENTS Widgets LinguistTools Svg Network)
if(APPLE)
    add_library(clip_shot_macos_clipboard STATIC src/platform/macos/imageclipboard.mm)
    target_link_libraries(clip_shot_macos_clipboard PUBLIC Qt6::Gui
        PRIVATE "-framework AppKit" "-framework CoreGraphics" "-framework ImageIO")
    add_library(clip_shot_macos_cursor STATIC
        src/platform/macos/windowcursorcoordinator.cpp
        src/platform/macos/windowcursorcoordinator.mm)
    target_link_libraries(clip_shot_macos_cursor PUBLIC Qt6::Widgets PRIVATE "-framework AppKit")

    # Window adapters own their shared cursor dependency. Link this target instead
    # of compiling adapter sources in each executable.
    add_library(clip_shot_macos_window_platform STATIC
        src/platform/macos/screenshotnative.mm
        src/platform/macos/pinnedwindowplatform.mm
        src/presentation/pinned/pinnedwindowplatform.cpp)
    target_include_directories(clip_shot_macos_window_platform PRIVATE include)
    target_link_libraries(clip_shot_macos_window_platform PUBLIC Qt6::Widgets
        PRIVATE Qt6::GuiPrivate clip_shot_macos_cursor "-framework AppKit" "-framework ApplicationServices")
endif()


function(clip_shot_link_macos_window_platform target)
    if(APPLE)
        target_link_libraries(${target} PRIVATE clip_shot_macos_window_platform)
    endif()
endfunction()
include(ClipTestCliGuard)
include(ClipQtOffscreenTest)

set(_clip_shot_static_qt_plugins
    # The full-color application icon embeds SVG images. QtSvg renders the
    # outer document, but QImageReader needs this plugin for those images.
    Qt6::QSvgPlugin
    # The --update-probe startup path switches QT_QPA_PLATFORM to "offscreen"
    # before constructing QApplication. Static Qt registers platform plugins
    # at link time, so the release binary must import it here or the packaged
    # startup probe blocks forever on the missing-plugin dialog.
    Qt6::QOffscreenIntegrationPlugin
)
if(WIN32)
    list(APPEND _clip_shot_static_qt_plugins
        Qt6::QWindowsIntegrationPlugin
        Qt6::QSchannelBackendPlugin)
elseif(APPLE)
    list(APPEND _clip_shot_static_qt_plugins
        Qt6::QCocoaIntegrationPlugin
        Qt6::QMacStylePlugin
        Qt6::QSecureTransportBackendPlugin)
endif()

if(CLIP_SHOT_QT_STATIC)
    if(NOT QT_FEATURE_static)
        message(FATAL_ERROR
            "CLIP_SHOT_QT_STATIC requires a static Qt ${CLIP_QT_VERSION} kit. "
            "Set CLIP_QT_STATIC_DIR or Qt6_DIR to its lib/cmake/Qt6 directory.")
    endif()
    foreach(_clip_shot_static_qt_plugin IN LISTS _clip_shot_static_qt_plugins)
        if(NOT TARGET ${_clip_shot_static_qt_plugin})
            message(FATAL_ERROR
                "The static Qt kit does not provide ${_clip_shot_static_qt_plugin}.")
        endif()
    endforeach()
    unset(_clip_shot_static_qt_plugin)

    if(CLIP_SHOT_RELEASE_STATIC)
        get_target_property(_clip_shot_qt_core_private_features
            Qt6::Core QT_ENABLED_PRIVATE_FEATURES)
        get_target_property(_clip_shot_qt_gui_private_features
            Qt6::Gui QT_ENABLED_PRIVATE_FEATURES)
        if(NOT "system_zlib" IN_LIST _clip_shot_qt_core_private_features OR
           NOT "system_png" IN_LIST _clip_shot_qt_gui_private_features)
            message(FATAL_ERROR
                "The production static build requires Qt to use the same system "
                "zlib and libpng packages as Clip Image. Rebuild Qt with "
                "the platform build-static-qt script; bundled Qt codecs cannot be linked "
                "safely into the unified executable.")
        endif()
        unset(_clip_shot_qt_core_private_features)
        unset(_clip_shot_qt_gui_private_features)
    endif()
endif()

if(NOT DEFINED CLIP_SHOT_API_BASE_URL OR CLIP_SHOT_API_BASE_URL STREQUAL "")
    if(CMAKE_CONFIGURATION_TYPES)
        set(CLIP_SHOT_API_BASE_URL "https://clipshot.top" CACHE STRING
            "Clip API base URL." FORCE)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CLIP_SHOT_API_BASE_URL "http://localhost:8080" CACHE STRING
            "Clip API base URL." FORCE)
    else()
        set(CLIP_SHOT_API_BASE_URL "https://clipshot.top" CACHE STRING
            "Clip API base URL." FORCE)
    endif()
endif()
clip_find_qt_private(Gui)
set(CLIP_SHOT_DRAW_ENGINE_QT_DIR
    "${CLIP_DRAW_ENGINE_SOURCE_DIR}"
    CACHE PATH
    "Path to the clip_draw_engine_qt project."
)
set(CLIP_SHOT_CAPTURE_CRATES_DIR
    "${CLIP_CAPTURE_CRATES_DIR}"
    CACHE PATH
    "Path to the clip-crates workspace."
)

set(CLIP_DRAW_ENGINE_QT_BUILD_DEMO OFF CACHE BOOL "Build Clip Draw Engine Qt demo." FORCE)
if(NOT CLIP_SHOT_BUILD_TESTS)
    # Keep the production configuration free of test targets from the nested
    # draw engine project, including values left in an existing build cache.
    set(
        CLIP_DRAW_ENGINE_QT_BUILD_TESTS
        OFF
        CACHE BOOL
        "Build Clip Draw Engine Qt test executables."
        FORCE
    )
    set(
        CLIP_DRAW_ENGINE_QT_BUILD_BENCHMARKS
        OFF
        CACHE BOOL
        "Build opt-in Clip Draw Engine Qt benchmarks."
        FORCE
    )
endif()
if(NOT TARGET clip_draw_engine_qt_canvas)
    add_subdirectory("${CLIP_SHOT_DRAW_ENGINE_QT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/clip_draw_engine_qt")
endif()

if(WIN32)
    clip_resolve_rust_libclang_directory(_clip_shot_libclang_dir)
    set(CLIP_SHOT_LIBCLANG_DIR
        "${_clip_shot_libclang_dir}"
        CACHE PATH
        "Directory containing host-compatible libclang.dll for ffmpeg-sys bindgen."
        FORCE
    )
endif()

if(NOT TARGET clip_capture_c)
    set(_clip_scrolling_perf_features)
    if(CLIP_SHOT_SCROLLING_PERF_DETAIL)
        list(APPEND _clip_scrolling_perf_features clip-stitch-images-c/perf-instrumentation)
    endif()
    clip_add_rust_static_libraries(clip_shot_rust_ffi
        MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        FEATURES ${_clip_scrolling_perf_features}
        TARGETS
            clip_capture_c
            clip_ui_selector_c
            clip_stitch_images_c
        PACKAGES
            clip-capture-c
            clip-ui-selector-c
            clip-stitch-images-c
        OUTPUT_NAMES
            clip_capture_c
            clip_ui_selector_c
            clip_stitch_images_c
        STRIP_MSVC_DIRECTIVES
    )
endif()

# Selector framework requirements travel with the Rust static library on both Apple targets.
if(APPLE)
    target_link_libraries(clip_ui_selector_c INTERFACE
        "-framework ApplicationServices" "-framework CoreFoundation" "-framework CoreGraphics")
endif()

if(NOT TARGET clip_recording_c)
    clip_add_rust_static_library(clip_recording_c PACKAGE clip-recording-c
        MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}" OUTPUT_NAME clip_recording_c)
endif()

if(NOT TARGET clip_recording_c)
    clip_add_rust_static_library(clip_recording_c PACKAGE clip-recording-c
        MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}" OUTPUT_NAME clip_recording_c)
endif()

if(NOT TARGET clip_visual_region_detector_c)
    clip_add_rust_static_library(clip_visual_region_detector_c PACKAGE clip-visual-region-detector-c MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}" OUTPUT_NAME clip_visual_region_detector_c)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipSelectedText.cmake")

# OpenCV's contrib wechat_qrcode module backs in-process QR recognition and
# objdetect's cv::barcode detector covers EAN/UPC 1-D symbologies; text
# recognition lives in the standalone OCR process. Pin the config package to
# the manifest-installed copy: the Rust OpenCV crates export a user-level
# OpenCV_DIR that would otherwise shadow it.
set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4" CACHE PATH
    "OpenCV package configuration directory" FORCE)
find_package(OpenCV CONFIG REQUIRED)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FetchClipShotQrModels.cmake")
clip_shot_fetch_qr_models(CLIP_SHOT_QR_MODELS_DIR)
# ONNX Runtime backs the standalone OCR process: static release builds link
# its closure into clip-ocr-process.exe, dynamic builds stage its DLL.
find_package(onnxruntime CONFIG REQUIRED)
find_package(minizip-ng CONFIG REQUIRED)

if(WIN32)
    set(CLIP_SHOT_DIRECTML_RUNTIME_DLL_RELEASE
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/DirectML.dll")
    if(CLIP_SHOT_RELEASE_STATIC)
        set(CLIP_SHOT_DIRECTML_RUNTIME_DLL_DEBUG
            "${CLIP_SHOT_DIRECTML_RUNTIME_DLL_RELEASE}")
        set(_clip_shot_directml_runtimes
            "${CLIP_SHOT_DIRECTML_RUNTIME_DLL_RELEASE}")
    else()
        set(CLIP_SHOT_DIRECTML_RUNTIME_DLL_DEBUG
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/DirectML.dll")
        set(_clip_shot_directml_runtimes
            "${CLIP_SHOT_DIRECTML_RUNTIME_DLL_DEBUG}"
            "${CLIP_SHOT_DIRECTML_RUNTIME_DLL_RELEASE}")
    endif()
    foreach(_clip_shot_directml_runtime IN LISTS _clip_shot_directml_runtimes)
        if(NOT EXISTS "${_clip_shot_directml_runtime}")
            message(FATAL_ERROR
                "The DirectML-enabled ONNX Runtime package is incomplete: "
                "${_clip_shot_directml_runtime} was not found.")
        endif()
    endforeach()
    unset(_clip_shot_directml_runtime)
    unset(_clip_shot_directml_runtimes)
endif()

# The standalone OCR process owns every OCR dependency. Static release builds
include(ClipDiagnostics)

# The standalone OCR process owns every OCR dependency. Static release builds
# link the vcpkg ONNX Runtime closure into the executable itself; the dynamic
# workspace builds keep loading the staged onnxruntime.dll at runtime.
set(CLIP_SHOT_OCR_STATIC_ONNXRUNTIME OFF)
if(CLIP_SHOT_RELEASE_STATIC OR CLIP_APPS_RELEASE_STATIC)
    set(CLIP_SHOT_OCR_STATIC_ONNXRUNTIME ON)
endif()

# Depth-first walk of the onnxruntime::onnxruntime imported-target closure.
# Depth-first expansion keeps every library behind its dependents, matching
# the order the previous in-process C++ link received from CMake. Interface
# libraries are header-only and contribute no linker input; configuration
# generator expressions (Debug-only diagnostics helpers) are skipped.
function(_clip_shot_onnxruntime_link_items out_items)
    set(_pending onnxruntime::onnxruntime)
    set(_visited)
    set(_items)
    while(_pending)
        list(POP_FRONT _pending _current)
        # A LINK_ONLY wrapper may carry either a library file or another
        # target name; strip it before resolving the item.
        string(REGEX REPLACE "^\\$<LINK_ONLY:(.+)>$" "\\1" _current "${_current}")
        if("${_current}" IN_LIST _visited)
            continue()
        endif()
        list(APPEND _visited "${_current}")
        if(NOT TARGET "${_current}")
            string(FIND "${_current}" "$<" _genex_pos)
            if(NOT _current STREQUAL "" AND _genex_pos EQUAL -1)
                list(APPEND _items "${_current}")
            endif()
            continue()
        endif()
        get_target_property(_current_type "${_current}" TYPE)
        if(NOT _current_type STREQUAL "INTERFACE_LIBRARY")
            list(APPEND _items "$<TARGET_FILE:${_current}>")
        endif()
        get_target_property(_current_deps "${_current}" INTERFACE_LINK_LIBRARIES)
        if(_current_deps)
            list(PREPEND _pending ${_current_deps})
        endif()
    endwhile()
    get_target_property(_link_options onnxruntime::onnxruntime INTERFACE_LINK_OPTIONS)
    if(_link_options)
        foreach(_link_option IN LISTS _link_options)
            list(APPEND _items "${_link_option}")
        endforeach()
    endif()
    set(${out_items} "${_items}" PARENT_SCOPE)
endfunction()

set(CLIP_SHOT_OCR_PROCESS_FEATURES)
set(CLIP_SHOT_OCR_PROCESS_NO_DEFAULT_FEATURES)
set(CLIP_SHOT_OCR_PROCESS_ENVIRONMENT)
if(CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
    _clip_shot_onnxruntime_link_items(_clip_shot_onnxruntime_link_items)
    if(NOT _clip_shot_onnxruntime_link_items)
        message(FATAL_ERROR
            "The static ONNX Runtime link closure for the OCR process is empty.")
    endif()
    list(JOIN _clip_shot_onnxruntime_link_items "\n" _clip_shot_onnxruntime_link_manifest)
    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/clip-ocr-process-onnxruntime-$<CONFIG>.rsp"
        CONTENT "${_clip_shot_onnxruntime_link_manifest}\n")
    set(CLIP_SHOT_OCR_PROCESS_FEATURES static-onnx-runtime)
    if(WIN32)
        list(APPEND CLIP_SHOT_OCR_PROCESS_FEATURES directml-provider)
    endif()
    set(CLIP_SHOT_OCR_PROCESS_NO_DEFAULT_FEATURES NO_DEFAULT_FEATURES)
    set(CLIP_SHOT_OCR_PROCESS_ENVIRONMENT
        "CLIP_ONNXRUNTIME_LINK_FILE=${CMAKE_CURRENT_BINARY_DIR}/generated/clip-ocr-process-onnxruntime-$<CONFIG>.rsp")
endif()

if(APPLE AND NOT CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
    set(CLIP_SHOT_OCR_PROCESS_NO_DEFAULT_FEATURES NO_DEFAULT_FEATURES)
    set(CLIP_SHOT_OCR_PROCESS_FEATURES dynamic-onnx-runtime)
endif()

if(WIN32 OR APPLE)
    list(APPEND CLIP_SHOT_OCR_PROCESS_FEATURES crash-diagnostics)
    list(APPEND CLIP_SHOT_OCR_PROCESS_ENVIRONMENT
        "CLIP_CRASHPAD_LINK_FILE=${CMAKE_CURRENT_BINARY_DIR}/generated/clip-ocr-crash-$<CONFIG>.rsp"
        "CARGO_PROFILE_RELEASE_DEBUG=2" "CARGO_PROFILE_RELEASE_FAST_DEBUG=2")
endif()
if(APPLE)
    list(APPEND CLIP_SHOT_OCR_PROCESS_ENVIRONMENT
        "CARGO_PROFILE_RELEASE_SPLIT_DEBUGINFO=packed"
        "CARGO_PROFILE_RELEASE_FAST_SPLIT_DEBUGINFO=packed")
endif()
clip_add_rust_executable(clip_ocr_process
    PACKAGE clip-ocr-process
    MANIFEST_DIR "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
    OUTPUT_NAME clip-ocr-process
    PERFORMANCE_PROFILE
    REPRODUCIBLE
    ${CLIP_SHOT_OCR_PROCESS_NO_DEFAULT_FEATURES}
    FEATURES ${CLIP_SHOT_OCR_PROCESS_FEATURES}
    ENVIRONMENT ${CLIP_SHOT_OCR_PROCESS_ENVIRONMENT}
)

if(WIN32 OR APPLE)
    add_dependencies(clip_ocr_process_build clip_ocr_diagnostics_bridge)
endif()

set(CLIP_SHOT_CLIP_IMAGE_DIR
    "${CLIP_IMAGE_SOURCE_DIR}"
    CACHE PATH
    "Path to the clip_image project used by Clip."
)
if(NOT TARGET clip_image::clip_image)
    set(CLIP_IMAGE_BUILD_TESTS OFF CACHE BOOL "Build clip_image tests." FORCE)
    set(CLIP_IMAGE_BUILD_SHARED OFF CACHE BOOL "Build the shared clip_image library." FORCE)
    set(CLIP_IMAGE_BUILD_STATIC ON CACHE BOOL "Build the static clip_image library." FORCE)
    set(CLIP_IMAGE_CODEC_PNG ON CACHE BOOL "Enable the libpng codec." FORCE)
    set(CLIP_IMAGE_CODEC_JPEG ON CACHE BOOL "Enable the libjpeg-turbo codec." FORCE)
    set(CLIP_IMAGE_CODEC_HEIF ON CACHE BOOL "Enable HEIF and AVIF codecs." FORCE)
    set(CLIP_IMAGE_CODEC_JXL ON CACHE BOOL "Enable JPEG XL codecs." FORCE)
    set(CLIP_IMAGE_CODEC_WEBP ON CACHE BOOL "Enable WebP codecs." FORCE)
    add_subdirectory("${CLIP_SHOT_CLIP_IMAGE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/clip_image")
endif()
if(NOT TARGET clip_image::clip_image_static)
    message(FATAL_ERROR
        "Clip requires the clip_image static target for its isolated image codec backend.")
endif()
if(TARGET clip_image_static)
    set_property(TARGET clip_image_static PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

set(CLIP_SHOT_IMAGE_CODEC_BACKEND_SOURCES
    src/image/clipimagecodecbridge.h
    src/image/clipimagecodecbackend.cpp
)
if(CLIP_SHOT_RELEASE_STATIC AND CLIP_SHOT_QT_STATIC)
    set(CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC ON CACHE INTERNAL
        "Link the Clip image codec backend statically." FORCE)
    set(_CLIP_SHOT_IMAGE_CODEC_BACKEND_TYPE STATIC)
else()
    set(CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC OFF CACHE INTERNAL
        "Link the Clip image codec backend statically." FORCE)
    set(_CLIP_SHOT_IMAGE_CODEC_BACKEND_TYPE SHARED)
endif()
add_library(clip_shot_image_codec_backend ${_CLIP_SHOT_IMAGE_CODEC_BACKEND_TYPE}
    ${CLIP_SHOT_IMAGE_CODEC_BACKEND_SOURCES}
)
unset(_CLIP_SHOT_IMAGE_CODEC_BACKEND_TYPE)
target_include_directories(clip_shot_image_codec_backend PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
)
target_link_libraries(clip_shot_image_codec_backend PRIVATE
    $<BUILD_INTERFACE:clip_pixel_buffer>
    clip_image::clip_image_static
)
if(NOT CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
    # This backend is a DLL in the Debug/performance workspace builds and
    # cannot borrow codec symbols from the Qt executable. Keep its suppressed
    # static-library dependencies local to the DLL.
    if(NOT CLIP_IMAGE_LINK_PNG_DEPENDENCIES)
        find_package(PNG REQUIRED)
        target_link_libraries(clip_shot_image_codec_backend PRIVATE PNG::PNG)
    endif()
    # Clip Image selects its JPEG headers and library together, including Qt's
    # bundled libjpeg when requested. Its static target propagates those link
    # dependencies; adding vcpkg libjpeg here would override the matching ABI.
endif()
if(CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
    target_compile_definitions(clip_shot_image_codec_backend PUBLIC
        CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC=1
    )
else()
    target_compile_definitions(clip_shot_image_codec_backend PRIVATE
        CLIP_SHOT_IMAGE_CODEC_BACKEND_BUILD=1
    )
endif()
target_compile_features(clip_shot_image_codec_backend PUBLIC cxx_std_20)
set_target_properties(clip_shot_image_codec_backend PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

set(CLIP_SHOT_IMAGE_CODEC_SOURCES
    src/image/clipimageqtcodec.h
    src/image/clipimageqtcodec.cpp
    src/image/clipimageqtsrgbrowreader.h
    src/image/screenshotimageencoding.cpp
)
add_library(clip_shot_image_codec STATIC ${CLIP_SHOT_IMAGE_CODEC_SOURCES})
clip_enable_unity_build(clip_shot_image_codec)
target_include_directories(clip_shot_image_codec PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
    "${CLIP_SHOT_CLIP_IMAGE_DIR}/include"
)
target_link_libraries(clip_shot_image_codec PUBLIC
    clip_shot_image_codec_backend
    clip_canvas_image
    Qt6::Core
    Qt6::Gui
)
target_compile_features(clip_shot_image_codec PUBLIC cxx_std_20)
if(APPLE)
    target_link_libraries(clip_shot_macos_clipboard PRIVATE clip_shot_image_codec)
endif()

if(WIN32 AND NOT CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
    set(_CLIP_SHOT_IMAGE_RUNTIME_STAGING_SCRIPT
        "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/StageRuntimeDependencies.cmake"
    )
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(_CLIP_SHOT_IMAGE_RUNTIME_SEARCH_DIRECTORY
            "$<IF:$<CONFIG:Debug>,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin>"
        )
    else()
        set(_CLIP_SHOT_IMAGE_RUNTIME_SEARCH_DIRECTORY
            "$<TARGET_FILE_DIR:clip_shot_image_codec_backend>"
        )
    endif()

    function(_clip_shot_stage_image_runtime target)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-DCLIP_RUNTIME_ROOT_LIBRARY=$<TARGET_FILE:clip_shot_image_codec_backend>"
                "-DCLIP_RUNTIME_DESTINATION=$<TARGET_FILE_DIR:${target}>"
                "-DCLIP_RUNTIME_SEARCH_DIRECTORY=${_CLIP_SHOT_IMAGE_RUNTIME_SEARCH_DIRECTORY}"
                -P "${_CLIP_SHOT_IMAGE_RUNTIME_STAGING_SCRIPT}"
            COMMENT "Staging Clip Image runtime dependencies for ${target}"
            VERBATIM
        )
    endfunction()

    function(_clip_shot_target_uses_image_codec target output visited_targets)
        if("${target}" IN_LIST visited_targets)
            set(${output} FALSE PARENT_SCOPE)
            return()
        endif()
        list(APPEND visited_targets "${target}")

        get_target_property(_clip_shot_link_libraries "${target}" LINK_LIBRARIES)
        if(NOT _clip_shot_link_libraries)
            set(${output} FALSE PARENT_SCOPE)
            return()
        endif()

        foreach(_clip_shot_link_library IN LISTS _clip_shot_link_libraries)
            if(_clip_shot_link_library MATCHES "clip_shot_image_codec")
                set(${output} TRUE PARENT_SCOPE)
                return()
            endif()
            if(TARGET "${_clip_shot_link_library}")
                get_target_property(_clip_shot_link_target_imported
                    "${_clip_shot_link_library}" IMPORTED)
                if(NOT _clip_shot_link_target_imported)
                    _clip_shot_target_uses_image_codec(
                        "${_clip_shot_link_library}"
                        _clip_shot_transitively_uses_image_codec
                        "${visited_targets}"
                    )
                    if(_clip_shot_transitively_uses_image_codec)
                        set(${output} TRUE PARENT_SCOPE)
                        return()
                    endif()
                endif()
            endif()
        endforeach()

        set(${output} FALSE PARENT_SCOPE)
    endfunction()
endif()

set(
    CLIP_SHOT_ADQT_DIR
    "${CLIP_ANT_DESIGN_SOURCE_DIR}"
    CACHE PATH
    "Path to the ant_design_qt source tree for standalone builds."
)

if(NOT EXISTS "${CLIP_SHOT_ADQT_DIR}/packages/ant_design_qt")
    message(
        FATAL_ERROR
        "Unable to locate ant_design_qt. Set CLIP_SHOT_ADQT_DIR to an external source tree for a standalone build."
    )
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CLIP_SHOT_ICON_MANIFEST
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.manifest.json"
)
set(CLIP_SHOT_ICON_MANIFEST_BUILDER
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_icon_manifest.py"
)
set(CLIP_SHOT_ICON_PACK_GENERATOR
    "${CLIP_SHOT_ADQT_DIR}/tools/generate_icon_pack.py"
)
set(CLIP_SHOT_ICON_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clip_shot/presentation/components/icons/clipshoticons.h"
)
set(CLIP_SHOT_ICON_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/components/icons/clipshoticons.cpp"
)
file(GLOB CLIP_SHOT_ICON_ASSETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/components/icons/resources/*.svg"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CLIP_SHOT_ICON_MANIFEST}"
    "${CLIP_SHOT_ICON_MANIFEST_BUILDER}"
    "${CLIP_SHOT_ICON_PACK_GENERATOR}"
    ${CLIP_SHOT_ICON_ASSETS}
)

execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CLIP_SHOT_ICON_MANIFEST_BUILDER}" --check
    RESULT_VARIABLE _clip_shot_icon_manifest_is_stale
    OUTPUT_QUIET
    ERROR_QUIET
)
if(_clip_shot_icon_manifest_is_stale)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CLIP_SHOT_ICON_MANIFEST_BUILDER}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}" "${CLIP_SHOT_ICON_PACK_GENERATOR}"
        "${CLIP_SHOT_ICON_MANIFEST}"
        --header "${CLIP_SHOT_ICON_HEADER}"
        --source "${CLIP_SHOT_ICON_SOURCE}"
        --check
    RESULT_VARIABLE _clip_shot_icon_pack_is_stale
    OUTPUT_QUIET
    ERROR_QUIET
)
if(_clip_shot_icon_pack_is_stale)
    execute_process(
        COMMAND
            "${Python3_EXECUTABLE}" "${CLIP_SHOT_ICON_PACK_GENERATOR}"
            "${CLIP_SHOT_ICON_MANIFEST}"
            --header "${CLIP_SHOT_ICON_HEADER}"
            --source "${CLIP_SHOT_ICON_SOURCE}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

set(ADQT_ICON_CORE_DIR "${CLIP_SHOT_ADQT_DIR}/packages/adqt_icon_core")
set(ANT_DESIGN_ICONS_QT_DIR "${CLIP_SHOT_ADQT_DIR}/packages/ant_design_icons_qt")
set(ANT_DESIGN_QT_DIR "${CLIP_SHOT_ADQT_DIR}/packages/ant_design_qt")

if(NOT TARGET adqt::widgets)
    add_subdirectory("${CLIP_SHOT_ADQT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/ant_design_qt")
endif()

set(CLIP_SHOT_STORAGE_SOURCES
    include/clip_shot/storage/preparedpngimage.h
    include/clip_shot/storage/applicationstorage.h
    include/clip_shot/storage/appstorageusage.h
    include/clip_shot/storage/capturehistoryrepository.h
    include/clip_shot/storage/capturehistorytypes.h
    include/clip_shot/storage/configurationarchive.h
    include/clip_shot/storage/configurationstore.h
    include/clip_shot/storage/configurationschema.h
    include/clip_shot/storage/persistedselectioncodec.h
    include/clip_shot/storage/persistedwindowgeometry.h
    include/clip_shot/storage/pinnedwindowrepository.h
    include/clip_shot/storage/pinnedwindowtypes.h
    include/clip_shot/storage/settingsadapters.h
    include/clip_shot/storage/storagelogging.h
    include/clip_shot/storage/storageresult.h
    include/clip_shot/storage/storageusagetracker.h
    src/storage/applicationstorage.cpp
    src/storage/storagedirectorychange.cpp
    src/storage/capturehistorypolicy_p.h
    src/storage/capturehistoryrepository.cpp
    src/storage/configurationarchive.cpp
    src/storage/configurationstore.cpp
    src/storage/configurationschema.cpp
    src/storage/persistedselectioncodec.cpp
    src/storage/persistedwindowgeometry.cpp
    src/storage/pinnedwindowrepository.cpp
    src/storage/preparedpngimage.cpp
    src/storage/settingsadapters.cpp
    src/storage/storagedirectoryutils_p.h
    src/storage/storagelogging.cpp
    src/storage/storageusagetracker.cpp
)
add_library(clip_shot_shortcuts STATIC
    include/clip_shot/shortcuts/shortcutbinding.h
    include/clip_shot/shortcuts/shortcutdisplayservice.h
    src/shortcuts/shortcutbinding.cpp
    src/shortcuts/shortcutdisplayservice.cpp
)
target_include_directories(clip_shot_shortcuts PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_shortcuts PUBLIC Qt6::Core Qt6::Gui)
if(APPLE)
    find_library(CLIP_SHOT_CARBON_FRAMEWORK NAMES Carbon REQUIRED)
    find_library(CLIP_SHOT_CORE_FOUNDATION_FRAMEWORK NAMES CoreFoundation REQUIRED)
    target_link_libraries(clip_shot_shortcuts PRIVATE
        "${CLIP_SHOT_CARBON_FRAMEWORK}"
        "${CLIP_SHOT_CORE_FOUNDATION_FRAMEWORK}"
    )
endif()

add_library(clip_shot_storage STATIC ${CLIP_SHOT_STORAGE_SOURCES})
clip_enable_unity_build(clip_shot_storage)
# Repository implementations have independent anonymous-namespace helpers.
set_source_files_properties(
    src/storage/capturehistoryrepository.cpp
    src/storage/pinnedwindowrepository.cpp
    src/storage/configurationschema.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
target_include_directories(clip_shot_storage PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_storage PUBLIC
    clip_shot_diagnostics
    clip_shot_image_codec
    clip_shot_shortcuts
    MINIZIP::minizip-ng
    Qt6::Core
    Qt6::Gui
)

add_library(clip_shot_translation STATIC
    include/clip_shot/translation/translationservice.h
    include/clip_shot/translation/translationlanguages.h
    include/clip_shot/translation/translationlanguagecatalog.h
    src/app/translationservice.cpp
    include/clip_shot/presentation/selectedtexttranslationcontroller.h
    src/presentation/services/selectedtexttranslationcontroller.cpp
    include/clip_shot/network/clipshotapiclient.h
    src/network/clipshotapiclient.cpp
    include/clip_shot/presentation/translationlanguages.h
    src/presentation/services/translationlanguages.cpp
    include/clip_shot/presentation/translationpagecontroller.h
    src/presentation/services/translationpagecontroller.cpp
)
clip_enable_unity_build(clip_shot_translation)
target_include_directories(clip_shot_translation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_translation PUBLIC clip_shot_storage Qt6::Network
    PRIVATE clip_selected_text_c)
target_compile_definitions(clip_shot_translation PRIVATE
    CLIP_SHOT_API_BASE_URL=\"${CLIP_SHOT_API_BASE_URL}\"
)

add_library(clip_shot_window_shortcuts STATIC
    include/clip_shot/presentation/mousereleaseactioncontroller.h
    src/presentation/services/mousereleaseactioncontroller.cpp
    include/clip_shot/presentation/windowshortcutmanager.h
    src/presentation/services/windowshortcutmanager.cpp
)
target_include_directories(clip_shot_window_shortcuts PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(clip_shot_window_shortcuts PUBLIC Qt6::Widgets)
target_link_libraries(clip_shot_window_shortcuts PUBLIC clip_shot_shortcuts)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipShotTranslations.cmake")

set(CLIP_SHOT_SETTINGS_CATALOG_SOURCES
    include/clip_shot/presentation/settings/settingscatalog.h
    include/clip_shot/presentation/settings/settingsregistry.h
    src/presentation/components/icons/clipshoticons.cpp
    src/presentation/settings/settingscatalog.cpp
    src/presentation/settings/settingsregistry.cpp
)
# Widget targets also compile these icons. Keep their definitions separate from
# the catalog so pulling the catalog from its archive does not pull duplicate icons.
set_source_files_properties(src/presentation/components/icons/clipshoticons.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
add_library(clip_shot_settings_catalog STATIC ${CLIP_SHOT_SETTINGS_CATALOG_SOURCES})
clip_enable_unity_build(clip_shot_settings_catalog)
target_include_directories(clip_shot_settings_catalog PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_settings_catalog PUBLIC
    ant_design_icons_qt
    clip_shot_storage
    Qt6::Widgets
)

set(CLIP_SHOT_SETTINGS_SEARCH_SOURCES
    include/clip_shot/presentation/components/applicationsearchwidget.h
    include/clip_shot/presentation/languagemanager.h
    include/clip_shot/presentation/settings/settingssearchindex.h
    src/presentation/components/applicationsearchwidget.cpp
    src/presentation/services/languagemanager.cpp
    src/presentation/settings/settingssearchindex.cpp
)
add_library(clip_shot_settings_search STATIC ${CLIP_SHOT_SETTINGS_SEARCH_SOURCES})
clip_enable_unity_build(clip_shot_settings_search)
target_include_directories(clip_shot_settings_search PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_settings_search PUBLIC
    ant_design_icons_qt
    ant_design_qt
    clip_shot_settings_catalog
    clip_shot_storage
    Qt6::Widgets
)

add_library(clip_shot_main_window_skin STATIC
    include/clip_shot/presentation/mainwindowskincontroller.h
    include/clip_shot/presentation/mainwindowskinwidget.h
    src/presentation/services/mainwindowskincontroller.cpp
    src/presentation/components/mainwindowskinwidget.cpp)
target_include_directories(clip_shot_main_window_skin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_main_window_skin PUBLIC Qt6::Widgets
    PRIVATE clip_shot_storage clip_shot_image_codec clip_draw_engine_qt_canvas)

set(CLIP_SHOT_SETTINGS_SOURCES
    include/clip_shot/presentation/selectedtexttranslationcoordinator.h
    src/presentation/services/selectedtexttranslationcoordinator.cpp
    include/clip_shot/presentation/components/standalonetranslationwindow.h
    src/presentation/components/standalonetranslationwindow.cpp
    include/clip_shot/presentation/windowgeometrymemory.h
    src/presentation/windowgeometrymemory.cpp
    include/clip_shot/presentation/components/translationpagewidget.h
    src/presentation/components/translationpagewidget.cpp
    include/clip_shot/presentation/components/aboutpagewidget.h
    include/clip_shot/platform/windows/autostartregistration.h
    include/clip_shot/platform/windows/focusedfullscreenwindow.h
    include/clip_shot/platform/windows/printscreenshortcutrecorder.h
    include/clip_shot/shortcuts/shortcutrecorder.h
    include/clip_shot/presentation/components/pathinput.h
    include/clip_shot/presentation/components/formfields.h
    include/clip_shot/presentation/translatabletext.h
    include/clip_shot/presentation/settings/settingsformfield.h
    include/clip_shot/presentation/components/globalmouserow.h
    include/clip_shot/presentation/components/toolbareditorsettingswidget.h
    include/clip_shot/presentation/components/infotooltipicon.h
    include/clip_shot/presentation/components/pagecontainerwidget.h
    include/clip_shot/presentation/components/sectionheaderwidget.h
    include/clip_shot/presentation/components/confirmationskipbutton.h
    include/clip_shot/presentation/components/screenshothistorypagewidget.h
    include/clip_shot/presentation/components/emptystateicon.h
    include/clip_shot/presentation/components/texttranslationsettingswidget.h
    include/clip_shot/presentation/components/customaimodelssettingswidget.h
    include/clip_shot/presentation/components/settingscustomwidget.h
    include/clip_shot/presentation/components/settingspagewidget.h
    include/clip_shot/presentation/shortcutdisplaytext.h
    include/clip_shot/presentation/components/actionrow.h
    include/clip_shot/presentation/components/shortcutkeyrow.h
    include/clip_shot/presentation/components/shortcutconfigurationbutton.h
    include/clip_shot/presentation/components/storagestatussettingswidget.h
    include/clip_shot/presentation/screenshottoolbarlayoutmodel.h
    include/clip_shot/presentation/settings/settingsbackend.h
    include/clip_shot/presentation/settings/settingsruntimesession.h
    include/clip_shot/presentation/settings/applicationpriority.h
    include/clip_shot/presentation/settings/textrecognitionacceleration.h
    include/clip_shot/presentation/styles/thememanager.h
    src/presentation/components/aboutpagewidget.cpp
    src/presentation/components/infotooltipicon.cpp
    src/presentation/components/globalmouserow.cpp
    src/presentation/components/pathinput.cpp
    src/presentation/components/formfields.cpp
    src/presentation/settings/settingsformfield.cpp
    src/presentation/components/pagecontainerwidget.cpp
    src/presentation/components/sectionheaderwidget.cpp
    src/presentation/components/screenshothistorypagewidget.cpp
    include/clip_shot/presentation/components/historyselectionbar.h
    include/clip_shot/presentation/components/historypagecommon.h
    src/presentation/components/historypagecommon.cpp
    include/clip_shot/presentation/components/thumbnailcache.h
    src/presentation/components/thumbnailcache.cpp
    include/clip_shot/presentation/components/pinnedwindowmanagementpagewidget.h
    src/presentation/components/pinnedwindowmanagementpagewidget.cpp
    src/presentation/services/screenshotclipboardcontent.cpp
    src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp
    src/presentation/components/texttranslationsettingswidget.cpp
    src/presentation/components/customaimodelssettingswidget.cpp
    src/presentation/components/settingscustomwidget.cpp
    src/presentation/components/settingspagewidget.cpp
    include/clip_shot/presentation/components/screenrecordingsettingsdialog.h
    include/clip_shot/presentation/components/screenrecordingmodal.h
    src/presentation/components/screenrecordingsettingsdialog.cpp
    src/presentation/components/actionrow.cpp
    src/presentation/components/shortcutkeyrow.cpp
    src/presentation/components/shortcutconfigurationbutton.cpp
    src/presentation/components/storagestatussettingswidget.cpp
    src/presentation/settings/settingsbackend.cpp
    src/presentation/settings/settingsruntimesession.cpp
    src/presentation/settings/applicationpriority.cpp
    src/presentation/settings/textrecognitionacceleration.cpp
    src/presentation/styles/buttonborder.cpp
    src/presentation/styles/mainwindowcomponenttoken.cpp
    src/presentation/styles/themecolorscheme.cpp
    src/presentation/styles/thememanager.cpp
    src/presentation/services/shortcutrecorder.cpp
)
if(WIN32)
    list(APPEND CLIP_SHOT_SETTINGS_SOURCES
        src/platform/windows/printscreenshortcutrecorder.cpp)
endif()
set(CLIP_SHOT_GLOBAL_SHORTCUT_SOURCES
    include/clip_shot/platform/focusedfullscreenwindow.h
    include/clip_shot/presentation/globalshortcutmanager.h
    include/clip_shot/presentation/globalshortcuttypes.h
    src/presentation/services/globalshortcutbackend_p.h
    src/presentation/services/globalshortcutbackend.cpp
    src/presentation/services/globalshortcutmanager.cpp
    src/platform/focusedfullscreenwindow.cpp
)
if(WIN32)
    list(APPEND CLIP_SHOT_GLOBAL_SHORTCUT_SOURCES
        src/platform/windows/globalshortcutbackend.cpp
        src/platform/windows/focusedfullscreenwindow.cpp)
elseif(APPLE)
    enable_language(OBJCXX)
    list(APPEND CLIP_SHOT_GLOBAL_SHORTCUT_SOURCES
        src/platform/macos/globalshortcutbackend.cpp
        src/platform/macos/focusedfullscreenwindow.mm)
endif()
list(APPEND CLIP_SHOT_SETTINGS_SOURCES ${CLIP_SHOT_GLOBAL_SHORTCUT_SOURCES})
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/components/screenshothistorypagewidget.cpp"
     _clip_shot_history_source)
string(FIND "${_clip_shot_history_source}" "QThreadPool::globalInstance()"
       _clip_shot_history_global_pool_call)
if(NOT _clip_shot_history_global_pool_call EQUAL -1)
    message(FATAL_ERROR
        "Screenshot History must use its dedicated executor, not QThreadPool::globalInstance().")
endif()
unset(_clip_shot_history_source)
unset(_clip_shot_history_global_pool_call)
add_library(clip_shot_login_item STATIC
    include/clip_shot/platform/macos/loginitemservice.h
    src/platform/macos/loginitemservice.cpp)
target_include_directories(clip_shot_login_item PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_login_item PUBLIC Qt6::Core)
if(APPLE)
    target_sources(clip_shot_login_item PRIVATE src/platform/macos/loginitembackend.mm)
    target_link_libraries(clip_shot_login_item PRIVATE clip_shot_storage
        "-framework AppKit" "-framework Carbon" "-framework Security" "-framework ServiceManagement")
endif()
if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-login-item-tests tests/login_item_tests.cpp)
    target_link_libraries(clip-shot-login-item-tests PRIVATE clip_shot_login_item)
    add_test(NAME clip-shot-login-item-tests COMMAND clip-shot-login-item-tests)
    set_tests_properties(clip-shot-login-item-tests PROPERTIES LABELS unit)
endif()
add_library(clip_shot_system_notification_controller STATIC
    include/clip_shot/presentation/systemnotificationcontroller.h
    src/presentation/services/systemnotificationcontroller.cpp
    src/presentation/services/systemnotificationfeedback.cpp)
target_include_directories(clip_shot_system_notification_controller PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_system_notification_controller PUBLIC Qt6::Core
    PRIVATE ant_design_qt Qt6::Widgets)
if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-system-notification-controller-tests
        tests/system_notification_controller_tests.cpp)
    target_link_libraries(clip-shot-system-notification-controller-tests PRIVATE
        clip_shot_system_notification_controller ant_design_qt Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-system-notification-controller-tests)
    clip_shot_offscreen_qpa_environment(_CLIP_NOTIFICATION_QPA_ENVIRONMENT)
    add_test(NAME clip-shot-system-notification-controller-tests
        COMMAND clip-shot-system-notification-controller-tests -platform offscreen)
    set_tests_properties(clip-shot-system-notification-controller-tests PROPERTIES
        LABELS "unit" TIMEOUT 30 ENVIRONMENT "${_CLIP_NOTIFICATION_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()
if(APPLE)
    add_library(clip_shot_system_notifications STATIC
        src/platform/macos/systemnotificationservice.h
        src/platform/macos/systemnotificationservice.mm)
    target_include_directories(clip_shot_system_notifications PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos"
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip_shot_system_notifications PUBLIC Qt6::Core
        PRIVATE "-framework Foundation" "-framework UserNotifications")
    if(CLIP_SHOT_BUILD_TESTS)
        add_executable(clip-shot-macos-system-notification-tests
            tests/macos_system_notification_tests.mm)
        target_link_libraries(clip-shot-macos-system-notification-tests PRIVATE
            clip_shot_system_notifications "-framework UserNotifications")
        add_test(NAME clip-shot-macos-system-notification-tests
            COMMAND clip-shot-macos-system-notification-tests)
        set_tests_properties(clip-shot-macos-system-notification-tests PROPERTIES
            LABELS "unit;macos" TIMEOUT 30)
    endif()
endif()
add_library(clip_shot_administrator STATIC
    src/platform/windows/autostartregistration.cpp
    src/platform/windows/administratorlaunch.cpp)
target_include_directories(clip_shot_administrator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_administrator PUBLIC Qt6::Core Qt6::Network)
if(WIN32)
    target_link_libraries(clip_shot_administrator PRIVATE taskschd ole32 oleaut32 advapi32 shell32 uuid comsuppw)
endif()
if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-administrator-launch-tests tests/administrator_launch_tests.cpp)
    target_link_libraries(clip-shot-administrator-launch-tests PRIVATE clip_shot_administrator)
    add_test(NAME clip-shot-administrator-launch-tests COMMAND clip-shot-administrator-launch-tests)
    set_tests_properties(clip-shot-administrator-launch-tests PROPERTIES LABELS "unit;windows" TIMEOUT 30)

    add_executable(clip-shot-startup-registration-report-tests
        tests/startup_registration_report_tests.cpp)
    target_link_libraries(clip-shot-startup-registration-report-tests PRIVATE Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-startup-registration-report-tests)
    clip_shot_offscreen_qpa_environment(_CLIP_SHOT_STARTUP_REPORT_QPA_ENVIRONMENT)
    add_test(NAME clip-shot-startup-registration-report-tests
        COMMAND clip-shot-startup-registration-report-tests)
    set_tests_properties(clip-shot-startup-registration-report-tests PROPERTIES
        LABELS unit TIMEOUT 30 ENVIRONMENT "${_CLIP_SHOT_STARTUP_REPORT_QPA_ENVIRONMENT}")

    add_executable(clip-shot-application-restart-tests
        tests/application_restart_tests.cpp
        src/app/applicationrestart.cpp)
    target_include_directories(clip-shot-application-restart-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-application-restart-tests PRIVATE Qt6::Core Qt6::Network)
    add_test(NAME clip-shot-application-restart-tests COMMAND clip-shot-application-restart-tests)
    set_tests_properties(clip-shot-application-restart-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()
add_library(clip_shot_permissions STATIC
    include/clip_shot/presentation/apppermissionservice.h
    src/presentation/services/apppermissionservice.cpp)
target_include_directories(clip_shot_permissions PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_permissions PUBLIC Qt6::Widgets clip_shot_shortcuts
    PRIVATE clip_shot_diagnostics)
if(APPLE)
    target_sources(clip_shot_permissions PRIVATE src/platform/macos/apppermissionbackend.mm)
    target_link_libraries(clip_shot_permissions PRIVATE
        "-framework AppKit"
        "-framework ApplicationServices"
        "-framework AVFoundation")
endif()
find_package(Qt6 REQUIRED COMPONENTS Concurrent)
add_library(clip_shot_cloud_upload STATIC
    include/clip_shot/presentation/screenshotclouduploadservice.h
    src/presentation/services/screenshotclouduploadservice.cpp)
target_include_directories(clip_shot_cloud_upload PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_cloud_upload PUBLIC clip_shot_storage Qt6::Network Qt6::Concurrent)

add_library(clip_shot_settings STATIC ${CLIP_SHOT_SETTINGS_SOURCES}
    include/clip_shot/presentation/permissionguidecontroller.h
    include/clip_shot/presentation/components/permissionguidewidget.h
    src/presentation/services/permissionguidecontroller.cpp
    src/presentation/components/permissionguidewidget.cpp)
if(APPLE)
    target_sources(clip_shot_settings PRIVATE src/platform/macos/permissionguideplatform.mm)
endif()
target_sources(clip_shot_settings PRIVATE
    include/clip_shot/presentation/components/clouduploadsettingswidget.h
    src/presentation/components/clouduploadsettingswidget.cpp)
target_link_libraries(clip_shot_settings PUBLIC clip_shot_permissions clip_shot_login_item)
qt_add_resources(clip_shot_settings "clip_shot_about_assets"
    PREFIX "/clip_shot/about"
    BASE "resources/about"
    FILES resources/about/illustration.svg
)
target_compile_definitions(clip_shot_settings PRIVATE
    CLIP_SHOT_WEBSITE_URL="${CLIP_SHOT_WEBSITE_URL}"
    CLIP_SHOT_PROJECT_URL="${CLIP_SHOT_PROJECT_URL}"
    CLIP_SHOT_QQ_GROUP_2_URL="${CLIP_SHOT_QQ_GROUP_2_URL}"
    CLIP_SHOT_QQ_GROUP_3_URL="${CLIP_SHOT_QQ_GROUP_3_URL}"
)
target_link_libraries(clip_shot_settings PRIVATE clip_shot_image_codec Qt6::Svg Qt6::Network clip_shot_administrator)
add_library(clip_shot_global_mouse STATIC
    include/clip_shot/presentation/globalmousemanager.h
    include/clip_shot/presentation/globalmousegesture.h
    include/clip_shot/presentation/globalmousetypes.h
    src/presentation/services/globalmousemanager.cpp
    src/presentation/services/globalmousegesture.cpp
    src/platform/globalmousebackend.cpp
)
target_include_directories(clip_shot_global_mouse PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_global_mouse PUBLIC clip_shot_storage Qt6::Gui)
if(WIN32)
    target_sources(clip_shot_global_mouse PRIVATE
        src/platform/windows/globalmousebackend.cpp
        src/platform/windows/globalmousebackend_p.h)
    target_link_libraries(clip_shot_global_mouse PRIVATE dwmapi user32)
endif()
if(APPLE)
    enable_language(OBJCXX)
    target_sources(clip_shot_global_mouse PRIVATE
        src/platform/macos/globalmousebackend.mm
        src/platform/macos/globalmouseinput.cpp
        src/platform/macos/globalmousebackend_p.h)
    find_library(CLIP_SHOT_APPKIT_FRAMEWORK NAMES AppKit REQUIRED)
    find_library(CLIP_SHOT_APPLICATION_SERVICES_FRAMEWORK NAMES ApplicationServices REQUIRED)
    find_library(CLIP_SHOT_CORE_FOUNDATION_FRAMEWORK NAMES CoreFoundation REQUIRED)
    find_library(CLIP_SHOT_CORE_GRAPHICS_FRAMEWORK NAMES CoreGraphics REQUIRED)
    target_link_libraries(clip_shot_global_mouse PRIVATE
        "${CLIP_SHOT_CARBON_FRAMEWORK}"
        "${CLIP_SHOT_APPKIT_FRAMEWORK}"
        "${CLIP_SHOT_APPLICATION_SERVICES_FRAMEWORK}"
        "${CLIP_SHOT_CORE_FOUNDATION_FRAMEWORK}"
        "${CLIP_SHOT_CORE_GRAPHICS_FRAMEWORK}")
    target_link_libraries(clip_shot_settings PRIVATE
        "${CLIP_SHOT_CARBON_FRAMEWORK}"
        "${CLIP_SHOT_APPKIT_FRAMEWORK}"
        "${CLIP_SHOT_CORE_GRAPHICS_FRAMEWORK}")
endif()
target_link_libraries(clip_shot_settings PRIVATE clip_shot_global_mouse)
clip_enable_unity_build(clip_shot_settings)
set_source_files_properties(
    src/presentation/styles/thememanager.cpp
    src/presentation/components/shortcutkeyrow.cpp
    src/platform/macos/globalshortcutbackend.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
target_include_directories(clip_shot_settings PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_settings PUBLIC
    clip_shot_main_window_skin
    clip_shot_window_shortcuts
    clip_shot_translation
    ant_design_icons_qt
    ant_design_qt
    clip_shot_settings_search
    clip_shot_storage
    Qt6::Widgets
)
if(WIN32)
    target_link_libraries(clip_shot_settings PUBLIC advapi32 bcrypt dwmapi ntdll user32)
endif()

set(PROJECT_HEADERS
    include/clip_shot/app/featureavailability.h
    include/clip_shot/app/applicationinputguard.h
    include/clip_shot/presentation/screenshotencodingsettings.h
    include/clip_shot/presentation/directcapturecontroller.h
    include/clip_shot/presentation/directcaptureworkflow.h
    include/clip_shot/presentation/directcapturehistory.h
    include/clip_shot/presentation/screenshottabledocument.h
    include/clip_shot/presentation/screenshottableeditor.h
    include/clip_shot/app/applicationcontroller.h
    include/clip_shot/app/applicationrestart.h
    include/clip_shot/app/updateconfirmationdialog.h
    include/clip_shot/app/singleinstancecoordinator.h
    include/clip_shot/platform/windows/autostartregistration.h
    include/clip_shot/platform/windows/focusedfullscreenwindow.h
    include/clip_shot/platform/windows/monitorgeometry.h
    include/clip_shot/platform/windows/windowchrome.h
    include/clip_shot/platform/windowcaptureexclusion.h
    include/clip_shot/platform/physicalcursor.h
    include/clip_shot/presentation/components/aspectratiolockbutton.h
    include/clip_shot/presentation/components/contentcardwidget.h
    include/clip_shot/presentation/components/maincontentheaderwidget.h
    include/clip_shot/presentation/components/icons/clipshoticons.h
    include/clip_shot/presentation/components/settingspageutils.h
    include/clip_shot/presentation/components/sidebarwidget.h
    include/clip_shot/presentation/components/titlebarwidget.h
    include/clip_shot/presentation/mainwindow.h
    include/clip_shot/presentation/screenshotcapturecoordinator.h
    include/clip_shot/presentation/screenshotcapturedisplaymodelreconciler.h
    include/clip_shot/presentation/screenshotcaptureruntimeadapter.h
    include/clip_shot/presentation/screenshotcapturestate.h
    include/clip_shot/presentation/screenshotcaptureworkflow.h
    include/clip_shot/presentation/screenshotstartupcontext.h
    include/clip_shot/presentation/screenshotcanvascolorsampler.h
    include/clip_shot/presentation/screenshotcanvascolorsamplerwindow.h
    include/clip_shot/presentation/screenshotcaptureworkflowports.h
    include/clip_shot/presentation/screenshotcanvasrenderer.h
    include/clip_shot/presentation/screenshotclipboardcontent.h
    include/clip_shot/presentation/screenshotclipboardservice.h
    include/clip_shot/presentation/screenshotresultcompositor.h
    include/clip_shot/presentation/screenshotselectionshadowrenderer.h
    include/clip_shot/presentation/screenshotcolorpickercontroller.h
    include/clip_shot/presentation/screenshotcolorpickerwindow.h
    include/clip_shot/presentation/screenshotcontroller.h
    include/clip_shot/presentation/screenshotdialogowner.h
    include/clip_shot/presentation/screenshotshortcutexitconfirmation.h
    include/clip_shot/presentation/windowgroupswitchercontroller.h
    include/clip_shot/presentation/windowgroupswitcherpopup.h
    include/clip_shot/presentation/pinnedwindowgroupmanager.h
    include/clip_shot/presentation/globalshortcuttypes.h
    include/clip_shot/presentation/screenshotdefaultstyles.h
    include/clip_shot/presentation/screenshotcanvastoolstyles.h
    include/clip_shot/presentation/screenshotdisplayconfigurationobserver.h
    include/clip_shot/presentation/screenshotdisplaysession.h
    include/clip_shot/presentation/screenshotexportservice.h
    include/clip_shot/presentation/screenshotexportartifact.h
    include/clip_shot/presentation/screenshotexportcoordinator.h
    include/clip_shot/presentation/screenshotimagefileservice.h
    include/clip_shot/presentation/screenshotimagerowsource.h
    include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
    include/clip_shot/presentation/historypinplacement.h
    include/clip_shot/presentation/screenshotselectionpin.h
    include/clip_shot/presentation/screenshotgeometry.h
    include/clip_shot/presentation/screenshotguidelinerendering.h
    include/clip_shot/presentation/screenshothistoryservice.h
    include/clip_shot/presentation/screenshothistorytypes.h
    include/clip_shot/presentation/screenshotintelligentselectionmodel.h
    include/clip_shot/presentation/screenshotinteractionstate.h
    include/clip_shot/presentation/screenshotocrcontroller.h
    include/clip_shot/presentation/screenshotocrrecognitionservice.h
    include/clip_shot/presentation/screenshotocrassets.h
    include/clip_shot/presentation/screenshotrecognitionresults.h
    include/clip_shot/presentation/screenshotqrrecognitionservice.h
    include/clip_shot/presentation/screenshotmessageservice.h
    include/clip_shot/presentation/screenshotocrpresentation.h
    include/clip_shot/ocrtextoptions.h
    include/clip_shot/presentation/screenshotocrtexteditingsession.h
    include/clip_shot/presentation/screenshotocrvisuals.h
    include/clip_shot/presentation/screenshotocrtextlayer.h
    include/clip_shot/presentation/screenshotrecognitionwindow.h
    include/clip_shot/presentation/screenshotrecognitionsessioncontroller.h
    include/clip_shot/presentation/screenshotpinnededitcontroller.h
    include/clip_shot/presentation/screenshotpinnedwindow.h
    include/clip_shot/presentation/screenshotpresentationservices.h
    include/clip_shot/presentation/screenshotshortcuthints.h
    include/clip_shot/presentation/screenshotsmartselectiontransition.h
    include/clip_shot/presentation/screenshotselectioneditworkflow.h
    include/clip_shot/presentation/screenshotselectioneditworkflowports.h
    include/clip_shot/presentation/screenshotselectionexportuiservices.h
    include/clip_shot/presentation/screenshotselectiongeometry.h
    include/clip_shot/presentation/screenshotselectionlimits.h
    include/clip_shot/image/screenshotregiongeometry.h
    include/clip_shot/image/screenshotregionpoints.h
    include/clip_shot/presentation/screenshotregionpreferences.h
    include/clip_shot/presentation/screenshotregiontypecontrol.h
    include/clip_shot/presentation/screenshotselectionmodel.h
    include/clip_shot/presentation/screenshotselectionexportworkflowports.h
    include/clip_shot/presentation/screenshotselectionparams.h
    include/clip_shot/presentation/screenshotselectionresizemodalcontent.h
    include/clip_shot/presentation/screenshotselectionresizeworkflow.h
    include/clip_shot/presentation/screenshotsaveasfiledialog.h
    include/clip_shot/presentation/screenshotselectionsettingsstore.h
    include/clip_shot/presentation/screenshotsourceimagecomposer.h
    include/clip_shot/presentation/screenshotselectiontoolbarwidget.h
    include/clip_shot/presentation/screenshotscrollingcapturecontroller.h
    include/clip_shot/presentation/screenshotscrollingsnapshot.h
    include/clip_shot/presentation/screenshotscrollingtypes.h
    include/clip_shot/presentation/screenshotscrollingthumbnailwidget.h
    include/clip_shot/presentation/screenshotoverlaycanvaspresenter.h
    include/clip_shot/presentation/screenshotoverlaycoordinator.h
    include/clip_shot/presentation/screenshotoverlayeventsink.h
    include/clip_shot/presentation/screenshotoverlayinteractionadapter.h
    include/clip_shot/presentation/screenshotoverlayinputhandler.h
    include/clip_shot/presentation/screenshotoverlayshortcutcontroller.h
    include/clip_shot/presentation/screenshotoverlaypool.h
    include/clip_shot/presentation/screenshotoverlayuihost.h
    include/clip_shot/presentation/screenshotoverlaywindow.h
    include/clip_shot/presentation/screenshotselectorcoordinator.h
    src/presentation/selector/screenshotselectorpolicy.h
    include/clip_shot/presentation/screenshotselectorworkflow.h
    include/clip_shot/presentation/screenshotselectorworkflowports.h
    include/clip_shot/presentation/screenshottoolbarcommands.h
    include/clip_shot/presentation/screenshottoolbarpresenter.h
    include/clip_shot/presentation/screenshottoolbarpresentationstatefactory.h
    include/clip_shot/presentation/screenshottoolcommandworkflow.h
    include/clip_shot/presentation/screenshottoolcommandworkflowports.h
    include/clip_shot/presentation/screenshottoolbarmainpanel.h
    include/clip_shot/presentation/screenshottoolpalette.h
    include/clip_shot/presentation/screenshottoolpalettehost.h
    include/clip_shot/presentation/screenshottoolbarwindow.h
    include/clip_shot/presentation/screenshottypes.h
    include/clip_shot/presentation/screenshotuipreferences.h
    include/clip_shot/presentation/screenshotselectiondisplayunit.h
    include/clip_shot/presentation/screenrecordingareawindow.h
    include/clip_shot/presentation/screenrecordingcontroller.h
    include/clip_shot/presentation/screenrecordingfolder.h
    include/clip_shot/presentation/screenrecordingshortcutcontroller.h
    include/clip_shot/presentation/screenrecordingtoolbarwindow.h
    include/clip_shot/presentation/styles/mainwindowcomponenttoken.h
    include/clip_shot/presentation/styles/buttonborder.h
    include/clip_shot/presentation/styles/themecolorscheme.h
    include/clip_shot/presentation/floatingtoolbarcontroller.h
    include/clip_shot/presentation/systemtraycontroller.h
)

add_library(clip_shot_print STATIC src/presentation/services/screenshotprintservice.cpp)
target_include_directories(clip_shot_print PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(clip_shot_print PUBLIC Qt6::Widgets PRIVATE Qt6::Concurrent)
if(WIN32)
    target_sources(clip_shot_print PRIVATE src/platform/windows/nativeprint.cpp
        src/platform/windows/nativeprintdocument.cpp)
    target_link_libraries(clip_shot_print PRIVATE shell32 uuid ole32 runtimeobject windowsapp
        d2d1 d3d11 dxgi windowscodecs)
elseif(APPLE)
    target_sources(clip_shot_print PRIVATE src/platform/macos/nativeprint.mm)
    target_compile_options(clip_shot_print PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
    target_link_libraries(clip_shot_print PRIVATE "-framework AppKit")
else()
    target_sources(clip_shot_print PRIVATE src/presentation/services/nativeprintunsupported.cpp)
endif()

set(PROJECT_CXX_SOURCES
    ${CLIP_SELECTED_FILES_SOURCES}
    src/presentation/services/screenshotfilepinbatch.cpp
    src/presentation/capture/directcapturecontroller.cpp
    src/presentation/capture/directcapturenative.cpp
    src/presentation/capture/directcaptureworkflow.cpp
    src/presentation/capture/directcapturehistory.cpp
    src/presentation/capture/camerashuttersound.cpp
    src/app/applicationcontroller.cpp
    src/app/updateconfirmationdialog.cpp
    src/app/applicationrestart.cpp
    src/app/featureavailability.cpp
    src/app/main.cpp
    src/app/singleinstancecoordinator.cpp
    src/platform/physicalcursor.cpp
    src/platform/windows/windowchrome.cpp
    src/platform/windows/windowcursorrefresh.h
    src/platform/windows/scrollinput.cpp
    src/presentation/components/contentcardwidget.cpp
    src/presentation/components/maincontentheaderwidget.cpp
    src/presentation/components/sidebarwidget.cpp
    src/presentation/components/titlebarwidget.cpp
    src/presentation/mainwindow.cpp
    src/presentation/capture/screenshotcapturecoordinator.cpp
    src/presentation/capture/screenshotcapturedisplaymodelreconciler.cpp
    src/presentation/capture/screenshotcaptureruntimeadapter.cpp
    src/presentation/capture/screenshotcaptureworker.cpp
    src/presentation/capture/screenshotcapturepolicy.cpp
    src/presentation/capture/screenshotcaptureperfinstrumentation.cpp
    src/presentation/capture/screenshotcaptureworkflow.cpp
    src/presentation/capture/screenshotglobalmousedrag.cpp
    src/presentation/capture/adaptivescrollingcapturecadence.h
    src/presentation/capture/windowcaptureexclusion.h
    src/presentation/capture/windowinputtransparency.h
    src/presentation/capture/latestframemailbox.h
    src/presentation/capture/screenshotscrollingcapturecontroller.cpp
    src/presentation/capture/screenshotscrollingpipeline.cpp
    src/presentation/capture/screenshotscrollingnativesource.cpp
    src/presentation/capture/screenshotscrollingthumbnailwidget.cpp
    src/presentation/capture/scrollinghoverpreview.h
    src/presentation/services/screenshotcanvascolorsampler.cpp
    src/presentation/services/screenshotcanvascolorsamplerwindow.cpp
    src/presentation/services/screenshotcolorpickercontroller.cpp
    src/presentation/services/screenshotclipboardservice.cpp
    src/presentation/services/screenshotcolorpickerwindow.cpp
    src/presentation/services/screenshotcontroller.cpp
    src/presentation/services/screenshotshortcutexitconfirmation.cpp
    src/presentation/services/screenshotexportcoordinator.cpp
    src/presentation/services/screenshotmessageservice.cpp
    src/presentation/services/screenshothistoryservice.cpp
    src/presentation/services/screenshotdisplayconfigurationobserver.cpp
    src/presentation/services/screenshotexportservice.cpp
    src/presentation/services/screenshotexportartifact.cpp
    src/presentation/services/screenshotexportpinnedsource.cpp
    src/presentation/ocr/screenshotrecognitionimage.cpp
    src/presentation/services/screenshotexportscrollingsource.cpp
    src/presentation/services/screenshotimagefileservice.cpp
    src/presentation/services/screenshotrecognitionfileexport.cpp
        src/presentation/services/screenshotpdfexport.cpp
    src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
    src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
    src/presentation/core/screenshotcanvastoolstyles.cpp
    src/presentation/core/screenshotstylebinding.cpp
    src/presentation/core/screenshotintelligentselectionmodel.cpp
    src/presentation/core/screenshotinteractionstate.cpp
    src/presentation/core/screenshotsmartselectiontransition.cpp
    src/presentation/ocr/screenshotocrcontroller.cpp
    src/presentation/ocr/screenshotrecognitionsessioncontroller.cpp
    src/presentation/ocr/screenshotocrlayout.cpp
    src/presentation/ocr/screenshotrecognitionwindow.cpp
    src/presentation/ocr/screenshottabledocument.cpp
    src/presentation/ocr/screenshottableeditor.cpp
    src/presentation/ocr/screenshotocrtexttransform.cpp
    src/presentation/ocr/screenshotocrrecognitionservice.cpp
    src/presentation/ocr/screenshotocrassets.cpp
    src/presentation/ocr/screenshotqrcontroller.cpp
    include/clip_shot/presentation/screenshotqrcontroller.h
    src/presentation/ocr/screenshotqrrecognitionservice.cpp
    src/presentation/ocr/screenshotocrpresentation.cpp
    src/presentation/ocr/screenshotocrvisuals.cpp
    src/presentation/ocr/screenshotocrtexteditingsession.cpp
    src/presentation/pinned/screenshotpinnednativegeometrycontroller.cpp
    src/presentation/pinned/screenshotpinnednativegeometrycontroller.h
    src/presentation/pinned/screenshotpinnedclickthroughgeometry.cpp
    src/presentation/pinned/screenshotpinnedclickthroughgeometry.h
    src/presentation/pinned/screenshotpinnedinteraction.cpp
    include/clip_shot/presentation/pinnedwindowselectioncontroller.h
    src/presentation/pinned/pinnedwindowselectioncontroller.cpp
    src/presentation/pinned/pinnedwindowselectioninteraction.cpp
    src/presentation/pinned/pinnedwindowselectiongeometry.cpp
    src/presentation/pinned/screenshotpinneddragexport.cpp
    ${CLIP_PINNED_PLATFORM_SOURCES}
    src/presentation/pinned/screenshotpinnededitcontroller.cpp
    include/clip_shot/presentation/globalcanvascontroller.h
    src/presentation/globalcanvas/globalcanvascontroller.cpp
    src/presentation/globalcanvas/globalcanvasplatform.cpp
    src/presentation/pinned/screenshotpinnedresizegeometry.cpp
    src/presentation/pinned/screenshotpinnedrestoregeometry.cpp
    src/presentation/pinned/screenshotpinnedhidetotopcontroller.cpp
    src/presentation/pinned/screenshotpinnedwindow.cpp
    src/presentation/pinned/screenshotpintoperfinstrumentation.cpp
    src/presentation/services/screenshotpresentationservices.cpp
    src/presentation/services/windowgroupswitchercontroller.cpp
    src/presentation/components/windowgroupswitcherpopup.cpp
    src/presentation/services/pinnedwindowgroupmanager.cpp
    src/presentation/services/screenshotsourceimagecomposer.cpp
    src/presentation/services/systemtraycontroller.cpp
    src/presentation/selection/screenshotselectioneditworkflow.cpp
    src/presentation/selection/screenshotselectionexportuiservices.cpp
    src/presentation/selection/screenshotselectiongeometry.cpp
    src/presentation/selection/screenshotselectionmodel.cpp
    src/presentation/selection/screenshotselectionparams.cpp
    src/presentation/selection/screenshotselectionresizemodalcontent.cpp
    src/presentation/selection/screenshotselectionresizeworkflow.cpp
    src/presentation/services/screenshotsaveasfiledialog.cpp
    src/presentation/services/screenshotsaveexportpipeline.cpp
    include/clip_shot/presentation/screenshotsavepreviewcanvas.h
    src/presentation/services/screenshotsavepreviewcanvas.cpp
    src/presentation/selection/screenshotselectionsettingsstore.cpp
    src/presentation/selection/screenshotselectiontoolbarwidget.cpp
    src/presentation/selection/screenshotselectiontoolbarwidgets.cpp
    src/presentation/overlay/screenshotoverlaycanvaspresenter.cpp
    src/presentation/overlay/screenshotoverlaycanvaswidget.cpp
    src/presentation/overlay/screenshotoverlaycanvaswidget.h
    src/presentation/ocr/screenshotocrtextlayout.cpp
    src/presentation/overlay/screenshotcanvasrenderer.cpp
    src/presentation/overlay/screenshotoverlaycoordinator.cpp
    src/presentation/overlay/screenshotoverlayinteractionadapter.cpp
    src/presentation/overlay/screenshotoverlayinputhandler.cpp
    src/presentation/overlay/screenshotoverlayshortcutcontroller.cpp
    src/presentation/overlay/screenshotoverlaypool.cpp
    src/presentation/overlay/screenshotoverlayuihost.cpp
    src/presentation/overlay/screenshotoverlayframepresenter.cpp
    src/presentation/overlay/screenshotoverlayframepresenter.h
    src/presentation/overlay/screenshotoverlaywindow.cpp
    src/presentation/selector/screenshotselectorserviceclient.cpp
    src/presentation/selector/screenshotselectorcoordinator.cpp
    src/presentation/selector/screenshotselectorworkflow.cpp
    src/presentation/toolbar/screenshottoolbarpresenter.cpp
    src/presentation/toolbar/screenshottoolbarpresentationstatefactory.cpp
    src/presentation/tools/screenshotautofiltercontroller.cpp
    include/clip_shot/presentation/screenshotautofiltercontroller.h
    src/presentation/tools/screenshottoolcommandworkflow.cpp
    src/presentation/tools/screenshottoolbarmainpanel.cpp
    src/presentation/tools/screenshottoolpalettebuttons.cpp
    src/presentation/tools/screenshottoolpalettestylecontrols.cpp
    src/presentation/tools/screenshottoolpalettestylecomponents.cpp
    src/presentation/tools/screenshottoolpalettestylemodel.cpp
    src/presentation/tools/screenshottoolpalettestylepresets.cpp
    src/presentation/tools/screenshottoolpalette.cpp
    src/presentation/recording/recordingaudiogainpopover.cpp
    src/presentation/recording/recordingaudiogainpopover.h
    src/presentation/tools/screenshottoolpalettehost.cpp
    src/presentation/toolbar/floatingtoolbarcontroller.cpp
    src/presentation/toolbar/screenshottoolbarwindow.cpp
    src/presentation/recording/screenrecordingareawindow.cpp
    src/presentation/recording/recordingtrimtoolbar.cpp
    src/presentation/recording/recordingtrimsession.cpp
    src/presentation/recording/recordingrenderdialog.cpp
    src/presentation/recording/recordingrenderdialog.h
    src/presentation/recording/recordingrenderjob.cpp
    src/presentation/recording/recordingrenderjob.h
    src/presentation/recording/screenrecordingcontroller.cpp
    src/presentation/recording/recordingcolorsampler.cpp
    src/presentation/recording/recordingcolorsampler.h
    src/presentation/recording/screenrecordingfolder.cpp
    src/presentation/recording/recordingcountdownoverlay.cpp
    src/presentation/recording/recordingcountdownoverlay.h
        src/presentation/recording/recordingeffectpreview.cpp
        src/presentation/recording/recordingeffectpreview.h
        src/presentation/recording/recordingeffectstyle.h
    src/presentation/recording/screenrecordingshortcutcontroller.cpp
    src/presentation/recording/screenrecordinggeometry.cpp
    src/presentation/recording/screenrecordingtoolbarwindow.cpp
)
if(APPLE)
    list(APPEND PROJECT_CXX_SOURCES
        src/platform/macos/applicationactivation.mm
        src/platform/macos/systemtraymenu.mm
        src/platform/macos/windowcaptureexclusion.mm)
    list(APPEND PROJECT_HEADERS include/clip_shot/platform/macos/applicationactivation.h)
endif()

include(ClipSimd)
clip_configure_avx2_sources(src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp)

set(PROJECT_RCC_SOURCES)

set(PROJECT_SOURCES
    ${PROJECT_CXX_SOURCES}
    ${PROJECT_HEADERS}
    ${PROJECT_RCC_SOURCES}
    ${TS_FILES}
)

qt_add_executable(clip_shot
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)
clip_shot_link_macos_window_platform(clip_shot)
if(CLIP_SHOT_BUILD_TESTS)
    clip_shot_import_offscreen_platform(clip_shot)
endif()

if(WIN32)
    configure_file(
        resources/windows-app-resource.rc.in
        "${CMAKE_CURRENT_BINARY_DIR}/generated/windows-app-resource.rc"
        @ONLY
    )
    target_sources(clip_shot PRIVATE
        resources/app-icon.ico
        "${CMAKE_CURRENT_BINARY_DIR}/generated/windows-app-resource.rc"
    )
endif()

target_compile_definitions(clip_shot PRIVATE
    CLIP_SHOT_API_BASE_URL=\"${CLIP_SHOT_API_BASE_URL}\"
)
if(APPLE AND CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
    target_compile_definitions(clip_shot PRIVATE CLIP_SHOT_OCR_STATIC_ONNXRUNTIME=1)
endif()
if(WIN32)
    target_compile_definitions(clip_shot PRIVATE
        CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE=\"${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}\"
    )
endif()
target_link_libraries(clip_shot PRIVATE
    clip_shot_crash_bridge
    clip_shot_image_codec
    MINIZIP::minizip-ng
)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipShotUpdates.cmake")
if(WIN32)
    target_link_libraries(clip_shot PRIVATE ole32 oleaut32 shell32 uuid)
endif()
target_compile_definitions(clip_shot PRIVATE
    QT_MESSAGELOGCONTEXT
    CLIP_DIAGNOSTICS_VERSION="${CLIP_SHOT_VERSION}"
    CLIP_DIAGNOSTICS_REVISION="${CLIP_DIAGNOSTICS_REVISION}"
    CLIP_DIAGNOSTICS_BUILD="$<CONFIG>")
if(WIN32)
    add_custom_command(TARGET clip_shot POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CLIP_CRASHPAD_HANDLER}"
            "$<TARGET_FILE_DIR:clip_shot>/crashpad_handler.exe")
    install(PROGRAMS "${CLIP_CRASHPAD_HANDLER}" DESTINATION bin)
endif()
if(MSVC)
    target_compile_options(clip_shot PRIVATE $<$<CONFIG:Release>:/Z7>)
    target_link_options(clip_shot PRIVATE $<$<CONFIG:Release>:/DEBUG:FULL>)
endif()

target_link_libraries(clip_shot PRIVATE clip_shot_system_notification_controller)
if(APPLE)
    target_link_libraries(clip_shot PRIVATE ${CLIP_SELECTED_FILES_LIBRARIES})
    target_link_libraries(clip_shot PRIVATE clip_shot_system_notifications)
    target_sources(clip_shot PRIVATE src/platform/macos/camerashuttersound.mm)
    target_link_libraries(clip_shot PRIVATE "-framework AVFoundation" "-framework Foundation" objc)
    qt_add_resources(clip_shot "clip_shot_shutter_audio"
        PREFIX "/clip-shot"
        BASE "resources"
        FILES "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}")
endif()

if(EXISTS "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}")
    add_custom_command(TARGET clip_shot POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:clip_shot>/audios"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}"
            "$<TARGET_FILE_DIR:clip_shot>/audios/camera_shutter.mp3"
        COMMENT "Staging the camera shutter audio"
        VERBATIM
    )
else()
    message(WARNING
        "Camera shutter audio was not found: ${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}")
endif()

qt_add_resources(clip_shot "clip_shot_app_assets"
    PREFIX "/clip-shot/app-icons"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-default.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-light.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-dark.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-default.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-light.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-dark.png"
)

function(clip_shot_add_image_codec_smoke_test)
    add_executable(clip-shot-image-codec-backend-smoke-tests
        tests/clipimage_codec_backend_smoke_tests.cpp
    )
    target_link_libraries(clip-shot-image-codec-backend-smoke-tests PRIVATE
        clip_shot_image_codec_backend
    )
    set_target_properties(clip-shot-image-codec-backend-smoke-tests PROPERTIES
        AUTOMOC OFF
        AUTORCC OFF
        AUTOUIC OFF
    )
endfunction()

if(CLIP_SHOT_BUILD_IMAGE_CODEC_SMOKE_TEST AND NOT CLIP_SHOT_BUILD_TESTS)
    clip_shot_add_image_codec_smoke_test()
    set_target_properties(clip-shot-image-codec-backend-smoke-tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/verification-bin"
    )
endif()

set(CLIP_SHOT_RECOGNITION_IMAGE_RENDER_SOURCES
    src/presentation/ocr/screenshotrecognitionimage.cpp
    src/presentation/ocr/screenshotocrtextlayout.cpp
    src/presentation/ocr/screenshotocrpresentation.cpp
    src/presentation/overlay/screenshotselectionshadowrenderer.cpp)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipShotRuntimeTests.cmake")

if(CLIP_SHOT_BUILD_TESTS)
    # Private C ABI names keep these fixtures' mocks distinct from the real
    # exports in the Rust bundle used by unrelated transitive dependencies.
    function(clip_shot_alias_test_rust_ffi target)
        foreach(symbol IN LISTS ARGN)
            string(REGEX REPLACE "^clip_" "clip_test_" alias "${symbol}")
            target_compile_definitions(${target} PRIVATE "${symbol}=${alias}")
        endforeach()
    endfunction()

    add_test(NAME clip-shot-translation-catalog-tests
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/translation_catalog_tests.py")
    set_tests_properties(clip-shot-translation-catalog-tests PROPERTIES LABELS "unit")

    if(WIN32)
        add_test(NAME clip-shot-windows-architecture-tests
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-windows-architecture.py")
        set_tests_properties(clip-shot-windows-architecture-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 120)
        find_program(CLIP_SHOT_INSTALLER_TEST_POWERSHELL NAMES pwsh powershell REQUIRED)
        add_test(NAME clip-shot-installer-running-app-tests
            COMMAND "${CLIP_SHOT_INSTALLER_TEST_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-clip-shot-installer.ps1")
        set_tests_properties(clip-shot-installer-running-app-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 180)
        add_test(NAME clip-shot-installer-upgrade-tests
            COMMAND "${CLIP_SHOT_INSTALLER_TEST_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-clip-shot-installer-upgrade.ps1"
                -HelperPath "$<TARGET_FILE:clip-shot-updater-binary>"
                -Architecture "${CLIP_WINDOWS_ARCHITECTURE}")
        set_tests_properties(clip-shot-installer-upgrade-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 180)
        add_test(NAME clip-shot-installer-i18n-tests
            COMMAND "${CLIP_SHOT_INSTALLER_TEST_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-clip-shot-installer-i18n.ps1")
        set_tests_properties(clip-shot-installer-i18n-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 180)
        add_test(NAME clip-shot-package-compression-tests
            COMMAND "${CLIP_SHOT_INSTALLER_TEST_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-clip-shot-package-compression.ps1")
        set_tests_properties(clip-shot-package-compression-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 60)
    endif()
    if(DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(_CLIP_SHOT_RUNTIME_OUTPUT_DIRECTORY_WAS_DEFINED TRUE)
        set(
            _CLIP_SHOT_SAVED_RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    else()
        set(_CLIP_SHOT_RUNTIME_OUTPUT_DIRECTORY_WAS_DEFINED FALSE)
    endif()
    set(
        CMAKE_RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/test-bin"
    )

    enable_testing()

    add_executable(clip-shot-recognition-image-tests tests/screenshot_recognition_image_tests.cpp
        ${CLIP_SHOT_RECOGNITION_IMAGE_RENDER_SOURCES})
    target_include_directories(clip-shot-recognition-image-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-recognition-image-tests PRIVATE adqt::widgets Qt6::Gui clip_canvas_image)
    clip_shot_import_offscreen_platform(clip-shot-recognition-image-tests)
    qt_import_plugins(clip-shot-recognition-image-tests
        NO_DEFAULT INCLUDE Qt6::QOffscreenIntegrationPlugin)
    add_test(NAME clip-shot-recognition-image-tests
        COMMAND clip-shot-recognition-image-tests -platform offscreen)
    set_tests_properties(clip-shot-recognition-image-tests PROPERTIES LABELS "unit" TIMEOUT 60)

    if(WIN32 OR APPLE)
        if(APPLE)
            set(_clip_shot_shortcut_platform_label macos)
        else()
            set(_clip_shot_shortcut_platform_label windows)
        endif()
        add_executable(clip-shot-global-shortcut-manager-tests
            tests/global_shortcut_manager_tests.cpp
            ${CLIP_SHOT_GLOBAL_SHORTCUT_SOURCES})
        target_link_libraries(clip-shot-global-shortcut-manager-tests PRIVATE
            clip_shot_global_mouse Qt6::Widgets)
        add_test(NAME clip-shot-global-shortcut-manager-tests
            COMMAND clip-shot-global-shortcut-manager-tests)

        set_tests_properties(clip-shot-global-shortcut-manager-tests PROPERTIES
            LABELS "unit;${_clip_shot_shortcut_platform_label}" TIMEOUT 60)
        add_test(NAME clip-shot-global-shortcut-native-smoke-tests
            COMMAND clip-shot-global-shortcut-manager-tests --native-registration-smoke)
        set_tests_properties(clip-shot-global-shortcut-native-smoke-tests PROPERTIES
            LABELS "interactive;${_clip_shot_shortcut_platform_label}" TIMEOUT 60)
        if(APPLE)
            target_link_libraries(clip-shot-global-shortcut-manager-tests PRIVATE
                "${CLIP_SHOT_CARBON_FRAMEWORK}"
                "${CLIP_SHOT_APPKIT_FRAMEWORK}"
                "${CLIP_SHOT_CORE_GRAPHICS_FRAMEWORK}")
        endif()
        unset(_clip_shot_shortcut_platform_label)
    endif()

    add_executable(clip-shot-global-mouse-tests tests/global_mouse_tests.cpp
        ${CLIP_SHOT_GLOBAL_SHORTCUT_SOURCES}
        src/presentation/capture/screenshotglobalmousedrag.cpp)
    target_link_libraries(clip-shot-global-mouse-tests PRIVATE clip_shot_global_mouse Qt6::Widgets)
    if(APPLE)
        target_link_libraries(clip-shot-global-mouse-tests PRIVATE
            "${CLIP_SHOT_CARBON_FRAMEWORK}"
            "${CLIP_SHOT_APPKIT_FRAMEWORK}"
            "${CLIP_SHOT_CORE_GRAPHICS_FRAMEWORK}")
    endif()
    add_test(NAME clip-shot-global-mouse-tests COMMAND clip-shot-global-mouse-tests)
    add_test(NAME clip-shot-global-mouse-storage-tests
        COMMAND clip-shot-application-storage-tests --global-mouse-only)
    add_test(NAME clip-shot-global-mouse-settings-runtime-tests
        COMMAND clip-shot-settings-runtime-session-tests --global-mouse-only)
    set_tests_properties(clip-shot-global-mouse-storage-tests
        clip-shot-global-mouse-settings-runtime-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    set_tests_properties(clip-shot-global-mouse-tests PROPERTIES
        LABELS "unit" TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    clip_shot_import_offscreen_platform(clip-shot-global-mouse-tests)
    if(WIN32)
        target_sources(clip-shot-global-mouse-tests PRIVATE tests/global_mouse_native_input_tests.cpp)
        add_test(NAME clip-shot-global-mouse-native-smoke-tests
            COMMAND clip-shot-global-mouse-tests --native-hook-smoke)
        set_tests_properties(clip-shot-global-mouse-native-smoke-tests PROPERTIES
            LABELS "interactive;windows" TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=windows")
    elseif(APPLE)
        add_executable(clip-shot-macos-global-mouse-tests tests/macos_global_mouse_tests.mm)
        target_link_libraries(clip-shot-macos-global-mouse-tests PRIVATE clip_shot_global_mouse Qt6::Widgets)
        add_test(NAME clip-shot-macos-global-mouse-tests COMMAND clip-shot-macos-global-mouse-tests)
        set_tests_properties(clip-shot-macos-global-mouse-tests PROPERTIES
            LABELS "unit;macos" TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        add_test(NAME clip-shot-global-mouse-native-smoke-tests
            COMMAND clip-shot-macos-global-mouse-tests --native-hook-smoke)
        set_tests_properties(clip-shot-global-mouse-native-smoke-tests PROPERTIES
            LABELS "interactive;macos" TIMEOUT 30 SKIP_RETURN_CODE 77 ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
    endif()

    add_executable(clip-shot-diagnostics-tests tests/diagnostics_tests.cpp)
    target_link_libraries(clip-shot-diagnostics-tests PRIVATE clip_shot_diagnostics)
    add_test(NAME clip-shot-diagnostics-tests COMMAND clip-shot-diagnostics-tests)
    set_tests_properties(clip-shot-diagnostics-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    if(WIN32)
        set_tests_properties(clip-shot-diagnostics-tests PROPERTIES
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;$ENV{PATH}")
    endif()
    clip_shot_add_diagnostics_crash_tests()

    clip_shot_offscreen_qpa_environment(_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT)

    # Test binaries must report failures as stderr output plus a non-zero
    # exit code instead of blocking on modal dialogs.
    clip_link_test_cli_guard(qt)

    # The image-codec backend smoke test is Qt-free; give it the Qt-free
    # dialog guard so the guard does not add a Qt runtime dependency.
    set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_native)

    clip_shot_add_image_codec_smoke_test()
    set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_qt)
    add_test(NAME clip-shot-image-codec-backend-smoke-tests
             COMMAND clip-shot-image-codec-backend-smoke-tests)

    add_test(NAME clip-shot-arm64-stack-cookie-image-tests
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test-arm64-stack-cookie.py")
    set_tests_properties(clip-shot-arm64-stack-cookie-image-tests PROPERTIES
        LABELS unit TIMEOUT 30)

    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND
       CLIP_WINDOWS_ARCHITECTURE STREQUAL "arm64")
        add_executable(clip-shot-arm64-stack-cookie-tests tests/arm64_stack_cookie_tests.cpp)
        target_link_libraries(clip-shot-arm64-stack-cookie-tests PRIVATE Qt6::Core)
        clip_apply_strict_warnings(clip-shot-arm64-stack-cookie-tests)
        clip_apply_release_options(clip-shot-arm64-stack-cookie-tests POLICY size)
        add_test(NAME clip-shot-arm64-stack-cookie-tests COMMAND clip-shot-arm64-stack-cookie-tests)
        set_tests_properties(clip-shot-arm64-stack-cookie-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 30
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()

    if(WIN32)
        add_executable(clip-shot-storage-directory-change-tests tests/storage_directory_change_tests.cpp)
        target_link_libraries(clip-shot-storage-directory-change-tests PRIVATE clip_shot_storage Qt6::Core)
        add_test(NAME clip-shot-storage-directory-change-tests COMMAND clip-shot-storage-directory-change-tests)
        set_tests_properties(clip-shot-storage-directory-change-tests PROPERTIES LABELS "unit;windows" TIMEOUT 90
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

        add_executable(clip-shot-storage-directory-crash-capture-tests
            tests/storage_directory_crash_capture_tests.cpp)
        target_link_libraries(clip-shot-storage-directory-crash-capture-tests PRIVATE
            clip_shot_storage Qt6::Core)
        target_compile_definitions(clip-shot-storage-directory-crash-capture-tests PRIVATE
            CLIP_TEST_CRASHPAD_HANDLER="${CLIP_CRASHPAD_HANDLER}")
        add_test(NAME clip-shot-storage-directory-crash-capture-tests
            COMMAND clip-shot-storage-directory-crash-capture-tests)
        set_tests_properties(clip-shot-storage-directory-crash-capture-tests PROPERTIES
            LABELS "unit;windows;crash" TIMEOUT 120)
    endif()
    add_executable(clip-shot-application-storage-tests
        tests/application_storage_tests.cpp
    )
    target_link_libraries(clip-shot-application-storage-tests PRIVATE clip_shot_storage Qt6::Core)
    add_test(NAME clip-shot-update-settings-tests
        COMMAND clip-shot-application-storage-tests --update-settings-only)
    set_tests_properties(clip-shot-update-settings-tests PROPERTIES LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(APPLE)
        add_test(NAME clip-shot-storage-startup-directory-tests
            COMMAND clip-shot-application-storage-tests --directory-resolution-only)
        set_tests_properties(clip-shot-storage-startup-directory-tests PROPERTIES
            LABELS "unit;macos" TIMEOUT 30)
    endif()
    add_test(NAME clip-shot-selection-aspect-ratio-settings-tests
        COMMAND clip-shot-application-storage-tests --selection-aspect-ratio-only)
    set_tests_properties(clip-shot-selection-aspect-ratio-settings-tests PROPERTIES LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-recording-post-processing-settings-tests
        COMMAND clip-shot-application-storage-tests --recording-post-processing-only)
    set_tests_properties(clip-shot-recording-post-processing-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-scrolling-interval-settings-tests
        COMMAND clip-shot-application-storage-tests --scrolling-interval-only)
    set_tests_properties(clip-shot-scrolling-interval-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    # Flaky by design: the async policy mutation settles on timers, and
    add_test(NAME clip-shot-pin-to-screen-storage-tests
        COMMAND clip-shot-application-storage-tests --pin-shortcuts-only)
    set_tests_properties(clip-shot-pin-to-screen-storage-tests PROPERTIES LABELS "unit")
    # CPU contention from parallel tests can stretch the settling window.
    # The test must not run in parallel with other tests (RUN_SERIAL), and
    # intermittent failures are tolerated via the retry launcher.
    add_test(NAME clip-shot-application-storage-tests
        COMMAND ${CMAKE_COMMAND}
            -DTEST_COMMAND=$<TARGET_FILE:clip-shot-application-storage-tests>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_flaky_test_with_retry.cmake
    )
    add_test(NAME clip-shot-storage-quit-lifetime-tests
        COMMAND clip-shot-application-storage-tests --quit-lifetime-only)
    add_test(NAME clip-shot-draw-template-storage-tests
        COMMAND clip-shot-application-storage-tests --draw-template-only)
    set_tests_properties(clip-shot-application-storage-tests
        clip-shot-storage-quit-lifetime-tests clip-shot-draw-template-storage-tests PROPERTIES
        RUN_SERIAL TRUE
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-capture-history-repository-tests
        tests/capture_history_repository_tests.cpp
    )

    target_link_libraries(clip-shot-capture-history-repository-tests PRIVATE
        clip_shot_storage
        Qt6::Core
        Qt6::Gui
    )
    add_test(NAME clip-shot-capture-history-repository-tests
             COMMAND clip-shot-capture-history-repository-tests)
    add_test(NAME clip-shot-capture-history-revision-tests
             COMMAND clip-shot-capture-history-repository-tests --revision-only)
    set_tests_properties(clip-shot-capture-history-repository-tests
        clip-shot-capture-history-revision-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-pinned-window-repository-tests
        tests/pinned_window_repository_tests.cpp
    )
    target_link_libraries(clip-shot-pinned-window-repository-tests PRIVATE
        clip_shot_storage
        Qt6::Core
        Qt6::Gui
    )
    add_test(NAME clip-shot-pinned-window-repository-tests
             COMMAND clip-shot-pinned-window-repository-tests)
    set_tests_properties(clip-shot-pinned-window-repository-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-storage-usage-tracker-tests
        tests/storage_usage_tracker_tests.cpp
    )
    target_link_libraries(clip-shot-storage-usage-tracker-tests PRIVATE
        clip_shot_storage
        Qt6::Core
    )
    add_test(NAME clip-shot-storage-usage-tracker-tests
             COMMAND clip-shot-storage-usage-tracker-tests)
    set_tests_properties(clip-shot-storage-usage-tracker-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-form-fields-tests tests/form_fields_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-form-fields-tests PRIVATE clip_shot_settings)
    clip_shot_import_offscreen_platform(clip-shot-form-fields-tests)
    add_test(NAME clip-shot-form-fields-tests COMMAND clip-shot-form-fields-tests)
    set_tests_properties(clip-shot-form-fields-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-text-translation-settings-tests tests/text_translation_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-text-translation-settings-tests PRIVATE clip_shot_settings ant_design_qt Qt6::Widgets Qt6::Network)
    clip_shot_add_translations(clip-shot-text-translation-settings-tests)
    clip_shot_import_offscreen_platform(clip-shot-text-translation-settings-tests)
    add_test(NAME clip-shot-text-translation-settings-tests COMMAND clip-shot-text-translation-settings-tests -platform offscreen)
    set_tests_properties(clip-shot-text-translation-settings-tests PROPERTIES LABELS "unit" TIMEOUT 60)

    add_executable(clip-shot-text-translation-tests tests/text_translation_tests.cpp)
    target_link_libraries(clip-shot-text-translation-tests PRIVATE clip_shot_translation)
    add_test(NAME clip-shot-text-translation-tests COMMAND clip-shot-text-translation-tests)
    set_tests_properties(clip-shot-text-translation-tests PROPERTIES LABELS "unit" TIMEOUT 60)

    add_executable(clip-shot-custom-ai-models-tests tests/custom_ai_models_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-custom-ai-models-tests PRIVATE clip_shot_settings ant_design_qt Qt6::Widgets Qt6::Network)
    clip_shot_add_translations(clip-shot-custom-ai-models-tests)
    clip_shot_import_offscreen_platform(clip-shot-custom-ai-models-tests)
    add_test(NAME clip-shot-custom-ai-models-tests COMMAND clip-shot-custom-ai-models-tests -platform offscreen)
    set_tests_properties(clip-shot-custom-ai-models-tests PROPERTIES TIMEOUT 60)

    add_executable(clip-shot-storage-status-widget-tests
        tests/storage_status_widget_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_include_directories(clip-shot-storage-status-widget-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-storage-status-widget-tests PRIVATE clip_shot_image_codec
        clip_shot_settings
        ant_design_qt
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-storage-status-widget-tests
        COMMAND clip-shot-storage-status-widget-tests
    )
    if(WIN32)
        set_tests_properties(clip-shot-storage-status-widget-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-storage-status-widget-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    clip_shot_import_offscreen_platform(clip-shot-storage-status-widget-tests)
    add_test(NAME clip-shot-annotation-toolbar-editor-tests
        COMMAND clip-shot-storage-status-widget-tests --toolbar-editor-only -platform offscreen)
    set_tests_properties(clip-shot-annotation-toolbar-editor-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-storage-directory-widget-tests
        COMMAND clip-shot-storage-status-widget-tests --storage-directory-only -platform offscreen)
    set_tests_properties(clip-shot-storage-directory-widget-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-configuration-transfer-widget-tests
        tests/configuration_transfer_widget_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_include_directories(clip-shot-configuration-transfer-widget-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-configuration-transfer-widget-tests PRIVATE
        clip_shot_image_codec
        clip_shot_settings
        clip_shot_settings_catalog
        clip_shot_storage
        ant_design_qt
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-configuration-transfer-widget-tests
        COMMAND clip-shot-configuration-transfer-widget-tests
    )
    if(WIN32)
        set_tests_properties(clip-shot-configuration-transfer-widget-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-configuration-transfer-widget-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-pinned-window-management-page-tests tests/pinned_window_management_page_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-pinned-window-management-page-tests PRIVATE clip_shot_settings Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-pinned-window-management-page-tests)
    add_test(NAME clip-shot-pinned-window-management-page-tests COMMAND clip-shot-pinned-window-management-page-tests)
    set_tests_properties(clip-shot-pinned-window-management-page-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-screenshot-history-page-tests
        tests/screenshot_history_page_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_include_directories(clip-shot-screenshot-history-page-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screenshot-history-page-tests PRIVATE clip_shot_image_codec
        clip_shot_settings
        ant_design_qt
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-screenshot-history-page-tests
        COMMAND clip-shot-screenshot-history-page-tests
    )
    add_test(NAME clip-shot-screenshot-history-page-loading-tests
        COMMAND clip-shot-screenshot-history-page-tests --loading-only)
    set_tests_properties(clip-shot-screenshot-history-page-tests
        clip-shot-screenshot-history-page-loading-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(NAME clip-shot-screenshot-history-context-menu-tests
        COMMAND clip-shot-screenshot-history-page-tests --context-menu-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-history-context-menu-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    clip_shot_add_translations(clip-shot-screenshot-history-page-tests)
    clip_shot_import_offscreen_platform(clip-shot-screenshot-history-page-tests)

    add_executable(clip-shot-single-instance-coordinator-tests
        tests/single_instance_coordinator_tests.cpp
        include/clip_shot/app/singleinstancecoordinator.h
        src/app/singleinstancecoordinator.cpp
    )
    target_include_directories(clip-shot-single-instance-coordinator-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-single-instance-coordinator-tests PRIVATE
        Qt6::Core
        Qt6::Network
    )
    add_test(NAME clip-shot-single-instance-coordinator-tests
             COMMAND clip-shot-single-instance-coordinator-tests)
    set_tests_properties(clip-shot-single-instance-coordinator-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_test(NAME clip-shot-storage-boundary-check
        COMMAND ${CMAKE_COMMAND}
            -DCLIP_SHOT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_storage_boundary.cmake
    )

    add_executable(clip-shot-screenshot-result-compositor-tests
        tests/screenshot_result_compositor_tests.cpp
        include/clip_shot/presentation/screenshotresultcompositor.h
        include/clip_shot/presentation/screenshotselectionshadowrenderer.h
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
    )
    target_include_directories(clip-shot-screenshot-result-compositor-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screenshot-result-compositor-tests PRIVATE
        ant_design_qt
        clip_draw_engine_qt_canvas
        Qt6::Widgets
    )
    add_test(NAME clip-shot-screenshot-result-compositor-tests
             COMMAND clip-shot-screenshot-result-compositor-tests)

    add_executable(clip-shot-screenshot-export-service-tests
        tests/screenshot_export_service_tests.cpp
        include/clip_shot/presentation/screenshotclipboardservice.h
        include/clip_shot/presentation/screenshotdisplaysession.h
        include/clip_shot/presentation/screenshotexportservice.h
        include/clip_shot/presentation/screenshotgeometry.h
        include/clip_shot/presentation/screenshotresultcompositor.h
        include/clip_shot/presentation/screenshotselectionshadowrenderer.h
        include/clip_shot/presentation/screenshotselectionpin.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotexportservice.cpp
        src/presentation/services/screenshotselectionpin.cpp
    )
    target_compile_definitions(clip-shot-screenshot-export-service-tests PRIVATE NOMINMAX)
    target_include_directories(clip-shot-screenshot-export-service-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screenshot-export-service-tests PRIVATE clip_shot_image_codec
        clip_shot_storage
        clip_shot_diagnostics
        ant_design_qt
        Qt6::Widgets
        clip_draw_engine_qt_canvas
    )
    if(WIN32)
        target_link_libraries(clip-shot-screenshot-export-service-tests PRIVATE user32)
    endif()
    add_test(NAME clip-shot-screenshot-export-service-tests
             COMMAND clip-shot-screenshot-export-service-tests)
    add_test(NAME clip-shot-fractional-dpi-export-tests
             COMMAND clip-shot-screenshot-export-service-tests --fractional-dpi)
    set_tests_properties(clip-shot-screenshot-export-service-tests
        clip-shot-fractional-dpi-export-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-screenshot-export-service-tests)

    add_executable(clip-shot-screenshot-result-geometry-tests
        tests/screenshot_result_geometry_tests.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
    )
    target_include_directories(clip-shot-screenshot-result-geometry-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screenshot-result-geometry-tests PRIVATE Qt6::Widgets)
    add_test(NAME clip-shot-screenshot-result-geometry-tests
             COMMAND clip-shot-screenshot-result-geometry-tests)

    add_executable(clip-shot-physical-cursor-tests
        tests/physical_cursor_tests.cpp
        include/clip_shot/platform/physicalcursor.h
        src/platform/physicalcursor.cpp
    )
    target_include_directories(clip-shot-physical-cursor-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-physical-cursor-tests PRIVATE Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-physical-cursor-tests)
    if(APPLE)
        target_link_libraries(clip-shot-physical-cursor-tests PRIVATE "-framework CoreGraphics")
    endif()
    if(WIN32)
        target_compile_definitions(clip-shot-physical-cursor-tests PRIVATE NOMINMAX)
        target_link_libraries(clip-shot-physical-cursor-tests PRIVATE user32)
    endif()
    add_test(NAME clip-shot-physical-cursor-tests
             COMMAND clip-shot-physical-cursor-tests)

    if(WIN32)
        add_executable(clip-shot-window-chrome-tests
            tests/window_chrome_tests.cpp
            include/clip_shot/platform/windows/windowchrome.h
            src/platform/windows/windowchrome.cpp
        )
        target_compile_definitions(clip-shot-window-chrome-tests PRIVATE NOMINMAX)
        target_include_directories(clip-shot-window-chrome-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        target_link_libraries(clip-shot-window-chrome-tests PRIVATE
            Qt6::Widgets
            dwmapi
            user32
        )
        add_test(NAME clip-shot-window-chrome-tests
                 COMMAND clip-shot-window-chrome-tests)
        set_tests_properties(clip-shot-window-chrome-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )

        add_executable(clip-shot-window-cursor-refresh-tests
            tests/window_cursor_refresh_tests.cpp
            src/platform/windows/windowchrome.cpp
            src/platform/windows/windowcursorrefresh.h
        )
        target_compile_definitions(clip-shot-window-cursor-refresh-tests PRIVATE NOMINMAX)
        target_include_directories(clip-shot-window-cursor-refresh-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
        )
        target_link_libraries(clip-shot-window-cursor-refresh-tests PRIVATE
            Qt6::Widgets dwmapi user32
        )
        clip_shot_import_offscreen_platform(clip-shot-window-cursor-refresh-tests)
        add_test(NAME clip-shot-window-cursor-refresh-tests
                 COMMAND clip-shot-window-cursor-refresh-tests)
        set_tests_properties(clip-shot-window-cursor-refresh-tests PROPERTIES
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
            TIMEOUT 20
            LABELS "unit;windows"
        )
    endif()

    add_executable(clip-shot-canvas-color-sampler-tests
        tests/screenshot_canvas_color_sampler_tests.cpp
        include/clip_shot/presentation/screenshotcanvascolorsampler.h
        src/presentation/services/screenshotcanvascolorsampler.cpp
    )
    target_include_directories(clip-shot-canvas-color-sampler-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-canvas-color-sampler-tests PRIVATE Qt6::Widgets clip_canvas_image)
    add_test(NAME clip-shot-canvas-color-sampler-tests
             COMMAND clip-shot-canvas-color-sampler-tests)

    add_executable(clip-shot-language-manager-tests
        tests/language_manager_tests.cpp
        include/clip_shot/presentation/languagemanager.h
        src/presentation/services/languagemanager.cpp
    )
    target_include_directories(clip-shot-language-manager-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-language-manager-tests PRIVATE
        ant_design_qt
        Qt6::Widgets
    )
    clip_shot_add_translations(clip-shot-language-manager-tests)
    add_test(
        NAME clip-shot-language-manager-tests
        COMMAND clip-shot-language-manager-tests
    )
    set_tests_properties(clip-shot-language-manager-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-about-page-tests
        tests/about_page_tests.cpp
        include/clip_shot/presentation/mainwindow.h
        include/clip_shot/presentation/components/contentcardwidget.h
        include/clip_shot/presentation/components/maincontentheaderwidget.h
        include/clip_shot/presentation/components/sidebarwidget.h
        include/clip_shot/presentation/components/titlebarwidget.h
        src/presentation/mainwindow.cpp
        src/presentation/components/contentcardwidget.cpp
        src/presentation/components/maincontentheaderwidget.cpp
        src/presentation/components/sidebarwidget.cpp
        src/presentation/components/titlebarwidget.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/platform/windows/windowchrome.cpp
    )
    if(APPLE)
        target_sources(clip-shot-about-page-tests PRIVATE
            src/platform/macos/applicationactivation.mm)
        target_link_libraries(clip-shot-about-page-tests PRIVATE "-framework AppKit")
    endif()
    target_link_libraries(clip-shot-about-page-tests PRIVATE
        clip_shot_settings
        clip_shot_image_codec
    )
    target_compile_definitions(clip-shot-about-page-tests PRIVATE
        CLIP_SHOT_TEST_VERSION="${CLIP_SHOT_VERSION}"
        CLIP_SHOT_TEST_TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}"
        CLIP_SHOT_TEST_WEBSITE_URL="${CLIP_SHOT_WEBSITE_URL}"
        CLIP_SHOT_TEST_PROJECT_URL="${CLIP_SHOT_PROJECT_URL}"
        CLIP_SHOT_TEST_QQ_GROUP_2_URL="${CLIP_SHOT_QQ_GROUP_2_URL}"
        CLIP_SHOT_TEST_QQ_GROUP_3_URL="${CLIP_SHOT_QQ_GROUP_3_URL}"
    )
    add_dependencies(clip-shot-about-page-tests clip_shot_release_translations)
    clip_shot_import_offscreen_platform(clip-shot-about-page-tests)
    add_test(NAME clip-shot-about-page-tests COMMAND clip-shot-about-page-tests)
    target_link_libraries(clip-shot-about-page-tests PRIVATE clip_shot_updates)
    set_tests_properties(clip-shot-about-page-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    get_target_property(_main_window_font_sources clip-shot-about-page-tests SOURCES)
    list(REMOVE_ITEM _main_window_font_sources tests/about_page_tests.cpp)
    add_executable(clip-shot-main-window-font-tests
        tests/main_window_font_tests.cpp
        ${_main_window_font_sources}
    )
    target_link_libraries(clip-shot-main-window-font-tests PRIVATE
        clip_shot_settings clip_shot_image_codec clip_shot_updates)
    if(APPLE)
        target_link_libraries(clip-shot-main-window-font-tests PRIVATE "-framework AppKit")
    endif()
    clip_shot_import_offscreen_platform(clip-shot-main-window-font-tests)
    add_test(NAME clip-shot-main-window-font-tests COMMAND clip-shot-main-window-font-tests)
    set_tests_properties(clip-shot-main-window-font-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT};QT_SCALE_FACTOR=1.5"
    )

    get_target_property(_window_geometry_memory_sources clip-shot-about-page-tests SOURCES)
    list(REMOVE_ITEM _window_geometry_memory_sources tests/about_page_tests.cpp)
    add_executable(clip-shot-window-geometry-memory-tests
        tests/window_geometry_memory_tests.cpp
        ${_window_geometry_memory_sources}
    )
    target_link_libraries(clip-shot-window-geometry-memory-tests PRIVATE
        clip_shot_settings clip_shot_image_codec clip_shot_updates)
    if(APPLE)
        target_link_libraries(clip-shot-window-geometry-memory-tests PRIVATE "-framework AppKit")
    endif()
    clip_shot_import_offscreen_platform(clip-shot-window-geometry-memory-tests)
    add_test(NAME clip-shot-window-geometry-memory-tests
             COMMAND clip-shot-window-geometry-memory-tests)
    set_tests_properties(clip-shot-window-geometry-memory-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
    )

    add_executable(clip-shot-settings-catalog-tests
        tests/settings_catalog_tests.cpp
    )
    target_include_directories(clip-shot-settings-catalog-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-settings-catalog-tests PRIVATE
        clip_shot_settings_search
        Qt6::Core
    )
    add_test(NAME clip-shot-storage-directory-catalog-tests
        COMMAND clip-shot-settings-catalog-tests --storage-directory-only)
    set_tests_properties(clip-shot-storage-directory-catalog-tests PROPERTIES LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-settings-catalog-tests
             COMMAND clip-shot-settings-catalog-tests)
    add_test(NAME clip-shot-ocr-text-options-catalog-tests
             COMMAND clip-shot-settings-catalog-tests --ocr-text-options-only)
    set_tests_properties(clip-shot-ocr-text-options-catalog-tests PROPERTIES LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-settings-catalog-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-main-window-skin-codec-tests tests/main_window_skin_codec_tests.cpp)
    target_link_libraries(clip-shot-main-window-skin-codec-tests PRIVATE clip_shot_image_codec)
    add_test(NAME clip-shot-main-window-skin-codec-tests COMMAND clip-shot-main-window-skin-codec-tests)
    set_tests_properties(clip-shot-main-window-skin-codec-tests PROPERTIES LABELS unit TIMEOUT 60)
    add_test(NAME clip-shot-managed-image-reader-tests
        COMMAND clip-shot-main-window-skin-codec-tests --managed-reader-only)
    set_tests_properties(clip-shot-managed-image-reader-tests PROPERTIES LABELS unit TIMEOUT 60)

    add_executable(clip-shot-main-window-skin-tests tests/main_window_skin_tests.cpp)
    target_link_libraries(clip-shot-main-window-skin-tests PRIVATE
        clip_shot_main_window_skin clip_shot_storage clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-main-window-skin-tests)
    add_test(NAME clip-shot-main-window-skin-tests COMMAND clip-shot-main-window-skin-tests)
    set_tests_properties(clip-shot-main-window-skin-tests PROPERTIES LABELS unit TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")

    add_executable(clip-shot-skin-settings-tests tests/skin_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-skin-settings-tests PRIVATE clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-skin-settings-tests)
    clip_shot_add_translations(clip-shot-skin-settings-tests)
    add_test(NAME clip-shot-skin-settings-tests COMMAND clip-shot-skin-settings-tests)
    set_tests_properties(clip-shot-skin-settings-tests PROPERTIES LABELS unit TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")

    add_executable(clip-shot-app-font-tests
        tests/app_font_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-app-font-tests PRIVATE
        clip_shot_settings
        clip_shot_image_codec
    )
    add_test(NAME clip-shot-app-font-tests
             COMMAND clip-shot-app-font-tests)
    set_tests_properties(clip-shot-app-font-tests PROPERTIES
        TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-app-font-tests)
    clip_shot_add_translations(clip-shot-app-font-tests)

    add_executable(clip-shot-theme-primary-color-tests
        tests/theme_primary_color_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-theme-primary-color-tests PRIVATE
        clip_shot_settings
        clip_shot_image_codec
    )
    add_test(NAME clip-shot-theme-primary-color-tests
             COMMAND clip-shot-theme-primary-color-tests)
    set_tests_properties(clip-shot-theme-primary-color-tests PROPERTIES
        TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-theme-primary-color-tests)
    clip_shot_add_translations(clip-shot-theme-primary-color-tests)

    add_executable(clip-shot-settings-color-commit-tests
        tests/settings_color_commit_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-settings-color-commit-tests PRIVATE
        clip_shot_settings
        clip_shot_image_codec
    )
    add_test(NAME clip-shot-settings-color-commit-tests
             COMMAND clip-shot-settings-color-commit-tests)
    set_tests_properties(clip-shot-settings-color-commit-tests PROPERTIES
        TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-settings-color-commit-tests)

    add_executable(clip-shot-settings-slider-commit-tests
        tests/settings_slider_commit_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-settings-slider-commit-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-settings-slider-commit-tests)
    add_test(NAME clip-shot-settings-slider-commit-tests
        COMMAND clip-shot-settings-slider-commit-tests)
    set_tests_properties(clip-shot-settings-slider-commit-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-settings-page-loading-tests
        tests/settings_page_loading_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-settings-page-loading-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-settings-page-loading-tests)
    clip_shot_add_translations(clip-shot-settings-page-loading-tests)
    add_test(NAME clip-shot-settings-select-tests
        COMMAND clip-shot-settings-page-loading-tests --selects-only)
    set_tests_properties(clip-shot-settings-select-tests PROPERTIES
        TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(NAME clip-shot-settings-page-loading-tests
        COMMAND clip-shot-settings-page-loading-tests)
    set_tests_properties(clip-shot-settings-page-loading-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-settings-navigation-tests
        COMMAND clip-shot-settings-page-loading-tests --navigation-only)
    add_test(NAME clip-shot-action-toolbar-settings-tests
        COMMAND clip-shot-settings-page-loading-tests --action-toolbar-settings-only)
    set_tests_properties(clip-shot-settings-navigation-tests clip-shot-action-toolbar-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-annotation-shortcut-settings-tests
        tests/annotation_shortcut_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-annotation-shortcut-settings-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-annotation-shortcut-settings-tests)
    add_test(NAME clip-shot-annotation-shortcut-settings-tests
             COMMAND clip-shot-annotation-shortcut-settings-tests)
    set_tests_properties(clip-shot-annotation-shortcut-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-screen-recording-shortcut-settings-tests
        tests/screen_recording_shortcut_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-screen-recording-shortcut-settings-tests PRIVATE
        clip_shot_settings
        clip_shot_image_codec
    )
    add_test(NAME clip-shot-screen-recording-shortcut-settings-tests
             COMMAND clip-shot-screen-recording-shortcut-settings-tests)
    set_tests_properties(clip-shot-screen-recording-shortcut-settings-tests PROPERTIES
        TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-screen-recording-shortcut-settings-tests)

    add_executable(clip-shot-local-shortcut-reset-tests
        tests/local_shortcut_reset_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-local-shortcut-reset-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    add_test(NAME clip-shot-local-shortcut-reset-tests COMMAND clip-shot-local-shortcut-reset-tests)
    set_tests_properties(clip-shot-local-shortcut-reset-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    clip_shot_import_offscreen_platform(clip-shot-local-shortcut-reset-tests)

    add_executable(clip-shot-pin-to-screen-shortcut-settings-tests
        tests/pin_to_screen_shortcut_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-pin-to-screen-shortcut-settings-tests PRIVATE
        clip_shot_settings
        clip_shot_image_codec
    )
    add_test(NAME clip-shot-pin-to-screen-shortcut-settings-tests
             COMMAND clip-shot-pin-to-screen-shortcut-settings-tests)
    set_tests_properties(clip-shot-pin-to-screen-shortcut-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-pin-to-screen-shortcut-settings-tests)

    add_executable(clip-shot-translation-settings-tests
        tests/translation_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-translation-settings-tests PRIVATE
        clip_shot_settings
        clip_shot_image_codec
    )
    add_test(NAME clip-shot-translation-settings-tests COMMAND clip-shot-translation-settings-tests)
    add_test(NAME clip-shot-text-detection-processing-settings-tests
        COMMAND clip-shot-translation-settings-tests --text-detection-processing-only -platform offscreen)
    set_tests_properties(clip-shot-text-detection-processing-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-translation-settings-tests PROPERTIES
        TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-translation-settings-tests)

    add_executable(clip-shot-settings-runtime-session-tests
        tests/settings_runtime_session_tests.cpp
        include/clip_shot/presentation/settings/settingsruntimesession.h
        include/clip_shot/presentation/settings/settingsbackend.h
        src/presentation/settings/settingsruntimesession.cpp
    )
    target_include_directories(clip-shot-settings-runtime-session-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-settings-runtime-session-tests PRIVATE
        clip_shot_global_mouse
        clip_shot_settings_catalog
        clip_shot_storage
        Qt6::Core
        Qt6::Gui
    )
    add_test(NAME clip-shot-settings-runtime-session-tests
             COMMAND clip-shot-settings-runtime-session-tests)
    set_tests_properties(clip-shot-settings-runtime-session-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-configuration-archive-tests
        tests/configuration_archive_tests.cpp
    )
    target_include_directories(clip-shot-configuration-archive-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-configuration-archive-tests PRIVATE
        clip_shot_storage
        Qt6::Core
        Qt6::Gui
    )
    add_test(NAME clip-shot-configuration-archive-tests
             COMMAND clip-shot-configuration-archive-tests)
    add_test(NAME clip-shot-selection-aspect-ratio-archive-tests
        COMMAND clip-shot-configuration-archive-tests --selection-aspect-ratio-only)
    set_tests_properties(clip-shot-selection-aspect-ratio-archive-tests PROPERTIES LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-configuration-archive-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-button-border-tests
        tests/button_border_tests.cpp
        src/presentation/styles/buttonborder.cpp
    )
    target_include_directories(clip-shot-button-border-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-button-border-tests PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-button-border-tests COMMAND clip-shot-button-border-tests)
    set_tests_properties(clip-shot-button-border-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-global-mouse-page-tests
        tests/global_mouse_page_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_link_libraries(clip-shot-global-mouse-page-tests PRIVATE
        clip_shot_settings
        Qt6::Widgets
    )
    add_test(NAME clip-shot-global-mouse-page-tests
             COMMAND clip-shot-global-mouse-page-tests)
    set_tests_properties(clip-shot-global-mouse-page-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    clip_shot_import_offscreen_platform(clip-shot-global-mouse-page-tests)

    add_executable(clip-shot-permission-guide-tests tests/permission_guide_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-permission-guide-tests PRIVATE clip_shot_settings)
    clip_shot_add_translations(clip-shot-permission-guide-tests)
    add_test(NAME clip-shot-permission-guide-tests COMMAND clip-shot-permission-guide-tests)
    set_tests_properties(clip-shot-permission-guide-tests PROPERTIES
        LABELS "unit" TIMEOUT 60 ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    clip_shot_import_offscreen_platform(clip-shot-permission-guide-tests)

    add_executable(clip-shot-app-permissions-tests tests/app_permissions_tests.cpp
        src/app/featureavailability.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-app-permissions-tests PRIVATE clip_shot_settings)
    add_test(NAME clip-shot-app-permissions-tests COMMAND clip-shot-app-permissions-tests)
    set_tests_properties(clip-shot-app-permissions-tests PROPERTIES
        LABELS "unit" ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    clip_shot_import_offscreen_platform(clip-shot-app-permissions-tests)

    if(APPLE)
        add_executable(clip-shot-macos-permission-guide-tests MACOSX_BUNDLE
            tests/macos_permission_guide_tests.mm
            src/presentation/services/screenshotclipboardservice.cpp)
        target_link_libraries(clip-shot-macos-permission-guide-tests PRIVATE
            clip_shot_settings "-framework AppKit")
        clip_shot_add_translations(clip-shot-macos-permission-guide-tests)
        set_target_properties(clip-shot-macos-permission-guide-tests PROPERTIES
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.clipshot.tests.permission-guide"
            MACOSX_BUNDLE_BUNDLE_NAME "Clip Permission Guide")
        add_test(NAME clip-shot-macos-permission-guide-tests
            COMMAND clip-shot-macos-permission-guide-tests)
        set_tests_properties(clip-shot-macos-permission-guide-tests PROPERTIES
            LABELS "interactive;macos" TIMEOUT 30 RUN_SERIAL TRUE)
        add_executable(clip-shot-macos-smart-selection-smoke
            tests/macos_smart_selection_smoke_test.mm
            src/platform/macos/windowcaptureexclusion.mm)
        target_include_directories(clip-shot-macos-smart-selection-smoke PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector-c/include")
        target_link_libraries(clip-shot-macos-smart-selection-smoke PRIVATE
            clip_ui_selector_c Qt6::Widgets "-framework AppKit")
        add_test(NAME clip-shot-macos-smart-selection-smoke COMMAND clip-shot-macos-smart-selection-smoke)
        set_tests_properties(clip-shot-macos-smart-selection-smoke PROPERTIES
            LABELS "interactive;macos" SKIP_RETURN_CODE 77 TIMEOUT 30 RUN_SERIAL TRUE)
    endif()

    add_executable(clip-shot-selector-policy-tests
        tests/screenshot_selector_policy_tests.cpp
        src/presentation/selector/screenshotselectorpolicy.h
    )
    target_include_directories(clip-shot-selector-policy-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/selector"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector-c/include"
    )
    target_link_libraries(clip-shot-selector-policy-tests PRIVATE
        Qt6::Core
    )
    add_test(NAME clip-shot-selector-policy-tests
             COMMAND clip-shot-selector-policy-tests)
    set_tests_properties(clip-shot-selector-policy-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-window-element-api-tests
        tests/window_element_api_tests.cpp
        include/clip_shot/presentation/screenshotselectorcoordinator.h
        src/presentation/selector/screenshotselectorcoordinator.cpp
        src/presentation/selector/screenshotselectorserviceclient.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    clip_shot_alias_test_rust_ffi(clip-shot-window-element-api-tests
        clip_ui_selector_service_create
        clip_ui_selector_service_destroy
        clip_ui_selector_service_invalidate_refinement
        clip_ui_selector_service_query
        clip_ui_selector_service_refine
        clip_ui_selector_service_refresh
        clip_ui_selector_service_refresh_with_displays
        clip_ui_selector_service_release_cache)
    target_include_directories(clip-shot-window-element-api-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector-c/include"
    )
    target_link_libraries(clip-shot-window-element-api-tests PRIVATE
        clip_shot_settings
        Qt6::Widgets
    )
    if(APPLE)
        target_link_libraries(clip-shot-window-element-api-tests PRIVATE
            "-framework CoreGraphics" "-framework AppKit")
    endif()
    add_test(NAME clip-shot-window-element-api-tests
             COMMAND clip-shot-window-element-api-tests)
    add_test(NAME clip-shot-selector-phased-tests
             COMMAND clip-shot-window-element-api-tests --selector-only)
    add_test(NAME clip-shot-shortcut-exit-confirmation-settings-tests
             COMMAND clip-shot-window-element-api-tests
                     --shortcut-exit-confirmation-settings-only)
    add_test(NAME clip-shot-auto-qr-settings-tests
             COMMAND clip-shot-window-element-api-tests --auto-qr-settings-only)
    add_test(NAME clip-shot-screenshot-guide-settings-reset-tests
             COMMAND clip-shot-window-element-api-tests --guide-settings-only)
    clip_shot_import_offscreen_platform(clip-shot-window-element-api-tests)
    set_tests_properties(
        clip-shot-window-element-api-tests
        clip-shot-selector-phased-tests
        clip-shot-shortcut-exit-confirmation-settings-tests
        clip-shot-auto-qr-settings-tests
        clip-shot-screenshot-guide-settings-reset-tests
        PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-system-tray-controller-tests
        tests/system_tray_controller_tests.cpp
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/pinnedwindowgroupmanager.h
        include/clip_shot/presentation/systemtraycontroller.h
        src/presentation/services/languagemanager.cpp
        src/presentation/services/pinnedwindowgroupmanager.cpp
        src/presentation/services/systemtraycontroller.cpp
    )
    target_include_directories(clip-shot-system-tray-controller-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CLIP_SHOT_DRAW_ENGINE_QT_DIR}/include"
    )
    target_link_libraries(clip-shot-system-tray-controller-tests PRIVATE
        clip_shot_system_notification_controller
        clip_shot_image_codec
        ant_design_qt
        clip_shot_settings_catalog
        Qt6::Widgets
    )
    # Match deployment: ICO decoding must not depend on the optional Qt plugin.
    qt_import_plugins(clip-shot-system-tray-controller-tests EXCLUDE Qt6::QICOPlugin)
    qt_add_resources(clip-shot-system-tray-controller-tests "clip_shot_tray_test_assets"
        PREFIX "/clip-shot/app-icons"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-default.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-light.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-dark.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-default.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-light.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-dark.png"
    )
    clip_shot_add_translations(clip-shot-system-tray-controller-tests)
    add_test(
        NAME clip-shot-system-tray-controller-tests
        COMMAND clip-shot-system-tray-controller-tests
    )
    add_test(NAME clip-shot-tray-skin-tests
        COMMAND clip-shot-system-tray-controller-tests --skin-only -platform offscreen)
    add_test(NAME clip-shot-recording-export-tray-tests
        COMMAND clip-shot-system-tray-controller-tests --recording-export-only -platform offscreen)
    set_tests_properties(clip-shot-recording-export-tray-tests PROPERTIES LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-tray-group-refresh-tests
        COMMAND clip-shot-system-tray-controller-tests --group-refresh-only -platform offscreen)
    add_test(NAME clip-shot-tray-menu-position-tests
        COMMAND clip-shot-system-tray-controller-tests --menu-position-only -platform offscreen)
    clip_shot_import_offscreen_platform(clip-shot-system-tray-controller-tests)
    set_tests_properties(clip-shot-tray-skin-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-tray-group-refresh-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-tray-menu-position-tests PROPERTIES
        LABELS "unit" TIMEOUT 30 CLIP_TEST_CROSS_PLATFORM TRUE
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(WIN32)
        set_tests_properties(clip-shot-system-tray-controller-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-system-tray-controller-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    if(APPLE)
        target_link_libraries(clip-shot-system-tray-controller-tests PRIVATE
            clip_shot_system_notifications)
        target_sources(clip-shot-system-tray-controller-tests PRIVATE
            src/platform/macos/systemtraymenu.mm
            tests/system_tray_menu_native_tests.mm)
        target_link_libraries(clip-shot-system-tray-controller-tests PRIVATE "-framework AppKit")
        add_test(NAME clip-shot-system-tray-menu-native-tests
            COMMAND clip-shot-system-tray-controller-tests --native-menu -platform cocoa)
        set_tests_properties(clip-shot-system-tray-menu-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 30)
    endif()

    add_executable(clip-shot-window-group-switcher-tests
        tests/window_group_switcher_tests.cpp
        ${CLIP_SHOT_GLOBAL_SHORTCUT_SOURCES}
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
        include/clip_shot/presentation/windowgroupswitchercontroller.h
        include/clip_shot/presentation/windowgroupswitcherpopup.h
        include/clip_shot/presentation/pinnedwindowgroupmanager.h
        include/clip_shot/presentation/languagemanager.h
        src/presentation/services/windowgroupswitchercontroller.cpp
        src/presentation/components/windowgroupswitcherpopup.cpp
        src/presentation/services/pinnedwindowgroupmanager.cpp
        src/presentation/services/languagemanager.cpp)
    target_include_directories(clip-shot-window-group-switcher-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CLIP_SHOT_DRAW_ENGINE_QT_DIR}/include")
    target_link_libraries(clip-shot-window-group-switcher-tests PRIVATE clip_shot_global_mouse ant_design_qt Qt6::Widgets)
    if(APPLE)
        set_source_files_properties(tests/window_group_switcher_tests.cpp PROPERTIES LANGUAGE OBJCXX)
        target_link_libraries(clip-shot-window-group-switcher-tests PRIVATE
            "${CLIP_SHOT_CARBON_FRAMEWORK}" "${CLIP_SHOT_APPKIT_FRAMEWORK}" "${CLIP_SHOT_CORE_GRAPHICS_FRAMEWORK}")
        add_test(NAME clip-shot-window-group-switcher-native-tests
            COMMAND clip-shot-window-group-switcher-tests -platform cocoa)
        set_tests_properties(clip-shot-window-group-switcher-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    if(WIN32)
        add_test(NAME clip-shot-window-group-switcher-native-tests
            COMMAND clip-shot-window-group-switcher-tests --native -platform windows)
        set_tests_properties(clip-shot-window-group-switcher-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 60 SKIP_RETURN_CODE 77)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-window-group-switcher-tests)
    clip_shot_add_translations(clip-shot-window-group-switcher-tests)
    add_test(NAME clip-shot-window-group-switcher-tests COMMAND clip-shot-window-group-switcher-tests)
    set_tests_properties(clip-shot-window-group-switcher-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS unit)

    add_executable(clip-shot-pinned-window-group-tests
        tests/pinned_window_group_tests.cpp
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/pinnedwindowgroupmanager.h
        src/presentation/services/languagemanager.cpp
        src/presentation/services/pinnedwindowgroupmanager.cpp
    )
    target_include_directories(clip-shot-pinned-window-group-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CLIP_SHOT_DRAW_ENGINE_QT_DIR}/include"
    )
    target_link_libraries(clip-shot-pinned-window-group-tests PRIVATE
        ant_design_qt
        clip_shot_storage
        Qt6::Widgets
    )
    clip_shot_add_translations(clip-shot-pinned-window-group-tests)
    add_test(
        NAME clip-shot-pinned-window-group-tests
        COMMAND clip-shot-pinned-window-group-tests
    )
    set_tests_properties(clip-shot-pinned-window-group-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    if(APPLE)
        clip_find_qt(COMPONENTS Test)
        add_executable(clip-shot-action-popup-native-tests tests/action_popup_menu_native_tests.mm)
        target_include_directories(clip-shot-action-popup-native-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(clip-shot-action-popup-native-tests PRIVATE
            ant_design_qt clip_shot_shortcuts Qt6::Test "-framework AppKit")
        add_test(NAME clip-shot-action-popup-native-tests COMMAND clip-shot-action-popup-native-tests)
        set_tests_properties(clip-shot-action-popup-native-tests PROPERTIES
            LABELS "interactive"
            ENVIRONMENT "QT_QPA_PLATFORM=cocoa"
            TIMEOUT 20)
    endif()

    add_executable(clip-shot-context-menu-rendering-tests
        tests/context_menu_rendering_tests.cpp
    )
    target_link_libraries(clip-shot-context-menu-rendering-tests PRIVATE
        ant_design_qt
        Qt6::Widgets
    )
    clip_shot_import_offscreen_platform(clip-shot-context-menu-rendering-tests)
    add_test(
        NAME clip-shot-context-menu-rendering-tests
        COMMAND clip-shot-context-menu-rendering-tests
    )
    if(WIN32)
        set_tests_properties(clip-shot-context-menu-rendering-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-context-menu-rendering-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-window-shortcut-manager-tests
        tests/window_shortcut_manager_tests.cpp
    )
    add_executable(clip-shot-mouse-release-action-tests tests/mouse_release_action_tests.cpp)
    target_link_libraries(clip-shot-mouse-release-action-tests PRIVATE clip_shot_window_shortcuts)
    clip_shot_import_offscreen_platform(clip-shot-mouse-release-action-tests)
    add_test(NAME clip-shot-mouse-release-action-tests
        COMMAND clip-shot-mouse-release-action-tests -platform offscreen)
    set_tests_properties(clip-shot-mouse-release-action-tests PROPERTIES LABELS "unit")
    target_link_libraries(clip-shot-window-shortcut-manager-tests PRIVATE
        clip_shot_window_shortcuts
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-window-shortcut-manager-tests
        COMMAND clip-shot-window-shortcut-manager-tests
    )
    set_tests_properties(clip-shot-window-shortcut-manager-tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=$<IF:$<BOOL:${WIN32}>,windows,offscreen>"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-shortcut-key-row-tests
        tests/shortcut_key_row_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_include_directories(clip-shot-shortcut-key-row-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-shortcut-key-row-tests PRIVATE
        clip_shot_settings
        Qt6::Widgets
    )
    clip_shot_import_offscreen_platform(clip-shot-shortcut-key-row-tests)
    add_test(
        NAME clip-shot-shortcut-key-row-tests
        COMMAND clip-shot-shortcut-key-row-tests
    )
    set_tests_properties(clip-shot-shortcut-key-row-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        add_test(
            NAME clip-shot-shortcut-key-row-native-tests
            COMMAND clip-shot-shortcut-key-row-tests --native-print-screen-only
        )
        set_tests_properties(clip-shot-shortcut-key-row-native-tests PROPERTIES
            LABELS "interactive;windows"
            RUN_SERIAL TRUE
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-screenshot-shortcut-hints-tests
        tests/screenshot_shortcut_hints_tests.cpp
        include/clip_shot/presentation/screenshotshortcuthints.h
    )
    target_include_directories(clip-shot-screenshot-shortcut-hints-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screenshot-shortcut-hints-tests PRIVATE
        clip_draw_engine_qt_canvas
        clip_shot_storage
        Qt6::Core
    )
    add_test(
        NAME clip-shot-screenshot-shortcut-hints-tests
        COMMAND clip-shot-screenshot-shortcut-hints-tests
    )
    set_tests_properties(clip-shot-screenshot-shortcut-hints-tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=$<IF:$<BOOL:${WIN32}>,windows,offscreen>"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-form-layout-tests
        tests/form_layout_tests.cpp
        include/clip_shot/presentation/screenshotselectionresizemodalcontent.h
        src/presentation/components/icons/clipshoticons.cpp
        src/presentation/selection/screenshotselectionresizemodalcontent.cpp
        src/presentation/selection/screenshotselectionparams.cpp
    )
    target_link_libraries(clip-shot-form-layout-tests PRIVATE
        ant_design_qt
        Qt6::Widgets
    )
    target_include_directories(clip-shot-form-layout-tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(
        NAME clip-shot-form-layout-tests
        COMMAND clip-shot-form-layout-tests
    )
    set_tests_properties(clip-shot-form-layout-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-selection-resize-workflow-tests
        tests/screenshot_selection_resize_workflow_tests.cpp
        include/clip_shot/presentation/screenshotselectioneditworkflowports.h
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/screenshotselectionresizemodalcontent.h
        include/clip_shot/presentation/screenshotselectionresizeworkflow.h
        include/clip_shot/presentation/screenshotselectionsettingsstore.h
        src/presentation/components/icons/clipshoticons.cpp
        src/presentation/services/languagemanager.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/selection/screenshotselectionresizemodalcontent.cpp
        src/presentation/selection/screenshotselectionresizeworkflow.cpp
        src/presentation/selection/screenshotselectionsettingsstore.cpp
    )
    target_include_directories(clip-shot-selection-resize-workflow-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-selection-resize-workflow-tests PRIVATE
        ant_design_qt
        Qt6::Widgets
    )
    clip_shot_add_translations(clip-shot-selection-resize-workflow-tests)
    add_test(
        NAME clip-shot-selection-resize-workflow-tests
        COMMAND clip-shot-selection-resize-workflow-tests
    )
    set_tests_properties(clip-shot-selection-resize-workflow-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-selection-effects-tests
        tests/screenshot_selection_effects_tests.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotinteractionstate.cpp
        src/presentation/selection/screenshotselectioneditworkflow.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/selection/screenshotselectionsettingsstore.cpp
    )
    target_link_libraries(clip-shot-selection-effects-tests PRIVATE clip_shot_storage Qt6::Widgets)
    add_test(NAME clip-shot-selection-effects-tests COMMAND clip-shot-selection-effects-tests)
    add_executable(clip-shot-direct-capture-workflow-tests
        tests/direct_capture_workflow_tests.cpp
        src/presentation/capture/directcaptureworkflow.cpp
    )
    target_include_directories(clip-shot-direct-capture-workflow-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-direct-capture-workflow-tests PRIVATE
        clip_shot_image_codec
        Qt6::Gui
    )
    add_test(NAME clip-shot-direct-capture-workflow-tests
        COMMAND clip-shot-direct-capture-workflow-tests)

    if(APPLE)
        add_executable(clip-shot-camera-shutter-sound-tests
            tests/camera_shutter_sound_tests.mm
            src/presentation/capture/camerashuttersound.cpp
            src/platform/macos/camerashuttersound.mm)
        target_include_directories(clip-shot-camera-shutter-sound-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/capture")
        target_link_libraries(clip-shot-camera-shutter-sound-tests PRIVATE
            Qt6::Core "-framework AVFoundation" "-framework Foundation" objc)
        qt_add_resources(clip-shot-camera-shutter-sound-tests "clip_shot_shutter_audio_tests"
            PREFIX "/clip-shot"
            BASE "resources"
            FILES "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}")
        add_test(NAME clip-shot-camera-shutter-sound-tests
            COMMAND clip-shot-camera-shutter-sound-tests)
        set_tests_properties(clip-shot-camera-shutter-sound-tests PROPERTIES
            LABELS "unit" TIMEOUT 30)
    endif()

    add_executable(clip-shot-direct-capture-frame-tests tests/direct_capture_frame_tests.cpp)
    target_include_directories(clip-shot-direct-capture-frame-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/capture"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-recording-c/include")
    target_link_libraries(clip-shot-direct-capture-frame-tests PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-direct-capture-frame-tests COMMAND clip-shot-direct-capture-frame-tests)

    add_executable(clip-shot-direct-capture-native-tests
        tests/direct_capture_native_tests.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/capture/directcapturenative.cpp)
    clip_shot_alias_test_rust_ffi(clip-shot-direct-capture-native-tests
        clip_capture_desktop_session_capture
        clip_capture_desktop_session_create
        clip_capture_desktop_session_destroy
        clip_capture_frame_lease_release
        clip_capture_last_error_message
        clip_capture_screenshot_result_destroy
        clip_capture_screenshot_result_display_count
        clip_capture_screenshot_result_display_geometry
        clip_capture_screenshot_result_display_info
        clip_capture_screenshot_result_display_retain
        clip_capture_screenshot_result_focused_window_geometry
        clip_capture_screenshot_result_focused_window_info
        clip_capture_screenshot_result_focused_window_retain)
    target_include_directories(clip-shot-direct-capture-native-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/capture"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-recording-c/include")
    target_link_libraries(clip-shot-direct-capture-native-tests PRIVATE clip_shot_storage
        clip_shot_image_codec Qt6::Gui)
    add_test(NAME clip-shot-direct-capture-native-tests COMMAND clip-shot-direct-capture-native-tests)

    if(WIN32)
        # Explicit opt-in: captures a visible fixture and its actual monitor.
        add_executable(clip-shot-direct-capture-native-smoke
            tests/direct_capture_native_smoke.cpp
            src/presentation/capture/directcapturenative.cpp)
        target_compile_definitions(clip-shot-direct-capture-native-smoke PRIVATE NOMINMAX)
        target_include_directories(clip-shot-direct-capture-native-smoke PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/capture"
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-recording-c/include")
        target_link_libraries(clip-shot-direct-capture-native-smoke PRIVATE
            Qt6::Widgets clip_shot_image_codec clip_capture_c clip_recording_c PkgConfig::CLIP_SHOT_FFMPEG
            d3d11 dxgi dxguid dwmapi gdi32 ole32 oleaut32 runtimeobject
            user32 userenv winmm windowsapp ws2_32 advapi32 bcrypt psapi)
    endif()

    add_executable(clip-shot-capture-worker-tests
        tests/screenshot_capture_worker_tests.cpp
        include/clip_shot/presentation/screenshotcapturecoordinator.h
        src/presentation/capture/screenshotcapturecoordinator.cpp
        src/presentation/capture/screenshotcaptureworker.cpp
        src/presentation/capture/screenshotcapturepolicy.cpp
    )
    clip_shot_alias_test_rust_ffi(clip-shot-capture-worker-tests
        clip_capture_cancellation_token_cancel
        clip_capture_cancellation_token_create
        clip_capture_cancellation_token_destroy
        clip_capture_cursor_snapshot_create
        clip_capture_cursor_snapshot_destroy
        clip_capture_desktop_layout_count
        clip_capture_desktop_layout_destroy
        clip_capture_desktop_layout_display
        clip_capture_desktop_session_capture
        clip_capture_desktop_session_capture_with_layout
        clip_capture_desktop_session_create
        clip_capture_desktop_session_destroy
        clip_capture_desktop_session_layout_snapshot
        clip_capture_desktop_session_prepare
        clip_capture_desktop_session_refresh_layout
        clip_capture_desktop_session_reset_to_prepared
        clip_capture_desktop_session_state
        clip_capture_frame_lease_release
        clip_capture_last_error_message
        clip_capture_screenshot_result_composite_cursor
        clip_capture_screenshot_result_cursor_patch
        clip_capture_screenshot_result_destroy
        clip_capture_screenshot_result_display_count
        clip_capture_screenshot_result_display_geometry
        clip_capture_screenshot_result_display_info
        clip_capture_screenshot_result_display_retain
        clip_capture_screenshot_result_focused_window_geometry)
    target_include_directories(clip-shot-capture-worker-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/capture"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-recording-c/include")
    target_link_libraries(clip-shot-capture-worker-tests PRIVATE clip_shot_storage Qt6::Gui)
    add_test(NAME clip-shot-capture-worker-tests COMMAND clip-shot-capture-worker-tests)

    if(WIN32)
        add_executable(clip-shot-recapture-cursor-native-e2e-test
            tests/recapture_cursor_native_e2e_test.cpp
            include/clip_shot/presentation/screenshotcapturecoordinator.h
            src/presentation/capture/screenshotcapturecoordinator.cpp
            src/presentation/capture/screenshotcaptureworker.cpp
            src/presentation/capture/screenshotcapturepolicy.cpp
            src/platform/windows/windowchrome.cpp)
        target_compile_definitions(clip-shot-recapture-cursor-native-e2e-test PRIVATE NOMINMAX)
        target_include_directories(clip-shot-recapture-cursor-native-e2e-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/capture"
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include")
        target_link_libraries(clip-shot-recapture-cursor-native-e2e-test PRIVATE
            clip_shot_storage Qt6::Widgets clip_capture_c PkgConfig::CLIP_SHOT_FFMPEG
            d3d11 dxgi dxguid dwmapi gdi32 ole32 oleaut32 runtimeobject
            user32 userenv winmm windowsapp ws2_32 advapi32 bcrypt psapi)
        add_test(NAME clip-shot-recapture-cursor-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native)
        add_test(NAME clip-shot-recapture-cursor-movement-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --move-during-refresh)
        add_test(NAME clip-shot-recapture-cursor-snapshot-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --change-after-ready)
        add_test(NAME clip-shot-recapture-class-cursor-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --native-target)
        add_test(NAME clip-shot-recapture-cursor-hidden-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --hide-overlay)
        add_test(NAME clip-shot-recapture-cursor-own-window-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --same-thread)
        add_test(NAME clip-shot-recapture-cursor-unchanged-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --same-thread --same-cursor)
        add_test(NAME clip-shot-recapture-cursor-own-hidden-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --same-thread --hide-overlay)
        add_test(NAME clip-shot-recapture-cursor-own-titlebar-native-e2e-test
                 COMMAND clip-shot-recapture-cursor-native-e2e-test --run-native --same-thread --nonclient)
        set_tests_properties(clip-shot-recapture-cursor-native-e2e-test
            clip-shot-recapture-cursor-movement-native-e2e-test
            clip-shot-recapture-cursor-snapshot-native-e2e-test
            clip-shot-recapture-class-cursor-native-e2e-test
            clip-shot-recapture-cursor-hidden-native-e2e-test
            clip-shot-recapture-cursor-own-window-native-e2e-test
            clip-shot-recapture-cursor-unchanged-native-e2e-test
            clip-shot-recapture-cursor-own-hidden-native-e2e-test
            clip-shot-recapture-cursor-own-titlebar-native-e2e-test PROPERTIES
            LABELS "interactive;e2e;windows"
            RUN_SERIAL TRUE
            TIMEOUT 60
            ENVIRONMENT "QT_QPA_PLATFORM=windows")
    endif()

    add_executable(clip-shot-capture-workflow-tests
        tests/screenshot_capture_workflow_tests.cpp
        src/presentation/toolbar/screenshottoolbarpresentationstatefactory.cpp
        src/presentation/selector/screenshotselectorworkflow.cpp
        include/clip_shot/presentation/screenshotcapturedisplaymodelreconciler.h
        include/clip_shot/presentation/screenshotcapturestate.h
        include/clip_shot/presentation/screenshotcaptureworkflow.h
        include/clip_shot/presentation/screenshotcaptureworkflowports.h
        include/clip_shot/presentation/screenshotdisplaysession.h
        include/clip_shot/presentation/screenshotgeometry.h
        include/clip_shot/presentation/screenshotintelligentselectionmodel.h
        include/clip_shot/presentation/screenshotinteractionstate.h
        include/clip_shot/presentation/screenshotselectiongeometry.h
        include/clip_shot/presentation/screenshotselectionmodel.h
        include/clip_shot/presentation/screenshotselectionparams.h
        src/presentation/capture/screenshotcapturedisplaymodelreconciler.cpp
        src/presentation/capture/screenshotcaptureworkflow.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotintelligentselectionmodel.cpp
        src/presentation/core/screenshotinteractionstate.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
    )
    target_include_directories(clip-shot-capture-workflow-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-capture-workflow-tests PRIVATE
        Qt6::Gui
    )
    add_test(
        NAME clip-shot-capture-workflow-tests
        COMMAND clip-shot-capture-workflow-tests
    )

    add_executable(clip-shot-capture-modal-closer-tests
        tests/screenshot_capture_modal_closer_tests.cpp
        src/presentation/capture/screenshotcapturemodalcloser.h
    )
    target_link_libraries(clip-shot-capture-modal-closer-tests PRIVATE Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-capture-modal-closer-tests)
    add_test(NAME clip-shot-capture-modal-closer-tests
        COMMAND clip-shot-capture-modal-closer-tests)
    set_tests_properties(clip-shot-capture-modal-closer-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-tool-command-workflow-tests
        tests/screenshot_tool_command_workflow_tests.cpp
        include/clip_shot/presentation/screenshotcapturestate.h
        include/clip_shot/presentation/screenshotdisplaysession.h
        include/clip_shot/presentation/screenshotgeometry.h
        include/clip_shot/presentation/screenshotintelligentselectionmodel.h
        include/clip_shot/presentation/screenshotinteractionstate.h
        include/clip_shot/presentation/screenshotselectiongeometry.h
        include/clip_shot/presentation/screenshotselectionmodel.h
        include/clip_shot/presentation/screenshotselectionparams.h
        include/clip_shot/presentation/screenshottoolcommandworkflow.h
        include/clip_shot/presentation/screenshottoolcommandworkflowports.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotintelligentselectionmodel.cpp
        src/presentation/core/screenshotinteractionstate.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/tools/screenshottoolcommandworkflow.cpp
    )
    target_include_directories(clip-shot-tool-command-workflow-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-tool-command-workflow-tests PRIVATE
        Qt6::Widgets
        clip_draw_engine_qt_canvas
    )
    add_test(
        NAME clip-shot-tool-command-workflow-tests
        COMMAND clip-shot-tool-command-workflow-tests
    )

    add_executable(clip-shot-smart-selection-transition-tests
        tests/screenshot_smart_selection_transition_tests.cpp
        include/clip_shot/presentation/screenshotsmartselectiontransition.h
        src/presentation/core/screenshotsmartselectiontransition.cpp
    )
    target_include_directories(clip-shot-smart-selection-transition-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-smart-selection-transition-tests PRIVATE
        Qt6::Core
    )
    add_test(
        NAME clip-shot-smart-selection-transition-tests
        COMMAND clip-shot-smart-selection-transition-tests
    )

    add_executable(clip-shot-multi-region-tests
        tests/screenshot_multi_region_tests.cpp
        src/presentation/core/screenshotsmartselectiontransition.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotinteractionstate.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/storage/persistedselectioncodec.cpp
    )
    target_include_directories(clip-shot-multi-region-tests PRIVATE include)
    target_link_libraries(clip-shot-multi-region-tests PRIVATE
        Qt6::Gui clip_draw_engine_qt_canvas ant_design_qt)
    if(WIN32)
        target_link_libraries(clip-shot-multi-region-tests PRIVATE user32)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-multi-region-tests)
    add_test(NAME clip-shot-multi-region-tests COMMAND clip-shot-multi-region-tests)
    add_test(NAME clip-shot-region-overlay-input-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --region-input-only -platform offscreen)
    set_tests_properties(clip-shot-region-overlay-input-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-region-shape-input-tests COMMAND clip-shot-screenshot-history-tests --region-shapes-only)
    set_tests_properties(clip-shot-region-shape-input-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit")
    set_tests_properties(clip-shot-multi-region-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}" LABELS "unit")

    add_executable(clip-shot-selection-geometry-tests
        tests/screenshot_selection_geometry_tests.cpp
        include/clip_shot/platform/windows/monitorgeometry.h
        include/clip_shot/presentation/historypinplacement.h
        include/clip_shot/presentation/screenshotgeometry.h
        include/clip_shot/presentation/screenshotresultcompositor.h
        include/clip_shot/presentation/screenshotselectionpin.h
        include/clip_shot/presentation/screenshotselectiongeometry.h
        include/clip_shot/presentation/screenshotselectionmodel.h
        include/clip_shot/presentation/screenshotselectionparams.h
        include/clip_shot/presentation/screenshotselectionshadowrenderer.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/services/historypinplacement.cpp
        src/presentation/services/screenshotselectionpin.cpp
    )
    target_include_directories(clip-shot-selection-geometry-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-selection-geometry-tests PRIVATE
        clip_canvas_image
        ant_design_qt
        Qt6::Gui
    )
    if(WIN32)
        target_link_libraries(clip-shot-selection-geometry-tests PRIVATE user32)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-selection-geometry-tests)
    add_test(
        NAME clip-shot-selection-geometry-tests
        COMMAND clip-shot-selection-geometry-tests
    )
    set_tests_properties(clip-shot-selection-geometry-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        # Re-runs the binary on the real Windows platform, where the native monitor query
        # inside historyPinDesktopUsesNativeMonitorRects executes; the offscreen pass
        # skips that query by design.
        add_test(
            NAME clip-shot-selection-geometry-windows-desktop-tests
            COMMAND clip-shot-selection-geometry-tests
        )
        set_tests_properties(clip-shot-selection-geometry-windows-desktop-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
            LABELS "unit;windows"
        )
        add_test(NAME clip-shot-selection-geometry-windows-fractional-tests
            COMMAND clip-shot-selection-geometry-tests)
        set_tests_properties(clip-shot-selection-geometry-windows-fractional-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_SCALE_FACTOR=1.5;QT_ENABLE_HIGHDPI_SCALING=1;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
            LABELS "unit;windows"
        )
    endif()

    add_executable(clip-shot-selection-export-workflow-tests
        tests/screenshot_selection_export_workflow_tests.cpp
        include/clip_shot/presentation/screenshotclipboardservice.h
        include/clip_shot/presentation/screenshotselectionexportworkflow.h
        include/clip_shot/presentation/screenshotselectionexportworkflowports.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/selection/screenshotselectionexportworkflow.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_include_directories(clip-shot-selection-export-workflow-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-selection-export-workflow-tests PRIVATE
        clip_shot_diagnostics
        clip_shot_image_codec
        Qt6::Gui
    )
    if(WIN32)
        target_link_libraries(clip-shot-selection-export-workflow-tests PRIVATE user32)
    endif()
    add_test(NAME clip-shot-selection-export-workflow-tests
             COMMAND clip-shot-selection-export-workflow-tests)
    set_tests_properties(clip-shot-selection-export-workflow-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-screenshot-color-picker-tests
        tests/screenshot_color_picker_tests.cpp
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/services/screenshotcolorpickerwindow.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
    )
    clip_shot_link_macos_window_platform(clip-shot-screenshot-color-picker-tests)
    target_include_directories(clip-shot-screenshot-color-picker-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screenshot-color-picker-tests PRIVATE
        clip_shot_storage clip_draw_engine_qt_canvas ant_design_qt Qt6::Widgets
    )
    clip_shot_import_offscreen_platform(clip-shot-screenshot-color-picker-tests)
    if(APPLE)
        set_source_files_properties(tests/screenshot_color_picker_tests.cpp
            PROPERTIES LANGUAGE OBJCXX)
        target_link_libraries(clip-shot-screenshot-color-picker-tests PRIVATE "-framework AppKit")
        add_test(NAME clip-shot-macos-color-picker-animation-native-tests
            COMMAND clip-shot-screenshot-color-picker-tests --magnifier-animation-only -platform cocoa)
        set_tests_properties(clip-shot-macos-color-picker-animation-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 30)
    endif()
    add_test(NAME clip-shot-screenshot-color-picker-tests
        COMMAND clip-shot-screenshot-color-picker-tests)
    set_tests_properties(clip-shot-screenshot-color-picker-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-screenshot-history-tests
        tests/screenshot_history_tests.cpp
        src/presentation/selector/screenshotselectorworkflow.cpp
        src/presentation/capture/directcapturehistory.cpp
        include/clip_shot/presentation/screenshothistoryservice.h
        include/clip_shot/presentation/screenshotdisplaysession.h
        include/clip_shot/presentation/screenshotgeometry.h
        include/clip_shot/presentation/screenshotintelligentselectionmodel.h
        include/clip_shot/presentation/screenshotinteractionstate.h
        include/clip_shot/presentation/screenshotoverlayinputhandler.h
        include/clip_shot/presentation/screenshotoverlayshortcutcontroller.h
        include/clip_shot/presentation/screenshotshortcutexitconfirmation.h
        include/clip_shot/presentation/screenshotselectiongeometry.h
        include/clip_shot/presentation/screenshotselectionmodel.h
        include/clip_shot/presentation/screenshotselectionparams.h
        src/presentation/services/screenshothistoryservice.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/services/screenshotshortcutexitconfirmation.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotintelligentselectionmodel.cpp
        src/presentation/core/screenshotinteractionstate.cpp
        src/presentation/overlay/screenshotoverlayinputhandler.cpp
        src/presentation/overlay/screenshotoverlayshortcutcontroller.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
    )
    target_include_directories(clip-shot-screenshot-history-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
    )
    target_link_libraries(clip-shot-screenshot-history-tests PRIVATE
        clip_draw_engine_qt_canvas
        clip_shot_image_codec
        clip_shot_window_shortcuts
        ant_design_qt
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-screenshot-history-tests
        COMMAND clip-shot-screenshot-history-tests
    )
    add_test(
        NAME clip-shot-direct-capture-history-tests
        COMMAND clip-shot-screenshot-history-tests --direct-capture-only
    )
    add_test(
        NAME clip-shot-history-metadata-lifecycle-tests
        COMMAND clip-shot-screenshot-history-tests --history-metadata-lifecycle-only
    )
    add_test(
        NAME clip-shot-selection-border-resize-tests
        COMMAND clip-shot-screenshot-history-tests --selection-border-resize-only
    )
    add_test(NAME clip-shot-selection-aspect-ratio-confirmation-tests
        COMMAND clip-shot-screenshot-history-tests --selection-aspect-ratio-only)
    add_test(NAME clip-shot-screenshot-eraser-wheel-tests
        COMMAND clip-shot-screenshot-history-tests --eraser-wheel-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-eraser-wheel-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    clip_shot_import_offscreen_platform(clip-shot-screenshot-history-tests)
    add_test(
        NAME clip-shot-screenshot-completion-gesture-tests
        COMMAND clip-shot-screenshot-history-tests --completion-gestures-only
    )
    set_tests_properties(
        clip-shot-direct-capture-history-tests
        clip-shot-history-metadata-lifecycle-tests
        clip-shot-selection-border-resize-tests
        clip-shot-selection-aspect-ratio-confirmation-tests
        clip-shot-screenshot-completion-gesture-tests
        PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(
        NAME clip-shot-screenshot-shortcut-priority-tests
        COMMAND clip-shot-screenshot-history-tests --shortcut-input-only
    )
    add_test(NAME clip-shot-global-screenshot-shortcut-tests
        COMMAND clip-shot-screenshot-history-tests --global-screenshot-shortcuts-only -platform offscreen)
    set_tests_properties(clip-shot-global-screenshot-shortcut-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-screenshot-exit-confirmation-tests
        COMMAND clip-shot-screenshot-history-tests --shortcut-exit-confirmation-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-exit-confirmation-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(
        NAME clip-shot-intelligent-selection-cursor-shortcut-tests
        COMMAND clip-shot-screenshot-history-tests
                --intelligent-selection-cursor-shortcut-only
    )
    add_test(NAME clip-shot-screenshot-cursor-shortcut-tests
        COMMAND clip-shot-screenshot-history-tests --cursor-shortcuts-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-cursor-shortcut-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(
        NAME clip-shot-canvas-color-sampling-tests
        COMMAND clip-shot-screenshot-history-tests --canvas-color-sampling-only
    )
    set_tests_properties(
        clip-shot-screenshot-history-tests
        clip-shot-screenshot-shortcut-priority-tests
        clip-shot-intelligent-selection-cursor-shortcut-tests
        clip-shot-canvas-color-sampling-tests
        PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=$<IF:$<BOOL:${WIN32}>,windows,offscreen>"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-scrolling-preview-benchmark
        tests/scrolling_preview_benchmark.cpp
        include/clip_shot/presentation/screenshotscrollingtypes.h
        include/clip_shot/presentation/screenshotscrollingthumbnailwidget.h
        src/presentation/capture/screenshotscrollingthumbnailwidget.cpp
    )
    target_compile_definitions(clip-shot-scrolling-preview-benchmark PRIVATE
        CLIP_SHOT_BENCH_INTERNALS=1
    )
    target_include_directories(clip-shot-scrolling-preview-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-scrolling-preview-benchmark PRIVATE
        clip_canvas_image
        ant_design_qt
        Qt6::Widgets
    )
    clip_shot_import_offscreen_platform(clip-shot-scrolling-preview-benchmark)

    add_executable(clip-shot-scrolling-result-async-benchmark
        tests/scrolling_result_async_benchmark.cpp
    )
    target_include_directories(clip-shot-scrolling-result-async-benchmark PRIVATE
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-stitch-images-c/include"
    )
    target_link_libraries(clip-shot-scrolling-result-async-benchmark PRIVATE
        clip_stitch_images_c
        clip_shot_image_codec
        Qt6::Widgets
    )
    if(WIN32)
        target_link_libraries(clip-shot-scrolling-result-async-benchmark PRIVATE
            Ntdll
            Ws2_32
        )
    endif()

    # This test is intentionally Qt-free; swap in the Qt-free dialog guard so
    # linking the guard does not add a Qt runtime dependency.
    set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_native)
    add_executable(clip-shot-latest-frame-mailbox-tests
        tests/latest_frame_mailbox_tests.cpp
        src/presentation/capture/latestframemailbox.h
    )
    set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_qt)
    target_include_directories(clip-shot-latest-frame-mailbox-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    add_test(
        NAME clip-shot-latest-frame-mailbox-tests
        COMMAND clip-shot-latest-frame-mailbox-tests
    )

    add_executable(clip-shot-scrolling-capture-cadence-tests
        tests/scrolling_capture_cadence_tests.cpp
        src/presentation/capture/adaptivescrollingcapturecadence.h
    )
    add_executable(clip-shot-scrolling-selection-movement-tests
        tests/scrolling_selection_movement_tests.cpp)
    target_include_directories(clip-shot-scrolling-selection-movement-tests PRIVATE include src)
    target_link_libraries(clip-shot-scrolling-selection-movement-tests PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME clip-shot-scrolling-selection-movement-tests
        COMMAND clip-shot-scrolling-selection-movement-tests)

    add_executable(clip-shot-scrolling-auto-scroll-tests
        tests/scrolling_auto_scroll_tests.cpp
        src/platform/windows/scrollinput.cpp
    )
    target_include_directories(clip-shot-scrolling-auto-scroll-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-scrolling-auto-scroll-tests PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME clip-shot-scrolling-auto-scroll-tests
        COMMAND clip-shot-scrolling-auto-scroll-tests)
    target_include_directories(clip-shot-scrolling-capture-cadence-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    add_test(
        NAME clip-shot-scrolling-capture-cadence-tests
        COMMAND clip-shot-scrolling-capture-cadence-tests
    )

    add_executable(clip-shot-screenshot-canvas-renderer-tests
        tests/screenshot_canvas_renderer_tests.cpp
        tests/screenshot_selection_effect_editor_tests.cpp
        src/presentation/overlay/screenshotoverlayinputhandler.cpp
        src/presentation/overlay/screenshotoverlayinteractionadapter.cpp
        src/presentation/overlay/screenshotoverlayshortcutcontroller.cpp
        include/clip_shot/presentation/screenshotoverlayshortcutcontroller.h
        src/presentation/selection/screenshotselectioneditworkflow.cpp
        src/presentation/selection/screenshotselectionsettingsstore.cpp
        tests/screenshot_cursor_tests.cpp
        src/presentation/services/screenshotsourceimagecomposer.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
        include/clip_shot/presentation/screenshothistoryservice.h
        src/presentation/capture/directcapturehistory.cpp
        src/presentation/services/screenshothistoryservice.cpp
        src/presentation/core/screenshotintelligentselectionmodel.cpp
        src/presentation/core/screenshotinteractionstate.cpp
        include/clip_shot/presentation/screenshotocrtextlayer.h
        include/clip_shot/presentation/screenshotmessageservice.h
        include/clip_shot/presentation/screenshotoverlaywindow.h
        include/clip_shot/presentation/screenshotoverlaypool.h
        include/clip_shot/presentation/screenshotscrollingtypes.h
        include/clip_shot/presentation/screenshotscrollingthumbnailwidget.h
        src/presentation/capture/screenshotscrollingthumbnailwidget.cpp
        src/presentation/capture/screenshotcapturedisplaymodelreconciler.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        src/presentation/overlay/screenshotcanvasrenderer.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/overlay/screenshotoverlaycanvaspresenter.cpp
        src/presentation/overlay/screenshotoverlayframepresenter.cpp
        src/presentation/overlay/screenshotoverlayframepresenter.h
        src/presentation/overlay/screenshotoverlaycanvaswidget.cpp
        src/presentation/overlay/screenshotoverlaypool.cpp
        src/presentation/overlay/screenshotoverlaywindow.cpp
        src/presentation/services/screenshotmessageservice.cpp
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp
        src/presentation/ocr/screenshottabledocument.cpp
    )
    if(CLIP_APPS_BUILD_BENCHMARKS)
        target_sources(clip-shot-screenshot-canvas-renderer-tests PRIVATE
            tests/screenshot_cursor_performance_benchmark.cpp)
        target_compile_definitions(clip-shot-screenshot-canvas-renderer-tests PRIVATE
            CLIP_SHOT_CURSOR_BENCHMARK=1)
        if(WIN32)
            target_link_libraries(clip-shot-screenshot-canvas-renderer-tests PRIVATE Psapi)
        endif()
        add_test(NAME clip-shot-screenshot-cursor-benchmark
            COMMAND clip-shot-screenshot-canvas-renderer-tests --cursor-benchmark -platform offscreen)
        set_tests_properties(clip-shot-screenshot-cursor-benchmark PROPERTIES LABELS "benchmark")
    endif()
    add_test(NAME clip-shot-screenshot-cursor-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --cursor-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-cursor-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-selection-shortcut-hint-stages-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --shortcut-hints-only -platform offscreen)
    set_tests_properties(clip-shot-selection-shortcut-hint-stages-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-selection-effect-editor-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --selection-effect-editors-only -platform offscreen)
    set_tests_properties(clip-shot-selection-effect-editor-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-selection-effect-preview-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --selection-effect-preview-only -platform offscreen)
    set_tests_properties(clip-shot-selection-effect-preview-tests PROPERTIES LABELS "unit")
    clip_shot_link_macos_window_platform(clip-shot-screenshot-canvas-renderer-tests)
    target_include_directories(clip-shot-screenshot-canvas-renderer-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_compile_definitions(clip-shot-screenshot-canvas-renderer-tests PRIVATE
        CLIP_SHOT_BENCH_INTERNALS=1
    )
    target_link_libraries(clip-shot-screenshot-canvas-renderer-tests PRIVATE
        clip_shot_diagnostics
        ant_design_qt
        Qt6::Widgets
        Qt6::GuiPrivate
        clip_shot_storage
        clip_shot_window_shortcuts
        clip_draw_engine_qt_canvas
    )
    add_test(
        NAME clip-shot-screenshot-canvas-renderer-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests
    )
    add_test(NAME clip-shot-screenshot-guide-preferences-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --screenshot-ui-preferences)
    add_test(NAME clip-shot-screenshot-guide-rendering-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --guide-rendering-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-guide-rendering-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-direct-capture-history-rendering-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --direct-capture-history-rendering)
    add_test(NAME clip-shot-pinned-filter-reference-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --pinned-filter-reference-only -platform offscreen)
    add_test(NAME clip-shot-fractional-dpi-rendering-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --fractional-dpi)
    add_test(NAME clip-shot-pinned-result-scaling-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --pinned-result-scaling -platform offscreen)
    set_tests_properties(clip-shot-pinned-result-scaling-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-fractional-dpi-rendering-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(NAME clip-shot-scrolling-overlay-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --scrolling-overlay -platform offscreen)
    if(WIN32)
        add_test(NAME clip-shot-scrolling-overlay-native-tests
            COMMAND clip-shot-screenshot-canvas-renderer-tests --scrolling-overlay -platform windows)
        set_tests_properties(clip-shot-scrolling-overlay-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 60
            ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    add_test(NAME clip-shot-overlay-safe-area-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --overlay-safe-area -platform offscreen)
    add_test(NAME clip-shot-overlay-window-shadow-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --window-shadow-only -platform offscreen)
    set_tests_properties(clip-shot-overlay-window-shadow-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-overlay-boundary-input-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --overlay-boundary-input -platform offscreen)
    set_tests_properties(clip-shot-overlay-boundary-input-tests PROPERTIES LABELS "unit")
    if(APPLE)
        target_sources(clip-shot-screenshot-canvas-renderer-tests PRIVATE
            ../cmake/test-support/macos_native_input.mm)
        target_include_directories(clip-shot-screenshot-canvas-renderer-tests PRIVATE
            ../cmake/test-support)
        add_test(NAME clip-shot-macos-overlay-window-shadow-native-tests
            COMMAND clip-shot-screenshot-canvas-renderer-tests --window-shadow-only -platform cocoa)
        set_tests_properties(clip-shot-macos-overlay-window-shadow-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 30)
        add_test(NAME clip-shot-macos-overlay-boundary-input-native-tests
            COMMAND clip-shot-screenshot-canvas-renderer-tests --overlay-boundary-input -platform cocoa)
        set_tests_properties(clip-shot-macos-overlay-boundary-input-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 30)
    endif()
    set_tests_properties(clip-shot-scrolling-overlay-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-overlay-close-release-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --close-release-only -platform offscreen)
    set_tests_properties(clip-shot-direct-capture-history-rendering-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(
        NAME clip-shot-ocr-canvas-renderer-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --ocr-presentation
    )
    add_test(
        NAME clip-shot-overlay-pool-prewarm-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --overlay-pool-prewarm
    )
    set_tests_properties(
        clip-shot-screenshot-canvas-renderer-tests
        clip-shot-screenshot-guide-preferences-tests
        clip-shot-screenshot-guide-rendering-tests
        clip-shot-overlay-pool-prewarm-tests
        PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-ocr-canvas-renderer-tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        add_test(
            NAME clip-shot-overlay-native-surface-retirement-test
            COMMAND clip-shot-screenshot-canvas-renderer-tests
                --overlay-native-surface-retirement
        )
        set_tests_properties(clip-shot-overlay-native-surface-retirement-test PROPERTIES
            LABELS "e2e;interactive;windows"
            RUN_SERIAL TRUE
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-overlay-frame-presenter-tests
        tests/screenshot_overlay_frame_presenter_tests.cpp
        src/presentation/overlay/screenshotoverlayframepresenter.cpp
        src/presentation/overlay/screenshotoverlayframepresenter.h
    )
    target_include_directories(clip-shot-overlay-frame-presenter-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/overlay"
    )
    target_compile_definitions(clip-shot-overlay-frame-presenter-tests PRIVATE
        CLIP_SHOT_BENCH_INTERNALS=1
    )
    target_link_libraries(clip-shot-overlay-frame-presenter-tests PRIVATE
        Qt6::Widgets
    )
    clip_shot_import_offscreen_platform(clip-shot-overlay-frame-presenter-tests)
    add_test(
        NAME clip-shot-overlay-frame-presenter-tests
        COMMAND clip-shot-overlay-frame-presenter-tests
    )
    set_tests_properties(clip-shot-overlay-frame-presenter-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        target_link_libraries(clip-shot-overlay-frame-presenter-tests PRIVATE dwmapi)
        add_test(
            NAME clip-shot-overlay-frame-presenter-native-reveal-test
            COMMAND clip-shot-overlay-frame-presenter-tests --native-reveal
        )
        set_tests_properties(clip-shot-overlay-frame-presenter-native-reveal-test PROPERTIES
            LABELS "e2e;interactive;windows"
            RUN_SERIAL TRUE
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-pinned-geometry-mapping-tests
        tests/screenshot_pinned_geometry_mapping_tests.cpp)
    target_include_directories(clip-shot-pinned-geometry-mapping-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/pinned")
    target_link_libraries(clip-shot-pinned-geometry-mapping-tests PRIVATE Qt6::Core)
    add_test(NAME clip-shot-pinned-geometry-mapping-tests COMMAND clip-shot-pinned-geometry-mapping-tests)
    set_tests_properties(clip-shot-pinned-geometry-mapping-tests PROPERTIES
        LABELS "unit" CLIP_TEST_CROSS_PLATFORM TRUE)

    add_executable(clip-shot-pinned-resize-geometry-tests
        tests/screenshot_pinned_resize_geometry_tests.cpp
        src/presentation/pinned/screenshotpinnedresizegeometry.cpp
    )
    target_include_directories(clip-shot-pinned-resize-geometry-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/pinned"
    )
    target_link_libraries(clip-shot-pinned-resize-geometry-tests PRIVATE
        Qt6::Core
    )
    add_test(
        NAME clip-shot-pinned-resize-geometry-tests
        COMMAND clip-shot-pinned-resize-geometry-tests
    )

    add_executable(clip-shot-pinned-native-geometry-controller-tests
        tests/screenshot_pinned_native_geometry_controller_tests.cpp
        src/presentation/pinned/screenshotpinnednativegeometrycontroller.cpp
        src/presentation/pinned/screenshotpinnedresizegeometry.cpp
    )
    target_include_directories(clip-shot-pinned-native-geometry-controller-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/pinned"
    )
    target_link_libraries(clip-shot-pinned-native-geometry-controller-tests PRIVATE
        Qt6::Core
    )
    add_test(
        NAME clip-shot-pinned-native-geometry-controller-tests
        COMMAND clip-shot-pinned-native-geometry-controller-tests
    )

    add_executable(clip-shot-pinned-restore-geometry-tests
        tests/screenshot_pinned_restore_geometry_tests.cpp
        src/presentation/pinned/screenshotpinnedrestoregeometry.cpp
    )
    target_include_directories(clip-shot-pinned-restore-geometry-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/pinned"
    )
    target_link_libraries(clip-shot-pinned-restore-geometry-tests PRIVATE
        Qt6::Core
    )
    add_test(
        NAME clip-shot-pinned-restore-geometry-tests
        COMMAND clip-shot-pinned-restore-geometry-tests
    )

    add_executable(clip-shot-pinned-window-tests
        tests/screenshot_pinned_hide_to_top_tests.cpp
        src/presentation/services/screenshotfilepinbatch.cpp
        tests/screenshot_pinned_window_tests.cpp
        tests/original_image_translation_tests.cpp
        include/clip_shot/presentation/components/pathinput.h
        src/presentation/components/pathinput.cpp
        include/clip_shot/presentation/screenshotsavepreviewcanvas.h
        src/presentation/services/screenshotsaveasfiledialog.cpp
        src/presentation/services/screenshotsaveexportpipeline.cpp
        src/presentation/services/screenshotsavepreviewcanvas.cpp
        src/platform/physicalcursor.cpp
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenshotcanvascolorsampler.h
        include/clip_shot/presentation/screenshotcanvascolorsamplerwindow.h
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/screenshotpinnededitcontroller.h
        include/clip_shot/presentation/screenshotpinnedwindow.h
        include/clip_shot/presentation/pinnedwindowgroupmanager.h
        include/clip_shot/presentation/screenshotrecognitionsessioncontroller.h
        include/clip_shot/presentation/screenshotrecognitionwindow.h
        include/clip_shot/presentation/screenshottableeditor.h
        include/clip_shot/presentation/screenshotmessageservice.h
        include/clip_shot/presentation/screenshotselectiongeometry.h
        include/clip_shot/presentation/screenshotselectionmodel.h
        include/clip_shot/presentation/screenshotselectionparams.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp
        src/presentation/ocr/screenshotocrtexteditingsession.cpp
        src/presentation/ocr/screenshotocrtexttransform.cpp
        src/presentation/ocr/screenshotrecognitionsessioncontroller.cpp
        src/presentation/ocr/screenshotocrlayout.cpp
        src/presentation/ocr/screenshotrecognitionwindow.cpp
        src/presentation/ocr/screenshottabledocument.cpp
        src/presentation/ocr/screenshottableeditor.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        src/presentation/overlay/screenshotcanvasrenderer.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/services/screenshotcanvascolorsampler.cpp
        src/presentation/services/screenshotcanvascolorsamplerwindow.cpp
        src/presentation/services/screenshotclipboardcontent.cpp
        src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotrecognitionfileexport.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/services/screenshotmessageservice.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/pinned/screenshotpinnededitcontroller.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
        src/presentation/services/screenshotexportartifact.cpp
        src/presentation/services/screenshotexportpinnedsource.cpp
        src/presentation/ocr/screenshotrecognitionimage.cpp
        src/presentation/selection/screenshotselectionexportuiservices.cpp
        src/presentation/pinned/screenshotpinnednativegeometrycontroller.cpp
        src/presentation/pinned/screenshotpinnedclickthroughgeometry.cpp
        src/presentation/pinned/screenshotpinnedresizegeometry.cpp
        src/presentation/pinned/screenshotpinnedrestoregeometry.cpp
        src/presentation/pinned/screenshotpinnedhidetotopcontroller.cpp
        src/presentation/pinned/screenshotpinnedwindow.cpp
        src/presentation/services/pinnedwindowgroupmanager.cpp
        src/presentation/pinned/screenshotpinnedinteraction.cpp
        include/clip_shot/presentation/pinnedwindowselectioncontroller.h
        src/presentation/pinned/pinnedwindowselectioncontroller.cpp
        src/presentation/pinned/pinnedwindowselectioninteraction.cpp
        src/presentation/pinned/pinnedwindowselectiongeometry.cpp
        src/presentation/pinned/screenshotpinneddragexport.cpp
        ${CLIP_PINNED_PLATFORM_SOURCES}
        src/presentation/services/languagemanager.cpp
        src/presentation/styles/buttonborder.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
    )
    clip_shot_link_macos_window_platform(clip-shot-pinned-window-tests)
    target_include_directories(clip-shot-pinned-window-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_compile_definitions(clip-shot-pinned-window-tests PRIVATE
        CLIP_SHOT_TEST_HOOKS=1
        CLIP_SHOT_TEST_PINNED_TRANSLATION=1
    )
    target_link_libraries(clip-shot-pinned-window-tests PRIVATE
        clip_shot_translation
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        Qt6::Network
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        clip_shot_image_codec
        clip_shot_storage
        clip_shot_window_shortcuts
    )
    if(WIN32)
        target_link_libraries(clip-shot-pinned-window-tests PRIVATE
            Qt6::GuiPrivate
            comctl32
            dwmapi
            user32
            psapi
        )
    endif()
    add_test(
        NAME clip-shot-pinned-window-tests
        COMMAND clip-shot-pinned-window-tests
    )
    set_tests_properties(clip-shot-pinned-window-tests PROPERTIES
        LABELS "interactive;e2e;windows" RUN_SERIAL TRUE TIMEOUT 180
        ENVIRONMENT "QT_QPA_PLATFORM=windows")
    clip_shot_import_offscreen_platform(clip-shot-pinned-window-tests)
    add_test(NAME clip-shot-pinned-restored-text-theme-tests
        COMMAND clip-shot-pinned-window-tests --restored-text-theme-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-restored-text-theme-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-file-pin-presentation-tests
        COMMAND clip-shot-pinned-window-tests --file-pin-batch-only)
    set_tests_properties(clip-shot-file-pin-presentation-tests PROPERTIES
        LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-management-lifecycle-tests
        COMMAND clip-shot-pinned-window-tests --management-only -platform offscreen)
    add_test(NAME clip-shot-pinned-close-persistence-tests
        COMMAND clip-shot-pinned-window-tests --deferred-user-close-only -platform offscreen)
    add_test(NAME clip-shot-pinned-window-confirmation-tests
        COMMAND clip-shot-pinned-window-tests --window-confirmation-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-window-confirmation-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-pinned-close-persistence-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-transaction-lifetime-tests
        COMMAND clip-shot-pinned-window-tests --pin-lifetime-only -platform offscreen)
    add_test(NAME clip-shot-pinned-pixel-lifetime-tests
        COMMAND clip-shot-pinned-window-tests --pixel-lifetime-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-pixel-lifetime-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-pinned-transaction-lifetime-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-duplicate-pin-tests
        COMMAND clip-shot-pinned-window-tests --duplicate-pin-only -platform offscreen)
    set_tests_properties(clip-shot-duplicate-pin-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    set_tests_properties(clip-shot-pinned-management-lifecycle-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-hide-to-top-tests
        COMMAND clip-shot-pinned-window-tests --hide-to-top-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-hide-to-top-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-hide-to-top-native-tests
        COMMAND clip-shot-pinned-window-tests --hide-to-top-native-only -platform windows)
    set_tests_properties(clip-shot-pinned-hide-to-top-native-tests PROPERTIES LABELS "unit;windows"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-lock-tests
        COMMAND clip-shot-pinned-window-tests --lock-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-lock-tests PROPERTIES LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
    if(WIN32)
        add_test(NAME clip-shot-pinned-lock-native-tests
            COMMAND clip-shot-pinned-window-tests --lock-native-only -platform windows)
        set_tests_properties(clip-shot-pinned-lock-native-tests PROPERTIES
            LABELS "unit;windows" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    add_test(NAME clip-shot-pinned-click-through-tests
        COMMAND clip-shot-pinned-window-tests --click-through-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-click-through-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-always-on-top-tests
        COMMAND clip-shot-pinned-window-tests --always-on-top-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-always-on-top-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-show-border-tests
        COMMAND clip-shot-pinned-window-tests --show-border-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-show-border-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-transparent-edges-tests
        COMMAND clip-shot-pinned-window-tests --transparent-edges-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-transparent-edges-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-checkerboard-tests
        COMMAND clip-shot-pinned-window-tests --checkerboard-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-checkerboard-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-eraser-tests
        COMMAND clip-shot-pinned-window-tests --eraser-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-eraser-tests PROPERTIES LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-remembered-drawing-tool-tests
        COMMAND clip-shot-pinned-window-tests --remembered-drawing-tool-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-remembered-drawing-tool-tests PROPERTIES LABELS "unit"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-click-through-native-tests
        COMMAND clip-shot-pinned-window-tests --click-through-native-only -platform windows)
    set_tests_properties(clip-shot-pinned-click-through-native-tests PROPERTIES
        LABELS "interactive;e2e;windows" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-image-processing-shortcut-tests
        COMMAND clip-shot-pinned-window-tests --image-processing-shortcuts-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-image-processing-shortcut-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-pinned-shortcut-display-tests
        COMMAND clip-shot-pinned-window-tests --shortcut-display-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-shortcut-display-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-pinned-menu-language-tests
        COMMAND clip-shot-pinned-window-tests --menu-language-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-menu-language-tests PROPERTIES
        LABELS "unit" TIMEOUT 30 CLIP_TEST_CROSS_PLATFORM TRUE
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-destroy-shortcut-tests
        COMMAND clip-shot-pinned-window-tests --pinned-destroy-shortcut-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-destroy-shortcut-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-pinned-resize-window-tool-tests
        COMMAND clip-shot-pinned-window-tests --resize-window-tool-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-resize-window-tool-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 90)
    if(WIN32)
        add_test(NAME clip-shot-pinned-resize-window-native-tests
            COMMAND clip-shot-pinned-window-tests --resize-window-native-only -platform windows)
        set_tests_properties(clip-shot-pinned-resize-window-native-tests PROPERTIES
            LABELS "unit;windows" RUN_SERIAL TRUE TIMEOUT 120)
    endif()
    foreach(_thumbnail_scenario IN ITEMS appearance dpi snapshot copy)
        add_test(NAME clip-shot-pinned-thumbnail-${_thumbnail_scenario}-tests
            COMMAND clip-shot-pinned-window-tests --thumbnail-${_thumbnail_scenario}-only -platform offscreen)
        set_tests_properties(clip-shot-pinned-thumbnail-${_thumbnail_scenario}-tests PROPERTIES
            TIMEOUT 30 RUN_SERIAL TRUE
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endforeach()
    set_tests_properties(clip-shot-pinned-thumbnail-copy-tests PROPERTIES
        ENVIRONMENT "QT_SCALE_FACTOR=2")
    add_test(NAME clip-shot-pinned-thumbnail-copy-fractional-tests
        COMMAND clip-shot-pinned-window-tests --thumbnail-copy-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-thumbnail-copy-fractional-tests PROPERTIES
        TIMEOUT 30 RUN_SERIAL TRUE ENVIRONMENT "QT_SCALE_FACTOR=1.25"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-thumbnail-background-tests
        COMMAND clip-shot-pinned-window-tests --thumbnail-background-only)
    add_test(NAME clip-shot-pinned-thumbnail-recreation-tests
        COMMAND clip-shot-pinned-window-tests --thumbnail-recreation-only)
    add_test(NAME clip-shot-pinned-thumbnail-reentry-tests
        COMMAND clip-shot-pinned-window-tests --thumbnail-reentry-only)
    add_test(NAME clip-shot-pinned-middle-click-tests
        COMMAND clip-shot-pinned-window-tests --middle-click-only)
    add_test(NAME clip-shot-pinned-middle-click-offscreen-tests
        COMMAND clip-shot-pinned-window-tests --middle-click-only)
    set_tests_properties(clip-shot-pinned-middle-click-offscreen-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_test(NAME clip-shot-pinned-double-click-tests
        COMMAND clip-shot-pinned-window-tests --double-click-only)
    add_test(NAME clip-shot-pinned-double-click-offscreen-tests
        COMMAND clip-shot-pinned-window-tests --double-click-only)
    set_tests_properties(clip-shot-pinned-double-click-offscreen-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_test(NAME clip-shot-pinned-text-double-click-tests
        COMMAND clip-shot-pinned-window-tests --text-double-click-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-text-double-click-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-save-dialog-tests
        COMMAND clip-shot-pinned-window-tests --save-dialog-only)
    set_tests_properties(clip-shot-pinned-save-dialog-tests PROPERTIES TIMEOUT 90)
    add_test(NAME clip-shot-pinned-style-wheel-tests
        COMMAND clip-shot-pinned-window-tests --arrow-label-wheel-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-style-wheel-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(
        NAME clip-shot-pinned-toolbar-parity-tests
        COMMAND clip-shot-pinned-window-tests --toolbar-parity-only -platform offscreen
    )
    add_test(NAME clip-shot-pinned-translation-tests
        COMMAND clip-shot-pinned-window-tests --translation-only)
    add_test(
        NAME clip-shot-pinned-movement-shortcut-tests
        COMMAND clip-shot-pinned-window-tests --movement-shortcut-only
    )
    add_test(
        NAME clip-shot-pinned-drawing-shortcut-toggle-tests
        COMMAND clip-shot-pinned-window-tests --drawing-shortcut-toggle-only
    )
    add_test(NAME clip-shot-pinned-history-shortcut-tests
        COMMAND clip-shot-pinned-window-tests --history-shortcuts-only -platform offscreen)
    if(WIN32)
        add_test(
            NAME clip-shot-pinned-native-drag-shortcut-tests
            COMMAND clip-shot-pinned-window-tests --native-drag-shortcut-only
        )
        add_test(
            NAME clip-shot-pinned-system-move-shortcut-real-mouse-tests
            COMMAND clip-shot-pinned-window-tests --system-move-loop-only
        )
        set_tests_properties(clip-shot-pinned-system-move-shortcut-real-mouse-tests PROPERTIES
            LABELS "interactive;windows"
        )
        add_test(
            NAME clip-shot-pinned-native-drag-dpi-tests
            COMMAND clip-shot-pinned-window-tests --native-drag-dpi-only
        )
    endif()

    add_executable(clip-shot-pdf-export-tests
        tests/screenshot_pdf_export_tests.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/services/screenshotimagefileservice.cpp
    )
    target_include_directories(clip-shot-pdf-export-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src/image")
    target_link_libraries(clip-shot-pdf-export-tests PRIVATE clip_shot_storage clip_shot_image_codec Qt6::Gui)
    add_test(NAME clip-shot-pdf-export-tests COMMAND clip-shot-pdf-export-tests)
    set_tests_properties(clip-shot-pdf-export-tests PROPERTIES
        LABELS "unit" ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-screenshot-image-file-service-tests
        tests/screenshot_image_file_service_tests.cpp
        include/clip_shot/presentation/screenshotimagefileservice.h
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
    )
    target_include_directories(clip-shot-screenshot-image-file-service-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
    )
    target_link_libraries(clip-shot-screenshot-image-file-service-tests PRIVATE clip_shot_storage
        clip_shot_image_codec
        Qt6::Gui
    )
    add_test(
        NAME clip-shot-screenshot-image-file-service-tests
        COMMAND clip-shot-screenshot-image-file-service-tests
    )
    set_tests_properties(clip-shot-screenshot-image-file-service-tests PROPERTIES
        RUN_SERIAL TRUE
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-recognition-file-export-tests
        tests/screenshot_recognition_file_export_tests.cpp
        src/presentation/services/screenshotrecognitionfileexport.cpp
    )
    target_include_directories(clip-shot-recognition-file-export-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-recognition-file-export-tests PRIVATE Qt6::Widgets)
    clip_shot_add_translations(clip-shot-recognition-file-export-tests)
    clip_shot_import_offscreen_platform(clip-shot-recognition-file-export-tests)
    add_test(NAME clip-shot-recognition-file-export-tests
             COMMAND clip-shot-recognition-file-export-tests)
    set_tests_properties(clip-shot-recognition-file-export-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-update-confirmation-tests
        tests/update_confirmation_dialog_tests.cpp
        src/app/updateconfirmationdialog.cpp
    )
    target_include_directories(clip-shot-update-confirmation-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-update-confirmation-tests PRIVATE Qt6::Widgets)
    clip_shot_add_translations(clip-shot-update-confirmation-tests)
    clip_shot_import_offscreen_platform(clip-shot-update-confirmation-tests)
    add_test(NAME clip-shot-update-confirmation-tests
             COMMAND clip-shot-update-confirmation-tests)
    set_tests_properties(clip-shot-update-confirmation-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-file-pin-settings-tests tests/file_pin_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-file-pin-settings-tests PRIVATE clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-file-pin-settings-tests)
    add_test(NAME clip-shot-file-pin-settings-tests COMMAND clip-shot-file-pin-settings-tests)
    set_tests_properties(clip-shot-file-pin-settings-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-custom-server-settings-tests
        tests/custom_server_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-custom-server-settings-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-custom-server-settings-tests)
    add_test(NAME clip-shot-custom-server-settings-tests COMMAND clip-shot-custom-server-settings-tests)
    set_tests_properties(clip-shot-custom-server-settings-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-selection-snap-shortcut-settings-tests
        tests/selection_snap_shortcut_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-selection-snap-shortcut-settings-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-selection-snap-shortcut-settings-tests)
    add_test(NAME clip-shot-selection-snap-shortcut-settings-tests
             COMMAND clip-shot-selection-snap-shortcut-settings-tests)
    set_tests_properties(clip-shot-selection-snap-shortcut-settings-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-selection-resize-mode-settings-tests
        tests/selection_resize_mode_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-selection-resize-mode-settings-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-selection-resize-mode-settings-tests)
    add_test(NAME clip-shot-selection-resize-mode-settings-tests
             COMMAND clip-shot-selection-resize-mode-settings-tests)
    set_tests_properties(clip-shot-selection-resize-mode-settings-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-remember-last-used-tool-settings-tests
        tests/remember_last_used_tool_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-remember-last-used-tool-settings-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-remember-last-used-tool-settings-tests)
    add_test(NAME clip-shot-remember-last-used-tool-settings-tests
             COMMAND clip-shot-remember-last-used-tool-settings-tests)
    set_tests_properties(clip-shot-remember-last-used-tool-settings-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-screen-recording-interaction-settings-tests
        tests/screen_recording_interaction_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-screen-recording-interaction-settings-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-screen-recording-interaction-settings-tests)
    add_test(NAME clip-shot-screen-recording-interaction-settings-tests
        COMMAND clip-shot-screen-recording-interaction-settings-tests)
    set_tests_properties(clip-shot-screen-recording-interaction-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-edit-selection-toolbar-settings-tests
        tests/edit_selection_toolbar_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-edit-selection-toolbar-settings-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-edit-selection-toolbar-settings-tests)
    add_test(NAME clip-shot-edit-selection-toolbar-settings-tests
             COMMAND clip-shot-edit-selection-toolbar-settings-tests)
    set_tests_properties(clip-shot-edit-selection-toolbar-settings-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-file-pin-batch-tests
        tests/screenshot_file_pin_batch_tests.cpp
        ${CLIP_SELECTED_FILES_SOURCES}
        src/presentation/services/screenshotfilepinbatch.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
        include/clip_shot/presentation/screenshotexportcoordinator.h
        src/presentation/services/screenshotclipboardcontent.cpp
        src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp
    )
    target_include_directories(clip-shot-file-pin-batch-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-file-pin-batch-tests PRIVATE clip_shot_image_codec Qt6::Widgets
        ${CLIP_SELECTED_FILES_LIBRARIES})
    if(APPLE)
        target_link_libraries(clip-shot-file-pin-batch-tests PRIVATE clip_shot_rust_ffi_selector)
    endif()
    if(WIN32)
        target_link_libraries(clip-shot-file-pin-batch-tests PRIVATE ole32 oleaut32 shell32 uuid)
        add_executable(clip-shot-selected-files-native-smoke-tests
            tests/selected_files_native_smoke_tests.cpp ${CLIP_SELECTED_FILES_SOURCES})
        target_include_directories(clip-shot-selected-files-native-smoke-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(clip-shot-selected-files-native-smoke-tests PRIVATE
            Qt6::Core ole32 oleaut32 shell32 uuid user32)
        add_test(NAME clip-shot-selected-files-native-smoke-tests
            COMMAND clip-shot-selected-files-native-smoke-tests)
        set_tests_properties(clip-shot-selected-files-native-smoke-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    if(APPLE)
        add_executable(clip-shot-macos-selected-files-tests
            tests/macos_selected_files_tests.mm ${CLIP_SELECTED_FILES_SOURCES})
        target_include_directories(clip-shot-macos-selected-files-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(clip-shot-macos-selected-files-tests PRIVATE
            Qt6::Core ${CLIP_SELECTED_FILES_LIBRARIES})
        add_test(NAME clip-shot-macos-selected-files-tests COMMAND clip-shot-macos-selected-files-tests)
        add_executable(clip-shot-macos-selected-files-native-smoke MACOSX_BUNDLE
            tests/macos_selected_files_native_smoke.mm ${CLIP_SELECTED_FILES_SOURCES})
        target_include_directories(clip-shot-macos-selected-files-native-smoke PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(clip-shot-macos-selected-files-native-smoke PRIVATE
            Qt6::Gui ${CLIP_SELECTED_FILES_LIBRARIES})
        set_target_properties(clip-shot-macos-selected-files-native-smoke PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/tests/macos_selected_files_smoke.plist.in")
        add_test(NAME clip-shot-macos-selected-files-native-smoke
            COMMAND clip-shot-macos-selected-files-native-smoke)
        set_tests_properties(clip-shot-macos-selected-files-native-smoke PROPERTIES
            LABELS "interactive" RUN_SERIAL TRUE TIMEOUT 120 SKIP_RETURN_CODE 77)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-file-pin-batch-tests)
    add_test(NAME clip-shot-file-pin-batch-tests COMMAND clip-shot-file-pin-batch-tests)
    set_tests_properties(clip-shot-file-pin-batch-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-clipboard-content-tests
        tests/screenshot_clipboard_content_tests.cpp
        include/clip_shot/presentation/screenshotclipboardcontent.h
        src/presentation/services/screenshotclipboardcontent.cpp
        src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp
    )
    target_include_directories(clip-shot-clipboard-content-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
    )
    target_link_libraries(clip-shot-clipboard-content-tests PRIVATE
        clip_shot_image_codec
        Qt6::Widgets
    )
    add_test(NAME clip-shot-clipboard-content-tests
             COMMAND clip-shot-clipboard-content-tests)
    clip_shot_import_offscreen_platform(clip-shot-clipboard-content-tests)
    add_test(NAME clip-shot-clipboard-mime-data-tests
             COMMAND clip-shot-clipboard-content-tests --mime-data-only)
    add_test(NAME clip-shot-clipboard-embedded-images-tests
             COMMAND clip-shot-clipboard-content-tests --embedded-images-only)
    set_tests_properties(clip-shot-clipboard-embedded-images-tests PROPERTIES
        LABELS unit
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-clipboard-mime-data-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-clipboard-content-tests PROPERTIES
        RUN_SERIAL TRUE
        # The live snapshot exercises an in-process mime provider that hides
        # the Qt image format; only the offscreen clipboard keeps that
        # in-process semantics, the native OS clipboard drops the payload.
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        add_executable(clip-shot-clipboard-roundtrip-tests
            tests/screenshot_clipboard_roundtrip_tests.cpp
            src/presentation/services/screenshotclipboardservice.cpp
            src/presentation/services/screenshotclipboardcontent.cpp
            src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp
        )
        target_include_directories(clip-shot-clipboard-roundtrip-tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
        )
        target_link_libraries(clip-shot-clipboard-roundtrip-tests PRIVATE
            clip_shot_image_codec clip_shot_diagnostics Qt6::Widgets user32
        )
        add_test(NAME clip-shot-clipboard-roundtrip-tests
                 COMMAND clip-shot-clipboard-roundtrip-tests)
        # This exercises the real Windows clipboard without showing any windows.
        set_tests_properties(clip-shot-clipboard-roundtrip-tests PROPERTIES
            LABELS "unit;windows"
            RUN_SERIAL TRUE
            TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()
    add_test(
        NAME clip-shot-pinned-async-presentation-tests
        COMMAND clip-shot-pinned-window-tests --async-presentation-only
    )
    add_test(NAME clip-shot-pinned-content-replacement-tests
        COMMAND clip-shot-pinned-window-tests --content-replacement-only -platform offscreen)
    add_test(NAME clip-shot-pinned-file-drop-tests
        COMMAND clip-shot-pinned-window-tests --file-drop-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-file-drop-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit" TIMEOUT 90)
    set_tests_properties(clip-shot-pinned-content-replacement-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit" TIMEOUT 90 RUN_SERIAL TRUE)
    add_test(
        NAME clip-shot-pinned-window-pool-tests
        COMMAND clip-shot-pinned-window-tests --pooling-only
    )
    add_test(
        NAME clip-shot-pinned-group-menu-tests
        COMMAND clip-shot-pinned-window-tests --group-menu-only
    )
    add_test(
        NAME clip-shot-pinned-cached-ocr-tests
        COMMAND clip-shot-pinned-window-tests --cached-ocr-only
    )
    add_test(
        NAME clip-shot-pinned-restored-ocr-tests
        COMMAND clip-shot-pinned-window-tests --restored-ocr-only
    )
    add_test(NAME clip-shot-pinned-initial-ocr-tests
        COMMAND clip-shot-pinned-window-tests --initial-ocr-only)
    add_test(NAME clip-shot-pinned-early-ocr-snapshot-tests
        COMMAND clip-shot-pinned-window-tests --early-ocr-snapshot-only)
    add_test(NAME clip-shot-pinned-invalid-ocr-restore-tests
        COMMAND clip-shot-pinned-window-tests --invalid-ocr-restore-only)
    add_test(NAME clip-shot-pinned-cached-ocr-provider-tests
        COMMAND clip-shot-pinned-window-tests --cached-ocr-provider-only)
    add_test(NAME clip-shot-pinned-cloud-upload-tests COMMAND clip-shot-pinned-window-tests --cloud-upload-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-cloud-upload-tests PROPERTIES LABELS "unit" TIMEOUT 45
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-toolbar-layout-tests
        COMMAND clip-shot-pinned-window-tests --pinned-toolbar-layout-only)
    set_tests_properties(clip-shot-pinned-toolbar-layout-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-pinned-recognition-shortcut-tests
        COMMAND clip-shot-pinned-window-tests --recognition-shortcut-only)
    add_test(NAME clip-shot-pinned-recognition-shortcut-offscreen-tests
        COMMAND clip-shot-pinned-window-tests --recognition-shortcut-only -platform offscreen)
    add_test(NAME clip-shot-pinned-recognition-context-menu-tests
        COMMAND clip-shot-pinned-window-tests --recognition-context-menu-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-recognition-context-menu-tests PROPERTIES
        LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-pinned-transformed-ocr-tests
        COMMAND clip-shot-pinned-window-tests --transformed-ocr-only)
    add_test(NAME clip-shot-pinned-transform-geometry-tests
        COMMAND clip-shot-pinned-window-tests --transform-geometry-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-transform-geometry-tests PROPERTIES
        LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-pinned-selection-restore-tests
        COMMAND clip-shot-pinned-window-tests --selection-restore-only -platform offscreen)
    add_test(NAME clip-shot-pinned-restored-state-persistence-tests
        COMMAND clip-shot-pinned-window-tests --restored-state-persistence-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-restored-state-persistence-tests PROPERTIES
        LABELS "unit" TIMEOUT 60)
    if(WIN32)
        add_test(NAME clip-shot-pinned-resize-style-tests
            COMMAND clip-shot-pinned-window-tests --resize-style-only -platform offscreen)
        set_tests_properties(clip-shot-pinned-resize-style-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 60)
    endif()
    add_test(
        NAME clip-shot-pinned-restore-wiring-tests
        COMMAND clip-shot-pinned-window-tests --restore-wiring-only
    )
    if(WIN32)
        add_test(NAME clip-shot-pinned-escape-activation-tests
            COMMAND clip-shot-pinned-window-tests --escape-activation-only)
        set_tests_properties(clip-shot-pinned-escape-activation-tests PROPERTIES
            LABELS "interactive;windows"
            RUN_SERIAL TRUE
            TIMEOUT 90
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    add_test(NAME clip-shot-pinned-escape-burst-tests
        COMMAND clip-shot-pinned-window-tests --escape-burst-only)
    add_test(NAME clip-shot-pinned-tool-rotation-tests
        COMMAND clip-shot-pinned-window-tests --tool-rotation-only)
    add_test(NAME clip-shot-pinned-scale-readout-tests
        COMMAND clip-shot-pinned-window-tests --scale-readout-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-scale-readout-tests PROPERTIES
        LABELS "unit" TIMEOUT 90
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-passive-geometry-tests
        COMMAND clip-shot-pinned-window-tests --passive-geometry-only)
    add_test(NAME clip-shot-pinned-pointer-presence-offscreen-tests
        COMMAND clip-shot-pinned-window-tests --pointer-presence-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-pointer-presence-offscreen-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-pinned-passive-geometry-tests PROPERTIES
        LABELS "unit"
        RUN_SERIAL TRUE
        TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(WIN32)
        add_test(NAME clip-shot-pinned-passive-native-geometry-tests
            COMMAND clip-shot-pinned-window-tests --passive-geometry-only)
        set_tests_properties(clip-shot-pinned-passive-native-geometry-tests PROPERTIES
            LABELS "unit;windows"
            RUN_SERIAL TRUE
            TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        add_test(NAME clip-shot-pinned-pointer-presence-tests
            COMMAND clip-shot-pinned-window-tests --pointer-presence-only)
        set_tests_properties(clip-shot-pinned-pointer-presence-tests PROPERTIES
            LABELS "unit;windows"
            RUN_SERIAL TRUE
            TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    set(_clip_shot_pinned_window_test_names
        clip-shot-pinned-escape-burst-tests
        clip-shot-pinned-tool-rotation-tests
        clip-shot-pinned-middle-click-tests
        clip-shot-pinned-middle-click-offscreen-tests
        clip-shot-pinned-double-click-tests
        clip-shot-pinned-double-click-offscreen-tests
        clip-shot-pinned-window-tests
        clip-shot-pinned-translation-tests
        clip-shot-pinned-async-presentation-tests
        clip-shot-pinned-window-pool-tests
        clip-shot-pinned-group-menu-tests
        clip-shot-pinned-cached-ocr-tests
        clip-shot-pinned-restored-ocr-tests
        clip-shot-pinned-initial-ocr-tests
        clip-shot-pinned-early-ocr-snapshot-tests
        clip-shot-pinned-invalid-ocr-restore-tests
        clip-shot-pinned-cached-ocr-provider-tests
        clip-shot-pinned-recognition-shortcut-tests
        clip-shot-pinned-recognition-shortcut-offscreen-tests
        clip-shot-pinned-transformed-ocr-tests
        clip-shot-pinned-restore-wiring-tests
        clip-shot-pinned-selection-restore-tests
        clip-shot-pinned-thumbnail-recreation-tests
        clip-shot-pinned-thumbnail-reentry-tests
        clip-shot-pinned-thumbnail-background-tests
        clip-shot-pinned-toolbar-parity-tests
        clip-shot-pinned-movement-shortcut-tests
        clip-shot-pinned-drawing-shortcut-toggle-tests
        clip-shot-pinned-history-shortcut-tests
    )
    if(WIN32)
        list(APPEND _clip_shot_pinned_window_test_names
            clip-shot-pinned-native-drag-shortcut-tests
            clip-shot-pinned-system-move-shortcut-real-mouse-tests
            clip-shot-pinned-native-drag-dpi-tests
        )
    endif()
    set_tests_properties(
        ${_clip_shot_pinned_window_test_names}
        PROPERTIES
        RUN_SERIAL TRUE
        TIMEOUT 90
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-export-coordinator-tests
        tests/screenshot_export_coordinator_tests.cpp
        include/clip_shot/presentation/screenshotclipboardservice.h
        include/clip_shot/presentation/screenshotexportcoordinator.h
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
    )
    target_include_directories(clip-shot-export-coordinator-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-export-coordinator-tests PRIVATE clip_shot_image_codec clip_shot_diagnostics Qt6::Widgets)
    if(WIN32)
        target_link_libraries(clip-shot-export-coordinator-tests PRIVATE user32)
    endif()
    add_test(NAME clip-shot-export-coordinator-tests
             COMMAND clip-shot-export-coordinator-tests)
    set_tests_properties(clip-shot-export-coordinator-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-export-dialog-workflow-tests
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        tests/screenshot_export_dialog_workflow_tests.cpp
        include/clip_shot/presentation/screenshotsavepreviewcanvas.h
        src/presentation/services/screenshotsaveasfiledialog.cpp
        src/presentation/services/screenshotsaveexportpipeline.cpp
        src/presentation/services/screenshotsavepreviewcanvas.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotexportartifact.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
        src/presentation/services/screenshotexportpinnedsource.cpp
        src/presentation/ocr/screenshotrecognitionimage.cpp
    )
    target_include_directories(clip-shot-export-dialog-workflow-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/services"
    )
    target_link_libraries(clip-shot-export-dialog-workflow-tests PRIVATE
        clip_shot_image_codec clip_shot_settings clip_draw_engine_qt_canvas Qt6::Widgets
    )
    if(APPLE)
        target_sources(clip-shot-export-dialog-workflow-tests PRIVATE
            ../cmake/test-support/macos_native_input.mm)
        target_include_directories(clip-shot-export-dialog-workflow-tests PRIVATE ../cmake/test-support)
        target_link_libraries(clip-shot-export-dialog-workflow-tests PRIVATE "-framework AppKit")
    endif()
    clip_shot_import_offscreen_platform(clip-shot-export-dialog-workflow-tests)
    add_test(NAME clip-shot-pdf-dialog-tests COMMAND clip-shot-export-dialog-workflow-tests --pdf)
    set_tests_properties(clip-shot-pdf-dialog-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-export-dialog-workflow-tests
        COMMAND clip-shot-export-dialog-workflow-tests)
    add_test(NAME clip-shot-export-dialog-shortcut-tests
        COMMAND clip-shot-export-dialog-workflow-tests --shortcuts)
    add_test(NAME clip-shot-export-preview-wheel-tests
        COMMAND clip-shot-export-dialog-workflow-tests --wheel-zoom -platform offscreen)
    set_tests_properties(clip-shot-export-preview-wheel-tests PROPERTIES
        TIMEOUT 30 LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    # These tests drive QWidget events synchronously. Cocoa menu tracking has its
    # own event loop and is exercised by clip-shot-action-popup-native-tests.
    set_tests_properties(
        clip-shot-export-dialog-workflow-tests clip-shot-export-dialog-shortcut-tests
        PROPERTIES ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    set_tests_properties(clip-shot-export-dialog-shortcut-tests PROPERTIES
        TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-save-format-tests
        COMMAND clip-shot-export-dialog-workflow-tests --remember-format)
    set_tests_properties(clip-shot-save-format-tests PROPERTIES
        TIMEOUT 90
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit")
    add_test(NAME clip-shot-export-dialog-pipeline-tests
        COMMAND clip-shot-export-dialog-workflow-tests --optimized-pipeline)
    add_test(NAME clip-shot-export-dialog-placement-tests
        COMMAND clip-shot-export-dialog-workflow-tests --placement)
    set_tests_properties(clip-shot-export-dialog-placement-tests PROPERTIES
        TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-export-dialog-workflow-tests PROPERTIES
        TIMEOUT 180 RUN_SERIAL TRUE
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-export-dialog-pipeline-tests PROPERTIES
        TIMEOUT 90
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-export-artifact-tests
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        tests/screenshot_export_artifact_tests.cpp
        tests/screenshot_export_row_performance_benchmark.cpp
        include/clip_shot/presentation/screenshotclipboardservice.h
        include/clip_shot/presentation/screenshotexportartifact.h
        include/clip_shot/presentation/screenshotexportcoordinator.h
        include/clip_shot/presentation/screenshotresultcompositor.h
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotexportartifact.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
        src/presentation/services/screenshotexportpinnedsource.cpp
        src/presentation/ocr/screenshotrecognitionimage.cpp
    )
    target_include_directories(clip-shot-export-artifact-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
    )
    target_link_libraries(clip-shot-export-artifact-tests PRIVATE
        clip_shot_diagnostics
        clip_shot_image_codec
        clip_shot_storage
        clip_draw_engine_qt_canvas
        adqt::widgets
        Qt6::Widgets
    )
    if(WIN32)
        target_link_libraries(clip-shot-export-artifact-tests PRIVATE user32)
    endif()
    add_test(NAME clip-shot-export-artifact-tests COMMAND clip-shot-export-artifact-tests)
    add_test(NAME clip-shot-pinned-export-lifetime-tests
        COMMAND clip-shot-export-artifact-tests --pinned-lifetime-only)
    set_tests_properties(clip-shot-pinned-export-lifetime-tests PROPERTIES
        LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    get_target_property(_clip_cloud_upload_test_sources clip-shot-export-artifact-tests SOURCES)
    list(REMOVE_ITEM _clip_cloud_upload_test_sources tests/screenshot_export_artifact_tests.cpp
        tests/screenshot_export_row_performance_benchmark.cpp)
    add_executable(clip-shot-cloud-upload-tests ${_clip_cloud_upload_test_sources} tests/cloud_upload_tests.cpp)
    target_include_directories(clip-shot-cloud-upload-tests PRIVATE include src/image)
    target_link_libraries(clip-shot-cloud-upload-tests PRIVATE clip_shot_cloud_upload clip_shot_settings
        clip_shot_diagnostics clip_shot_image_codec clip_shot_storage clip_draw_engine_qt_canvas ant_design_qt Qt6::Widgets)
    if(WIN32)
        target_link_libraries(clip-shot-cloud-upload-tests PRIVATE user32)
    endif()
    clip_shot_add_translations(clip-shot-cloud-upload-tests)
    clip_shot_import_offscreen_platform(clip-shot-cloud-upload-tests)
    add_test(NAME clip-shot-cloud-upload-tests COMMAND clip-shot-cloud-upload-tests -platform offscreen)
    add_test(NAME clip-shot-cloud-upload-settings-tests
        COMMAND clip-shot-cloud-upload-tests --settings-only -platform offscreen)
    set_tests_properties(clip-shot-cloud-upload-tests clip-shot-cloud-upload-settings-tests
        PROPERTIES LABELS "unit" TIMEOUT 90
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_test(NAME clip-shot-quick-save-command-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --quick-save-only -platform offscreen)
    add_test(NAME clip-shot-quick-save-layout-storage-tests
        COMMAND clip-shot-application-storage-tests --toolbar-layout-only)
    add_test(NAME clip-shot-quick-save-export-tests
        COMMAND clip-shot-export-artifact-tests --quick-save-only)
    add_test(NAME clip-shot-pinned-hidden-text-selection-restore-tests
        COMMAND clip-shot-pinned-window-tests --hidden-text-selection-restore-only)
    add_test(NAME clip-shot-pinned-hidden-text-selection-tests
        COMMAND clip-shot-pinned-window-tests --hidden-text-selection-only -platform offscreen)
    add_test(NAME clip-shot-pinned-recognition-save-tests
        COMMAND clip-shot-pinned-window-tests --recognition-save-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-recognition-save-tests PROPERTIES LABELS "unit" TIMEOUT 90)
    add_test(NAME clip-shot-pinned-shared-image-export-tests
        COMMAND clip-shot-pinned-window-tests --shared-image-export-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-shared-image-export-tests PROPERTIES
        LABELS "unit" TIMEOUT 90
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-pinned-opacity-export-tests
        COMMAND clip-shot-pinned-window-tests --opacity-export-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-opacity-export-tests PROPERTIES
        LABELS "unit" TIMEOUT 90)
    add_test(NAME clip-shot-pinned-quick-save-tests
        COMMAND clip-shot-pinned-window-tests --quick-save-only -platform $<IF:$<PLATFORM_ID:Darwin>,offscreen,windows>)
    set_tests_properties(clip-shot-pinned-quick-save-tests PROPERTIES LABELS "unit;windows")
    add_test(NAME clip-shot-quick-save-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --quick-save-only -platform offscreen)
    set_tests_properties(clip-shot-quick-save-export-tests clip-shot-pinned-quick-save-tests
        clip-shot-quick-save-toolbar-tests clip-shot-quick-save-command-tests
        clip-shot-quick-save-layout-storage-tests PROPERTIES TIMEOUT 90
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    set_tests_properties(clip-shot-export-artifact-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-distance-annotation-integration-tests
        tests/distance_annotation_integration_tests.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp)
    target_include_directories(clip-shot-distance-annotation-integration-tests PRIVATE include)
    target_link_libraries(clip-shot-distance-annotation-integration-tests PRIVATE
        clip_shot_storage clip_shot_window_shortcuts clip_draw_engine_qt_canvas
        ant_design_qt adqt_icon_core ant_design_icons_qt Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-distance-annotation-integration-tests)
    add_test(NAME clip-shot-distance-annotation-integration-tests
        COMMAND clip-shot-distance-annotation-integration-tests -platform offscreen)
    set_tests_properties(clip-shot-distance-annotation-integration-tests PROPERTIES
        LABELS "unit" TIMEOUT 60 CLIP_TEST_CROSS_PLATFORM TRUE
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-screenshot-tool-palette-tests
        tests/screenshot_tool_palette_tests.cpp
        tests/screenshot_style_binding_tests.cpp
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/services/languagemanager.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
    )
    target_include_directories(clip-shot-screenshot-tool-palette-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_compile_definitions(clip-shot-screenshot-tool-palette-tests PRIVATE
        CLIP_SHOT_TEST_HOOKS=1
    )
    target_link_libraries(clip-shot-screenshot-tool-palette-tests PRIVATE
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        Qt6::Widgets
        clip_draw_engine_qt_canvas
    )
    clip_shot_add_translations(clip-shot-screenshot-tool-palette-tests)
    add_test(NAME clip-shot-ocr-text-options-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --ocr-translation-only -platform offscreen)
    set_tests_properties(clip-shot-ocr-text-options-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-selection-display-unit-toolbar-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --selection-unit-only -platform offscreen)
    set_tests_properties(clip-shot-selection-display-unit-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-move-region-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --move-options-only -platform offscreen)
    set_tests_properties(clip-shot-move-region-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-region-switcher-style-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --region-switcher-only -platform offscreen)
    set_tests_properties(clip-shot-region-switcher-style-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-screenshot-serial-number-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --serial-number-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-serial-number-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(
        NAME clip-shot-screenshot-tool-palette-tests
        COMMAND clip-shot-screenshot-tool-palette-tests
    )
    add_test(NAME clip-shot-screenshot-tool-palette-distance-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --distance-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-tool-palette-distance-tests
        PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-scrolling-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --scrolling-only -platform offscreen)
    add_test(NAME clip-shot-text-style-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --text-style-only -platform offscreen)
    set_tests_properties(clip-shot-text-style-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    set_tests_properties(clip-shot-scrolling-toolbar-tests PROPERTIES LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-screenshot-tool-palette-remembered-drawing-tool-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --remembered-drawing-tool-only
            -platform offscreen)
    set_tests_properties(clip-shot-screenshot-tool-palette-remembered-drawing-tool-tests
        PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-eraser-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --eraser-only -platform offscreen)
    set_tests_properties(clip-shot-eraser-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-eraser-source-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --eraser-source-only -platform offscreen)
    set_tests_properties(clip-shot-eraser-source-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-style-commit-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --style-commit-only -platform offscreen)
    set_tests_properties(clip-shot-style-commit-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-spotlight-wheel-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --spotlight-wheel-only -platform offscreen)
    set_tests_properties(clip-shot-spotlight-wheel-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-spotlight-shape-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --spotlight-shape-only -platform offscreen)
    set_tests_properties(clip-shot-spotlight-shape-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-watermark-template-width-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --watermark-template-width-only
            -platform offscreen)
    set_tests_properties(clip-shot-watermark-template-width-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-draw-template-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --draw-template-only -platform offscreen)
    set_tests_properties(clip-shot-draw-template-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-toolbar-popup-recovery-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --popup-recovery-only -platform offscreen)
    set_tests_properties(clip-shot-toolbar-popup-recovery-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-toolbar-popover-lifecycle-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --popover-lifecycle-only
            -platform offscreen)
    set_tests_properties(clip-shot-toolbar-popover-lifecycle-tests PROPERTIES LABELS "unit"
        TIMEOUT 90
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-toolbar-popover-hover-switching-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --popover-hover-switching-only
            -platform offscreen)
    set_tests_properties(clip-shot-toolbar-popover-hover-switching-tests PROPERTIES LABELS "unit"
        TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-cloud-upload-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --cloud-upload-only -platform offscreen)
    set_tests_properties(clip-shot-cloud-upload-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 45
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(
        NAME clip-shot-screenshot-action-toolbar-layout-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --action-toolbar-layout-only
    )
    add_test(NAME clip-shot-screenshot-result-toolbar-layout-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --screenshot-result-layout-only
                -platform offscreen)
    set_tests_properties(clip-shot-screenshot-result-toolbar-layout-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(
        NAME clip-shot-toolbar-color-input-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --color-input-only -platform offscreen
    )
    add_test(
        NAME clip-shot-screenshot-tool-shortcut-toggle-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --tool-shortcuts-only
    )
    add_test(NAME clip-shot-hidden-toolbar-shortcut-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --hidden-tools-only -platform offscreen)
    add_test(NAME clip-shot-annotation-toolbar-layout-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --toolbar-layout-only -platform offscreen)
    add_test(NAME clip-shot-toolbar-group-button-settings-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --first-group-button-only -platform offscreen)
    set_tests_properties(clip-shot-toolbar-group-button-settings-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-annotation-toolbar-layout-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-hidden-toolbar-shortcut-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-screenshot-action-shortcut-display-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --screenshot-actions-tooltips-only
            -platform offscreen)
    set_tests_properties(clip-shot-screenshot-action-shortcut-display-tests PROPERTIES LABELS "unit")
    add_test(
        NAME clip-shot-screenshot-shortcut-tooltip-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --shortcut-tooltips-only
    )
    add_test(
        NAME clip-shot-screenshot-selection-reset-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --selection-reset-only
    )
    add_test(
        NAME clip-shot-screen-recording-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --recording-controls-only
    )
    add_test(
        NAME clip-shot-toolbar-shared-controls-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --color-control-styles-only
    )
    set_tests_properties(clip-shot-toolbar-shared-controls-tests PROPERTIES TIMEOUT 180)
    add_test(
        NAME clip-shot-screenshot-toolbar-i18n-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --dynamic-i18n-only
    )
    set_tests_properties(
        clip-shot-screenshot-tool-palette-tests
        clip-shot-screenshot-tool-palette-remembered-drawing-tool-tests
        clip-shot-screenshot-action-toolbar-layout-tests
        clip-shot-screenshot-tool-shortcut-toggle-tests
        clip-shot-hidden-toolbar-shortcut-tests
        clip-shot-screenshot-shortcut-tooltip-tests
        clip-shot-screenshot-selection-reset-tests
        clip-shot-screen-recording-toolbar-tests
        clip-shot-toolbar-shared-controls-tests
        clip-shot-screenshot-toolbar-i18n-tests
        PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-screenshot-selection-toolbar-hover-tests
        tests/screenshot_selection_toolbar_hover_tests.cpp
        include/clip_shot/presentation/screenshotselectiontoolbarwidget.h
        src/presentation/components/icons/clipshoticons.cpp
        src/presentation/selection/screenshotselectiontoolbarwidget.cpp
        src/presentation/selection/screenshotselectiontoolbarwidgets.h
        src/presentation/selection/screenshotselectiontoolbarwidgets.cpp
    )
    clip_shot_import_offscreen_platform(clip-shot-screenshot-selection-toolbar-hover-tests)
    target_include_directories(clip-shot-screenshot-selection-toolbar-hover-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/selection"
    )
    target_link_libraries(clip-shot-screenshot-selection-toolbar-hover-tests PRIVATE
        clip_shot_window_shortcuts
        adqt::widgets
        adqt_icon_core
        ant_design_icons_qt
        Qt6::Widgets
        clip_draw_engine_qt_canvas
    )
    add_test(
        NAME clip-shot-screenshot-selection-toolbar-hover-tests
        COMMAND clip-shot-screenshot-selection-toolbar-hover-tests
    )
    add_test(
        NAME clip-shot-screenshot-selection-toolbar-compact-ratio-tests
        COMMAND clip-shot-screenshot-selection-toolbar-hover-tests --compact-ratio-tests
    )
    add_test(
        NAME clip-shot-screenshot-selection-toolbar-icon-centering-tests
        COMMAND clip-shot-screenshot-selection-toolbar-hover-tests --icon-centering-tests
    )
    set_tests_properties(clip-shot-screenshot-selection-toolbar-icon-centering-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit"
    )
    set_tests_properties(clip-shot-screenshot-selection-toolbar-compact-ratio-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit"
    )
    set_tests_properties(clip-shot-screenshot-selection-toolbar-hover-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        set_tests_properties(clip-shot-screenshot-selection-toolbar-hover-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
        )
    else()
        set_tests_properties(clip-shot-screenshot-selection-toolbar-hover-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        )
    endif()

    add_executable(clip-shot-screenshot-toolbar-remembered-tool-tests
        tests/screenshot_toolbar_remembered_tool_tests.cpp
        src/presentation/core/screenshotinteractionstate.cpp
        src/presentation/core/screenshotintelligentselectionmodel.cpp
        src/presentation/overlay/screenshotoverlayinputhandler.cpp
        src/presentation/overlay/screenshotoverlayshortcutcontroller.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/toolbar/screenshottoolbarpresentationstatefactory.cpp
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenshottoolbarwindow.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
        src/presentation/toolbar/screenshottoolbarwindow.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
    )
    clip_shot_link_macos_window_platform(clip-shot-screenshot-toolbar-remembered-tool-tests)
    target_include_directories(clip-shot-screenshot-toolbar-remembered-tool-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screenshot-toolbar-remembered-tool-tests PRIVATE
        clip_shot_window_shortcuts
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        clip_shot_storage
    )
    if(WIN32)
        target_link_libraries(clip-shot-screenshot-toolbar-remembered-tool-tests PRIVATE comctl32)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-screenshot-toolbar-remembered-tool-tests)
    add_test(NAME clip-shot-scrolling-interval-command-tests
        COMMAND clip-shot-screenshot-toolbar-remembered-tool-tests --scrolling-interval-only -platform offscreen)
    add_test(NAME clip-shot-edit-selection-toolbar-visibility-tests
        COMMAND clip-shot-screenshot-toolbar-remembered-tool-tests --edit-selection-toolbar-only -platform offscreen)
    set_tests_properties(clip-shot-edit-selection-toolbar-visibility-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-eraser-commands-tests
        COMMAND clip-shot-screenshot-toolbar-remembered-tool-tests --eraser-only -platform offscreen)
    set_tests_properties(clip-shot-eraser-commands-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-screenshot-toolbar-remembered-tool-tests
        COMMAND clip-shot-screenshot-toolbar-remembered-tool-tests)
    set_tests_properties(clip-shot-scrolling-interval-command-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    set_tests_properties(clip-shot-screenshot-toolbar-remembered-tool-tests
        clip-shot-scrolling-interval-command-tests PROPERTIES
        RUN_SERIAL TRUE
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-global-canvas-tests
        tests/global_canvas_tests.cpp
        src/presentation/services/screenshotcanvascolorsampler.cpp
        src/presentation/services/screenshotcanvascolorsamplerwindow.cpp
        include/clip_shot/presentation/globalcanvascontroller.h
        src/presentation/globalcanvas/globalcanvascontroller.cpp
        src/presentation/globalcanvas/globalcanvasplatform.cpp
        src/platform/physicalcursor.cpp
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
    )
    target_include_directories(clip-shot-global-canvas-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-global-canvas-tests PRIVATE
        clip_shot_window_shortcuts clip_shot_storage clip_draw_engine_qt_canvas
        adqt_icon_core ant_design_icons_qt ant_design_qt Qt6::Widgets Qt6::GuiPrivate)
    clip_shot_link_macos_window_platform(clip-shot-global-canvas-tests)
    if(WIN32)
        target_sources(clip-shot-global-canvas-tests PRIVATE src/platform/windows/pinnedwindownative.cpp)
        target_link_libraries(clip-shot-global-canvas-tests PRIVATE Qt6::GuiPrivate comctl32)
    endif()
    if(WIN32)
        add_test(NAME clip-shot-global-canvas-native-tests
            COMMAND clip-shot-global-canvas-tests --native-only -platform windows)
        set_tests_properties(clip-shot-global-canvas-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 90
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        add_test(NAME clip-shot-global-canvas-color-sampling-native-tests
            COMMAND clip-shot-global-canvas-tests --color-sampling-native-only -platform windows)
        set_tests_properties(clip-shot-global-canvas-color-sampling-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 30
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    if(APPLE)
        add_test(NAME clip-shot-global-canvas-platform-tests
            COMMAND clip-shot-global-canvas-tests --platform-only -platform offscreen)
        set_tests_properties(clip-shot-global-canvas-platform-tests PROPERTIES
            LABELS "unit;macos" TIMEOUT 30)
        add_test(NAME clip-shot-global-canvas-native-tests
            COMMAND clip-shot-global-canvas-tests --native-only -platform cocoa)
        set_tests_properties(clip-shot-global-canvas-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 90)
        add_test(NAME clip-shot-global-canvas-window-shadow-native-tests
            COMMAND clip-shot-global-canvas-tests --window-shadow-only -platform cocoa)
        set_tests_properties(clip-shot-global-canvas-window-shadow-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 30)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-global-canvas-tests)
    add_test(NAME clip-shot-global-canvas-window-shadow-tests
        COMMAND clip-shot-global-canvas-tests --window-shadow-only -platform offscreen)
    set_tests_properties(clip-shot-global-canvas-window-shadow-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-global-canvas-history-shortcut-tests
        COMMAND clip-shot-global-canvas-tests --history-shortcuts-only -platform offscreen)
    set_tests_properties(clip-shot-global-canvas-history-shortcut-tests PROPERTIES
        LABELS "unit" TIMEOUT 90 RUN_SERIAL TRUE)
    add_test(NAME clip-shot-global-canvas-saved-style-tests
        COMMAND clip-shot-global-canvas-tests --saved-styles-only -platform offscreen)
    set_tests_properties(clip-shot-global-canvas-saved-style-tests PROPERTIES LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-global-canvas-tests COMMAND clip-shot-global-canvas-tests -platform offscreen)
    set_tests_properties(clip-shot-global-canvas-tests PROPERTIES LABELS "unit" TIMEOUT 90
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-global-canvas-eraser-tests
        COMMAND clip-shot-global-canvas-tests --eraser-only -platform offscreen)
    set_tests_properties(clip-shot-global-canvas-eraser-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-global-canvas-color-sampling-tests
        COMMAND clip-shot-global-canvas-tests --color-sampling-only -platform offscreen)
    set_tests_properties(clip-shot-global-canvas-color-sampling-tests PROPERTIES LABELS "unit" TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    if(WIN32)
        add_executable(clip-shot-screenshot-toolbar-fill-color-hover-tests
            tests/screenshot_toolbar_fill_color_hover_tests.cpp
            include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
            include/clip_shot/presentation/screenshottoolbarwindow.h
            include/clip_shot/presentation/screenshottoolpalette.h
            include/clip_shot/presentation/screenshottoolpalettehost.h
            include/clip_shot/presentation/styles/thememanager.h
            src/presentation/components/icons/clipshoticons.cpp
            src/platform/windows/monitorgeometry.cpp
            src/presentation/core/screenshotgeometry.cpp
            src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
            src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
            src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
            src/presentation/tools/screenshottoolbarmainpanel.cpp
            src/presentation/tools/screenshottoolpalettebuttons.cpp
            src/presentation/tools/screenshottoolpalettehost.cpp
            src/presentation/tools/screenshottoolpalettestylecontrols.cpp
            src/presentation/tools/screenshottoolpalettestylecomponents.cpp
            src/presentation/tools/screenshottoolpalettestylemodel.cpp
            src/presentation/tools/screenshottoolpalettestylepresets.cpp
            src/presentation/tools/screenshottoolpalette.cpp
            src/presentation/recording/recordingaudiogainpopover.cpp
            src/presentation/recording/recordingaudiogainpopover.h
            src/presentation/toolbar/screenshottoolbarwindow.cpp
            src/presentation/styles/thememanager.cpp
            src/presentation/styles/themecolorscheme.cpp
        )
        target_include_directories(
            clip-shot-screenshot-toolbar-fill-color-hover-tests
            PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        target_link_libraries(
            clip-shot-screenshot-toolbar-fill-color-hover-tests
            PRIVATE
                adqt_icon_core
                ant_design_icons_qt
                ant_design_qt
                Qt6::Widgets
                clip_draw_engine_qt_canvas
                comctl32
        )
        # Flaky by design: drives a real hover animation over native toolbar
        # surfaces; parallel test load can steal the frame budget and stall
        # the hover transition. The test must not run in parallel with other
        # tests (RUN_SERIAL), and intermittent failures are tolerated via
        # the retry launcher.
        add_test(
            NAME clip-shot-screenshot-toolbar-fill-color-hover-tests
            COMMAND ${CMAKE_COMMAND}
                -DTEST_COMMAND=$<TARGET_FILE:clip-shot-screenshot-toolbar-fill-color-hover-tests>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_flaky_test_with_retry.cmake
        )
        set_tests_properties(
            clip-shot-screenshot-toolbar-fill-color-hover-tests
            PROPERTIES
                RUN_SERIAL TRUE
                ENVIRONMENT
                    "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
                ENVIRONMENT_MODIFICATION
                    "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )

        # This is intentionally not registered with CTest. It creates native
        # always-on-top windows, moves the real cursor, and is designed for
        # explicit Release benchmarking sessions.
        add_executable(clip-shot-screenshot-toolbar-render-benchmark
            tests/screenshot_toolbar_render_benchmark.cpp
            include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
            include/clip_shot/presentation/screenshottoolbarwindow.h
            include/clip_shot/presentation/screenshottoolpalette.h
            include/clip_shot/presentation/screenshottoolpalettehost.h
            include/clip_shot/presentation/styles/thememanager.h
            src/presentation/components/icons/clipshoticons.cpp
            src/platform/windows/monitorgeometry.cpp
            src/presentation/core/screenshotgeometry.cpp
            src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
            src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
            src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
            src/presentation/tools/screenshottoolbarmainpanel.cpp
            src/presentation/tools/screenshottoolpalettebuttons.cpp
            src/presentation/tools/screenshottoolpalettehost.cpp
            src/presentation/tools/screenshottoolpalettestylecontrols.cpp
            src/presentation/tools/screenshottoolpalettestylecomponents.cpp
            src/presentation/tools/screenshottoolpalettestylemodel.cpp
            src/presentation/tools/screenshottoolpalettestylepresets.cpp
            src/presentation/tools/screenshottoolpalette.cpp
            src/presentation/recording/recordingaudiogainpopover.cpp
            src/presentation/recording/recordingaudiogainpopover.h
            src/presentation/tools/screenshottoolbarperfinstrumentation.h
            src/presentation/toolbar/screenshottoolbarwindow.cpp
            src/presentation/styles/thememanager.cpp
            src/presentation/styles/themecolorscheme.cpp
        )
        target_compile_definitions(
            clip-shot-screenshot-toolbar-render-benchmark
            PRIVATE
                NOMINMAX
                CLIP_SHOT_TOOLBAR_PERF_INSTRUMENTATION=1
        )
        target_include_directories(
            clip-shot-screenshot-toolbar-render-benchmark
            PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        target_link_libraries(
            clip-shot-screenshot-toolbar-render-benchmark
            PRIVATE
                adqt_icon_core
                ant_design_icons_qt
                ant_design_qt
                Qt6::Widgets
                clip_draw_engine_qt_canvas
                comctl32
                dwmapi
                user32
        )
    endif()

    add_executable(clip-shot-screen-recording-shortcut-tests
        tests/screen_recording_shortcut_tests.cpp
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenrecordingareawindow.h
        include/clip_shot/presentation/screenrecordingtoolbarwindow.h
        include/clip_shot/presentation/screenrecordingshortcutcontroller.h
        src/presentation/recording/screenrecordingareawindow.cpp
        src/presentation/recording/screenrecordingtoolbarwindow.cpp
        src/presentation/recording/screenrecordingshortcutcontroller.cpp
        src/presentation/recording/screenrecordinggeometry.cpp
        src/presentation/recording/recordingcountdownoverlay.cpp
        src/presentation/recording/recordingcountdownoverlay.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
    )
    clip_shot_link_macos_window_platform(clip-shot-screen-recording-shortcut-tests)
    target_compile_definitions(clip-shot-screen-recording-shortcut-tests PRIVATE
        CLIP_SHOT_TEST_HOOKS=1
    )
    target_include_directories(clip-shot-screen-recording-shortcut-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-screen-recording-shortcut-tests PRIVATE
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        clip_shot_window_shortcuts
        clip_shot_storage
    )
    if(WIN32)
        target_link_libraries(clip-shot-screen-recording-shortcut-tests PRIVATE user32 comctl32)
    endif()
    add_test(NAME clip-shot-screen-recording-shortcut-tests
        COMMAND clip-shot-screen-recording-shortcut-tests)
    if(WIN32)
        foreach(_close_release_pair IN ITEMS
                "overlay|clip-shot-screenshot-canvas-renderer-tests"
                "pinned|clip-shot-pinned-window-tests"
                "recording|clip-shot-screen-recording-shortcut-tests")
            string(REPLACE "|" ";" _close_release_parts "${_close_release_pair}")
            list(GET _close_release_parts 0 _close_release_name)
            list(GET _close_release_parts 1 _close_release_target)
            add_test(NAME clip-shot-${_close_release_name}-close-release-native-tests
                COMMAND ${_close_release_target} --close-release-native -platform windows)
            set_tests_properties(clip-shot-${_close_release_name}-close-release-native-tests
                PROPERTIES LABELS "interactive;e2e;windows" RUN_SERIAL TRUE TIMEOUT 60
                ENVIRONMENT "QT_QPA_PLATFORM=windows")
        endforeach()
    endif()
    if(APPLE)
        set(_clip_recording_shortcut_platform cocoa)
        set(_clip_recording_shortcut_os macos)
    else()
        set(_clip_recording_shortcut_platform windows)
        set(_clip_recording_shortcut_os windows)
    endif()
    add_test(NAME clip-shot-recording-history-shortcut-tests
        COMMAND clip-shot-screen-recording-shortcut-tests --history-shortcuts-only)
    set_tests_properties(clip-shot-screen-recording-shortcut-tests
        clip-shot-recording-history-shortcut-tests PROPERTIES
        # The shortcut gating asserts real keyboard focus ownership, which the
        # non-activating floating toolbar only supports on a native platform.
        LABELS "interactive;e2e;${_clip_recording_shortcut_os}" RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=${_clip_recording_shortcut_platform}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-floating-toolbar-drag-tests
        tests/screenshot_floating_toolbar_drag_tests.cpp
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenshottoolbarwindow.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
        src/presentation/toolbar/screenshottoolbarwindow.cpp
    )
    clip_shot_link_macos_window_platform(clip-shot-floating-toolbar-drag-tests)
    target_compile_definitions(clip-shot-floating-toolbar-drag-tests PRIVATE
        CLIP_SHOT_TEST_HOOKS=1
    )
    target_include_directories(clip-shot-floating-toolbar-drag-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-floating-toolbar-drag-tests PRIVATE
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        Qt6::Widgets
        clip_draw_engine_qt_canvas
    )
    if(WIN32)
        target_link_libraries(clip-shot-floating-toolbar-drag-tests PRIVATE
            comctl32
        )
    endif()
    if(APPLE)
        target_sources(clip-shot-floating-toolbar-drag-tests PRIVATE
            ../cmake/test-support/macos_native_input.mm)
        target_link_libraries(clip-shot-floating-toolbar-drag-tests PRIVATE "-framework AppKit")
        target_include_directories(clip-shot-floating-toolbar-drag-tests PRIVATE ../cmake/test-support)
        add_test(NAME clip-shot-macos-toolbar-shadow-input-tests
            COMMAND clip-shot-floating-toolbar-drag-tests --macos-shadow-input-only -platform cocoa)
        set_tests_properties(clip-shot-macos-toolbar-shadow-input-tests PROPERTIES
            LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60)
        add_test(NAME clip-shot-macos-toolbar-cross-display-drag-tests
            COMMAND clip-shot-floating-toolbar-drag-tests --macos-cross-display-only -platform cocoa)
        set_tests_properties(clip-shot-macos-toolbar-cross-display-drag-tests PROPERTIES
            LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE
            TIMEOUT 60 SKIP_RETURN_CODE 77)
    endif()
    # Exercise the real UI host with the same overlay and toolbar implementations.
    get_target_property(_color_picker_overlay_sources
        clip-shot-screenshot-canvas-renderer-tests SOURCES)
    get_target_property(_color_picker_toolbar_sources
        clip-shot-screenshot-toolbar-remembered-tool-tests SOURCES)
    set(_color_picker_lifecycle_sources
        ${_color_picker_overlay_sources} ${_color_picker_toolbar_sources})
    list(FILTER _color_picker_lifecycle_sources EXCLUDE REGEX "^tests/")
    list(FILTER _color_picker_lifecycle_sources EXCLUDE
        REGEX "/(directcapturehistory|screenshothistoryservice)[.]")
    list(REMOVE_DUPLICATES _color_picker_lifecycle_sources)
    # Exercise scrolling session replacement with synthetic frames and native policy adapters.
    add_library(clip-shot-scrolling-settings-test-runtime OBJECT
        include/clip_shot/presentation/screenshotscrollingcapturecontroller.h
        src/presentation/capture/screenshotscrollingcapturecontroller.cpp
        src/presentation/capture/screenshotscrollingpipeline.cpp)
    target_include_directories(clip-shot-scrolling-settings-test-runtime PRIVATE include src
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-stitch-images-c/include")
    target_link_libraries(clip-shot-scrolling-settings-test-runtime PRIVATE
        Qt6::Widgets clip_draw_engine_qt_canvas clip_canvas_image clip_stitch_images_c
        clip_shot_diagnostics ant_design_qt adqt_icon_core ant_design_icons_qt)
    target_compile_definitions(clip-shot-scrolling-settings-test-runtime PRIVATE
        setWindowExcludedFromCapture=clipTestSetWindowExcludedFromCapture
        captureWindowId=clipTestCaptureWindowId
        screenshotScrollPermission=clipTestScrollPermission
        sendScreenshotScroll=clipTestSendScreenshotScroll
        sendScrollingWheelStep=clipTestSendScrollingWheelStep
        flushWindowComposition=clipTestFlushWindowComposition)
    add_executable(clip-shot-scrolling-capture-settings-tests
        tests/scrolling_capture_settings_tests.cpp
        ${_color_picker_lifecycle_sources}
        src/presentation/overlay/screenshotoverlaycoordinator.cpp
        src/presentation/overlay/screenshotoverlayuihost.cpp
        src/presentation/services/screenshotcolorpickerwindow.cpp
        include/clip_shot/presentation/screenshotselectiontoolbarwidget.h
        src/presentation/selection/screenshotselectiontoolbarwidget.cpp
        src/presentation/selection/screenshotselectiontoolbarwidgets.h
        src/presentation/selection/screenshotselectiontoolbarwidgets.cpp)
    target_include_directories(clip-shot-scrolling-capture-settings-tests PRIVATE include src)
    target_link_libraries(clip-shot-scrolling-capture-settings-tests PRIVATE
        clip-shot-scrolling-settings-test-runtime clip_shot_diagnostics clip_shot_storage
        clip_shot_window_shortcuts clip_draw_engine_qt_canvas clip_canvas_image clip_stitch_images_c
        ant_design_qt adqt_icon_core ant_design_icons_qt Qt6::Widgets)
    clip_shot_link_macos_window_platform(clip-shot-scrolling-capture-settings-tests)
    if(WIN32)
        target_link_libraries(clip-shot-scrolling-capture-settings-tests PRIVATE comctl32)
    elseif(APPLE)
        target_sources(clip-shot-scrolling-capture-settings-tests PRIVATE
            src/platform/macos/windowcaptureexclusion.mm)
        target_link_libraries(clip-shot-scrolling-capture-settings-tests PRIVATE
            "-framework AppKit" "-framework CoreGraphics")
    endif()
    clip_shot_import_offscreen_platform(clip-shot-scrolling-capture-settings-tests)
    add_test(NAME clip-shot-scrolling-capture-settings-tests
        COMMAND clip-shot-scrolling-capture-settings-tests -platform offscreen)
    set_tests_properties(clip-shot-scrolling-capture-settings-tests PROPERTIES
        LABELS "unit" CLIP_TEST_CROSS_PLATFORM TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_executable(clip-shot-color-picker-lifecycle-tests
        tests/screenshot_color_picker_lifecycle_tests.cpp
        src/presentation/services/screenshotcanvascolorsamplerwindow.cpp
        src/presentation/services/screenshotcolorpickercontroller.cpp
        src/platform/physicalcursor.cpp
        ${_color_picker_lifecycle_sources}
        src/presentation/overlay/screenshotoverlaycoordinator.cpp
        src/presentation/overlay/screenshotoverlayuihost.cpp
        src/presentation/services/screenshotcolorpickerwindow.cpp
        include/clip_shot/presentation/screenshotselectiontoolbarwidget.h
        src/presentation/selection/screenshotselectiontoolbarwidget.cpp
        src/presentation/selection/screenshotselectiontoolbarwidgets.h
        src/presentation/selection/screenshotselectiontoolbarwidgets.cpp
    )
    add_test(NAME clip-shot-screenshot-style-binding-tests
        COMMAND clip-shot-color-picker-lifecycle-tests --style-binding-only -platform offscreen)
    set_tests_properties(clip-shot-screenshot-style-binding-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-selection-toolbar-visibility-tests
        COMMAND clip-shot-color-picker-lifecycle-tests --selection-toolbar-visibility-only -platform offscreen)
    set_tests_properties(clip-shot-selection-toolbar-visibility-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    clip_shot_link_macos_window_platform(clip-shot-color-picker-lifecycle-tests)
    target_compile_definitions(clip-shot-color-picker-lifecycle-tests PRIVATE
        CLIP_SHOT_BENCH_INTERNALS=1)
    target_include_directories(clip-shot-color-picker-lifecycle-tests PRIVATE include)
    target_link_libraries(clip-shot-color-picker-lifecycle-tests PRIVATE
        clip_shot_diagnostics clip_shot_storage clip_shot_window_shortcuts
        clip_draw_engine_qt_canvas ant_design_qt adqt_icon_core ant_design_icons_qt Qt6::Widgets)
    if(WIN32)
        target_link_libraries(clip-shot-color-picker-lifecycle-tests PRIVATE comctl32)
    elseif(APPLE)
        target_sources(clip-shot-color-picker-lifecycle-tests PRIVATE
            src/platform/macos/windowcaptureexclusion.mm)
        target_link_libraries(clip-shot-color-picker-lifecycle-tests PRIVATE
            "-framework AppKit" "-framework CoreGraphics")
    endif()
    if(APPLE)
        set_source_files_properties(tests/screenshot_color_picker_lifecycle_tests.cpp
            PROPERTIES LANGUAGE OBJCXX)
        add_test(NAME clip-shot-macos-canvas-sampler-stacking-native-tests
            COMMAND clip-shot-color-picker-lifecycle-tests --canvas-sampler-only -platform cocoa)
        set_tests_properties(clip-shot-macos-canvas-sampler-stacking-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-color-picker-lifecycle-tests)
    add_test(NAME clip-shot-color-picker-lifecycle-tests
        COMMAND clip-shot-color-picker-lifecycle-tests)
    add_test(NAME clip-shot-toolbar-picker-hover-tests
        COMMAND clip-shot-color-picker-lifecycle-tests --toolbar-picker-hover-only)
    add_test(NAME clip-shot-toolbar-guide-hover-tests
        COMMAND clip-shot-color-picker-lifecycle-tests --toolbar-guide-hover-only)
    set_tests_properties(clip-shot-toolbar-guide-hover-tests PROPERTIES
        LABELS "unit" CLIP_TEST_CROSS_PLATFORM TRUE
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        TIMEOUT 60)
    set_tests_properties(clip-shot-toolbar-picker-hover-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        TIMEOUT 60)
    if(WIN32)
        add_test(NAME clip-shot-tool-stacking-native-tests
            COMMAND clip-shot-color-picker-lifecycle-tests --stacking-only -platform windows)
        set_tests_properties(clip-shot-tool-stacking-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE)
    endif()
    set_tests_properties(clip-shot-color-picker-lifecycle-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        TIMEOUT 60)
    unset(_color_picker_overlay_sources)
    unset(_color_picker_toolbar_sources)
    unset(_color_picker_lifecycle_sources)

    clip_shot_import_offscreen_platform(clip-shot-floating-toolbar-drag-tests)
    add_test(NAME clip-shot-toolbar-stable-tool-frame-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --stable-tool-frame-only -platform offscreen)
    set_tests_properties(clip-shot-toolbar-stable-tool-frame-tests PROPERTIES
        LABELS "unit" CLIP_TEST_CROSS_PLATFORM TRUE)
    add_test(NAME clip-shot-border-cursor-tool-restoration-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --border-cursor-only -platform offscreen)
    set_tests_properties(clip-shot-border-cursor-tool-restoration-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-interrupted-toolbar-drag-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --interrupted-drag-only -platform offscreen)
    set_tests_properties(clip-shot-interrupted-toolbar-drag-tests PROPERTIES LABELS "unit")
    if(NOT APPLE)
        add_test(NAME clip-shot-floating-toolbar-dpi-frame-reconcile-tests
            COMMAND clip-shot-floating-toolbar-drag-tests --dpi-frame-reconcile-only)
        set_tests_properties(clip-shot-floating-toolbar-dpi-frame-reconcile-tests PROPERTIES
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    if(APPLE)
        foreach(_toolbar_dpr IN ITEMS 1 2)
            add_test(NAME clip-shot-macos-toolbar-logical-${_toolbar_dpr}x-tests
                COMMAND clip-shot-floating-toolbar-drag-tests --macos-logical-only)
            set_tests_properties(clip-shot-macos-toolbar-logical-${_toolbar_dpr}x-tests PROPERTIES
                LABELS "unit;macos" CLIP_TEST_CROSS_PLATFORM TRUE TIMEOUT 60
                ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT};QT_SCALE_FACTOR=${_toolbar_dpr}")
        endforeach()
        add_test(NAME clip-shot-macos-toolbar-native-tests
            COMMAND clip-shot-floating-toolbar-drag-tests --macos-logical-only -platform cocoa)
        add_test(NAME clip-shot-macos-toolbar-focus-tests
            COMMAND clip-shot-floating-toolbar-drag-tests --keyboard-focus-only -platform cocoa)
        set_tests_properties(clip-shot-macos-toolbar-native-tests clip-shot-macos-toolbar-focus-tests
            PROPERTIES LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE
            RUN_SERIAL TRUE TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
    endif()
    if(WIN32)
        add_test(NAME clip-shot-floating-toolbar-capture-screen-switch-tests
            COMMAND clip-shot-floating-toolbar-drag-tests --capture-screen-switch-only)
        set_tests_properties(clip-shot-floating-toolbar-capture-screen-switch-tests PROPERTIES
            RUN_SERIAL TRUE
            SKIP_RETURN_CODE 77
            LABELS "interactive;windows"
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    add_test(NAME clip-shot-floating-toolbar-keyboard-focus-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --keyboard-focus-only)
    set_tests_properties(clip-shot-floating-toolbar-keyboard-focus-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(WIN32)
        add_test(NAME clip-shot-floating-toolbar-keyboard-focus-windows-tests
            COMMAND clip-shot-floating-toolbar-drag-tests --keyboard-focus-only)
        set_tests_properties(clip-shot-floating-toolbar-keyboard-focus-windows-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    # The forward and reverse drag tests drive a native toolbar through real
    # hardware-style drag sequences and assert final HWND positions within a
    # few pixels. They are flaky by design under machine load: parallel tests
    # compete for the input queue and DWM frame budget, shifting the settled
    # position. They must not run in parallel with other tests (RUN_SERIAL),
    # and intermittent failures are tolerated via the retry launcher.
    add_test(
        NAME clip-shot-floating-toolbar-drag-tests
        COMMAND ${CMAKE_COMMAND}
            -DTEST_COMMAND=$<TARGET_FILE:clip-shot-floating-toolbar-drag-tests>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_flaky_test_with_retry.cmake
    )
    add_test(
        NAME clip-shot-floating-toolbar-reverse-drag-tests
        COMMAND ${CMAKE_COMMAND}
            "-DTEST_COMMAND=$<TARGET_FILE:clip-shot-floating-toolbar-drag-tests>;--reverse-hardware-drag-only"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_flaky_test_with_retry.cmake
    )
    add_test(
        NAME clip-shot-floating-toolbar-seam-straddle-drag-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --seam-straddle-drag-only
    )
    add_test(
        NAME clip-shot-floating-toolbar-action-layout-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --action-layout-only
    )
    add_test(
        NAME clip-shot-floating-toolbar-recording-hit-test-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --recording-hit-test-only
    )
    add_test(
        NAME clip-shot-screenshot-selection-reset-command-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --selection-reset-only
    )
    set_tests_properties(clip-shot-screenshot-selection-reset-command-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(
        NAME clip-shot-screenshot-cancel-ordering-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --cancel-ordering-only -platform offscreen
    )
    set_tests_properties(clip-shot-screenshot-cancel-ordering-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(
        NAME clip-shot-jump-to-translation-page-toolbar-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --jump-to-translation-page-only
            -platform offscreen
    )
    set_tests_properties(clip-shot-jump-to-translation-page-toolbar-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-floating-toolbar-drag-tests PROPERTIES
        RUN_SERIAL TRUE
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-floating-toolbar-reverse-drag-tests PROPERTIES
        RUN_SERIAL TRUE
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-floating-toolbar-seam-straddle-drag-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-floating-toolbar-action-layout-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    set_tests_properties(clip-shot-floating-toolbar-recording-hit-test-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(NOT WIN32)
        set_tests_properties(clip-shot-floating-toolbar-drag-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        )
        set_tests_properties(clip-shot-floating-toolbar-reverse-drag-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        )
        set_tests_properties(clip-shot-floating-toolbar-seam-straddle-drag-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        )
        set_tests_properties(clip-shot-floating-toolbar-action-layout-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        )
        set_tests_properties(clip-shot-floating-toolbar-recording-hit-test-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        )
    endif()

    add_executable(clip-shot-adqt-dpi-stable-tests
        tests/adqt_dpi_stable_tests.cpp
    )
    if(WIN32)
        target_link_libraries(clip-shot-adqt-dpi-stable-tests PRIVATE comctl32)
    endif()
    target_link_libraries(clip-shot-adqt-dpi-stable-tests PRIVATE
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        Qt6::Widgets
        Qt6::Svg
    )
    clip_shot_import_offscreen_platform(clip-shot-adqt-dpi-stable-tests)
    add_test(
        NAME clip-shot-adqt-dpi-stable-tests
        COMMAND clip-shot-adqt-dpi-stable-tests
    )
    add_test(NAME clip-shot-toolbar-control-scaling-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --dpi-scaling-only -platform offscreen)
    set_tests_properties(clip-shot-toolbar-control-scaling-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-adqt-dpi-stable-offscreen-tests
        COMMAND clip-shot-adqt-dpi-stable-tests -platform offscreen)
    set_tests_properties(clip-shot-adqt-dpi-stable-offscreen-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-floating-toolbar-dpi-transaction-offscreen-tests
        COMMAND clip-shot-floating-toolbar-drag-tests --dpi-frame-reconcile-only -platform offscreen)
    set_tests_properties(clip-shot-floating-toolbar-dpi-transaction-offscreen-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(WIN32)
        set_tests_properties(clip-shot-adqt-dpi-stable-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-adqt-dpi-stable-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-generated-icon-pack-tests
        tests/generated_icon_pack_tests.cpp
        src/presentation/components/icons/clipshoticons.cpp
    )
    target_include_directories(clip-shot-generated-icon-pack-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-generated-icon-pack-tests PRIVATE
        ant_design_icons_qt
        Qt6::Widgets
        Qt6::Svg
    )
    if(WIN32 AND QT_FEATURE_static)
        # Exercise the release application's explicit plugin policy even
        # when the Debug preset enables Qt's default plugin imports.
        qt_import_plugins(clip-shot-generated-icon-pack-tests
            NO_DEFAULT INCLUDE ${_clip_shot_static_qt_plugins})
    endif()
    add_test(
        NAME clip-shot-generated-icon-pack-tests
        COMMAND clip-shot-generated-icon-pack-tests
    )
    if(WIN32)
        set_tests_properties(clip-shot-generated-icon-pack-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-generated-icon-pack-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-application-icon-tests
        tests/application_icon_tests.cpp
        src/presentation/components/icons/clipshoticons.cpp
    )
    if(WIN32)
        target_sources(clip-shot-application-icon-tests PRIVATE
            resources/app-icon.ico
            resources/windows-app-icon.rc
        )
    endif()
    target_include_directories(clip-shot-application-icon-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-application-icon-tests PRIVATE
        ant_design_icons_qt
        Qt6::Widgets
        Qt6::Svg
    )
    if(WIN32 AND QT_FEATURE_static)
        qt_import_plugins(clip-shot-application-icon-tests
            NO_DEFAULT INCLUDE ${_clip_shot_static_qt_plugins})
    endif()
    add_test(
        NAME clip-shot-application-icon-tests
        COMMAND clip-shot-application-icon-tests
    )
    set_tests_properties(clip-shot-application-icon-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-sidebar-widget-tests
        tests/sidebar_widget_tests.cpp
        include/clip_shot/presentation/components/sidebarwidget.h
        include/clip_shot/presentation/styles/themecolorscheme.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/sidebarwidget.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/styles/thememanager.cpp
    )
    target_include_directories(clip-shot-sidebar-widget-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-sidebar-widget-tests PRIVATE
        ant_design_qt
        clip_shot_settings_search
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-sidebar-widget-tests
        COMMAND clip-shot-sidebar-widget-tests
    )
    if(WIN32)
        set_tests_properties(clip-shot-sidebar-widget-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-sidebar-widget-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-application-search-tests
        tests/application_search_tests.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../test-support/fonts/recording_test_fonts.qrc"
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/styles/thememanager.cpp
    )
    target_link_libraries(clip-shot-application-search-tests PRIVATE
        clip_shot_settings_search
    )
    clip_shot_import_offscreen_platform(clip-shot-application-search-tests)
    add_test(
        NAME clip-shot-application-search-tests
        COMMAND clip-shot-application-search-tests -platform offscreen
    )
    set_tests_properties(clip-shot-application-search-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit"
    )

    add_executable(clip-shot-main-content-header-tests
        tests/main_content_header_tests.cpp
        include/clip_shot/presentation/components/sidebarwidget.h
        src/presentation/components/sidebarwidget.cpp
        include/clip_shot/presentation/components/maincontentheaderwidget.h
        include/clip_shot/presentation/styles/themecolorscheme.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/maincontentheaderwidget.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/styles/thememanager.cpp
    )
    target_include_directories(clip-shot-main-content-header-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-main-content-header-tests PRIVATE
        ant_design_icons_qt
        ant_design_qt
        clip_shot_settings_search
        Qt6::Widgets
        clip_shot_storage
    )
    clip_shot_import_offscreen_platform(clip-shot-main-content-header-tests)
    add_test(
        NAME clip-shot-main-content-header-tests
        COMMAND clip-shot-main-content-header-tests
    )
    if(WIN32)
        set_tests_properties(clip-shot-main-content-header-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-main-content-header-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    add_executable(clip-shot-toolbar-main-panel-tests
        tests/screenshot_toolbar_main_panel_tests.cpp
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
    )
    target_include_directories(clip-shot-toolbar-main-panel-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-toolbar-main-panel-tests PRIVATE
        ant_design_qt
        clip_draw_engine_qt_canvas
        Qt6::Widgets
        Qt6::Svg
        Qt6::Network
    )
    add_test(
        NAME clip-shot-toolbar-main-panel-tests
        COMMAND clip-shot-toolbar-main-panel-tests
    )
    set_tests_properties(clip-shot-toolbar-main-panel-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(NAME clip-shot-toolbar-skin-tests
        COMMAND clip-shot-toolbar-main-panel-tests --skin-only -platform offscreen)
    set_tests_properties(clip-shot-toolbar-skin-tests PROPERTIES
        LABELS "unit" CLIP_TEST_CROSS_PLATFORM TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(
        NAME clip-shot-toolbar-icon-centering-hidpi-tests
        COMMAND clip-shot-toolbar-main-panel-tests --icon-centering-only
    )
    set_tests_properties(clip-shot-toolbar-icon-centering-hidpi-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT};QT_SCALE_FACTOR=2"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit;windows"
    )

    if(APPLE)
        add_executable(clip-shot-macos-window-capture-exclusion-tests
            tests/macos_window_capture_exclusion_tests.mm
            src/platform/macos/windowcaptureexclusion.mm)
        target_include_directories(clip-shot-macos-window-capture-exclusion-tests PRIVATE
            include src "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include")
        target_link_libraries(clip-shot-macos-window-capture-exclusion-tests PRIVATE
            Qt6::Widgets clip_capture_c "${CLIP_SHOT_APPKIT_FRAMEWORK}")
        add_test(NAME clip-shot-macos-window-capture-exclusion-tests
            COMMAND clip-shot-macos-window-capture-exclusion-tests -platform cocoa)
        add_test(NAME clip-shot-macos-window-capture-exclusion-pixels-tests
            COMMAND clip-shot-macos-window-capture-exclusion-tests -platform cocoa --pixels)
        set_tests_properties(clip-shot-macos-window-capture-exclusion-tests
            clip-shot-macos-window-capture-exclusion-pixels-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60 SKIP_RETURN_CODE 77)
    endif()

    add_executable(clip-shot-window-capture-exclusion-tests
        tests/window_capture_exclusion_tests.cpp
        src/presentation/capture/windowcaptureexclusion.h
        src/presentation/capture/windowinputtransparency.h
    )
    target_include_directories(clip-shot-window-capture-exclusion-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(clip-shot-window-capture-exclusion-tests PRIVATE Qt6::Widgets)
    add_test(
        NAME clip-shot-window-capture-exclusion-tests
        COMMAND clip-shot-window-capture-exclusion-tests
    )
    set_tests_properties(clip-shot-window-capture-exclusion-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-scrolling-capture-exclusion-tests
        tests/scrolling_capture_exclusion_tests.cpp
        src/presentation/capture/screenshotscrollingpipeline.cpp
        src/presentation/capture/screenshotscrollingnativesource.cpp)
    target_include_directories(clip-shot-scrolling-capture-exclusion-tests PRIVATE
        include src "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-stitch-images-c/include")
    target_link_libraries(clip-shot-scrolling-capture-exclusion-tests PRIVATE
        clip_canvas_image
        Qt6::Widgets clip_stitch_images_c clip_shot_diagnostics)
    foreach(_capture_test_function IN ITEMS stream_create_region stream_receive stream_stop
            stream_destroy stream_stats stream_set_target_fps stream_frame_info
            stream_frame_release last_error_message)
        target_compile_definitions(clip-shot-scrolling-capture-exclusion-tests PRIVATE
            "clip_capture_${_capture_test_function}=clip_test_capture_${_capture_test_function}")
    endforeach()
    clip_shot_import_offscreen_platform(clip-shot-scrolling-capture-exclusion-tests)
    add_test(NAME clip-shot-scrolling-capture-exclusion-tests
        COMMAND clip-shot-scrolling-capture-exclusion-tests -platform offscreen)
    set_tests_properties(clip-shot-scrolling-capture-exclusion-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    get_target_property(_recording_test_sources clip-shot-screenshot-tool-palette-tests SOURCES)
    list(FILTER _recording_test_sources EXCLUDE REGEX "tests/screenshot_tool_palette_tests.cpp$")
    # The shared settings libraries supply the icons, theme objects and Qt metaobjects.
    list(REMOVE_ITEM _recording_test_sources
        src/presentation/components/icons/clipshoticons.cpp
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp)
    add_executable(clip-shot-screen-recording-controller-tests
        ${_recording_test_sources}
        tests/screen_recording_controller_tests.cpp
        tests/recording_color_sampler_tests.cpp
        tests/recording_popup_exclusion_tests.cpp
        include/clip_shot/presentation/screenrecordingcontroller.h
        include/clip_shot/presentation/screenrecordingfolder.h
        include/clip_shot/presentation/screenrecordingareawindow.h
        include/clip_shot/presentation/screenrecordingtoolbarwindow.h
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        src/presentation/recording/recordingtrimtoolbar.cpp
        src/presentation/recording/recordingtrimsession.cpp
        src/presentation/recording/recordingrenderdialog.cpp
        src/presentation/recording/recordingrenderdialog.h
        src/presentation/recording/recordingrenderjob.cpp
        src/presentation/recording/recordingrenderjob.h
        src/presentation/recording/screenrecordingcontroller.cpp
        src/presentation/recording/recordingcolorsampler.cpp
        src/presentation/recording/recordingcolorsampler.h
        src/presentation/services/screenshotcanvascolorsampler.cpp
        src/presentation/services/screenshotcanvascolorsamplerwindow.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/recording/screenrecordingfolder.cpp
        src/presentation/recording/recordingeffectpreview.cpp
        src/presentation/recording/recordingeffectpreview.h
        src/presentation/recording/recordingeffectstyle.h
        include/clip_shot/presentation/screenrecordingshortcutcontroller.h
        src/presentation/recording/screenrecordingshortcutcontroller.cpp
        src/presentation/recording/screenrecordingareawindow.cpp
        src/presentation/recording/screenrecordingtoolbarwindow.cpp
        src/presentation/recording/screenrecordinggeometry.cpp
        src/presentation/recording/recordingcountdownoverlay.cpp
        src/presentation/recording/recordingcountdownoverlay.h
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/platform/windows/windowchrome.cpp
    )
    clip_shot_link_macos_window_platform(clip-shot-screen-recording-controller-tests)
    target_include_directories(clip-shot-screen-recording-controller-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-recording-c/include")
    target_link_libraries(clip-shot-screen-recording-controller-tests PRIVATE
        adqt_icon_core ant_design_icons_qt ant_design_qt clip_draw_engine_qt_canvas
        clip_shot_storage clip_shot_image_codec clip_shot_window_shortcuts clip_shot_settings Qt6::Widgets clip_recording_effects_c clip_capture_c)
    if(APPLE)
        target_sources(clip-shot-screen-recording-controller-tests PRIVATE
            src/platform/macos/windowcaptureexclusion.mm
            tests/macos_capture_exclusion_probe.mm
            tests/macos_recording_modal_probe.mm)
        target_link_libraries(clip-shot-screen-recording-controller-tests PRIVATE "${CLIP_SHOT_APPKIT_FRAMEWORK}")
    endif()
    target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE CLIP_SHOT_TEST_HOOKS=1)
    target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE
        CLIP_SHOT_TEST_TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}")
    add_dependencies(clip-shot-screen-recording-controller-tests clip_shot_release_translations)
    target_sources(clip-shot-screen-recording-controller-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../test-support/fonts/recording_test_fonts.qrc")
    # Override only the test executable: exercise the real controller without
    # opening capture devices, recording the desktop, or exporting media.
    foreach(_operation create_direct create_deferred finalize_deferred destroy start pause resume request_stop stop state set_audio_gain set_audio_metering take_audio_levels request_exclusions exclusion_status)
        target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE
            "clip_recording_session_${_operation}=test_recording_session_${_operation}")
    endforeach()
    foreach(_operation create cancel destroy set_gain set_metering take_levels)
        target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE
            "clip_recording_audio_monitor_${_operation}=test_recording_audio_monitor_${_operation}")
    endforeach()
    foreach(_operation open destroy info boundary seek acquire frame_destroy error export_start export_poll export_error export_cancel export_destroy)
        target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE
            "clip_recording_clip_${_operation}=test_recording_clip_${_operation}")
    endforeach()
    target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE
        clip_recording_source_render_start=test_recording_source_render_start
        clip_recording_source_path=test_recording_source_path
        clip_recording_source_discard=test_recording_source_discard
        clip_recording_source_destroy=test_recording_source_destroy
        clip_recording_render_task_poll=test_recording_render_task_poll
        clip_recording_render_task_cancel=test_recording_render_task_cancel
        clip_recording_render_task_error=test_recording_render_task_error
        clip_recording_render_task_destroy=test_recording_render_task_destroy
        clip_recording_last_error_message=test_recording_last_error_message
        clip_recording_region_output_dimensions=test_recording_region_output_dimensions)
    if(WIN32)
        target_link_libraries(clip-shot-screen-recording-controller-tests PRIVATE user32 dwmapi gdi32)
        target_sources(clip-shot-screen-recording-controller-tests PRIVATE
            tests/screen_recording_toolbar_display_tests.cpp)
        add_test(NAME clip-shot-screen-recording-toolbar-display-tests
            COMMAND clip-shot-screen-recording-controller-tests
                --native-toolbar-display-only -platform windows)
        add_test(NAME clip-shot-screen-recording-toolbar-ready-display-tests
            COMMAND clip-shot-screen-recording-controller-tests
                --native-toolbar-ready-display-only -platform windows)
        add_test(NAME clip-shot-recording-color-sampler-native-tests
            COMMAND clip-shot-screen-recording-controller-tests
                --color-sampling-native-only -platform windows)
        set_tests_properties(clip-shot-recording-color-sampler-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CLIP_FFMPEG_ROOT}/bin")
        set_tests_properties(clip-shot-screen-recording-toolbar-display-tests
            clip-shot-screen-recording-toolbar-ready-display-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 30 SKIP_RETURN_CODE 77
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
    add_test(NAME clip-shot-recording-color-sampler-tests
        COMMAND clip-shot-screen-recording-controller-tests --color-sampling-only -platform offscreen)
    set_tests_properties(clip-shot-recording-color-sampler-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recording-tool-toggle-tests
        COMMAND clip-shot-screen-recording-controller-tests --tool-toggle-only -platform offscreen)
    set_tests_properties(clip-shot-recording-tool-toggle-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recording-eraser-tests
        COMMAND clip-shot-screen-recording-controller-tests --eraser-only -platform offscreen)
    set_tests_properties(clip-shot-recording-eraser-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recording-style-wheel-tests
        COMMAND clip-shot-screen-recording-controller-tests --style-wheel-only -platform offscreen)
    set_tests_properties(clip-shot-recording-style-wheel-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recording-trimming-tests
        COMMAND clip-shot-screen-recording-controller-tests --trim-only -platform offscreen)
    set_tests_properties(clip-shot-recording-trimming-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recording-post-processing-tests
        COMMAND clip-shot-screen-recording-controller-tests --post-processing-only -platform offscreen)
    set_tests_properties(clip-shot-recording-post-processing-tests PROPERTIES LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recording-render-layout-tests
        COMMAND clip-shot-screen-recording-controller-tests --render-layout-only -platform offscreen)
    set_tests_properties(clip-shot-recording-render-layout-tests PROPERTIES LABELS "unit" TIMEOUT 30)
    add_test(NAME clip-shot-recording-settings-dialog-tests
        COMMAND clip-shot-screen-recording-controller-tests --settings-dialog-only -platform offscreen)
    set_tests_properties(clip-shot-recording-settings-dialog-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-recording-api-mode-tests
        COMMAND clip-shot-screen-recording-controller-tests --api-mode-only -platform offscreen)
    set_tests_properties(clip-shot-recording-api-mode-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-recording-startup-retry-tests
        COMMAND clip-shot-screen-recording-controller-tests --startup-retry-only -platform offscreen)
    set_tests_properties(clip-shot-recording-startup-retry-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-recording-modal-stacking-tests
        COMMAND clip-shot-screen-recording-controller-tests --modal-stacking-only -platform offscreen)
    set_tests_properties(clip-shot-recording-modal-stacking-tests PROPERTIES LABELS "unit" TIMEOUT 30)
    if(APPLE)
        add_test(NAME clip-shot-macos-recording-modal-stacking-tests
            COMMAND clip-shot-screen-recording-controller-tests --modal-stacking-only -platform cocoa)
        set_tests_properties(clip-shot-macos-recording-modal-stacking-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    add_test(NAME clip-shot-screen-recording-controller-tests
        COMMAND clip-shot-screen-recording-controller-tests)
    add_test(NAME clip-shot-recording-finalization-close-tests
        COMMAND clip-shot-screen-recording-controller-tests --finalization-close-only -platform offscreen)
    set_tests_properties(clip-shot-recording-finalization-close-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-recording-auto-exit-tests
        COMMAND clip-shot-screen-recording-controller-tests --auto-exit-only -platform offscreen)
    set_tests_properties(clip-shot-recording-auto-exit-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-recording-export-notification-tests
        COMMAND clip-shot-screen-recording-controller-tests --export-notifications-only -platform offscreen)
    set_tests_properties(clip-shot-recording-export-notification-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
    if(WIN32)
        add_test(NAME clip-shot-recording-effects-preview-native-tests
            COMMAND clip-shot-screen-recording-controller-tests --effects-preview-native -platform windows)
        set_tests_properties(clip-shot-recording-effects-preview-native-tests PROPERTIES
            LABELS "interactive;e2e;windows" RUN_SERIAL TRUE TIMEOUT 90
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CLIP_FFMPEG_ROOT}/bin")
    endif()
    add_test(NAME clip-shot-recording-effects-preview-tests
        COMMAND clip-shot-screen-recording-controller-tests --effects-preview-only)
    set_tests_properties(clip-shot-recording-effects-preview-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
    if(APPLE)
        foreach(_preview_scale IN ITEMS 1 1.25 1.5 1.75 2)
            add_test(NAME clip-shot-recording-effects-preview-dpi-${_preview_scale}-tests
                COMMAND clip-shot-screen-recording-controller-tests --effects-preview-dpi-only)
            set_tests_properties(clip-shot-recording-effects-preview-dpi-${_preview_scale}-tests
                PROPERTIES
                ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT};QT_SCALE_FACTOR=${_preview_scale}"
                LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60)
        endforeach()
    endif()
    add_test(NAME clip-shot-recording-capture-exclusion-tests
        COMMAND clip-shot-screen-recording-controller-tests --capture-exclusion-only)
    set_tests_properties(clip-shot-recording-capture-exclusion-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60 ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    if(APPLE)
        add_test(NAME clip-shot-macos-recording-capture-exclusion-tests
            COMMAND clip-shot-screen-recording-controller-tests --capture-exclusion-only -platform cocoa)
        set_tests_properties(clip-shot-macos-recording-capture-exclusion-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-screen-recording-controller-tests)
    set_tests_properties(clip-shot-screen-recording-controller-tests PROPERTIES
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        RUN_SERIAL TRUE TIMEOUT 60)

    add_executable(clip-shot-screenshot-recording-workflow-tests
        tests/screenshot_recording_workflow_tests.cpp
    )
    target_link_libraries(clip-shot-screenshot-recording-workflow-tests PRIVATE Qt6::Core)
    add_test(NAME clip-shot-screenshot-recording-workflow-tests
        COMMAND clip-shot-screenshot-recording-workflow-tests)
    set_tests_properties(clip-shot-screenshot-recording-workflow-tests PROPERTIES
        LABELS "unit" TIMEOUT 30)
    if(WIN32)
        set_tests_properties(clip-shot-screenshot-recording-workflow-tests PROPERTIES
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;$ENV{PATH}")
    endif()

    add_executable(clip-shot-screen-recording-geometry-tests
        tests/screen_recording_geometry_tests.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/recording/screenrecordinggeometry.cpp
    )
    target_include_directories(clip-shot-screen-recording-geometry-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(clip-shot-screen-recording-geometry-tests PRIVATE
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-screen-recording-geometry-tests
        COMMAND clip-shot-screen-recording-geometry-tests
    )
    set_tests_properties(clip-shot-screen-recording-geometry-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT};QT_SCALE_FACTOR=1.25"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-screen-recording-area-window-tests
        tests/screen_recording_area_window_tests.cpp
        include/clip_shot/presentation/screenrecordingareawindow.h
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/recording/screenrecordingareawindow.cpp
        src/presentation/recording/screenrecordinggeometry.cpp
        src/presentation/recording/recordingcountdownoverlay.cpp
        src/presentation/recording/recordingcountdownoverlay.h
    )
    clip_shot_link_macos_window_platform(clip-shot-screen-recording-area-window-tests)
    target_include_directories(clip-shot-screen-recording-area-window-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(clip-shot-screen-recording-area-window-tests PRIVATE
        clip_shot_window_shortcuts
        ant_design_qt adqt_icon_core
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        clip_shot_storage
    )
    if(APPLE)
        target_sources(clip-shot-screen-recording-area-window-tests PRIVATE
            ../cmake/test-support/macos_native_input.mm)
        target_include_directories(clip-shot-screen-recording-area-window-tests PRIVATE
            ../cmake/test-support)
        add_test(NAME clip-shot-macos-recording-border-input-tests
            COMMAND clip-shot-screen-recording-area-window-tests --native-border-input-only -platform cocoa)
        set_tests_properties(clip-shot-macos-recording-border-input-tests PROPERTIES
            LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE
            TIMEOUT 60 SKIP_RETURN_CODE 77)
        add_test(NAME clip-shot-macos-recording-cross-display-drag-tests
            COMMAND clip-shot-screen-recording-area-window-tests --native-logical-drag-only -platform cocoa)
        set_tests_properties(clip-shot-macos-recording-cross-display-drag-tests PROPERTIES
            LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE
            TIMEOUT 60 SKIP_RETURN_CODE 77)
    endif()
    if(WIN32)
        target_link_libraries(clip-shot-screen-recording-area-window-tests PRIVATE user32)
    endif()
    add_test(
        NAME clip-shot-screen-recording-area-window-tests
        COMMAND clip-shot-screen-recording-area-window-tests
    )
    if(WIN32)
        add_test(NAME clip-shot-screen-recording-area-native-tests
            COMMAND clip-shot-screen-recording-area-window-tests --native-geometry-only -platform windows)
        set_tests_properties(clip-shot-screen-recording-area-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 45)
    endif()
    set_tests_properties(clip-shot-screen-recording-area-window-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT};QT_SCALE_FACTOR=1.25"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-ocr-background-tests
        tests/screenshot_ocr_background_tests.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp)
    target_include_directories(clip-shot-ocr-background-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-ocr-background-tests PRIVATE Qt6::Gui clip_draw_engine_qt_canvas)
    add_test(NAME clip-shot-ocr-background-tests COMMAND clip-shot-ocr-background-tests)
    set_tests_properties(clip-shot-ocr-background-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-ocr-presentation-tests
        tests/screenshot_ocr_presentation_tests.cpp
        include/clip_shot/presentation/screenshotocrpresentation.h
        include/clip_shot/presentation/screenshotocrtexteditingsession.h
        include/clip_shot/presentation/screenshotocrtexttransform.h
        include/clip_shot/presentation/screenshottabledocument.h
        include/clip_shot/presentation/screenshotsourceimagecomposer.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrtexteditingsession.cpp
        src/presentation/ocr/screenshottabledocument.cpp
        src/presentation/ocr/screenshotocrtexttransform.cpp
        src/presentation/ocr/screenshotocrlayout.cpp
        src/presentation/services/screenshotsourceimagecomposer.cpp
    )
    target_include_directories(clip-shot-ocr-presentation-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-ocr-presentation-tests PRIVATE
        clip_canvas_image
        Qt6::Widgets
    )
    add_test(
        NAME clip-shot-ocr-presentation-tests
        COMMAND clip-shot-ocr-presentation-tests
    )
    set_tests_properties(clip-shot-ocr-presentation-tests PROPERTIES
        ENVIRONMENT
            "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-recognition-window-tests
        tests/screenshot_recognition_window_tests.cpp
        include/clip_shot/presentation/screenshotcanvasrenderer.h
        include/clip_shot/presentation/screenshotgeometry.h
        include/clip_shot/presentation/screenshotocrpresentation.h
        include/clip_shot/presentation/screenshotocrtextlayer.h
        include/clip_shot/presentation/screenshottabledocument.h
        include/clip_shot/presentation/screenshottableeditor.h
        include/clip_shot/presentation/screenshotrecognitionwindow.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrtexttransform.cpp
        src/presentation/ocr/screenshotocrlayout.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp
        src/presentation/ocr/screenshottabledocument.cpp
        src/presentation/ocr/screenshottableeditor.cpp
        src/presentation/ocr/screenshotrecognitionwindow.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        src/presentation/overlay/screenshotcanvasrenderer.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/styles/buttonborder.cpp
    )
    clip_shot_link_macos_window_platform(clip-shot-recognition-window-tests)
    target_include_directories(clip-shot-recognition-window-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-recognition-window-tests PRIVATE
        ant_design_qt
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        clip_shot_storage
        clip_shot_window_shortcuts
    )
    if(WIN32)
        target_link_libraries(clip-shot-recognition-window-tests PRIVATE
            user32
        )
    endif()
    clip_shot_import_offscreen_platform(clip-shot-recognition-window-tests)
    if(APPLE)
        set_source_files_properties(tests/screenshot_recognition_window_tests.cpp
            PROPERTIES LANGUAGE OBJCXX)
        target_link_libraries(clip-shot-recognition-window-tests PRIVATE "-framework AppKit")
    endif()
    add_test(NAME clip-shot-recognition-message-shadow-tests
        COMMAND clip-shot-recognition-window-tests --message-shadow-only -platform offscreen)
    set_tests_properties(clip-shot-recognition-message-shadow-tests PROPERTIES
        LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recognition-context-menu-tests
        COMMAND clip-shot-recognition-window-tests --context-menu-only -platform offscreen)
    set_tests_properties(clip-shot-recognition-context-menu-tests PROPERTIES
        LABELS "unit" TIMEOUT 60)
    add_test(NAME clip-shot-recognition-double-click-tests
        COMMAND clip-shot-recognition-window-tests --ocr-double-click-only -platform offscreen)
    set_tests_properties(clip-shot-recognition-double-click-tests PROPERTIES
        LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-recognition-selection-only-rendering-tests
        COMMAND clip-shot-recognition-window-tests --selection-only-rendering -platform offscreen)
    add_test(NAME clip-shot-recognition-ocr-hover-tests
        COMMAND clip-shot-recognition-window-tests --ocr-hover-only -platform offscreen)
    add_test(
        NAME clip-shot-recognition-window-tests
        COMMAND clip-shot-recognition-window-tests
    )
    if(WIN32)
        set_tests_properties(clip-shot-recognition-window-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    else()
        set_tests_properties(clip-shot-recognition-window-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        )
    endif()

    clip_shot_add_ocr_runtime_tests()

    add_executable(clip-shot-ocr-layout-tests
        tests/screenshot_ocr_layout_tests.cpp
        src/presentation/ocr/screenshotocrlayout.cpp
    )
    target_include_directories(clip-shot-ocr-layout-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-ocr-layout-tests PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-ocr-layout-tests COMMAND clip-shot-ocr-layout-tests)
    set_tests_properties(clip-shot-ocr-layout-tests PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-recognition-session-controller-tests
        tests/screenshot_recognition_session_controller_tests.cpp
        tests/original_image_translation_tests.cpp
        tests/image_conversion_tests.cpp
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/screenshotocrpresentation.h
        include/clip_shot/presentation/screenshotrecognitionsessioncontroller.h
        include/clip_shot/presentation/screenshotrecognitionwindow.h
        include/clip_shot/presentation/screenshottabledocument.h
        include/clip_shot/presentation/screenshottableeditor.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrtexteditingsession.cpp
        src/presentation/ocr/screenshotocrtexttransform.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp
        src/presentation/ocr/screenshotrecognitionsessioncontroller.cpp
        src/presentation/ocr/screenshotocrlayout.cpp
        src/presentation/ocr/screenshotrecognitionwindow.cpp
        src/presentation/ocr/screenshottabledocument.cpp
        src/presentation/ocr/screenshottableeditor.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        src/presentation/overlay/screenshotcanvasrenderer.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/services/languagemanager.cpp
        src/presentation/styles/buttonborder.cpp
    )
    clip_shot_link_macos_window_platform(clip-shot-recognition-session-controller-tests)
    target_include_directories(clip-shot-recognition-session-controller-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CLIP_DRAW_ENGINE_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-recognition-session-controller-tests PRIVATE
        clip_shot_translation
        ant_design_qt
        Qt6::Network
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        clip_shot_image_codec
        clip_shot_storage
        clip_shot_window_shortcuts
    )
    if(WIN32)
        target_link_libraries(clip-shot-recognition-session-controller-tests PRIVATE
            user32
        )
    endif()
    add_test(
        NAME clip-shot-recognition-session-controller-tests
        COMMAND clip-shot-recognition-session-controller-tests
    )
    set_tests_properties(clip-shot-recognition-session-controller-tests PROPERTIES
        TIMEOUT 120
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    if(WIN32)
        set_tests_properties(clip-shot-recognition-session-controller-tests PROPERTIES
            ENVIRONMENT
                "QT_QPA_PLATFORM=windows;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QWindowsIntegrationPlugin>"
        )
    else()
        set_tests_properties(clip-shot-recognition-session-controller-tests PROPERTIES
            ENVIRONMENT
                "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        )
    endif()

    foreach(_original_image_test IN ITEMS screenshot-tool-palette recognition-window recognition-session-controller)
        add_test(NAME clip-shot-${_original_image_test}-original-image-tests
            COMMAND clip-shot-${_original_image_test}-tests --original-image-only -platform offscreen)
        set_tests_properties(clip-shot-${_original_image_test}-original-image-tests
            PROPERTIES LABELS "unit" TIMEOUT 90)
    endforeach()

    add_test(NAME clip-shot-recognition-defaults-tests
        COMMAND clip-shot-recognition-session-controller-tests --ocr-defaults-only -platform offscreen)
    set_tests_properties(clip-shot-recognition-defaults-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_test(NAME clip-shot-recognition-image-snapshot-tests
        COMMAND clip-shot-recognition-window-tests --image-snapshot-only -platform offscreen)
    set_tests_properties(clip-shot-recognition-image-snapshot-tests PROPERTIES LABELS "unit" TIMEOUT 60)

    add_test(NAME clip-shot-original-image-translation-tests
        COMMAND clip-shot-recognition-session-controller-tests --translation-only)
    set_tests_properties(clip-shot-original-image-translation-tests PROPERTIES
        TIMEOUT 120
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
    add_test(NAME clip-shot-image-conversion-tests
        COMMAND clip-shot-recognition-session-controller-tests --image-conversion-only -platform offscreen)
    set_tests_properties(clip-shot-image-conversion-tests PROPERTIES TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-image-conversion-storage-tests
        COMMAND clip-shot-application-storage-tests --image-conversion-only)
    set_tests_properties(clip-shot-image-conversion-storage-tests PROPERTIES TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-image-conversion-toolbar-tests
        COMMAND clip-shot-screenshot-tool-palette-tests --image-conversion-only -platform offscreen)
    set_tests_properties(clip-shot-image-conversion-toolbar-tests PROPERTIES TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-image-conversion-pinned-tests
        COMMAND clip-shot-pinned-window-tests --image-conversion-only)
    set_tests_properties(clip-shot-image-conversion-pinned-tests PROPERTIES TIMEOUT 60
        ENVIRONMENT "QT_QPA_PLATFORM=windows"
        LABELS "unit;windows"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-table-recognition-session-tests
        COMMAND clip-shot-recognition-session-controller-tests --table-only -platform offscreen)
    set_tests_properties(clip-shot-table-recognition-session-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    clip_shot_import_offscreen_platform(clip-shot-recognition-session-controller-tests)
    get_target_property(_translation_tool_sources clip-shot-color-picker-lifecycle-tests SOURCES)
    get_target_property(_translation_session_sources clip-shot-recognition-session-controller-tests SOURCES)
    list(APPEND _translation_tool_sources ${_translation_session_sources})
    list(FILTER _translation_tool_sources EXCLUDE REGEX "^tests/")
    list(REMOVE_DUPLICATES _translation_tool_sources)
    add_executable(clip-shot-translation-tool-tests ${_translation_tool_sources}
        tests/screenshot_translation_tool_tests.cpp
        include/clip_shot/presentation/screenshotocrcontroller.h
        src/presentation/ocr/screenshotocrcontroller.cpp)
    target_include_directories(clip-shot-translation-tool-tests PRIVATE include src/image)
    target_link_libraries(clip-shot-translation-tool-tests PRIVATE clip_shot_translation
        clip_shot_diagnostics clip_shot_storage clip_shot_image_codec clip_shot_window_shortcuts
        clip_draw_engine_qt_canvas ant_design_qt adqt_icon_core ant_design_icons_qt
        Qt6::Widgets Qt6::Network Qt6::GuiPrivate)
    clip_shot_link_macos_window_platform(clip-shot-translation-tool-tests)
    if(WIN32)
        target_link_libraries(clip-shot-translation-tool-tests PRIVATE user32 comctl32)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-translation-tool-tests)
    add_test(NAME clip-shot-translation-tool-tests
        COMMAND clip-shot-translation-tool-tests -platform offscreen)
    set_tests_properties(clip-shot-translation-tool-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    unset(_translation_tool_sources)
    unset(_translation_session_sources)
    add_executable(clip-shot-qr-recognition-service-tests
        tests/screenshot_qr_recognition_service_tests.cpp
        include/clip_shot/presentation/screenshotqrrecognitionservice.h
        src/presentation/ocr/screenshotqrrecognitionservice.cpp
    )
    target_include_directories(clip-shot-qr-recognition-service-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-qr-recognition-service-tests PRIVATE
        clip_canvas_image
        opencv_wechat_qrcode
        opencv_objdetect
        Qt6::Core
        Qt6::Gui
    )
    if(APPLE)
        target_link_libraries(clip-shot-qr-recognition-service-tests PRIVATE clip_shot_rust_ffi_selector)
    endif()
    add_test(
        NAME clip-shot-qr-recognition-service-tests
        COMMAND clip-shot-qr-recognition-service-tests
    )
    set_tests_properties(clip-shot-qr-recognition-service-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(clip-shot-selected-text-translation-tests
        tests/selected_text_translation_tests.cpp)
    target_link_libraries(clip-shot-selected-text-translation-tests PRIVATE clip_shot_translation)
    add_test(NAME clip-shot-selected-text-translation-tests
        COMMAND clip-shot-selected-text-translation-tests)
    set_tests_properties(clip-shot-selected-text-translation-tests PROPERTIES LABELS "unit" TIMEOUT 30)

    # Compile the production backend against a fake C boundary; do not link the Rust runtime.
    add_executable(clip-shot-selected-text-strategy-tests
        tests/selected_text_strategy_tests.cpp
        src/presentation/services/selectedtexttranslationcontroller.cpp
        include/clip_shot/presentation/selectedtexttranslationcontroller.h)
    target_include_directories(clip-shot-selected-text-strategy-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-selected-text-c/include")
    target_link_libraries(clip-shot-selected-text-strategy-tests PRIVATE Qt6::Core)
    add_test(NAME clip-shot-selected-text-strategy-tests
        COMMAND clip-shot-selected-text-strategy-tests)
    set_tests_properties(clip-shot-selected-text-strategy-tests PROPERTIES LABELS "unit" TIMEOUT 30)

    add_executable(clip-shot-translation-page-controller-tests
        tests/translation_page_controller_tests.cpp
        tests/translation_test_support.h
    )
    target_link_libraries(clip-shot-translation-page-controller-tests PRIVATE clip_shot_translation)
    add_test(NAME clip-shot-translation-page-controller-tests
             COMMAND clip-shot-translation-page-controller-tests)
    set_tests_properties(clip-shot-translation-page-controller-tests PROPERTIES LABELS "unit")

    add_executable(clip-shot-translation-service-tests tests/translation_service_tests.cpp)
    target_link_libraries(clip-shot-translation-service-tests PRIVATE clip_shot_translation)
    add_test(NAME clip-shot-translation-service-tests COMMAND clip-shot-translation-service-tests)
    set_tests_properties(clip-shot-translation-service-tests PROPERTIES LABELS "unit")

    get_target_property(_translation_window_sources clip-shot-about-page-tests SOURCES)
    list(REMOVE_ITEM _translation_window_sources tests/about_page_tests.cpp)
    add_executable(clip-shot-translation-page-tests
        tests/translation_page_tests.cpp
        src/presentation/components/screenshottranslationsettingsdialog.cpp
        tests/translation_test_support.h
        ${_translation_window_sources}
    )
    target_link_libraries(clip-shot-translation-page-tests PRIVATE clip_shot_settings)
    target_compile_definitions(clip-shot-translation-page-tests PRIVATE
        CLIP_SHOT_TEST_TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    )
    add_dependencies(clip-shot-translation-page-tests clip_shot_release_translations)
    clip_shot_import_offscreen_platform(clip-shot-translation-page-tests)
    add_test(NAME clip-shot-translation-page-tests COMMAND clip-shot-translation-page-tests)
    add_test(NAME clip-shot-screenshot-translation-settings-dialog-tests
        COMMAND clip-shot-translation-page-tests --screenshot-settings)
    set_tests_properties(clip-shot-screenshot-translation-settings-dialog-tests PROPERTIES
        LABELS "unit" TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-translation-page-tests PROPERTIES
        LABELS "unit"
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
    )

    add_executable(clip-shot-standalone-translation-tests
        tests/standalone_translation_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-standalone-translation-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    if(WIN32)
        target_link_libraries(clip-shot-standalone-translation-tests PRIVATE PkgConfig::CLIP_SHOT_FFMPEG)
    endif()
    target_compile_definitions(clip-shot-standalone-translation-tests PRIVATE
        CLIP_SHOT_TEST_TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}")
    add_dependencies(clip-shot-standalone-translation-tests clip_shot_release_translations)
    clip_shot_import_offscreen_platform(clip-shot-standalone-translation-tests)
    add_test(NAME clip-shot-standalone-translation-tests COMMAND clip-shot-standalone-translation-tests)
    set_tests_properties(clip-shot-standalone-translation-tests PROPERTIES
        LABELS "unit" ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")

    add_executable(clip-shot-translation-page-streaming-tests
        tests/translation_page_streaming_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-translation-page-streaming-tests PRIVATE
        clip_shot_settings clip_shot_image_codec)
    if(WIN32)
        target_link_libraries(clip-shot-translation-page-streaming-tests PRIVATE
            PkgConfig::CLIP_SHOT_FFMPEG)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-translation-page-streaming-tests)
    add_test(NAME clip-shot-translation-page-streaming-tests
             COMMAND clip-shot-translation-page-streaming-tests)
    set_tests_properties(clip-shot-translation-page-streaming-tests PROPERTIES
        LABELS "unit" ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")

    if(CLIP_APPS_BUILD_BENCHMARKS)
        add_executable(clip-shot-table-preparation-benchmark
            tests/table_preparation_performance_benchmark.cpp)
        target_include_directories(clip-shot-table-preparation-benchmark PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(clip-shot-table-preparation-benchmark PRIVATE
            clip_shot_translation clip_shot_diagnostics Qt6::Core Qt6::Gui Qt6::Network
            clip_shot_image_codec)
    endif()

    add_executable(clip-shot-api-client-tests
        tests/clipshot_api_client_tests.cpp
    )
    target_include_directories(clip-shot-api-client-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-api-client-tests PRIVATE
        clip_shot_translation
        clip_shot_diagnostics
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        clip_shot_image_codec
    )
    add_test(
        NAME clip-shot-api-client-tests
        COMMAND clip-shot-api-client-tests
    )
    set_tests_properties(clip-shot-api-client-tests PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    if(WIN32)
        # This UIA end-to-end driver is Qt-free; give it the Qt-free dialog
        # guard so the guard does not add a Qt runtime dependency.
        set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_native)
        add_executable(clip-shot-scrolling-thumbnail-uia-e2e-test
            tests/scrolling_thumbnail_uia_e2e_test.cpp
        )
        set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_qt)
        target_include_directories(clip-shot-scrolling-thumbnail-uia-e2e-test PRIVATE
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-stitch-images-c/include"
        )
        target_link_libraries(clip-shot-scrolling-thumbnail-uia-e2e-test PRIVATE
            clip_stitch_images_c
            Ole32
            Uiautomationcore
            Gdi32
            User32
            Userenv
            Ntdll
            Ws2_32
        )
        add_dependencies(clip-shot-scrolling-thumbnail-uia-e2e-test clip_shot)
        add_test(
            NAME clip-shot-scrolling-thumbnail-uia-e2e-test
            COMMAND clip-shot-scrolling-thumbnail-uia-e2e-test $<TARGET_FILE:clip_shot>
        )
        add_test(
            NAME clip-shot-scrolling-thumbnail-capture-excluded-uia-e2e-test
            COMMAND clip-shot-scrolling-thumbnail-uia-e2e-test
                $<TARGET_FILE:clip_shot>
                --capture-excluded
        )
        set_tests_properties(clip-shot-scrolling-thumbnail-uia-e2e-test PROPERTIES
            LABELS "e2e;uia;interactive"
            RUN_SERIAL TRUE
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        set_tests_properties(clip-shot-scrolling-thumbnail-capture-excluded-uia-e2e-test PROPERTIES
            LABELS "e2e;uia;interactive;windows"
            RUN_SERIAL TRUE
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )

        # This UIA end-to-end driver is Qt-free; give it the Qt-free dialog
        # guard so the guard does not add a Qt runtime dependency.
        set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_native)
        add_executable(clip-shot-screenshot-ctrl-c-uia-e2e-test
            tests/screenshot_ctrl_c_uia_e2e_test.cpp
        )
        set_property(DIRECTORY PROPERTY LINK_LIBRARIES clip_test_cli_guard_qt)
        target_link_libraries(clip-shot-screenshot-ctrl-c-uia-e2e-test PRIVATE
            Ole32
            Uiautomationcore
            User32
        )
        add_dependencies(clip-shot-screenshot-ctrl-c-uia-e2e-test clip_shot)
        add_test(
            NAME clip-shot-screenshot-ctrl-c-uia-e2e-test
            COMMAND clip-shot-screenshot-ctrl-c-uia-e2e-test $<TARGET_FILE:clip_shot>
        )
        set_tests_properties(clip-shot-screenshot-ctrl-c-uia-e2e-test PROPERTIES
            LABELS "e2e;uia;interactive"
            RUN_SERIAL TRUE
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
    endif()

    foreach(_offscreen_qpa_test_target IN ITEMS
        clip-shot-floating-toolbar-drag-tests
        clip-shot-language-manager-tests
        clip-shot-shortcut-key-row-tests
        clip-shot-form-layout-tests
        clip-shot-selection-resize-workflow-tests
        clip-shot-screenshot-canvas-renderer-tests
        clip-shot-screenshot-tool-palette-tests
        clip-shot-application-icon-tests
        clip-shot-toolbar-main-panel-tests
        clip-shot-screen-recording-geometry-tests
        clip-shot-screen-recording-area-window-tests
        clip-shot-screen-recording-shortcut-tests
        clip-shot-window-capture-exclusion-tests
        clip-shot-ocr-presentation-tests
    )
        clip_shot_import_offscreen_platform(${_offscreen_qpa_test_target})
    endforeach()
    if(NOT WIN32)
        foreach(_offscreen_qpa_test_target IN ITEMS
            clip-shot-system-tray-controller-tests
            clip-shot-context-menu-rendering-tests
            clip-shot-screenshot-selection-toolbar-hover-tests
            clip-shot-floating-toolbar-drag-tests
            clip-shot-adqt-dpi-stable-tests
            clip-shot-generated-icon-pack-tests
            clip-shot-sidebar-widget-tests
            clip-shot-main-content-header-tests
            clip-shot-recognition-window-tests
            clip-shot-recognition-session-controller-tests
            clip-shot-storage-status-widget-tests
            clip-shot-configuration-transfer-widget-tests
        )
            clip_shot_import_offscreen_platform(${_offscreen_qpa_test_target})
        endforeach()
    endif()

    foreach(_storage_test_target IN ITEMS
        clip-shot-language-manager-tests
        clip-shot-system-tray-controller-tests
        clip-shot-shortcut-key-row-tests
        clip-shot-selection-resize-workflow-tests
        clip-shot-screenshot-history-tests
        clip-shot-pinned-window-tests
        clip-shot-screenshot-tool-palette-tests
        clip-shot-screenshot-toolbar-fill-color-hover-tests
        clip-shot-screenshot-toolbar-render-benchmark
        clip-shot-floating-toolbar-drag-tests
        clip-shot-sidebar-widget-tests
        clip-shot-toolbar-main-panel-tests
    )
        if(TARGET ${_storage_test_target})
            target_link_libraries(${_storage_test_target} PRIVATE clip_shot_storage)
        endif()
    endforeach()

    if(_CLIP_SHOT_RUNTIME_OUTPUT_DIRECTORY_WAS_DEFINED)
        set(
            CMAKE_RUNTIME_OUTPUT_DIRECTORY
            "${_CLIP_SHOT_SAVED_RUNTIME_OUTPUT_DIRECTORY}"
        )
    else()
        unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    endif()
    unset(_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT)
    unset(_offscreen_qpa_test_target)
    unset(_CLIP_SHOT_SAVED_RUNTIME_OUTPUT_DIRECTORY)
    unset(_CLIP_SHOT_RUNTIME_OUTPUT_DIRECTORY_WAS_DEFINED)

endif()

function(clip_shot_configure_scrolling_replay_target target)
    target_sources(${target} PRIVATE
        include/clip_shot/presentation/screenshotscrollingthumbnailwidget.h
        tests/scrolling_image_replay.cpp
        src/presentation/capture/screenshotscrollingpipeline.cpp
        src/presentation/capture/screenshotscrollingthumbnailwidget.cpp
    )
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-stitch-images-c/include"
    )
    target_compile_definitions(${target} PRIVATE
        CLIP_SHOT_BENCH_INTERNALS=1
        CLIP_SHOT_SCROLLING_PERF_INSTRUMENTATION=1
        CLIP_SCROLLING_DEFAULT_IMAGE="${CMAKE_CURRENT_SOURCE_DIR}/test-imgs/scrollscreenshot-test.png"
        CLIP_SCROLLING_DEFAULT_OUTPUT="${CMAKE_BINARY_DIR}/scrolling-image-performance"
        CLIP_SCROLLING_BUILD_CONFIG="$<CONFIG>"
    )
    if(CLIP_SHOT_SCROLLING_PERF_DETAIL)
        target_compile_definitions(${target} PRIVATE
            CLIP_SHOT_SCROLLING_PERF_DETAIL=1 CLIP_STITCH_PERF_INSTRUMENTATION=1)
    endif()
    target_link_libraries(${target} PRIVATE
        clip_stitch_images_c clip_shot_image_codec clip_shot_diagnostics ant_design_qt Qt6::Widgets)
    if(WIN32)
        target_link_libraries(${target} PRIVATE Ntdll Ws2_32)
        target_link_libraries(${target} PRIVATE PkgConfig::CLIP_SHOT_FFMPEG)
    endif()
    clip_shot_import_offscreen_platform(${target})
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-bin")
endfunction()

if(APPLE AND CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-permission-guide-benchmark MACOSX_BUNDLE
        tests/macos_permission_guide_tests.mm
        src/presentation/services/screenshotclipboardservice.cpp)
    target_compile_definitions(clip-shot-permission-guide-benchmark PRIVATE
        CLIP_PERMISSION_GUIDE_BENCHMARK=1)
    target_link_libraries(clip-shot-permission-guide-benchmark PRIVATE
        clip_shot_settings "-framework AppKit")
    set_target_properties(clip-shot-permission-guide-benchmark PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.clipshot.tests.permission-guide-benchmark"
        MACOSX_BUNDLE_BUNDLE_NAME "Clip Permission Guide Benchmark")
endif()

if(CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-window-group-switcher-benchmark
        tests/window_group_switcher_benchmark.cpp
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
        include/clip_shot/presentation/windowgroupswitcherpopup.h
        src/presentation/components/windowgroupswitcherpopup.cpp)
    target_include_directories(clip-shot-window-group-switcher-benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-window-group-switcher-benchmark PRIVATE clip_shot_storage ant_design_qt Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-window-group-switcher-benchmark)
    add_test(NAME clip-shot-window-group-switcher-benchmark COMMAND clip-shot-window-group-switcher-benchmark)
    set_tests_properties(clip-shot-window-group-switcher-benchmark PROPERTIES
        LABELS benchmark ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
endif()

if(CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-settings-page-performance-benchmark
        tests/settings_page_performance_benchmark.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-settings-page-performance-benchmark PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-settings-page-performance-benchmark)

    add_executable(clip-shot-main-page-performance-benchmark
        tests/main_page_performance_benchmark.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-main-page-performance-benchmark PRIVATE
        clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-main-page-performance-benchmark)

    add_executable(clip-shot-main-window-skin-performance-benchmark
        tests/main_window_skin_performance_benchmark.cpp)
    target_link_libraries(clip-shot-main-window-skin-performance-benchmark PRIVATE
        clip_shot_main_window_skin clip_shot_storage clip_shot_image_codec clip_draw_engine_qt_canvas)
    clip_shot_import_offscreen_platform(clip-shot-main-window-skin-performance-benchmark)

    add_executable(clip-shot-main-window-no-skin-performance-benchmark
        tests/main_window_no_skin_performance_benchmark.cpp
        include/clip_shot/presentation/mainwindow.h
        include/clip_shot/presentation/components/contentcardwidget.h
        include/clip_shot/presentation/components/maincontentheaderwidget.h
        include/clip_shot/presentation/components/sidebarwidget.h
        include/clip_shot/presentation/components/titlebarwidget.h
        src/presentation/mainwindow.cpp
        src/presentation/components/contentcardwidget.cpp
        src/presentation/components/maincontentheaderwidget.cpp
        src/presentation/components/sidebarwidget.cpp
        src/presentation/components/titlebarwidget.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/platform/windows/windowchrome.cpp)
    if(APPLE)
        target_sources(clip-shot-main-window-no-skin-performance-benchmark PRIVATE
            src/platform/macos/applicationactivation.mm)
        target_link_libraries(clip-shot-main-window-no-skin-performance-benchmark PRIVATE
            "-framework AppKit")
    endif()
    target_link_libraries(clip-shot-main-window-no-skin-performance-benchmark PRIVATE
        clip_shot_settings clip_shot_image_codec clip_shot_updates)
    clip_shot_import_offscreen_platform(clip-shot-main-window-no-skin-performance-benchmark)
    clip_shot_add_translations(clip-shot-main-window-no-skin-performance-benchmark)

    add_executable(clip-shot-pinned-window-management-performance-benchmark
        tests/pinned_window_management_performance_benchmark.cpp)
    target_link_libraries(clip-shot-pinned-window-management-performance-benchmark PRIVATE
        clip_shot_storage Qt6::Core Qt6::Gui)
    if(MSVC)
        # Text-source and bulk-removal cases do not call the image codec. Delay its
        # loading so unrelated HEIF codec startup does not affect these measurements.
        target_link_options(clip-shot-pinned-window-management-performance-benchmark PRIVATE
            "/DELAYLOAD:clip_shot_image_codec_backend.dll")
        target_link_libraries(clip-shot-pinned-window-management-performance-benchmark PRIVATE
            delayimp)
    endif()
    add_test(NAME clip-shot-pinned-window-management-performance-benchmark
        COMMAND clip-shot-pinned-window-management-performance-benchmark)
    set_tests_properties(clip-shot-pinned-window-management-performance-benchmark PROPERTIES
        LABELS "benchmark" TIMEOUT 180
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-ocr-background-performance-benchmark
        tests/screenshot_ocr_background_performance_benchmark.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp)
    target_include_directories(clip-shot-ocr-background-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-ocr-background-performance-benchmark PRIVATE
        Qt6::Gui clip_draw_engine_qt_canvas)
    add_test(NAME clip-shot-ocr-background-performance-benchmark
        COMMAND clip-shot-ocr-background-performance-benchmark)
    set_tests_properties(clip-shot-ocr-background-performance-benchmark PROPERTIES
        LABELS "benchmark" TIMEOUT 120
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-recognition-image-performance-benchmark
        tests/screenshot_recognition_image_performance_benchmark.cpp
        ${CLIP_SHOT_RECOGNITION_IMAGE_RENDER_SOURCES})
    target_include_directories(clip-shot-recognition-image-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(clip-shot-recognition-image-performance-benchmark PRIVATE
        adqt::widgets Qt6::Gui clip_canvas_image)
    clip_shot_import_offscreen_platform(clip-shot-recognition-image-performance-benchmark)
    qt_import_plugins(clip-shot-recognition-image-performance-benchmark
        NO_DEFAULT INCLUDE Qt6::QOffscreenIntegrationPlugin)

    add_executable(clip-shot-memory-performance-benchmark
        tests/screenshot_memory_performance_benchmark.cpp
        src/presentation/services/screenshotclipboardcontent.cpp
        src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp
        ${CLIP_SHOT_RECOGNITION_IMAGE_RENDER_SOURCES})
    target_include_directories(clip-shot-memory-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_definitions(clip-shot-memory-performance-benchmark PRIVATE
        CLIP_SHOT_BENCHMARK_MANAGED_READER=1)
    target_link_libraries(clip-shot-memory-performance-benchmark PRIVATE
        adqt::widgets clip_shot_image_codec clip_shot_diagnostics Qt6::Widgets)
    if(WIN32)
        target_link_libraries(clip-shot-memory-performance-benchmark PRIVATE user32 psapi)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-memory-performance-benchmark)
    add_test(NAME clip-shot-memory-performance-benchmark
        COMMAND clip-shot-memory-performance-benchmark -platform offscreen)
    set_tests_properties(clip-shot-memory-performance-benchmark PROPERTIES
        LABELS "benchmark" RUN_SERIAL TRUE TIMEOUT 180
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-translation-page-streaming-benchmark
        tests/translation_page_streaming_benchmark.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-translation-page-streaming-benchmark PRIVATE
        clip_shot_settings clip_shot_image_codec)
    if(WIN32)
        target_link_libraries(clip-shot-translation-page-streaming-benchmark PRIVATE
            PkgConfig::CLIP_SHOT_FFMPEG)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-translation-page-streaming-benchmark)

    add_executable(clip-shot-scrolling-image-performance-benchmark
        tests/scrolling_image_performance_benchmark.cpp)
    clip_shot_configure_scrolling_replay_target(clip-shot-scrolling-image-performance-benchmark)
    add_executable(clip-shot-scrolling-hover-preview-benchmark
        tests/scrolling_hover_preview_benchmark.cpp)
    clip_shot_configure_scrolling_replay_target(clip-shot-scrolling-hover-preview-benchmark)
    add_executable(clip-shot-scrolling-frame-handoff-benchmark
        tests/scrolling_frame_handoff_benchmark.cpp)
    target_include_directories(clip-shot-scrolling-frame-handoff-benchmark PRIVATE
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-stitch-images-c/include")
    target_link_libraries(clip-shot-scrolling-frame-handoff-benchmark PRIVATE clip_stitch_images_c)
    target_compile_definitions(clip-shot-scrolling-frame-handoff-benchmark PRIVATE
        CLIP_SCROLLING_BUILD_CONFIG="$<CONFIG>")
    set_target_properties(clip-shot-scrolling-frame-handoff-benchmark PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-bin")
    if(CLIP_SHOT_SCROLLING_PERF_DETAIL)
        target_compile_definitions(clip-shot-scrolling-frame-handoff-benchmark PRIVATE
            CLIP_SHOT_SCROLLING_PERF_DETAIL=1 CLIP_STITCH_PERF_INSTRUMENTATION=1)
    endif()
    if(WIN32)
        target_link_libraries(clip-shot-scrolling-frame-handoff-benchmark PRIVATE psapi)
        target_compile_definitions(clip-shot-scrolling-frame-handoff-benchmark PRIVATE NOMINMAX)
    endif()
    add_test(NAME clip-shot-scrolling-frame-handoff-benchmark
        COMMAND clip-shot-scrolling-frame-handoff-benchmark)
    set_tests_properties(clip-shot-scrolling-frame-handoff-benchmark PROPERTIES
        LABELS "benchmark" RUN_SERIAL TRUE TIMEOUT 120)
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-scrolling-hover-preview-tests tests/scrolling_hover_preview_tests.cpp)
    target_include_directories(clip-shot-scrolling-hover-preview-tests PRIVATE src)
    target_link_libraries(clip-shot-scrolling-hover-preview-tests PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-scrolling-hover-preview-tests
        COMMAND clip-shot-scrolling-hover-preview-tests)
    set_tests_properties(clip-shot-scrolling-hover-preview-tests PROPERTIES LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-scrolling-hover-geometry-tests
        tests/scrolling_hover_geometry_tests.cpp
        include/clip_shot/presentation/screenshotscrollingthumbnailwidget.h
        src/presentation/capture/screenshotscrollingthumbnailwidget.cpp)
    target_include_directories(clip-shot-scrolling-hover-geometry-tests PRIVATE include)
    target_compile_definitions(clip-shot-scrolling-hover-geometry-tests PRIVATE
        CLIP_SHOT_BENCH_INTERNALS=1)
    target_link_libraries(clip-shot-scrolling-hover-geometry-tests PRIVATE
        clip_canvas_image ant_design_qt Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-scrolling-hover-geometry-tests)
    add_test(NAME clip-shot-scrolling-hover-geometry-tests
        COMMAND clip-shot-scrolling-hover-geometry-tests -platform offscreen)
    set_tests_properties(clip-shot-scrolling-hover-geometry-tests PROPERTIES LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")

    add_executable(clip-shot-scrolling-image-replay-tests tests/scrolling_image_replay_tests.cpp)
    clip_shot_configure_scrolling_replay_target(clip-shot-scrolling-image-replay-tests)
    add_test(NAME clip-shot-scrolling-image-replay-tests COMMAND clip-shot-scrolling-image-replay-tests)
    add_test(NAME clip-shot-scrolling-snapshot-lifetime-tests
        COMMAND clip-shot-scrolling-image-replay-tests --snapshot-teardown-only)
    set_tests_properties(clip-shot-scrolling-snapshot-lifetime-tests PROPERTIES
        LABELS "unit" ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    add_test(NAME clip-shot-scrolling-thumbnail-drag-tests
        COMMAND clip-shot-scrolling-image-replay-tests --thumbnail-drag-only)
    add_test(NAME clip-shot-scrolling-hover-pipeline-tests
        COMMAND clip-shot-scrolling-image-replay-tests --hover-preview-only)
    set_tests_properties(clip-shot-scrolling-hover-pipeline-tests PROPERTIES LABELS "unit"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(clip-shot-scrolling-thumbnail-drag-tests PROPERTIES LABELS "unit")
    set_tests_properties(clip-shot-scrolling-image-replay-tests PROPERTIES
        LABELS "unit" ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()

if(CLIP_SHOT_BUILD_BENCHMARKS)
    # This executable deliberately is not registered with CTest. It creates a
    # native widget and reports hardware-sensitive timing for explicit
    # Release performance sessions on Windows and macOS.
    add_executable(clip-shot-selection-render-benchmark
        tests/screenshot_selection_render_benchmark.cpp
        include/clip_shot/presentation/screenshotcanvasrenderer.h
        include/clip_shot/presentation/screenshotselectionshadowrenderer.h
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        src/presentation/ocr/screenshotocrtexttransform.cpp
        src/presentation/ocr/screenshotocrlayout.cpp
        src/presentation/ocr/screenshottabledocument.cpp
        src/presentation/overlay/screenshotcanvasrenderer.cpp
        src/presentation/overlay/screenshotimagerendering.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
    )
    target_compile_definitions(clip-shot-selection-render-benchmark PRIVATE
        NOMINMAX
        CLIP_SHOT_BENCH_INTERNALS=1
    )
    target_include_directories(clip-shot-selection-render-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CLIP_SHOT_DRAW_ENGINE_QT_DIR}/src/rendering"
        "${CLIP_SHOT_DRAW_ENGINE_QT_DIR}/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-shot-selection-render-benchmark PRIVATE
        Qt6::Widgets
        ant_design_qt
        clip_draw_engine_qt_canvas
    )
    if(WIN32)
        target_link_libraries(clip-shot-selection-render-benchmark PRIVATE dwmapi user32)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-selection-render-benchmark)
endif()

if(WIN32 AND CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-selection-toolbar-performance-benchmark
        tests/screenshot_selection_toolbar_performance_benchmark.cpp
        include/clip_shot/presentation/screenshotselectiontoolbarwidget.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/components/icons/clipshoticons.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/selection/screenshotselectiontoolbarwidget.cpp
        src/presentation/selection/screenshotselectiontoolbarwidgets.h
        src/presentation/selection/screenshotselectiontoolbarwidgets.cpp)
    target_include_directories(clip-shot-selection-toolbar-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/selection")
    target_link_libraries(clip-shot-selection-toolbar-performance-benchmark PRIVATE
        adqt::widgets adqt_icon_core ant_design_icons_qt Qt6::Widgets clip_draw_engine_qt_canvas
        user32)
    clip_shot_import_offscreen_platform(clip-shot-selection-toolbar-performance-benchmark)

    add_executable(clip-shot-decoration-area-benchmark
        tests/screenshot_decoration_area_benchmark.cpp)
    target_link_libraries(clip-shot-decoration-area-benchmark PRIVATE
        Qt6::Widgets clip_draw_engine_qt_canvas)

    # This executable deliberately is not registered with CTest. It creates
    # real always-on-top toolbar windows and measures real DWM presentation
    # for explicit Release benchmarking sessions. It compiles instrumented
    # copies of the toolbar sources; production builds compile the perf
    # scopes to no-ops because CLIP_SHOT_TOOLBAR_PERF_INSTRUMENTATION is not
    # defined for them.
    add_executable(clip-shot-screenshot-toolbar-display-benchmark
        tests/screenshot_toolbar_display_benchmark.cpp
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenshottoolbarwindow.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
        src/presentation/tools/screenshottoolbarperfinstrumentation.h
        src/presentation/toolbar/screenshottoolbarwindow.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp
    )
    target_compile_definitions(
        clip-shot-screenshot-toolbar-display-benchmark
        PRIVATE
            NOMINMAX
            CLIP_SHOT_TOOLBAR_PERF_INSTRUMENTATION=1
    )
    target_include_directories(
        clip-shot-screenshot-toolbar-display-benchmark
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(
        clip-shot-screenshot-toolbar-display-benchmark
        PRIVATE
            adqt_icon_core
            ant_design_icons_qt
            ant_design_qt
            Qt6::Widgets
            clip_draw_engine_qt_canvas
            clip_shot_storage
            comctl32
            dwmapi
            user32
    )

    add_executable(clip-shot-history-performance-benchmark
        tests/screenshot_history_performance_benchmark.cpp
        include/clip_shot/presentation/components/contentcardwidget.h
        src/presentation/components/contentcardwidget.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_compile_definitions(clip-shot-history-performance-benchmark PRIVATE
        NOMINMAX
        CLIP_SHOT_HISTORY_BENCHMARK_BASELINE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/baselines/screenshot_history_performance.json"
    )
    target_include_directories(clip-shot-history-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-history-performance-benchmark PRIVATE
        clip_shot_settings
        clip_shot_storage
        clip_shot_settings_catalog
        ant_design_qt
        Qt6::Widgets
        Psapi
    )
    if(WIN32)
        target_link_libraries(clip-shot-history-performance-benchmark PRIVATE
            PkgConfig::CLIP_SHOT_FFMPEG
        )
    endif()

    # This executable deliberately is not registered with CTest. It publishes
    # generated images through the real Windows clipboard for explicit Release
    # performance sessions and leaves the final fixture on the clipboard.
    add_executable(clip-shot-clipboard-performance-benchmark
        tests/screenshot_clipboard_performance_benchmark.cpp
        include/clip_shot/presentation/screenshotclipboardservice.h
        include/clip_shot/presentation/screenshotdisplaysession.h
        include/clip_shot/presentation/screenshotexportservice.h
        include/clip_shot/presentation/screenshotgeometry.h
        include/clip_shot/presentation/screenshotresultcompositor.h
        include/clip_shot/presentation/screenshotselectionshadowrenderer.h
        include/clip_shot/presentation/screenshotselectionpin.h
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/services/screenshotclipboardperfinstrumentation.h
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotexportservice.cpp
        src/presentation/services/screenshotselectionpin.cpp
    )
    target_compile_definitions(clip-shot-clipboard-performance-benchmark PRIVATE
        NOMINMAX
        CLIP_SHOT_CLIPBOARD_PERF_INSTRUMENTATION=1
    )
    target_include_directories(clip-shot-clipboard-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-clipboard-performance-benchmark PRIVATE clip_shot_image_codec
        clip_shot_storage
        clip_shot_diagnostics
        adqt::widgets
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        Psapi
        user32
    )

    # This is intentionally not registered with CTest. It exercises the real
    # Save-to-File pixel preparation, encoding, and preview decode routes and
    # reports hardware-sensitive medians for explicit Release sessions.
    add_executable(clip-shot-save-to-file-performance-benchmark
        tests/screenshot_save_to_file_performance_benchmark.cpp
        src/presentation/services/screenshotsaveexportpipeline.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
    )
    target_include_directories(clip-shot-save-to-file-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/image"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/services"
    )
    target_link_libraries(clip-shot-save-to-file-performance-benchmark PRIVATE
        clip_shot_image_codec
        clip_shot_storage
        Qt6::Gui
    )

    # This is intentionally not registered with CTest. It drives the real
    # file-pin batch snapshot/decode/presentation pipeline with real pinned
    # windows and reports hardware-sensitive medians for explicit Release
    # sessions.
    add_executable(clip-shot-file-pin-batch-performance-benchmark
        tests/screenshot_file_pin_batch_performance_benchmark.cpp
        tests/screenshot_pinned_lifecycle_performance_benchmark.cpp
        src/presentation/pinned/screenshotpinnedclickthroughgeometry.cpp
        ${CLIP_SELECTED_FILES_SOURCES}
        src/presentation/services/screenshotfilepinbatch.cpp
        include/clip_shot/presentation/components/pathinput.h
        src/presentation/components/pathinput.cpp
        include/clip_shot/presentation/screenshotsavepreviewcanvas.h
        src/presentation/services/screenshotsaveasfiledialog.cpp
        src/presentation/services/screenshotsaveexportpipeline.cpp
        src/presentation/services/screenshotsavepreviewcanvas.cpp
        src/platform/physicalcursor.cpp
        include/clip_shot/presentation/screenshotfloatingtoolpalettewindow.h
        include/clip_shot/presentation/screenshotcanvascolorsampler.h
        include/clip_shot/presentation/screenshotcanvascolorsamplerwindow.h
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/screenshotpinnededitcontroller.h
        include/clip_shot/presentation/screenshotpinnedwindow.h
        include/clip_shot/presentation/pinnedwindowgroupmanager.h
        include/clip_shot/presentation/screenshotrecognitionsessioncontroller.h
        include/clip_shot/presentation/screenshotrecognitionwindow.h
        include/clip_shot/presentation/screenshottableeditor.h
        include/clip_shot/presentation/screenshotmessageservice.h
        include/clip_shot/presentation/screenshotselectiongeometry.h
        include/clip_shot/presentation/screenshotselectionmodel.h
        include/clip_shot/presentation/screenshotselectionparams.h
        include/clip_shot/presentation/screenshottoolpalette.h
        include/clip_shot/presentation/screenshottoolpalettehost.h
        include/clip_shot/presentation/styles/thememanager.h
        src/presentation/components/icons/clipshoticons.cpp
        src/platform/windows/monitorgeometry.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/core/screenshotcanvastoolstyles.cpp
        src/presentation/core/screenshotstylebinding.cpp
        src/presentation/ocr/screenshotocrpresentation.cpp
        src/presentation/ocr/screenshotocrvisuals.cpp
        src/presentation/ocr/screenshotocrtexteditingsession.cpp
        src/presentation/ocr/screenshotocrtexttransform.cpp
        src/presentation/ocr/screenshotrecognitionsessioncontroller.cpp
        src/presentation/ocr/screenshotocrlayout.cpp
        src/presentation/ocr/screenshotrecognitionwindow.cpp
        src/presentation/ocr/screenshottabledocument.cpp
        src/presentation/ocr/screenshottableeditor.cpp
        src/presentation/ocr/screenshotocrtextlayout.cpp
        src/presentation/overlay/screenshotcanvasrenderer.cpp
        src/presentation/overlay/screenshotselectionshadowrenderer.cpp
        src/presentation/services/screenshotcanvascolorsampler.cpp
        src/presentation/services/screenshotcanvascolorsamplerwindow.cpp
        src/presentation/services/screenshotclipboardcontent.cpp
        src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotimagefileservice.cpp
        src/presentation/services/screenshotrecognitionfileexport.cpp
        src/presentation/services/screenshotpdfexport.cpp
        src/presentation/services/screenshotmessageservice.cpp
        src/presentation/selection/screenshotselectiongeometry.cpp
        src/presentation/selection/screenshotselectionmodel.cpp
        src/presentation/selection/screenshotselectionparams.cpp
        src/presentation/pinned/screenshotpinnededitcontroller.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
        src/presentation/services/screenshotexportartifact.cpp
        src/presentation/services/screenshotexportpinnedsource.cpp
        src/presentation/ocr/screenshotrecognitionimage.cpp
        src/presentation/selection/screenshotselectionexportuiservices.cpp
        src/presentation/pinned/screenshotpinnednativegeometrycontroller.cpp
        src/presentation/pinned/screenshotpinnedresizegeometry.cpp
        src/presentation/pinned/screenshotpinnedrestoregeometry.cpp
        src/presentation/pinned/screenshotpinnedhidetotopcontroller.cpp
        src/presentation/pinned/screenshotpinnedwindow.cpp
        src/presentation/services/pinnedwindowgroupmanager.cpp
        src/presentation/pinned/screenshotpinnedinteraction.cpp
        include/clip_shot/presentation/pinnedwindowselectioncontroller.h
        src/presentation/pinned/pinnedwindowselectioncontroller.cpp
        src/presentation/pinned/pinnedwindowselectioninteraction.cpp
        src/presentation/pinned/pinnedwindowselectiongeometry.cpp
        src/presentation/pinned/screenshotpinneddragexport.cpp
        ${CLIP_PINNED_PLATFORM_SOURCES}
        src/presentation/services/languagemanager.cpp
        src/presentation/styles/buttonborder.cpp
        src/presentation/styles/themecolorscheme.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/tools/screenshotfloatingtoolpalettenative.cpp
        src/presentation/tools/screenshotfloatingtoolpalettewindow.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalette.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/tools/screenshottoolpalettehost.cpp
        src/presentation/tools/screenshottoolpalettestylecontrols.cpp
        src/presentation/tools/screenshottoolpalettestylecomponents.cpp
        src/presentation/tools/screenshottoolpalettestylemodel.cpp
        src/presentation/tools/screenshottoolpalettestylepresets.cpp
    )
    target_include_directories(clip-shot-file-pin-batch-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../clip_draw_engine_qt/crates/clip-draw-engine-c/include"
    )
    target_link_libraries(clip-shot-file-pin-batch-performance-benchmark PRIVATE
        ${CLIP_SELECTED_FILES_LIBRARIES}
        clip_shot_translation
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        Qt6::Network
        Qt6::Widgets
        clip_draw_engine_qt_canvas
        clip_shot_image_codec
        clip_shot_storage
        clip_shot_window_shortcuts
    )
    if(WIN32)
        target_link_libraries(clip-shot-file-pin-batch-performance-benchmark PRIVATE
            Qt6::GuiPrivate
            comctl32
            dwmapi
            user32
            ole32
            oleaut32
            shell32
            uuid
        )
    endif()
    if(QT_FEATURE_static)
        # Static Qt registers platform plugins at link time; the benchmark
        # presents real windows, so it must import the windows integration.
        target_link_libraries(clip-shot-file-pin-batch-performance-benchmark PRIVATE
            Qt6::QWindowsIntegrationPlugin
        )
        qt_import_plugins(clip-shot-file-pin-batch-performance-benchmark
            INCLUDE Qt6::QWindowsIntegrationPlugin
        )
    endif()
endif()

target_include_directories(clip_shot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-recording-c/include"
    "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-visual-region-detector-c/include"
    "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector-c/include"
    "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-stitch-images-c/include"
)
target_precompile_headers(clip_shot PRIVATE
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

target_link_libraries(
    clip_shot PRIVATE
        clip_recording_effects_c
        adqt_icon_core
        ant_design_icons_qt
        ant_design_qt
        clip_shot_settings
        clip_shot_storage
        clip_shot_window_shortcuts
        clip_draw_engine_qt_canvas
        clip_capture_c
        clip_recording_c
        clip_ui_selector_c
        clip_visual_region_detector_c
        clip_stitch_images_c
        opencv_wechat_qrcode
        opencv_objdetect
        Qt6::Widgets
        Qt6::Svg
        Qt6::Network
)

target_compile_definitions(clip_shot PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)
add_dependencies(clip_shot clip_ocr_process)

if(WIN32 AND CLIP_SHOT_BUILD_BENCHMARKS)
    clip_windows_rust_environment(_clip_shot_benchmark_environment "${CLIP_SHOT_RUST_TARGET}")
    list(APPEND _clip_shot_benchmark_environment "LIBCLANG_PATH=${CLIP_SHOT_LIBCLANG_DIR}")
    add_executable(clip-shot-uia-qt-performance-fixture
        tests/screenshot_uia_qt_performance_fixture.cpp
    )
    target_link_libraries(clip-shot-uia-qt-performance-fixture PRIVATE Qt6::Widgets user32)
    target_compile_definitions(clip-shot-uia-qt-performance-fixture PRIVATE NOMINMAX)

    add_custom_target(clip-shot-uia-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-ui-selector --example hit_bench --features windows-examples
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL
        VERBATIM
    )

    # A standalone selector archive avoids pulling the recording/FFmpeg umbrella
    # into this focused benchmark and matches the preserved baseline archive.
    set(_uia_phased_lib "${CMAKE_BINARY_DIR}/selector-cargo/${CLIP_SHOT_RUST_TARGET}/release/clip_ui_selector_c.lib")
    file(GLOB_RECURSE _uia_selector_sources CONFIGURE_DEPENDS
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector/src/*.rs"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector-c/src/*.rs")
    add_custom_command(OUTPUT "${_uia_phased_lib}"
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment} "CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/selector-cargo"
            "RUSTFLAGS=-C target-feature=+crt-static"
            "${CARGO_EXECUTABLE}" build --locked --release --target ${CLIP_SHOT_RUST_TARGET}
            --manifest-path "${CLIP_SHOT_CAPTURE_CRATES_DIR}/Cargo.toml" -p clip-ui-selector-c
        DEPENDS ${_uia_selector_sources}
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/Cargo.lock"
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/Cargo.toml"
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector/Cargo.toml"
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector-c/Cargo.toml"
        VERBATIM)
    add_custom_target(clip-shot-selector-phased-rust DEPENDS "${_uia_phased_lib}")
    add_executable(clip-shot-selector-phased-performance-benchmark
        tests/screenshot_selector_phased_performance_benchmark.cpp
        include/clip_shot/presentation/screenshotselectorcoordinator.h
        src/presentation/selector/screenshotselectorcoordinator.cpp
        src/presentation/selector/screenshotselectorserviceclient.cpp
        src/presentation/core/screenshotintelligentselectionmodel.cpp
        src/presentation/services/screenshotclipboardservice.cpp
    )
    target_include_directories(clip-shot-selector-phased-performance-benchmark PRIVATE
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-ui-selector-c/include")
    target_link_libraries(clip-shot-selector-phased-performance-benchmark PRIVATE
        clip_shot_settings "${_uia_phased_lib}" Qt6::Widgets
        user32 userenv ole32 oleaut32 oleacc gdi32 dwmapi ntdll ws2_32 bcrypt advapi32)
    add_dependencies(clip-shot-selector-phased-performance-benchmark clip-shot-selector-phased-rust)
    qt_import_plugins(clip-shot-selector-phased-performance-benchmark INCLUDE Qt6::QWindowsIntegrationPlugin)

    set(CLIP_SHOT_UIA_BASELINE_DIR "" CACHE PATH "Preserved pre-refactor selector sources and standalone Rust workspace for comparison.")
    if(CLIP_SHOT_UIA_BASELINE_DIR)
        set(_uia_baseline_lib "${CLIP_SHOT_UIA_BASELINE_DIR}/target/${CLIP_SHOT_RUST_TARGET}/release/clip_ui_selector_c.lib")
        add_custom_command(OUTPUT "${_uia_baseline_lib}"
            COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment} "CARGO_TARGET_DIR=${CLIP_SHOT_UIA_BASELINE_DIR}/target"
                "RUSTFLAGS=-C target-feature=+crt-static"
                "${CARGO_EXECUTABLE}" build --offline --release --target ${CLIP_SHOT_RUST_TARGET}
                --manifest-path "${CLIP_SHOT_UIA_BASELINE_DIR}/Cargo.toml" -p clip-ui-selector-c
            VERBATIM)
        add_custom_target(clip-shot-selector-baseline-rust DEPENDS "${_uia_baseline_lib}")
        add_executable(clip-shot-selector-baseline-performance-benchmark
            tests/screenshot_selector_phased_performance_benchmark.cpp
            "${CLIP_SHOT_UIA_BASELINE_DIR}/clip_shot/include/clip_shot/presentation/screenshotselectorcoordinator.h"
            "${CLIP_SHOT_UIA_BASELINE_DIR}/clip_shot/src/presentation/selector/screenshotselectorcoordinator.cpp"
            "${CLIP_SHOT_UIA_BASELINE_DIR}/clip_shot/src/presentation/selector/screenshotselectorserviceclient.cpp"
            src/presentation/core/screenshotintelligentselectionmodel.cpp
            src/presentation/services/screenshotclipboardservice.cpp)
        target_compile_definitions(clip-shot-selector-baseline-performance-benchmark PRIVATE CLIP_UIA_LEGACY_BENCHMARK)
        target_include_directories(clip-shot-selector-baseline-performance-benchmark BEFORE PRIVATE
            "${CLIP_SHOT_UIA_BASELINE_DIR}/clip_shot/include"
            "${CLIP_SHOT_UIA_BASELINE_DIR}/crates/clip-ui-selector-c/include")
        target_link_libraries(clip-shot-selector-baseline-performance-benchmark PRIVATE
            clip_shot_settings "${_uia_baseline_lib}" Qt6::Widgets
            user32 userenv ole32 oleaut32 oleacc gdi32 dwmapi ntdll ws2_32 bcrypt advapi32)
        add_dependencies(clip-shot-selector-baseline-performance-benchmark clip-shot-selector-baseline-rust)
        qt_import_plugins(clip-shot-selector-baseline-performance-benchmark INCLUDE Qt6::QWindowsIntegrationPlugin)
    endif()

    # Not registered with CTest: this generates final-container fixtures through
    # the streaming encoder used by direct recording and reports Release metrics.
    add_custom_target(clip-shot-direct-recording-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "FFMPEG_DIR=${CLIP_FFMPEG_ROOT}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-export --example direct_recording_benchmark
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(clip-shot-recording-trim-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "FFMPEG_DIR=${CLIP_FFMPEG_ROOT}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-export --example recording_trim_benchmark
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL VERBATIM)

    add_custom_target(clip-shot-recording-encoder-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "FFMPEG_DIR=${CLIP_FFMPEG_ROOT}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-export --example recording_encoder_benchmark
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(clip-shot-gif-encoding-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "FFMPEG_DIR=${CLIP_FFMPEG_ROOT}"
            "CLIP_GIF_BENCH_OUTPUT=${CMAKE_BINARY_DIR}/gif-encoding-performance"
            "${CARGO_EXECUTABLE}" test --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-export --test gif_encoding
            gif_encoding_release_comparison -- --ignored --exact --nocapture
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(clip-shot-recording-effects-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-effects --example effects_benchmark
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL VERBATIM)
    add_custom_target(clip-shot-recording-postprocessing-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-effects --example postprocessing_benchmark
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL VERBATIM)
    if(WIN32 AND TARGET clip-shot-screen-recording-controller-tests)
        target_sources(clip-shot-screen-recording-controller-tests PRIVATE
            tests/recording_effects_performance_benchmark.cpp)
        target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE
            CLIP_RECORDING_EFFECTS_BENCHMARK=1)
        add_dependencies(clip-shot-recording-effects-performance-benchmark
            clip-shot-screen-recording-controller-tests)
        add_test(NAME clip-shot-recording-effects-presentation-benchmark
            COMMAND clip-shot-screen-recording-controller-tests --effects-preview-performance -platform windows)
        set_tests_properties(clip-shot-recording-effects-presentation-benchmark PROPERTIES
            LABELS "benchmark;interactive;windows" RUN_SERIAL TRUE TIMEOUT 90
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CLIP_FFMPEG_ROOT}/bin")
        add_test(NAME clip-shot-recording-effects-rasterization-benchmark
            COMMAND "${CLIP_RUST_CARGO_TARGET_DIR}/${CLIP_SHOT_RUST_TARGET}/release/examples/effects_benchmark.exe")
        set_tests_properties(clip-shot-recording-effects-rasterization-benchmark PROPERTIES
            LABELS "benchmark;windows" RUN_SERIAL TRUE TIMEOUT 90)
    endif()

    # Measures the recording window startup phases (controller open ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¾ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ windows
    # shown ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¾ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ first paint ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¾ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ first motion-preview frame) through the
    # compile-time gated recording_perf instrumentation. Deterministic and
    # offscreen, so unlike the effects presentation benchmark it stays out of
    # the interactive label.
    if(TARGET clip-shot-screen-recording-controller-tests)
        target_sources(clip-shot-screen-recording-controller-tests PRIVATE
            tests/screen_recording_window_startup_performance_benchmark.cpp
            tests/screen_recording_window_startup_performance_benchmark.h
            src/presentation/recording/screenrecordingperfinstrumentation.cpp
            src/presentation/recording/screenrecordingperfinstrumentation.h)
        target_compile_definitions(clip-shot-screen-recording-controller-tests PRIVATE
            CLIP_RECORDING_WINDOW_STARTUP_BENCHMARK=1
            CLIP_SHOT_RECORDING_PERF_INSTRUMENTATION=1)
        add_test(NAME clip-shot-recording-window-startup-performance-benchmark
            COMMAND clip-shot-screen-recording-controller-tests
                --recording-window-startup-performance)
        set_tests_properties(clip-shot-recording-window-startup-performance-benchmark
            PROPERTIES LABELS "benchmark" RUN_SERIAL TRUE TIMEOUT 180
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()

    add_custom_target(clip-shot-keyboard-recording-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "FFMPEG_DIR=${CLIP_FFMPEG_ROOT}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-runtime --example keyboard_overlay_benchmark
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL
        VERBATIM
    )

    # Drives the real DirectRecordingSession path (capture stream, input
    # hooks, overlay compositing, encoder) against a temporary animated
    # fullscreen window with simulated mouse/keyboard input. The metric
    # groups are compile-time cargo features so each is measured without
    # the instrumentation overhead of the others.
    add_custom_target(clip-shot-realtime-recording-performance-benchmark
        COMMAND "${CMAKE_COMMAND}" -E env ${_clip_shot_benchmark_environment}
            "CARGO_TARGET_DIR=${CLIP_RUST_CARGO_TARGET_DIR}"
            "FFMPEG_DIR=${CLIP_FFMPEG_ROOT}"
            "${CARGO_EXECUTABLE}" build --locked --release
            --target ${CLIP_SHOT_RUST_TARGET}
            --package clip-recording-runtime
            --features bench-stage-timing,bench-compositor-timing,bench-pipeline-timing
            --example realtime_recording_benchmark
        WORKING_DIRECTORY "${CLIP_SHOT_CAPTURE_CRATES_DIR}"
        USES_TERMINAL
        VERBATIM
    )
    # Seizes the primary monitor and injects input; interactive label keeps it
    # out of default test presets.
    add_test(NAME clip-shot-realtime-recording-performance-benchmark
        COMMAND "${CLIP_RUST_CARGO_TARGET_DIR}/${CLIP_SHOT_RUST_TARGET}/release/examples/realtime_recording_benchmark.exe"
            --duration-seconds 4 --warmup-seconds 1)
    set_tests_properties(clip-shot-realtime-recording-performance-benchmark PROPERTIES
        LABELS "benchmark;interactive;windows" RUN_SERIAL TRUE TIMEOUT 300
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CLIP_FFMPEG_ROOT}/bin")

    add_executable(clip-shot-pin-to-screen-performance-benchmark
        tests/screenshot_pin_to_screen_performance_benchmark.cpp
    )
    target_include_directories(clip-shot-pin-to-screen-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-pin-to-screen-performance-benchmark PRIVATE
        Qt6::Core
        Qt6::Gui
        dwmapi
        gdi32
        ole32
        oleaut32
        psapi
        Uiautomationcore
        user32
    )
    target_compile_definitions(clip-shot-pin-to-screen-performance-benchmark PRIVATE NOMINMAX)

    target_compile_definitions(clip_shot PRIVATE
        CLIP_SHOT_PIN_PERF_INSTRUMENTATION=1
    )

    # Not registered with CTest: drives the real clip_shot.exe through UIA and
    # reports hardware-sensitive first/second-capture timing for explicit
    # Windows performance sessions.
    add_executable(clip-shot-capture-startup-performance-benchmark
        tests/screenshot_capture_startup_performance_benchmark.cpp
    )
    target_include_directories(clip-shot-capture-startup-performance-benchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(clip-shot-capture-startup-performance-benchmark PRIVATE
        Qt6::Core
        dwmapi
        gdi32
        ole32
        oleaut32
        psapi
        Uiautomationcore
        user32
    )
    target_compile_definitions(clip-shot-capture-startup-performance-benchmark PRIVATE NOMINMAX)

    target_compile_definitions(clip_shot PRIVATE
        CLIP_SHOT_CAPTURE_PERF_INSTRUMENTATION=1
    )
endif()

clip_shot_add_translations(clip_shot)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipShotUpdateUiTests.cmake")

if(WIN32)
    # These DLLs are consumed by the standalone OCR process that is staged
    # next to each host target, not by clip_shot itself. Static ONNX Runtime
    # delay-loads DirectML; dynamic builds derive the complete non-system DLL
    # closure from onnxruntime.dll instead of copying its entire bin directory.
    foreach(_ocr_runtime_target IN ITEMS clip_shot clip-shot-ocr-recognition-service-tests)
        if(TARGET ${_ocr_runtime_target})
            if(CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
                add_custom_command(TARGET ${_ocr_runtime_target} POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "$<IF:$<CONFIG:Debug>,${CLIP_SHOT_DIRECTML_RUNTIME_DLL_DEBUG},${CLIP_SHOT_DIRECTML_RUNTIME_DLL_RELEASE}>"
                        "$<TARGET_FILE_DIR:${_ocr_runtime_target}>/DirectML.dll"
                    COMMENT "Staging DirectML for ${_ocr_runtime_target}"
                    VERBATIM
                )
            else()
                # Resolve only the DLLs actually imported by ONNX Runtime and
                # its companions. This keeps unrelated vcpkg DLLs out of the
                # application directory and fails the build on a broken closure.
                _clip_shot_onnxruntime_link_items(_clip_shot_onnxruntime_dll_items)
                add_custom_command(TARGET ${_ocr_runtime_target} POST_BUILD
                    COMMAND "${CMAKE_COMMAND}"
                        "-DCOPY_FROM=${_clip_shot_onnxruntime_dll_items}"
                        "-DCOPY_DIR=$<TARGET_FILE_DIR:onnxruntime::onnxruntime>"
                        "-DCOPY_TO=$<TARGET_FILE_DIR:${_ocr_runtime_target}>"
                        -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/copy_runtime_dlls.cmake"
                    COMMENT "Staging dynamic ONNX Runtime closure for ${_ocr_runtime_target}"
                    VERBATIM
                )
                unset(_clip_shot_onnxruntime_dll_items)
            endif()
        endif()
    endforeach()
    unset(_ocr_runtime_target)
endif()

foreach(_ocr_process_host_target IN ITEMS clip_shot clip-shot-ocr-recognition-service-tests)
    if(TARGET ${_ocr_process_host_target})
        add_dependencies(${_ocr_process_host_target} clip_ocr_process)
        add_custom_command(TARGET ${_ocr_process_host_target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:clip_ocr_process>"
                "$<TARGET_FILE_DIR:${_ocr_process_host_target}>/$<TARGET_FILE_NAME:clip_ocr_process>"
            COMMENT "Staging standalone OCR process for ${_ocr_process_host_target}"
            VERBATIM
        )
        # Packaged installers ship the verified OCR payload (or, for the online
        # variant, its trusted manifest) next to the executable. Development
        # builds must stage the same payload explicitly, otherwise the managed
        # asset pipeline finds no manifest and rejects every OCR request with
        # "Text recognition components could not be prepared".
        if(WIN32 AND EXISTS "${CLIP_SHOT_OCR_ASSET_MANIFEST}")
            add_custom_command(TARGET ${_ocr_process_host_target} POST_BUILD
                COMMAND "${CMAKE_COMMAND}"
                    -D "CLIP_OCR_ASSET_MANIFEST=${CLIP_SHOT_OCR_ASSET_MANIFEST}"
                    -D "CLIP_OCR_PLATFORM=${CLIP_WINDOWS_PLATFORM}"
                    -D "CLIP_OCR_ARTIFACTS_DIR=${_clip_ocr_artifacts_directory}"
                    -D "CLIP_OCR_DESTINATION=$<TARGET_FILE_DIR:${_ocr_process_host_target}>/assets/ocr"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/StageClipShotOcrAssets.cmake"
                COMMENT "Staging verified OCR assets for ${_ocr_process_host_target}"
                VERBATIM
            )
        endif()
    endif()
endforeach()
unset(_ocr_process_host_target)

if(CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-qr-performance-benchmark
        tests/screenshot_qr_performance_benchmark.cpp
        src/presentation/ocr/screenshotqrcontroller.cpp
        include/clip_shot/presentation/screenshotqrcontroller.h
        src/presentation/ocr/screenshotqrrecognitionservice.cpp
        include/clip_shot/presentation/screenshotqrrecognitionservice.h
        src/presentation/components/icons/clipshoticons.cpp)
    target_include_directories(clip-shot-qr-performance-benchmark PRIVATE include)
    target_link_libraries(clip-shot-qr-performance-benchmark PRIVATE clip_draw_engine_qt_canvas
        ant_design_qt opencv_wechat_qrcode opencv_objdetect Qt6::Widgets)
    if(WIN32)
        target_link_libraries(clip-shot-qr-performance-benchmark PRIVATE psapi)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-qr-performance-benchmark)
endif()

# The in-process barcode recognition service loads the pinned WeChat QR
# models at runtime; stage them next to every host that links the service.
foreach(_qr_host_target IN ITEMS clip_shot clip-shot-qr-recognition-service-tests
        clip-shot-qr-performance-benchmark)
    if(TARGET ${_qr_host_target})
        add_custom_command(TARGET ${_qr_host_target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "$<TARGET_FILE_DIR:${_qr_host_target}>/assets/qrcode"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CLIP_SHOT_QR_MODELS_DIR}/detect.prototxt"
                "${CLIP_SHOT_QR_MODELS_DIR}/detect.caffemodel"
                "${CLIP_SHOT_QR_MODELS_DIR}/sr.prototxt"
                "${CLIP_SHOT_QR_MODELS_DIR}/sr.caffemodel"
                "$<TARGET_FILE_DIR:${_qr_host_target}>/assets/qrcode"
            COMMENT "Staging WeChat QR models for ${_qr_host_target}"
            VERBATIM
        )
    endif()
endforeach()
unset(_qr_host_target)

if(MSVC AND CLIP_SHOT_RELEASE_STATIC)
    target_link_options(clip_shot PRIVATE
        $<$<CONFIG:Release>:/MAP>
        $<$<CONFIG:Release>:/MAPINFO:EXPORTS>
    )
endif()

if(WIN32)
    target_link_libraries(clip_shot PRIVATE
        Qt6::GuiPrivate
        dwmapi
        advapi32
        bcrypt
        comctl32
        d3d11
        dxgi
        dxguid
        gdi32
        ole32
        oleacc
        oleaut32
        psapi
        runtimeobject
        user32
        userenv
        winmm
        windowsapp
        ws2_32
    )
endif()

if(WIN32)
    set(CLIP_SHOT_FFMPEG_BIN_DIR
        "${CLIP_FFMPEG_ROOT}/bin"
        CACHE PATH
        "Directory containing the FFmpeg runtime DLLs used by clip-screen-recorder."
    )
    if(VCPKG_HOST_TRIPLET)
        set(_clip_pkgconf_host_triplet "${VCPKG_HOST_TRIPLET}")
    else()
        clip_windows_normalize_architecture(_clip_pkgconf_host_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
        set(_clip_pkgconf_host_triplet "${_clip_pkgconf_host_arch}-windows")
    endif()
    find_program(PKG_CONFIG_EXECUTABLE
        NAMES pkgconf pkg-config
        HINTS "${VCPKG_INSTALLED_DIR}/${_clip_pkgconf_host_triplet}/tools/pkgconf"
        NO_DEFAULT_PATH
        REQUIRED
    )
    set(_CLIP_SHOT_SAVED_PKG_CONFIG_PATH "$ENV{PKG_CONFIG_PATH}")
    set(ENV{PKG_CONFIG_PATH} "${CLIP_FFMPEG_ROOT}/lib/pkgconfig")
    find_package(PkgConfig REQUIRED)
    set(_clip_ffmpeg_modules libavformat libavcodec libswresample libswscale libavutil)
    if(CLIP_WINDOWS_ARCHITECTURE STREQUAL "x64")
        # FFmpeg's static .pc files omit this private QSV dependency.
        list(APPEND _clip_ffmpeg_modules vpl)
    endif()
    pkg_check_modules(CLIP_SHOT_FFMPEG REQUIRED IMPORTED_TARGET GLOBAL ${_clip_ffmpeg_modules})
    set(ENV{PKG_CONFIG_PATH} "${_CLIP_SHOT_SAVED_PKG_CONFIG_PATH}")
    unset(_CLIP_SHOT_SAVED_PKG_CONFIG_PATH)
    # Canvas-only consumers also reach recording symbols in the bundled archive.
    foreach(_clip_rust_bundle IN ITEMS clip_shot_rust_ffi_bundle clip_shot_full_rust_ffi_bundle)
        if(TARGET ${_clip_rust_bundle})
            target_link_libraries(${_clip_rust_bundle} INTERFACE PkgConfig::CLIP_SHOT_FFMPEG)
        endif()
    endforeach()
    unset(_clip_rust_bundle)
    target_link_libraries(clip_shot PRIVATE PkgConfig::CLIP_SHOT_FFMPEG)
    if(TARGET clip-shot-screen-recording-controller-tests)
        target_link_libraries(clip-shot-screen-recording-controller-tests PRIVATE PkgConfig::CLIP_SHOT_FFMPEG)
        set_property(TEST clip-shot-screen-recording-controller-tests clip-shot-recording-effects-preview-tests APPEND PROPERTY
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CLIP_FFMPEG_ROOT}/bin")
    endif()
    if(TARGET clip-shot-screenshot-canvas-renderer-tests)
        target_link_libraries(clip-shot-screenshot-canvas-renderer-tests PRIVATE
            PkgConfig::CLIP_SHOT_FFMPEG
        )
    endif()
    # The toolbar benchmarks link the canvas engine, which pulls the Rust FFI
    # bundle and with it the FFmpeg import libraries.
    foreach(_clip_shot_ffmpeg_toolbar_benchmark IN ITEMS
            clip-shot-table-preparation-benchmark
            clip-shot-screenshot-toolbar-render-benchmark
            clip-shot-screenshot-toolbar-display-benchmark
            clip-shot-file-pin-batch-performance-benchmark)
        if(TARGET ${_clip_shot_ffmpeg_toolbar_benchmark})
            target_link_libraries(${_clip_shot_ffmpeg_toolbar_benchmark} PRIVATE
                PkgConfig::CLIP_SHOT_FFMPEG
            )
        endif()
    endforeach()
    unset(_clip_shot_ffmpeg_toolbar_benchmark)
    # The OCR background executables also reach the shared Rust bundle through the canvas.
    foreach(_clip_shot_ocr_background_target IN ITEMS
            clip-shot-ocr-background-tests
            clip-shot-ocr-background-performance-benchmark)
        if(TARGET ${_clip_shot_ocr_background_target})
            target_link_libraries(${_clip_shot_ocr_background_target} PRIVATE
                PkgConfig::CLIP_SHOT_FFMPEG)
            set_property(TEST ${_clip_shot_ocr_background_target} APPEND PROPERTY
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CLIP_FFMPEG_ROOT}/bin")
        endif()
    endforeach()
    unset(_clip_shot_ocr_background_target)
    if(EXISTS "${CLIP_SHOT_FFMPEG_BIN_DIR}" AND NOT CLIP_SHOT_RELEASE_STATIC)
        set(CLIP_SHOT_FFMPEG_DLL_PATTERNS
            "avcodec-*.dll"
            "avdevice-*.dll"
            "avfilter-*.dll"
            "avformat-*.dll"
            "avutil-*.dll"
            "swresample-*.dll"
            "swscale-*.dll"
        )
        set(CLIP_SHOT_FFMPEG_DLLS)
        foreach(_ffmpeg_dll_pattern IN LISTS CLIP_SHOT_FFMPEG_DLL_PATTERNS)
            file(GLOB _ffmpeg_matching_dlls
                CONFIGURE_DEPENDS "${CLIP_SHOT_FFMPEG_BIN_DIR}/${_ffmpeg_dll_pattern}"
            )
            list(APPEND CLIP_SHOT_FFMPEG_DLLS ${_ffmpeg_matching_dlls})
        endforeach()
        # FFmpeg is built with the release CRT even in Debug. Resolve its DLL
        # imports from its own bin directory, including dependencies whose names
        # differ from the application's Debug codecs (zlib1.dll, libvpl.dll).
        set(_clip_ffmpeg_runtime_manifest
            "${CMAKE_CURRENT_BINARY_DIR}/generated/ffmpeg-runtime.cmake")
        execute_process(COMMAND "${CMAKE_COMMAND}"
            "-DCLIP_RUNTIME_ROOT_LIBRARIES=${CLIP_SHOT_FFMPEG_DLLS}"
            "-DCLIP_RUNTIME_MANIFEST=${_clip_ffmpeg_runtime_manifest}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/StageRuntimeDependencies.cmake"
            COMMAND_ERROR_IS_FATAL ANY)
        include("${_clip_ffmpeg_runtime_manifest}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            ${CLIP_RUNTIME_FILES})
        set(CLIP_SHOT_FFMPEG_RUNTIME_FILES ${CLIP_RUNTIME_FILES}
            CACHE INTERNAL "FFmpeg runtime files installed with Clip.")
        unset(CLIP_RUNTIME_FILES)
    endif()
endif()

function(clip_shot_stage_ffmpeg_runtime target)
    if(WIN32 AND NOT CLIP_SHOT_RELEASE_STATIC AND CLIP_SHOT_FFMPEG_RUNTIME_FILES)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CLIP_SHOT_FFMPEG_RUNTIME_FILES}
                "$<TARGET_FILE_DIR:${target}>" COMMAND_EXPAND_LISTS VERBATIM)
    endif()
endfunction()

if(MSVC)
    # Parallel CL invocations otherwise race over the target's shared PDB.
    target_compile_options(clip_shot PRIVATE /FS /utf-8)
endif()

if(CLIP_SHOT_ENABLE_STRICT_COMPILE)
    # The directory-wide policy above covers clip_shot too. Keep only the
    # executable's additional GCC/Clang diagnostics here.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(clip_shot PRIVATE
            -Wextra-semi
            -Wredundant-decls
            -Wunreachable-code
            -Wzero-as-null-pointer-constant
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(clip_shot PRIVATE
            -Wcomma
            -Wdocumentation
            -Wimplicit-int-conversion
            -Wnewline-eof
            -Wreserved-identifier
            -Wshorten-64-to-32
            -Wthread-safety
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(clip_shot PRIVATE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()
endif()

set_target_properties(clip_shot PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.clipshot.clip_shot
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
if(MSVC AND CLIP_SHOT_RELEASE_STATIC AND NOT CLIP_SHOT_QT_STATIC)
    # The shared Qt kit's entry-point shim is built with /MD. Use the static
    # CRT startup directly so the /MT release does not mix CRT variants.
    set_property(TARGET clip_shot PROPERTY qt_no_entrypoint TRUE)
    target_link_options(clip_shot PRIVATE /ENTRY:mainCRTStartup)
endif()

if(WIN32 AND NOT CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
    get_property(_clip_shot_directory_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_clip_shot_directory_target IN LISTS _clip_shot_directory_targets)
        get_target_property(_clip_shot_directory_target_type
            "${_clip_shot_directory_target}" TYPE)
        if(_clip_shot_directory_target_type STREQUAL "EXECUTABLE")
            _clip_shot_target_uses_image_codec(
                "${_clip_shot_directory_target}"
                _clip_shot_needs_image_runtime
                ""
            )
            if(_clip_shot_needs_image_runtime)
                _clip_shot_stage_image_runtime("${_clip_shot_directory_target}")
            endif()
        endif()
    endforeach()
    unset(_clip_shot_directory_target)
    unset(_clip_shot_directory_target_type)
    unset(_clip_shot_directory_targets)
    unset(_clip_shot_needs_image_runtime)
endif()

include(GNUInstallDirs)
if(APPLE)
    set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ClipShot)
    set(CMAKE_INSTALL_BINDIR "clip_shot.app/Contents/MacOS")
    set(CMAKE_INSTALL_LIBDIR "clip_shot.app/Contents/Frameworks")
    set(CMAKE_INSTALL_DATADIR "clip_shot.app/Contents/Resources")
endif()
install(TARGETS clip_shot
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(PROGRAMS "$<TARGET_FILE:clip_ocr_process>"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(EXISTS "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}")
    install(FILES "${CLIP_SHOT_CAMERA_SHUTTER_AUDIO_SOURCE}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/audios"
        RENAME camera_shutter.mp3
    )
endif()
if(NOT CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
    install(TARGETS clip_shot_image_codec_backend
        RUNTIME_DEPENDENCY_SET clip_shot_image_runtime_dependencies
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    if(WIN32)
        install(RUNTIME_DEPENDENCY_SET clip_shot_image_runtime_dependencies
            DIRECTORIES
                "$<TARGET_FILE_DIR:clip_shot_image_codec_backend>"
                "${_CLIP_SHOT_IMAGE_RUNTIME_SEARCH_DIRECTORY}"
            PRE_EXCLUDE_REGEXES
                "api-ms-.*"
                "ext-ms-.*"
                "msvcp.*\\.dll"
                "ucrtbased\\.dll"
                "vcruntime.*\\.dll"
            POST_EXCLUDE_REGEXES
                ".*[\\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\\/][Ss][Yy][Ss][Tt][Ee][Mm]32[\\\\/].*"
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()
if(WIN32 AND NOT CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
    # Dynamic ONNX Runtime for the standalone OCR process; the static release
    # links the runtime into clip-ocr-process.exe instead.
    install(FILES "$<TARGET_FILE:onnxruntime::onnxruntime>"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(WIN32)
    # DirectML remains a runtime dependency of the standalone OCR process;
    # even the statically linked ONNX Runtime delay-loads it for the
    # DirectML execution provider.
    install(FILES "${CLIP_SHOT_DIRECTML_RUNTIME_DLL_RELEASE}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# WeChat QR models for the in-process barcode recognition service; the runtime
# resolves them relative to the executable as assets/qrcode.
install(FILES
    "${CLIP_SHOT_QR_MODELS_DIR}/detect.prototxt"
    "${CLIP_SHOT_QR_MODELS_DIR}/detect.caffemodel"
    "${CLIP_SHOT_QR_MODELS_DIR}/sr.prototxt"
    "${CLIP_SHOT_QR_MODELS_DIR}/sr.caffemodel"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/assets/qrcode"
)

if(WIN32 AND NOT CLIP_SHOT_RELEASE_STATIC)
    clip_shot_stage_ffmpeg_runtime(clip_shot)
    if(CLIP_SHOT_FFMPEG_RUNTIME_FILES)
        install(FILES ${CLIP_SHOT_FFMPEG_RUNTIME_FILES}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()

if(WIN32 AND NOT CLIP_SHOT_QT_STATIC)
    set(_clip_shot_qt_deploy_arguments)
    # The release staging policy keeps only the platform and Schannel TLS
    # plugins. Clip uses its project-owned resources and statically linked
    # image codecs, so Qt's optional image, style, and generic plugins are not
    # needed in the installer.
    list(APPEND _clip_shot_qt_deploy_arguments
        EXCLUDE_PLUGIN_TYPES
            generic
            iconengines
            imageformats
            networkinformation
            styles
        EXCLUDE_PLUGINS
            qcertonlybackend
    )
    if(CLIP_SHOT_RELEASE_STATIC)
        list(APPEND _clip_shot_qt_deploy_arguments
            NO_COMPILER_RUNTIME
            DEPLOY_TOOL_OPTIONS
                --release
        )
    endif()
    qt_generate_deploy_app_script(
        TARGET clip_shot
        OUTPUT_SCRIPT CLIP_SHOT_QT_DEPLOY_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
        ${_clip_shot_qt_deploy_arguments}
    )
    install(SCRIPT "${CLIP_SHOT_QT_DEPLOY_SCRIPT}")

    # Qt's deployment helper copies every Qt base translation catalog. Clip
    # Shot ships only en_US, zh_CN, and zh_TW application catalogs, so retain
    # only the corresponding Qt catalogs in the final install tree.
    install(CODE [=[
        set(_clip_shot_qt_translation_dir
            "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/translations")
        set(_clip_shot_allowed_qt_translations
            qt_en.qm
            qt_zh_CN.qm
            qt_zh_TW.qm
        )
        if(IS_DIRECTORY "${_clip_shot_qt_translation_dir}")
            file(GLOB _clip_shot_qt_translation_catalogs
                "${_clip_shot_qt_translation_dir}/qt_*.qm")
            foreach(_clip_shot_qt_catalog IN LISTS _clip_shot_qt_translation_catalogs)
                get_filename_component(_clip_shot_qt_catalog_name
                    "${_clip_shot_qt_catalog}" NAME)
                if(NOT _clip_shot_qt_catalog_name IN_LIST _clip_shot_allowed_qt_translations)
                    file(REMOVE "${_clip_shot_qt_catalog}")
                endif()
            endforeach()
        endif()
        unset(_clip_shot_qt_translation_dir)
        unset(_clip_shot_allowed_qt_translations)
        unset(_clip_shot_qt_translation_catalogs)
        unset(_clip_shot_qt_catalog)
        unset(_clip_shot_qt_catalog_name)
    ]=])

    # windeployqt deploys optional Qt plugins based on broad module metadata.
    # Clip only needs the Windows platform integration and the Schannel
    # TLS backend at runtime. Remove optional image, style, input,
    # network-information, icon-engine, and certificate-only backends.
    install(CODE [=[
        set(_clip_shot_qt_plugin_dir
            "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/plugins")
        set(_clip_shot_allowed_qt_plugins
            platforms/qwindows.dll
            tls/qschannelbackend.dll
        )
        if(IS_DIRECTORY "${_clip_shot_qt_plugin_dir}")
            file(GLOB_RECURSE _clip_shot_qt_plugin_files
                RELATIVE "${_clip_shot_qt_plugin_dir}"
                "${_clip_shot_qt_plugin_dir}/*.dll")
            foreach(_clip_shot_qt_plugin IN LISTS _clip_shot_qt_plugin_files)
                if(NOT _clip_shot_qt_plugin IN_LIST _clip_shot_allowed_qt_plugins)
                    file(REMOVE "${_clip_shot_qt_plugin_dir}/${_clip_shot_qt_plugin}")
                endif()
            endforeach()
        endif()
        unset(_clip_shot_qt_plugin_dir)
        unset(_clip_shot_allowed_qt_plugins)
        unset(_clip_shot_qt_plugin_files)
        unset(_clip_shot_qt_plugin)
    ]=])
endif()

if(CLIP_SHOT_QT_STATIC)
    # Static Qt does not load these plugins from a deployment directory.
    # Import only the integrations required by the Windows release binary.
    qt_import_plugins(clip_shot
        NO_DEFAULT
        INCLUDE ${_clip_shot_static_qt_plugins}
    )
endif()

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/COPYRIGHT"
    "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses"
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../ant_design_qt/THIRD_PARTY_NOTICES.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses/third-party/project-notices/ant-design-icons"
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip_image/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip_image/COPYRIGHT"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses/components/clip-image"
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../ant_design_qt/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ant_design_qt/COPYRIGHT"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses/components/ant-design-qt"
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip-crates/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip-crates/COPYRIGHT"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses/components/clip-crates"
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip_draw_engine_qt/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip_draw_engine_qt/COPYRIGHT"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses/components/clip-draw-engine-qt"
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip_rust_ffi/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../clip_rust_ffi/COPYRIGHT"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses/components/clip-rust-ffi"
)
install(DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/third-party-licenses/third-party/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/clip-shot/licenses/third-party"
    OPTIONAL
)

foreach(_conversion_view_target IN ITEMS clip_shot clip-shot-recognition-window-tests
        clip-shot-recognition-session-controller-tests clip-shot-translation-tool-tests
        clip-shot-pinned-window-tests
        clip-shot-file-pin-batch-performance-benchmark)
    if(TARGET ${_conversion_view_target})
        target_sources(${_conversion_view_target} PRIVATE
            include/clip_shot/presentation/screenshotimageconversion.h
            include/clip_shot/presentation/screenshotimageconversionview.h
            src/presentation/ocr/screenshotimageconversionview.cpp)
    endif()
endforeach()
foreach(_conversion_controller_target IN ITEMS clip_shot
        clip-shot-recognition-session-controller-tests clip-shot-translation-tool-tests
        clip-shot-pinned-window-tests
        clip-shot-file-pin-batch-performance-benchmark)
    if(TARGET ${_conversion_controller_target})
        target_sources(${_conversion_controller_target} PRIVATE
            include/clip_shot/presentation/components/screenshottranslationsettingsdialog.h
            src/presentation/components/screenshottranslationsettingsdialog.cpp
            include/clip_shot/presentation/screenshotimageconversioncontroller.h
            src/presentation/ocr/screenshotimageconversioncontroller.cpp)
        # Settings already owns these widget implementations and their Qt metaobjects.
        # Standalone conversion hosts compile them locally only without that library.
        get_target_property(_conversion_controller_libraries
            ${_conversion_controller_target} LINK_LIBRARIES)
        if(NOT "clip_shot_settings" IN_LIST _conversion_controller_libraries)
            target_sources(${_conversion_controller_target} PRIVATE
                include/clip_shot/presentation/components/formfields.h
                src/presentation/components/formfields.cpp
                include/clip_shot/presentation/components/pathinput.h
                src/presentation/components/pathinput.cpp)
            target_link_libraries(${_conversion_controller_target} PRIVATE
                clip_shot_settings_catalog)
            get_target_property(_dialog_host_sources ${_conversion_controller_target} SOURCES)
            if(NOT "${_dialog_host_sources}" MATCHES "styles/thememanager\\.cpp")
                target_sources(${_conversion_controller_target} PRIVATE
                    include/clip_shot/presentation/styles/thememanager.h
                    src/presentation/styles/thememanager.cpp
                    src/presentation/styles/themecolorscheme.cpp)
            endif()
        endif()
    endif()
endforeach()

# Standalone palette hosts need the same shared form fields as the scrolling settings dialog.
# Hosts linked to Settings already own their widget implementations and Qt metaobjects.
get_property(_palette_form_host_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_palette_form_host_target IN LISTS _palette_form_host_targets)
    get_target_property(_palette_form_host_sources ${_palette_form_host_target} SOURCES)
    if(NOT "${_palette_form_host_sources}" MATCHES "presentation/tools/screenshottoolpalette\\.cpp")
        continue()
    endif()
    get_target_property(_palette_form_host_libraries ${_palette_form_host_target} LINK_LIBRARIES)
    if(NOT "clip_shot_settings" IN_LIST _palette_form_host_libraries AND
       NOT "${_palette_form_host_sources}" MATCHES "presentation/components/formfields\\.cpp")
        target_sources(${_palette_form_host_target} PRIVATE
            include/clip_shot/presentation/components/formfields.h
            src/presentation/components/formfields.cpp
            include/clip_shot/presentation/components/pathinput.h
            src/presentation/components/pathinput.cpp)
        target_link_libraries(${_palette_form_host_target} PRIVATE clip_shot_settings_catalog)
    endif()
endforeach()

qt_finalize_executable(clip_shot)

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-auto-filter-tests
        tests/screenshot_auto_filter_tests.cpp
        src/presentation/tools/screenshotautofiltercontroller.cpp
        src/presentation/services/screenshotexportcoordinator.cpp
        include/clip_shot/presentation/screenshotautofiltercontroller.h)
    target_include_directories(clip-shot-auto-filter-tests PRIVATE include
        "${CLIP_SHOT_CLIP_IMAGE_DIR}/include"
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-visual-region-detector-c/include")
    target_link_libraries(clip-shot-auto-filter-tests PRIVATE clip_draw_engine_qt_canvas clip_visual_region_detector_c Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-auto-filter-tests)
    add_test(NAME clip-shot-auto-filter-tests COMMAND clip-shot-auto-filter-tests)
    set_tests_properties(clip-shot-auto-filter-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit;windows")
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_test(NAME clip-shot-auto-filter-toolbar-tests COMMAND clip-shot-screenshot-tool-palette-tests --auto-filter-only)
    set_tests_properties(clip-shot-auto-filter-toolbar-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit;windows")
endif()

foreach(_auto_filter_host_target IN ITEMS clip-shot-pinned-window-tests
        clip-shot-file-pin-batch-performance-benchmark)
    if(TARGET ${_auto_filter_host_target})
        target_sources(${_auto_filter_host_target} PRIVATE
            src/presentation/tools/screenshotautofiltercontroller.cpp
            include/clip_shot/presentation/screenshotautofiltercontroller.h)
        target_include_directories(${_auto_filter_host_target} PRIVATE
            "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-visual-region-detector-c/include")
        target_link_libraries(${_auto_filter_host_target} PRIVATE clip_visual_region_detector_c)
    endif()
endforeach()
if(CLIP_SHOT_BUILD_TESTS)
    add_test(NAME clip-shot-auto-filter-pinned-tests
        COMMAND clip-shot-pinned-window-tests --auto-filter-only -platform offscreen)
    set_tests_properties(clip-shot-auto-filter-pinned-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit;windows" TIMEOUT 60)
endif()

if(CLIP_SHOT_BUILD_TESTS)
    # These toolbar cases exercise Qt behavior on both Cocoa and Windows. Opt
    # them out of the repository's legacy name-based Windows label heuristic.
    set_tests_properties(
        clip-shot-pinned-toolbar-parity-tests
        clip-shot-pinned-toolbar-layout-tests
        clip-shot-interrupted-toolbar-drag-tests
        clip-shot-floating-toolbar-keyboard-focus-tests
        clip-shot-floating-toolbar-action-layout-tests
        clip-shot-floating-toolbar-recording-hit-test-tests
        clip-shot-toolbar-popup-recovery-tests
        clip-shot-toolbar-color-input-tests
        clip-shot-toolbar-shared-controls-tests
        clip-shot-scrolling-toolbar-tests
        clip-shot-screen-recording-toolbar-tests
        PROPERTIES CLIP_TEST_CROSS_PLATFORM TRUE)
    if(APPLE)
        set_tests_properties(clip-shot-pinned-toolbar-parity-tests
            clip-shot-pinned-toolbar-layout-tests PROPERTIES
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    endif()
endif()

if(APPLE)
    include(ClipShotMacOS)
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-pinned-placement-tests tests/pinned_placement_tests.cpp
        src/presentation/pinned/screenshotpinnednativegeometrycontroller.cpp
        src/presentation/pinned/screenshotpinnedresizegeometry.cpp)
    target_include_directories(clip-shot-pinned-placement-tests PRIVATE include src)
    target_link_libraries(clip-shot-pinned-placement-tests PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-pinned-placement-tests COMMAND clip-shot-pinned-placement-tests)
    add_executable(clip-shot-feature-action-router-tests tests/feature_action_router_tests.cpp
        src/app/featureavailability.cpp)
    target_include_directories(clip-shot-feature-action-router-tests PRIVATE include)
    target_link_libraries(clip-shot-feature-action-router-tests PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-feature-action-router-tests COMMAND clip-shot-feature-action-router-tests)
    set_tests_properties(clip-shot-feature-action-router-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE)
    add_test(NAME clip-shot-pinned-controlled-interaction-tests
        COMMAND clip-shot-pinned-window-tests --controlled-interaction-only -platform offscreen)
    add_test(NAME clip-shot-pinned-pointer-routing-tests
        COMMAND clip-shot-pinned-window-tests --pointer-routing-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-controlled-interaction-tests PROPERTIES
        LABELS unit RUN_SERIAL TRUE TIMEOUT 60 CLIP_TEST_CROSS_PLATFORM TRUE)
    set_tests_properties(clip-shot-pinned-pointer-routing-tests PROPERTIES
        LABELS unit RUN_SERIAL TRUE TIMEOUT 60 CLIP_TEST_CROSS_PLATFORM TRUE)
    set_tests_properties(clip-shot-pinned-movement-shortcut-tests PROPERTIES
        CLIP_TEST_CROSS_PLATFORM TRUE)
endif()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_test(NAME clip-shot-macos-startup-tests COMMAND clip_shot --startup-probe)
    set_tests_properties(clip-shot-macos-startup-tests PROPERTIES
        LABELS "unit;macos" TIMEOUT 30)
    add_executable(clip-shot-macos-shell-policy-tests
        tests/macos_shell_policy_tests.cpp
        include/clip_shot/app/featureavailability.h
        src/app/featureavailability.cpp)
    target_include_directories(clip-shot-macos-shell-policy-tests PRIVATE include)
    target_link_libraries(clip-shot-macos-shell-policy-tests PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME clip-shot-macos-shell-policy-tests
        COMMAND clip-shot-macos-shell-policy-tests)
    set_tests_properties(clip-shot-macos-shell-policy-tests PROPERTIES
        LABELS "unit;macos" TIMEOUT 30)

    add_executable(clip-shot-macos-shell-native-tests
        tests/macos_shell_native_tests.mm
        include/clip_shot/platform/macos/applicationactivation.h
        src/platform/macos/applicationactivation.mm)
    target_include_directories(clip-shot-macos-shell-native-tests PRIVATE include)
    target_compile_definitions(clip-shot-macos-shell-native-tests PRIVATE
        CLIP_SHOT_MACOS_INFO_PLIST="${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in")
    target_link_libraries(clip-shot-macos-shell-native-tests PRIVATE
        clip_shot_login_item Qt6::Widgets "-framework AppKit")
    add_test(NAME clip-shot-macos-shell-native-tests
        COMMAND clip-shot-macos-shell-native-tests -platform cocoa)
    set_tests_properties(clip-shot-macos-shell-native-tests PROPERTIES
        LABELS "unit;macos" RUN_SERIAL TRUE TIMEOUT 30)

    add_executable(clip-shot-macos-pinned-window-tests
        tests/macos_pinned_window_tests.mm
        ${CLIP_PINNED_PLATFORM_SOURCES}
        src/presentation/services/mousereleaseactioncontroller.cpp)
    clip_shot_link_macos_window_platform(clip-shot-macos-pinned-window-tests)
    set_target_properties(clip-shot-macos-pinned-window-tests PROPERTIES
        MACOSX_BUNDLE TRUE MACOSX_BUNDLE_GUI_IDENTIFIER "top.clipshot.tests.pinned")
    target_include_directories(clip-shot-macos-pinned-window-tests PRIVATE include src)
    target_link_libraries(clip-shot-macos-pinned-window-tests PRIVATE Qt6::Widgets adqt::widgets
        "-framework AppKit" "-framework CoreGraphics")
    add_test(NAME clip-shot-pinned-auxiliary-ownership-tests
        COMMAND clip-shot-macos-pinned-window-tests --ownership-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-auxiliary-ownership-tests PROPERTIES
        LABELS "unit;macos" CLIP_TEST_CROSS_PLATFORM TRUE TIMEOUT 30)
    add_test(NAME clip-shot-macos-pinned-window-tests
        COMMAND clip-shot-macos-pinned-window-tests -platform cocoa)
    add_test(NAME clip-shot-macos-pinned-focus-tests
        COMMAND clip-shot-macos-pinned-window-tests --focus -platform cocoa)
    add_test(NAME clip-shot-macos-pinned-delivery-tests
        COMMAND clip-shot-macos-pinned-window-tests --delivery -platform cocoa)
    add_test(NAME clip-shot-macos-pinned-modal-delivery-tests
        COMMAND clip-shot-macos-pinned-window-tests --modal-delivery -platform cocoa)
    add_test(NAME clip-shot-macos-pinned-application-modal-delivery-tests
        COMMAND clip-shot-macos-pinned-window-tests --application-modal-delivery -platform cocoa)
    set_tests_properties(clip-shot-macos-pinned-window-tests clip-shot-macos-pinned-focus-tests clip-shot-macos-pinned-delivery-tests clip-shot-macos-pinned-modal-delivery-tests clip-shot-macos-pinned-application-modal-delivery-tests
        PROPERTIES LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()

if(CLIP_SHOT_BUILD_TESTS)
    set_tests_properties(clip-shot-pinned-window-repository-tests clip-shot-pinned-window-group-tests
        PROPERTIES CLIP_TEST_CROSS_PLATFORM TRUE)
endif()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    # Shared behavior fixtures synthesize Qt input. Real Cocoa delivery and
    # native window policy are covered by the serial interactive targets above.
    set_tests_properties(${_clip_shot_pinned_window_test_names}
        clip-shot-pinned-save-dialog-tests
        clip-shot-pinned-hidden-text-selection-restore-tests
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" CLIP_TEST_CROSS_PLATFORM TRUE)
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_test(NAME clip-shot-pinned-physical-authority-tests
        COMMAND clip-shot-pinned-window-tests --physical-authority-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-physical-authority-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_test(NAME clip-shot-pinned-passive-publication-tests
        COMMAND clip-shot-pinned-window-tests --passive-publication-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-passive-publication-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 30
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_test(NAME clip-shot-clipboard-appearance-presentation-tests
        COMMAND clip-shot-pinned-window-tests --clipboard-appearance-only -platform offscreen)
    set_tests_properties(clip-shot-clipboard-appearance-presentation-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_test(NAME clip-shot-decoded-content-presentation-tests
        COMMAND clip-shot-pinned-window-tests --decoded-content-only -platform offscreen)
    set_tests_properties(clip-shot-decoded-content-presentation-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_test(NAME clip-shot-pinned-color-space-tests
        COMMAND clip-shot-pinned-window-tests --color-space-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-color-space-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_test(NAME clip-shot-history-selection-presentation-tests
        COMMAND clip-shot-pinned-window-tests --history-selection-only -platform offscreen)
    set_tests_properties(clip-shot-history-selection-presentation-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
endif()

if(WIN32 AND CLIP_SHOT_BUILD_TESTS)
    # Qt does not activate a screen-factor override of 1 by itself. Use an
    # explicit 2x screen factor and a global multiplier to test exact DPRs,
    # independently of the host monitor's configured scale.
    set(_pin_dprs 1.0 1.25 1.5 1.75 2.0)
    set(_pin_factors 0.5 0.625 0.75 0.875 1.0)
    foreach(_pin_scale _pin_factor IN ZIP_LISTS _pin_dprs _pin_factors)
        add_test(NAME clip-shot-pinned-small-extent-${_pin_scale}-tests
            COMMAND clip-shot-pinned-window-tests --small-extent-only -platform windows)
        set_tests_properties(clip-shot-pinned-small-extent-${_pin_scale}-tests PROPERTIES
            LABELS "unit;windows" RUN_SERIAL TRUE TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_SCREEN_SCALE_FACTORS=2;QT_SCALE_FACTOR=${_pin_factor};CLIP_PIN_TEST_DPR=${_pin_scale}")
        add_test(NAME clip-shot-pinned-pixel-alignment-${_pin_scale}-tests
            COMMAND clip-shot-pinned-window-tests --pixel-alignment-only -platform windows)
        set_tests_properties(clip-shot-pinned-pixel-alignment-${_pin_scale}-tests PROPERTIES
            LABELS "unit;windows" RUN_SERIAL TRUE TIMEOUT 90
            ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_SCREEN_SCALE_FACTORS=2;QT_SCALE_FACTOR=${_pin_factor};CLIP_PIN_TEST_DPR=${_pin_scale}")
        add_test(NAME clip-shot-pinned-selection-content-alignment-${_pin_scale}-tests
            COMMAND clip-shot-pinned-window-tests --selection-content-alignment-only -platform windows)
        set_tests_properties(clip-shot-pinned-selection-content-alignment-${_pin_scale}-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 90
            ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_SCREEN_SCALE_FACTORS=2;QT_SCALE_FACTOR=${_pin_factor};CLIP_PIN_TEST_DPR=${_pin_scale}")
        add_test(NAME clip-shot-pinned-native-transparent-edges-${_pin_scale}-tests
            COMMAND clip-shot-pinned-window-tests --native-transparent-edges-only -platform windows)
        set_tests_properties(clip-shot-pinned-native-transparent-edges-${_pin_scale}-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 90
            ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_SCREEN_SCALE_FACTORS=2;QT_SCALE_FACTOR=${_pin_factor};CLIP_PIN_TEST_DPR=${_pin_scale}")
    endforeach()
    # A deterministic capture fixture published through the native layered
    # window. Also run at real monitor DPI, without inherited Qt overrides.
    add_test(NAME clip-shot-pinned-selection-content-alignment-tests
        COMMAND clip-shot-pinned-window-tests --selection-content-alignment-only -platform windows)
    set_tests_properties(clip-shot-pinned-selection-content-alignment-tests PROPERTIES
        LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 90
        ENVIRONMENT "QT_QPA_PLATFORM=windows"
        ENVIRONMENT_MODIFICATION "QT_SCREEN_SCALE_FACTORS=unset:;QT_SCALE_FACTOR=unset:;CLIP_PIN_TEST_DPR=unset:")
endif()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_test(NAME clip-shot-pinned-pixel-alignment-tests
        COMMAND clip-shot-pinned-window-tests --pixel-alignment-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-pixel-alignment-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE ENVIRONMENT "QT_SCALE_FACTOR=2" RUN_SERIAL TRUE)
    add_test(NAME clip-shot-pinned-retina-interaction-tests
        COMMAND clip-shot-pinned-window-tests --controlled-interaction-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-retina-interaction-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE ENVIRONMENT "QT_SCALE_FACTOR=2" RUN_SERIAL TRUE)
    add_test(NAME clip-shot-pinned-creation-tests
        COMMAND clip-shot-pinned-window-tests --creation-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-creation-tests PROPERTIES
        LABELS "unit;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-macos-pinned-creation-tests
        COMMAND clip-shot-pinned-window-tests --creation-only -platform cocoa)
    set_tests_properties(clip-shot-macos-pinned-creation-tests PROPERTIES
        LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-macos-pinned-drag-tests
        COMMAND clip-shot-pinned-window-tests --native-controlled-drag-only -platform cocoa)
    set_tests_properties(clip-shot-macos-pinned-drag-tests PROPERTIES
        LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE
        TIMEOUT 60 SKIP_RETURN_CODE 77)
    add_test(NAME clip-shot-macos-pinned-pixel-alignment-tests
        COMMAND clip-shot-pinned-window-tests --pixel-alignment-only -platform cocoa)
    set_tests_properties(clip-shot-macos-pinned-pixel-alignment-tests PROPERTIES
        LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60)
endif()

if(CLIP_SHOT_BUILD_TESTS)
    set_tests_properties(clip-shot-pinned-recognition-context-menu-tests
        clip-shot-image-conversion-pinned-tests clip-shot-auto-filter-pinned-tests
        PROPERTIES CLIP_TEST_CROSS_PLATFORM TRUE LABELS unit)
    if(APPLE)
        set_tests_properties(clip-shot-image-conversion-pinned-tests
            clip-shot-pinned-quick-save-tests
            PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" CLIP_TEST_CROSS_PLATFORM TRUE LABELS unit)
    endif()
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-selection-render-tests
        tests/screenshot_selection_render_tests.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/services/screenshotsourceimagecomposer.cpp)
    if(WIN32)
        target_sources(clip-shot-selection-render-tests PRIVATE
            src/platform/windows/monitorgeometry.cpp)
        target_link_libraries(clip-shot-selection-render-tests PRIVATE user32)
    endif()
    target_include_directories(clip-shot-selection-render-tests PRIVATE include
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include")
    target_link_libraries(clip-shot-selection-render-tests PRIVATE Qt6::Widgets clip_canvas_image)
    clip_shot_import_offscreen_platform(clip-shot-selection-render-tests)
    add_test(NAME clip-shot-selection-render-tests COMMAND clip-shot-selection-render-tests)
    set_tests_properties(clip-shot-selection-render-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit")
endif()

# Overlay and recording hosts share widget controls and their custom icon pack.
get_property(_overlay_widget_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_target IN LISTS _overlay_widget_targets)
    get_target_property(_sources ${_target} SOURCES)
    if(_sources MATCHES "screenrecordingareawindow.cpp")
        target_sources(${_target} PRIVATE
            src/presentation/recording/recordingregiondraghandle.cpp
            src/presentation/recording/recordingregiondraghandle.h)
    endif()
    if(WIN32 AND _sources MATCHES "screenshotoverlaywindow.cpp" AND
       NOT _sources MATCHES "src/platform/windows/windowchrome.cpp")
        target_sources(${_target} PRIVATE src/platform/windows/windowchrome.cpp)
        target_link_libraries(${_target} PRIVATE dwmapi user32)
    endif()
    if(NOT "${_target}" STREQUAL "clip_shot" AND
       _sources MATCHES "(screenshotoverlaywindow|screenrecordingareawindow).cpp" AND
       NOT _sources MATCHES "clipshoticons.cpp")
        target_sources(${_target} PRIVATE src/presentation/components/icons/clipshoticons.cpp)
    endif()
endforeach()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-macos-raster-color-space-tests
        tests/macos_raster_color_space_tests.mm)
    target_include_directories(clip-shot-macos-raster-color-space-tests PRIVATE include)
    target_link_libraries(clip-shot-macos-raster-color-space-tests PRIVATE Qt6::Widgets
        "-framework AppKit")
    clip_shot_import_offscreen_platform(clip-shot-macos-raster-color-space-tests)
    add_test(NAME clip-shot-raster-color-space-tests
        COMMAND clip-shot-macos-raster-color-space-tests -platform offscreen)
    set_tests_properties(clip-shot-raster-color-space-tests PROPERTIES LABELS "unit;macos")
    add_test(NAME clip-shot-macos-raster-color-space-tests
        COMMAND clip-shot-macos-raster-color-space-tests -platform cocoa)
    set_tests_properties(clip-shot-macos-raster-color-space-tests PROPERTIES LABELS "unit;macos")

    add_executable(clip-shot-macos-screenshot-window-target-tests
        tests/macos_screenshot_window_target_tests.mm)
    target_link_libraries(clip-shot-macos-screenshot-window-target-tests PRIVATE Qt6::Core
        "-framework Foundation" "-framework CoreGraphics")
    add_test(NAME clip-shot-macos-screenshot-window-target-tests
        COMMAND clip-shot-macos-screenshot-window-target-tests)
    set_tests_properties(clip-shot-macos-screenshot-window-target-tests PROPERTIES LABELS "unit;macos")

    add_executable(clip-shot-macos-screenshot-native-smoke
        tests/macos_screenshot_native_smoke.mm
        src/presentation/capture/directcapturenative.cpp
        src/presentation/core/screenshotgeometry.cpp
        src/presentation/services/screenshotsourceimagecomposer.cpp)
    clip_shot_link_macos_window_platform(clip-shot-macos-screenshot-native-smoke)
    target_include_directories(clip-shot-macos-screenshot-native-smoke PRIVATE include
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include")
    target_link_libraries(clip-shot-macos-screenshot-native-smoke PRIVATE Qt6::Widgets clip_capture_c clip_canvas_image
        clip_image::clip_image_static
        "-framework AppKit" "-framework ApplicationServices")
    add_test(NAME clip-shot-macos-screenshot-native-smoke COMMAND clip-shot-macos-screenshot-native-smoke)
    set_tests_properties(clip-shot-macos-screenshot-native-smoke PROPERTIES LABELS "interactive;macos" SKIP_RETURN_CODE 77 TIMEOUT 60)
endif()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-macos-region-shortcut-native-tests
        tests/macos_region_shortcut_native_tests.mm)
    target_include_directories(clip-shot-macos-region-shortcut-native-tests PRIVATE include)
    target_link_libraries(clip-shot-macos-region-shortcut-native-tests PRIVATE
        clip_shot_window_shortcuts Qt6::Widgets "-framework AppKit"
        "-framework ApplicationServices")
    add_test(NAME clip-shot-macos-region-shortcut-native-tests
        COMMAND clip-shot-macos-region-shortcut-native-tests)
    set_tests_properties(clip-shot-macos-region-shortcut-native-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=cocoa" LABELS "interactive;macos"
        RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 30)

    add_executable(clip-shot-macos-recapture-native-tests
        tests/macos_recapture_native_tests.mm
        src/platform/macos/applicationactivation.mm)
    clip_shot_link_macos_window_platform(clip-shot-macos-recapture-native-tests)
    target_include_directories(clip-shot-macos-recapture-native-tests PRIVATE include
        "${CLIP_SHOT_CAPTURE_CRATES_DIR}/crates/clip-capture-c/include")
    target_link_libraries(clip-shot-macos-recapture-native-tests PRIVATE Qt6::Widgets clip_capture_c
        clip_shot_login_item
        "-framework AppKit" "-framework ApplicationServices")
    add_test(NAME clip-shot-macos-recapture-native-tests COMMAND clip-shot-macos-recapture-native-tests)
    set_tests_properties(clip-shot-macos-recapture-native-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=cocoa" LABELS "interactive;macos"
        RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 30)
    foreach(target_kind IN ITEMS floating local local-floating)
        add_test(NAME clip-shot-macos-recapture-${target_kind}-tests
            COMMAND clip-shot-macos-recapture-native-tests --${target_kind})
        set_tests_properties(clip-shot-macos-recapture-${target_kind}-tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=cocoa" LABELS "interactive;macos"
            RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 30)
    endforeach()

    add_executable(clip-shot-macos-recapture-focus-tests tests/macos_recapture_focus_tests.cpp)
    target_include_directories(clip-shot-macos-recapture-focus-tests PRIVATE include)
    target_link_libraries(clip-shot-macos-recapture-focus-tests PRIVATE Qt6::Core)
    add_test(NAME clip-shot-macos-recapture-focus-tests COMMAND clip-shot-macos-recapture-focus-tests)
    set_tests_properties(clip-shot-macos-recapture-focus-tests PROPERTIES LABELS "unit;macos")

    add_executable(clip-shot-macos-scrolling-input-tests
        tests/macos_scrolling_input_tests.mm)
    clip_shot_link_macos_window_platform(clip-shot-macos-scrolling-input-tests)
    target_include_directories(clip-shot-macos-scrolling-input-tests PRIVATE include)
    target_link_libraries(clip-shot-macos-scrolling-input-tests PRIVATE Qt6::Widgets
        "-framework AppKit" "-framework ApplicationServices")
    add_test(NAME clip-shot-macos-scrolling-input-tests
        COMMAND clip-shot-macos-scrolling-input-tests)
    set_tests_properties(clip-shot-macos-scrolling-input-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit;macos")
    add_test(NAME clip-shot-macos-scrolling-input-native-tests
        COMMAND clip-shot-macos-scrolling-input-tests -platform cocoa)
    set_tests_properties(clip-shot-macos-scrolling-input-native-tests PROPERTIES
        LABELS "interactive;macos" RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 30)

    add_executable(clip-shot-macos-overlay-initialization-tests
        tests/macos_overlay_initialization_tests.mm
        ${CLIP_PINNED_PLATFORM_SOURCES}
        src/presentation/core/screenshotgeometry.cpp)
    clip_shot_link_macos_window_platform(clip-shot-macos-overlay-initialization-tests)
    target_include_directories(clip-shot-macos-overlay-initialization-tests PRIVATE include src)
    target_link_libraries(clip-shot-macos-overlay-initialization-tests PRIVATE ant_design_qt Qt6::Widgets
        "-framework AppKit" "-framework ApplicationServices")
    add_test(NAME clip-shot-macos-overlay-initialization-tests
        COMMAND clip-shot-macos-overlay-initialization-tests)
    set_tests_properties(clip-shot-macos-overlay-initialization-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit;macos")
    add_test(NAME clip-shot-macos-overlay-animation-tests
        COMMAND clip-shot-macos-overlay-initialization-tests --overlay-animation-only -platform offscreen)
    set_tests_properties(clip-shot-macos-overlay-animation-tests PROPERTIES LABELS "unit;macos")
    add_test(NAME clip-shot-macos-overlay-animation-native-tests
        COMMAND clip-shot-macos-overlay-initialization-tests --overlay-animation-only -platform cocoa)
    set_tests_properties(clip-shot-macos-overlay-animation-native-tests PROPERTIES
        LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 30)
    add_test(NAME clip-shot-macos-recognition-stacking-native-tests
        COMMAND clip-shot-macos-overlay-initialization-tests --recognition-stacking-only -platform cocoa)
    set_tests_properties(clip-shot-macos-recognition-stacking-native-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=cocoa" LABELS "interactive;macos"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-macos-screenshot-stacking-native-tests
        COMMAND clip-shot-macos-overlay-initialization-tests -platform cocoa)
    set_tests_properties(clip-shot-macos-screenshot-stacking-native-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=cocoa" LABELS "interactive;macos"
        RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-macos-modal-stacking-tests
        COMMAND clip-shot-macos-overlay-initialization-tests --modal-stacking-only -platform offscreen)
    set_tests_properties(clip-shot-macos-modal-stacking-tests PROPERTIES LABELS "unit;macos" TIMEOUT 60)
    add_test(NAME clip-shot-macos-modal-stacking-native-tests
        COMMAND clip-shot-macos-overlay-initialization-tests --modal-stacking-only -platform cocoa)
    set_tests_properties(clip-shot-macos-modal-stacking-native-tests PROPERTIES
        LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME clip-shot-macos-file-panel-stacking-tests
        COMMAND clip-shot-macos-overlay-initialization-tests --native-file-panels-only -platform offscreen)
    set_tests_properties(clip-shot-macos-file-panel-stacking-tests PROPERTIES
        LABELS "unit;macos" TIMEOUT 60)
    add_test(NAME clip-shot-macos-file-panel-stacking-native-tests
        COMMAND clip-shot-macos-overlay-initialization-tests --native-file-panels-only -platform cocoa)
    set_tests_properties(clip-shot-macos-file-panel-stacking-native-tests PROPERTIES
        LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
endif()

# Physical cursor warps enter Qt's platform input path so Qt's pointer state and
# widget grabs remain consistent with subsequent native mouse events.
foreach(_cursor_host IN ITEMS clip_shot clip-shot-physical-cursor-tests clip-shot-color-picker-lifecycle-tests
        clip-shot-pinned-window-tests clip-shot-file-pin-batch-performance-benchmark)
    if(TARGET ${_cursor_host})
        target_link_libraries(${_cursor_host} PRIVATE Qt6::GuiPrivate)
    endif()
endforeach()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-application-qos-tests tests/application_qos_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-application-qos-tests PRIVATE clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-application-qos-tests)
    foreach(_clip_qos IN ITEMS user_interactive user_initiated default utility background invalid missing)
        add_test(NAME clip-shot-application-qos-${_clip_qos}-tests COMMAND clip-shot-application-qos-tests ${_clip_qos})
        set_tests_properties(clip-shot-application-qos-${_clip_qos}-tests PROPERTIES LABELS "unit;macos" TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
    unset(_clip_qos)
endif()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-login-item-settings-tests tests/login_item_settings_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_link_libraries(clip-shot-login-item-settings-tests PRIVATE clip_shot_settings clip_shot_image_codec)
    clip_shot_import_offscreen_platform(clip-shot-login-item-settings-tests)
    clip_shot_add_translations(clip-shot-login-item-settings-tests)
    add_test(NAME clip-shot-login-item-settings-tests COMMAND clip-shot-login-item-settings-tests)
    set_tests_properties(clip-shot-login-item-settings-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit;macos")
endif()
if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-login-item-native-tests tests/login_item_native_tests.mm)
    target_link_libraries(clip-shot-login-item-native-tests PRIVATE clip_shot_login_item)
    add_test(NAME clip-shot-login-item-native-tests COMMAND clip-shot-login-item-native-tests)
    set_tests_properties(clip-shot-login-item-native-tests PROPERTIES LABELS "unit;macos")
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-qr-controller-tests
        tests/screenshot_qr_controller_tests.cpp
        src/presentation/ocr/screenshotqrcontroller.cpp
        include/clip_shot/presentation/screenshotqrcontroller.h
        src/presentation/components/icons/clipshoticons.cpp)
    target_include_directories(clip-shot-qr-controller-tests PRIVATE include)
    target_link_libraries(clip-shot-qr-controller-tests PRIVATE clip_shot_shortcuts clip_draw_engine_qt_canvas
        ant_design_qt Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-qr-controller-tests)
    add_test(NAME clip-shot-qr-controller-tests COMMAND clip-shot-qr-controller-tests)
    if(WIN32)
        add_test(NAME clip-shot-qr-stacking-native-tests
            COMMAND clip-shot-qr-controller-tests --stacking-only -platform windows)
        set_tests_properties(clip-shot-qr-stacking-native-tests PROPERTIES
            LABELS "interactive;windows" RUN_SERIAL TRUE)
    endif()
    set_tests_properties(clip-shot-qr-controller-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "unit")
endif()

if(TARGET clip-shot-recognition-session-controller-tests)
    add_test(NAME clip-shot-latex-recognition-tests COMMAND clip-shot-recognition-session-controller-tests --latex-only -platform offscreen)
    set_tests_properties(clip-shot-latex-recognition-tests PROPERTIES LABELS "unit" TIMEOUT 60)
endif()

if(TARGET clip-shot-pinned-window-tests)
    add_test(NAME clip-shot-pinned-template-dialog-tests
        COMMAND clip-shot-pinned-window-tests --template-dialogs-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-template-dialog-tests PROPERTIES
        LABELS "unit" CLIP_TEST_CROSS_PLATFORM TRUE TIMEOUT 60)
    add_test(NAME clip-shot-pinned-latex-tests COMMAND clip-shot-pinned-window-tests --latex-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-latex-tests PROPERTIES LABELS "unit" TIMEOUT 60)
endif()

if(APPLE AND CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-macos-cursor-tests tests/macos_cursor_tests.cpp)
    target_link_libraries(clip-shot-macos-cursor-tests PRIVATE clip_shot_macos_cursor)
    clip_shot_import_offscreen_platform(clip-shot-macos-cursor-tests)
    add_test(NAME clip-shot-macos-cursor-tests COMMAND clip-shot-macos-cursor-tests -platform offscreen)
    set_tests_properties(clip-shot-macos-cursor-tests PROPERTIES LABELS "unit;macos")

    add_executable(clip-shot-macos-cursor-native-tests tests/macos_cursor_native_tests.mm
        ../cmake/test-support/macos_native_input.mm)
    clip_shot_link_macos_window_platform(clip-shot-macos-cursor-native-tests)
    target_include_directories(clip-shot-macos-cursor-native-tests PRIVATE include src)
    target_link_libraries(clip-shot-macos-cursor-native-tests PRIVATE clip_shot_macos_cursor
        "-framework AppKit" "-framework ApplicationServices")
    add_test(NAME clip-shot-macos-cursor-native-tests
        COMMAND clip-shot-macos-cursor-native-tests -platform cocoa)
    set_tests_properties(clip-shot-macos-cursor-native-tests PROPERTIES
        LABELS "interactive;macos" RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 60)
endif()

if(TARGET clip-shot-pinned-window-tests)
    add_test(NAME clip-shot-pinned-drag-export-tests
        COMMAND clip-shot-pinned-window-tests --drag-export-only -platform offscreen)
    set_tests_properties(clip-shot-pinned-drag-export-tests PROPERTIES LABELS "unit")
    if(WIN32)
        add_test(NAME clip-shot-pinned-ctrl-hover-tests
            COMMAND clip-shot-pinned-window-tests --ctrl-hover-only -platform offscreen)
        set_tests_properties(clip-shot-pinned-ctrl-hover-tests PROPERTIES LABELS "unit;windows")
        add_test(NAME clip-shot-pinned-drag-export-native-tests
            COMMAND clip-shot-pinned-window-tests --drag-export-native-only -platform windows)
        set_tests_properties(clip-shot-pinned-drag-export-native-tests PROPERTIES LABELS "unit;windows")
    endif()
endif()

if(CLIP_SHOT_BUILD_TESTS)
    clip_shot_offscreen_qpa_environment(_clip_clipboard_qpa_environment)
    add_executable(clip-shot-clipboard-lifecycle-tests
        tests/screenshot_clipboard_lifecycle_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    target_include_directories(clip-shot-clipboard-lifecycle-tests PRIVATE include)
    target_link_libraries(clip-shot-clipboard-lifecycle-tests PRIVATE
        clip_shot_image_codec clip_shot_diagnostics Qt6::Widgets)
    if(WIN32)
        target_link_libraries(clip-shot-clipboard-lifecycle-tests PRIVATE user32)
    endif()
    clip_shot_import_offscreen_platform(clip-shot-clipboard-lifecycle-tests)
    add_test(NAME clip-shot-clipboard-lifecycle-tests COMMAND clip-shot-clipboard-lifecycle-tests)
    set_tests_properties(clip-shot-clipboard-lifecycle-tests PROPERTIES
        ENVIRONMENT "${_clip_clipboard_qpa_environment}" LABELS "unit" CLIP_TEST_CROSS_PLATFORM TRUE)

    if(TARGET clip-shot-pinned-window-tests)
        add_test(NAME clip-shot-selection-clipboard-lifecycle-tests
            COMMAND clip-shot-pinned-window-tests --clipboard-publication-only -platform offscreen)
        set_tests_properties(clip-shot-selection-clipboard-lifecycle-tests PROPERTIES
            ENVIRONMENT "${_clip_clipboard_qpa_environment}" LABELS "unit" TIMEOUT 60)
    endif()

    if(APPLE)
        add_executable(clip-shot-macos-clipboard-roundtrip-tests
            tests/macos_clipboard_roundtrip_tests.mm
            src/presentation/services/screenshotclipboardservice.cpp
            src/presentation/services/screenshotclipboardcontent.cpp)
        target_include_directories(clip-shot-macos-clipboard-roundtrip-tests PRIVATE include)
        target_link_libraries(clip-shot-macos-clipboard-roundtrip-tests PRIVATE
            clip_shot_image_codec clip_shot_diagnostics Qt6::Widgets
            "-framework AppKit" "-framework ImageIO")
        add_test(NAME clip-shot-macos-clipboard-roundtrip-tests
            COMMAND clip-shot-macos-clipboard-roundtrip-tests -platform cocoa)
        set_tests_properties(clip-shot-macos-clipboard-roundtrip-tests PROPERTIES
            LABELS "unit;macos" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    unset(_clip_clipboard_qpa_environment)
endif()

# History copying shares the same placement and outline calculations as direct pinning.
add_library(clip_shot_history_pin STATIC
    src/platform/windows/monitorgeometry.cpp
    src/presentation/services/historypinplacement.cpp
    src/presentation/services/screenshotselectionpin.cpp
    src/presentation/core/screenshotgeometry.cpp
    src/presentation/overlay/screenshotselectionshadowrenderer.cpp)
target_include_directories(clip_shot_history_pin PUBLIC include)
target_link_libraries(clip_shot_history_pin PUBLIC Qt6::Widgets PRIVATE ant_design_qt clip_canvas_image)
if(APPLE)
    target_link_libraries(clip_shot_history_pin PRIVATE "-framework AppKit")
elseif(WIN32)
    target_link_libraries(clip_shot_history_pin PRIVATE user32)
endif()
target_link_libraries(clip_shot_settings PRIVATE clip_shot_history_pin clip_shot_clipboard_placement)

# Shared clipboard metadata must have one native MIME converter per process.
add_library(clip_shot_clipboard_placement STATIC
    src/presentation/services/screenshotclipboardplacement.cpp
    src/presentation/services/screenshotclipboardappearance.cpp)
target_include_directories(clip_shot_clipboard_placement PUBLIC include)
target_link_libraries(clip_shot_clipboard_placement PUBLIC Qt6::Gui)
if(APPLE)
    target_sources(clip_shot_clipboard_placement PRIVATE src/platform/macos/clipboardrevision.mm)
    target_link_libraries(clip_shot_clipboard_placement PRIVATE "-framework AppKit")
elseif(WIN32)
    target_link_libraries(clip_shot_clipboard_placement PRIVATE user32)
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-clipboard-placement-tests
        tests/screenshot_clipboard_placement_tests.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotclipboardcontent.cpp
        src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp)
    target_include_directories(clip-shot-clipboard-placement-tests PRIVATE include src/image)
    target_link_libraries(clip-shot-clipboard-placement-tests PRIVATE
        clip_shot_clipboard_placement clip_shot_image_codec clip_shot_diagnostics Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-clipboard-placement-tests)
    add_test(NAME clip-shot-clipboard-placement-tests COMMAND clip-shot-clipboard-placement-tests)
    clip_shot_offscreen_qpa_environment(_clip_clipboard_placement_qpa_environment)
    set_tests_properties(clip-shot-clipboard-placement-tests PROPERTIES
        LABELS "unit" ENVIRONMENT "${_clip_clipboard_placement_qpa_environment}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    unset(_clip_clipboard_placement_qpa_environment)
    if(APPLE OR WIN32)
        add_test(NAME clip-shot-clipboard-placement-native-tests
            COMMAND clip-shot-clipboard-placement-tests --native-roundtrip)
        set_tests_properties(clip-shot-clipboard-placement-native-tests PROPERTIES
            LABELS "interactive" RUN_SERIAL TRUE TIMEOUT 60)
        if(APPLE)
            set_tests_properties(clip-shot-clipboard-placement-native-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
        else()
            target_link_libraries(clip-shot-clipboard-placement-tests PRIVATE user32)
            set_tests_properties(clip-shot-clipboard-placement-native-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
        endif()
    endif()
endif()

if(CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-clipboard-placement-benchmark
        tests/screenshot_clipboard_placement_benchmark.cpp
        src/presentation/services/screenshotclipboardservice.cpp
        src/presentation/services/screenshotclipboardcontent.cpp
        src/presentation/services/screenshotclipboarddibdecoder_avx2.cpp)
    target_include_directories(clip-shot-clipboard-placement-benchmark PRIVATE include src/image)
    target_link_libraries(clip-shot-clipboard-placement-benchmark PRIVATE
        clip_shot_clipboard_placement clip_shot_image_codec clip_shot_diagnostics Qt6::Widgets)
    clip_shot_import_offscreen_platform(clip-shot-clipboard-placement-benchmark)
    if(WIN32)
        target_link_libraries(clip-shot-clipboard-placement-benchmark PRIVATE user32)
    endif()
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_test(NAME clip-shot-recording-audio-settings-tests
        COMMAND clip-shot-application-storage-tests --recording-audio-only)
    set_tests_properties(clip-shot-recording-audio-settings-tests PROPERTIES LABELS "unit")
    add_test(NAME clip-shot-recording-audio-controller-tests
        COMMAND clip-shot-screen-recording-controller-tests --audio-gain-only -platform offscreen)
    set_tests_properties(clip-shot-recording-audio-controller-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()

if(CLIP_SHOT_BUILD_TESTS)
    clip_find_qt(COMPONENTS Test)
    add_executable(clip-shot-recording-audio-gain-popover-tests
        tests/recording_audio_gain_popover_tests.cpp
        src/presentation/recording/recordingaudiogainpopover.cpp
        src/presentation/recording/recordingaudiogainpopover.h)
    set_target_properties(clip-shot-recording-audio-gain-popover-tests PROPERTIES AUTOMOC ON)
    target_link_libraries(clip-shot-recording-audio-gain-popover-tests PRIVATE ant_design_qt Qt6::Widgets Qt6::Test)
    target_include_directories(clip-shot-recording-audio-gain-popover-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    get_target_property(_clip_audio_qt_core_type Qt6::Core TYPE)
    if(_clip_audio_qt_core_type STREQUAL "STATIC_LIBRARY" AND TARGET Qt6::QOffscreenIntegrationPlugin)
        target_link_libraries(clip-shot-recording-audio-gain-popover-tests PRIVATE Qt6::QOffscreenIntegrationPlugin)
        qt_import_plugins(clip-shot-recording-audio-gain-popover-tests INCLUDE Qt6::QOffscreenIntegrationPlugin)
    endif()
    add_test(NAME clip-shot-recording-audio-gain-popover-tests
        COMMAND clip-shot-recording-audio-gain-popover-tests -platform offscreen)
    set_tests_properties(clip-shot-recording-audio-gain-popover-tests PROPERTIES LABELS "unit" TIMEOUT 60
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()
if(CLIP_SHOT_BUILD_TESTS)
    # This isolated skin test exercises portable widget surfaces, including
    # builds where the legacy toolbar executable's other cases are Windows-only.
    set_tests_properties(clip-shot-toolbar-skin-tests PROPERTIES LABELS "unit")
endif()

if(CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-skin-surface-performance-benchmark
        tests/skin_surface_performance_benchmark.cpp
        include/clip_shot/presentation/languagemanager.h
        include/clip_shot/presentation/pinnedwindowgroupmanager.h
        include/clip_shot/presentation/styles/thememanager.h
        include/clip_shot/presentation/systemtraycontroller.h
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/services/languagemanager.cpp
        src/presentation/services/pinnedwindowgroupmanager.cpp
        src/presentation/services/systemtraycontroller.cpp
        src/presentation/styles/thememanager.cpp
        src/presentation/styles/themecolorscheme.cpp)
    target_include_directories(clip-shot-skin-surface-performance-benchmark PRIVATE include)
    target_compile_definitions(clip-shot-skin-surface-performance-benchmark PRIVATE
        CLIP_SKIN_SURFACE_BUILD_CONFIG="$<CONFIG>")
    target_link_libraries(clip-shot-skin-surface-performance-benchmark PRIVATE
        clip_shot_system_notification_controller
        clip_shot_storage clip_shot_image_codec clip_shot_settings_catalog
        clip_draw_engine_qt_canvas ant_design_qt Qt6::Widgets)
    if(APPLE)
        target_sources(clip-shot-skin-surface-performance-benchmark PRIVATE
            src/platform/macos/systemtraymenu.mm)
        target_link_libraries(clip-shot-skin-surface-performance-benchmark PRIVATE
            clip_shot_system_notifications "-framework AppKit")
    endif()
    qt_import_plugins(clip-shot-skin-surface-performance-benchmark EXCLUDE Qt6::QICOPlugin)
    qt_add_resources(clip-shot-skin-surface-performance-benchmark "clip_shot_skin_surface_tray_assets"
        PREFIX "/clip-shot/app-icons"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-default.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-light.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-dark.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-default.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-light.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/clip-shot-tray-clip-dark.png")
    clip_shot_add_translations(clip-shot-skin-surface-performance-benchmark)
    clip_shot_import_offscreen_platform(clip-shot-skin-surface-performance-benchmark)
endif()

# Desktop floating toolbar coverage also supplies the focused Release benchmark.
if(CLIP_SHOT_BUILD_TESTS OR CLIP_APPS_BUILD_BENCHMARKS)
    set(_clip_floating_test_sources
        tests/floating_toolbar_tests.cpp
        include/clip_shot/presentation/floatingtoolbarcontroller.h
        src/presentation/toolbar/floatingtoolbarcontroller.cpp
        src/presentation/tools/screenshottoolbarmainpanel.cpp
        src/presentation/tools/screenshottoolpalettebuttons.cpp
        src/presentation/services/screenshotclipboardservice.cpp)
    set(_clip_floating_targets)
    if(CLIP_SHOT_BUILD_TESTS)
        add_executable(clip-shot-floating-toolbar-tests ${_clip_floating_test_sources})
        target_compile_definitions(clip-shot-floating-toolbar-tests PRIVATE
            CLIP_FLOATING_TEST_TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}")
        add_dependencies(clip-shot-floating-toolbar-tests clip_shot_release_translations)
        list(APPEND _clip_floating_targets clip-shot-floating-toolbar-tests)
        add_test(NAME clip-shot-floating-toolbar-tests COMMAND clip-shot-floating-toolbar-tests)
        add_test(NAME clip-shot-floating-toolbar-icon-activation-tests
            COMMAND clip-shot-floating-toolbar-tests --icon-activation-only -platform offscreen)
        set_tests_properties(clip-shot-floating-toolbar-icon-activation-tests PROPERTIES
            LABELS "unit" TIMEOUT 60 CLIP_TEST_CROSS_PLATFORM TRUE
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test-support/menu-screen.json"
            CONTENT "{\"screens\":[{\"name\":\"menu-screen\",\"width\":240,\"height\":600,\"dpr\":1.0}]}\n")
        add_test(NAME clip-shot-floating-toolbar-menu-lifecycle-tests
            COMMAND clip-shot-floating-toolbar-tests --menu-lifecycle-only
                -platform "offscreen:configfile=menu-screen.json")
        set_tests_properties(clip-shot-floating-toolbar-menu-lifecycle-tests PROPERTIES
            LABELS "unit" TIMEOUT 30 CLIP_TEST_CROSS_PLATFORM TRUE
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-support"
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        add_test(NAME clip-shot-floating-toolbar-content-drop-tests
            COMMAND clip-shot-floating-toolbar-tests --content-drops-only -platform offscreen)
        set_tests_properties(clip-shot-floating-toolbar-content-drop-tests PROPERTIES
            LABELS "unit" TIMEOUT 60 CLIP_TEST_CROSS_PLATFORM TRUE
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        set_tests_properties(clip-shot-floating-toolbar-tests PROPERTIES LABELS "unit" TIMEOUT 60
            CLIP_TEST_CROSS_PLATFORM TRUE
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        if(WIN32)
            add_test(NAME clip-shot-floating-toolbar-native-tests
                COMMAND clip-shot-floating-toolbar-tests --native-probe -platform windows)
            set_tests_properties(clip-shot-floating-toolbar-native-tests PROPERTIES
                LABELS "interactive;windows" RUN_SERIAL TRUE TIMEOUT 60
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        endif()
    endif()
    if(CLIP_APPS_BUILD_BENCHMARKS)
        add_executable(clip-shot-floating-toolbar-performance-benchmark ${_clip_floating_test_sources})
        list(APPEND _clip_floating_targets clip-shot-floating-toolbar-performance-benchmark)
    endif()
    if(APPLE AND CLIP_SHOT_BUILD_TESTS)
        set(_clip_floating_macos_sources ${_clip_floating_test_sources})
        list(REMOVE_ITEM _clip_floating_macos_sources tests/floating_toolbar_tests.cpp)
        add_executable(clip-shot-macos-floating-toolbar-drop-tests
            tests/macos_floating_toolbar_drop_tests.mm
            ../cmake/test-support/macos_native_input.mm
            ${_clip_floating_macos_sources})
        list(APPEND _clip_floating_targets clip-shot-macos-floating-toolbar-drop-tests)
        target_include_directories(clip-shot-macos-floating-toolbar-drop-tests PRIVATE src)
        target_link_libraries(clip-shot-macos-floating-toolbar-drop-tests PRIVATE
            "-framework AppKit" "-framework ApplicationServices")
        add_test(NAME clip-shot-macos-floating-toolbar-drop-policy-tests
            COMMAND clip-shot-macos-floating-toolbar-drop-tests -platform offscreen)
        set_tests_properties(clip-shot-macos-floating-toolbar-drop-policy-tests PROPERTIES
            LABELS "unit;macos" CLIP_TEST_CROSS_PLATFORM TRUE TIMEOUT 30)
        add_test(NAME clip-shot-macos-floating-toolbar-content-drop-native-tests
            COMMAND clip-shot-macos-floating-toolbar-drop-tests --native -platform cocoa)
        set_tests_properties(clip-shot-macos-floating-toolbar-content-drop-native-tests PROPERTIES
            LABELS "interactive;macos" CLIP_TEST_CROSS_PLATFORM TRUE
            RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 30)
    endif()
    foreach(_clip_floating_target IN LISTS _clip_floating_targets)
        target_include_directories(${_clip_floating_target} PRIVATE include src/image)
        target_link_libraries(${_clip_floating_target} PRIVATE
            clip_shot_settings clip_shot_image_codec clip_shot_clipboard_placement
            clip_draw_engine_qt_canvas ant_design_qt Qt6::Widgets)
        clip_shot_import_offscreen_platform(${_clip_floating_target})
        clip_shot_link_macos_window_platform(${_clip_floating_target})
        if(WIN32)
            target_link_libraries(${_clip_floating_target} PRIVATE dwmapi gdi32 user32)
            if(NOT CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
                _clip_shot_stage_image_runtime(${_clip_floating_target})
            endif()
        endif()
    endforeach()
endif()

# Toolbar and tray sources are also compiled directly by focused tests and
# benchmarks. Keep the skin dependency identical for every consumer.
get_property(_clip_skin_consumers DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_clip_target IN LISTS _clip_skin_consumers)
    get_target_property(_clip_sources ${_clip_target} SOURCES)
    if("${_clip_sources}" MATCHES "(screenshottoolbarmainpanel|systemtraycontroller)\\.cpp")
        target_link_libraries(${_clip_target} PRIVATE clip_shot_main_window_skin)
    endif()
endforeach()

add_library(clip_shot_original_image_preview STATIC
    include/clip_shot/presentation/screenshotoriginalimagepreviewwindow.h
    src/presentation/ocr/screenshotoriginalimagepreviewwindow.cpp
    src/presentation/overlay/screenshotimagerendering.cpp)
target_include_directories(clip_shot_original_image_preview PUBLIC include)
target_link_libraries(clip_shot_original_image_preview PUBLIC Qt6::Widgets
    PRIVATE clip_canvas_image adqt::dpi_window)
clip_shot_link_macos_window_platform(clip_shot_original_image_preview)
if(WIN32)
    target_link_libraries(clip_shot_original_image_preview PRIVATE user32 gdi32)
endif()

get_property(_clip_preview_consumers DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_clip_target IN LISTS _clip_preview_consumers)
    get_target_property(_clip_sources ${_clip_target} SOURCES)
    if("${_clip_sources}" MATCHES "screenshot(recognitionwindow|canvasrenderer)\\.cpp")
        target_link_libraries(${_clip_target} PRIVATE clip_shot_original_image_preview)
    endif()
endforeach()

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-original-image-preview-tests
        tests/screenshot_original_image_preview_tests.cpp)
    target_link_libraries(clip-shot-original-image-preview-tests PRIVATE
        clip_shot_original_image_preview)
    clip_shot_import_offscreen_platform(clip-shot-original-image-preview-tests)
    add_test(NAME clip-shot-original-image-preview-tests
        COMMAND clip-shot-original-image-preview-tests -platform offscreen)
    set_tests_properties(clip-shot-original-image-preview-tests PROPERTIES LABELS "unit"
        TIMEOUT 60 ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(APPLE)
        set_source_files_properties(tests/screenshot_original_image_preview_tests.cpp
            PROPERTIES LANGUAGE OBJCXX)
        target_link_libraries(clip-shot-original-image-preview-tests PRIVATE "-framework AppKit")
        add_test(NAME clip-shot-original-image-preview-native-macos-tests
            COMMAND clip-shot-original-image-preview-tests --native-only -platform cocoa)
        set_tests_properties(clip-shot-original-image-preview-native-macos-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 120)
    endif()
    if(WIN32)
        set(_preview_scales 1 1.25 1.5 1.75 2)
        set(_preview_factors 0.5 0.625 0.75 0.875 1)
        foreach(_preview_scale _preview_factor IN ZIP_LISTS _preview_scales _preview_factors)
            add_test(NAME clip-shot-original-image-preview-native-${_preview_scale}-tests
                COMMAND clip-shot-original-image-preview-tests --native-only -platform windows)
            set_tests_properties(clip-shot-original-image-preview-native-${_preview_scale}-tests
                PROPERTIES LABELS "unit;windows" RUN_SERIAL TRUE TIMEOUT 60
                ENVIRONMENT "QT_SCREEN_SCALE_FACTORS=2;QT_SCALE_FACTOR=${_preview_factor};CLIP_PREVIEW_TEST_DPR=${_preview_scale}"
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
            add_test(NAME clip-shot-pinned-original-image-preview-native-${_preview_scale}-tests
                COMMAND clip-shot-pinned-window-tests --original-image-preview-only -platform windows)
            set_tests_properties(clip-shot-pinned-original-image-preview-native-${_preview_scale}-tests
                PROPERTIES LABELS "unit;windows" RUN_SERIAL TRUE TIMEOUT 120
                ENVIRONMENT "QT_SCREEN_SCALE_FACTORS=2;QT_SCALE_FACTOR=${_preview_factor};CLIP_PREVIEW_TEST_DPR=${_preview_scale}"
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
        endforeach()
        add_test(NAME clip-shot-original-image-preview-native-platform-tests
            COMMAND clip-shot-original-image-preview-tests --native-only -platform windows)
        add_test(NAME clip-shot-pinned-original-image-preview-native-platform-tests
            COMMAND clip-shot-pinned-window-tests --original-image-preview-only -platform windows)
        set_tests_properties(clip-shot-original-image-preview-native-platform-tests
            clip-shot-pinned-original-image-preview-native-platform-tests PROPERTIES
            LABELS "unit;windows" RUN_SERIAL TRUE TIMEOUT 120
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>;QT_SCREEN_SCALE_FACTORS=unset:;QT_SCALE_FACTOR=unset:;CLIP_PREVIEW_TEST_DPR=unset:")
    endif()
    add_test(NAME clip-shot-recognition-original-image-preview-tests
        COMMAND clip-shot-recognition-session-controller-tests --original-image-preview-only
            -platform offscreen)
    add_test(NAME clip-shot-recognition-original-image-preview-window-tests
        COMMAND clip-shot-recognition-window-tests --preview-window-only -platform offscreen)
    add_test(NAME clip-shot-pinned-original-image-preview-tests
        COMMAND clip-shot-pinned-window-tests --original-image-preview-only -platform offscreen)
    add_test(NAME clip-shot-original-image-preview-settings-tests
        COMMAND clip-shot-translation-settings-tests --original-image-preview-only -platform offscreen)
    add_test(NAME clip-shot-large-image-slice-rendering-tests
        COMMAND clip-shot-screenshot-canvas-renderer-tests --large-image-slice-rendering
            -platform offscreen)
    set_tests_properties(clip-shot-recognition-original-image-preview-tests
        clip-shot-recognition-original-image-preview-window-tests
        clip-shot-pinned-original-image-preview-tests clip-shot-original-image-preview-settings-tests
        clip-shot-large-image-slice-rendering-tests
        PROPERTIES LABELS "unit" TIMEOUT 120
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    foreach(_preview_scale IN ITEMS 1.25 2)
        add_test(NAME clip-shot-pinned-original-image-preview-${_preview_scale}-tests
            COMMAND clip-shot-pinned-window-tests --original-image-preview-only -platform offscreen)
        set_tests_properties(clip-shot-pinned-original-image-preview-${_preview_scale}-tests
            PROPERTIES LABELS "unit" TIMEOUT 120 ENVIRONMENT "QT_SCALE_FACTOR=${_preview_scale}"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endforeach()
    add_test(NAME clip-shot-original-image-preview-retina-tests
        COMMAND clip-shot-original-image-preview-tests -platform offscreen)
    set_tests_properties(clip-shot-original-image-preview-retina-tests PROPERTIES LABELS "unit"
        TIMEOUT 60 ENVIRONMENT "QT_SCALE_FACTOR=2"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()
if(CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-original-image-preview-performance-benchmark
        tests/screenshot_original_image_preview_performance_benchmark.cpp)
    target_link_libraries(clip-shot-original-image-preview-performance-benchmark PRIVATE
        clip_shot_original_image_preview)
    clip_shot_import_offscreen_platform(clip-shot-original-image-preview-performance-benchmark)
    add_test(NAME clip-shot-original-image-preview-performance-benchmark
        COMMAND clip-shot-original-image-preview-performance-benchmark -platform offscreen)
    set_tests_properties(clip-shot-original-image-preview-performance-benchmark PROPERTIES
        LABELS "benchmark" TIMEOUT 120
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    if(WIN32)
        add_test(NAME clip-shot-original-image-preview-native-performance-benchmark
            COMMAND clip-shot-original-image-preview-performance-benchmark
                --native-only -platform windows)
        set_tests_properties(clip-shot-original-image-preview-native-performance-benchmark PROPERTIES
            LABELS "benchmark;windows" RUN_SERIAL TRUE TIMEOUT 120
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endif()

# Link after all clipboard test and benchmark targets have been declared so
# metadata and native image converters are available to every consumer.
get_property(_clip_clipboard_consumers DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_clip_target IN LISTS _clip_clipboard_consumers)
    get_target_property(_clip_sources ${_clip_target} SOURCES)
    if("${_clip_sources}" MATCHES "screenshotclipboard(service|content)\\.cpp")
        target_link_libraries(${_clip_target} PRIVATE clip_shot_clipboard_placement)
    endif()
    if(APPLE AND "${_clip_sources}" MATCHES "screenshotclipboard(service|content)\\.cpp")
        target_link_libraries(${_clip_target} PRIVATE clip_shot_macos_clipboard)
    endif()
endforeach()

foreach(_clip_hidden_shortcut_pair IN ITEMS
        "capture|clip-shot-screenshot-toolbar-remembered-tool-tests"
        "pinned|clip-shot-pinned-window-tests"
        "recording|clip-shot-screen-recording-shortcut-tests"
        "global-canvas|clip-shot-global-canvas-tests"
        "floating|clip-shot-global-shortcut-manager-tests")
    string(REPLACE "|" ";" _clip_hidden_shortcut_parts "${_clip_hidden_shortcut_pair}")
    list(GET _clip_hidden_shortcut_parts 0 _clip_hidden_shortcut_context)
    list(GET _clip_hidden_shortcut_parts 1 _clip_hidden_shortcut_target)
    if(TARGET ${_clip_hidden_shortcut_target})
        add_test(NAME clip-shot-${_clip_hidden_shortcut_context}-hidden-tool-shortcut-tests
            COMMAND ${_clip_hidden_shortcut_target} --hidden-tools-only -platform offscreen)
        set_tests_properties(clip-shot-${_clip_hidden_shortcut_context}-hidden-tool-shortcut-tests
            PROPERTIES LABELS "unit" RUN_SERIAL TRUE TIMEOUT 60
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endforeach()

foreach(_right_selection_host IN ITEMS overlay pinned recording global)
    if(_right_selection_host STREQUAL "overlay")
        set(_right_selection_target clip-shot-screenshot-canvas-renderer-tests)
    elseif(_right_selection_host STREQUAL "pinned")
        set(_right_selection_target clip-shot-pinned-window-tests)
    elseif(_right_selection_host STREQUAL "recording")
        set(_right_selection_target clip-shot-screen-recording-area-window-tests)
    else()
        set(_right_selection_target clip-shot-global-canvas-tests)
    endif()
    if(TARGET ${_right_selection_target})
        add_test(NAME clip-shot-${_right_selection_host}-right-quick-selection-tests
            COMMAND ${_right_selection_target} --right-quick-selection-only -platform offscreen)
        set_tests_properties(clip-shot-${_right_selection_host}-right-quick-selection-tests PROPERTIES
            LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endforeach()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipShotReleaseOptimization.cmake")
# Standalone test targets reuse the pinned/controller sources as well as the application.
get_property(_clip_cloud_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_clip_cloud_target IN LISTS _clip_cloud_targets)
    get_target_property(_clip_cloud_sources ${_clip_cloud_target} SOURCES)
    foreach(_clip_cloud_source IN LISTS _clip_cloud_sources)
        if(_clip_cloud_source MATCHES "screenshot(pinnedwindow|controller)\\.cpp$")
            target_link_libraries(${_clip_cloud_target} PRIVATE clip_shot_cloud_upload)
            break()
        endif()
    endforeach()
endforeach()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipShotMini.cmake")

# Controllers and their focused fixtures compile the same sources independently.
get_property(_clip_print_consumers DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_clip_print_consumer IN LISTS _clip_print_consumers)
    get_target_property(_clip_print_sources ${_clip_print_consumer} SOURCES)
    if(_clip_print_sources MATCHES "screenshot(pinnedwindow|controller)\\.cpp")
        target_link_libraries(${_clip_print_consumer} PRIVATE clip_shot_print)
    endif()
endforeach()

if(CLIP_SHOT_BUILD_TESTS)
    target_sources(clip-shot-pinned-window-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../test-support/fonts/recording_test_fonts.qrc")
    add_executable(clip-shot-print-service-tests tests/screenshot_print_service_tests.cpp)
    target_link_libraries(clip-shot-print-service-tests PRIVATE clip_shot_print)
    clip_shot_import_offscreen_platform(clip-shot-print-service-tests)
    add_test(NAME clip-shot-print-service-tests
        COMMAND clip-shot-print-service-tests -platform offscreen)
    set_tests_properties(clip-shot-print-service-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    add_executable(clip-shot-native-print-smoke tests/native_print_smoke.cpp)
    target_link_libraries(clip-shot-native-print-smoke PRIVATE clip_shot_print)
    if(APPLE)
        add_executable(clip-shot-macos-native-print-stacking-tests
            tests/macos_native_print_stacking_tests.mm)
        target_include_directories(clip-shot-macos-native-print-stacking-tests PRIVATE include src)
        target_link_libraries(clip-shot-macos-native-print-stacking-tests PRIVATE
            clip_shot_print clip_shot_macos_window_platform "-framework AppKit")
        add_test(NAME clip-shot-macos-native-print-stacking-tests
            COMMAND clip-shot-macos-native-print-stacking-tests -platform offscreen)
        set_tests_properties(clip-shot-macos-native-print-stacking-tests PROPERTIES
            LABELS "unit;macos" TIMEOUT 30)
        add_test(NAME clip-shot-macos-native-print-stacking-native-tests
            COMMAND clip-shot-macos-native-print-stacking-tests -platform cocoa)
        set_tests_properties(clip-shot-macos-native-print-stacking-native-tests PROPERTIES
            LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
    if(WIN32)
        add_executable(clip-shot-windows-print-dialog-tests tests/windows_print_dialog_tests.cpp)
        target_link_libraries(clip-shot-windows-print-dialog-tests PRIVATE clip_shot_print)
        clip_shot_import_offscreen_platform(clip-shot-windows-print-dialog-tests)
        add_test(NAME clip-shot-windows-print-dialog-tests
            COMMAND clip-shot-windows-print-dialog-tests -platform offscreen)
        set_tests_properties(clip-shot-windows-print-dialog-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 60
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
        add_executable(clip-shot-windows-print-document-tests
            tests/windows_print_document_tests.cpp)
        target_link_libraries(clip-shot-windows-print-document-tests PRIVATE clip_shot_print)
        add_test(NAME clip-shot-windows-print-document-tests
            COMMAND clip-shot-windows-print-document-tests)
        set_tests_properties(clip-shot-windows-print-document-tests PROPERTIES
            LABELS "unit;windows" TIMEOUT 60)
    endif()
    foreach(_print_host IN ITEMS pinned-window toolbar-main-panel floating-toolbar-drag)
        add_test(NAME clip-shot-${_print_host}-print-tests
            COMMAND clip-shot-${_print_host}-tests --print-only -platform offscreen)
        set_tests_properties(clip-shot-${_print_host}-print-tests PROPERTIES
            LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
            ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
    endforeach()
    add_test(NAME clip-shot-print-export-snapshot-tests
        COMMAND clip-shot-export-artifact-tests --print-snapshots-only -platform offscreen)
    set_tests_properties(clip-shot-print-export-snapshot-tests PROPERTIES
        LABELS unit CLIP_TEST_CROSS_PLATFORM TRUE RUN_SERIAL TRUE TIMEOUT 60
        ENVIRONMENT "${_CLIP_SHOT_OFFSCREEN_QPA_ENVIRONMENT}")
endif()

if(CLIP_SHOT_BUILD_TESTS)
    add_executable(clip-shot-pinned-selection-geometry-tests
        tests/pinned_window_selection_geometry_tests.cpp
        src/presentation/pinned/pinnedwindowselectiongeometry.cpp)
    target_include_directories(clip-shot-pinned-selection-geometry-tests PRIVATE
        src/presentation/pinned "${CMAKE_SOURCE_DIR}/clip_draw_engine_qt/include")
    target_link_libraries(clip-shot-pinned-selection-geometry-tests PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-pinned-selection-geometry-tests
        COMMAND clip-shot-pinned-selection-geometry-tests)
    set_tests_properties(clip-shot-pinned-selection-geometry-tests PROPERTIES LABELS "unit")
    if(TARGET clip-shot-pinned-window-tests)
        add_test(NAME clip-shot-pinned-multi-selection-tests
            COMMAND clip-shot-pinned-window-tests --multi-selection-only -platform offscreen)
        set_tests_properties(clip-shot-pinned-multi-selection-tests PROPERTIES
            LABELS "unit" RUN_SERIAL TRUE TIMEOUT 90)
        if(APPLE)
            add_test(NAME clip-shot-macos-pinned-multi-selection-geometry-tests
                COMMAND clip-shot-pinned-window-tests --multi-selection-geometry-only -platform cocoa)
            set_tests_properties(clip-shot-macos-pinned-multi-selection-geometry-tests PROPERTIES
                LABELS "interactive;macos" RUN_SERIAL TRUE TIMEOUT 60)
            add_test(NAME clip-shot-macos-pinned-multi-selection-tests
                COMMAND clip-shot-pinned-window-tests --multi-selection-native-only -platform cocoa)
            set_tests_properties(clip-shot-macos-pinned-multi-selection-tests PROPERTIES
                LABELS "interactive;e2e;macos" RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 60)
        elseif(WIN32)
            add_test(NAME clip-shot-pinned-multi-selection-native-tests
                COMMAND clip-shot-pinned-window-tests --multi-selection-native-only -platform windows)
            set_tests_properties(clip-shot-pinned-multi-selection-native-tests PROPERTIES
                LABELS "interactive;e2e;windows" RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 60)
        endif()
    endif()
    if(TARGET clip-shot-pinned-window-group-tests)
        add_test(NAME clip-shot-pinned-batch-group-tests
            COMMAND clip-shot-pinned-window-group-tests --batch-group-only -platform offscreen)
        set_tests_properties(clip-shot-pinned-batch-group-tests PROPERTIES LABELS "unit")
    endif()
    if(TARGET clip-shot-pinned-window-repository-tests)
        add_test(NAME clip-shot-pinned-batch-group-repository-tests
            COMMAND clip-shot-pinned-window-repository-tests --batch-group-only)
        set_tests_properties(clip-shot-pinned-batch-group-repository-tests PROPERTIES LABELS "unit")
    endif()
endif()
if(CLIP_SHOT_BUILD_BENCHMARKS)
    add_executable(clip-shot-pinned-selection-performance-benchmark
        tests/pinned_window_selection_performance_benchmark.cpp
        src/presentation/pinned/pinnedwindowselectiongeometry.cpp)
    target_include_directories(clip-shot-pinned-selection-performance-benchmark PRIVATE
        src/presentation/pinned "${CMAKE_SOURCE_DIR}/clip_draw_engine_qt/include")
    target_link_libraries(clip-shot-pinned-selection-performance-benchmark PRIVATE Qt6::Gui)
    add_test(NAME clip-shot-pinned-selection-performance-benchmark
        COMMAND clip-shot-pinned-selection-performance-benchmark)
    set_tests_properties(clip-shot-pinned-selection-performance-benchmark PROPERTIES LABELS "benchmark")
    if(TARGET clip-shot-pinned-window-tests)
        add_test(NAME clip-shot-pinned-multi-selection-performance-benchmark
            COMMAND clip-shot-pinned-window-tests --multi-selection-performance-only -platform offscreen)
        set_tests_properties(clip-shot-pinned-multi-selection-performance-benchmark PROPERTIES
            LABELS "benchmark" RUN_SERIAL TRUE TIMEOUT 120)
    endif()
endif()
