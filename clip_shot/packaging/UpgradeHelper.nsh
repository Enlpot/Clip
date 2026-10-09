!ifndef CLIP_SHOT_UPGRADE_HELPER_INCLUDED
!define CLIP_SHOT_UPGRADE_HELPER_INCLUDED

Var ClipShotUpgradeHelperRoot
Var ClipShotUpgradeHelperBackup

; Old uninstallers launch the helper installed alongside them. Bootstrap that
; entry point with this package's helper so fixes also apply to existing copies.
; The native helper owns locking, backup, and atomic replacement.
Function ClipShotPrepareUpgradeHelper
  Exch $0
  Push $1
  Push $2
  StrCpy $ClipShotUpgradeHelperRoot ""
  StrCpy $ClipShotUpgradeHelperBackup ""
  IfFileExists "$0\bin\${CLIP_SHOT_INSTALLER_UPDATER}.exe" 0 clipUpgradeHelperDone
  InitPluginsDir
  StrCpy $1 $OUTDIR
  ClearErrors
  SetOutPath "$PLUGINSDIR"
  File /oname=clip-shot-installer-updater.exe "${INST_DIR}\bin\${CLIP_SHOT_INSTALLER_UPDATER}.exe"
  StrCpy $2 0
  IfErrors 0 +2
    StrCpy $2 1
  SetOutPath "$1"
  StrCmp $2 0 0 clipUpgradeHelperFailed
  ClearErrors
  ExecWait '"$PLUGINSDIR\clip-shot-installer-updater.exe" --prepare-installer-upgrade --target "$0" --backup "$PLUGINSDIR\previous-updater.exe"' $2
  IfErrors clipUpgradeHelperFailed
  StrCmp $2 0 0 clipUpgradeHelperFailed
  StrCpy $ClipShotUpgradeHelperRoot $0
  StrCpy $ClipShotUpgradeHelperBackup "$PLUGINSDIR\previous-updater.exe"
  Goto clipUpgradeHelperDone
clipUpgradeHelperFailed:
  MessageBox MB_OK|MB_ICONSTOP "$(ClipShotStartupCleanupFailed)" /SD IDOK
  SetErrorLevel 12
  Quit
clipUpgradeHelperDone:
  Pop $2
  Pop $1
  Pop $0
  ClearErrors
FunctionEnd

; Restore the original entry point when the previous uninstall fails. The
; native helper checks that another operation has not changed it in the meantime.
Function ClipShotRestoreUpgradeHelper
  Push $0
  StrCmp $ClipShotUpgradeHelperRoot "" clipUpgradeRestoreDone
  ClearErrors
  ExecWait '"$PLUGINSDIR\clip-shot-installer-updater.exe" --restore-installer-upgrade --target "$ClipShotUpgradeHelperRoot" --backup "$ClipShotUpgradeHelperBackup"' $0
  IfErrors clipUpgradeRestoreFailed
  StrCmp $0 0 clipUpgradeRestoreDone
clipUpgradeRestoreFailed:
  MessageBox MB_OK|MB_ICONSTOP "$(ClipShotStartupCleanupFailed)" /SD IDOK
  SetErrorLevel 14
  Quit
clipUpgradeRestoreDone:
  StrCpy $ClipShotUpgradeHelperRoot ""
  StrCpy $ClipShotUpgradeHelperBackup ""
  Pop $0
FunctionEnd
!endif
