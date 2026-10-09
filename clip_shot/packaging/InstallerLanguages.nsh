!ifndef CLIP_SHOT_INSTALLER_LANGUAGES_INCLUDED
!define CLIP_SHOT_INSTALLER_LANGUAGES_INCLUDED
!include "LogicLib.nsh"
!define MUI_LANGDLL_ALWAYSSHOW
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_LANGDLL_WINDOWTITLE "$(ClipShotLanguageTitle)"
!define MUI_LANGDLL_INFO "$(ClipShotLanguagePrompt)"

!macro ClipShotInstallerLanguages
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "TradChinese"
!macroend

; Match CPack's registry context before language selection and upgrade prompts.
!macro ClipShotLanguageContext
  Push $0
  ClearErrors
  UserInfo::GetAccountType
  Pop $0
  SetShellVarContext current
  ${If} $0 == "Admin"
  ${OrIf} $0 == "Power"
    SetShellVarContext all
  ${EndIf}
  Pop $0
!macroend

; A reinstall can change language without running the previous uninstaller.
!macro ClipShotDeleteUninstallShortcuts Folder
  Delete "${Folder}\${ClipShotUninstallShortcut1033}.lnk"
  Delete "${Folder}\${ClipShotUninstallShortcut2052}.lnk"
  Delete "${Folder}\${ClipShotUninstallShortcut1028}.lnk"
  Delete "${Folder}\Uninstall.lnk"
!macroend
!endif
