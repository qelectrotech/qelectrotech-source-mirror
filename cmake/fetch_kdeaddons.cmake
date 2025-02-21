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

message(STATUS "fetch_kdeaddons")

if(BUILD_WITH_KF6)
  option(BUILD_KF6 "Build KF6 libraries, use system ones otherwise" OFF)
  block(PROPAGATE KF6_GIT_TAG)
  set(BUILD_TESTING OFF)
  set(KDE_SKIP_TEST_SETTINGS ON)
  set(BUILD_DESIGNERPLUGIN OFF)
  set(KCOREADDONS_USE_QML OFF)
  set(BUILD_QCH OFF)
  set(BUILD_SHARED_LIBS OFF)
  find_package(ECM 6.8.0 REQUIRED NO_MODULE)
  list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

  if(BUILD_KF6)
    Include(FetchContent)

    if(NOT DEFINED KF6_GIT_TAG)
      set(KF6_GIT_TAG v6.8.0)
    endif()

    FetchContent_Declare(
      kcoreaddons
      GIT_REPOSITORY https://invent.kde.org/frameworks/kcoreaddons.git
      GIT_TAG        ${KF6_GIT_TAG})
    FetchContent_MakeAvailable(kcoreaddons)
    get_target_property(kca_version KF6::CoreAddons VERSION)

    FetchContent_Declare(
      kwidgetsaddons
      GIT_REPOSITORY https://invent.kde.org/frameworks/kwidgetsaddons.git
      GIT_TAG        ${KF6_GIT_TAG})
    FetchContent_MakeAvailable(kwidgetsaddons)
    get_target_property(kwa_version KF6::WidgetsAddons VERSION)
  else()
    find_package(KF6CoreAddons REQUIRED)
    set(kca_version ${KF6CoreAddons_VERSION})
    find_package(KF6WidgetsAddons REQUIRED)
    set(kwa_version ${KF6WidgetsAddons_VERSION})
  endif()

  get_target_property(kca_type KF6::WidgetsAddons TYPE)
  get_target_property(kwa_type KF6::CoreAddons TYPE)

  message(NOTICE "ecm version            : " ${ECM_VERSION})
  message(NOTICE "kcoreaddons library    : " ${kca_type})
  message(NOTICE "kcoreaddons version    : " ${kca_version})
  message(NOTICE "kwidgetsaddons library : " ${kwa_type})
  message(NOTICE "kwidgetsaddons version : " ${kwa_version})
  endblock()

  set(KF6_PRIVATE_LIBRARIES
    KF6::WidgetsAddons
    KF6::CoreAddons
    )
endif()
