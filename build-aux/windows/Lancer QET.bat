@echo off

rem Se rend dans le dossier qui convient
set current_dir=%~dp0
cd /d %current_dir%

rem lance QElectroTech
rem Sans option --config-dir, la configuration de QElectroTech seront dans
rem "C:/Users/<USER>/AppData/Local/QElectroTech/QElectroTech"
rem Sans l'option --data-dir, les données utilisateur (elements, titleblocks,
rem log, ...) de QElectroTech sont stockées dans
rem "C:/Users/<USER>/AppData/Roaming/QElectroTech/QElectroTech"
set command=bin\qelectrotech.exe --common-elements-dir=elements/ --common-tbt-dir=titleblocks/ --lang-dir=lang/ -style windowsvista %*
@start %command%
