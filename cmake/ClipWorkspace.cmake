include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/ClipRustLibclang.cmake")

function(clip_workspace_configure_paths)
    set(_vcpkg_default "${CMAKE_CURRENT_SOURCE_DIR}/.tools/vcpkg")
    if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        set(_vcpkg_default "$ENV{VCPKG_ROOT}")
    endif()

    set(CLIP_VCPKG_ROOT "${_vcpkg_default}" CACHE PATH
        "Repository-local vcpkg root used by the active build.")
    set(CLIP_ANT_DESIGN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ant_design_qt" CACHE PATH
        "Ant Design Qt source directory.")
    set(CLIP_IMAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/clip_image" CACHE PATH
        "Clip Image source directory.")
    set(CLIP_DRAW_ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/clip_draw_engine_qt" CACHE PATH
        "Clip Draw Engine Qt source directory.")
    set(CLIP_CAPTURE_CRATES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/clip-crates" CACHE PATH
        "Clip crates workspace directory.")
    clip_default_rust_libclang_directory(_clip_libclang_default)
    set(CLIP_LIBCLANG_BIN_DIR "${_clip_libclang_default}" CACHE PATH
        "Directory containing libclang.dll for the host Rust bindgen process.")

    if(NOT DEFINED VCPKG_TARGET_TRIPLET OR VCPKG_TARGET_TRIPLET STREQUAL "")
        if(WIN32)
            include(ClipWindowsArchitecture)
            clip_configure_windows_architecture()
            set(_clip_default_triplet "${CLIP_WINDOWS_ARCHITECTURE}-windows")
        else()
            set(_clip_default_triplet "x64-windows")
        endif()
        set(VCPKG_TARGET_TRIPLET "${_clip_default_triplet}" CACHE STRING
            "vcpkg target triplet used by this build." FORCE)
    endif()
    set(_vcpkg_installed_default "${CLIP_VCPKG_ROOT}/installed")
    if(DEFINED VCPKG_INSTALLED_DIR AND NOT VCPKG_INSTALLED_DIR STREQUAL "")
        set(_vcpkg_installed_default "${VCPKG_INSTALLED_DIR}")
    endif()
    set(CLIP_VCPKG_INSTALLED_DIR "${_vcpkg_installed_default}" CACHE PATH
        "vcpkg installed tree used by the active build preset.")
    set(CLIP_FFMPEG_ROOT "${CLIP_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}" CACHE PATH
        "vcpkg installation prefix containing FFmpeg.")

    foreach(_required_dir IN ITEMS
        CLIP_ANT_DESIGN_SOURCE_DIR
        CLIP_IMAGE_SOURCE_DIR
        CLIP_DRAW_ENGINE_SOURCE_DIR
        CLIP_CAPTURE_CRATES_DIR
    )
        if(NOT IS_DIRECTORY "${${_required_dir}}")
            message(FATAL_ERROR
                "${_required_dir} does not exist: ${${_required_dir}}. "
                "Run scripts/bootstrap.ps1 or set the path explicitly.")
        endif()
    endforeach()

    if(NOT EXISTS "${CLIP_CAPTURE_CRATES_DIR}/Cargo.toml")
        message(FATAL_ERROR "Clip crates workspace is missing Cargo.toml: ${CLIP_CAPTURE_CRATES_DIR}")
    endif()
endfunction()

