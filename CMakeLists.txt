cmake_minimum_required(VERSION 4.2)

if(CMAKE_HOST_WIN32 AND CMAKE_CXX_COMPILER MATCHES "clang-cl")
    find_program(CLIP_MSVC_LIB_EXECUTABLE NAMES lib.exe REQUIRED)
    set(CMAKE_AR "${CLIP_MSVC_LIB_EXECUTABLE}" CACHE FILEPATH "MSVC archiver for clang-cl." FORCE)
endif()

project(clip_apps VERSION 0.2.0 LANGUAGES CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ClipWindowsArchitecture.cmake")
clip_configure_windows_architecture()

set(CLIP_SHOT_VERSION "1.3.1")
if(NOT CLIP_SHOT_VERSION MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)(-[0-9A-Za-z.-]+)?$")
    message(FATAL_ERROR "CLIP_SHOT_VERSION must be a semantic version such as 0.8.0-dev")
endif()
set(CLIP_SHOT_VERSION_NUMERIC "${CMAKE_MATCH_1}")
set(CLIP_SHOT_PRODUCT_NAME "Clip")
set(CLIP_SHOT_EXECUTABLE_NAME "clip_shot")
set(CLIP_SHOT_VENDOR "Clip Apps")
set(CLIP_SHOT_DESCRIPTION
    "A screenshot utility for capturing, annotating, pinning, and recognizing screen content.")
set(CLIP_SHOT_HOMEPAGE_URL "https://clipshot.top")
set(CLIP_SHOT_SUPPORT_EMAIL "chao@mgchao.top")
set(CLIP_SHOT_COPYRIGHT "Copyright (C) 2025-2026 mg-chao")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(/MP)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectWarnings)
include(ClipWorkspace)
include(ClipQt)
include(RustStaticLibrary)

option(CLIP_APPS_BUILD_ANT_DESIGN_QT "Build the Ant Design Qt libraries." ON)
option(CLIP_APPS_BUILD_CLIP_IMAGE "Build the Clip Image library." ON)
option(CLIP_APPS_BUILD_DRAW_ENGINE "Build the Clip Draw Engine Qt library." ON)
option(CLIP_APPS_BUILD_DRAW_ENGINE_DEMO "Build the Clip Draw Engine Qt demo." OFF)
option(CLIP_APPS_BUILD_IMAGE_VIEWER "Build Clip Image Viewer." ON)
option(CLIP_APPS_BUILD_CLIP_SHOT "Build Clip." ON)
set(_clip_mini_default ${CLIP_APPS_BUILD_CLIP_SHOT})
option(CLIP_APPS_BUILD_CLIP_SHOT_MINI "Build Clip Mini alongside Clip." ${_clip_mini_default})
if(CLIP_APPS_BUILD_CLIP_SHOT_MINI AND NOT CLIP_APPS_BUILD_CLIP_SHOT)
    message(FATAL_ERROR "Clip Mini requires CLIP_APPS_BUILD_CLIP_SHOT=ON")
endif()
option(CLIP_APPS_BUILD_TESTS "Build tests for all enabled projects." ON)
option(CLIP_SHOT_BUILD_RUNTIME_TESTS
    "Build focused OCR and crash diagnostics tests without unrelated test targets." OFF)
option(CLIP_APPS_BUILD_BENCHMARKS "Build opt-in benchmark targets." OFF)
option(CLIP_SHOT_SCROLLING_PERF_DETAIL "Compile detailed scrolling C++ and Rust timings." OFF)
option(CLIP_APPS_STRICT_COMPILE "Enable strict warnings for all projects." ON)
option(CLIP_APPS_ENABLE_CLANG_TIDY "Run clang-tidy while compiling C++ targets." OFF)
option(CLIP_APPS_RELEASE_STATIC "Prefer static libraries and the static MSVC runtime." OFF)
option(CLIP_APPS_QT_STATIC "Link the release executable against a static Qt build." OFF)
option(CLIP_APPS_PACKAGE_CLIP_SHOT "Configure the Clip platform package." OFF)
option(
    CLIP_APPS_ENABLE_RELEASE_OPTIMIZATION
    "Enable IPO/LTO and size/dead-code linker optimizations for Release targets."
    ON
)

