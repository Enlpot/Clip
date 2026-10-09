!ifndef CLIP_SHOT_INSTALLER_LAUNCH_INCLUDED
!define CLIP_SHOT_INSTALLER_LAUNCH_INCLUDED
!ifndef CLIP_SHOT_INSTALLER_UPDATER
!define CLIP_SHOT_INSTALLER_UPDATER "clip-shot-updater"
!endif

; Use the desktop shell through the installed helper so an elevated installer
; does not restart Clip with its own administrator token.
Function ClipShotLaunchDesktop
  ClearErrors
  ExecWait '"$INSTDIR\bin\${CLIP_SHOT_INSTALLER_UPDATER}.exe" --launch-desktop --target "$INSTDIR"' $0
  IfErrors clipDesktopFailed
  StrCmp $0 0 clipDesktopDone
clipDesktopFailed:
  DetailPrint "$(ClipShotDesktopLaunchFailed)"
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(ClipShotDesktopLaunchFailed)" /SD IDOK
clipDesktopDone:
FunctionEnd

; MUI's finish-page checkbox handles interactive launches. Silent setup has
; no finish page, and only restarts an app it closed after installation succeeds.
Function .onInstSuccess
  IfSilent 0 clipRestartDone
  StrCmp $ClipShotRestartRequired 1 0 clipRestartDone
  Call ClipShotLaunchDesktop
clipRestartDone:
FunctionEnd
!endif
