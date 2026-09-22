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

project(qt_unittests LANGUAGES CXX)

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

find_package(
 Qt6
 COMPONENTS
 ${QET_COMPONENTS}
 Test
 REQUIRED)

# tst_crashhandler compiles crashhandler.cpp, which on a non-Windows build
# may pull in backtrace(). Resolved the same way as in the top-level
# CMakeLists so this directory also configures standalone -- on FreeBSD the
# symbol lives in libexecinfo, not libc.
if(NOT DEFINED Backtrace_FOUND)
  find_package(Backtrace QUIET)
  if(Backtrace_FOUND)
    include_directories(${Backtrace_INCLUDE_DIRS})
    add_compile_definitions(QET_CRASH_BACKTRACE)
  endif()
endif()

include(../../cmake/fetch_kdeaddons.cmake)
include(../../cmake/fetch_singleapplication.cmake)
include(../../cmake/fetch_pugixml.cmake)

enable_testing()

add_executable(${PROJECT_NAME} tst_mytest.cpp)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  Qt::Test
  ${KF_PRIVATE_LIBRARIES}
  ${QET_PRIVATE_LIBRARIES})

# diagramsortkeys.h is a header-only helper (no QET link deps needed
# beyond Qt itself), so this test builds independently of the rest of
# the QET sources.
add_executable(tst_diagramsortkeys tst_diagramsortkeys.cpp)
add_test(NAME tst_diagramsortkeys COMMAND tst_diagramsortkeys)
target_include_directories(tst_diagramsortkeys PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_diagramsortkeys PRIVATE Qt::Test)

# contactusage.h is a header-only helper holding the contact counting
# rules, so this test builds independently of the rest of the QET sources.
add_executable(tst_contactusage tst_contactusage.cpp)
add_test(NAME tst_contactusage COMMAND tst_contactusage)
target_include_directories(tst_contactusage PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_contactusage PRIVATE Qt::Test)

add_executable(
  tst_qetpalette
  tst_qetpalette.cpp
  inkcontrast.h
  ${QET_DIR}/sources/palettegraphicsview.cpp
  ${QET_DIR}/sources/palettegraphicsview.h
  ${QET_DIR}/sources/qetpalette.cpp
  ${QET_DIR}/sources/qetpalette.h
  ${QET_DIR}/sources/ElementsCollection/elementpreviewdelegate.cpp
  ${QET_DIR}/sources/ElementsCollection/elementpreviewdelegate.h)
add_test(NAME tst_qetpalette COMMAND tst_qetpalette)
target_include_directories(tst_qetpalette PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_qetpalette PRIVATE Qt::Test Qt::Widgets)

# qeticons.cpp is compiled in so the test can check the table entries
# themselves (icon sizes in the element panel), not only the theme files.
add_executable(
  tst_qeticons
  tst_qeticons.cpp
  inkcontrast.h
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/qeticons.h
  ${QET_DIR}/sources/qetpalette.cpp
  ${QET_DIR}/sources/qetstyle.cpp
  ${QET_DIR}/sources/qetpalette.h
  ${QET_DIR}/ico/icon-themes.qrc)
add_test(NAME tst_qeticons COMMAND tst_qeticons)
target_include_directories(tst_qeticons PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_qeticons PRIVATE Qt::Test Qt::Widgets Qt::Svg)

add_executable(
  tst_smart_device
  tst_smart_device.cpp
  ${QET_DIR}/sources/bomexport.cpp
  ${QET_DIR}/sources/bomexport.h)
add_test(NAME tst_smart_device COMMAND tst_smart_device)
target_include_directories(tst_smart_device PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_smart_device PRIVATE Qt::Test Qt::Sql)

