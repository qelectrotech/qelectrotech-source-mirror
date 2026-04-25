; this file is part of installer for QElectroTech
; Copyright (C)2015 QElectroTech Team <scorpio@qelectrotech.org>
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

; WebSite : https://qelectrotech.org/

;==============================================================================
; NSIS 3.x compatibility notes:
;   - Unicode is native in NSIS 3 (no need for Unicode installer plugin)
;   - XPStyle is deprecated/removed; ManifestSupportedOS replaces it
;   - SetCompressor must appear before any Section/Function
;   - SetRegView 64 moved to top of the hidden section (before WriteRegStr)
;   - x64.nsh is still available but ${RunningX64} is now also in x64.nsh
;   - MUI2.nsh is unchanged; MUI_LANGDLL_ALLLANGUAGES is still valid
;   - FileFunc.nsh / Locate macro: unchanged
;   - Var /GLOBAL must be declared at global scope, not inside a Section
;==============================================================================

;--------------------------------
; NSIS 3 requires SetCompressor BEFORE any Section or Function
SetCompressor /FINAL /SOLID lzma

;--------------------------------
; Includes
!include x64.nsh
!include "MUI2.nsh"
!include "FileFunc.nsh"
!insertmacro Locate
!insertmacro GetParameters
!insertmacro GetOptions

;--------------------------------
; NSIS 3: Unicode is the default. The installer binary will be Unicode.
; No extra plugin needed.

!ifndef PROC
    !define PROC 64
!endif

;--------------------------------
; General Product Description Definitions
!define SOFT_NAME     "QElectroTech"
!define SOFT_VERSION  "0.5-dev_x86_64-win64+4094"
!define SOFT_WEB_SITE "https://qelectrotech.org/"
!define SOFT_BUILD    "1"

;--------------------------------
; General settings
CRCCheck force
BrandingText "${SOFT_NAME}-${SOFT_VERSION}-${SOFT_BUILD}"

; NSIS 3: XPStyle is removed. Use ManifestSupportedOS to declare modern OS support.
; This replaces "XPStyle on" and enables proper DPI awareness + visual styles.
ManifestSupportedOS all
ManifestDPIAware true

; Name and output file
Name "${SOFT_NAME} ${SOFT_VERSION}"
OutFile "Installer_${SOFT_NAME}-${SOFT_VERSION}-${SOFT_BUILD}.exe"

; Default installation folder
InstallDir "$PROGRAMFILES64\${SOFT_NAME}"

; Get installation folder from registry if available
; NSIS 3: InstallDirRegKey still works, but SetRegView 64 must be set at
; runtime (in .onInit) to read 64-bit registry hive correctly.
InstallDirRegKey HKCU "Software\${SOFT_NAME}" ""

; Request admin privileges (required for HKLM / file associations)
RequestExecutionLevel admin

;--------------------------------
; Declare global variables at script scope (NSIS 3 requirement)
; In NSIS 2 these could be declared inside a Section; that still compiles
; in NSIS 3 but triggers a warning. Declare them here.
Var final_qet_exe
Var final_project_ico
Var final_element_ico
Var final_titleblock_ico

;--------------------------------
; MUI Interface Settings
!define MUI_ABORTWARNING

; NSIS 3 ships updated icons; XPUI icons are still present for compatibility.
; You may switch to the modern ones:
!define MUI_ICON   "${NSISDIR}\Contrib\Graphics\Icons\nsis3-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\nsis3-uninstall.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP          ".\images\wizard.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP ".\images\header.bmp"

;--------------------------------
; Language Selection Dialog Settings (remember chosen language in registry)
!define MUI_LANGDLL_REGISTRY_ROOT      "HKCU"
!define MUI_LANGDLL_REGISTRY_KEY       "Software\${SOFT_NAME}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
; Pages
!define MUI_COMPONENTSPAGE_SMALLDESC
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "files\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Finish page – checkbox to launch QElectroTech
!define MUI_FINISHPAGE_RUN             "$INSTDIR\Lancer QET.bat"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_RUN_TEXT        "$(Check)"
!insertmacro MUI_PAGE_FINISH

