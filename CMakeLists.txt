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

include(cmake/hoto_update_cmake_message.cmake)

cmake_minimum_required(VERSION 3.5...4.2)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/QET_VERSION" QET_VERSION_STRING LIMIT_COUNT 1)
string(STRIP "${QET_VERSION_STRING}" QET_VERSION_STRING)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/QET_RELEASE" QET_VERSION_TYPE LIMIT_COUNT 1)
string(STRIP "${QET_VERSION_TYPE}" QET_VERSION_TYPE)

set(_qet_valid_version_types dev alpha1 alpha2 alpha3 stable)
if(NOT QET_VERSION_TYPE IN_LIST _qet_valid_version_types)
  message(FATAL_ERROR "VERSION_TYPE file contains '${QET_VERSION_TYPE}', must be one of: ${_qet_valid_version_types}")
endif()

project(qelectrotech
        VERSION ${QET_VERSION_STRING}
        DESCRIPTION "QET is a CAD/CAE editor focusing on schematics drawing features."
        HOMEPAGE_URL "https://qelectrotech.org/"
        LANGUAGES C CXX)
        
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/sources/qetversion.cpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/qetversion.cpp"
  @ONLY
)

include(cmake/copyright_message.cmake)

set(QET_DIR ${PROJECT_SOURCE_DIR})

# QT_VERSION_MAJOR is chosen explicitly by whoever configures the build,
# via -DQT_VERSION_MAJOR=5 or -DQT_VERSION_MAJOR=6.
# Default to Qt5 (current stable) when not specified, so existing
# CI/scripts that don't pass this option keep working unchanged.
# This must happen BEFORE add_subdirectory(tests) and the fetch_*.cmake
# includes below, so every subdirectory and every FetchContent dependency
# sees a consistent, already-defined value.
if(NOT DEFINED QT_VERSION_MAJOR)
    set(QT_VERSION_MAJOR 5)
endif()

# Some third-party CMake projects we pull in via FetchContent (e.g.
# SingleApplication) don't know about QET_VERSION_MAJOR: they follow Qt's
# own "QT_DEFAULT_MAJOR_VERSION" convention instead, and silently default
# to Qt5 if it isn't set. Propagate our choice so they stay in sync,
# otherwise they can end up linked against a different Qt major version
# than the rest of QET, which breaks AUTOMOC with an
# INTERFACE_QT_MAJOR_VERSION mismatch at generate time.
set(QT_DEFAULT_MAJOR_VERSION ${QT_VERSION_MAJOR} CACHE STRING "Qt version to use (5 or 6)" FORCE)

include(cmake/paths_compilation_installation.cmake)
include(cmake/start_options.cmake)
include(cmake/developer_options.cmake)
include(cmake/git_update_submodules.cmake)
include(cmake/git_last_commit_sha.cmake)
include(cmake/fetch_singleapplication.cmake)
include(cmake/fetch_pugixml.cmake)
include(cmake/qet_compilation_vars.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(
    Qt${QT_VERSION_MAJOR}
    COMPONENTS
    ${QET_COMPONENTS}
    REQUIRED)

# <private/qpdf_p.h> (QPdfEngine::drawHyperlink) needs Qt's private GUI module.
# Qt >= 6.7 ships it as a proper find_package component, but some distro
# packages (e.g. Ubuntu's qt6-base-private-dev) omit Qt6GuiPrivateConfig.cmake
# and only provide the implicit Qt6::GuiPrivate target created alongside
# Qt6::Gui. Try the component quietly, then verify the target below so a
# missing private-headers package fails here instead of at compile time.
# Qt5 has no such component as its GuiPrivate target always exists once Gui is found.
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 QUIET COMPONENTS GuiPrivate)
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6 AND NOT TARGET Qt6::GuiPrivate)
    message(FATAL_ERROR
        "Qt6::GuiPrivate was not found. It is required for PDF hyperlink "
        "support (<private/qpdf_p.h>). Install the Qt6 private headers "
        "(e.g. 'qt6-base-private-dev' on Debian/Ubuntu) or use a Qt build "
        "that provides the GuiPrivate component.")
endif()

find_package(SQLite3 REQUIRED)

