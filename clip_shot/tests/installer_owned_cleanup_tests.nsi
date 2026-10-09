Unicode true
Name "Clip installer owned cleanup tests"
OutFile "${OUTPUT}"
RequestExecutionLevel user
!include "LogicLib.nsh"
!include "${GUARD}"
!include "${OWNED_CLEANUP}"

Section
  StrCpy $INSTDIR "${DESTINATION}"
  SetOutPath "$INSTDIR\bin"
  File /oname=clip_shot.exe "${PAYLOAD}"
  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
  !insertmacro ClipShotUninstallOwnedCleanup
  Delete "$INSTDIR\bin\clip_shot.exe"
  Delete "$INSTDIR\bin\clip-shot-updater.exe"
  Delete "$INSTDIR\clip-shot-installation.json"
  RMDir "$INSTDIR\bin"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"
SectionEnd
