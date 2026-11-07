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

# Packages of what "cmake --install" installs, made by CPack. Nothing here
# applies until CPACK_GENERATOR names the formats at the configure step.

if(NOT CPACK_GENERATOR)
  return()
endif()

foreach(generator ${CPACK_GENERATOR})
  if(NOT generator MATCHES "^(ZIP|TGZ)$"
     AND NOT (WIN32 AND generator STREQUAL "WIX"))
    message(FATAL_ERROR
      "CPACK_GENERATOR: ${generator} is not supported, "
      "use ZIP, TGZ or, on Windows, WIX.")
  endif()
endforeach()

set(CPACK_PACKAGE_VENDOR "The QElectroTech Team")
# QET's own component, without the headers, static libraries and translations
# that the fetched KDE libraries install (see CMakeLists.txt).
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};qelectrotech;/")

if("WIX" IN_LIST CPACK_GENERATOR)
  # WiX Toolset 4 or newer, the ones with a single "wix" program
  if(CMAKE_VERSION VERSION_LESS 3.30)
    message(FATAL_ERROR
      "CPACK_GENERATOR: WIX needs CMake 3.30, this is ${CMAKE_VERSION}.")
  endif()
  set(CPACK_WIX_VERSION 4)
  # The one of the official installer (build-aux/windows/QElectroTech.wxs), so
  # that the two never land in the same folder: the one with the higher
  # version replaces the other, and the lower one refuses to install
  set(CPACK_WIX_UPGRADE_GUID "A1B2C3D4-E5F6-7890-ABCD-EF1234567890")
  # Windows Installer skips a file when the installed one has a higher version,
  # then removes that one with the old install: the program, built with an
  # older Qt than the old install, would be left without Qt's DLLs
  set(CPACK_WIX_PROPERTY_REINSTALLMODE "amus")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "QElectroTech")
  set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/wix.cmake")
  # The icon of the official installers, also given to the shortcuts
  set(nsis_icons "${PROJECT_SOURCE_DIR}/build-aux/windows/nsis_base/files/ico")
  set(CPACK_WIX_PRODUCT_ICON "${nsis_icons}/qelectrotech.ico")
  # The shortcuts, in the start menu itself and on the desktop, and the file
  # types Windows opens with QElectroTech. The shortcuts are there and not
  # from CPACK_PACKAGE_EXECUTABLES, which can't give them an icon: the
  # program has none of its own.
  set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/wix-patch.xml")
  # CPack wants a known extension here, and the license has none
  configure_file("${PROJECT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_BINARY_DIR}/License.txt" COPYONLY)
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/License.txt")
endif()

include(CPack)
