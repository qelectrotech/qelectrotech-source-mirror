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

# TODO remove path as soon as Qt5 gets retired
if(BUILD_WITH_KF)
  Include(FetchContent)

  option(BUILD_KF "Build KF libraries, use system ones otherwise" YES)

  if(BUILD_KF)

    if(KF_MAJOR_VERSION EQUAL 5)
      if(NOT DEFINED KF_GIT_TAG)
        #https://qelectrotech.org/forum/viewtopic.php?pid=13924#p13924
        set(KF_GIT_TAG v5.77.0)
      endif()
    else()
      if(NOT DEFINED KF_GIT_TAG)
        # this is a more or less random version, taken as an conservative approach
        set(KF_GIT_TAG v6.10.0)
      endif()
    endif()
    # using a function in order to limit the scope of the variables
    # with CMake >=3.25 we could use a block()
    function(qet_make_kf_available)
      # Fix stop the run autotests of kcoreaddons
      # see
      # https://invent.kde.org/frameworks/kcoreaddons/-/blob/master/CMakeLists.txt#L98
      # issue:
      # CMake Error at /usr/share/ECM/modules/ECMAddTests.cmake:89 (add_executable):
      # Cannot find source file:
      # see
      # https://qelectrotech.org/forum/viewtopic.php?pid=13929#p13929
      set(KDE_SKIP_TEST_SETTINGS ON)
      set(BUILD_TESTING OFF)
    
      # QElectroTech is a plain QtWidgets application with no QML anywhere in
      # it; these disable optional features of the fetched KF modules that
      # would otherwise pull in extra Qt6 components (e.g. Qt6Qml) we don't
      # have and don't need.
      set(BUILD_DESIGNERPLUGIN OFF)
      set(KCOREADDONS_USE_QML OFF)
      set(BUILD_QCH OFF)
      set(BUILD_SHARED_LIBS OFF)

      FetchContent_Declare(
        ecm
        GIT_REPOSITORY https://invent.kde.org/frameworks/extra-cmake-modules.git
        GIT_TAG        ${KF_GIT_TAG})
      FetchContent_MakeAvailable(ecm)
      
      list(APPEND CMAKE_PREFIX_PATH "${ecm_BINARY_DIR}" "${ecm_SOURCE_DIR}")

      FetchContent_Declare(
        kcoreaddons
        GIT_REPOSITORY https://invent.kde.org/frameworks/kcoreaddons.git
        GIT_TAG        ${KF_GIT_TAG})
      FetchContent_MakeAvailable(kcoreaddons)
      

      FetchContent_Declare(
        kwidgetsaddons
        GIT_REPOSITORY https://invent.kde.org/frameworks/kwidgetsaddons.git
        GIT_TAG        ${KF_GIT_TAG})
      FetchContent_MakeAvailable(kwidgetsaddons)
      
    endfunction()
    qet_make_kf_available()
  else()
    find_package(KF${KF_MAJOR_VERSION}CoreAddons REQUIRED)
    find_package(KF${KF_MAJOR_VERSION}WidgetsAddons REQUIRED)
  endif()

  set(KF_PRIVATE_LIBRARIES
    KF${KF_MAJOR_VERSION}::WidgetsAddons
    KF${KF_MAJOR_VERSION}::CoreAddons
    )
endif()
