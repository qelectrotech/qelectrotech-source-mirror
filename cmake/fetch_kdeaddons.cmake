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

message(" - fetch_kdeaddons")

if(BUILD_KF)
  message(STATUS "Building minimal KF libraries")

  if(NOT TARGET KF6::CoreAddons)
    message(
      VERBOSE
      "Building bundled minimal kcoreaddons")
    add_subdirectory(${QET_DIR}/thirdparty/kcoreaddons)
  else()
    message(
      VERBOSE
      "Target KF6::CoreAddons already exists, skipping build")
  endif()
  if(NOT TARGET KF6::WidgetsAddons)
    message(
      VERBOSE
      "Building bundled minimal kwidgetsaddons")
    add_subdirectory(${QET_DIR}/thirdparty/kwidgetsaddons)
  else()
    message(
      VERBOSE
      "Target KF6::KWidgetsAddons already exists, skipping build")
  endif()
else()
  message(STATUS "Using system KF libraries")

  find_package(KF6CoreAddons REQUIRED)
  find_package(KF6WidgetsAddons REQUIRED)
endif()

set(KF_PRIVATE_LIBRARIES
  KF6::WidgetsAddons
  KF6::CoreAddons
  )
