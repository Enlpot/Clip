# CPack runs the old uninstaller in .onInit, before its public pre-install hook.
# Generate a minimally extended copy of the installed CPack template so the
# previous version is protected even when its own uninstaller has no guard.
file(READ "${CMAKE_ROOT}/Modules/Internal/CPack/NSIS.template.in" _clip_nsis_template)
# Fail visibly on CPack template drift instead of shipping partially translated UI.
function(clip_shot_nsis_replace original replacement)
    string(FIND "${_clip_nsis_template}" "${original}" _clip_nsis_position)
    if(_clip_nsis_position EQUAL -1)
        message(FATAL_ERROR "The CPack NSIS template changed; missing installer hook: ${original}")
    endif()
    string(REPLACE "${original}" "${replacement}" _clip_nsis_template "${_clip_nsis_template}")
    set(_clip_nsis_template "${_clip_nsis_template}" PARENT_SCOPE)
endfunction()
set(_clip_nsis_init "Function .onInit\n")
string(FIND "${_clip_nsis_template}" "${_clip_nsis_init}" _clip_nsis_init_position)
if(_clip_nsis_init_position EQUAL -1)
    message(FATAL_ERROR "The CPack NSIS .onInit hook changed; review the running-app guard.")
endif()
string(REPLACE "${_clip_nsis_init}" [=[Function .onInit
  !insertmacro ClipShotLanguageContext
  !insertmacro ClipShotCheckArchitecture
  SetRegView 64
  !insertmacro ClipShotRestoreInstallDirectory "Software\@CPACK_PACKAGE_VENDOR@\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@" "@CPACK_NSIS_INSTALL_ROOT@\@CPACK_PACKAGE_INSTALL_DIRECTORY@"
  !insertmacro MUI_LANGDLL_DISPLAY
  Push $0
  ReadRegStr $0 HKLM "Software\@CPACK_PACKAGE_VENDOR@\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@" ""
  StrCmp $0 "" +5
    Push "$0\bin\@CLIP_SHOT_EXECUTABLE_NAME@.exe"
    Call ClipShotEnsureMainAppClosed
    Push "$0\bin\crashpad_handler.exe"
    Call ClipShotEnsureAppClosed
  ReadRegStr $0 HKCU "Software\@CPACK_PACKAGE_VENDOR@\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@" ""
  StrCmp $0 "" +5
    Push "$0\bin\@CLIP_SHOT_EXECUTABLE_NAME@.exe"
    Call ClipShotEnsureMainAppClosed
    Push "$0\bin\crashpad_handler.exe"
    Call ClipShotEnsureAppClosed
  Pop $0
]=] _clip_nsis_template "${_clip_nsis_template}")
# CPack does not carry project variables into its template configuration.
string(REPLACE "@CLIP_SHOT_EXECUTABLE_NAME@" "${CLIP_SHOT_EXECUTABLE_NAME}"
    _clip_nsis_template "${_clip_nsis_template}")
# Restrict the wizard to languages with complete Clip catalogs.
string(REGEX MATCHALL "!insertmacro MUI_LANGUAGE \"[A-Za-z]+\"[^\n]*" _clip_nsis_languages
    "${_clip_nsis_template}")
if(NOT _clip_nsis_languages)
    message(FATAL_ERROR "The CPack NSIS language declarations changed.")
endif()
foreach(_clip_nsis_language IN LISTS _clip_nsis_languages)
    string(REPLACE "${_clip_nsis_language}" "" _clip_nsis_template "${_clip_nsis_template}")
endforeach()
clip_shot_nsis_replace(";Languages" ";Languages\n  !insertmacro ClipShotInstallerLanguages")
clip_shot_nsis_replace("!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS"
    "!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS\n  !insertmacro MUI_RESERVEFILE_LANGDLL")
string(FIND "${_clip_nsis_template}" "Function un.onInit\n" _clip_nsis_uninit_start)
if(_clip_nsis_uninit_start EQUAL -1)
    message(FATAL_ERROR "The CPack NSIS un.onInit hook changed.")
endif()
string(SUBSTRING "${_clip_nsis_template}" ${_clip_nsis_uninit_start} -1 _clip_nsis_uninit)
string(FIND "${_clip_nsis_uninit}" "FunctionEnd" _clip_nsis_uninit_end)
if(_clip_nsis_uninit_end EQUAL -1)
    message(FATAL_ERROR "The CPack NSIS un.onInit terminator changed.")
endif()
math(EXPR _clip_nsis_uninit_length "${_clip_nsis_uninit_end} + 11")
string(SUBSTRING "${_clip_nsis_uninit}" 0 ${_clip_nsis_uninit_length} _clip_nsis_uninit)
string(REPLACE "Function un.onInit\n" "Function un.onInit\n  SetRegView 64\n"
    _clip_nsis_localized_uninit "${_clip_nsis_uninit}")
string(REPLACE "FunctionEnd" "  !insertmacro MUI_UNGETLANGUAGE\nFunctionEnd"
    _clip_nsis_localized_uninit "${_clip_nsis_localized_uninit}")
clip_shot_nsis_replace("${_clip_nsis_uninit}" "${_clip_nsis_localized_uninit}")
clip_shot_nsis_replace([=["$1 is already installed. $\n$\nDo you want to uninstall the old version before installing the new one?"]=]
    [=["$(ClipShotUpgradePrompt)"]=])
clip_shot_nsis_replace([=["Uninstall failed."]=] [=["$(ClipShotUninstallFailed)"]=])
clip_shot_nsis_replace([=["Install Options" "Choose options for installing @CPACK_NSIS_PACKAGE_NAME@"]=]
    [=["$(ClipShotOptionsTitle)" "$(ClipShotOptionsDescription)"]=])