# CMake < 4.3 only creates the SQLite::SQLite3 target (no SQLite3::SQLite3
# alias yet), while CMake >= 4.3's bundled FindSQLite3 creates SQLite3::SQLite3
# and deprecates the old name. Add the missing alias ourselves so we can use
# the modern target name everywhere regardless of the CMake version in use
# (this project must keep building on CMake versions below 4.3, e.g. on most
# current Linux distros).
if(NOT TARGET SQLite3::SQLite3 AND TARGET SQLite::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()

set(CMAKE_AUTOUIC_SEARCH_PATHS ${QET_DIR}/sources/ui)

if(QT_VERSION_MAJOR EQUAL 6)
    set(KF_MAJOR_VERSION 6)
else()
    set(KF_MAJOR_VERSION 5)
endif()
include(cmake/fetch_kdeaddons.cmake)

# Add sub directories
option(PACKAGE_TESTS "Build the tests" ON)
if(PACKAGE_TESTS)
    message("Add sub directory tests")
    add_subdirectory(tests)
endif()

# als laatse
include(cmake/define_definitions.cmake)

# On Windows, WIN32 sets /SUBSYSTEM:WINDOWS to suppress the console window.
# Qt automatically links qtmain.lib which provides the WinMain entry point,
# so no source code change is needed.
if(WIN32)
    add_executable(
        ${PROJECT_NAME}
        WIN32
        ${QET_RES_FILES}
        ${QET_SRC_FILES}
        ${QM_FILES}
        ${QET_DIR}/qelectrotech.qrc
    )
elseif(APPLE)
    add_executable(
        ${PROJECT_NAME}
        MACOSX_BUNDLE
        ${QET_RES_FILES}
        ${QET_SRC_FILES}
        ${QM_FILES}
        ${QET_DIR}/qelectrotech.qrc
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${QET_DIR}/build-aux/macos/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "QElectroTech"
    )
else()
    add_executable(
        ${PROJECT_NAME}
        ${QET_RES_FILES}
        ${QET_SRC_FILES}
        ${QM_FILES}
        ${QET_DIR}/qelectrotech.qrc
    )
endif()

# The default build only compiles the tracked .ts files to .qm (lrelease).
# Refreshing the .ts from the sources (lupdate) is a developer action behind
# the explicit "update_translations" target below: running lupdate on every
# build rewrote tracked files as a side effect, and under high parallelism
# lupdate rewriting a .ts while lrelease read the same file made the build
# fail with "Premature end of document".
set_source_files_properties(
    ${TS_FILES}
    PROPERTIES OUTPUT_LOCATION "${QET_DIR}/lang"
)
if(QT_VERSION_MAJOR EQUAL 6)
    if(Qt6_VERSION VERSION_LESS "6.2")
        # Qt 6.0–6.1
        qt6_add_translation(QM_FILES ${TS_FILES})

        # qt6_add_translation() only creates custom commands. Something must
        # depend on their outputs for them to run during the default build.
        add_custom_target(${PROJECT_NAME}_lrelease ALL
            DEPENDS ${QM_FILES}
        )

        add_custom_target(update_translations
            COMMAND
                $<TARGET_FILE:Qt6::lupdate>
                "${CMAKE_SOURCE_DIR}/sources"
                -ts ${TS_FILES}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT
                "Updating .ts files from sources/ (lupdate) - developer target"
            VERBATIM
        )

    elseif(Qt6_VERSION VERSION_LESS "6.7")
        # Qt 6.2–6.6: old target-based signature
        qt_add_lrelease(
            ${PROJECT_NAME}
            TS_FILES ${TS_FILES}
            QM_FILES_OUTPUT_VARIABLE QM_FILES
        )

        # Automatically creates the update_translations umbrella target.
        qt_add_lupdate(
            ${PROJECT_NAME}
            TS_FILES ${TS_FILES}
        )

    else()
        # Qt 6.7+: new signature
        qt_add_lrelease(
            TS_FILES ${TS_FILES}
            LRELEASE_TARGET ${PROJECT_NAME}_lrelease
            QM_FILES_OUTPUT_VARIABLE QM_FILES
        )

        qt_add_lupdate(
            SOURCE_TARGETS ${PROJECT_NAME}
            TS_FILES ${TS_FILES}
            LUPDATE_TARGET update_translations
            NO_GLOBAL_TARGET
        )
    endif()

else()
    # Qt 5
    qt5_add_translation(QM_FILES ${TS_FILES})

    # Likewise, qt5_add_translation() needs a target depending on its outputs,
    # unless QM_FILES are already consumed elsewhere.
    add_custom_target(${PROJECT_NAME}_lrelease ALL
        DEPENDS ${QM_FILES}
    )

    add_custom_target(update_translations
        COMMAND
            $<TARGET_FILE:Qt5::lupdate>
            "${CMAKE_SOURCE_DIR}/sources"
            -ts ${TS_FILES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT
            "Updating .ts files from sources/ (lupdate) - developer target"
        VERBATIM
    )
endif()

# Optional precompiled headers -- see QET_ENABLE_PCH in
# cmake/developer_options.cmake for what this trades away.
#
# target_precompile_headers() needs CMake 3.16; the project still declares a
# 3.5 minimum, so guard rather than raise it for an opt-in developer feature.
#
# The generator expressions are load-bearing, not decoration: this target also
# compiles the 18 C files of the bundled LZMA decoder
# (sources/import/edz/lzma/*.c), and an unguarded list applies to every
# language in the target, so the Qt headers would reach the C compiler and fail
# with "unknown type name 'namespace'". $<ANGLE-R> is required because a
# literal '>' would terminate the generator expression.
if(QET_ENABLE_PCH)
    if(CMAKE_VERSION VERSION_LESS 3.16)
        message(WARNING
            "QET_ENABLE_PCH needs CMake 3.16 or newer (found ${CMAKE_VERSION}); "
            "building without precompiled headers.")
    else()
        target_precompile_headers(${PROJECT_NAME} PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX>:<QtCore/QtCore$<ANGLE-R>>"
            "$<$<COMPILE_LANGUAGE:CXX>:<QtGui/QtGui$<ANGLE-R>>"
            "$<$<COMPILE_LANGUAGE:CXX>:<QtWidgets/QtWidgets$<ANGLE-R>>"
            "$<$<COMPILE_LANGUAGE:CXX>:<QtXml/QtXml$<ANGLE-R>>"
        )
        message(STATUS "QET_ENABLE_PCH: precompiled headers enabled")
    endif()
endif()

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    PRIVATE
    pugixml::pugixml
    SingleApplication::SingleApplication
    SQLite3::SQLite3
    ${KF_PRIVATE_LIBRARIES}
    ${QET_PRIVATE_LIBRARIES}
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${QET_DIR}/sources
    ${QET_DIR}/sources/titleblock
    ${QET_DIR}/sources/ui
    ${QET_DIR}/sources/qetgraphicsitem
    ${QET_DIR}/sources/qetgraphicsitem/ViewItem
    ${QET_DIR}/sources/qetgraphicsitem/ViewItem/ui
    ${QET_DIR}/sources/richtext
    ${QET_DIR}/sources/factory
    ${QET_DIR}/sources/properties
    ${QET_DIR}/sources/dvevent
    ${QET_DIR}/sources/editor
    ${QET_DIR}/sources/editor/esevent
    ${QET_DIR}/sources/editor/graphicspart
    ${QET_DIR}/sources/editor/ui
    ${QET_DIR}/sources/editor/UndoCommand
    ${QET_DIR}/sources/undocommand
    ${QET_DIR}/sources/diagramevent
    ${QET_DIR}/sources/ElementsCollection
    ${QET_DIR}/sources/ElementsCollection/ui
    ${QET_DIR}/sources/autoNum
    ${QET_DIR}/sources/autoNum/ui
    ${QET_DIR}/sources/ui/configpage
    ${QET_DIR}/sources/SearchAndReplace
    ${QET_DIR}/sources/SearchAndReplace/ui
    ${QET_DIR}/sources/NameList
    ${QET_DIR}/sources/NameList/ui
    ${QET_DIR}/sources/utils
    ${QET_DIR}/pugixml/src
    ${QET_DIR}/sources/dataBase
    ${QET_DIR}/sources/dataBase/ui
    ${QET_DIR}/sources/factory/ui
    ${QET_DIR}/sources/print
    ${QET_DIR}/sources/svg
)

if(NOT BUILD_WITH_KF)
    target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${QET_DIR}/sources/ui/nokde
    )
