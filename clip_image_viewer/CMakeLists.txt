cmake_minimum_required(VERSION 3.25)

project(clip_image_viewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLIP_IMAGE_VIEWER_BUILD_TESTS "Build clip_image_viewer tests." ON)
option(CLIP_IMAGE_VIEWER_STRICT_COMPILE "Treat viewer warnings as errors." ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ProjectWarnings.cmake")
if(CLIP_IMAGE_VIEWER_STRICT_COMPILE)
    clip_enable_strict_warnings()
endif()
clip_enable_clang_tidy()

set(QT_NO_PRIVATE_MODULE_WARNING ON)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClipQt.cmake")
clip_find_qt(COMPONENTS Widgets Svg Network Concurrent)
clip_find_qt_private(Gui)
find_package(VulkanHeaders CONFIG REQUIRED)

if(NOT TARGET clip_image::clip_image)
    set(CLIP_IMAGE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CLIP_IMAGE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(CLIP_IMAGE_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(CLIP_IMAGE_DEFAULT_LINKAGE static CACHE STRING "" FORCE)
    set(CLIP_IMAGE_PROFILE redistributable CACHE STRING "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../clip_image"
                     "${CMAKE_CURRENT_BINARY_DIR}/clip_image")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AntDesignQt)
clip_image_viewer_add_ant_design_qt()

set(CLIP_IMAGE_VIEWER_SOURCES
    src/app/application_setup.cpp
    src/app/application_setup.h
    src/core/folder_sequence.cpp
    src/core/folder_sequence.h
    src/core/image_raster_store.cpp
    src/core/image_raster_store.h
    src/core/image_types.cpp
    src/core/image_types.h
    src/core/image_tile_store.cpp
    src/core/image_tile_store.h
    src/core/view_transform.cpp
    src/core/view_transform.h
    src/decoding/image_decoder.h
    src/decoding/image_loader.cpp
    src/decoding/image_loader.h
    src/decoding/clip_image_decoder.cpp
    src/decoding/clip_image_decoder.h
    src/decoding/system_thumbnail.cpp
    src/decoding/system_thumbnail.h
    src/editing/edit_export_settings.h
    src/editing/encoded_artifact.cpp
    src/editing/encoded_artifact.h
    src/editing/edit_pipeline_controller.cpp
    src/editing/edit_pipeline_controller.h
    src/editing/gpu_resize_readback.h
    src/editing/raster_package.cpp
    src/editing/raster_package.h
    src/editing/raster_asset.h
    src/editing/temporary_file_lease.cpp
    src/editing/temporary_file_lease.h
    src/editing/worker_protocol.cpp
    src/editing/worker_protocol.h
    src/editing/worker_core.cpp
    src/editing/worker_core.h
    src/platform/windows_background.h
    src/platform/windows_file_explorer.cpp
    src/platform/windows_file_explorer.h
    src/platform/windows_open_with.cpp
    src/platform/windows_open_with.h
    src/platform/windows_open_with_internal.h
    src/platform/windows_print.h
    src/platform/windows_share.h
    src/render/rhi_image_window.cpp
    src/render/rhi_image_window.h
    src/render/texture_size.h
    src/ui/app_icons.cpp
    src/ui/app_icons.h
    src/ui/icon_theme_adapter.cpp
    src/ui/icon_theme_adapter.h
    src/ui/edit_size_format_window.cpp
    src/ui/edit_size_format_window.h
    src/ui/viewer_window.cpp
    src/ui/viewer_window.h
)

if(WIN32)
    add_library(clip_image_viewer_windows_runtime OBJECT
        src/platform/windows_background.cpp
        src/platform/windows_print.cpp
        src/platform/windows_share.cpp
    )
    target_include_directories(
        clip_image_viewer_windows_runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(
        clip_image_viewer_windows_runtime
        PRIVATE Qt6::Widgets Qt6::Concurrent clip_image::clip_image
    )
    target_compile_definitions(
        clip_image_viewer_windows_runtime PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(
            clip_image_viewer_windows_runtime PRIVATE /FS /utf-8 /permissive- /Zc:__cplusplus
        )
    endif()
    if(CLIP_ENABLE_CLANG_TIDY)
        # C++/WinRT ABI projections and Windows flag enums are mis-modeled by these
        # two analyzer checks. All other configured checks still run on this source.
        set(CLIP_WINDOWS_RUNTIME_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY})
        list(APPEND CLIP_WINDOWS_RUNTIME_CLANG_TIDY
            "--checks=-clang-analyzer-core.CallAndMessage,-clang-analyzer-optin.core.EnumCastOutOfRange"
        )
        set_property(
            TARGET clip_image_viewer_windows_runtime
            PROPERTY CXX_CLANG_TIDY "${CLIP_WINDOWS_RUNTIME_CLANG_TIDY}"
        )
    endif()
else()
    list(APPEND CLIP_IMAGE_VIEWER_SOURCES
        src/platform/windows_background.cpp
        src/platform/windows_print.cpp
        src/platform/windows_share.cpp
    )
endif()

add_library(clip_image_viewer_lib STATIC ${CLIP_IMAGE_VIEWER_SOURCES})
if(WIN32)
    target_sources(
        clip_image_viewer_lib PRIVATE $<TARGET_OBJECTS:clip_image_viewer_windows_runtime>
    )
endif()
target_include_directories(clip_image_viewer_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(
    clip_image_viewer_lib
    PUBLIC
        Qt6::Widgets
        Qt6::Concurrent
        adqt::widgets
        adqt::icons
        clip_image::clip_image
        Vulkan::Headers
    PRIVATE
        Qt6::GuiPrivate
)

add_executable(clip_image_viewer WIN32
    src/app/main.cpp
    resources/resources.qrc
)
target_link_libraries(clip_image_viewer PRIVATE clip_image_viewer_lib)

add_executable(clip_image_edit_worker src/editing/worker_main.cpp)
target_link_libraries(clip_image_edit_worker PRIVATE clip_image_viewer_lib)
add_dependencies(clip_image_viewer clip_image_edit_worker)

if(WIN32)
    target_compile_definitions(clip_image_viewer_lib PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_definitions(clip_image_edit_worker PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(clip_image_viewer_lib PRIVATE d2d1 d3d11 dxgi gdi32 ole32 psapi propsys runtimeobject shell32 windowsapp windowscodecs)
endif()

if(MSVC)
    target_compile_options(clip_image_viewer_lib PRIVATE /FS /utf-8 /permissive- /Zc:__cplusplus)
    target_compile_options(clip_image_viewer PRIVATE /FS /utf-8 /permissive- /Zc:__cplusplus)
    target_compile_options(clip_image_edit_worker PRIVATE /FS /utf-8 /permissive- /Zc:__cplusplus)
    if(CLIP_IMAGE_VIEWER_STRICT_COMPILE)
        target_compile_options(clip_image_viewer_lib PRIVATE /W4 /WX)
        target_compile_options(clip_image_viewer PRIVATE /W4 /WX)
        target_compile_options(clip_image_edit_worker PRIVATE /W4 /WX)
    endif()
endif()

if(WIN32)
    get_target_property(CLIP_IMAGE_VIEWER_QT_CORE_TYPE Qt6::Core TYPE)
    if(CLIP_IMAGE_VIEWER_QT_CORE_TYPE STREQUAL "SHARED_LIBRARY")
        find_program(CLIP_IMAGE_VIEWER_WINDEPLOYQT
            NAMES windeployqt
            HINTS "${Qt6_DIR}/../../../bin"
            REQUIRED
        )
    endif()
    add_custom_command(
        TARGET clip_image_viewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:clip_image_viewer>
            $<TARGET_FILE_DIR:clip_image_viewer>
        COMMAND_EXPAND_LISTS
    )
    if(DEFINED CLIP_IMAGE_VIEWER_VCPKG_ROOT)
        add_custom_command(
            TARGET clip_image_viewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "$<IF:$<CONFIG:Debug>,${CLIP_IMAGE_VIEWER_VCPKG_ROOT}/debug/bin,${CLIP_IMAGE_VIEWER_VCPKG_ROOT}/bin>"
                "$<TARGET_FILE_DIR:clip_image_viewer>"
        )
    endif()
    if(CLIP_IMAGE_VIEWER_QT_CORE_TYPE STREQUAL "SHARED_LIBRARY")
        add_custom_command(
            TARGET clip_image_viewer POST_BUILD
            COMMAND "${CLIP_IMAGE_VIEWER_WINDEPLOYQT}"
                "$<$<CONFIG:Debug>:--debug>"
                "$<$<NOT:$<CONFIG:Debug>>:--release>"
                --no-translations
                --no-compiler-runtime
                "$<TARGET_FILE:clip_image_viewer>"
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(CLIP_IMAGE_VIEWER_BUILD_TESTS)
    enable_testing()
    clip_find_qt(COMPONENTS Test)
    add_subdirectory(tests)
endif()
