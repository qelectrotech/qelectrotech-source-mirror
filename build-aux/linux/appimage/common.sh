#!/bin/bash

# Common methods for the scripts that build the QElectroTech AppImage bundle.
#
# Adapted from the digiKam AppImage scripts by Gilles Caulier.
# Copyright (c) 2013-2019, Gilles Caulier, <caulier dot gilles at gmail dot com>
# Copyright (c) 2026, The QElectroTech Team
#
# Redistribution and use is allowed according to the terms of the BSD license.

########################################################################
# These scripts must NOT run as root: only apt in 01-build-host.sh uses sudo.
ChecksNotRoot()
{

if [[ $EUID -eq 0 ]]; then
    echo "Run this script as a normal user, not as root (sudo is used where needed)."
    exit 1
fi

}

########################################################################
# CPU cores to use
ChecksCPUCores()
{

CPU_CORES=$(nproc 2>/dev/null || grep -c ^processor /proc/cpuinfo)

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

# The AppImage only runs on systems with a glibc at least as recent as
# the build host's (2.39 on Ubuntu 24.04).
if ! grep -q 'VERSION_ID="24.04"' /etc/os-release ; then
    echo "Warning: these scripts are written for Ubuntu/Xubuntu 24.04."
fi

export LC_ALL=C.UTF-8
export LANG=C.UTF-8

ARCH=$(uname -m)

if [[ "$ARCH" != "x86_64" ]] ; then
    echo "Architecture $ARCH is not supported by these scripts (x86_64 only)."
    exit 1
fi

if [[ "$QT_FROM" = "system" ]] ; then
    QMAKE=/usr/lib/qt6/bin/qmake
    QET_WITH_KF=0
else
    QMAKE="$QT_ROOT/$QT_VERSION/gcc_64/bin/qmake"
fi

if [[ -x "$QMAKE" ]] ; then
    QT_PREFIX=$($QMAKE -query QT_INSTALL_PREFIX)
    QT_LIBDIR=$($QMAKE -query QT_INSTALL_LIBS)
    QT_PLUGINDIR=$($QMAKE -query QT_INSTALL_PLUGINS)
    QT_TRANSDIR=$($QMAKE -query QT_INSTALL_TRANSLATIONS)
    echo "Qt used        : $($QMAKE -query QT_VERSION) ($QT_PREFIX)"
fi

}

########################################################################
# Is this file an ELF binary or shared library ?
IsElf()
{

[[ -f "$1" ]] && head -c 4 "$1" 2>/dev/null | grep -q "ELF"

}

########################################################################
# Copy dependencies with ldd analysis
# arg1 : file to parse.
# arg2 : target path to copy dependencies
CopyReccursiveDependencies()
{

FILES=$(LD_LIBRARY_PATH="$QT_LIBDIR:$2" ldd "$1" 2>/dev/null | grep "=>" | awk '{print $3}')

for FILE in $FILES ; do
    if [[ -f "$FILE" ]] && [[ ! -e "$2/$(basename $FILE)" ]] ; then
        cp -L "$FILE" "$2/"
        echo "   ==> $(basename $FILE)"
    fi
done

if LD_LIBRARY_PATH="$QT_LIBDIR:$2" ldd "$1" 2>/dev/null | grep -q "not found" ; then
    echo "WARNING: unresolved dependencies for $1 :"
    LD_LIBRARY_PATH="$QT_LIBDIR:$2" ldd "$1" | grep "not found"
fi

}
