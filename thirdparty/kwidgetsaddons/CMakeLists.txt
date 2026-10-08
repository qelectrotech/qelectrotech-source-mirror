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

# Match the top-level policy range: this file is only ever pulled in via
# add_subdirectory(), and an old-style cmake_minimum_required() here would
# reset all policies, which Qt 6.10's qt_add_library() machinery cannot
# work with (CMP0071 warnings, then a configure error).
cmake_minimum_required(VERSION 3.5...4.2)

# Plain add_library(): these are private static libraries linked into the
# QET binary only, so qt_add_library()'s metatype extraction buys nothing.
# AUTOMOC from the including scope is all the vendored sources need.
add_library(kwidgetsaddons STATIC
  src/kabstractcoloredit.cpp
  src/kcolorbutton.cpp
  src/kcolorcombo.cpp
  src/kcolormimedata.cpp
)

add_library(KF6::WidgetsAddons ALIAS kwidgetsaddons)

target_include_directories(kwidgetsaddons
  PUBLIC
    include
)

target_link_libraries(kwidgetsaddons PUBLIC Qt6::Widgets)

set_target_properties(kwidgetsaddons PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
