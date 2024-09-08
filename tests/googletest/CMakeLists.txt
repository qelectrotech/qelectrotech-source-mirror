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

message("..___________________________________________________________________")

project(G_unittests LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(".. PROJECT_NAME              :" ${PROJECT_NAME})
message(".. PROJECT_SOURCE_DIR        :" ${PROJECT_SOURCE_DIR})
if(NOT DEFINED QET_DIR)
  set(QET_DIR "../..")
  message(".. QET_DIR is not set, assuming QET is ../..")
endif()
message(".. QET_DIR                   :" ${QET_DIR})
if(NOT DEFINED QET_COMPONENTS)
  message(".. QET_COMPONENTS is not set !!! I set them up !!!")
  include(../../cmake/qet_compilation_vars.cmake)
endif()
if(NOT DEFINED QT_VERSION_MAJOR)
  find_package(
    QT
   NAMES
    Qt6
   COMPONENTS
    ${QET_COMPONENTS}
   REQUIRED
   )
endif()

find_package(
  Qt${QT_VERSION_MAJOR}
 COMPONENTS
 ${QET_COMPONENTS}
 REQUIRED)
 message(".. Qt version                :" ${Qt${QT_VERSION_MAJOR}_VERSION})

Include(FetchContent)

FetchContent_Declare(
  GTest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2)

FetchContent_MakeAvailable(GTest)

include(../../cmake/fetch_kdeaddons.cmake)
include(../../cmake/fetch_singleapplication.cmake)
include(../../cmake/fetch_pugixml.cmake)

enable_testing()

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
    ${KF6_PRIVATE_LIBRARIES}
    ${QET_PRIVATE_LIBRARIES})

