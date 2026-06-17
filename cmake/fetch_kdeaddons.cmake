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

# Accept the legacy flag name (BUILD_WITH_KF5) as well as the new one (BUILD_WITH_KF).
if(DEFINED BUILD_WITH_KF5 OR DEFINED BUILD_WITH_KF)

  include(FetchContent)

  # QT_VERSION_MAJOR may not be set yet if find_package(QT) hasn't run.
  # Do a quiet probe so we can choose KF5 vs KF6 correctly.
  if(NOT DEFINED QT_VERSION_MAJOR)
    find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core)
  endif()

  # Pick KDE Frameworks major version to match Qt.
  if(QT_VERSION_MAJOR EQUAL 6)
    set(KF_MAJOR_VERSION 6)
  else()
    set(KF_MAJOR_VERSION 5)
  endif()

  message(" - KDE Frameworks major version: ${KF_MAJOR_VERSION}")

  # ── KF6 path ───────────────────────────────────────────────────────────────
  if(KF_MAJOR_VERSION EQUAL 6)

    option(BUILD_KF "Build KF6 from source (FetchContent); use system packages otherwise" YES)

    if(BUILD_KF)
      if(NOT DEFINED KF6_GIT_TAG)
        set(KF6_GIT_TAG v6.3.0)
      endif()

      set(KDE_SKIP_TEST_SETTINGS "TRUE")
      set(BUILD_TESTING "0")

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

    set(KF_PRIVATE_LIBRARIES
      KF6::WidgetsAddons
      KF6::CoreAddons
    )

  # ── KF5 path (unchanged from original) ─────────────────────────────────────
  else()

    option(BUILD_KF5 "Build KF5 from source (FetchContent); use system packages otherwise" YES)

    if(BUILD_KF5)
      if(NOT DEFINED KF5_GIT_TAG)
        # https://qelectrotech.org/forum/viewtopic.php?pid=13924#p13924
        set(KF5_GIT_TAG v5.77.0)
      endif()

      # Fix: stop the run autotests of kcoreaddons
      # https://invent.kde.org/frameworks/kcoreaddons/-/blob/master/CMakeLists.txt#L98
      set(KDE_SKIP_TEST_SETTINGS "TRUE")
      set(BUILD_TESTING "0")

      FetchContent_Declare(
        ecm
        GIT_REPOSITORY https://invent.kde.org/frameworks/extra-cmake-modules.git
        GIT_TAG        ${KF5_GIT_TAG})
      FetchContent_MakeAvailable(ecm)

      FetchContent_Declare(
        kcoreaddons
        GIT_REPOSITORY https://invent.kde.org/frameworks/kcoreaddons.git
        GIT_TAG        ${KF5_GIT_TAG})
      FetchContent_MakeAvailable(kcoreaddons)

      FetchContent_Declare(
        kwidgetsaddons
        GIT_REPOSITORY https://invent.kde.org/frameworks/kwidgetsaddons.git
        GIT_TAG        ${KF5_GIT_TAG})
      FetchContent_MakeAvailable(kwidgetsaddons)

    else()
      find_package(KF5CoreAddons REQUIRED)
      find_package(KF5WidgetsAddons REQUIRED)
    endif()

    set(KF_PRIVATE_LIBRARIES
      KF5::WidgetsAddons
      KF5::CoreAddons
    )
    # Backward-compat alias used elsewhere in the original build system.
    set(KF5_PRIVATE_LIBRARIES ${KF_PRIVATE_LIBRARIES})

  endif()

endif()
