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

cmake_minimum_required(VERSION 3.5)

message("_____________________________________________________________________")

project(C_unittests LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
SET(CMAKE_CXX_STANDARD 17)

if(DEFINED ENV{QET_DIR})
  set(QET_DIR "../..")
  message("QET_DIR is not set, assuming QET is ../..")
endif()

message("PROJECT_NAME:       " ${PROJECT_NAME})
message("PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message("QET_DIR:            " ${QET_DIR})

SET(CMAKE_CXX_STANDARD 17)

find_package(Qt5Gui REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(KF5WidgetsAddons)
find_package(KF5CoreAddons)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.1)

FetchContent_MakeAvailable(Catch2)


add_executable(
    ${PROJECT_NAME}
    tst_My_test.cpp
    src/borderproperties_test.cpp
    src/qet_test.cpp
    main.cpp
    ${QET_DIR}/sources/borderproperties.cpp
    ${QET_DIR}/sources/borderproperties.h
    ${QET_DIR}/sources/qet.cpp
    ${QET_DIR}/sources/qet.h
    ${QET_DIR}/sources/qeticons.cpp
    ${QET_DIR}/sources/qeticons.h)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    Catch2::Catch2
    PRIVATE
    KF5::WidgetsAddons
    KF5::CoreAddons
    Qt5::Gui
    Qt5::Xml
    Qt5::Svg
    Qt5::Sql
    Qt5::Widgets
    Qt5::Concurrent)

if (DEFINED ENV{CATCH_INCLUDE_DIR})
    set(CATCH_INCLUDE_DIR $ENV{CATCH_INCLUDE_DIR})
else ()
    set(CATCH_INCLUDE_DIR "") # set by Qt Creator wizard
endif ()
if (CATCH_INCLUDE_DIR STREQUAL "")
    message("CATCH_INCLUDE_DIR is not set,"
      "assuming Catch2 can be found automatically in your system")
elseif (EXISTS ${CATCH_INCLUDE_DIR})
    include_directories(${CATCH_INCLUDE_DIR})
endif ()
