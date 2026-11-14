#!/bin/bash

# Prepare a macOS host to build the QElectroTech bundle: installs Homebrew
# if needed, then only the formulae QET needs, and Qt6 from Homebrew
# (QT_FROM=brew) or the official universal binaries (QT_FROM=aqt).
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

echo -e "---------- Install build tools\n"

# Only the missing ones: git comes with the Xcode CLI tools, and CI runners
# already have cmake and ninja (installing them again with brew can fail
# on a link conflict).
for TOOL in $BREW_TOOLS ; do
    if command -v "$TOOL" >/dev/null 2>&1 ; then
        echo "$TOOL already installed: $(command -v "$TOOL")"
    else
        "$BREW" install "$TOOL"
    fi
done

HostAdjustments

if [[ "$QT_FROM" = "brew" ]] ; then

    echo -e "---------- Install libraries and Qt6 modules with Homebrew\n"

    "$BREW" install $BREW_LIBS $BREW_QT

    for FORMULA in $BREW_QT_OPTIONAL ; do
        if "$BREW" info --formula "$FORMULA" >/dev/null 2>&1 ; then
            "$BREW" install "$FORMULA"
        else
            echo "Optional formula $FORMULA not available in Homebrew, skipped."
        fi
    done

    QT_SOURCE_INFO="Homebrew: $("$BREW" list --versions $BREW_QT $BREW_QT_OPTIONAL 2>/dev/null | tr '\n' ' ')"

else

    echo -e "---------- Install Qt $QT_VERSION with aqtinstall\n"

    if [[ -x "$QT_PREFIX/bin/qmake" ]] ; then

        echo "Qt $QT_VERSION already installed in $QT_ROOT (cache), skipped."

    else

        # python3 of the Xcode Command Line Tools
        /usr/bin/python3 -m venv "$WORK_DIR/aqt-venv"
        "$WORK_DIR/aqt-venv/bin/pip" install --upgrade aqtinstall

        "$WORK_DIR/aqt-venv/bin/aqt" install-qt mac desktop "$QT_VERSION" clang_64 \
             -m $QT_AQT_MODULES \
             -O "$QT_ROOT"

    fi

    QT_SOURCE_INFO="aqtinstall: Qt $QT_VERSION ($QT_AQT_MODULES)"

fi

#################################################################################################

HostAdjustments

if [[ ! -x "$QMAKE" ]] ; then
    echo "ERROR: Qt6 not found after installation ($QT_PREFIX)."
    exit 1
fi

echo -e "\n---------- Installed versions\n"
for TOOL in $BREW_TOOLS ; do echo "$TOOL : $("$TOOL" --version 2>/dev/null | head -n 1)" ; done
echo "$QT_SOURCE_INFO"

TerminateScript
