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

project(C_unittests LANGUAGES CXX)

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
find_package(Qt6 REQUIRED COMPONENTS ${QET_COMPONENTS})
message(".. Qt version                :" ${Qt6_VERSION})

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.0)

FetchContent_MakeAvailable(Catch2)

include(../../cmake/fetch_kdeaddons.cmake)
include(../../cmake/fetch_singleapplication.cmake)
include(../../cmake/fetch_pugixml.cmake)

enable_testing()

add_executable(
    ${PROJECT_NAME}
    tst_My_test.cpp
    src/borderproperties_test.cpp
    main.cpp
    ${QET_DIR}/sources/borderproperties.cpp
    ${QET_DIR}/sources/borderproperties.h
    )

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    Catch2::Catch2
    PRIVATE
    ${KF6_PRIVATE_LIBRARIES}
    ${QET_PRIVATE_LIBRARIES})

if (DEFINED ENV{CATCH_INCLUDE_DIR})
    set(CATCH_INCLUDE_DIR $ENV{CATCH_INCLUDE_DIR})
else ()
    set(CATCH_INCLUDE_DIR "") # set by Qt Creator wizard
endif ()
if (CATCH_INCLUDE_DIR STREQUAL "")
    message(".. CATCH_INCLUDE_DIR is not set,"
      "assuming Catch2 can be found automatically in your system")
elseif (EXISTS ${CATCH_INCLUDE_DIR})
    include_directories(${CATCH_INCLUDE_DIR})
endif ()