set(CLIP_QT_STATIC_DIR "" CACHE PATH
    "Qt 6 CMake package directory for the static release kit.")
if(CLIP_APPS_QT_STATIC AND CLIP_QT_STATIC_DIR STREQUAL "" AND
   DEFINED ENV{CLIP_QT_STATIC_DIR} AND NOT "$ENV{CLIP_QT_STATIC_DIR}" STREQUAL "")
    set(CLIP_QT_STATIC_DIR "$ENV{CLIP_QT_STATIC_DIR}" CACHE PATH
        "Qt 6 CMake package directory for the static release kit." FORCE)
endif()
if(CLIP_APPS_QT_STATIC AND NOT CLIP_QT_STATIC_DIR STREQUAL "")
    set(Qt6_DIR "${CLIP_QT_STATIC_DIR}" CACHE PATH
        "Qt 6 CMake package directory." FORCE)
endif()

set(CLIP_APPS_API_BASE_URL "" CACHE STRING
    "Default Clip API base URL. Set explicitly for reproducible builds.")
set(CLIP_RUST_CARGO_TARGET_DIR
    "${CMAKE_BINARY_DIR}/cargo"
    CACHE PATH
    "Cargo target directory for the active CMake build tree.")
set(CLIP_RUST_RELEASE_PROFILE "release" CACHE STRING
    "Cargo profile used for non-Debug Rust builds.")

clip_workspace_configure_paths()
# Resolve imported metadata before configuring codecs, CRT linkage, or creating
# any workspace targets. Subprojects also use this contract when built alone.
if(CLIP_APPS_BUILD_ANT_DESIGN_QT OR CLIP_APPS_BUILD_DRAW_ENGINE OR
   CLIP_APPS_BUILD_IMAGE_VIEWER OR CLIP_APPS_BUILD_CLIP_SHOT)
    clip_find_qt(COMPONENTS Core Gui GLOBAL)
endif()
clip_workspace_configure_options()

clip_msvc_arm64_requires_native_codegen(_clip_arm64_native_codegen)
if(_clip_arm64_native_codegen)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL OFF)
    add_compile_options($<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:/GL->)
elseif(CLIP_APPS_ENABLE_RELEASE_OPTIMIZATION)
    # CMake's IPO abstraction selects /GL + /LTCG on MSVC and -flto on GCC/Clang.
    # Applying it at directory scope also covers the C++ libraries that feed the
    # Clip executable, instead of optimizing only the final executable.
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
endif()

# Resolve the Clip Image codec profile before any child project can configure
# the shared clip_image target. Child-level cache writes would otherwise occur
# after target creation and leave the cache claiming codecs that were never
# compiled into the library.
if(NOT DEFINED CLIP_IMAGE_PROFILE OR CLIP_IMAGE_PROFILE STREQUAL "")
    set(CLIP_IMAGE_PROFILE "full" CACHE STRING
        "Clip Image codec profile used by the workspace." FORCE)
endif()
set(_clip_workspace_all_image_codecs
    PNG JPEG GIF ICON XBITMAP SVG HEIF JXL EXR WEBP
)
if(CLIP_IMAGE_PROFILE STREQUAL "full")
    set(_clip_workspace_image_codecs
        ${_clip_workspace_all_image_codecs}
    )
elseif(CLIP_APPS_BUILD_CLIP_SHOT)
    # Clip's Save As workflow supports all five raster formats in every
    # application profile, so keep the corresponding codecs in the minimal
    # workspace configuration as well as the full profile.
    set(_clip_workspace_image_codecs PNG JPEG HEIF JXL WEBP)
else()
    set(_clip_workspace_image_codecs)
