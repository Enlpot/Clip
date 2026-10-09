if(NOT TARGET clip_shot)
    message(FATAL_ERROR "CLIP_APPS_PACKAGE_CLIP_SHOT requires CLIP_APPS_BUILD_CLIP_SHOT=ON")
endif()
set(CPACK_PACKAGE_NAME "clip-shot")
set(CPACK_PACKAGE_VENDOR "${CLIP_SHOT_VENDOR}")
set(CPACK_PACKAGE_VERSION "${CLIP_SHOT_VERSION_NUMERIC}")
set(CPACK_PACKAGE_FILE_NAME "clip-shot-${CLIP_SHOT_VERSION}-macos-${CMAKE_OSX_ARCHITECTURES}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_GENERATOR DragNDrop)
set(CPACK_DMG_VOLUME_NAME "Clip")
set(CPACK_DMG_FORMAT UDZO)
set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK OFF)
set(_clip_dmg_assets "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/macos")
set(_clip_dmg_wordmark
    "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/src/presentation/components/icons/resources/clip-shot-logo.svg")
set(_clip_dmg_background_svg "${CMAKE_CURRENT_BINARY_DIR}/macos/dmg-background.svg")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/macos/dmg-background.png")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/macos")
# Keep the installer heading identical to the in-app title bar without
# maintaining a second copy of the wordmark paths. The DMG has a fixed light
# background, so resolve the title bar's theme-controlled text color here.
file(READ "${_clip_dmg_wordmark}" CLIP_DMG_WORDMARK)
string(REGEX REPLACE "^<svg[^>]*>" "" CLIP_DMG_WORDMARK "${CLIP_DMG_WORDMARK}")
string(REGEX REPLACE "</svg>[ \t\r\n]*$" "" CLIP_DMG_WORDMARK "${CLIP_DMG_WORDMARK}")
string(REPLACE "currentColor" "#152c4a" CLIP_DMG_WORDMARK "${CLIP_DMG_WORDMARK}")
configure_file("${_clip_dmg_assets}/dmg-background.svg" "${_clip_dmg_background_svg}" @ONLY)
execute_process(COMMAND /usr/bin/sips -s format png
    "${_clip_dmg_background_svg}" --out "${CPACK_DMG_BACKGROUND_IMAGE}"
    OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${_clip_dmg_assets}/dmg-layout.applescript")
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/PrepareClipShotMacOSDmg.cmake")
set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/SignClipShotMacOSDmg.cmake")
# Do not ship SDK headers, static archives or other projects' install rules.
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${CMAKE_PROJECT_NAME};ClipShot;/")
include(CPack)
