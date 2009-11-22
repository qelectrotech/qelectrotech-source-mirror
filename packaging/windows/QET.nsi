; Installation script for QET
; Written by Cyril Frausti
; Licence : CC-BY-SA - http://creativecommons.org/licenses/by-sa/3.0/
; WebSite : http://qelectrotech.org/

;--------------------------------
;Include Modern UI
	!include "MUI2.nsh"
	
; MUI Settings
;--------------------------------
;General
	; General Product Description Definitions
	!define SOFT_NAME "QElectroTech"
	!define SOFT_VERSION "svn"
	!define SOFT_WEB_SITE "http://qelectrotech.org/"
	!define SOFT_BUILD "1cf"
	
	SetCompressor /final /solid lzma
	CRCCheck force
	XPStyle on
	
	;Name and file
	Name "${SOFT_NAME} ${SOFT_VERSION}"
	OutFile "Installer_${SOFT_NAME}-${SOFT_VERSION}-${SOFT_BUILD}.exe"

	;Default installation folder
	InstallDir "$PROGRAMFILES\${SOFT_NAME}"
  
	;Get installation folder from registry if available
	InstallDirRegKey HKCU "Software\${SOFT_NAME}" ""

	;Request application privileges for Windows Vista
	RequestExecutionLevel user

;--------------------------------
;Interface Settings
	!define MUI_ABORTWARNING
	!define MUI_ICON "install.ico"
	!define MUI_UNICON "uninstall.ico"
	
;--------------------------------
;Language Selection Dialog Settings

	;Remember the installer language
	!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
	!define MUI_LANGDLL_REGISTRY_KEY "Software\${SOFT_NAME}" 
	!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

	!insertmacro MUI_PAGE_LICENSE "files\LICENSE"
	;!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES

	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

	!insertmacro MUI_LANGUAGE "English" ;first language is the default language
	!insertmacro MUI_LANGUAGE "French"
	!insertmacro MUI_LANGUAGE "German"
	!insertmacro MUI_LANGUAGE "Spanish"
	!insertmacro MUI_LANGUAGE "SpanishInternational"
	!insertmacro MUI_LANGUAGE "SimpChinese"
	!insertmacro MUI_LANGUAGE "TradChinese"
	!insertmacro MUI_LANGUAGE "Japanese"
	!insertmacro MUI_LANGUAGE "Korean"
	!insertmacro MUI_LANGUAGE "Italian"
	!insertmacro MUI_LANGUAGE "Dutch"
	!insertmacro MUI_LANGUAGE "Danish"
	!insertmacro MUI_LANGUAGE "Swedish"
	!insertmacro MUI_LANGUAGE "Norwegian"
	!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
	!insertmacro MUI_LANGUAGE "Finnish"
	!insertmacro MUI_LANGUAGE "Greek"
	!insertmacro MUI_LANGUAGE "Russian"
	!insertmacro MUI_LANGUAGE "Portuguese"
	!insertmacro MUI_LANGUAGE "PortugueseBR"
	!insertmacro MUI_LANGUAGE "Polish"
	!insertmacro MUI_LANGUAGE "Ukrainian"
	!insertmacro MUI_LANGUAGE "Czech"
	!insertmacro MUI_LANGUAGE "Slovak"
	!insertmacro MUI_LANGUAGE "Croatian"
	!insertmacro MUI_LANGUAGE "Bulgarian"
	!insertmacro MUI_LANGUAGE "Hungarian"
	!insertmacro MUI_LANGUAGE "Thai"
	!insertmacro MUI_LANGUAGE "Romanian"
	!insertmacro MUI_LANGUAGE "Latvian"
	!insertmacro MUI_LANGUAGE "Macedonian"
	!insertmacro MUI_LANGUAGE "Estonian"
	!insertmacro MUI_LANGUAGE "Turkish"
	!insertmacro MUI_LANGUAGE "Lithuanian"
	!insertmacro MUI_LANGUAGE "Slovenian"
	!insertmacro MUI_LANGUAGE "Serbian"
	!insertmacro MUI_LANGUAGE "SerbianLatin"
	!insertmacro MUI_LANGUAGE "Arabic"
	!insertmacro MUI_LANGUAGE "Farsi"
	!insertmacro MUI_LANGUAGE "Hebrew"
	!insertmacro MUI_LANGUAGE "Indonesian"
	!insertmacro MUI_LANGUAGE "Mongolian"
	!insertmacro MUI_LANGUAGE "Luxembourgish"
	!insertmacro MUI_LANGUAGE "Albanian"
	!insertmacro MUI_LANGUAGE "Breton"
	!insertmacro MUI_LANGUAGE "Belarusian"
	!insertmacro MUI_LANGUAGE "Icelandic"
	!insertmacro MUI_LANGUAGE "Malay"
	!insertmacro MUI_LANGUAGE "Bosnian"
	!insertmacro MUI_LANGUAGE "Kurdish"
	!insertmacro MUI_LANGUAGE "Irish"
	!insertmacro MUI_LANGUAGE "Uzbek"
	!insertmacro MUI_LANGUAGE "Galician"
	!insertmacro MUI_LANGUAGE "Afrikaans"
	!insertmacro MUI_LANGUAGE "Catalan"
	!insertmacro MUI_LANGUAGE "Esperanto"

;--------------------------------
;Reserve Files
  
	;If you are using solid compression, files that are required before
	;the actual installation should be stored first in the data block,
	;because this will make your installer start faster.

	!insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
;Installer Sections

Section ""

	SetOutPath "$INSTDIR"  
	;ADD YOUR OWN FILES HERE...
	File /nonfatal /r "files\*"
	;Store installation folder
	WriteRegStr HKCU "Software\${SOFT_NAME}" "" $INSTDIR
	; write uninstall strings
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" "DisplayName" "${SOFT_NAME} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	; use defaults for parameters, icon, etc.
	CreateDirectory "$SMPROGRAMS\${SOFT_NAME}"
	CreateShortCut "$SMPROGRAMS\${SOFT_NAME}\QET.lnk" "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\ico\qelectrotech.ico"
	CreateShortCut "$DESKTOP\QET.lnk" "$INSTDIR\Lancer QET.bat" 0 "$INSTDIR\ico\qelectrotech.ico"
	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"
  
SectionEnd

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
	;  !insertmacro MUI_DESCRIPTION_TEXT ${CopyFiles} "CopyFiles"
	;!insertmacro MUI_FUNCTION_DESCRIPTION_END
 
;--------------------------------
;Uninstaller Section

Section "Uninstall"

	;remove start menu shortcuts
	Delete "$SMPROGRAMS\${SOFT_NAME}\QET.lnk"
	Delete "$DESKTOP\QET.lnk"
	RMDir "$SMPROGRAMS\${SOFT_NAME}"

	;ADD YOUR OWN FILES HERE...
	Delete "$INSTDIR\*.*"
	RMDir /r "$INSTDIR"

	;remove installation registary keys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SOFT_NAME}"
	DeleteRegKey /ifempty HKCU "Software\${SOFT_NAME}"

	IfFileExists "$INSTDIR" 0 NoErrorMsg
	MessageBox MB_OK "Note: $INSTDIR could not be removed!" IDOK 0 ; skipped if file doesn't exist
	NoErrorMsg:
  
SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

	!insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd
