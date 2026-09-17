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

add_library(kcoreaddons STATIC
  src/kautosavefile.cpp
  src/krandom.cpp
)

add_library(KF6::CoreAddons ALIAS kcoreaddons)

target_include_directories(kcoreaddons
  PUBLIC
    include
)

target_link_libraries(kcoreaddons PUBLIC Qt6::Core)

set_target_properties(kcoreaddons PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