# qet.cpp carries the SingleApplication argument wire format
# (joinWithSpaces/splitWithSpaces); it pulls in qeticons and shortcutmanager,
# so those are compiled alongside rather than linking the whole application.
add_executable(
  tst_qetstrings
  tst_qetstrings.cpp
  ${QET_DIR}/sources/qet.cpp
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/shortcutmanager.cpp)
add_test(NAME tst_qetstrings COMMAND tst_qetstrings)
target_include_directories(tst_qetstrings PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_qetstrings PRIVATE Qt::Test Qt::Widgets Qt::Xml pugixml::pugixml)

# CrashHandler::formatInt() -- the async-signal-safe decimal formatter the
# signal handler uses for the "Signal: N" line of a crash dump. Compiles
# crashhandler.cpp and logring.cpp alongside; the handler deliberately
# depends on nothing heavier than QByteArray.
add_executable(
  tst_crashhandler
  tst_crashhandler.cpp
  ${QET_DIR}/sources/logging/crashhandler.cpp
  ${QET_DIR}/sources/logging/logring.cpp
  ${QET_DIR}/sources/qetversion.cpp)
add_test(NAME tst_crashhandler COMMAND tst_crashhandler)
target_include_directories(tst_crashhandler PRIVATE ${QET_DIR}/sources)
# Qt::Xml because crashhandler.cpp includes qetversion.h for the header it
# builds at install() time, and that pulls in QDomElement.
if(Backtrace_FOUND)
  target_link_libraries(tst_crashhandler PRIVATE
    Qt::Test Qt::Xml ${Backtrace_LIBRARIES})
else()
  target_link_libraries(tst_crashhandler PRIVATE Qt::Test Qt::Xml)
endif()

# The crash-dump bookkeeping from #905: which dumps get listed, offered and
# deleted, and what redact() masks. qetlogger.cpp needs exactly one symbol
# from the application (QETApp::dataDir()), which the test supplies itself,
# so this links the logging sources rather than the whole program.
add_executable(
  tst_crashdumps
  tst_crashdumps.cpp
  ${QET_DIR}/sources/logging/qetlogger.cpp
  ${QET_DIR}/sources/logging/crashhandler.cpp
  ${QET_DIR}/sources/logging/logring.cpp
  ${QET_DIR}/sources/qetversion.cpp)
add_test(NAME tst_crashdumps COMMAND tst_crashdumps)
target_include_directories(tst_crashdumps PRIVATE
  ${QET_DIR}
  ${QET_DIR}/sources
  ${QET_DIR}/sources/NameList)
if(Backtrace_FOUND)
  target_link_libraries(tst_crashdumps PRIVATE
    Qt::Test Qt::Widgets Qt::Xml pugixml::pugixml ${Backtrace_LIBRARIES})
else()
  target_link_libraries(tst_crashdumps PRIVATE Qt::Test Qt::Widgets Qt::Xml)
endif()

add_executable(
  tst_menubarkeyboard
  tst_menubarkeyboard.cpp)
add_test(NAME tst_menubarkeyboard COMMAND tst_menubarkeyboard)
target_include_directories(tst_menubarkeyboard PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_menubarkeyboard PRIVATE Qt::Test Qt::Widgets)

# Bugtracker #734 regression: exports fixtures/qet_bug_repro_resaved.qet
# (the report's own canonical reproduction) via the built qelectrotech
# binary's --export-svg and checks no conductor path is self-retracing.
# Runs the real binary rather than linking conductor.cpp directly, since
# Conductor::generateConductorPath() pulls in the full graphics-item
# dependency graph (Diagram, Element, Terminal...) that the other,
# deliberately header-only tests above avoid.
add_executable(
  tst_conductorselfretrace
  tst_conductorselfretrace.cpp)
add_test(NAME tst_conductorselfretrace COMMAND tst_conductorselfretrace)
add_dependencies(tst_conductorselfretrace qelectrotech)
target_include_directories(tst_conductorselfretrace PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_conductorselfretrace PRIVATE Qt::Test)
target_compile_definitions(tst_conductorselfretrace PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")
