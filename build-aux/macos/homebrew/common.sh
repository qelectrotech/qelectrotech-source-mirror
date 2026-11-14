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

HOST_ARCH=$(uname -m)
echo "macOS host      : $(sw_vers -productName) $(sw_vers -productVersion) ($HOST_ARCH)"

export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8

if [[ "$QT_FROM" = "brew" ]] ; then

    # Homebrew bottles exist for the host architecture only, and Homebrew
    # no longer builds Intel ones.
    if [[ "$HOST_ARCH" != "arm64" ]] ; then
        echo "QT_FROM=brew needs an Apple Silicon Mac. Use QT_FROM=aqt on Intel."
        exit 1
    fi

    OSX_ARCHS="arm64"
    QT_PREFIX=$("$BREW" --prefix qtbase 2>/dev/null || true)
    SQLITE_PREFIX=$("$BREW" --prefix sqlite 2>/dev/null || true)

    # The Homebrew prefix holds Qt, hidapi and the other libraries.
    BUILD_PREFIX_PATH="$KF_PREFIX;$BREW_PREFIX"
    BUILD_IGNORE_PATH=""

elif [[ "$QT_FROM" = "aqt" ]] ; then

    QT_PREFIX="$QT_ROOT/$QT_VERSION/macos"
    SQLITE_PREFIX="$DEPS_PREFIX"

    # Only our universal libraries: anything found in the Homebrew prefix
    # would be arm64 only and break the x86_64 link. Homebrew still
    # provides cmake, ninja, git and ccache through the PATH.
    BUILD_PREFIX_PATH="$KF_PREFIX;$DEPS_PREFIX;$QT_PREFIX"
    BUILD_IGNORE_PATH="$BREW_PREFIX"
    export PKG_CONFIG_PATH=""
    export PKG_CONFIG_LIBDIR="$DEPS_PREFIX/lib/pkgconfig"

else

    echo "QT_FROM=$QT_FROM : unknown value (aqt or brew)."
    exit 1

fi

# Name of the architecture in the DMG file name
if [[ "$OSX_ARCHS" = *";"* ]] ; then
    PKG_ARCH="universal"
else
    PKG_ARCH="$OSX_ARCHS"
fi

# Same list with spaces, for lipo and the shell loops
OSX_ARCHS_LIST=$(echo "$OSX_ARCHS" | tr ';' ' ')

echo "Qt from         : $QT_FROM"
echo "Architectures   : $OSX_ARCHS_LIST ($PKG_ARCH)"

QMAKE="$QT_PREFIX/bin/qmake"

if [[ -x "$QMAKE" ]] ; then
    QT_PLUGINDIR=$("$QMAKE" -query QT_INSTALL_PLUGINS)
    QT_TRANSDIR=$("$QMAKE" -query QT_INSTALL_TRANSLATIONS)
    echo "Qt used         : $("$QMAKE" -query QT_VERSION) ($QT_PREFIX)"
fi

}

########################################################################
# Check that a Mach-O file holds every architecture of OSX_ARCHS.
# arg1 : file to check. Returns 1 and prints the missing ones otherwise.
ChecksArchs()
{

_archs=$(lipo -archs "$1" 2>/dev/null || true)
_missing=""

for _a in $OSX_ARCHS_LIST ; do
    case " $_archs " in
        *" $_a "*) ;;
        *) _missing="$_missing $_a" ;;
    esac
done

if [[ -n "$_missing" ]] ; then
    echo "   $1 : missing$_missing (has: ${_archs:-none})"
    return 1
fi

return 0

}

########################################################################
# Minimal macOS version a Mach-O file was built for (its "minos").
# arg1 : file. Prints the highest minos of its architectures.
MachoMinos()
{

otool -arch all -l "$1" 2>/dev/null | awk '/LC_BUILD_VERSION/{f=1} f && $1=="minos"{print $2; f=0}' \
    | sort -t . -k1,1n -k2,2n | tail -n 1

}
