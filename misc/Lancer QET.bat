@echo off

rem Se rend dans le dossier qui convient
set current_dir=%~dp0
cd /d %current_dir%

rem Met la collection QET en lecture seule
attrib +r elements/* /S /D

rem lance QElectroTech
set command=bin\qelectrotech.exe --common-elements-dir=elements/ --common-tbt-dir=titleblocks/ --lang-dir=lang/ --config-dir=conf/ -style plastique %*
@start %command%
