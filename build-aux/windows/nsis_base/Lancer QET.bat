@echo off

rem Se rend dans le dossier qui convient
set current_dir=%~dp0
cd /d %current_dir%

rem lance QElectroTech
rem Sans option --config-dir, la configuration de QElectroTech ainsi que la
rem collection d'elements perso seront dans "%APPDATA%\qet"
set command=bin\qelectrotech.exe -platform windows:fontengine=freetype --common-elements-dir=elements/ --common-tbt-dir=titleblocks/ --lang-dir=lang/ %*
@start %command%
