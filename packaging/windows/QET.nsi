; Installation script for QET
; Written by QElectroTech Team
; Licence : CC-BY-SA - http://creativecommons.org/licenses/by-sa/3.0/
; WebSite : http://qelectrotech.org/

;--------------------------------
;Include Modern UI
	
	;!include "UMUI.nsh"
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
	!define SOFT_VERSION  "0.5-dev+4103"
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
	;!define MUI_ICON ".\images\npp_inst.ico"

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

	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "files\LICENSE"
	!insertmacro MUI_PAGE_COMPONENTS  #todo listbox to choice components to install.
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
	;!insertmacro MUI_LANGUAGE "Nederland"

;--------------------------------
;Reserve Files
  
	;If you are using solid compression, files that are required before
	;the actual installation should be stored first in the data block,
	;because this will make your installer start faster.

	!insertmacro MUI_RESERVEFILE_LANGDLL
	
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
	
SetOverwrite on
Section "Elements" SEC01
  SetOutPath "$INSTDIR"
  ;SetOverwrite try
  File /r "./files/elements"
SectionEnd

SetOverwrite on
Section "Lang" SEC02
  SetOutPath "$INSTDIR\lang"
  ;SetOverwrite try
  File "./files/lang/*.qm"
SectionEnd

SetOverwrite on
Section "Titleblocks" SEC03
  SetOutPath "$INSTDIR"
  ;SetOverwrite try
  File /r "./files/titleblocks"
SectionEnd

SetOverwrite on
Section "Examples" SEC04
  SetOutPath "$INSTDIR"
  ;SetOverwrite try
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
	StrCpy $final_project_ico "$INSTDIR\ico\application-x-qet-project.ico"
	StrCpy $final_element_ico "$INSTDIR\ico\application-x-qet-element.ico"
	StrCpy $final_titleblock_ico "$INSTDIR\ico\application-x-qet-titleblock.ico"
	
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
	
	SetShellVarContext all ; all users
	; shortcuts in the start menu
	CreateDirectory "$SMPROGRAMS\${SOFT_NAME}"
	CreateDirectory "$SMPROGRAMS\${SOFT_NAME}\Manual"
	CreateDirectory "$SMPROGRAMS\${SOFT_NAME}\Upgrade"
	CreateShortCut  "$SMPROGRAMS\${SOFT_NAME}\QElectroTech.lnk"           "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\ico\qelectrotech.ico"
	CreateShortCut  "$SMPROGRAMS\${SOFT_NAME}\Uninstall QElectroTech.lnk" "$INSTDIR\Uninstall.exe"
	
	; shortcut on the desktop
	CreateShortCut "$DESKTOP\QElectroTech.lnk" "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\ico\qelectrotech.ico"
	WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Manual\Manual_English.url" "InternetShortcut" "URL" "http://download.tuxfamily.org/qet/joshua/html/QET.html"
	WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Manual\Manual_Russian.url" "InternetShortcut" "URL" "http://download.tuxfamily.org/qet/joshua/html/QET_ru.html"
	WriteINIStr "$SMPROGRAMS\${SOFT_NAME}\Upgrade\Download.url"      "InternetShortcut" "URL" "http://download.tuxfamily.org/qet/builds/"
	
	;changing $INSTDIR\elements\ *.elmt to read-only attribute
	
	${Locate} "$INSTDIR\elements\" "/L=FD /M=*.elmt" "LocateCallback"
	IfErrors 0 +2
	MessageBox MB_OK "Error"
SectionEnd

Function LocateCallback
	SetFileAttributes $R9 FILE_ATTRIBUTE_READONLY
	Push $0
FunctionEnd
;--------------------------------
;Installer Functions

Function .onInit

	!insertmacro MUI_LANGDLL_DISPLAY

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
	
	IfFileExists "$INSTDIR" 0 NoErrorMsg
	;MessageBox MB_OK "Note: $INSTDIR could not be removed!" IDOK 0 ; skipped if file doesn't exist
	NoErrorMsg:
SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

	!insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd
