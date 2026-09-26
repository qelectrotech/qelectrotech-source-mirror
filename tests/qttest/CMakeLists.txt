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

# bordercelllabels.h is header-only too.
add_executable(tst_bordercelllabels tst_bordercelllabels.cpp)
add_test(NAME tst_bordercelllabels COMMAND tst_bordercelllabels)
target_include_directories(tst_bordercelllabels PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_bordercelllabels PRIVATE Qt::Test)

# contactusage.h is a header-only helper holding the contact counting
# rules, so this test builds independently of the rest of the QET sources.
add_executable(tst_contactusage tst_contactusage.cpp)
add_test(NAME tst_contactusage COMMAND tst_contactusage)
target_include_directories(tst_contactusage PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_contactusage PRIVATE Qt::Test)

# textgrid.h is a header-only helper holding the text snap rules, so this
# test builds independently of the rest of the QET sources.
add_executable(tst_textgrid tst_textgrid.cpp)
add_test(NAME tst_textgrid COMMAND tst_textgrid)
target_include_directories(tst_textgrid PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_textgrid PRIVATE Qt::Test)

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

# QETSql::execReadOnly() -- read-only enforcement for every project-database
# query, including the ones a .qet file carries. Compiles sqlreadonly.cpp
# alone against its own in-memory QSQLITE connection, so the security
# property is checked without standing up a QETProject.
add_executable(
  tst_sqlreadonly
  tst_sqlreadonly.cpp
  ${QET_DIR}/sources/dataBase/sqlreadonly.cpp)
add_test(NAME tst_sqlreadonly COMMAND tst_sqlreadonly)
target_include_directories(tst_sqlreadonly PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_sqlreadonly PRIVATE Qt::Test Qt::Sql)

# QetSettings::scriptingEnabled() -- whether QElectroTech may run a script.
# Compiles qetsettings.cpp alone: the setting is deliberately a plain
# QSettings read, so the test needs nothing else of QElectroTech.
add_executable(
  tst_scriptingsetting
  tst_scriptingsetting.cpp
  ${QET_DIR}/sources/utils/qetsettings.cpp)
add_test(NAME tst_scriptingsetting COMMAND tst_scriptingsetting)
target_include_directories(tst_scriptingsetting PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_scriptingsetting PRIVATE Qt::Test Qt::Gui)

# QetSettings::sheetBackground() -- the sheet (folio) background the diagram
# editor remembers between runs. Compiles qetsettings.cpp alone for the same
# reason as the test above; Qt::Gui because the pair carries a QColor.
add_executable(
  tst_sheetbackgroundsetting
  tst_sheetbackgroundsetting.cpp
  ${QET_DIR}/sources/utils/qetsettings.cpp)
add_test(NAME tst_sheetbackgroundsetting COMMAND tst_sheetbackgroundsetting)
target_include_directories(tst_sheetbackgroundsetting PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_sheetbackgroundsetting PRIVATE Qt::Test Qt::Gui)

# SpaceMouseMotion::map() -- how a 3D mouse sample pans and zooms a view.
# Pure code with no device or widget behind it, so it is built and run
# whether or not QET_ENABLE_SPACEMOUSE is on.
add_executable(
  tst_spacemousemotion
  tst_spacemousemotion.cpp
  ${QET_DIR}/sources/spacemouse/spacemousemotion.cpp)
add_test(NAME tst_spacemousemotion COMMAND tst_spacemousemotion)
target_include_directories(tst_spacemousemotion PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_spacemousemotion PRIVATE Qt::Test Qt::Core)

# SpaceMouseHid -- decoding a 3D mouse's raw USB reports. Pure code, built
# whether or not QET_ENABLE_SPACEMOUSE is on; recordings from real devices
# go in fixtures/spacemouse/.
add_executable(
  tst_spacemousehid
  tst_spacemousehid.cpp
  ${QET_DIR}/sources/spacemouse/spacemousehid.cpp)
add_test(NAME tst_spacemousehid COMMAND tst_spacemousehid)
target_include_directories(tst_spacemousehid PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_spacemousehid PRIVATE Qt::Test Qt::Core)

# ConnexionBackend -- the macOS 3DxWare backend. Built on every platform and
# run against fakeconnexion, a stand-in for 3DxWare's client library that
# exports the same calls and answers from a thread of its own.
add_library(fakeconnexion MODULE fakeconnexion.cpp)
target_link_libraries(fakeconnexion PRIVATE Qt::Core)
add_executable(
  tst_spacemouseconnexion
  tst_spacemouseconnexion.cpp
  ${QET_DIR}/sources/spacemouse/connexionbackend.cpp
  ${QET_DIR}/sources/spacemouse/spacemousebackend.h)
add_dependencies(tst_spacemouseconnexion fakeconnexion)
add_test(NAME tst_spacemouseconnexion COMMAND tst_spacemouseconnexion)
target_compile_definitions(tst_spacemouseconnexion PRIVATE
  FAKE_CONNEXION="$<TARGET_FILE:fakeconnexion>")
target_include_directories(tst_spacemouseconnexion PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_spacemouseconnexion PRIVATE Qt::Test Qt::Core)

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
target_link_libraries(tst_crashdumps PRIVATE
  Qt::Test Qt::Widgets Qt::Xml pugixml::pugixml)
if(Backtrace_FOUND)
  target_link_libraries(tst_crashdumps PRIVATE ${Backtrace_LIBRARIES})
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
