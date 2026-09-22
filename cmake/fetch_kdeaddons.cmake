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

if(BUILD_WITH_KF)
  option(BUILD_KF "Build KF libraries, use system ones otherwise" YES)
  option(
    BUILD_KF_MINIMAL
    "Build only the bundled minimal set of KF libraries"
    OFF)

  if(BUILD_KF AND NOT BUILD_KF_MINIMAL)
    message(VERBOSE "Building KF libraries from KDE git repository")
    Include(FetchContent)

    # v6.10.0 is a more or less random version, taken as an conservative
    # approach. Pinned to the commits v6.10.0 points at, not to the tags
    # themselves; see the note in fetch_pugixml.cmake. Each module lives in its
    # own repository, so the same v6.10.0 release is a different commit in
    # each.
    #
    # KDE uses annotated tags, so "git ls-remote <repo> 'refs/tags/v6.10.0*'"
    # prints two hashes per module: refs/tags/v6.10.0 is the tag object (the
    # tagger, the date and the tag message) and refs/tags/v6.10.0^{} is the
    # commit that object points at. The hashes below are the "^{}" ones, i.e.
    # the commits. Lightweight tags, such as pugixml's v1.15 and
    # SingleApplication's v3.2.0, have no tag object and print only the
    # commit line.
    set(KF_ECM_GIT_COMMIT            7dd28cc56c339c3f8fb356f7c53c0e8f61433d81) # v6.10.0
    set(KF_KCOREADDONS_GIT_COMMIT    c569f974dab24b4784ad186a3db4b76b2fa36612) # v6.10.0
    set(KF_KWIDGETSADDONS_GIT_COMMIT 1abbed8a280d6626c59fb197f2c4667d2b1e7445) # v6.10.0

    if(DEFINED KF_GIT_TAG)
      # Explicit override: -DKF_GIT_TAG=<ref> selects one ref for all three
      # modules, unpinned, exactly as it did before.
      set(KF_ECM_GIT_COMMIT            ${KF_GIT_TAG})
      set(KF_KCOREADDONS_GIT_COMMIT    ${KF_GIT_TAG})
      set(KF_KWIDGETSADDONS_GIT_COMMIT ${KF_GIT_TAG})
    else()
      # Keep KF_GIT_TAG defined: define_definitions.cmake reports it.
      set(KF_GIT_TAG v6.10.0)
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
        GIT_TAG        ${KF_ECM_GIT_COMMIT})
      FetchContent_MakeAvailable(ecm)

      FetchContent_Declare(
        kcoreaddons
        GIT_REPOSITORY https://invent.kde.org/frameworks/kcoreaddons.git
        GIT_TAG        ${KF_KCOREADDONS_GIT_COMMIT})
      FetchContent_MakeAvailable(kcoreaddons)

      FetchContent_Declare(
        kwidgetsaddons
        GIT_REPOSITORY https://invent.kde.org/frameworks/kwidgetsaddons.git
        GIT_TAG        ${KF_KWIDGETSADDONS_GIT_COMMIT})
      FetchContent_MakeAvailable(kwidgetsaddons)
    endfunction()
    qet_make_kf_available()

  elseif(BUILD_KF AND BUILD_KF_MINIMAL)
    message(WARNING "BUILD_KF_MINIMAL is experimental and may not work as expected.")
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

  elseif(NOT BUILD_KF)
    message(
      VERBOSE
      "Using system KF libraries")
    find_package(KF6CoreAddons REQUIRED)
    find_package(KF6WidgetsAddons REQUIRED)
  endif()

  set(KF_PRIVATE_LIBRARIES
    KF6::WidgetsAddons
    KF6::CoreAddons
    )
endif()
