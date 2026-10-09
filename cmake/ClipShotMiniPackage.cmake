if(NOT TARGET clip_shot_mini)
    return()
endif()

# Use CPack's normal encoder to create a second configuration. The standard
# CPackConfig.cmake remains the historical full product configuration.
function(clip_shot_configure_mini_package)
    # include(CPack) leaves source-package variables in the caller's scope.
    # Start from the saved binary configuration, including its generator and
    # install inputs, so Mini cannot accidentally package the source tree.
    get_cmake_property(_clip_cpack_variables VARIABLES)
    foreach(_variable IN LISTS _clip_cpack_variables)
        if(_variable MATCHES "^CPACK_")
            unset(${_variable})
        endif()
    endforeach()
    include("${CMAKE_BINARY_DIR}/CPackConfig.cmake")
    set(CPACK_PACKAGE_NAME clip-shot-mini)
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Clip Mini screenshot utility")
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/clip_shot/packaging/README-mini.txt")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/clip_shot/packaging/README-mini.txt")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ClipShotMini)
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ClipShotMini)
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${CMAKE_PROJECT_NAME};ClipShotMini;/")
    set(CPACK_COMPONENTS_ALL ClipShotMini)
    if(APPLE)
        set(CPACK_PACKAGE_FILE_NAME "clip-shot-mini-${CLIP_SHOT_VERSION}-macos-arm64")
        set(CPACK_DMG_VOLUME_NAME "Clip Mini")
        set(CPACK_CLIP_SHOT_BUNDLE_NAME clip_shot_mini)
        set(CPACK_CLIP_SHOT_PRODUCT_NAME "Clip Mini")
        set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${_clip_dmg_assets}/dmg-mini-layout.applescript")
        set(_background "${CMAKE_BINARY_DIR}/macos/dmg-mini-background.svg")
        set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_BINARY_DIR}/macos/dmg-mini-background.png")
        configure_file("${_clip_dmg_assets}/dmg-mini-background.svg" "${_background}" @ONLY)
        execute_process(COMMAND /usr/bin/sips -s format png "${_background}"
            --out "${CPACK_DMG_BACKGROUND_IMAGE}" OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
    else()
        set(CLIP_SHOT_PRODUCT_NAME "Clip Mini")
        set(CLIP_SHOT_EXECUTABLE_NAME clip_shot_mini)
        if(NOT CLIP_WINDOWS_PLATFORM)
            if(NOT CLIP_WINDOWS_ARCHITECTURE)
                set(CLIP_WINDOWS_ARCHITECTURE x64)
            endif()
            set(CLIP_WINDOWS_PLATFORM "windows-${CLIP_WINDOWS_ARCHITECTURE}")
        endif()
        set(CPACK_PACKAGE_FILE_NAME "clip-shot-mini-${CLIP_SHOT_VERSION}-${CLIP_WINDOWS_PLATFORM}")
        set(CPACK_PACKAGE_EXECUTABLES clip_shot_mini "Clip Mini")
        set(CPACK_CREATE_DESKTOP_LINKS clip_shot_mini)
        set(CPACK_NSIS_DISPLAY_NAME "Clip Mini")
        set(CPACK_NSIS_PACKAGE_NAME "Clip Mini")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\clip_shot_mini.exe")
        set(CPACK_NSIS_MUI_FINISHPAGE_RUN clip_shot_mini.exe)
        set(CPACK_NSIS_UNINSTALL_NAME "Uninstall Clip Mini")
        set(CPACK_NSIS_BRANDING_TEXT "Clip Mini ${CLIP_SHOT_VERSION}")
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "")
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")
        string(CONCAT CPACK_NSIS_DEFINES
            "VIProductVersion \"${CLIP_SHOT_VERSION_NUMERIC}.0\"\n"
            "VIAddVersionKey /LANG=1033 \"CompanyName\" \"${CLIP_SHOT_VENDOR}\"\n"
            "VIAddVersionKey /LANG=1033 \"FileDescription\" \"Clip Mini installer\"\n"
            "VIAddVersionKey /LANG=1033 \"FileVersion\" \"${CLIP_SHOT_VERSION_NUMERIC}.0\"\n"
            "VIAddVersionKey /LANG=1033 \"InternalName\" \"clip-shot-mini-installer\"\n"
            "VIAddVersionKey /LANG=1033 \"LegalCopyright\" \"${CLIP_SHOT_COPYRIGHT}\"\n"
            "VIAddVersionKey /LANG=1033 \"OriginalFilename\" \"${CPACK_PACKAGE_FILE_NAME}.exe\"\n"
            "VIAddVersionKey /LANG=1033 \"ProductName\" \"Clip Mini\"\n"
            "VIAddVersionKey /LANG=1033 \"ProductVersion\" \"${CLIP_SHOT_VERSION}\"\n"
            "!define CLIP_SHOT_INSTALLER_PRODUCT_NAME \"Clip Mini\"\n"
            "!define CLIP_SHOT_INSTALLER_EXECUTABLE \"clip_shot_mini\"\n"
            "!define CLIP_SHOT_INSTALLER_UPDATER \"clip-shot-mini-updater\"\n")
        include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ClipShotPackageCompression.cmake")
        clip_shot_apply_nsis_compression()
        set(CLIP_SHOT_NSIS_DIRECTORY "${CMAKE_BINARY_DIR}/clip-shot-mini-nsis")
        include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ClipShotInstaller.cmake")
        set(CPACK_MODULE_PATH "${CMAKE_MODULE_PATH}")
        foreach(_var IN ITEMS CPACK_NSIS_EXTRA_INSTALL_COMMANDS CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS)
            string(REPLACE "clip-shot-updater.exe" "clip-shot-mini-updater.exe" ${_var} "${${_var}}")
        endforeach()
    endif()
    cpack_encode_variables()
    configure_file("${CMAKE_ROOT}/Templates/CPackConfig.cmake.in"
        "${CMAKE_BINARY_DIR}/CPackClipShotMiniConfig.cmake" @ONLY)
endfunction()
clip_shot_configure_mini_package()