endif()
foreach(_clip_image_codec IN LISTS _clip_workspace_all_image_codecs)
    list(FIND _clip_workspace_image_codecs "${_clip_image_codec}" _clip_image_codec_index)
    if(_clip_image_codec_index GREATER -1)
        set(_clip_image_codec_value ON)
    else()
        set(_clip_image_codec_value OFF)
    endif()
    set(CLIP_IMAGE_CODEC_${_clip_image_codec} ${_clip_image_codec_value} CACHE BOOL
        "Enable the ${_clip_image_codec} Clip Image codec for this workspace profile." FORCE)
endforeach()
unset(_clip_image_codec)
unset(_clip_image_codec_index)
unset(_clip_image_codec_value)
unset(_clip_workspace_all_image_codecs)
unset(_clip_workspace_image_codecs)

if(CLIP_APPS_BUILD_TESTS OR CLIP_SHOT_BUILD_RUNTIME_TESTS)
    enable_testing()
endif()

if(CLIP_APPS_BUILD_CLIP_SHOT)
    set(_clip_scrolling_perf_features)
    if(CLIP_SHOT_SCROLLING_PERF_DETAIL)
        list(APPEND _clip_scrolling_perf_features perf-instrumentation)
    endif()
    clip_add_rust_static_library(clip_shot_rust_ffi_bundle
        PACKAGE clip-shot-rust-ffi
        MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/clip_rust_ffi"
        OUTPUT_NAME clip_shot_rust_ffi
        FEATURES ${_clip_scrolling_perf_features}
        STRIP_MSVC_DIRECTIVES
    )
    clip_add_rust_bundle_selector(clip_shot_rust_ffi_selector clip_shot_rust_ffi_bundle)
    foreach(_clip_rust_ffi_target IN ITEMS
            clip_capture_c
            clip_recording_c
            clip_recording_effects_c
            clip_ui_selector_c
            clip_stitch_images_c
            clip_draw_engine_c
            clip_visual_region_detector_c)
        add_library("${_clip_rust_ffi_target}" INTERFACE)
        target_link_libraries("${_clip_rust_ffi_target}" INTERFACE
            clip_shot_rust_ffi_selector)
    endforeach()
    unset(_clip_rust_ffi_target)
    target_include_directories(clip_recording_effects_c INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/clip-crates/crates/clip-recording-effects-c/include")
endif()

if(CLIP_APPS_BUILD_IMAGE_VIEWER AND NOT CLIP_APPS_BUILD_CLIP_IMAGE)
    message(FATAL_ERROR "CLIP_APPS_BUILD_IMAGE_VIEWER requires CLIP_APPS_BUILD_CLIP_IMAGE=ON")
endif()
if(CLIP_APPS_BUILD_ANT_DESIGN_QT AND NOT CLIP_APPS_BUILD_CLIP_IMAGE)
    message(FATAL_ERROR "CLIP_APPS_BUILD_ANT_DESIGN_QT requires CLIP_APPS_BUILD_CLIP_IMAGE=ON")
endif()
if(CLIP_APPS_BUILD_CLIP_SHOT AND
   (NOT CLIP_APPS_BUILD_ANT_DESIGN_QT OR
    NOT CLIP_APPS_BUILD_CLIP_IMAGE OR
    NOT CLIP_APPS_BUILD_DRAW_ENGINE))
    message(FATAL_ERROR
        "CLIP_APPS_BUILD_CLIP_SHOT requires Ant Design Qt, Clip Image, and the Draw Engine")
endif()

if(CLIP_APPS_BUILD_CLIP_IMAGE)
    set(CLIP_IMAGE_BUILD_TESTS ${CLIP_APPS_BUILD_TESTS} CACHE BOOL
        "Build Clip Image tests." FORCE)
    if(CLIP_APPS_RELEASE_STATIC)
        set(CLIP_IMAGE_BUILD_SHARED OFF CACHE BOOL
            "Build the shared Clip Image library." FORCE)
    else()
        set(CLIP_IMAGE_BUILD_SHARED ON CACHE BOOL
            "Build the shared Clip Image library." FORCE)
    endif()
    set(CLIP_IMAGE_BUILD_STATIC ON CACHE BOOL
        "Build the static Clip Image library." FORCE)
    set(CLIP_IMAGE_BUILD_BENCHMARKS ${CLIP_APPS_BUILD_BENCHMARKS} CACHE BOOL
        "Build Clip Image benchmarks." FORCE)
    set(CLIP_IMAGE_ENABLE_INSTALL OFF CACHE BOOL
        "Install Clip Image as a standalone package." FORCE)
    add_subdirectory(clip_image "${CMAKE_BINARY_DIR}/clip_image")
endif()

if(CLIP_APPS_BUILD_ANT_DESIGN_QT)
    # Shipping builds retain all themes of icons used by the app and its widgets.
    # Development, benchmarks and other apps keep the public library's full pack.
    if(CLIP_APPS_ENABLE_RELEASE_SIZE_OPTIMIZATION AND
       CLIP_APPS_BUILD_CLIP_SHOT AND CLIP_APPS_PACKAGE_CLIP_SHOT AND
       NOT CLIP_APPS_BUILD_TESTS AND NOT CLIP_APPS_BUILD_BENCHMARKS AND
       NOT CLIP_APPS_BUILD_IMAGE_VIEWER)
        set(ADQT_ICON_USAGE_ROOTS
            "${CMAKE_CURRENT_SOURCE_DIR}/clip_shot/src"
            "${CMAKE_CURRENT_SOURCE_DIR}/clip_shot/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/clip_draw_engine_qt/src"
            "${CMAKE_CURRENT_SOURCE_DIR}/clip_draw_engine_qt/include")
    else()
        set(ADQT_ICON_USAGE_ROOTS "")
    endif()
    add_subdirectory(ant_design_qt "${CMAKE_BINARY_DIR}/ant_design_qt")
endif()

if(CLIP_APPS_BUILD_DRAW_ENGINE)
    set(CLIP_DRAW_ENGINE_QT_BUILD_DEMO ${CLIP_APPS_BUILD_DRAW_ENGINE_DEMO} CACHE BOOL
        "Build the Clip Draw Engine Qt demo." FORCE)
    set(CLIP_DRAW_ENGINE_QT_BUILD_TESTS ${CLIP_APPS_BUILD_TESTS} CACHE BOOL
        "Build Clip Draw Engine Qt tests." FORCE)
    set(CLIP_DRAW_ENGINE_QT_BUILD_BENCHMARKS ${CLIP_APPS_BUILD_BENCHMARKS} CACHE BOOL
        "Build Clip Draw Engine Qt benchmarks." FORCE)
    set(CLIP_DRAW_ENGINE_QT_STRICT_COMPILE ${CLIP_APPS_STRICT_COMPILE} CACHE BOOL
        "Strict Clip Draw Engine Qt compilation." FORCE)
    add_subdirectory(clip_draw_engine_qt "${CMAKE_BINARY_DIR}/clip_draw_engine_qt")
endif()

if(CLIP_APPS_BUILD_IMAGE_VIEWER)
    set(CLIP_IMAGE_VIEWER_BUILD_TESTS ${CLIP_APPS_BUILD_TESTS} CACHE BOOL
        "Build Clip Image Viewer tests." FORCE)
    set(CLIP_IMAGE_VIEWER_STRICT_COMPILE ${CLIP_APPS_STRICT_COMPILE} CACHE BOOL
        "Strict Clip Image Viewer compilation." FORCE)
    set(CLIP_IMAGE_VIEWER_VCPKG_ROOT "${CLIP_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
        CACHE PATH "Root-managed vcpkg installation used for runtime staging." FORCE)
    add_subdirectory(clip_image_viewer "${CMAKE_BINARY_DIR}/clip_image_viewer")
endif()

if(CLIP_APPS_BUILD_CLIP_SHOT)
    set(CLIP_SHOT_BUILD_TESTS ${CLIP_APPS_BUILD_TESTS} CACHE BOOL
        "Build Clip tests." FORCE)
    set(CLIP_SHOT_BUILD_BENCHMARKS ${CLIP_APPS_BUILD_BENCHMARKS} CACHE BOOL
        "Build Clip benchmarks." FORCE)
    set(CLIP_SHOT_ENABLE_STRICT_COMPILE ${CLIP_APPS_STRICT_COMPILE} CACHE BOOL
        "Strict Clip compilation." FORCE)
    set(CLIP_SHOT_RELEASE_STATIC ${CLIP_APPS_RELEASE_STATIC} CACHE BOOL
        "Prefer static Clip release linkage." FORCE)
    set(CLIP_SHOT_QT_STATIC ${CLIP_APPS_QT_STATIC} CACHE BOOL
        "Link Clip against a static Qt build." FORCE)
    set(CLIP_SHOT_API_BASE_URL "${CLIP_APPS_API_BASE_URL}" CACHE STRING
        "Clip API base URL." FORCE)
    set(CLIP_SHOT_LIBCLANG_DIR "${CLIP_LIBCLANG_BIN_DIR}" CACHE PATH
        "Directory containing libclang.dll for Rust bindgen." FORCE)
    set(CLIP_SHOT_FFMPEG_BIN_DIR "${CLIP_FFMPEG_ROOT}/bin" CACHE PATH
        "Directory containing FFmpeg runtime DLLs." FORCE)
    set(CLIP_DRAW_ENGINE_QT_BUILD_DEMO ${CLIP_APPS_BUILD_DRAW_ENGINE_DEMO} CACHE BOOL
        "Build the nested draw engine demo." FORCE)
    set(CLIP_DRAW_ENGINE_QT_BUILD_TESTS ${CLIP_APPS_BUILD_TESTS} CACHE BOOL
        "Build nested draw engine tests." FORCE)
    set(CLIP_DRAW_ENGINE_QT_BUILD_BENCHMARKS ${CLIP_APPS_BUILD_BENCHMARKS} CACHE BOOL
        "Build nested draw engine benchmarks." FORCE)
    add_subdirectory(clip_shot "${CMAKE_BINARY_DIR}/clip_shot")
endif()

function(_clip_apply_test_labels directory)
    get_property(_directory_tests DIRECTORY "${directory}" PROPERTY TESTS)
    foreach(_clip_test IN LISTS _directory_tests)
        unset(_clip_test_labels)
        get_property(_clip_test_labels TEST "${_clip_test}" DIRECTORY "${directory}" PROPERTY LABELS)
        if(NOT _clip_test_labels OR
           _clip_test_labels STREQUAL "NOTFOUND" OR
           _clip_test_labels MATCHES "-NOTFOUND$")
            set(_clip_test_labels unit)
        endif()
        if(_clip_test MATCHES "real-mouse|uia-e2e")
            list(APPEND _clip_test_labels interactive e2e)
        elseif(_clip_test MATCHES "benchmark|performance")
            list(APPEND _clip_test_labels benchmark)
        elseif(_clip_test MATCHES "pinned-window|context-menu|toolbar|tooltip|recognition-window")
            get_property(_clip_test_portable TEST "${_clip_test}" DIRECTORY "${directory}"
                PROPERTY CLIP_TEST_CROSS_PLATFORM)
            if(NOT _clip_test_portable)
                list(APPEND _clip_test_labels windows)
            endif()
        endif()
        list(REMOVE_DUPLICATES _clip_test_labels)
        set_tests_properties("${_clip_test}" DIRECTORY "${directory}"
            PROPERTIES LABELS "${_clip_test_labels}")
    endforeach()

    get_property(_child_directories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(_child_directory IN LISTS _child_directories)
        _clip_apply_test_labels("${_child_directory}")
    endforeach()
endfunction()

if(CLIP_APPS_BUILD_TESTS OR CLIP_SHOT_BUILD_RUNTIME_TESTS)
    _clip_apply_test_labels("${CMAKE_CURRENT_BINARY_DIR}")
endif()

function(_clip_collect_build_targets directory output_variable)
    get_property(_directory_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(_child_directories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    set(_collected_targets ${_directory_targets})
    foreach(_child_directory IN LISTS _child_directories)
        _clip_collect_build_targets("${_child_directory}" _child_targets)
        list(APPEND _collected_targets ${_child_targets})
    endforeach()
    list(REMOVE_DUPLICATES _collected_targets)
    set(${output_variable} "${_collected_targets}" PARENT_SCOPE)
endfunction()

if(NOT TARGET clip-all)
    add_custom_target(clip-all)
    _clip_collect_build_targets("${CMAKE_CURRENT_BINARY_DIR}" _clip_workspace_targets)
    foreach(_clip_target IN LISTS _clip_workspace_targets)
        if(TARGET ${_clip_target})
            get_target_property(_clip_target_type "${_clip_target}" TYPE)
            get_target_property(
                _clip_target_exclude_from_all "${_clip_target}" EXCLUDE_FROM_ALL)
        endif()
        if(NOT _clip_target_exclude_from_all AND
           _clip_target_type MATCHES
           "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
            add_dependencies(clip-all ${_clip_target})
        endif()
        unset(_clip_target_exclude_from_all)
        unset(_clip_target_type)
    endforeach()
    unset(_clip_workspace_targets)
endif()

if(NOT TARGET clip-tests)
    set(_clip_deterministic_test_exclude "interactive^|e2e^|benchmark")
    add_custom_target(clip-tests
        COMMAND ${CMAKE_CTEST_COMMAND}
            --test-dir "${CMAKE_BINARY_DIR}"
            -C "$<CONFIG>"
            -LE ${_clip_deterministic_test_exclude}
            --output-on-failure
        USES_TERMINAL
    )
    add_dependencies(clip-tests clip-all)
    unset(_clip_deterministic_test_exclude)
endif()

if(NOT TARGET clip-benchmarks)
    add_custom_target(clip-benchmarks)
    foreach(_clip_benchmark IN ITEMS
        clip-shot-history-performance-benchmark
        clip-shot-direct-recording-performance-benchmark
        clip-shot-realtime-recording-performance-benchmark
        clip-shot-selection-render-benchmark
        clip-shot-screenshot-toolbar-render-benchmark
        clip-shot-save-to-file-performance-benchmark
        clip-shot-scrolling-preview-benchmark
        clip-shot-scrolling-result-async-benchmark
        clip-shot-scrolling-image-performance-benchmark
    )
        if(TARGET ${_clip_benchmark})
            add_dependencies(clip-benchmarks ${_clip_benchmark})
        endif()
    endforeach()
endif()

set(_clip_format_command powershell -NoProfile -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check-cpp-format.ps1")
set(_clip_lint_command powershell -NoProfile -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check-rust.ps1")
if(NOT TARGET clip-format)
    add_custom_target(clip-format COMMAND ${_clip_format_command} USES_TERMINAL)
endif()
if(NOT TARGET clip-lint)
    add_custom_target(clip-lint COMMAND ${_clip_lint_command} USES_TERMINAL)
endif()

if(CLIP_APPS_PACKAGE_CLIP_SHOT)
    if(NOT TARGET clip_shot)
        message(FATAL_ERROR "CLIP_APPS_PACKAGE_CLIP_SHOT requires CLIP_APPS_BUILD_CLIP_SHOT=ON")
    endif()
    set(CPACK_PACKAGE_NAME "clip-shot")
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${CMAKE_PROJECT_NAME};ClipShot;/")
    set(CPACK_COMPONENTS_ALL ClipShot)
    set(CPACK_PACKAGE_VENDOR "${CLIP_SHOT_VENDOR}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CLIP_SHOT_PRODUCT_NAME} screenshot utility")
    set(CPACK_PACKAGE_DESCRIPTION "${CLIP_SHOT_DESCRIPTION}")
    set(CPACK_PACKAGE_HOMEPAGE_URL "${CLIP_SHOT_HOMEPAGE_URL}")
    set(CPACK_PACKAGE_VERSION "${CLIP_SHOT_VERSION}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "ClipShot")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "ClipShot")
    set(CPACK_PACKAGE_FILE_NAME "clip-shot-${CLIP_SHOT_VERSION}-${CLIP_WINDOWS_PLATFORM}")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CPACK_PACKAGE_CHECKSUM "SHA256")
    set(CPACK_PACKAGE_DESCRIPTION_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/clip_shot/packaging/README.txt")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/clip_shot/LICENSE")
    set(CPACK_RESOURCE_FILE_README
        "${CMAKE_CURRENT_SOURCE_DIR}/clip_shot/packaging/README.txt")
    set(CPACK_PACKAGE_EXECUTABLES
        "${CLIP_SHOT_EXECUTABLE_NAME}" "${CLIP_SHOT_PRODUCT_NAME}")
    set(CPACK_CREATE_DESKTOP_LINKS "${CLIP_SHOT_EXECUTABLE_NAME}")
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "${CLIP_SHOT_PRODUCT_NAME}")
    set(CPACK_NSIS_PACKAGE_NAME "${CLIP_SHOT_PRODUCT_NAME}")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${CLIP_SHOT_EXECUTABLE_NAME}.exe")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/clip_shot/resources/app-icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/clip_shot/resources/app-icon.ico")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${CLIP_SHOT_EXECUTABLE_NAME}.exe")
    set(CPACK_NSIS_UNINSTALL_NAME "Uninstall ${CLIP_SHOT_PRODUCT_NAME}")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    string(CONCAT CPACK_NSIS_DEFINES
        "VIProductVersion \"${CLIP_SHOT_VERSION_NUMERIC}.0\"\n"
        "VIAddVersionKey /LANG=1033 \"CompanyName\" \"${CLIP_SHOT_VENDOR}\"\n"
        "VIAddVersionKey /LANG=1033 \"FileDescription\" \"${CLIP_SHOT_PRODUCT_NAME} installer\"\n"
        "VIAddVersionKey /LANG=1033 \"FileVersion\" \"${CLIP_SHOT_VERSION_NUMERIC}.0\"\n"
        "VIAddVersionKey /LANG=1033 \"InternalName\" \"clip-shot-installer\"\n"
        "VIAddVersionKey /LANG=1033 \"LegalCopyright\" \"${CLIP_SHOT_COPYRIGHT}\"\n"
        "VIAddVersionKey /LANG=1033 \"OriginalFilename\" \"${CPACK_PACKAGE_FILE_NAME}.exe\"\n"
        "VIAddVersionKey /LANG=1033 \"ProductName\" \"${CLIP_SHOT_PRODUCT_NAME}\"\n"
        "VIAddVersionKey /LANG=1033 \"ProductVersion\" \"${CLIP_SHOT_VERSION}\""
    )
    set(CPACK_NSIS_BRANDING_TEXT "${CLIP_SHOT_PRODUCT_NAME} ${CLIP_SHOT_VERSION}")
    set(CPACK_NSIS_BRANDING_TEXT_TRIM_POSITION "CENTER")
    set(CPACK_NSIS_HELP_LINK "${CLIP_SHOT_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CLIP_SHOT_HOMEPAGE_URL}")
    set(CPACK_NSIS_CONTACT "${CLIP_SHOT_SUPPORT_EMAIL}")
    set(CPACK_VERBATIM_VARIABLES ON)
    include(cmake/ClipShotPackageCompression.cmake)
    clip_shot_apply_nsis_compression()
    include(cmake/ClipShotInstaller.cmake)
    include(CPack)
endif()

if(CLIP_APPS_PACKAGE_CLIP_SHOT)
    include(cmake/ClipShotMiniPackage.cmake)
endif()
