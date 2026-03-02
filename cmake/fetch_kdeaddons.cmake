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

option(BUILD_KF6 "Build KF6 libraries, use system ones otherwise" OFF)
if(BUILD_KF6)
  block(PROPAGATE KF6_GIT_TAG)
    message(STATUS " - fetch_kdeaddons")
    set(KDE_SKIP_TEST_SETTINGS ON)
    set(KCOREADDONS_USE_QML OFF)
    set(KWIDGETSADDONS_USE_QML OFF)
    set(BUILD_TESTING OFF)
    set(BUILD_DESIGNERPLUGIN OFF)
    set(BUILD_QCH OFF)
    set(BUILD_SHARED_LIBS OFF)

    Include(FetchContent)

    if(NOT DEFINED KF6_GIT_TAG)
      set(KF6_GIT_TAG v6.22.0)
    endif()

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
  endblock()
else()
  find_package(KF6CoreAddons REQUIRED)
  find_package(KF6WidgetsAddons REQUIRED)
endif()

set(KF6_PRIVATE_LIBRARIES
  KF6::CoreAddons
  KF6::WidgetsAddons
)