endif()

install(TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${QET_BINARY_PATH}
)

if ((NOT MINGW) AND (NOT WIN32))
    install(DIRECTORY ico/breeze-icons/16x16   DESTINATION ${QET_ICONS_PATH})
    install(DIRECTORY ico/breeze-icons/22x22   DESTINATION ${QET_ICONS_PATH})
    install(DIRECTORY ico/breeze-icons/32x32   DESTINATION ${QET_ICONS_PATH})
    install(DIRECTORY ico/breeze-icons/48x48   DESTINATION ${QET_ICONS_PATH})
    install(DIRECTORY ico/breeze-icons/64x64   DESTINATION ${QET_ICONS_PATH})
    install(DIRECTORY ico/breeze-icons/128x128 DESTINATION ${QET_ICONS_PATH})
    install(DIRECTORY ico/breeze-icons/256x256 DESTINATION ${QET_ICONS_PATH})
    install(DIRECTORY elements    DESTINATION share/qelectrotech)
    install(DIRECTORY examples    DESTINATION share/qelectrotech)
    install(DIRECTORY titleblocks DESTINATION share/qelectrotech)
    install(FILES LICENSE ELEMENTS.LICENSE CREDIT README ChangeLog DESTINATION share/doc/qelectrotech)
    if(UNIX AND NOT APPLE)
        install(FILES build-aux/linux/org.qelectrotech.qelectrotech.desktop DESTINATION share/applications)
        install(FILES build-aux/linux/qelectrotech.xml    DESTINATION ${QET_MIME_PACKAGE_PATH})
        install(FILES build-aux/linux/qelectrotech.appdata.xml DESTINATION ${QET_APPDATA_PATH})
    endif()
    install(FILES ${QM_FILES} DESTINATION ${QET_LANG_PATH})
endif()
