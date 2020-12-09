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

project(G_unittests LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{QET_DIR})
  set(QET_DIR "../..")
  set(QET_COMPONENTS
    Widgets
    Concurrent
    Xml
    Svg
    Network
    Sql
    PrintSupport
    LinguistTools)
  message("QET_DIR is not set, assuming QET is ../..")
endif()

message("PROJECT_NAME:       " ${PROJECT_NAME})
message("PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message("QET_DIR:            " ${QET_DIR})

find_package(
  QT
 NAMES
  Qt6
  Qt5
 COMPONENTS
  ${QET_COMPONENTS}
 REQUIRED
 )

find_package(
  Qt${QT_VERSION_MAJOR}
 COMPONENTS
 ${QET_COMPONENTS}
 REQUIRED)

Include(FetchContent)

FetchContent_Declare(
  GTest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0)

FetchContent_MakeAvailable(GTest)

find_package(KF5CoreAddons REQUIRED)
#---ToDo why you don't work :/
#FetchContent_Declare(
#  kcoreaddons
#  GIT_REPOSITORY https://invent.kde.org/frameworks/kcoreaddons.git
#  GIT_TAG        v5.76.0)
#FetchContent_MakeAvailable(kcoreaddons)
#___ToDo

FetchContent_Declare(
  kwidgetsaddons
  GIT_REPOSITORY https://invent.kde.org/frameworks/kwidgetsaddons.git
  GIT_TAG        v5.76.0)

FetchContent_MakeAvailable(kwidgetsaddons)

add_executable(
    ${PROJECT_NAME}
    tst_My_test.cpp
    main.cpp
    )

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    gtest gtest_main
    PRIVATE
    KF5::WidgetsAddons
    KF5::CoreAddons
    Qt5::Gui
    Qt5::Xml
    Qt5::Svg
    Qt5::Sql
    Qt5::Network
    Qt5::Widgets
    Qt5::Concurrent)

