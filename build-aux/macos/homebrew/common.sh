#!/bin/bash

# Common methods for the scripts that build the QElectroTech macOS bundle.
#
# Adapted from the digiKam Homebrew scripts by Gilles Caulier.
# Copyright (c) 2013-2026, Gilles Caulier, <caulier dot gilles at gmail dot com>
# Copyright (c) 2026, The QElectroTech Team
#
# Redistribution and use is allowed according to the terms of the BSD license.
#
# Note: macOS ships bash 3.2. No bash 4 features here (associative
# arrays, mapfile, ${var,,}...).

########################################################################
# These scripts must NOT run as root: Homebrew refuses it (the digiKam
# scripts still use sudo, which is why their 01 script fails now).
ChecksNotRoot()
{

if [[ $EUID -eq 0 ]]; then
    echo "Run this script as a normal user, not as root nor with sudo."
    echo "Homebrew refuses to run as root."
    exit 1
fi

}

########################################################################
# Xcode Command Line Tools (clang, codesign, notarytool, stapler, hdiutil)
ChecksXCodeCLI()
{

if ! xcode-select --print-path >/dev/null 2>&1 ; then
    echo "Xcode Command Line Tools are not installed. Run:"
    echo "    xcode-select --install"
    exit 1
fi

echo "Xcode CLI tools : $(xcode-select --print-path)"

export MACOSX_DEPLOYMENT_TARGET=$OSX_MIN_TARGET
echo "macOS target    : $MACOSX_DEPLOYMENT_TARGET"

}

########################################################################
# Find brew and set the Homebrew environment.
# arg1 : "optional" to only set BREW when brew is missing (01 script).
ChecksHomebrew()
{

BREW=""

for b in "$(command -v brew 2>/dev/null)" /opt/homebrew/bin/brew /usr/local/bin/brew ; do
    if [[ -n "$b" ]] && [[ -x "$b" ]] ; then
        BREW="$b"
        break
    fi
done

if [[ -z "$BREW" ]] ; then
    if [[ "$1" = "optional" ]] ; then
        return
    fi
    echo "Homebrew is not installed: run 01-build-homebrew.sh first."
    exit 1
fi

eval "$("$BREW" shellenv)"
BREW_PREFIX="$("$BREW" --prefix)"
echo "Homebrew        : $BREW_PREFIX ($("$BREW" --version | head -n 1))"

}

########################################################################
# CPU cores to use
ChecksCPUCores()
{

CPU_CORES=$(sysctl -n hw.ncpu)

if [[ $CPU_CORES -gt 1 ]]; then
    CPU_CORES=$((CPU_CORES-1))
fi

echo "CPU Cores to use : $CPU_CORES"

}

########################################################################
# Time execution measurement : startup
StartScript()
{

BEGIN_SCRIPT=$(date +"%s")

}

########################################################################
# Time execution measurement : shutdown
TerminateScript()
{

TERMIN_SCRIPT=$(date +"%s")
difftimelps=$(($TERMIN_SCRIPT-$BEGIN_SCRIPT))
echo "Elapsed time for script execution : $(($difftimelps / 3600 )) hours $((($difftimelps % 3600) / 60)) minutes $(($difftimelps % 60)) seconds"

}

########################################################################
# Check the host and set the Qt variables
HostAdjustments()
{

echo "macOS host      : $(sw_vers -productName) $(sw_vers -productVersion) ($(uname -m))"

if [[ "$ARCH_TARGET" != "arm64" ]] ; then
    echo "Architecture $ARCH_TARGET is not supported by these scripts (arm64 only)."
    exit 1
fi

export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8

QT_PREFIX=$("$BREW" --prefix qtbase 2>/dev/null || true)
QMAKE="$QT_PREFIX/bin/qmake"

if [[ -x "$QMAKE" ]] ; then
    QT_PLUGINDIR=$("$QMAKE" -query QT_INSTALL_PLUGINS)
    QT_TRANSDIR=$("$QMAKE" -query QT_INSTALL_TRANSLATIONS)
    echo "Qt used         : $("$QMAKE" -query QT_VERSION) ($QT_PREFIX)"
fi

}
