#!/bin/bash

# configuration
QET_EXE="../qelectrotech"
QET_ELEMENTS_DIR="../elements/"
QET_CONFIG_DIR=""
QET_LANG_DIR="../lang/"
REDEFINE_LANG=""

# checks for the qelectrotech binary executable file
if [ ! -x "${QET_EXE}" ]; then
	echo "Errror : executable file not found. Aborting."
	exit 1
fi

# checks options that will be given to QET
OPTIONS=""

if [ -d "${QET_ELEMENTS_DIR}" ]; then
	OPTIONS="${OPTIONS} --common-elements-dir=${QET_ELEMENTS_DIR}"
fi

if [ -d "${QET_CONFIG_DIR}" ]; then
	OPTIONS="${OPTIONS} --config-dir=${QET_CONFIG_DIR}"
fi

if [ -d "${QET_LANG_DIR}" ]; then
	OPTIONS="${OPTIONS} --lang-dir=${QET_LANG_DIR}"
fi

if [ "${REDEFINE_LANG}" ]; then
	export LANG="${REDEFINE_LANG}"
fi

# launching
${QET_EXE} ${OPTIONS} $*
