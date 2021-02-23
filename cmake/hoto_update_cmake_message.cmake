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

if(${CMAKE_VERSION} VERSION_LESS 3.14)
  message(
    "_____________________________________________________________________")
  message("to update Cmake on linux:")
  message("https://github.com/Kitware/CMake/")
  message("linux => cmake-3.19.1-Linux-x86_64.sh")
  message("         sudo ./cmake.sh --prefix=/usr/local/ --exclude-subdir")
  message("windows good luck :)")
endif()
