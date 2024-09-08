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

if(DEFINED BUILD_WITH_KF6)
  Include(FetchContent)

  option(BUILD_KF6 "Build KF6 libraries, use system ones otherwise" NO)

  if(BUILD_KF6)

    if(NOT DEFINED KF6_GIT_TAG)
      set(KF6_GIT_TAG v6.5.0)
    endif()

    # 
    set(BUILD_TESTING OFF)
    FetchContent_Declare(
      ecm
      GIT_REPOSITORY https://invent.kde.org/frameworks/extra-cmake-modules.git
      GIT_TAG        ${KF6_GIT_TAG})
    FetchContent_MakeAvailable(ecm)

    FetchContent_Declare(
      kcoreaddons
      GIT_REPOSITORY https://invent.kde.org/frameworks/kcoreaddons.git
      GIT_TAG        ${KF6_GIT_TAG})
    FetchContent_MakeAvailable(kcoreaddons)

    FetchContent_Declare(
      kwidgetsaddons
      GIT_REPOSITORY https://invent.kde.org/frameworks/kwidgetsaddons.git
      GIT_TAG        ${KF6_GIT_TAG})
    FetchContent_MakeAvailable(kwidgetsaddons)
  else()
    find_package(KF6CoreAddons REQUIRED)
    find_package(KF6WidgetsAddons REQUIRED)
  endif()

  set(KF6_PRIVATE_LIBRARIES
    KF6::WidgetsAddons
    KF6::CoreAddons
    )
endif()