;--------------------------------
; Languages
; NSIS 3: MUI_LANGDLL_ALLLANGUAGES is still supported and works as before.
!define MUI_LANGDLL_ALLLANGUAGES

!insertmacro MUI_LANGUAGE "English"   ; first = default
    !insertmacro MUI_LANGUAGE "Korean"
    !insertmacro MUI_LANGUAGE "French"
    !insertmacro MUI_LANGUAGE "Spanish"
    !insertmacro MUI_LANGUAGE "Russian"
    !insertmacro MUI_LANGUAGE "Portuguese"
    !insertmacro MUI_LANGUAGE "Czech"
    !insertmacro MUI_LANGUAGE "Polish"
    !insertmacro MUI_LANGUAGE "Greek"
    !insertmacro MUI_LANGUAGE "Arabic"
    !insertmacro MUI_LANGUAGE "German"
    !insertmacro MUI_LANGUAGE "Italian"
    !insertmacro MUI_LANGUAGE "Romanian"
    !insertmacro MUI_LANGUAGE "Catalan"
    !insertmacro MUI_LANGUAGE "Croatian"
    !insertmacro MUI_LANGUAGE "Dutch"
    !insertmacro MUI_LANGUAGE "Danish"
    !insertmacro MUI_LANGUAGE "Hungarian"
    !insertmacro MUI_LANGUAGE "Japanese"
    !insertmacro MUI_LANGUAGE "Mongolian"
    !insertmacro MUI_LANGUAGE "Norwegian"
    !insertmacro MUI_LANGUAGE "PortugueseBR"
    !insertmacro MUI_LANGUAGE "Serbian"
    !insertmacro MUI_LANGUAGE "Slovak"
    !insertmacro MUI_LANGUAGE "Slovenian"
    !insertmacro MUI_LANGUAGE "Swedish"
    !insertmacro MUI_LANGUAGE "Turkish"
    !insertmacro MUI_LANGUAGE "Ukrainian"
    !insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_RESERVEFILE_LANGDLL

; Language strings for all supported locales
!include lang_extra.nsh
!include lang_extra_fr.nsh
!include lang_extra_missing.nsh
; NOTE: The string "uninstFailed" must be defined in lang_extra.nsh and
; lang_extra_fr.nsh (and any other lang_extra_*.nsh) like so:
;   LangString uninstFailed ${LANG_ENGLISH} "Uninstallation of the previous version failed.$\nPlease uninstall QElectroTech manually before continuing."
;   LangString uninstFailed ${LANG_FRENCH}  "La désinstallation de la version précédente a échoué.$\nVeuillez désinstaller QElectroTech manuellement avant de continuer."

;==============================================================================
; SECTIONS
;==============================================================================

SetOverwrite on

Section "Main Program"
SectionIn RO  ; Read-only – always installed

    SetOutPath "$INSTDIR\bin\"
    File "./files/bin/${SOFT_NAME}.exe"

    SetOutPath "$INSTDIR"
    File "./files/ChangeLog"
    File "./files/CREDIT"
    File "./files/ELEMENTS.LICENSE"
    File "./files/LICENSE"
    File "./files/qet_uninstall_file_associations.reg"
    File "./files/README"
    File "./files/register_filetypes.bat"
    File "Lancer QET.bat"

    SetOutPath "$INSTDIR"
    File /r "./files/ico"

SectionEnd

