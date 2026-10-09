# The Rust static archive does not propagate Cargo's native link metadata to CMake.
find_package(PkgConfig REQUIRED)
set(_clip_saved_pkg_config_path "$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${CLIP_FFMPEG_ROOT}/lib/pkgconfig")
set(_clip_saved_pkg_config_argn "${PKG_CONFIG_ARGN}")
if(CLIP_SHOT_RELEASE_STATIC)
    set(PKG_CONFIG_ARGN --static)
endif()
pkg_check_modules(CLIP_SHOT_FFMPEG REQUIRED IMPORTED_TARGET GLOBAL
    libavformat libavcodec libswresample libswscale libavutil)
set(PKG_CONFIG_ARGN "${_clip_saved_pkg_config_argn}")
set(ENV{PKG_CONFIG_PATH} "${_clip_saved_pkg_config_path}")

# FindPkgConfig places Apple's two-token `-framework Name` pairs in
# INTERFACE_LINK_OPTIONS. CMake de-duplicates repeated options, which can leave
# later framework names as bare linker inputs. The complete framework closure
# is resolved with find_library below, so remove the unsafe pkg-config pairs
# while preserving unrelated options such as -pthread.
include("${CMAKE_CURRENT_LIST_DIR}/ClipPkgConfigAppleFrameworks.cmake")
get_target_property(_clip_ffmpeg_link_options
    PkgConfig::CLIP_SHOT_FFMPEG INTERFACE_LINK_OPTIONS)
if(_clip_ffmpeg_link_options)
    clip_strip_pkg_config_apple_framework_options(
        _clip_ffmpeg_link_options ${_clip_ffmpeg_link_options})
    set_property(TARGET PkgConfig::CLIP_SHOT_FFMPEG PROPERTY
        INTERFACE_LINK_OPTIONS "${_clip_ffmpeg_link_options}")
endif()
set(_clip_native_libraries PkgConfig::CLIP_SHOT_FFMPEG objc)
foreach(_framework IN ITEMS AppKit ApplicationServices AVFoundation AudioToolbox
        Carbon CoreAudio CoreFoundation CoreGraphics CoreImage CoreMedia CoreVideo
        Foundation IOSurface Metal ScreenCaptureKit Security VideoToolbox)
    find_library(CLIP_MACOS_${_framework} NAMES ${_framework} REQUIRED)
    list(APPEND _clip_native_libraries "${CLIP_MACOS_${_framework}}")
endforeach()
if(TARGET clip_shot_rust_ffi_bundle)
    # Each feature variant is a complete Rust archive. Full hosts omit the
    # default archive, so both variants need their native dependency closure.
    foreach(_clip_rust_bundle IN ITEMS
            clip_shot_rust_ffi_bundle clip_shot_full_rust_ffi_bundle)
        if(TARGET ${_clip_rust_bundle})
            target_link_libraries(${_clip_rust_bundle} INTERFACE ${_clip_native_libraries})
        endif()
    endforeach()
else()
    target_link_libraries(clip_recording_c INTERFACE ${_clip_native_libraries})
    target_link_libraries(clip_capture_c INTERFACE ${_clip_native_libraries})
endif()

set_target_properties(clip_shot PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/macos/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "Clip"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CLIP_SHOT_VERSION_NUMERIC}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${CLIP_SHOT_VERSION_NUMERIC}"
    MACOSX_BUNDLE_ICON_FILE "clip-shot.icns"
    INSTALL_RPATH "@executable_path/../Frameworks"
    INSTALL_RPATH_USE_LINK_PATH TRUE)

foreach(_clip_bundle_language IN ITEMS en zh-Hans zh-Hant)
    set(_clip_bundle_strings
        "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/macos/${_clip_bundle_language}.lproj/InfoPlist.strings")
    set_source_files_properties("${_clip_bundle_strings}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/${_clip_bundle_language}.lproj")
    target_sources(clip_shot PRIVATE "${_clip_bundle_strings}")
endforeach()

set(_clip_macos_icon_artwork
    "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/resources/app-icon.svg")
set(_clip_macos_icon "${CMAKE_CURRENT_BINARY_DIR}/macos/clip-shot.icns")
add_custom_command(
    OUTPUT "${_clip_macos_icon}"
    COMMAND "${CMAKE_COMMAND}"
        -DCLIP_MACOS_ICON_ARTWORK=${_clip_macos_icon_artwork}
        -DCLIP_MACOS_ICON_OUTPUT=${_clip_macos_icon}
        -P "${CMAKE_CURRENT_LIST_DIR}/GenerateMacOSIcon.cmake"
    DEPENDS
        "${_clip_macos_icon_artwork}"
        "${CMAKE_CURRENT_LIST_DIR}/GenerateMacOSIcon.cmake"
    COMMENT "Generating the Clip macOS application icon"
    VERBATIM)
set_source_files_properties("${_clip_macos_icon}" PROPERTIES
    GENERATED TRUE
    MACOSX_PACKAGE_LOCATION Resources)
