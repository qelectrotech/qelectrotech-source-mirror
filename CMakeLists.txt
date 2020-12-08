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

cmake_minimum_required(VERSION 3.5)

message(
  "_____________________________________________________________________"
  "\nCopyright 2006 The QElectroTech Team"
  "\nThis file is part of QElectroTech."

  "\n\nQElectroTech is free software: you can redistribute it and/or modify"
  "\nit under the terms of the GNU General Public License as published by"
  "\nthe Free Software Foundation, either version 2 of the License, or"
  "\n(at your option) any later version."

  "\nQElectroTech is distributed in the hope that it will be useful,"
  "\nbut WITHOUT ANY WARRANTY; without even the implied warranty of"
  "\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
  "\nGNU General Public License for more details."

  "\n\nYou should have received a copy of the GNU General Public License"
  "\nalong with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.\n"
  "_____________________________________________________________________")
project(qelectrotech
  VERSION 0.8.0
  DESCRIPTION "QET is a CAD/CAE editor focusing on schematics drawing features."
  HOMEPAGE_URL "https://qelectrotech.org/"
  LANGUAGES CXX)

set(QET_DIR ${PROJECT_SOURCE_DIR})

message("PROJECT_NAME:       " ${PROJECT_NAME})
message("PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message("QET_DIR:            " ${QET_DIR})

# Add sub directories
add_subdirectory(tests)