;---------------------------
SetOverwrite on
SubSection "$(Elements)" SEC01

    SetOverwrite on
    Section "$(Electric)"
        SetOutPath "$INSTDIR\elements"
        File /r "./files/elements/10_electric"
    SectionEnd

    SetOverwrite on
    Section "$(Logic)"
        SetOutPath "$INSTDIR\elements"
        File /r "./files/elements/20_logic"
    SectionEnd

    SetOverwrite on
    Section "$(Hydraulic)"
        SetOutPath "$INSTDIR\elements"
        File /r "./files/elements/30_hydraulic"
    SectionEnd

    SetOverwrite on
    Section "$(Pneumatic)"
        SetOutPath "$INSTDIR\elements"
        File /r "./files/elements/50_pneumatic"
    SectionEnd

    ;---------------------------------
    SubSection "$(Energy)"

        SetOverwrite on
        Section "$(water)"
            SetOutPath "$INSTDIR\elements\60_energy"
            File /r "./files/elements/60_energy/11_water"
            File /r "./files/elements/60_energy/"
        SectionEnd

        SetOverwrite on
        Section "$(Refrigeration)"
            SetOutPath "$INSTDIR\elements\60_energy"
            File /r "./files/elements/60_energy/21_refrigeration"
            File /r "./files/elements/60_energy/"
        SectionEnd

        SetOverwrite on
        Section "$(Solar_thermal)"
            SetOutPath "$INSTDIR\elements\60_energy"
            File /r "./files/elements/60_energy/31_solar_thermal"
            File /r "./files/elements/60_energy/"
        SectionEnd

    SubSectionEnd

SubSectionEnd

;-------------------------------
SetOverwrite on
Section "$(Lang)" SEC02
    SetOutPath "$INSTDIR\lang"
    File "./files/lang/*.qm"
SectionEnd

SetOverwrite on
Section "$(Titleblocks)" SEC03
    SetOutPath "$INSTDIR"
    File /r "./files/titleblocks"
SectionEnd

SetOverwrite on
Section "$(Examples)" SEC04
    SetOutPath "$INSTDIR"
    File /r "./files/examples"
SectionEnd

SetOverwrite on
Section "$(Fonts)" SEC05
    SetOutPath "$INSTDIR"
    File /r "./files/fonts"
SectionEnd

