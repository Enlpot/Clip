!ifndef CLIP_SHOT_OWNED_CLEANUP_INCLUDED
!define CLIP_SHOT_OWNED_CLEANUP_INCLUDED
!include "FileFunc.nsh"
!ifndef CLIP_SHOT_INSTALLER_EXECUTABLE
!define CLIP_SHOT_INSTALLER_EXECUTABLE "clip_shot"
!endif
!ifndef CLIP_SHOT_INSTALLER_UPDATER
!define CLIP_SHOT_INSTALLER_UPDATER "clip-shot-updater"
!endif

; The update helper tracks files that self-updates added after this
; uninstaller was built, plus the startup registrations. Run a copy of it
; before CPack's original file deletion list, so the copy is not deleted
; underneath itself. If the copy cannot be isolated, run the installed
; helper instead: file deletion has not started yet. Missing components of
; a partial installation are skipped like any other missing file: the guard
; functions finish without prompting when their target does not exist, and
; a missing or unlaunchable helper leaves nothing to run and removes
; nothing. Only a helper that ran and failed blocks the uninstallation,
; because its failure (running application, active update, access denied)
; is recoverable before files are removed.
!macro ClipShotUninstallOwnedCleanup
  Push "$INSTDIR\bin\${CLIP_SHOT_INSTALLER_EXECUTABLE}.exe"
  Call un.ClipShotEnsureAppClosed
  Push "$INSTDIR\bin\crashpad_handler.exe"
  Call un.ClipShotEnsureAppClosed
  IfFileExists "$INSTDIR\bin\${CLIP_SHOT_INSTALLER_UPDATER}.exe" 0 clipOwnedDone
    InitPluginsDir
    StrCpy $4 "$INSTDIR\bin\${CLIP_SHOT_INSTALLER_UPDATER}.exe"
    ClearErrors
    CopyFiles /SILENT "$INSTDIR\bin\${CLIP_SHOT_INSTALLER_UPDATER}.exe" "$PLUGINSDIR\${CLIP_SHOT_INSTALLER_UPDATER}.exe"
    IfErrors clipOwnedRun
    StrCpy $4 "$PLUGINSDIR\${CLIP_SHOT_INSTALLER_UPDATER}.exe"
  clipOwnedRun:
    StrCpy $1 ""
    ${GetParameters} $2
    ClearErrors
    ${GetOptions} $2 "/CLIPUPGRADE" $3
    IfErrors +2
      StrCpy $1 "--upgrade"
    ClearErrors
    ExecWait '"$4" --uninstall --target "$INSTDIR" $1' $0
    IfErrors clipOwnedDone
    StrCmp $0 0 clipOwnedDone clipOwnedFailed
clipOwnedFailed:
    MessageBox MB_OK|MB_ICONSTOP "$(ClipShotStartupCleanupFailed)" /SD IDOK
    SetErrorLevel 12
    Quit
clipOwnedDone:
!macroend
!endif
