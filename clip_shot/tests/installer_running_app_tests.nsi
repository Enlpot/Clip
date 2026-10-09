Unicode true
Name "Clip installer running application tests"
OutFile "${OUTPUT}"
RequestExecutionLevel user
!include "LogicLib.nsh"
!ifdef ANSWER
!macro ClipShotConfirmClose Prefix
  StrCmp "${ANSWER}" "closeApp" closeApp declined
!macroend
!endif
!ifdef GUARD
  !include "${GUARD}"
!endif
Section
  StrCpy $INSTDIR "${DESTINATION}"
!ifdef GUARD
  Push "$INSTDIR\clip_shot.exe"
  Call ClipShotEnsureMainAppClosed
!endif
  SetOutPath "$INSTDIR"
  File /oname=clip_shot.exe "${PAYLOAD}"
  IfErrors 0 +2
    SetErrorLevel 20
!ifdef GUARD
  WriteUninstaller "$INSTDIR\uninstall.exe"
!endif
SectionEnd
!ifdef GUARD
Section "Uninstall"
  Push "$INSTDIR\clip_shot.exe"
  Call un.ClipShotEnsureAppClosed
  Delete "$INSTDIR\clip_shot.exe"
SectionEnd
!endif
