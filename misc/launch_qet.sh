#!/bin/bash

current_dir=$(dirname "$0")

# configuration
QET_EXE=$(readlink -f "${current_dir}/../qelectrotech")
QET_ELEMENTS_DIR=$(readlink -f "${current_dir}/../elements/")
QET_TBT_DIR=$(readlink -f "${current_dir}/../titleblocks/")
QET_CONFIG_DIR=""
QET_LANG_DIR=$(readlink -f "${current_dir}/../lang/")
# REDEFINE_LANG="es"

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

if [ -d "${QET_TBT_DIR}" ]; then
	OPTIONS="${OPTIONS} --common-tbt-dir=${QET_TBT_DIR}"
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
