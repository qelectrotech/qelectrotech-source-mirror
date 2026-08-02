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
# When it is on, libspnav is looked for via its pkg-config file (present on
# every distro package of it this was checked against: Debian/Ubuntu's
# libspnav-dev ships /usr/share/pkgconfig/spnav.pc). If it isn't found, this
# does not hard-fail the whole configure -- it downgrades the option back to
# off with a clear message, so a developer who doesn't have the library
# installed still gets a normal build instead of a configure error for an
# opt-in feature they didn't ask to block on.
set(QET_SPACEMOUSE_ENABLED FALSE)

if(QET_ENABLE_SPACEMOUSE)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(SPNAV IMPORTED_TARGET spnav)
    endif()

    if(SPNAV_FOUND)
        set(QET_SPACEMOUSE_ENABLED TRUE)
        add_definitions(-DQET_SPACEMOUSE_SUPPORT)
        message("QET_ENABLE_SPACEMOUSE      ON  (libspnav ${SPNAV_VERSION} found)")
    else()
        message(WARNING "QET_ENABLE_SPACEMOUSE is ON but libspnav was not found via pkg-config "
                         "(install libspnav-dev, or the equivalent for your distribution) -- "
                         "building WITHOUT 3D mouse support.")
    endif()
endif()
