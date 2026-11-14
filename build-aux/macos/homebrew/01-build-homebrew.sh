#!/bin/bash

# Prepare a macOS host (Apple Silicon) to build the QElectroTech bundle:
# installs Homebrew if needed, then only the formulae QET needs.
# Runs as a normal user: the Homebrew installer asks for your password
# itself, once, to create /opt/homebrew.
#
# An existing Homebrew is kept as it is: this script never removes
# /opt/homebrew (the digiKam script offers to), it only adds formulae.
#
# Adapted from the digiKam Homebrew scripts by Gilles Caulier.
# Copyright (c) 2015      Shanti, <listaccount at revenant dot org>
# Copyright (c) 2015-2026, Gilles Caulier, <caulier dot gilles at gmail dot com>
# Copyright (c) 2026, The QElectroTech Team
#
# Redistribution and use is allowed according to the terms of the BSD license.

# Halt and catch errors
set -eE
trap 'PREVIOUS_COMMAND=$THIS_COMMAND; THIS_COMMAND=$BASH_COMMAND' DEBUG
trap 'echo "FAILED COMMAND: $PREVIOUS_COMMAND"' ERR

#################################################################################################
# Manage script traces to log file

mkdir -p ./logs
exec > >(tee ./logs/build-homebrew.full.log) 2>&1

#################################################################################################

echo "01-build-homebrew.sh : prepare Homebrew with the QElectroTech dependencies."
echo "---------------------------------------------------------------------------"

#################################################################################################
# Pre-processing checks

. ./config.sh
. ./common.sh
ChecksNotRoot
StartScript
ChecksXCodeCLI
ChecksCPUCores
ChecksHomebrew optional

mkdir -p "$WORK_DIR" "$BUILDING_DIR"

#################################################################################################

if [[ -z "$BREW" ]] ; then

    echo -e "---------- Install Homebrew\n"

    NONINTERACTIVE=1 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    ChecksHomebrew

    echo
    echo "To use brew in your shells, add to ~/.zprofile:"
    echo "    eval \"\$($BREW shellenv)\""

fi

export HOMEBREW_NO_INSTALL_CLEANUP=1

if [[ $BREW_UPDATE = 1 ]] ; then
    echo -e "---------- Update Homebrew\n"
    "$BREW" update
else
    export HOMEBREW_NO_AUTO_UPDATE=1
fi

#################################################################################################

echo -e "---------- Install build tools and libraries\n"

"$BREW" install $BREW_TOOLS $BREW_LIBS

echo -e "---------- Install Qt6 modules\n"

"$BREW" install $BREW_QT

for FORMULA in $BREW_QT_OPTIONAL ; do
    if "$BREW" info --formula "$FORMULA" >/dev/null 2>&1 ; then
        "$BREW" install "$FORMULA"
    else
        echo "Optional formula $FORMULA not available in Homebrew, skipped."
    fi
done

#################################################################################################

HostAdjustments

if [[ ! -x "$QMAKE" ]] ; then
    echo "ERROR: Qt6 not found after installation (brew --prefix qtbase)."
    exit 1
fi

echo -e "\n---------- Installed versions\n"
"$BREW" list --versions $BREW_TOOLS $BREW_LIBS $BREW_QT $BREW_QT_OPTIONAL 2>/dev/null || true

TerminateScript
