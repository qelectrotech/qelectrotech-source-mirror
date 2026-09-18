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

message(" - sqlite3")

# Use the bundled SQLite3 amalgamation (thirdparty/sqlite3/sqlite3.c/h)
# instead of the system-provided SQLite3 found via find_package.
#
# This is useful when the system SQLite3 is too old or unavailable, or
# when a fully self-contained, reproducible build is desired.
option(QET_USE_BUNDLED_SQLITE3 "\
Use the bundled SQLite3 amalgamation from thirdparty/sqlite3" OFF)

if(QET_USE_BUNDLED_SQLITE3)
    message(STATUS "Using bundled SQLite3 amalgamation")
    add_subdirectory(${QET_DIR}/thirdparty/sqlite3)
else()
    message(STATUS "Using system SQLite3")
    find_package(SQLite3 REQUIRED)

    # CMake < 4.3 only creates the SQLite::SQLite3 target (no SQLite3::SQLite3
    # alias yet), while CMake >= 4.3's bundled FindSQLite3 creates
    # SQLite3::SQLite3 and deprecates the old name. Add the missing alias
    # ourselves so we can use the modern target name everywhere regardless of
    # the CMake version in use (this project must keep building on CMake
    # versions below 4.3, e.g. on most current Linux distros).
    if(NOT TARGET SQLite3::SQLite3 AND TARGET SQLite::SQLite3)
        add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
    endif()
endif()
