@echo off
rem Enregistre les associations de fichiers de QElectroTech, a savoir les fichiers .qet

rem detecte le dossier courant et suppose que celui-ci contient les fichiers necessaires
set current_dir=%CD%
set expected_qet_exe=%current_dir%\bin\qelectrotech.exe
set expected_qet_ico=%current_dir%\ico\windows_icon\diagram_icon\qet-diagram.ico

rem verifie la presence du fichier qelectrotech.exe
if not exist %expected_qet_exe% (
	echo Le fichier %expected_qet_exe% n'a pas ete trouve. Abandon.
	pause
	exit /b 1
)

rem verifie la presence du fichier qet-diagram.ico
if not exist %expected_qet_ico% (
	echo Le fichier %expected_qet_ico% n'a pas ete trouve. Abandon.
	pause
	exit /b 1
)

rem echappe les backslashs dans les chemins absolus
set final_qet_exe=%expected_qet_exe:\=\\%
set final_qet_ico=%expected_qet_ico:\=\\%

rem genere le fichier .reg pour enregistrer les associations de fichiers
set reg_file=qet_install_file_associations.reg
(
	echo Windows Registry Editor Version 5.00
	echo.
	echo [HKEY_CLASSES_ROOT\Applications\qelectrotech.exe\shell\open\command]
	echo @="\"%final_qet_exe%\" \"%%1\""
	echo [HKEY_CLASSES_ROOT\.qet]
	echo @="qet_diagram_file"
	echo [HKEY_CLASSES_ROOT\qet_diagram_file]
	echo @="Schéma QET"
	echo "EditFlags"=dword:00000000
	echo "BrowserFlags"=dword:00000008
	echo [HKEY_CLASSES_ROOT\qet_diagram_file\DefaultIcon]
	echo @="%final_qet_ico%,0"
	echo [HKEY_CLASSES_ROOT\qet_diagram_file\shell\open\command]
	echo @="\"%final_qet_exe%\" \"%%1\""
) > %reg_file%

rem verifie que le fichier a bien ete ecrit
if not exist %reg_file% (
	echo Impossible de creer le fichier %reg_file%. Abandon.
	pause
	exit /b 1
)

rem Applique le .reg genere
regedit.exe /s %reg_file%
if errorlevel 1 (
	echo La prise en compte du fichier %reg_file% a echoue.
	pause
	exit /b 1
) else (
	echo Les associations de fichier ont bien ete crees.
	pause
	exit /b 0
)
