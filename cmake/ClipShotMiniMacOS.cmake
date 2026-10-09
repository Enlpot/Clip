set_target_properties(clip_shot_mini PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info-mini.plist.in"
    MACOSX_BUNDLE_ICON_FILE clip-shot.icns
    INSTALL_RPATH "@executable_path/../Frameworks" INSTALL_RPATH_USE_LINK_PATH TRUE)
target_sources(clip_shot_mini PRIVATE "${_clip_macos_icon}")
foreach(_language IN ITEMS en zh-Hans zh-Hant)
    set(_strings "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/mini/${_language}.lproj/InfoPlist.strings")
    set_source_files_properties("${_strings}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${_language}.lproj")
    target_sources(clip_shot_mini PRIVATE "${_strings}")
endforeach()
target_link_options(clip_shot_mini PRIVATE -Wl,-headerpad_max_install_names)
if(CLIP_SHOT_RELEASE_STATIC AND CMAKE_BUILD_TYPE STREQUAL "Release")
    # The shared OCR worker's dSYM is collected by the Full symbol rule.
    file(READ "${CMAKE_CURRENT_LIST_DIR}/GenerateClipShotDiagnosticsSymbols.cmake.in" _mini_symbols)
    string(REGEX REPLACE "# Cargo.*$" "" _mini_symbols "${_mini_symbols}")
    string(REPLACE "clip_shot>" "clip_shot_mini>" _mini_symbols "${_mini_symbols}")
    string(REPLACE "clip_shot.dSYM" "clip_shot_mini.dSYM" _mini_symbols "${_mini_symbols}")
    string(CONFIGURE "${_mini_symbols}" _mini_symbols @ONLY)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/GenerateClipShotMiniDiagnosticsSymbols-$<CONFIG>.cmake"
        CONTENT "${_mini_symbols}")
    add_custom_command(TARGET clip_shot_mini POST_BUILD COMMAND "${CMAKE_COMMAND}" -P
        "${CMAKE_CURRENT_BINARY_DIR}/GenerateClipShotMiniDiagnosticsSymbols-$<CONFIG>.cmake"
        VERBATIM)
endif()
if(NOT CLIP_MACOS_CODESIGN_IDENTITY STREQUAL "-")
    add_custom_command(TARGET clip_shot_mini POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:clip_shot_mini>"
            "$<TARGET_FILE:clip_shot_mini>.clip-signing"
        COMMAND /usr/bin/codesign --force --sign "${CLIP_MACOS_CODESIGN_IDENTITY}"
            --identifier com.clipshot.clip_shot_mini "$<TARGET_FILE:clip_shot_mini>.clip-signing"
        COMMAND /usr/bin/codesign --verify --strict "$<TARGET_FILE:clip_shot_mini>.clip-signing"
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:clip_shot_mini>.clip-signing"
            "$<TARGET_FILE:clip_shot_mini>"
        COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_FILE:clip_shot_mini>.clip-signing"
        VERBATIM)
endif()
add_custom_target(clip_shot_mini-ocr-assets
    COMMAND "${Python3_EXECUTABLE}" "${CLIP_MACOS_OCR_TOOL}" stage
        --arch "${CLIP_MACOS_OCR_ARCH}"
        --manifest "${CLIP_MACOS_OCR_MANIFEST}"
        --runtime-dir "$<TARGET_FILE_DIR:clip_shot_mini>"
        --worker "$<TARGET_FILE:clip_ocr_process>" --runtime-only ${_clip_macos_ocr_runtime_arguments}
    DEPENDS clip_ocr_process onnxruntime::onnxruntime VERBATIM)
add_dependencies(clip_shot_mini clip_shot_mini-ocr-assets)
install(PROGRAMS "$<TARGET_FILE:clip_ocr_process>" DESTINATION "${_mini_bindir}" COMPONENT ClipShotMini)
install(DIRECTORY "$<TARGET_FILE_DIR:clip_shot_mini>/assets/ocr"
    DESTINATION "${_mini_bindir}/assets" COMPONENT ClipShotMini PATTERN models EXCLUDE)
if(NOT CLIP_SHOT_OCR_STATIC_ONNXRUNTIME)
    install(FILES "$<TARGET_FILE:onnxruntime::onnxruntime>" DESTINATION "${_mini_bindir}"
        RENAME libonnxruntime.dylib COMPONENT ClipShotMini)
endif()
if(NOT CLIP_SHOT_QT_STATIC)
    install(FILES "${CLIP_QT_OFFSCREEN_PLUGIN}"
        DESTINATION "clip_shot_mini.app/Contents/PlugIns/platforms" COMPONENT ClipShotMini)
endif()
if(CLIP_SHOT_RELEASE_STATIC)
    install(DIRECTORY "${_clip_static_qt_licenses}/"
        DESTINATION "${_mini_datadir}/clip-shot-mini/licenses/third-party/qt" COMPONENT ClipShotMini)
    install(DIRECTORY "${CLIP_FFMPEG_ROOT}/share/"
        DESTINATION "${_mini_datadir}/clip-shot-mini/licenses/third-party/vcpkg"
        COMPONENT ClipShotMini FILES_MATCHING PATTERN copyright)
endif()
file(READ "${CMAKE_CURRENT_LIST_DIR}/DeployClipShotMacOS.cmake.in" _mini_deploy)
string(REPLACE "@CLIP_MACOS_OCR_RUNTIME_ONLY@" "ON" _mini_deploy "${_mini_deploy}")
string(REPLACE "clip_shot.app" "clip_shot_mini.app" _mini_deploy "${_mini_deploy}")
string(REPLACE "clip-shot-updater" "clip-shot-mini-updater" _mini_deploy "${_mini_deploy}")
string(REPLACE "macos-ocr-verification.json" "macos-mini-ocr-verification.json" _mini_deploy "${_mini_deploy}")
string(APPEND _mini_deploy [==[
set(CLIP_SHOT_MINI_APP "${_app}")
set(CLIP_SHOT_MINI_STATIC @CLIP_SHOT_RELEASE_STATIC@)
include("@CMAKE_CURRENT_LIST_DIR@/AssertClipShotMiniMacOSPayload.cmake")
]==])
string(CONFIGURE "${_mini_deploy}" _mini_deploy @ONLY)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/DeployClipShotMiniMacOS.cmake" "${_mini_deploy}")
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/DeployClipShotMiniMacOS.cmake" COMPONENT ClipShotMini)
