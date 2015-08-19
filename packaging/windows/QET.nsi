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

; WebSite : http://qelectrotech.org/

;--------------------------------
;Include Modern UI
	
	!include "MUI2.nsh"
	!include "FileFunc.nsh"
	!insertmacro Locate
	!include FileFunc.nsh
	!insertmacro GetParameters
	!insertmacro GetOptions


	
; MUI Settings
;--------------------------------
;General
	; General Product Description Definitions
	!define SOFT_NAME     "QElectroTech"
	!define SOFT_VERSION  "0.5-dev+4113"
	!define SOFT_WEB_SITE "http://qelectrotech.org/"
	!define SOFT_BUILD    "1"
	
	SetCompressor /final /solid lzma
	CRCCheck force
	XPStyle on
	BrandingText "${SOFT_NAME}-${SOFT_VERSION}-${SOFT_BUILD}"   ; Shows in the Bottom Left of the installer
	
	;Name and file
	Name "${SOFT_NAME} ${SOFT_VERSION}"
	OutFile "Installer_${SOFT_NAME}-${SOFT_VERSION}-${SOFT_BUILD}.exe"

	;Default installation folder
	InstallDir "$PROGRAMFILES\${SOFT_NAME}"

	;Get installation folder from registry if available
	InstallDirRegKey HKCU "Software\${SOFT_NAME}" ""

	;Request application privileges for Windows Vista
	; we request for admin because we write stuff into the registry
	RequestExecutionLevel admin

;--------------------------------
;Interface Settings

	!define MUI_ABORTWARNING
	!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\XPUI-install.ico"
	!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\XPUI-uninstall.ico"
	!define MUI_LICENSEPAGE_CHECKBOX
	

	!define MUI_WELCOMEFINISHPAGE_BITMAP ".\images\wizard.bmp"
	!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

	!define MUI_HEADERIMAGE
	!define MUI_HEADERIMAGE_BITMAP ".\images\header.bmp" ; optional
	
;--------------------------------
;Language Selection Dialog Settings

	;Remember the installer language
	!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
	!define MUI_LANGDLL_REGISTRY_KEY "Software\${SOFT_NAME}" 
	!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

	!define MUI_COMPONENTSPAGE_SMALLDESC
	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "files\LICENSE"
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES

	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES
	
; Finish page and checkbox to run QElectroTech
	!define MUI_FINISHPAGE_RUN "$INSTDIR\Lancer QET.bat"
	!define MUI_FINISHPAGE_RUN_NOTCHECKED
	!define MUI_FINISHPAGE_RUN_TEXT "Check to start ${SOFT_NAME}"
	!insertmacro MUI_PAGE_FINISH
;--------------------------------
;Languages
	;Since NSIS 2.26, the language selection dialog of Modern UI hides languages unsupported by the user's selected codepage by default.
	;To revert to the old behavior and display all languages, no matter what the user will see when they're selected, use MUI_LANGDLL_ALLLANGUAGES.
	!define MUI_LANGDLL_ALLLANGUAGES
	
	; For consistency, we limit the installer to languages supported by QElectroTech itself
	!insertmacro MUI_LANGUAGE "English" ;first language is the default language
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

	!insertmacro MUI_RESERVEFILE_LANGDLL
	!include lang_extra.nsh
	

	
SetOverwrite on	
Section "Main Program"
SectionIn RO ; Read only, always installed

Setoutpath "$INSTDIR\bin\"
File "./files/bin/${SOFT_NAME}.exe"

Setoutpath "$INSTDIR"
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

SetOutPath "$INSTDIR"
File /r "./files/conf"

SectionEnd

;---------------------------
SetOverwrite on
SubSection "$(Elements)" 

SetOverwrite on
Section "$(Electric)" 
  SetOutPath "$INSTDIR"
  File /r "./files/elements/10_electric"
SectionEnd

SetOverwrite on
Section "$(Logic)" 
  SetOutPath "$INSTDIR"
  File /r "./files/elements/20_logic"
SectionEnd

SetOverwrite on
Section  "$(Hydraulic)" 
  SetOutPath "$INSTDIR"
  File /r "./files/elements/30_hydraulic"
SectionEnd


SetOverwrite on
Section "$(Pneumatic)" 
  SetOutPath "$INSTDIR"
  File /r "./files/elements/50_pneumatic"
SectionEnd


;---------------------------------
SubSection "$(Energy)" 

SetOverwrite on
Section  "$(water)" 
  SetOutPath "$INSTDIR"
  File /r "./files/elements/60_energy/11_water"
SectionEnd

SetOverwrite on
Section  "$(Refrigeration)" 
  SetOutPath "$INSTDIR"
  File /r "./files/elements/60_energy/21_refrigeration"
SectionEnd

SetOverwrite on
Section  "$(Solar_thermal)" 
  SetOutPath "$INSTDIR"
  File /r "./files/elements/60_energy/31_solar_thermal"
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

;--------------------------------


;Installer Sections

Section ""
	SetOutPath "$INSTDIR"  
	;Store installation folder
	WriteRegStr HKCU "Software\${SOFT_NAME}" "" $INSTDIR
	; write uninstall strings
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" "DisplayName"     "${SOFT_NAME} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	
	; get the final path for the icons and the launch script
	Var /GLOBAL final_qet_exe
	Var /GLOBAL final_project_ico
	Var /GLOBAL final_element_ico
	Var /GLOBAL final_titleblock_ico

	StrCpy $final_qet_exe     "$INSTDIR\Lancer QET.bat"
	StrCpy $final_project_ico "$INSTDIR\\ico\application-x-qet-project.ico"
	StrCpy $final_element_ico "$INSTDIR\\ico\application-x-qet-element.ico"
	StrCpy $final_titleblock_ico "$INSTDIR\\ico\application-x-qet-titleblock.ico"
	
	; write file associations registry keys
	WriteRegStr   HKEY_CLASSES_ROOT "Applications\qelectrotech.exe\shell\open\command" ""             "$\"$final_qet_exe$\" $\"%1$\""
	WriteRegStr   HKEY_CLASSES_ROOT ".qet"                                             ""             "qet_diagram_file"
	WriteRegStr   HKEY_CLASSES_ROOT "qet_diagram_file"                                 ""             "Schéma QET"
	WriteRegDWORD HKEY_CLASSES_ROOT "qet_diagram_file"                                 "EditFlags"    0x00000000
	WriteRegDWORD HKEY_CLASSES_ROOT "qet_diagram_file"                                 "BrowserFlags" 0x00000008
	WriteRegStr   HKEY_CLASSES_ROOT "qet_diagram_file\DefaultIcon"                     ""             "$final_project_ico"
	WriteRegStr   HKEY_CLASSES_ROOT "qet_diagram_file\shell\open\command"              ""             "$\"$final_qet_exe$\" $\"%1$\""
	WriteRegStr   HKEY_CLASSES_ROOT ".elmt"                                            ""             "qet_element_file"
	WriteRegStr   HKEY_CLASSES_ROOT "qet_element_file"                                 ""             "Élément QET"
	WriteRegDWORD HKEY_CLASSES_ROOT "qet_element_file"                                 "EditFlags"    0x00000000
	WriteRegDWORD HKEY_CLASSES_ROOT "qet_element_file"                                 "BrowserFlags" 0x00000008
	WriteRegStr   HKEY_CLASSES_ROOT "qet_element_file\DefaultIcon"                     ""             "$final_element_ico"
	WriteRegStr   HKEY_CLASSES_ROOT "qet_element_file\shell\open\command"              ""             "$\"$final_qet_exe$\" $\"%1$\""
	WriteRegStr   HKEY_CLASSES_ROOT ".titleblock"                                      ""             "qet_titleblock_file"
	WriteRegStr   HKEY_CLASSES_ROOT "qet_titleblock_file"                              ""             "Titleblock QET"
	WriteRegDWORD HKEY_CLASSES_ROOT "qet_titleblock_file"                              "EditFlags"    0x00000000
	WriteRegDWORD HKEY_CLASSES_ROOT "qet_titleblock_file"                              "BrowserFlags" 0x00000008
	WriteRegStr   HKEY_CLASSES_ROOT "qet_titleblock_file\DefaultIcon"                  ""             "$final_titleblock_ico"
	WriteRegStr   HKEY_CLASSES_ROOT "qet_titleblock_file\shell\open\command"           ""             "$\"$final_qet_exe$\" $\"%1$\""
	
	SetShellVarContext all ; all users
	; shortcuts in the start menu
	CreateDirectory "$SMPROGRAMS\${SOFT_NAME}"
	CreateDirectory "$SMPROGRAMS\${SOFT_NAME}\Manual"
	CreateDirectory "$SMPROGRAMS\${SOFT_NAME}\Upgrade"
	CreateShortCut  "$SMPROGRAMS\${SOFT_NAME}\QElectroTech.lnk"           "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\\ico\qelectrotech.ico"
	CreateShortCut  "$SMPROGRAMS\${SOFT_NAME}\Uninstall QElectroTech.lnk" "$INSTDIR\Uninstall.exe"
	
	; shortcut on the desktop
	CreateShortCut "$DESKTOP\QElectroTech.lnk" "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\ico\qelectrotech.ico"
	WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Manual\Manual_English.url" "InternetShortcut" "URL" "http://download.tuxfamily.org/qet/joshua/html/QET.html"
	WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Manual\Manual_Russian.url" "InternetShortcut" "URL" "http://download.tuxfamily.org/qet/joshua/html/QET_ru.html"
	WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Upgrade\Download.url"      "InternetShortcut" "URL" "http://download.tuxfamily.org/qet/builds/nightly/"
	WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Donate.url"                "InternetShortcut" "URL" "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZZHC9D7C3MDPC"
	
	;changing $INSTDIR\elements\ *.elmt to read-only attribute
	${Locate} "$INSTDIR\elements\" "/L=FD /M=*.elmt" "LocateCallback"
	IfErrors 0 +2
	;MessageBox MB_OK "Error"
SectionEnd

Function LocateCallback
	SetFileAttributes $R9 FILE_ATTRIBUTE_READONLY
	Push $0
FunctionEnd
;--------------------------------
;Installer Functions

Function .onInit

	!insertmacro MUI_LANGDLL_DISPLAY
	
;Auto-uninstall old before installing new
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" \
  "UninstallString"
  StrCmp $R0 "" done

MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "$(installed)" \
  IDOK uninst
  Abort


;Run the uninstaller
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file

  IfErrors no_remove_uninstaller done
    ;You can either use Delete /REBOOTOK in the uninstaller or add some code
    ;here to remove the uninstaller. Use a registry key to check
    ;whether the user has chosen to uninstall. If you are using an uninstaller
    ;components page, make sure all sections are uninstalled.
  no_remove_uninstaller:
 
done:

FunctionEnd

;--------------------------------
;Descriptions

	;USE A LANGUAGE STRING IF YOU WANT YOUR DESCRIPTIONS TO BE LANGAUGE SPECIFIC

	;Assign descriptions to sections
	;!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	;!insertmacro MUI_DESCRIPTION_TEXT ${CopyFiles} "CopyFiles"
	;!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"
	SetShellVarContext all ; all users
	; remove start menu shortcuts
	RMDir /r "$SMPROGRAMS\${SOFT_NAME}"
	; remove shortcut on the desktop
	Delete "$DESKTOP\QElectroTech.lnk"
	
	; remove the application files
	Delete "$INSTDIR\*.*"
	RMDir /r "$INSTDIR"
	
	;remove installation registry keys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}"
	DeleteRegKey /ifempty HKCU "Software\${SOFT_NAME}"
	
	; remove file associations registry keys
	DeleteRegKey HKEY_CLASSES_ROOT "Applications\qelectrotech.exe"
	DeleteRegKey HKEY_CLASSES_ROOT ".qet"
	DeleteRegKey HKEY_CLASSES_ROOT "qet_diagram_file"
	DeleteRegKey HKEY_CLASSES_ROOT ".elmt"
	DeleteRegKey HKEY_CLASSES_ROOT "qet_element_file"
	DeleteRegKey HKEY_CLASSES_ROOT ".titleblock"
	DeleteRegKey HKEY_CLASSES_ROOT "qet_titleblock_file"
	
	IfFileExists "$INSTDIR" 0 NoErrorMsg
	;MessageBox MB_OK "Note: $INSTDIR could not be removed!" IDOK 0 ; skipped if file doesn't exist
	NoErrorMsg:
SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

	!insertmacro MUI_UNGETLANGUAGE

FunctionEnd