function(clip_workspace_configure_options)
    set(CLIP_ENABLE_CLANG_TIDY ${CLIP_APPS_ENABLE_CLANG_TIDY} CACHE BOOL
        "Run clang-tidy for enabled Clip projects." FORCE)

    set(ADQT_BUILD_TESTS ${CLIP_APPS_BUILD_TESTS} CACHE BOOL
        "Build Ant Design Qt tests." FORCE)
    set(ADQT_BUILD_BENCHMARKS ${CLIP_APPS_BUILD_BENCHMARKS} CACHE BOOL
        "Build Ant Design Qt benchmarks." FORCE)
    set(ADQT_STRICT_COMPILE ${CLIP_APPS_STRICT_COMPILE} CACHE BOOL
        "Strict Ant Design Qt compilation." FORCE)

    set(CLIP_IMAGE_STRICT_COMPILE ${CLIP_APPS_STRICT_COMPILE} CACHE BOOL
        "Strict Clip Image compilation." FORCE)
    set(CLIP_IMAGE_DEFAULT_LINKAGE static CACHE STRING
        "Default Clip Image target linkage." FORCE)

    # Static Qt kits can carry bundled image codecs. Avoid linking second
    # archives with the same symbols; production kits use the system codecs.
    # Read the resolved kit's imported targets rather than its generated files.
    set(CLIP_IMAGE_LINK_PNG_DEPENDENCIES ON CACHE BOOL
        "Link the PNG dependency into the static Clip Image target." FORCE)
    set(CLIP_IMAGE_LINK_JPEG_DEPENDENCIES ON CACHE BOOL
        "Link JPEG dependencies into the static Clip Image target." FORCE)
    if(TARGET Qt6::Gui)
        get_target_property(_clip_workspace_qt_gui_type Qt6::Gui TYPE)
        get_target_property(_clip_workspace_qt_gui_features Qt6::Gui QT_ENABLED_PRIVATE_FEATURES)
        if(_clip_workspace_qt_gui_type STREQUAL "STATIC_LIBRARY")
            if(NOT "system_png" IN_LIST _clip_workspace_qt_gui_features)
                set(CLIP_IMAGE_LINK_PNG_DEPENDENCIES OFF CACHE BOOL
                    "Link the PNG dependency into the static Clip Image target." FORCE)
            endif()
            if(NOT "system_jpeg" IN_LIST _clip_workspace_qt_gui_features)
                set(CLIP_IMAGE_LINK_JPEG_DEPENDENCIES OFF CACHE BOOL
                    "Link JPEG dependencies into the static Clip Image target." FORCE)
            endif()
        endif()
    endif()

    # Static Qt can use either CRT linkage. Its static_runtime feature is the
    # authority, independent of the kit's directory name or library linkage.
    if(MSVC AND TARGET Qt6::Core)
        get_target_property(_clip_workspace_qt_core_features Qt6::Core QT_ENABLED_PUBLIC_FEATURES)
        if("static_runtime" IN_LIST _clip_workspace_qt_core_features)
            set(_clip_workspace_crt "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        else()
            set(_clip_workspace_crt "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        endif()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "${_clip_workspace_crt}" CACHE STRING
            "MSVC runtime matching the resolved Qt kit." FORCE)
    endif()

    if(CLIP_APPS_RELEASE_STATIC AND MSVC)
        if(TARGET Qt6::Core AND NOT "static_runtime" IN_LIST _clip_workspace_qt_core_features)
            message(FATAL_ERROR
                "CLIP_APPS_RELEASE_STATIC requires a Qt kit built with the static MSVC runtime")
        endif()
        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded CACHE STRING
            "MSVC runtime used for static release builds." FORCE)
    endif()

    if(CLIP_APPS_RELEASE_STATIC AND VCPKG_TARGET_TRIPLET MATCHES "^(x64|arm64)-windows$")
        message(WARNING
            "CLIP_APPS_RELEASE_STATIC is enabled with ${VCPKG_TARGET_TRIPLET}. "
            "Use the matching Clip release preset for static vcpkg libraries.")
    endif()

    if(CLIP_APPS_API_BASE_URL STREQUAL "")
        if(CMAKE_CONFIGURATION_TYPES)
            set(CLIP_APPS_API_BASE_URL "https://clipshot.top" CACHE STRING
                "Default Clip API base URL. Set explicitly for reproducible builds." FORCE)
        elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CLIP_APPS_API_BASE_URL "http://localhost:8080" CACHE STRING
                "Default Clip API base URL." FORCE)
        else()
            set(CLIP_APPS_API_BASE_URL "https://clipshot.top" CACHE STRING
                "Default Clip API base URL." FORCE)
        endif()
    endif()
endfunction()
