# Copyright 2006-2026 The QElectroTech Team
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

# textlines.h is header-only: the lines of a text as they are laid out,
# for the exports that write a text line by line.
add_executable(tst_textlines tst_textlines.cpp)
add_test(NAME tst_textlines COMMAND tst_textlines)
set_tests_properties(tst_textlines PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_textlines PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_textlines PRIVATE Qt::Test Qt::Gui)

# elementviewgrid.h is header-only: the symbol editor's snap step at each
# zoom (bugtracker #112), tested without an editor window.
add_executable(tst_elementviewgrid tst_elementviewgrid.cpp)
add_test(NAME tst_elementviewgrid COMMAND tst_elementviewgrid)
target_include_directories(tst_elementviewgrid PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_elementviewgrid PRIVATE Qt::Test)

# alignment.h is header-only too (it builds on textgrid.h): the geometry
# behind the align commands, tested without a scene.
add_executable(tst_alignment tst_alignment.cpp)
add_test(NAME tst_alignment COMMAND tst_alignment)
target_include_directories(tst_alignment PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_alignment PRIVATE Qt::Test)

# terminalnamecheck.h is header-only: the terminal name rule applied by the
# element editor on save and by --check-elements.
add_executable(tst_terminalnamecheck tst_terminalnamecheck.cpp)
add_test(NAME tst_terminalnamecheck COMMAND tst_terminalnamecheck)
target_include_directories(tst_terminalnamecheck PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_terminalnamecheck PRIVATE Qt::Test)

# configprofile.h is header-only: which settings a saved configuration
# profile carries, and how loading one replaces the live settings.
add_executable(tst_configprofile tst_configprofile.cpp)
add_test(NAME tst_configprofile COMMAND tst_configprofile)
target_include_directories(tst_configprofile PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_configprofile PRIVATE Qt::Test Qt::Gui)

# itemgroups.cpp keeps group membership and the selection rule apart from
# Diagram, so it is tested here on a plain scene of rectangles.
add_executable(tst_itemgroups tst_itemgroups.cpp ${QET_DIR}/sources/itemgroups.cpp)
add_test(NAME tst_itemgroups COMMAND tst_itemgroups)
target_include_directories(tst_itemgroups PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_itemgroups PRIVATE Qt::Test Qt::Widgets Qt::Xml)

# shownkinds.cpp keeps View > Show (bugtracker #301) apart from the item
# classes, so it is tested here on a plain scene of rectangles.
add_executable(tst_shownkinds tst_shownkinds.cpp ${QET_DIR}/sources/shownkinds.cpp)
add_test(NAME tst_shownkinds COMMAND tst_shownkinds)
set_tests_properties(tst_shownkinds PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_shownkinds PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_shownkinds PRIVATE Qt::Test Qt::Widgets)

# textanchor.h is header-only: the point of a text chosen by its alignment,
# shown as Position X/Y in the text properties (#1155).
add_executable(tst_textanchor tst_textanchor.cpp)
add_test(NAME tst_textanchor COMMAND tst_textanchor)
set_tests_properties(tst_textanchor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_textanchor PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_textanchor PRIVATE Qt::Test Qt::Widgets)

# TextResizeHandles: the corner handles that change the width of a text box,
# on a plain QGraphicsTextItem with the "textWidth" property QET texts have.
add_executable(
  tst_textresizehandles
  tst_textresizehandles.cpp
  ${QET_DIR}/sources/QetGraphicsItemModeler/qetgraphicshandleritem.cpp
  ${QET_DIR}/sources/QetGraphicsItemModeler/textresizehandles.cpp)
add_test(NAME tst_textresizehandles COMMAND tst_textresizehandles)
set_tests_properties(tst_textresizehandles PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_textresizehandles PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_textresizehandles PRIVATE Qt::Test Qt::Widgets)

# legacyelementtypes.h is header-only: the rewrite of element type names in
# nomenclature queries saved before June 2022.
add_executable(tst_legacyelementtypes tst_legacyelementtypes.cpp)
add_test(NAME tst_legacyelementtypes COMMAND tst_legacyelementtypes)
target_include_directories(tst_legacyelementtypes PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_legacyelementtypes PRIVATE Qt::Test)

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

# CommandSearchPopup: accent-blind matching, ranking, and that Enter runs
# only the owning window's highlighted, enabled command.
add_executable(
  tst_commandsearch
  tst_commandsearch.cpp
  ${QET_DIR}/sources/commandsearchpopup.cpp
  ${QET_DIR}/sources/commandsearchpopup.h
  ${QET_DIR}/sources/shortcutmanager.cpp)
add_test(NAME tst_commandsearch COMMAND tst_commandsearch)
set_tests_properties(tst_commandsearch PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_commandsearch PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_commandsearch PRIVATE Qt::Test Qt::Widgets)

# ShortcutsConfigPage: icon and menu columns, category filter, finding a
# command by pressing its key, and the copied list.
add_executable(
  tst_shortcutspage
  tst_shortcutspage.cpp
  ${QET_DIR}/sources/ui/configpage/shortcutsconfigpage.cpp
  ${QET_DIR}/sources/ui/configpage/shortcutsconfigpage.h
  ${QET_DIR}/sources/ui/configpage/configpage.h
  ${QET_DIR}/sources/shortcutmanager.cpp
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/qetpalette.cpp
  ${QET_DIR}/sources/qetstyle.cpp)
add_test(NAME tst_shortcutspage COMMAND tst_shortcutspage)
set_tests_properties(tst_shortcutspage PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_shortcutspage PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_shortcutspage PRIVATE Qt::Test Qt::Widgets Qt::Svg)

# ToolbarSettings and its configuration page: defaults change nothing,
# saved values reach every open window that took them, panels are left alone.
add_executable(
  tst_toolbarsettings
  tst_toolbarsettings.cpp
  ${QET_DIR}/sources/toolbarsettings.cpp
  ${QET_DIR}/sources/toolbarsettings.h
  ${QET_DIR}/sources/ui/configpage/toolbarsconfigpage.cpp
  ${QET_DIR}/sources/ui/configpage/toolbarsconfigpage.h
  ${QET_DIR}/sources/ui/configpage/configpage.h
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/qetpalette.cpp
  ${QET_DIR}/sources/qetstyle.cpp)
add_test(NAME tst_toolbarsettings COMMAND tst_toolbarsettings)
set_tests_properties(tst_toolbarsettings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_toolbarsettings PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_toolbarsettings PRIVATE Qt::Test Qt::Widgets Qt::Svg)

# scriptheader.h is header-only: the // ==QETScript== block that turns a
# stored script into a button; and ShortcutManager::unregisterAction(), which
# takes a deleted script's command out of the lists again.
add_executable(
  tst_scriptheader
  tst_scriptheader.cpp
  ${QET_DIR}/sources/shortcutmanager.cpp)
