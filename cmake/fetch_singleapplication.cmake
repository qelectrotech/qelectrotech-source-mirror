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

message(" - fetch_singleapplication")

# https://github.com/itay-grudev/SingleApplication/issues/18
#qmake
#DEFINES += QAPPLICATION_CLASS=QGuiApplication
set(QAPPLICATION_CLASS QApplication)

Include(FetchContent)

if(EXISTS "${CMAKE_SOURCE_DIR}/SingleApplication/CMakeLists.txt")
  # Submodule deja present dans l'arbre source (clone --recursive, tarball
  # de distro deja peuple via "git submodule update", etc.) : on l'utilise
  # tel quel, sans acces reseau. Necessaire pour les builds hors-ligne
  # (pbuilder/sbuild avec FETCHCONTENT_FULLY_DISCONNECTED=ON, Launchpad PPA...).
  set(FETCHCONTENT_SOURCE_DIR_SINGLEAPPLICATION "${CMAKE_SOURCE_DIR}/SingleApplication")
endif()

# Pinned to the commit v3.2.0 points at, not to the tag itself; see the note in
# fetch_pugixml.cmake. v3.2.0 is a lightweight tag, a ref pointing straight at
# the commit, so "git ls-remote <repo> refs/tags/v3.2.0" prints that commit and
# nothing else. An annotated tag, as KDE uses in fetch_kdeaddons.cmake, would
# print the tag object under refs/tags/v3.2.0 as well, with the commit on the
# refs/tags/v3.2.0^{} line.
FetchContent_Declare(
  SingleApplication
  GIT_REPOSITORY https://github.com/itay-grudev/SingleApplication.git
  GIT_TAG        aede311d28d20179216c5419b581087be2a8409f) # v3.2.0
set(QT_DEFAULT_MAJOR_VERSION 6)
FetchContent_MakeAvailable(SingleApplication)