;--------------------------------
; Component descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} $(var1)
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} $(var2)
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} $(var3)
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} $(var4)
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC05} $(var5)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Hidden section: registry, shortcuts, file associations
Section ""
    SetOutPath "$INSTDIR"

    ; NSIS 3 on 64-bit Windows: set 64-bit registry view BEFORE any WriteRegStr
    ; so keys land in HKLM\SOFTWARE (not the Wow6432Node redirect).
    SetRegView 64

    ; Store installation folder
    WriteRegStr HKCU "Software\${SOFT_NAME}" "" $INSTDIR

    ; Uninstall registry entries
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" \
        "DisplayName"     "${SOFT_NAME} (remove only)"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" \
        "UninstallString" '"$INSTDIR\Uninstall.exe"'

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Build paths for icons and launch script
    StrCpy $final_qet_exe          "$INSTDIR\Lancer QET.bat"
    StrCpy $final_project_ico      "$INSTDIR\ico\application-x-qet-project.ico"
    StrCpy $final_element_ico      "$INSTDIR\ico\application-x-qet-element.ico"
    StrCpy $final_titleblock_ico   "$INSTDIR\ico\application-x-qet-titleblock.ico"

    ; File associations – .qet
    WriteRegStr   HKEY_CLASSES_ROOT "Applications\qelectrotech.exe\shell\open\command" "" \
        '"$final_qet_exe" "%1"'
    WriteRegStr   HKEY_CLASSES_ROOT ".qet"                                              "" "qet_diagram_file"
    WriteRegStr   HKEY_CLASSES_ROOT "qet_diagram_file"                                  "" "Diagram QET"
    WriteRegDWORD HKEY_CLASSES_ROOT "qet_diagram_file"                                  "EditFlags"    0x00000000
    WriteRegDWORD HKEY_CLASSES_ROOT "qet_diagram_file"                                  "BrowserFlags" 0x00000008
    WriteRegStr   HKEY_CLASSES_ROOT "qet_diagram_file\DefaultIcon"                      "" "$final_project_ico"
    WriteRegStr   HKEY_CLASSES_ROOT "qet_diagram_file\shell\open\command"               "" '"$final_qet_exe" "%1"'

    ; File associations – .elmt
    WriteRegStr   HKEY_CLASSES_ROOT ".elmt"                                             "" "qet_element_file"
    WriteRegStr   HKEY_CLASSES_ROOT "qet_element_file"                                  "" "Element QET"
    WriteRegDWORD HKEY_CLASSES_ROOT "qet_element_file"                                  "EditFlags"    0x00000000
    WriteRegDWORD HKEY_CLASSES_ROOT "qet_element_file"                                  "BrowserFlags" 0x00000008
    WriteRegStr   HKEY_CLASSES_ROOT "qet_element_file\DefaultIcon"                      "" "$final_element_ico"
    WriteRegStr   HKEY_CLASSES_ROOT "qet_element_file\shell\open\command"               "" '"$final_qet_exe" "%1"'

    ; File associations – .titleblock
    WriteRegStr   HKEY_CLASSES_ROOT ".titleblock"                                       "" "qet_titleblock_file"
    WriteRegStr   HKEY_CLASSES_ROOT "qet_titleblock_file"                               "" "Titleblock QET"
    WriteRegDWORD HKEY_CLASSES_ROOT "qet_titleblock_file"                               "EditFlags"    0x00000000
    WriteRegDWORD HKEY_CLASSES_ROOT "qet_titleblock_file"                               "BrowserFlags" 0x00000008
    WriteRegStr   HKEY_CLASSES_ROOT "qet_titleblock_file\DefaultIcon"                   "" "$final_titleblock_ico"
    WriteRegStr   HKEY_CLASSES_ROOT "qet_titleblock_file\shell\open\command"            "" '"$final_qet_exe" "%1"'

    SetShellVarContext all  ; apply shortcuts for all users

    ; Start Menu shortcuts
    CreateDirectory "$SMPROGRAMS\${SOFT_NAME}"
    CreateDirectory "$SMPROGRAMS\${SOFT_NAME}\Manual"
    CreateDirectory "$SMPROGRAMS\${SOFT_NAME}\Upgrade"
    CreateShortCut  "$SMPROGRAMS\${SOFT_NAME}\QElectroTech.lnk" \
        "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\ico\qelectrotech.ico"
    CreateShortCut  "$SMPROGRAMS\${SOFT_NAME}\Uninstall QElectroTech.lnk" \
        "$INSTDIR\Uninstall.exe"

    ; Desktop shortcut
    CreateShortCut "$DESKTOP\QElectroTech.lnk" \
        "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\ico\qelectrotech.ico"

    ; Internet shortcuts
    WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Manual\Manual_English.url" \
        "InternetShortcut" "URL" "https://download.qelectrotech.org/qet/manual_0.7/build/index.html"
    WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Manual\Manual_Russian.url" \
        "InternetShortcut" "URL" "https://download.qelectrotech.org/qet/joshua/html/QET_ru.html"
    WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Upgrade\Download.url" \
        "InternetShortcut" "URL" "https://download.qelectrotech.org/qet/builds/nightly/"
    WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Donate.url" \
        "InternetShortcut" "URL" "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZZHC9D7C3MDPC"

    ; Mark installed elements as read-only
    ${Locate} "$INSTDIR\elements\" "/L=FD /M=*.elmt" "LocateCallback"
    IfErrors 0 +2
    ; MessageBox MB_OK "Error in Locate"  ; uncomment for debugging

SectionEnd

;--------------------------------
; Locate callback – sets FILE_ATTRIBUTE_READONLY on each .elmt file
Function LocateCallback
    SetFileAttributes $R9 FILE_ATTRIBUTE_READONLY
    Push $0
FunctionEnd

;==============================================================================
; INSTALLER FUNCTIONS
;==============================================================================

