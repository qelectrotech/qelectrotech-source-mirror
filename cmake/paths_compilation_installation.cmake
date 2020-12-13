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

# Paths used for compilation and installation of QET

message(" - paths_compilation_installation")

if(UNIX AND NOT APPLE)
  # for Linux, BSD, Solaris, Minix
  set(COMPIL_PREFIX               "/usr/local/")
  set(INSTALL_PREFIX              "/usr/local/")
  set(QET_BINARY_PATH             "bin/")
  set(QET_COMMON_COLLECTION_PATH  "share/qelectrotech/elements/")
  set(QET_COMMON_TBT_PATH         "share/qelectrotech/titleblocks/")
  set(QET_LANG_PATH               "share/qelectrotech/lang/")
  set(QET_EXAMPLES_PATH           "share/qelectrotech/examples/")
  set(QET_LICENSE_PATH            "doc/qelectrotech/")
  set(QET_MIME_XML_PATH           "../share/mime/application/")
  set(QET_MIME_DESKTOP_PATH       "../share/mimelnk/application/")
  set(QET_MIME_PACKAGE_PATH       "../share/mime/packages/")
  set(QET_DESKTOP_PATH            "share/applications/")
  set(QET_ICONS_PATH              "share/icons/hicolor/")
  set(QET_MAN_PATH                "man/")
  set(QET_APPDATA_PATH            "share/appdata")
endif()

if(APPLE)
  # for MacOS X or iOS, watchOS, tvOS (since 3.10.3)
  set(COMPIL_PREFIX               "./")
  set(INSTALL_PREFIX              "/usr/local/")
  set(QET_BINARY_PATH             "bin/")
  set(QET_COMMON_COLLECTION_PATH  "../Resources/elements/")
  set(QET_COMMON_TBT_PATH         "../Resources/titleblocks/")
  set(QET_LANG_PATH               "../Resources/lang/")
  set(QET_EXAMPLES_PATH           "share/qelectrotech/examples/")
  set(QET_LICENSE_PATH            "doc/qelectrotech/")
  set(QET_MIME_XML_PATH           "../share/mime/application/")
  set(QET_MIME_DESKTOP_PATH       "../share/mimelnk/application/")
  set(QET_DESKTOP_PATH            "share/applications/")
  set(QET_ICONS_PATH              "share/icons/hicolor/")
  set(QET_MAN_PATH                "man/")
  set(ICON                        "ico/mac_icon/qelectrotech.icns")
endif()

if(WIN32)
  # for Windows operating system in general
  set(COMPIL_PREFIX               "./")
  set(INSTALL_PREFIX              "./")
  set(QET_BINARY_PATH             "./")
  set(QET_COMMON_COLLECTION_PATH  "elements/")
  set(QET_COMMON_TBT_PATH         "titleblocks/")
  set(QET_LANG_PATH               "l10n/")
  set(QET_LICENSE_PATH            "./")
  # Liste des ressources Windows
#RC_FILE = qelectrotech.rc
endif()
