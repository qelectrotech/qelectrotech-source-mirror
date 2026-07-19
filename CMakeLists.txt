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

project(qelectrotech
        VERSION 0.100.1
        DESCRIPTION "QET is a CAD/CAE editor focusing on schematics drawing features."
        HOMEPAGE_URL "https://qelectrotech.org/"
        LANGUAGES CXX)

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

# Add sub directories
option(PACKAGE_TESTS "Build the tests" ON)
if(PACKAGE_TESTS)
    message("Add sub directory tests")
    add_subdirectory(tests)
endif()

include(cmake/paths_compilation_installation.cmake)
include(cmake/start_options.cmake)
include(cmake/developer_options.cmake)
include(cmake/git_update_submodules.cmake)
include(cmake/git_last_commit_sha.cmake)
include(cmake/fetch_kdeaddons.cmake)
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

# Qt6 only creates the Qt::GuiPrivate target (used for QPdfEngine::drawHyperlink)
# when the GuiPrivate component is explicitly requested. Qt5 has no such
# component package and creates the target implicitly with Gui, so only
# request it on Qt6 - requesting it on Qt5 fails the whole configure.
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS GuiPrivate)
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

# The default build only compiles the tracked .ts files to .qm (lrelease).
# Refreshing the .ts from the sources (lupdate) is a developer action behind
# the explicit "update_translations" target below: running lupdate on every
# build rewrote tracked files as a side effect, and under high parallelism
# lupdate rewriting a .ts while lrelease read the same file made the build
# fail with "Premature end of document".
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${QET_DIR}/lang")
if(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_translation(QM_FILES ${TS_FILES})
else()
    qt5_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(update_translations
    COMMAND $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::lupdate> ${CMAKE_SOURCE_DIR}/sources -ts ${TS_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Updating .ts files from sources/ (lupdate) - developer target, run explicitly"
    VERBATIM
)

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
else()
    add_executable(
        ${PROJECT_NAME}
        ${QET_RES_FILES}
        ${QET_SRC_FILES}
        ${QM_FILES}
        ${QET_DIR}/qelectrotech.qrc
    )
endif()

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    PRIVATE
    pugixml::pugixml
    SingleApplication::SingleApplication
    SQLite3::SQLite3
    ${KF5_PRIVATE_LIBRARIES}
    ${QET_PRIVATE_LIBRARIES}
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
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

if(NOT BUILD_WITH_KF5)
    target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${QET_DIR}/sources/ui/nokde
    )
endif()

install(TARGETS ${PROJECT_NAME})

if (NOT MINGW)
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
        install(FILES misc/org.qelectrotech.qelectrotech.desktop DESTINATION share/applications)
        install(FILES misc/qelectrotech.xml    DESTINATION ${QET_MIME_PACKAGE_PATH})
        install(FILES misc/qelectrotech.appdata.xml DESTINATION ${QET_APPDATA_PATH})
    endif()
    install(FILES ${QM_FILES} DESTINATION ${QET_LANG_PATH})
endif()
