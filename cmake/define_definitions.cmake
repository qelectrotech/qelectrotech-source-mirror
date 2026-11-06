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

message(" - define_definitions")

# QET_RELATIVE_DATA_PATHS (cmake/start_options.cmake): the data paths start
# from the program's folder, so the installed files work wherever they are
# copied, a TGZ unpacked anywhere for one. The same paths, reached from
# bin/ instead of from the prefix.
if(QET_RELATIVE_DATA_PATHS)
  if(WIN32 OR APPLE)
    message(FATAL_ERROR
      "QET_RELATIVE_DATA_PATHS is for Linux and other Unix systems: on "
      "Windows and macOS the data paths are relative already.")
  endif()
  file(RELATIVE_PATH qet_bin_to_prefix
    "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_PREFIX}")
  string(REGEX REPLACE "/+$" "" qet_bin_to_prefix "${qet_bin_to_prefix}")
  set(COMPIL_PREFIX "${qet_bin_to_prefix}/")
  set(QET_COMMON_COLLECTION_PATH_RELATIVE_TO_BINARY_PATH ON)
  set(QET_COMMON_TBT_PATH_RELATIVE_TO_BINARY_PATH ON)
  set(QET_LANG_PATH_RELATIVE_TO_BINARY_PATH ON)
endif()

message("Paths used for compilation and installation of QET")
message("--------------------------------------------------")
message("COMPIL_PREFIX              " ${COMPIL_PREFIX})
message("INSTALL_PREFIX             " ${INSTALL_PREFIX})
message("QET_BINARY_PATH            " ${QET_BINARY_PATH})

if(${QET_COMMON_COLLECTION_PATH} STRGREATER "")
  message("QET_COMMON_COLLECTION_PATH " ${COMPIL_PREFIX}${QET_COMMON_COLLECTION_PATH})
  add_definitions(-DQET_COMMON_COLLECTION_PATH=${COMPIL_PREFIX}${QET_COMMON_COLLECTION_PATH})
endif()
if(${QET_COMMON_COLLECTION_PATH_RELATIVE_TO_BINARY_PATH})
  add_definitions(-DQET_COMMON_COLLECTION_PATH_RELATIVE_TO_BINARY_PATH)
endif()
if(${QET_COMMON_TBT_PATH} STRGREATER "")
  message("QET_COMMON_TBT_PATH        " ${COMPIL_PREFIX}${QET_COMMON_TBT_PATH})
  add_definitions(-DQET_COMMON_TBT_PATH=${COMPIL_PREFIX}${QET_COMMON_TBT_PATH})
endif()
if(${QET_COMMON_TBT_PATH_RELATIVE_TO_BINARY_PATH})
  add_definitions(-DQET_COMMON_TBT_PATH_RELATIVE_TO_BINARY_PATH)
endif()
if(${QET_LANG_PATH_RELATIVE_TO_BINARY_PATH})
  add_definitions(-DQET_LANG_PATH_RELATIVE_TO_BINARY_PATH)
endif()
if(${QET_LANG_PATH} STRGREATER "")
  message("QET_LANG_PATH              " ${COMPIL_PREFIX}${QET_LANG_PATH})
  add_definitions(-DQET_LANG_PATH=${COMPIL_PREFIX}${QET_LANG_PATH})
endif()
if (NOT MINGW)
if(${QET_EXAMPLES_PATH} STRGREATER "")
  message("QET_EXAMPLES_PATH          " ${COMPIL_PREFIX}${QET_EXAMPLES_PATH})
  add_definitions(-DQET_EXAMPLES_PATH=${COMPIL_PREFIX}${QET_EXAMPLES_PATH})
endif()
endif()

message("QET_LICENSE_PATH           " ${QET_LICENSE_PATH})
message("QET_MIME_PACKAGE_PATH      " ${QET_MIME_PACKAGE_PATH})
message("QET_DESKTOP_PATH           " ${QET_DESKTOP_PATH})
message("QET_ICONS_PATH             " ${QET_ICONS_PATH})
message("QET_MAN_PATH               " ${QET_MAN_PATH})
message("QET_APPDATA_PATH           " ${QET_APPDATA_PATH})
message("ICON                       " ${ICON})
message("--------------------------------------------------")
message("PROJECT_NAME              :" ${PROJECT_NAME})
message("PROJECT_VERSION           :" ${PROJECT_VERSION})
message("PROJECT_DESCRIPTION       :" ${PROJECT_DESCRIPTION})
message("PROJECT_HOMEPAGE_URL      :" ${PROJECT_HOMEPAGE_URL})
message("PROJECT_SOURCE_DIR        :" ${PROJECT_SOURCE_DIR})
message("QET_DIR                   :" ${QET_DIR})
message("GIT_COMMIT_SHA            :" ${GIT_COMMIT_SHA})

if(BUILD_WITH_KF)
  message("KF_GIT_TAG                :" ${KF_GIT_TAG})
else()
  add_definitions(-DBUILD_WITHOUT_KF)
endif()
message("QET_COMPONENTS            :" ${QET_COMPONENTS})
