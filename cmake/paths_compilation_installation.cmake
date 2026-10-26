# Copyright 2006-2026 The QElectroTech Team
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

# Paths used for compilation and installation of QET

message(" - paths_compilation_installation")

include(GNUInstallDirs)

if(UNIX AND NOT APPLE)
  # for Linux, BSD, Solaris, Minix
  # The paths below are relative to the install prefix: COMPIL_PREFIX is
  # put in front of them for the paths compiled into QET. An absolute
  # CMAKE_INSTALL_<dir> is accepted too, made relative to the prefix here.
  file(RELATIVE_PATH QET_BINDIR      "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_BINDIR}")
  file(RELATIVE_PATH QET_DATADIR     "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_DATADIR}")
  file(RELATIVE_PATH QET_DATAROOTDIR "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_DATAROOTDIR}")
  file(RELATIVE_PATH QET_DOCDIR      "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_DOCDIR}")
  file(RELATIVE_PATH QET_MANDIR      "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_MANDIR}")
  set(COMPIL_PREFIX               "${CMAKE_INSTALL_PREFIX}/")
  set(INSTALL_PREFIX              "${CMAKE_INSTALL_PREFIX}/")
  set(QET_BINARY_PATH             "${QET_BINDIR}/")
  set(QET_DATA_PATH               "${QET_DATADIR}/qelectrotech/")
  set(QET_COMMON_COLLECTION_PATH  "${QET_DATA_PATH}elements/")
  set(QET_COMMON_TBT_PATH         "${QET_DATA_PATH}titleblocks/")
  set(QET_LANG_PATH               "${QET_DATA_PATH}lang/")
  set(QET_EXAMPLES_PATH           "${QET_DATA_PATH}examples/")
  set(QET_LICENSE_PATH            "${QET_DOCDIR}/")
  set(QET_MIME_PACKAGE_PATH       "${QET_DATAROOTDIR}/mime/packages/")
  set(QET_DESKTOP_PATH            "${QET_DATAROOTDIR}/applications/")
  set(QET_ICONS_PATH              "${QET_DATAROOTDIR}/icons/hicolor/")
  set(QET_MAN_PATH                "${QET_MANDIR}/")
  set(QET_APPDATA_PATH            "${QET_DATAROOTDIR}/appdata")
endif()

if(APPLE)
  # for MacOS X or iOS, watchOS, tvOS (since 3.10.3)
  set(COMPIL_PREFIX               "./")
  set(INSTALL_PREFIX              "/usr/local/")
  set(QET_BINARY_PATH             "bin/")
  set(QET_COMMON_COLLECTION_PATH  "../Resources/elements/")
  set(QET_COMMON_TBT_PATH         "../Resources/titleblocks/")
  set(QET_LANG_PATH               "../Resources/lang/")
  set(QET_DATA_PATH               "share/qelectrotech/")
  set(QET_EXAMPLES_PATH           "share/qelectrotech/examples/")
  set(QET_LICENSE_PATH            "share/doc/qelectrotech/")
  set(QET_DESKTOP_PATH            "share/applications/")
  set(QET_ICONS_PATH              "share/icons/hicolor/")
  set(QET_MAN_PATH                "man/")
  set(ICON                        "ico/mac_icon/qelectrotech.icns")
endif()

if(WIN32)
  # for Windows operating system in general
  set(COMPIL_PREFIX               "./")
  set(INSTALL_PREFIX              "./")
  set(QET_BINARY_PATH             "bin/")
  set(QET_COMMON_COLLECTION_PATH  "elements/")
  set(QET_COMMON_TBT_PATH         "titleblocks/")
  # "lang/" and not "l10n/": that is where every Windows packaging actually
  # puts the .qm files (see build-aux/windows/QElectroTech.wxs and the
  # windows-build workflow), and what the shortcuts pass as --lang-dir.
  set(QET_LANG_PATH               "lang/")
  set(QET_EXAMPLES_PATH           "examples/")
  # "cmake --install" lays the files out as the Windows installers and the
  # portable folder do: the program in bin/, the data and the documents
  # beside it, where QET and the MCP server (misc/qet-mcp) look for them.
  set(QET_DATA_PATH               "./")
  set(QET_LICENSE_PATH            "./")
  set(QET_ICONS_PATH              "icons/hicolor/")
  # Liste des ressources Windows
#RC_FILE = qelectrotech.rc
endif()