Function .onInit
    ; NSIS 3: SetRegView in .onInit ensures InstallDirRegKey reads the right hive
    SetRegView 64

    !insertmacro MUI_LANGDLL_DISPLAY

    ; Abort if not running on a 64-bit OS
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP $(wrongArch)
        Abort
    ${EndIf}

    ; ----------------------------------------------------------------
    ; Auto-uninstall previous version before installing new one
    ; ----------------------------------------------------------------
    ReadRegStr $R0 HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" \
        "UninstallString"

    ; No previous installation found → proceed normally
    StrCmp $R0 "" done

    ; Also read the install dir of the previous version
    ReadRegStr $R1 HKCU "Software\${SOFT_NAME}" ""

    ; Ask user whether to uninstall the existing version
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "$(installed)" IDOK uninst
    Abort   ; user clicked Cancel → stop the installer

uninst:
    ; Remove surrounding quotes from the UninstallString if present
    ; (some installers write: "C:\path\Uninstall.exe" — ExecWait needs clean path)
    StrCpy $R2 $R0 1        ; first character
    StrCmp $R2 '"' 0 unquoted
        ; Strip leading and trailing quote
        StrCpy $R0 $R0 "" 1                 ; remove leading "
        StrLen $R3 $R0
        IntOp $R3 $R3 - 1
        StrCpy $R0 $R0 $R3                  ; remove trailing "
unquoted:

    ; Run the uninstaller silently, keeping it in its own directory
    ; _?= prevents NSIS from copying the uninstaller to a temp folder,
    ; so it can delete itself and the whole $INSTDIR tree.
    ClearErrors
    ${If} $R1 != ""
        ExecWait '"$R0" /S _?=$R1'         ; silent uninstall using saved install dir
    ${Else}
        ExecWait '"$R0" /S'                 ; fallback if install dir unknown
    ${EndIf}

    IfErrors uninstall_failed

    ; Verify the old installation is gone before continuing
    ${If} $R1 != ""
        IfFileExists "$R1\${SOFT_NAME}.exe" uninstall_failed
        IfFileExists "$R1\bin\${SOFT_NAME}.exe" uninstall_failed
    ${EndIf}

    Goto done

uninstall_failed:
    MessageBox MB_OK|MB_ICONSTOP "$(uninstFailed)"
    Abort

done:

FunctionEnd

;==============================================================================
; UNINSTALLER SECTION
;==============================================================================

Section "Uninstall"
    SetRegView 64   ; NSIS 3: required so we delete from the correct hive
    SetShellVarContext all

    ; Remove Start Menu shortcuts
    RMDir /r "$SMPROGRAMS\${SOFT_NAME}"

    ; Remove Desktop shortcut
    Delete "$DESKTOP\QElectroTech.lnk"

    ; Remove application files
    Delete "$INSTDIR\*.*"
    RMDir /r "$INSTDIR"

    ; Remove installation registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}"
    DeleteRegKey /ifempty HKCU "Software\${SOFT_NAME}"

    ; Remove file associations registry keys
    DeleteRegKey HKEY_CLASSES_ROOT "Applications\qelectrotech.exe"
    DeleteRegKey HKEY_CLASSES_ROOT ".qet"
    DeleteRegKey HKEY_CLASSES_ROOT "qet_diagram_file"
    DeleteRegKey HKEY_CLASSES_ROOT ".elmt"
    DeleteRegKey HKEY_CLASSES_ROOT "qet_element_file"
    DeleteRegKey HKEY_CLASSES_ROOT ".titleblock"
    DeleteRegKey HKEY_CLASSES_ROOT "qet_titleblock_file"

    IfFileExists "$INSTDIR" 0 NoErrorMsg
    ; MessageBox MB_OK "Note: $INSTDIR could not be removed!"
    NoErrorMsg:

SectionEnd

;==============================================================================
; UNINSTALLER FUNCTIONS
;==============================================================================

Function un.onInit
    SetRegView 64   ; NSIS 3: match the view used during install
    !insertmacro MUI_UNGETLANGUAGE
FunctionEnd