target_sources(clip_shot PRIVATE "${_clip_macos_icon}")
if(TARGET clip_shot_image_codec_backend)
    set_target_properties(clip_shot_image_codec_backend PROPERTIES
        INSTALL_RPATH "@loader_path" INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
target_link_options(clip_shot PRIVATE -Wl,-headerpad_max_install_names)

if(CLIP_SHOT_RELEASE_STATIC)
    get_filename_component(_clip_static_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    set(_clip_static_qt_licenses "${_clip_static_qt_prefix}/share/clip-apps/qt-licenses")
    if(NOT IS_DIRECTORY "${_clip_static_qt_licenses}")
        message(FATAL_ERROR
            "The audited static Qt license bundle is missing: ${_clip_static_qt_licenses}")
    endif()
    install(DIRECTORY "${_clip_static_qt_licenses}/"
        DESTINATION "clip_shot.app/Contents/Resources/clip-shot/licenses/third-party/qt"
        COMPONENT ClipShot)
    install(DIRECTORY "${CLIP_FFMPEG_ROOT}/share/"
        DESTINATION "clip_shot.app/Contents/Resources/clip-shot/licenses/third-party/vcpkg"
        COMPONENT ClipShot FILES_MATCHING PATTERN "copyright")
endif()

# Dynamic development builds load ONNX Runtime with dlopen. Production builds
# link it into the helper and therefore stage no runtime dylib.
if(NOT CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
    add_custom_command(TARGET clip_shot POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:onnxruntime::onnxruntime>"
            "$<TARGET_FILE_DIR:clip_shot>/libonnxruntime.dylib"
        VERBATIM)
    install(FILES "$<TARGET_FILE:onnxruntime::onnxruntime>"
        DESTINATION "clip_shot.app/Contents/MacOS" RENAME libonnxruntime.dylib
        COMPONENT ClipShot)
endif()

# Keep the crash collector inside the app, beside the main executable and worker.
set(_clip_handler_zlib "")
if(NOT CLIP_SHOT_RELEASE_STATIC)
    set(_clip_handler_zlib "$<TARGET_FILE:ZLIB::ZLIB>")
endif()
add_custom_command(TARGET clip_shot POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DCLIP_HANDLER_SOURCE=${CLIP_CRASHPAD_HANDLER}"
        "-DCLIP_HANDLER_DIRECTORY=$<TARGET_FILE_DIR:clip_shot>"
        "-DCLIP_HANDLER_ZLIB=${_clip_handler_zlib}"
        -P "${CMAKE_CURRENT_LIST_DIR}/StageClipShotCrashHandler.cmake"
    VERBATIM)
install(PROGRAMS "${CLIP_CRASHPAD_HANDLER}"
    DESTINATION "clip_shot.app/Contents/MacOS" COMPONENT ClipShot)

# Archive UUID-matched symbols outside the app bundle for offline symbolication.
find_program(CLIP_DSYMUTIL dsymutil REQUIRED)
if(TARGET clip_ocr_process)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/GenerateClipShotDiagnosticsSymbols.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/GenerateClipShotDiagnosticsSymbols.cmake.in" @ONLY)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/GenerateClipShotDiagnosticsSymbols-$<CONFIG>.cmake"
        INPUT "${CMAKE_CURRENT_BINARY_DIR}/GenerateClipShotDiagnosticsSymbols.cmake.in")
    add_custom_target(clip-shot-diagnostics-symbols
        COMMAND "${CMAKE_COMMAND}" -P
            "${CMAKE_CURRENT_BINARY_DIR}/GenerateClipShotDiagnosticsSymbols-$<CONFIG>.cmake"
        DEPENDS clip_shot clip_ocr_process
        VERBATIM)
endif()
target_compile_options(clip_shot PRIVATE -g)
foreach(_clip_diagnostics_target clip_shot_diagnostics clip_shot_crash_bridge clip_ocr_diagnostics_bridge)
    if(TARGET ${_clip_diagnostics_target})
        target_compile_options(${_clip_diagnostics_target} PRIVATE -g)
    endif()
endforeach()

set(CLIP_MACOS_OCR_ASSETS_ENABLED OFF)
if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
    set(CLIP_MACOS_OCR_ARCH x64)
else()
    set(CLIP_MACOS_OCR_ARCH arm64)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CLIP_MACOS_OCR_TOOL "${CMAKE_CURRENT_LIST_DIR}/../scripts/clip-shot-macos-ocr.py")
set(CLIP_MACOS_OCR_MANIFEST "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/clip-shot-ocr-asset-manifest.json")
if(CMAKE_OSX_ARCHITECTURES MATCHES "^(arm64|x86_64)$" AND TARGET clip_ocr_process)
    set(CLIP_MACOS_OCR_ASSETS_ENABLED ON)
    set(_clip_macos_ocr_runtime_arguments)
    if(CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
        list(APPEND _clip_macos_ocr_runtime_arguments --static-runtime)
    else()
        list(APPEND _clip_macos_ocr_runtime_arguments
            --library "$<TARGET_FILE:onnxruntime::onnxruntime>")
    endif()
    foreach(_host IN ITEMS clip_shot clip-shot-ocr-recognition-service-tests)
        if(TARGET ${_host})
            # Always check the content hashes: Cargo can rebuild the worker without
            # relinking its Qt host. A host POST_BUILD hook would miss that update.
            add_custom_target(${_host}-ocr-assets
                COMMAND "${Python3_EXECUTABLE}" "${CLIP_MACOS_OCR_TOOL}" stage
                    --arch "${CLIP_MACOS_OCR_ARCH}"
                    --manifest "${CLIP_MACOS_OCR_MANIFEST}"
                    --runtime-dir "$<TARGET_FILE_DIR:${_host}>"
                    --worker "$<TARGET_FILE:clip_ocr_process>"
                    ${_clip_macos_ocr_runtime_arguments}
                DEPENDS clip_ocr_process onnxruntime::onnxruntime
                    "${CLIP_MACOS_OCR_TOOL}" "${CLIP_MACOS_OCR_MANIFEST}"
                VERBATIM)
            add_dependencies(${_host} ${_host}-ocr-assets)
        endif()
    endforeach()
    install(DIRECTORY "$<TARGET_FILE_DIR:clip_shot>/assets/ocr"
        DESTINATION "clip_shot.app/Contents/MacOS/assets" COMPONENT ClipShot)
endif()

if(NOT CLIP_SHOT_QT_STATIC)
    include("${CMAKE_CURRENT_LIST_DIR}/ClipQt.cmake")
    clip_qt_bin_directory(_clip_qt_bin)
    if(NOT TARGET Qt6::QOffscreenIntegrationPlugin)
        message(FATAL_ERROR "The selected Qt kit does not provide the offscreen platform plugin")
    endif()
    install(FILES "$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>"
        DESTINATION "clip_shot.app/Contents/PlugIns/platforms" COMPONENT ClipShot)
    find_program(CLIP_MACDEPLOYQT NAMES macdeployqt HINTS "${_clip_qt_bin}"
        NO_DEFAULT_PATH REQUIRED)
else()
    set(CLIP_MACDEPLOYQT "")
endif()
set(CLIP_MACOS_CODESIGN_IDENTITY "-" CACHE STRING
    "Code-signing certificate name or SHA-1; AUTO provisions the local development identity; '-' uses ad-hoc signing")
if(CLIP_MACOS_CODESIGN_IDENTITY STREQUAL "")
    message(FATAL_ERROR "CLIP_MACOS_CODESIGN_IDENTITY must be AUTO, a certificate identity, or '-'")
elseif(CLIP_MACOS_CODESIGN_IDENTITY STREQUAL "AUTO")
    execute_process(
        COMMAND "${CMAKE_CURRENT_LIST_DIR}/../scripts/ensure-macos-codesign-identity.sh"
        RESULT_VARIABLE _clip_codesign_result
        OUTPUT_VARIABLE CLIP_MACOS_CODESIGN_IDENTITY
        ERROR_VARIABLE _clip_codesign_error
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(LENGTH "${CLIP_MACOS_CODESIGN_IDENTITY}" _clip_codesign_length)
    if(NOT _clip_codesign_result EQUAL 0 OR
       NOT CLIP_MACOS_CODESIGN_IDENTITY MATCHES "^[0-9A-Fa-f]+$" OR
       NOT _clip_codesign_length EQUAL 40)
        message(FATAL_ERROR
            "Could not provision the local macOS signing identity: ${_clip_codesign_error}")
    endif()
endif()
if(NOT CLIP_MACOS_CODESIGN_IDENTITY STREQUAL "-")
    # Ninja otherwise leaves the raw executable linker/ad-hoc signed, making its
    # TCC designated requirement change on every relink during IDE debugging.
    # Sign a standalone copy because codesign treats the in-bundle executable as
    # the whole bundle, while the raw development layout keeps data under MacOS.
    add_custom_command(TARGET clip_shot POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:clip_shot>"
            "$<TARGET_FILE:clip_shot>.clip-signing"
        COMMAND /usr/bin/codesign --force --sign "${CLIP_MACOS_CODESIGN_IDENTITY}"
            --identifier com.clipshot.clip_shot "$<TARGET_FILE:clip_shot>.clip-signing"
        COMMAND /usr/bin/codesign --verify --strict "$<TARGET_FILE:clip_shot>.clip-signing"
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:clip_shot>.clip-signing"
            "$<TARGET_FILE:clip_shot>"
        COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_FILE:clip_shot>.clip-signing"
        VERBATIM)
endif()
# Full includes its default model; Mini supplies its own runtime-only policy.
set(CLIP_MACOS_OCR_RUNTIME_ONLY OFF)
configure_file("${CMAKE_CURRENT_LIST_DIR}/DeployClipShotMacOS.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DeployClipShotMacOS.cmake" @ONLY)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/DeployClipShotMacOS.cmake" COMPONENT ClipShot)
