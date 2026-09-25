# Copyright 2006 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.

message(" - find_spacemouse")

# QET_ENABLE_SPACEMOUSE (cmake/developer_options.cmake) is off by default,
# so none of this runs, and the default build is entirely unaffected: no new
# dependency, no new source files, no new symbols.
#
# When it is on, QET_SPACEMOUSE_BACKEND picks how the device is read -- see
# sources/spacemouse/spacemousebackend.h for what a backend is:
#   spnav  Linux, through the spacenavd daemon (libspnav)
#   hid    any platform, directly over USB (hidapi), no 3Dconnexion driver
#   auto   spnav on Linux when libspnav is found, hid otherwise
# On macOS the 3DxWare backend is added to whichever of these is chosen.
# A backend whose library is not found downgrades the option to off with a
# warning, rather than failing configure for an opt-in feature.
set(QET_SPACEMOUSE_BACKEND "auto" CACHE STRING "3D mouse backend: auto, spnav or hid")
set_property(CACHE QET_SPACEMOUSE_BACKEND PROPERTY STRINGS auto spnav hid)

set(QET_SPACEMOUSE_ENABLED FALSE)
set(QET_SPACEMOUSE_BACKEND_SPNAV_ENABLED FALSE)
set(QET_SPACEMOUSE_BACKEND_HID_ENABLED FALSE)
set(QET_SPACEMOUSE_BACKEND_CONNEXION_ENABLED FALSE)

if(QET_ENABLE_SPACEMOUSE)
    find_package(PkgConfig)

    set(_qet_spacemouse_try_spnav FALSE)
    set(_qet_spacemouse_try_hid FALSE)
    if(QET_SPACEMOUSE_BACKEND STREQUAL "spnav")
        set(_qet_spacemouse_try_spnav TRUE)
    elseif(QET_SPACEMOUSE_BACKEND STREQUAL "hid")
        set(_qet_spacemouse_try_hid TRUE)
    else()
        if(UNIX AND NOT APPLE)
            set(_qet_spacemouse_try_spnav TRUE)
        endif()
        set(_qet_spacemouse_try_hid TRUE)
    endif()

    # libspnav: Debian/Ubuntu's libspnav-dev ships spnav.pc.
    if(_qet_spacemouse_try_spnav AND PkgConfig_FOUND)
        pkg_check_modules(SPNAV IMPORTED_TARGET spnav)
        if(SPNAV_FOUND)
            set(QET_SPACEMOUSE_ENABLED TRUE)
            set(QET_SPACEMOUSE_BACKEND_SPNAV_ENABLED TRUE)
            add_definitions(-DQET_SPACEMOUSE_BACKEND_SPNAV)
            message("QET_ENABLE_SPACEMOUSE      ON  (backend: libspnav ${SPNAV_VERSION})")
        endif()
    endif()

    # hidapi: hidapi-hidraw.pc on Linux (libhidapi-dev), hidapi.pc from
    # MSYS2 (mingw-w64-ucrt-x86_64-hidapi) and Homebrew (hidapi).
    if(NOT QET_SPACEMOUSE_ENABLED AND _qet_spacemouse_try_hid AND PkgConfig_FOUND)
        pkg_search_module(HIDAPI IMPORTED_TARGET hidapi-hidraw hidapi)
        if(HIDAPI_FOUND)
            set(QET_SPACEMOUSE_ENABLED TRUE)
            set(QET_SPACEMOUSE_BACKEND_HID_ENABLED TRUE)
            add_definitions(-DQET_SPACEMOUSE_BACKEND_HID)
            message("QET_ENABLE_SPACEMOUSE      ON  (backend: hidapi ${HIDAPI_VERSION})")
        endif()
    endif()

    # macOS: with 3DxWare installed the device can only be read through
    # 3DxWare, so its backend comes too, whichever backend was asked for.
    # It loads 3DxWare's library at run time and needs nothing to build.
    if(APPLE)
        set(QET_SPACEMOUSE_ENABLED TRUE)
        set(QET_SPACEMOUSE_BACKEND_CONNEXION_ENABLED TRUE)
        add_definitions(-DQET_SPACEMOUSE_BACKEND_CONNEXION)
        message("QET_ENABLE_SPACEMOUSE      ON  (backend: 3DxWare when installed)")
    endif()

    if(QET_SPACEMOUSE_ENABLED)
        add_definitions(-DQET_SPACEMOUSE_SUPPORT)
    else()
        message(WARNING "QET_ENABLE_SPACEMOUSE is ON but no library was found for the "
                         "'${QET_SPACEMOUSE_BACKEND}' backend (libspnav-dev for spnav, "
                         "hidapi for hid, via pkg-config) -- building WITHOUT 3D mouse support.")
    endif()
endif()
