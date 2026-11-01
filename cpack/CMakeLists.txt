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

# Archives of what "cmake --install" installs, made by CPack. Nothing here
# applies until CPACK_GENERATOR names the formats at the configure step.

if(NOT CPACK_GENERATOR)
  return()
endif()

foreach(generator ${CPACK_GENERATOR})
  if(NOT generator MATCHES "^(ZIP|TGZ)$")
    message(FATAL_ERROR
      "CPACK_GENERATOR: ${generator} is not supported, use ZIP or TGZ.")
  endif()
endforeach()

set(CPACK_PACKAGE_VENDOR "The QElectroTech Team")
# QET's own component, without the headers, static libraries and translations
# that the fetched KDE libraries install (see CMakeLists.txt).
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};qelectrotech;/")

include(CPack)