clip_shot_nsis_replace("Function InstallOptionsPage\n" [=[Function InstallOptionsPage
  !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 1" "Text" "$(ClipShotPathDescription)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 2" "Text" "$(ClipShotPathNone)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 3" "Text" "$(ClipShotPathAll)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 4" "Text" "$(ClipShotPathCurrent)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 5" "Text" "$(ClipShotDesktopIcon)"
]=])
# Extract options even when PATH changes are disabled. Initialize the default only
# once, so returning to the page does not overwrite an unchecked shortcut choice.
clip_shot_nsis_replace([=[  StrCmp "@CPACK_NSIS_MODIFY_PATH@" "ON" 0 noOptionsPage]=] "")
clip_shot_nsis_replace([=[    !insertmacro MUI_INSTALLOPTIONS_EXTRACT "NSIS.InstallOptions.ini"]=]
    [=[    !insertmacro MUI_INSTALLOPTIONS_EXTRACT "NSIS.InstallOptions.ini"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 5" "State" "1"]=])
if(NOT CPACK_NSIS_MODIFY_PATH)
    # Keep CPack's state fields for registry bookkeeping, but display only the
    # shortcut checkbox. Fields beyond NumFields remain readable in the INI.
    clip_shot_nsis_replace([=["Field 5"]=] [=["Field 1"]=])
    clip_shot_nsis_replace([=[    !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 1" "State" "1"]=]
        [=[    !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Settings" "NumFields" "1"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 1" "Type" "CheckBox"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 1" "Bottom" "12"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "NSIS.InstallOptions.ini" "Field 1" "State" "1"]=])
endif()
clip_shot_nsis_replace([=["Set install registry entry: '$1' to '$0'"]=]
    [=["$(ClipShotRegistryEntry)"]=])
clip_shot_nsis_replace([=["Selected environment for all users"]=] [=["$(ClipShotEnvironmentAll)"]=])
clip_shot_nsis_replace([=["Selected environment for current user only."]=]
    [=["$(ClipShotEnvironmentCurrent)"]=])
clip_shot_nsis_replace([=["Download failed: $1"]=] [=["$(ClipShotDownloadFailed)"]=])
clip_shot_nsis_replace([=[$STARTMENU_FOLDER\Uninstall.lnk]=]
    [=[$STARTMENU_FOLDER\$(ClipShotUninstallShortcut).lnk]=])
clip_shot_nsis_replace([=[CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\$(ClipShotUninstallShortcut).lnk"]=]
    [=[!insertmacro ClipShotDeleteUninstallShortcuts "$SMPROGRAMS\$STARTMENU_FOLDER"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\$(ClipShotUninstallShortcut).lnk"]=])
clip_shot_nsis_replace([=[Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"]=]
    [=[!insertmacro ClipShotDeleteUninstallShortcuts "$SMPROGRAMS\$MUI_TEMP"]=])
# Carry upgrade intent through the old uninstaller; final uninstall removes startup registrations.
clip_shot_nsis_replace([=[  Push "UninstallString"
  Push "$\"$INSTDIR\@CPACK_NSIS_UNINSTALL_NAME@.exe$\""
  Call ConditionalAddToRegistry]=] [=[  Push "UninstallString"
  Push "$\"$INSTDIR\@CPACK_NSIS_UNINSTALL_NAME@.exe$\""
  Call ConditionalAddToRegistry
  Push "QuietUninstallString"
  Push "$\"$INSTDIR\@CPACK_NSIS_UNINSTALL_NAME@.exe$\" /S"
  Call ConditionalAddToRegistry]=])
clip_shot_nsis_replace([=[ExecWait '"$0" /S _?=$3']=]
    [=[StrCpy $ClipShotPreviousRoot $3
  Push $3
  Call ClipShotPrepareUpgradeHelper
  ClearErrors
  ExecWait '"$0" /S /CLIPUPGRADE _?=$3' $2
  IfErrors clipPreviousUninstallFailed
  StrCmp $2 0 clipPreviousUninstallDone
clipPreviousUninstallFailed:
  Call ClipShotRestoreUpgradeHelper
  Goto uninst_failed
clipPreviousUninstallDone:
  StrCpy $ClipShotUpgradeHelperRoot ""
  StrCpy $ClipShotUpgradeHelperBackup ""
  ClearErrors]=])
clip_shot_nsis_replace("@CPACK_NSIS_INSTALLER_MUI_FINISHPAGE_RUN_CODE@" [=[
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION ClipShotLaunchDesktop
]=])
# Future self-updates can add files that this uninstaller did not know at build time.
# The shared ownership-cleanup sequence treats missing components of a partial
# installation like any other missing file, so uninstallation still completes.
clip_shot_nsis_replace("@CPACK_NSIS_DELETE_FILES@" [=[
  !insertmacro ClipShotUninstallOwnedCleanup
@CPACK_NSIS_DELETE_FILES@
]=])
if(NOT CLIP_SHOT_NSIS_DIRECTORY)
    set(CLIP_SHOT_NSIS_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/clip-shot-nsis")
endif()
file(MAKE_DIRECTORY "${CLIP_SHOT_NSIS_DIRECTORY}")
file(WRITE "${CLIP_SHOT_NSIS_DIRECTORY}/NSIS.template.in" "${_clip_nsis_template}")
list(PREPEND CMAKE_MODULE_PATH "${CLIP_SHOT_NSIS_DIRECTORY}")
set(_clip_nsis_guard "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/RunningApplication.nsh")
cmake_path(NATIVE_PATH _clip_nsis_guard NORMALIZE _clip_nsis_guard_native)
set(_clip_nsis_launch "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/InstallerLaunch.nsh")
cmake_path(NATIVE_PATH _clip_nsis_launch NORMALIZE _clip_nsis_launch_native)
set(_clip_nsis_owned_cleanup "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/OwnedCleanup.nsh")
cmake_path(NATIVE_PATH _clip_nsis_owned_cleanup NORMALIZE _clip_nsis_owned_cleanup_native)
set(_clip_nsis_upgrade_helper "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/UpgradeHelper.nsh")
cmake_path(NATIVE_PATH _clip_nsis_upgrade_helper NORMALIZE _clip_nsis_upgrade_helper_native)
set(_clip_nsis_localization "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/InstallerLanguages.nsh")
cmake_path(NATIVE_PATH _clip_nsis_localization NORMALIZE _clip_nsis_localization_native)
set(_clip_nsis_directory "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/InstallDirectory.nsh")
cmake_path(NATIVE_PATH _clip_nsis_directory NORMALIZE _clip_nsis_directory_native)
set(_clip_nsis_architecture "${CMAKE_CURRENT_LIST_DIR}/../clip_shot/packaging/InstallerArchitecture.nsh")
cmake_path(NATIVE_PATH _clip_nsis_architecture NORMALIZE _clip_nsis_architecture_native)
if(NOT CLIP_WINDOWS_ARCHITECTURE)
    set(CLIP_WINDOWS_ARCHITECTURE x64)
endif()
string(APPEND CPACK_NSIS_DEFINES "\nVar ClipShotPreviousRoot\nUnicode true\n!include \"${_clip_nsis_guard_native}\"\n"
    "!include \"${_clip_nsis_launch_native}\"\n"
    "!include \"${_clip_nsis_owned_cleanup_native}\"\n"
    "!include \"${_clip_nsis_upgrade_helper_native}\"\n"
    "!include \"${_clip_nsis_directory_native}\"\n"
    "!include \"${_clip_nsis_localization_native}\"\n"
    "!define CLIP_SHOT_INSTALLER_ARCHITECTURE \"${CLIP_WINDOWS_ARCHITECTURE}\"\n"
    "!include \"${_clip_nsis_architecture_native}\"\n"
    "!define MUI_LANGDLL_REGISTRY_ROOT SHCTX\n"
    "!define MUI_LANGDLL_REGISTRY_KEY \"Software\\${CPACK_PACKAGE_VENDOR}\\${CPACK_PACKAGE_INSTALL_REGISTRY_KEY}\"\n"
    "!define MUI_LANGDLL_REGISTRY_VALUENAME \"InstallerLanguage\"\n")
# MUI normally saves on the visible progress page; also persist silent installs.
string(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "\n!insertmacro MUI_LANGDLL_SAVELANGUAGE\n")
set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS
    "Push \"$INSTDIR\\bin\\${CLIP_SHOT_EXECUTABLE_NAME}.exe\"\nCall ClipShotEnsureMainAppClosed\nPush \"$INSTDIR\\bin\\crashpad_handler.exe\"\nCall ClipShotEnsureAppClosed")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
    "Push \"$INSTDIR\\bin\\${CLIP_SHOT_EXECUTABLE_NAME}.exe\"\nCall un.ClipShotEnsureAppClosed\nPush \"$INSTDIR\\bin\\crashpad_handler.exe\"\nCall un.ClipShotEnsureAppClosed")

string(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS [=[
  StrCmp $ClipShotPreviousRoot "" clipStartupMigrated
  StrCmp $ClipShotPreviousRoot $INSTDIR clipStartupMigrated
  ClearErrors
  ExecWait '"$INSTDIR\bin\clip-shot-updater.exe" --migrate-startup --previous "$ClipShotPreviousRoot" --target "$INSTDIR"' $0
  IfErrors clipStartupMigrationFailed
  StrCmp $0 0 clipStartupMigrated
clipStartupMigrationFailed:
  MessageBox MB_OK|MB_ICONSTOP "$(ClipShotStartupCleanupFailed)" /SD IDOK
  SetErrorLevel 13
  Abort
clipStartupMigrated:
]=])