add_test(NAME tst_scriptheader COMMAND tst_scriptheader)
set_tests_properties(tst_scriptheader PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_include_directories(tst_scriptheader PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_scriptheader PRIVATE Qt::Test Qt::Widgets)

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

# AiAssistantSetup: finding the MCP server beside an installation, and the
# configuration text for each AI assistant. Compiles the one file alone.
add_executable(
  tst_aiassistantsetup
  tst_aiassistantsetup.cpp
  ${QET_DIR}/sources/ui/aiassistantsetup.cpp)
add_test(NAME tst_aiassistantsetup COMMAND tst_aiassistantsetup)
target_include_directories(tst_aiassistantsetup PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_aiassistantsetup PRIVATE Qt::Test Qt::Core)

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

# A free text with a width keeps it when the project is saved again (the
# attribute is written only when set) and is drawn wrapped to it.
add_executable(
  tst_freetextwidth
  tst_freetextwidth.cpp)
add_test(NAME tst_freetextwidth COMMAND tst_freetextwidth)
set_tests_properties(tst_freetextwidth PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_dependencies(tst_freetextwidth qelectrotech)
target_link_libraries(tst_freetextwidth PRIVATE Qt::Test Qt::Gui Qt::Xml)
target_compile_definitions(tst_freetextwidth PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# The wiring list export (--export-cables) keeps every wire drawn to a folio
# report when a report has several, joins only one-to-one report pairs, and
# prints the folio number rather than its template. fixtures/wiring_list_arrows.qet
# has one of each report pair; the examples check the page column.
add_executable(
  tst_wiringlistexport
  tst_wiringlistexport.cpp)
add_test(NAME tst_wiringlistexport COMMAND tst_wiringlistexport)
add_dependencies(tst_wiringlistexport qelectrotech)
target_link_libraries(tst_wiringlistexport PRIVATE Qt::Test)
target_compile_definitions(tst_wiringlistexport PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
  "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

# --export-dxf writes a wrapped symbol text line by line, as it is drawn.
add_executable(
  tst_dxfwrappedtext
  tst_dxfwrappedtext.cpp)
add_test(NAME tst_dxfwrappedtext COMMAND tst_dxfwrappedtext)
add_dependencies(tst_dxfwrappedtext qelectrotech)
target_link_libraries(tst_dxfwrappedtext PRIVATE Qt::Test)
target_compile_definitions(tst_dxfwrappedtext PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# --export-pdf puts an A3 folio on an A3 page whichever way it is turned:
# QPageSize matches standard sheets upright only, so a wide folio is matched
# upright and turned. fixtures/pdf_page_a3.qet has one folio each way.
add_executable(
  tst_pdfpagesize
  tst_pdfpagesize.cpp)
add_test(NAME tst_pdfpagesize COMMAND tst_pdfpagesize)
add_dependencies(tst_pdfpagesize qelectrotech)
target_link_libraries(tst_pdfpagesize PRIVATE Qt::Test Qt::Gui)
target_compile_definitions(tst_pdfpagesize PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# --export-pdf writes the cross-reference links in the same order in every
# run, rather than in Element pointer order. Exports examples/industrial.qet
# several times, in separate processes.
add_executable(
  tst_pdflinkorder
  tst_pdflinkorder.cpp)
add_test(NAME tst_pdflinkorder COMMAND tst_pdflinkorder)
add_dependencies(tst_pdflinkorder qelectrotech)
target_link_libraries(tst_pdflinkorder PRIVATE Qt::Test)
target_compile_definitions(tst_pdflinkorder PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
  "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

# One text per potential: the number goes on the same conductor in every
# run when two conductors tie for longest. Exports
# fixtures/one_text_per_potential_tie.qet through the real binary, several
# times, since the order it used to depend on changes only between runs.
add_executable(
  tst_potentialtextcarrier
  tst_potentialtextcarrier.cpp)
add_test(NAME tst_potentialtextcarrier COMMAND tst_potentialtextcarrier)
add_dependencies(tst_potentialtextcarrier qelectrotech)
target_link_libraries(tst_potentialtextcarrier PRIVATE Qt::Test)
target_compile_definitions(tst_potentialtextcarrier PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# Junction dots: a wire's corner that lies on another wire of the same
# potential gets a dot, even when the two share no terminal (issue #1280).
# Exports fixtures/junction_dot_shared_potential.qet through the real binary.
add_executable(
  tst_junctiondots
  tst_junctiondots.cpp)
add_test(NAME tst_junctiondots COMMAND tst_junctiondots)
add_dependencies(tst_junctiondots qelectrotech)
target_link_libraries(tst_junctiondots PRIVATE Qt::Test)
target_compile_definitions(tst_junctiondots PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# With SOURCE_DATE_EPOCH set, --export-pdf writes the same bytes for the
# same project in every run. Exports examples/741.qet several times, in
# separate processes.
add_executable(
  tst_pdfreproducible
  tst_pdfreproducible.cpp)
add_test(NAME tst_pdfreproducible COMMAND tst_pdfreproducible)
add_dependencies(tst_pdfreproducible qelectrotech)
target_link_libraries(tst_pdfreproducible PRIVATE Qt::Test)
target_compile_definitions(tst_pdfreproducible PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
  "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

# Hops where wires cross (issue #436): the geometry in wirehops.cpp, and
# through the real binary, that --resave keeps the project setting and
# adds nothing to a project without it.
add_executable(
  tst_wirehops
  tst_wirehops.cpp
  ${QET_DIR}/sources/wirehops.cpp)
add_test(NAME tst_wirehops COMMAND tst_wirehops)
add_dependencies(tst_wirehops qelectrotech)
target_include_directories(tst_wirehops PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_wirehops PRIVATE Qt::Test Qt::Gui)
target_compile_definitions(tst_wirehops PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# Wires per terminal (discussion #1158): the rules in wiringrules.cpp, and
# through the real binary, that --resave keeps the project setting, adds
# none to a project without it, and that a wire past the limit is refused
# unless the master switch is off.
add_executable(
  tst_wiringrules
  tst_wiringrules.cpp
  ${QET_DIR}/sources/wiringrules.cpp)
add_test(NAME tst_wiringrules COMMAND tst_wiringrules)
add_dependencies(tst_wiringrules qelectrotech)
target_include_directories(tst_wiringrules PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_wiringrules PRIVATE Qt::Test Qt::Xml)
target_compile_definitions(tst_wiringrules PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# The routing behind qet.routeConductor(): conductorrouter.cpp needs only
# Qt Core, so the geometry is tested without a scene.
add_executable(
  tst_conductorrouter
  tst_conductorrouter.cpp
  ${QET_DIR}/sources/conductorrouter.cpp)
add_test(NAME tst_conductorrouter COMMAND tst_conductorrouter)
target_include_directories(tst_conductorrouter PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_conductorrouter PRIVATE Qt::Test)

# qet.conductorUuids() / qet.conductorEnds(): a script lists a folio's
# conductors by uuid and finds each one's two ends. Runs a script through
# the real binary's --run on fixtures/qet_bug_repro_resaved.qet, so only
# where --run exists: a build without the Qt Qml module has no scripting
# (see QET_HAS_SCRIPTING in the top-level CMakeLists.txt).
if(QET_HAS_SCRIPTING)
  add_executable(
    tst_scriptconductoruuid
    tst_scriptconductoruuid.cpp)
  add_test(NAME tst_scriptconductoruuid COMMAND tst_scriptconductoruuid)
  add_dependencies(tst_scriptconductoruuid qelectrotech)
  target_link_libraries(tst_scriptconductoruuid PRIVATE Qt::Test)
  target_compile_definitions(tst_scriptconductoruuid PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

  # qet.terminalPosition() and qet.conductorPath(), checked against each
  # other on a fixture's conductors.
  add_executable(
    tst_scriptlayoutreads
    tst_scriptlayoutreads.cpp)
  add_test(NAME tst_scriptlayoutreads COMMAND tst_scriptlayoutreads)
  add_dependencies(tst_scriptlayoutreads qelectrotech)
  target_link_libraries(tst_scriptlayoutreads PRIVATE Qt::Test)
  target_compile_definitions(tst_scriptlayoutreads PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

  # The project database keeps the width of a free text (independent_text),
  # read with qet.query() on fixtures/free_text_width.qet.
  add_executable(
    tst_scriptfreetextwidth
    tst_scriptfreetextwidth.cpp)
  add_test(NAME tst_scriptfreetextwidth COMMAND tst_scriptfreetextwidth)
  add_dependencies(tst_scriptfreetextwidth qelectrotech)
  target_link_libraries(tst_scriptfreetextwidth PRIVATE Qt::Test)
  target_compile_definitions(tst_scriptfreetextwidth PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

  # QET_SETTINGS_DIR moves the settings into an INI file there (#1178): a
  # script places a symbol only the folder's settings file can resolve.
  add_executable(
    tst_settingsdir
    tst_settingsdir.cpp)
  add_test(NAME tst_settingsdir COMMAND tst_settingsdir)
  add_dependencies(tst_settingsdir qelectrotech)
  target_link_libraries(tst_settingsdir PRIVATE Qt::Test)
  target_compile_definitions(tst_settingsdir PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_ELEMENTS_DIR=\"${QET_DIR}/elements\"")

  # qet.addElement() on a symbol that exists but cannot be read says so,
  # rather than reporting a collision with the copy already embedded.
  add_executable(
    tst_unreadableelement
    tst_unreadableelement.cpp)
  add_test(NAME tst_unreadableelement COMMAND tst_unreadableelement)
  add_dependencies(tst_unreadableelement qelectrotech)
  target_link_libraries(tst_unreadableelement PRIVATE Qt::Test)
  target_compile_definitions(tst_unreadableelement PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_ELEMENTS_DIR=\"${QET_DIR}/elements\"")

  # The project database filled from the file as it is read holds exactly
  # what it holds filled from the built folios (QET_DATABASE_FROM_FOLIOS=1),
  # on every example saved once; an older file falls back, saying why.
  add_executable(
    tst_databasefromdocument
    tst_databasefromdocument.cpp)
  add_test(NAME tst_databasefromdocument COMMAND tst_databasefromdocument)
  add_dependencies(tst_databasefromdocument qelectrotech)
  target_link_libraries(tst_databasefromdocument PRIVATE Qt::Test Qt::Xml)
  target_compile_definitions(tst_databasefromdocument PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

  # The project database keeps a folio's date with its century: 741.qet's
  # 2010-09-21, read through project_summary_view under LC_ALL=en_US.UTF-8.
  add_executable(
    tst_dbfoliodate
    tst_dbfoliodate.cpp)
  add_test(NAME tst_dbfoliodate COMMAND tst_dbfoliodate)
  add_dependencies(tst_dbfoliodate qelectrotech)
  target_link_libraries(tst_dbfoliodate PRIVATE Qt::Test)
  target_compile_definitions(tst_dbfoliodate PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

  # qet.elementTerminals() lists each terminal's uuid and qet.terminalIndex()
  # turns element uuid + terminal uuid back into the index the conductor
  # calls take, on examples/perceuse.qet (two terminals at one point).
  add_executable(
    tst_scriptterminaluuid
    tst_scriptterminaluuid.cpp)
  add_test(NAME tst_scriptterminaluuid COMMAND tst_scriptterminaluuid)
  add_dependencies(tst_scriptterminaluuid qelectrotech)
  target_link_libraries(tst_scriptterminaluuid PRIVATE Qt::Test Qt::Xml)
  target_compile_definitions(tst_scriptterminaluuid PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

  # Undoing a crop of a picture restores its crop rectangle too, so a
  # project saved after the undo does not record the undone crop.
  add_executable(
    tst_imagecropundo
    tst_imagecropundo.cpp)
  add_test(NAME tst_imagecropundo COMMAND tst_imagecropundo)
  add_dependencies(tst_imagecropundo qelectrotech)
  target_link_libraries(tst_imagecropundo PRIVATE Qt::Test Qt::Gui)
  target_compile_definitions(tst_imagecropundo PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

  # The same symbol placed twice in a project whose collection does not
  # start with its "import" category (examples/lmdg.qet).
  add_executable(
    tst_importcategory
    tst_importcategory.cpp)
  add_test(NAME tst_importcategory COMMAND tst_importcategory)
  add_dependencies(tst_importcategory qelectrotech)
  target_link_libraries(tst_importcategory PRIVATE Qt::Test)
  target_compile_definitions(tst_importcategory PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\""
    "QET_ELEMENTS_DIR=\"${QET_DIR}/elements\"")

  # qet.currentFolio() through --run: the first folio, or -1 with none; and
  # --run keeps one undo step per call (only an editor run is grouped).
  add_executable(
    tst_scriptcurrentfolio
    tst_scriptcurrentfolio.cpp)
  add_test(NAME tst_scriptcurrentfolio COMMAND tst_scriptcurrentfolio)
  add_dependencies(tst_scriptcurrentfolio qelectrotech)
  target_link_libraries(tst_scriptcurrentfolio PRIVATE Qt::Test)
  target_compile_definitions(tst_scriptcurrentfolio PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")
endif()

# A symbol saved without a uuid gets the same one on every load, and
# inserting a folio does not change it. Runs the real binary's --resave on
# fixtures/qet_bug_repro_resaved.qet with the symbols' uuids stripped.
add_executable(
  tst_derivedsymboluuid
  tst_derivedsymboluuid.cpp)
add_test(NAME tst_derivedsymboluuid COMMAND tst_derivedsymboluuid)
add_dependencies(tst_derivedsymboluuid qelectrotech)
target_link_libraries(tst_derivedsymboluuid PRIVATE Qt::Test Qt::Xml)
target_compile_definitions(tst_derivedsymboluuid PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\"")

# A wire saved without a uuid gets one derived from its two ends: the same
# on every load, saved, and unchanged by inserting a folio, reordering the
# wires or drawing a wire the other way. Runs the real binary's --resave on
# a fixture and on examples/tremie_vibrante.qet (ends by terminal number).
add_executable(
  tst_derivedwireuuid
  tst_derivedwireuuid.cpp)
add_test(NAME tst_derivedwireuuid COMMAND tst_derivedwireuuid)
add_dependencies(tst_derivedwireuuid qelectrotech)
target_link_libraries(tst_derivedwireuuid PRIVATE Qt::Test Qt::Xml)
target_compile_definitions(tst_derivedwireuuid PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
  "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

# Element numbering schemes carry an id and elements follow them by it:
# a legacy file gets stable ids on --resave; through --run, rename, edit,
# refused removal and undo/redo keep the elements following the scheme.
if(QET_HAS_SCRIPTING)
  add_executable(
    tst_elementautonumids
    tst_elementautonumids.cpp)
  add_test(NAME tst_elementautonumids COMMAND tst_elementautonumids)
  add_dependencies(tst_elementautonumids qelectrotech)
  target_link_libraries(tst_elementautonumids PRIVATE Qt::Test Qt::Xml)
  target_compile_definitions(tst_elementautonumids PRIVATE
    "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
    "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")
endif()

# Saving a project that was just saved changes nothing: runs the real
# binary's --resave twice on every project in examples/, and on one whose
# title block holds a single-space value (#973).

add_executable(
  tst_resaveunchanged
  tst_resaveunchanged.cpp)
add_test(NAME tst_resaveunchanged COMMAND tst_resaveunchanged)
add_dependencies(tst_resaveunchanged qelectrotech)
target_link_libraries(tst_resaveunchanged PRIVATE Qt::Test)
target_compile_definitions(tst_resaveunchanged PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
  "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

# Replacing a symbol's definition in a project keeps its terminal uuids, so
# the wires saved against them still load: TerminalUuids::keep() on its own,
# then the real binary's --info on an example whose symbols were replaced.
add_executable(
  tst_terminaluuids
  tst_terminaluuids.cpp
  ${QET_DIR}/sources/ElementsCollection/terminaluuids.cpp)
add_test(NAME tst_terminaluuids COMMAND tst_terminaluuids)
add_dependencies(tst_terminaluuids qelectrotech)
target_link_libraries(tst_terminaluuids PRIVATE Qt::Test Qt::Xml)
target_compile_definitions(tst_terminaluuids PRIVATE
  "QET_TEST_BINARY_PATH=\"$<TARGET_FILE:qelectrotech>\""
  "QET_EXAMPLES_DIR=\"${QET_DIR}/examples\"")

# ConductorProperties::applyChanges() -- the Selection properties panel
# writes only the fields the user edited.
add_executable(
  tst_conductorapplychanges
  tst_conductorapplychanges.cpp
  ${QET_DIR}/sources/conductorproperties.cpp
  ${QET_DIR}/sources/qet.cpp
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/shortcutmanager.cpp)
add_test(NAME tst_conductorapplychanges COMMAND tst_conductorapplychanges)
target_include_directories(tst_conductorapplychanges PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_conductorapplychanges PRIVATE Qt::Test Qt::Widgets Qt::Xml pugixml::pugixml)

# ConductorMultiEdit -- editing several wires at once in the Selection
# properties panel: order, which wires an edit reaches, one undo step for
# all of them, and fields shown blank because the wires disagree.
add_executable(
  tst_conductormultiedit
  tst_conductormultiedit.cpp
  ${QET_DIR}/sources/conductorproperties.cpp
  ${QET_DIR}/sources/QPropertyUndoCommand/qpropertyundocommand.cpp
  ${QET_DIR}/sources/qet.cpp
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/shortcutmanager.cpp)
add_test(NAME tst_conductormultiedit COMMAND tst_conductormultiedit)
target_include_directories(tst_conductormultiedit PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_conductormultiedit PRIVATE Qt::Test Qt::Widgets Qt::Xml pugixml::pugixml)

# DiagramContext::fromXml() -- the two readers (QDom for projects, pugixml
# for element definitions in the collection) give the same values: stray
# spaces trimmed, accents kept.
add_executable(
  tst_diagramcontext
  tst_diagramcontext.cpp
  ${QET_DIR}/sources/diagramcontext.cpp
  ${QET_DIR}/sources/qet.cpp
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/shortcutmanager.cpp)
add_test(NAME tst_diagramcontext COMMAND tst_diagramcontext)
target_include_directories(tst_diagramcontext PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_diagramcontext PRIVATE Qt::Test Qt::Widgets Qt::Xml pugixml::pugixml)

# XRefProperties: an empty or unknown cross-reference position, as older
# versions saved it, reads as the default instead of drawing the
# cross-reference over the element's label (issue #1238).
add_executable(
  tst_xrefpos
  tst_xrefpos.cpp
  ${QET_DIR}/sources/properties/xrefproperties.cpp
  ${QET_DIR}/sources/properties/propertiesinterface.cpp)
add_test(NAME tst_xrefpos COMMAND tst_xrefpos)
target_include_directories(tst_xrefpos PRIVATE ${QET_DIR}/sources)
target_link_libraries(tst_xrefpos PRIVATE Qt::Test Qt::Widgets Qt::Xml pugixml::pugixml)

# Qt 6.4.2 writes the attributes of an element in an order that changes from
# one process to the next, so a project saved twice by two runs of the binary
# differs in bytes only (seen on projects without any numbering, too). A fixed
# hash seed makes the order stable; it changes nothing for a newer Qt.
if(Qt6_VERSION VERSION_LESS 6.5.0)
  foreach(qet_test tst_resaveunchanged tst_elementautonumids)
    if(TEST ${qet_test})
      set_tests_properties(${qet_test} PROPERTIES ENVIRONMENT "QT_HASH_SEED=0")
    endif()
  endforeach()
endif()
