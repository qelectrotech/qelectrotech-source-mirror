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

cmake_minimum_required(VERSION 3.14...3.19 FATAL_ERROR)

project(qelectrotech
  VERSION 0.9.0
  DESCRIPTION "QET is a CAD/CAE editor focusing on schematics drawing features."
  HOMEPAGE_URL "https://qelectrotech.org/"
  LANGUAGES CXX)

include(cmake/copyright_message.cmake)

set(QET_DIR ${PROJECT_SOURCE_DIR})

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
  QT
 NAMES
  Qt6
 COMPONENTS
  ${QET_COMPONENTS}
 REQUIRED
 )

find_package(
  Qt${QT_VERSION_MAJOR}
 COMPONENTS
 ${QET_COMPONENTS}
 REQUIRED)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${QET_DIR}/sources/ui)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${QET_DIR}/lang")
qt_add_translation(QM_FILES ${TS_FILES})

# als laatse
include(cmake/define_definitions.cmake)

add_executable(
  ${PROJECT_NAME}
  ${QET_RES_FILES}
  ${QET_SRC_FILES}
  ${QM_FILES}
  ${QET_DIR}/qelectrotech.qrc
  )

find_package(SQLite3 REQUIRED)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
  PRIVATE
  pugixml::pugixml
  SingleApplication::SingleApplication
  SQLite::SQLite3
  ${KF6_PRIVATE_LIBRARIES}
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
  )

install(TARGETS ${PROJECT_NAME})
if (NOT MINGW)

install(DIRECTORY ico/breeze-icons/16x16 DESTINATION ${QET_ICONS_PATH})
install(DIRECTORY ico/breeze-icons/22x22 DESTINATION ${QET_ICONS_PATH})
install(DIRECTORY ico/breeze-icons/32x32 DESTINATION ${QET_ICONS_PATH})
install(DIRECTORY ico/breeze-icons/48x48 DESTINATION ${QET_ICONS_PATH})
install(DIRECTORY ico/breeze-icons/64x64 DESTINATION ${QET_ICONS_PATH})
install(DIRECTORY ico/breeze-icons/128x128 DESTINATION ${QET_ICONS_PATH})
install(DIRECTORY ico/breeze-icons/256x256 DESTINATION ${QET_ICONS_PATH})
install(DIRECTORY elements DESTINATION share/qelectrotech)
install(DIRECTORY examples DESTINATION share/qelectrotech)
install(DIRECTORY titleblocks DESTINATION share/qelectrotech)
install(FILES LICENSE ELEMENTS.LICENSE CREDIT README ChangeLog DESTINATION share/doc/qelectrotech)
install(FILES misc/org.qelectrotech.qelectrotech.desktop DESTINATION share/applications)
install(FILES misc/qelectrotech.xml DESTINATION share/mime/packages)
install(FILES misc/qelectrotech.appdata.xml DESTINATION ${QET_APPDATA_PATH})
install(FILES ${QM_FILES} DESTINATION ${QET_LANG_PATH})

endif()
