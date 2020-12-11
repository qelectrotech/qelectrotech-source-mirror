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

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

message(
  "_____________________________________________________________________"
  "\nCopyright 2006 The QElectroTech Team"
  "\nThis file is part of QElectroTech."

  "\n\nQElectroTech is free software: you can redistribute it and/or modify"
  "\nit under the terms of the GNU General Public License as published by"
  "\nthe Free Software Foundation, either version 2 of the License, or"
  "\n(at your option) any later version."

  "\nQElectroTech is distributed in the hope that it will be useful,"
  "\nbut WITHOUT ANY WARRANTY; without even the implied warranty of"
  "\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
  "\nGNU General Public License for more details."

  "\n\nYou should have received a copy of the GNU General Public License"
  "\nalong with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.\n"
  "_____________________________________________________________________")

project(qelectrotech
  VERSION 0.8.0
  DESCRIPTION "QET is a CAD/CAE editor focusing on schematics drawing features."
  HOMEPAGE_URL "https://qelectrotech.org/"
  LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QET_DIR ${PROJECT_SOURCE_DIR})

# Paths used for compilation and installation of QET
if(UNIX AND NOT APPLE)
  # for Linux, BSD, Solaris, Minix
  set(COMPIL_PREFIX               "/usr/local/")
  set(INSTALL_PREFIX              "/usr/local/")
  set(QET_BINARY_PATH             "bin/")
  set(QET_COMMON_COLLECTION_PATH  "share/qelectrotech/elements/")
  set(QET_COMMON_TBT_PATH         "share/qelectrotech/titleblocks/")
  set(QET_LANG_PATH               "share/qelectrotech/lang/")
  set(QET_EXAMPLES_PATH           "share/qelectrotech/examples/")
  set(QET_LICENSE_PATH            "doc/qelectrotech/")
  set(QET_MIME_XML_PATH           "../share/mime/application/")
  set(QET_MIME_DESKTOP_PATH       "../share/mimelnk/application/")
  set(QET_MIME_PACKAGE_PATH       "../share/mime/packages/")
  set(QET_DESKTOP_PATH            "share/applications/")
  set(QET_ICONS_PATH              "share/icons/hicolor/")
  set(QET_MAN_PATH                "man/")
  set(QET_APPDATA_PATH            "share/appdata")
endif()

if(APPLE)
  # for MacOS X or iOS, watchOS, tvOS (since 3.10.3)
  set(COMPIL_PREFIX               "./")
  set(INSTALL_PREFIX              "/usr/local/")
  set(QET_BINARY_PATH             "bin/")
  set(QET_COMMON_COLLECTION_PATH  "../Resources/elements/")
  set(QET_COMMON_TBT_PATH         "../Resources/titleblocks/")
  set(QET_LANG_PATH               "../Resources/lang/")
  set(QET_EXAMPLES_PATH           "share/qelectrotech/examples/")
  set(QET_LICENSE_PATH            "doc/qelectrotech/")
  set(QET_MIME_XML_PATH           "../share/mime/application/")
  set(QET_MIME_DESKTOP_PATH       "../share/mimelnk/application/")
  set(QET_DESKTOP_PATH            "share/applications/")
  set(QET_ICONS_PATH              "share/icons/hicolor/")
  set(QET_MAN_PATH                "man/")
  set(ICON                        "ico/mac_icon/qelectrotech.icns")
endif()

if(WIN32)
  # for Windows operating system in general
  set(COMPIL_PREFIX               "./")
  set(INSTALL_PREFIX              "./")
  set(QET_BINARY_PATH             "./")
  set(QET_COMMON_COLLECTION_PATH  "elements/")
  set(QET_COMMON_TBT_PATH         "titleblocks/")
  set(QET_LANG_PATH               "lang/")
  set(QET_LICENSE_PATH            "./")
  # Liste des ressources Windows
#RC_FILE = qelectrotech.rc
endif()
# Commenter la ligne ci-dessous pour desactiver l'option --common-elements-dir
add_definitions(-DQET_ALLOW_OVERRIDE_CED_OPTION)

# Comment the line below to disable the --common-tbt-dir option
add_definitions(-DQET_ALLOW_OVERRIDE_CTBTD_OPTION)

# Commenter la ligne ci-dessous pour desactiver l'option --config-dir
add_definitions(-DQET_ALLOW_OVERRIDE_CD_OPTION)

#comment the line below to disable the project database export
#add_definitions(-DQET_EXPORT_PROJECT_DB) #error Todo

# warn on *any* usage of deprecated APIs
add_definitions(-DQT_DEPRECATED_WARNINGS)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000 # disables all the APIs deprecated before Qt 6.0.0
set(QT_DISABLE_DEPRECATED_BEFORE 0x060000)

add_definitions(-DQT_MESSAGELOGCONTEXT)

# You can make your code warn on compile time for the TODO's
# In order to do so, uncomment the following line.
#add_definitions(-DTODO_LIST)

# From De-Backer ik ben nog aan het testen.
set(BUILD_QET_YES "YES")

# https://github.com/itay-grudev/SingleApplication/issues/18
#qmake
#DEFINES += QAPPLICATION_CLASS=QGuiApplication
set(QAPPLICATION_CLASS QApplication)

find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  # updates all git submodules
  execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SUBMOD_RESULT)

  if(NOT GIT_SUBMOD_RESULT EQUAL "0")
    message(
      FATAL_ERROR
      "git submodule update --init failed with "
      ${GIT_SUBMOD_RESULT}
      ", please checkout submodules")
  endif()
endif()

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  #get GIT COMMIT SHA
  execute_process(
    COMMAND ${GIT_EXECUTABLE} -C ${QET_DIR} rev-parse --verify HEAD
    OUTPUT_VARIABLE  GIT_COMMIT_SHA
    RESULT_VARIABLE GIT_SUBMOD_RESULT)

  # This strips terminating newline in the variable
  string(REGEX REPLACE "\n$" "" GIT_COMMIT_SHA "${GIT_COMMIT_SHA}")

  # This adds to definitions => .cpp
  add_definitions(-DGIT_COMMIT_SHA="${GIT_COMMIT_SHA}")

  if(NOT GIT_SUBMOD_RESULT EQUAL "0")
    message(
      FATAL_ERROR
      "git rev-parse --verify HEAD failed with "
      ${GIT_SUBMOD_RESULT}
      ", please check")
  endif()
endif()

message("PROJECT_NAME       :" ${PROJECT_NAME})
message("PROJECT_SOURCE_DIR :" ${PROJECT_SOURCE_DIR})
message("QET_DIR            :" ${QET_DIR})
message("GIT_COMMIT_SHA     :" ${GIT_COMMIT_SHA})

Include(FetchContent)

#ToDo De-Backer
set(BUILD_KF5_YES "YES")

if(DEFINED BUILD_KF5_YES)

  if(NOT DEFINED KF5_GIT_TAG)
    #https://qelectrotech.org/forum/viewtopic.php?pid=13924#p13924
    set(KF5_GIT_TAG v5.70.0)
    message("KF5_GIT_TAG        :" ${KF5_GIT_TAG})
  endif()

  #---ToDo why you don't work :/
  set(BUILD_TESTING "0")
  FetchContent_Declare(
    kcoreaddons
    GIT_REPOSITORY https://invent.kde.org/frameworks/kcoreaddons.git
    GIT_TAG        ${KF5_GIT_TAG})
  FetchContent_MakeAvailable(kcoreaddons)
  unset(BUILD_TESTING)
  #___ToDo

  FetchContent_Declare(
    kwidgetsaddons
    GIT_REPOSITORY https://invent.kde.org/frameworks/kwidgetsaddons.git
    GIT_TAG        ${KF5_GIT_TAG})
  FetchContent_MakeAvailable(kwidgetsaddons)
else()
  find_package(KF5CoreAddons REQUIRED)
  find_package(KF5WidgetsAddons REQUIRED)
endif()

FetchContent_Declare(
  SingleApplication
  GIT_REPOSITORY https://github.com/itay-grudev/SingleApplication.git
  GIT_TAG        v3.1.5)

FetchContent_MakeAvailable(SingleApplication)

FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        v1.11.1)

FetchContent_MakeAvailable(pugixml)

set(QET_COMPONENTS
  Widgets
  Concurrent
  Xml
  Svg
  Network
  Sql
  PrintSupport
  LinguistTools)

message("QET_COMPONENTS     :" ${QET_COMPONENTS})

find_package(
  QT
 NAMES
  Qt6
  Qt5
 COMPONENTS
  ${QET_COMPONENTS}
 REQUIRED
 )
message("QT_VERSION_MAJOR   :" ${QT_VERSION_MAJOR})

find_package(
  Qt${QT_VERSION_MAJOR}
 COMPONENTS
 ${QET_COMPONENTS}
 REQUIRED)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${QET_DIR}/sources/ui)

set(QET_SRC_FILES
  ${QET_DIR}/sources/borderproperties.cpp
  ${QET_DIR}/sources/borderproperties.h
  ${QET_DIR}/sources/bordertitleblock.cpp
  ${QET_DIR}/sources/bordertitleblock.h
  ${QET_DIR}/sources/conductorautonumerotation.cpp
  ${QET_DIR}/sources/conductorautonumerotation.h
  ${QET_DIR}/sources/conductornumexport.cpp
  ${QET_DIR}/sources/conductornumexport.h
  ${QET_DIR}/sources/conductorprofile.cpp
  ${QET_DIR}/sources/conductorprofile.h
  ${QET_DIR}/sources/conductorproperties.cpp
  ${QET_DIR}/sources/conductorproperties.h
  ${QET_DIR}/sources/conductorsegment.cpp
  ${QET_DIR}/sources/conductorsegment.h
  ${QET_DIR}/sources/conductorsegmentprofile.h
  ${QET_DIR}/sources/configdialog.cpp
  ${QET_DIR}/sources/configdialog.h
  ${QET_DIR}/sources/configpage.h
  ${QET_DIR}/sources/configpages.cpp
  ${QET_DIR}/sources/configpages.h
  ${QET_DIR}/sources/createdxf.cpp
  ${QET_DIR}/sources/createdxf.h
  ${QET_DIR}/sources/diagramcommands.cpp
  ${QET_DIR}/sources/diagramcommands.h
  ${QET_DIR}/sources/diagramcontent.cpp
  ${QET_DIR}/sources/diagramcontent.h
  ${QET_DIR}/sources/diagramcontext.cpp
  ${QET_DIR}/sources/diagramcontext.h
  ${QET_DIR}/sources/diagram.cpp
  ${QET_DIR}/sources/diagram.h
  ${QET_DIR}/sources/diagramposition.cpp
  ${QET_DIR}/sources/diagramposition.h
  ${QET_DIR}/sources/diagramview.cpp
  ${QET_DIR}/sources/diagramview.h
  ${QET_DIR}/sources/elementdialog.cpp
  ${QET_DIR}/sources/elementdialog.h
  ${QET_DIR}/sources/elementprovider.cpp
  ${QET_DIR}/sources/elementprovider.h
  ${QET_DIR}/sources/elementscategoryeditor.cpp
  ${QET_DIR}/sources/elementscategoryeditor.h
  ${QET_DIR}/sources/elementscollectioncache.cpp
  ${QET_DIR}/sources/elementscollectioncache.h
  ${QET_DIR}/sources/elementsmover.cpp
  ${QET_DIR}/sources/elementsmover.h
  ${QET_DIR}/sources/elementspanel.cpp
  ${QET_DIR}/sources/elementspanel.h
  ${QET_DIR}/sources/elementspanelwidget.cpp
  ${QET_DIR}/sources/elementspanelwidget.h
  ${QET_DIR}/sources/elementtextpattern.cpp
  ${QET_DIR}/sources/elementtextpattern.h
  ${QET_DIR}/sources/elementtextsmover.cpp
  ${QET_DIR}/sources/elementtextsmover.h
  ${QET_DIR}/sources/exportdialog.cpp
  ${QET_DIR}/sources/exportdialog.h
  ${QET_DIR}/sources/exportproperties.cpp
  ${QET_DIR}/sources/exportproperties.h
  ${QET_DIR}/sources/exportpropertieswidget.cpp
  ${QET_DIR}/sources/exportpropertieswidget.h
  ${QET_DIR}/sources/genericpanel.cpp
  ${QET_DIR}/sources/genericpanel.h
  ${QET_DIR}/sources/machine_info.cpp
  ${QET_DIR}/sources/machine_info.h
  ${QET_DIR}/sources/main.cpp
  ${QET_DIR}/sources/newelementwizard.cpp
  ${QET_DIR}/sources/newelementwizard.h
  ${QET_DIR}/sources/projectconfigpages.cpp
  ${QET_DIR}/sources/projectconfigpages.h
  ${QET_DIR}/sources/projectview.cpp
  ${QET_DIR}/sources/projectview.h
  ${QET_DIR}/sources/qetapp.cpp
  ${QET_DIR}/sources/qetapp.h
  ${QET_DIR}/sources/qetarguments.cpp
  ${QET_DIR}/sources/qetarguments.h
  ${QET_DIR}/sources/qet.cpp
  ${QET_DIR}/sources/qetdiagrameditor.cpp
  ${QET_DIR}/sources/qetdiagrameditor.h
  ${QET_DIR}/sources/qet.h
  ${QET_DIR}/sources/qeticons.cpp
  ${QET_DIR}/sources/qeticons.h
  ${QET_DIR}/sources/qetinformation.cpp
  ${QET_DIR}/sources/qetinformation.h
  ${QET_DIR}/sources/qetmainwindow.cpp
  ${QET_DIR}/sources/qetmainwindow.h
  ${QET_DIR}/sources/qetmessagebox.cpp
  ${QET_DIR}/sources/qetmessagebox.h
  ${QET_DIR}/sources/qetproject.cpp
  ${QET_DIR}/sources/qetproject.h
  ${QET_DIR}/sources/qetregexpvalidator.cpp
  ${QET_DIR}/sources/qetregexpvalidator.h
  ${QET_DIR}/sources/qetresult.cpp
  ${QET_DIR}/sources/qetresult.h
  ${QET_DIR}/sources/qetxml.cpp
  ${QET_DIR}/sources/qetxml.h
  ${QET_DIR}/sources/qfilenameedit.cpp
  ${QET_DIR}/sources/qfilenameedit.h
  ${QET_DIR}/sources/qgimanager.cpp
  ${QET_DIR}/sources/qgimanager.h
  ${QET_DIR}/sources/qtextorientationspinboxwidget.cpp
  ${QET_DIR}/sources/qtextorientationspinboxwidget.h
  ${QET_DIR}/sources/qtextorientationwidget.cpp
  ${QET_DIR}/sources/qtextorientationwidget.h
  ${QET_DIR}/sources/recentfiles.cpp
  ${QET_DIR}/sources/recentfiles.h
  ${QET_DIR}/sources/titleblockcell.cpp
  ${QET_DIR}/sources/titleblockcell.h
  ${QET_DIR}/sources/titleblockproperties.cpp
  ${QET_DIR}/sources/titleblockproperties.h
  ${QET_DIR}/sources/titleblocktemplate.cpp
  ${QET_DIR}/sources/titleblocktemplate.h
  ${QET_DIR}/sources/titleblocktemplaterenderer.cpp
  ${QET_DIR}/sources/titleblocktemplaterenderer.h

  ${QET_DIR}/sources/autoNum/assignvariables.cpp
  ${QET_DIR}/sources/autoNum/assignvariables.h
  ${QET_DIR}/sources/autoNum/numerotationcontextcommands.cpp
  ${QET_DIR}/sources/autoNum/numerotationcontextcommands.h
  ${QET_DIR}/sources/autoNum/numerotationcontext.cpp
  ${QET_DIR}/sources/autoNum/numerotationcontext.h
  ${QET_DIR}/sources/autoNum/ui/autonumberingdockwidget.cpp
  ${QET_DIR}/sources/autoNum/ui/autonumberingdockwidget.h
  ${QET_DIR}/sources/autoNum/ui/autonumberingmanagementw.cpp
  ${QET_DIR}/sources/autoNum/ui/autonumberingmanagementw.h
  ${QET_DIR}/sources/autoNum/ui/folioautonumbering.cpp
  ${QET_DIR}/sources/autoNum/ui/folioautonumbering.h
  ${QET_DIR}/sources/autoNum/ui/formulaautonumberingw.cpp
  ${QET_DIR}/sources/autoNum/ui/formulaautonumberingw.h
  ${QET_DIR}/sources/autoNum/ui/numparteditorw.cpp
  ${QET_DIR}/sources/autoNum/ui/numparteditorw.h
  ${QET_DIR}/sources/autoNum/ui/selectautonumw.cpp
  ${QET_DIR}/sources/autoNum/ui/selectautonumw.h

  ${QET_DIR}/sources/dataBase/projectdatabase.cpp
  ${QET_DIR}/sources/dataBase/projectdatabase.h

  ${QET_DIR}/sources/dataBase/ui/elementquerywidget.cpp
  ${QET_DIR}/sources/dataBase/ui/elementquerywidget.h
  ${QET_DIR}/sources/dataBase/ui/summaryquerywidget.cpp
  ${QET_DIR}/sources/dataBase/ui/summaryquerywidget.h

  ${QET_DIR}/sources/diagramevent/diagrameventaddelement.cpp
  ${QET_DIR}/sources/diagramevent/diagrameventaddelement.h
  ${QET_DIR}/sources/diagramevent/diagrameventaddimage.cpp
  ${QET_DIR}/sources/diagramevent/diagrameventaddimage.h
  ${QET_DIR}/sources/diagramevent/diagrameventaddshape.cpp
  ${QET_DIR}/sources/diagramevent/diagrameventaddshape.h
  ${QET_DIR}/sources/diagramevent/diagrameventaddtext.cpp
  ${QET_DIR}/sources/diagramevent/diagrameventaddtext.h
  ${QET_DIR}/sources/diagramevent/diagrameventinterface.cpp
  ${QET_DIR}/sources/diagramevent/diagrameventinterface.h

  ${QET_DIR}/sources/dvevent/dveventinterface.cpp
  ${QET_DIR}/sources/dvevent/dveventinterface.h

  ${QET_DIR}/sources/editor/arceditor.cpp
  ${QET_DIR}/sources/editor/arceditor.h
  ${QET_DIR}/sources/editor/editorcommands.cpp
  ${QET_DIR}/sources/editor/editorcommands.h
  ${QET_DIR}/sources/editor/elementcontent.h
  ${QET_DIR}/sources/editor/elementitemeditor.cpp
  ${QET_DIR}/sources/editor/elementitemeditor.h
  ${QET_DIR}/sources/editor/elementprimitivedecorator.cpp
  ${QET_DIR}/sources/editor/elementprimitivedecorator.h
  ${QET_DIR}/sources/editor/elementscene.cpp
  ${QET_DIR}/sources/editor/elementscene.h
  ${QET_DIR}/sources/editor/elementview.cpp
  ${QET_DIR}/sources/editor/elementview.h
  ${QET_DIR}/sources/editor/qetelementeditor.cpp
  ${QET_DIR}/sources/editor/qetelementeditor.h
  ${QET_DIR}/sources/editor/styleeditor.cpp
  ${QET_DIR}/sources/editor/styleeditor.h
  ${QET_DIR}/sources/editor/terminaleditor.cpp
  ${QET_DIR}/sources/editor/terminaleditor.h

  ${QET_DIR}/sources/editor/esevent/eseventaddarc.cpp
  ${QET_DIR}/sources/editor/esevent/eseventaddarc.h
  ${QET_DIR}/sources/editor/esevent/eseventadddynamictextfield.cpp
  ${QET_DIR}/sources/editor/esevent/eseventadddynamictextfield.h
  ${QET_DIR}/sources/editor/esevent/eseventaddellipse.cpp
  ${QET_DIR}/sources/editor/esevent/eseventaddellipse.h
  ${QET_DIR}/sources/editor/esevent/eseventaddline.cpp
  ${QET_DIR}/sources/editor/esevent/eseventaddline.h
  ${QET_DIR}/sources/editor/esevent/eseventaddpolygon.cpp
  ${QET_DIR}/sources/editor/esevent/eseventaddpolygon.h
  ${QET_DIR}/sources/editor/esevent/eseventaddrect.cpp
  ${QET_DIR}/sources/editor/esevent/eseventaddrect.h
  ${QET_DIR}/sources/editor/esevent/eseventaddterminal.cpp
  ${QET_DIR}/sources/editor/esevent/eseventaddterminal.h
  ${QET_DIR}/sources/editor/esevent/eseventaddtext.cpp
  ${QET_DIR}/sources/editor/esevent/eseventaddtext.h
  ${QET_DIR}/sources/editor/esevent/eseventinterface.cpp
  ${QET_DIR}/sources/editor/esevent/eseventinterface.h

  ${QET_DIR}/sources/editor/graphicspart/abstractpartellipse.cpp
  ${QET_DIR}/sources/editor/graphicspart/abstractpartellipse.h
  ${QET_DIR}/sources/editor/graphicspart/customelementgraphicpart.cpp
  ${QET_DIR}/sources/editor/graphicspart/customelementgraphicpart.h
  ${QET_DIR}/sources/editor/graphicspart/customelementpart.cpp
  ${QET_DIR}/sources/editor/graphicspart/customelementpart.h
  ${QET_DIR}/sources/editor/graphicspart/partarc.cpp
  ${QET_DIR}/sources/editor/graphicspart/partarc.h
  ${QET_DIR}/sources/editor/graphicspart/partdynamictextfield.cpp
  ${QET_DIR}/sources/editor/graphicspart/partdynamictextfield.h
  ${QET_DIR}/sources/editor/graphicspart/partellipse.cpp
  ${QET_DIR}/sources/editor/graphicspart/partellipse.h
  ${QET_DIR}/sources/editor/graphicspart/partline.cpp
  ${QET_DIR}/sources/editor/graphicspart/partline.h
  ${QET_DIR}/sources/editor/graphicspart/partpolygon.cpp
  ${QET_DIR}/sources/editor/graphicspart/partpolygon.h
  ${QET_DIR}/sources/editor/graphicspart/partrectangle.cpp
  ${QET_DIR}/sources/editor/graphicspart/partrectangle.h
  ${QET_DIR}/sources/editor/graphicspart/partterminal.cpp
  ${QET_DIR}/sources/editor/graphicspart/partterminal.h
  ${QET_DIR}/sources/editor/graphicspart/parttext.cpp
  ${QET_DIR}/sources/editor/graphicspart/parttext.h
  ${QET_DIR}/sources/editor/ui/dynamictextfieldeditor.cpp
  ${QET_DIR}/sources/editor/ui/dynamictextfieldeditor.h
  ${QET_DIR}/sources/editor/ui/elementpropertieseditorwidget.cpp
  ${QET_DIR}/sources/editor/ui/elementpropertieseditorwidget.h
  ${QET_DIR}/sources/editor/ui/ellipseeditor.cpp
  ${QET_DIR}/sources/editor/ui/ellipseeditor.h
  ${QET_DIR}/sources/editor/ui/lineeditor.cpp
  ${QET_DIR}/sources/editor/ui/lineeditor.h
  ${QET_DIR}/sources/editor/ui/polygoneditor.cpp
  ${QET_DIR}/sources/editor/ui/polygoneditor.h
  ${QET_DIR}/sources/editor/ui/rectangleeditor.cpp
  ${QET_DIR}/sources/editor/ui/rectangleeditor.h
  ${QET_DIR}/sources/editor/ui/texteditor.cpp
  ${QET_DIR}/sources/editor/ui/texteditor.h

  ${QET_DIR}/sources/editor/UndoCommand/pastepartscommand.cpp
  ${QET_DIR}/sources/editor/UndoCommand/pastepartscommand.h

  ${QET_DIR}/sources/ElementsCollection/elementcollectionhandler.cpp
  ${QET_DIR}/sources/ElementsCollection/elementcollectionhandler.h
  ${QET_DIR}/sources/ElementsCollection/elementcollectionitem.cpp
  ${QET_DIR}/sources/ElementsCollection/elementcollectionitem.h
  ${QET_DIR}/sources/ElementsCollection/elementscollectionmodel.cpp
  ${QET_DIR}/sources/ElementsCollection/elementscollectionmodel.h
  ${QET_DIR}/sources/ElementsCollection/elementscollectionwidget.cpp
  ${QET_DIR}/sources/ElementsCollection/elementscollectionwidget.h
  ${QET_DIR}/sources/ElementsCollection/elementslocation.cpp
  ${QET_DIR}/sources/ElementsCollection/elementslocation.h
  ${QET_DIR}/sources/ElementsCollection/elementstreeview.cpp
  ${QET_DIR}/sources/ElementsCollection/elementstreeview.h
  ${QET_DIR}/sources/ElementsCollection/fileelementcollectionitem.cpp
  ${QET_DIR}/sources/ElementsCollection/fileelementcollectionitem.h
  ${QET_DIR}/sources/ElementsCollection/xmlelementcollection.cpp
  ${QET_DIR}/sources/ElementsCollection/xmlelementcollection.h
  ${QET_DIR}/sources/ElementsCollection/xmlprojectelementcollectionitem.cpp
  ${QET_DIR}/sources/ElementsCollection/xmlprojectelementcollectionitem.h

  ${QET_DIR}/sources/ElementsCollection/ui/renamedialog.cpp
  ${QET_DIR}/sources/ElementsCollection/ui/renamedialog.h

  ${QET_DIR}/sources/factory/elementfactory.cpp
  ${QET_DIR}/sources/factory/elementfactory.h
  ${QET_DIR}/sources/factory/elementpicturefactory.cpp
  ${QET_DIR}/sources/factory/elementpicturefactory.h
  ${QET_DIR}/sources/factory/propertieseditorfactory.cpp
  ${QET_DIR}/sources/factory/propertieseditorfactory.h
  ${QET_DIR}/sources/factory/qetgraphicstablefactory.cpp
  ${QET_DIR}/sources/factory/qetgraphicstablefactory.h

  ${QET_DIR}/sources/factory/ui/addtabledialog.cpp
  ${QET_DIR}/sources/factory/ui/addtabledialog.h

  ${QET_DIR}/sources/NameList/nameslist.cpp
  ${QET_DIR}/sources/NameList/nameslist.h

  ${QET_DIR}/sources/NameList/ui/namelistdialog.cpp
  ${QET_DIR}/sources/NameList/ui/namelistdialog.h
  ${QET_DIR}/sources/NameList/ui/namelistwidget.cpp
  ${QET_DIR}/sources/NameList/ui/namelistwidget.h

  ${QET_DIR}/sources/print/projectprintwindow.cpp
  ${QET_DIR}/sources/print/projectprintwindow.h

  ${QET_DIR}/sources/properties/propertiesinterface.cpp
  ${QET_DIR}/sources/properties/propertiesinterface.h
  ${QET_DIR}/sources/properties/reportproperties.cpp
  ${QET_DIR}/sources/properties/reportproperties.h
  ${QET_DIR}/sources/properties/terminaldata.cpp
  ${QET_DIR}/sources/properties/terminaldata.h
  ${QET_DIR}/sources/properties/xrefproperties.cpp
  ${QET_DIR}/sources/properties/xrefproperties.h

  ${QET_DIR}/sources/PropertiesEditor/propertieseditordialog.h
  ${QET_DIR}/sources/PropertiesEditor/propertieseditordockwidget.cpp
  ${QET_DIR}/sources/PropertiesEditor/propertieseditordockwidget.h
  ${QET_DIR}/sources/PropertiesEditor/propertieseditorwidget.cpp
  ${QET_DIR}/sources/PropertiesEditor/propertieseditorwidget.h

  ${QET_DIR}/sources/pugixml/pugiconfig.hpp
  ${QET_DIR}/sources/pugixml/pugixml.cpp
  ${QET_DIR}/sources/pugixml/pugixml.hpp

  ${QET_DIR}/sources/qetgraphicsitem/conductor.cpp
  ${QET_DIR}/sources/qetgraphicsitem/conductor.h
  ${QET_DIR}/sources/qetgraphicsitem/conductortextitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/conductortextitem.h
  ${QET_DIR}/sources/qetgraphicsitem/crossrefitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/crossrefitem.h
  ${QET_DIR}/sources/qetgraphicsitem/diagramimageitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/diagramimageitem.h
  ${QET_DIR}/sources/qetgraphicsitem/diagramtextitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/diagramtextitem.h
  ${QET_DIR}/sources/qetgraphicsitem/dynamicelementtextitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/dynamicelementtextitem.h
  ${QET_DIR}/sources/qetgraphicsitem/element.cpp
  ${QET_DIR}/sources/qetgraphicsitem/element.h
  ${QET_DIR}/sources/qetgraphicsitem/elementtextitemgroup.cpp
  ${QET_DIR}/sources/qetgraphicsitem/elementtextitemgroup.h
  ${QET_DIR}/sources/qetgraphicsitem/independenttextitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/independenttextitem.h
  ${QET_DIR}/sources/qetgraphicsitem/masterelement.cpp
  ${QET_DIR}/sources/qetgraphicsitem/masterelement.h
  ${QET_DIR}/sources/qetgraphicsitem/qetgraphicsitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/qetgraphicsitem.h
  ${QET_DIR}/sources/qetgraphicsitem/qetshapeitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/qetshapeitem.h
  ${QET_DIR}/sources/qetgraphicsitem/qgraphicsitemutility.cpp
  ${QET_DIR}/sources/qetgraphicsitem/qgraphicsitemutility.h
  ${QET_DIR}/sources/qetgraphicsitem/reportelement.cpp
  ${QET_DIR}/sources/qetgraphicsitem/reportelement.h
  ${QET_DIR}/sources/qetgraphicsitem/simpleelement.cpp
  ${QET_DIR}/sources/qetgraphicsitem/simpleelement.h
  ${QET_DIR}/sources/qetgraphicsitem/slaveelement.cpp
  ${QET_DIR}/sources/qetgraphicsitem/slaveelement.h
  ${QET_DIR}/sources/qetgraphicsitem/terminal.cpp
  ${QET_DIR}/sources/qetgraphicsitem/terminalelement.cpp
  ${QET_DIR}/sources/qetgraphicsitem/terminalelement.h
  ${QET_DIR}/sources/qetgraphicsitem/terminal.h


  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/projectdbmodel.cpp
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/projectdbmodel.h
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/qetgraphicsheaderitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/qetgraphicsheaderitem.h
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/qetgraphicstableitem.cpp
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/qetgraphicstableitem.h

  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/ui/graphicstablepropertieseditor.cpp
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/ui/graphicstablepropertieseditor.h
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/ui/projectdbmodelpropertieswidget.cpp
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/ui/projectdbmodelpropertieswidget.h

  ${QET_DIR}/sources/QetGraphicsItemModeler/qetgraphicshandleritem.cpp
  ${QET_DIR}/sources/QetGraphicsItemModeler/qetgraphicshandleritem.h
  ${QET_DIR}/sources/QetGraphicsItemModeler/qetgraphicshandlerutility.cpp
  ${QET_DIR}/sources/QetGraphicsItemModeler/qetgraphicshandlerutility.h

  ${QET_DIR}/sources/QPropertyUndoCommand/qpropertyundocommand.cpp
  ${QET_DIR}/sources/QPropertyUndoCommand/qpropertyundocommand.h

  ${QET_DIR}/sources/QWidgetAnimation/qwidgetanimation.cpp
  ${QET_DIR}/sources/QWidgetAnimation/qwidgetanimation.h

  ${QET_DIR}/sources/richtext/richtexteditor.cpp
  ${QET_DIR}/sources/richtext/richtexteditor_p.h
  ${QET_DIR}/sources/richtext/ui_addlinkdialog.h

  ${QET_DIR}/sources/SearchAndReplace/searchandreplaceworker.cpp
  ${QET_DIR}/sources/SearchAndReplace/searchandreplaceworker.h

  ${QET_DIR}/sources/SearchAndReplace/ui/replaceadvanceddialog.cpp
  ${QET_DIR}/sources/SearchAndReplace/ui/replaceadvanceddialog.h
  ${QET_DIR}/sources/SearchAndReplace/ui/replaceconductordialog.cpp
  ${QET_DIR}/sources/SearchAndReplace/ui/replaceconductordialog.h
  ${QET_DIR}/sources/SearchAndReplace/ui/replaceelementdialog.cpp
  ${QET_DIR}/sources/SearchAndReplace/ui/replaceelementdialog.h
  ${QET_DIR}/sources/SearchAndReplace/ui/replacefoliowidget.cpp
  ${QET_DIR}/sources/SearchAndReplace/ui/replacefoliowidget.h
  ${QET_DIR}/sources/SearchAndReplace/ui/searchandreplacewidget.cpp
  ${QET_DIR}/sources/SearchAndReplace/ui/searchandreplacewidget.h

  ${QET_DIR}/sources/titleblock/dimension.cpp
  ${QET_DIR}/sources/titleblock/dimension.h
  ${QET_DIR}/sources/titleblock/dimensionwidget.cpp
  ${QET_DIR}/sources/titleblock/dimensionwidget.h
  ${QET_DIR}/sources/titleblock/gridlayoutanimation.cpp
  ${QET_DIR}/sources/titleblock/gridlayoutanimation.h
  ${QET_DIR}/sources/titleblock/helpercell.cpp
  ${QET_DIR}/sources/titleblock/helpercell.h
  ${QET_DIR}/sources/titleblock/integrationmovetemplateshandler.cpp
  ${QET_DIR}/sources/titleblock/integrationmovetemplateshandler.h
  ${QET_DIR}/sources/titleblock/movetemplateshandler.h
  ${QET_DIR}/sources/titleblock/qettemplateeditor.cpp
  ${QET_DIR}/sources/titleblock/qettemplateeditor.h
  ${QET_DIR}/sources/titleblock/splittedhelpercell.cpp
  ${QET_DIR}/sources/titleblock/splittedhelpercell.h
  ${QET_DIR}/sources/titleblock/templatecellsset.cpp
  ${QET_DIR}/sources/titleblock/templatecellsset.h
  ${QET_DIR}/sources/titleblock/templatecellwidget.cpp
  ${QET_DIR}/sources/titleblock/templatecellwidget.h
  ${QET_DIR}/sources/titleblock/templatecommands.cpp
  ${QET_DIR}/sources/titleblock/templatecommands.h
  ${QET_DIR}/sources/titleblock/templatedeleter.cpp
  ${QET_DIR}/sources/titleblock/templatedeleter.h
  ${QET_DIR}/sources/titleblock/templatelocationchooser.cpp
  ${QET_DIR}/sources/titleblock/templatelocationchooser.h
  ${QET_DIR}/sources/titleblock/templatelocation.cpp
  ${QET_DIR}/sources/titleblock/templatelocation.h
  ${QET_DIR}/sources/titleblock/templatelocationsaver.cpp
  ${QET_DIR}/sources/titleblock/templatelocationsaver.h
  ${QET_DIR}/sources/titleblock/templatelogomanager.cpp
  ${QET_DIR}/sources/titleblock/templatelogomanager.h
  ${QET_DIR}/sources/titleblock/templatescollection.cpp
  ${QET_DIR}/sources/titleblock/templatescollection.h
  ${QET_DIR}/sources/titleblock/templateview.cpp
  ${QET_DIR}/sources/titleblock/templateview.h
  ${QET_DIR}/sources/titleblock/templatevisualcell.cpp
  ${QET_DIR}/sources/titleblock/templatevisualcell.h

  ${QET_DIR}/sources/ui/aboutqetdialog.cpp
  ${QET_DIR}/sources/ui/aboutqetdialog.h
  ${QET_DIR}/sources/ui/abstractelementpropertieseditorwidget.cpp
  ${QET_DIR}/sources/ui/abstractelementpropertieseditorwidget.h
  ${QET_DIR}/sources/ui/alignmenttextdialog.cpp
  ${QET_DIR}/sources/ui/alignmenttextdialog.h
  ${QET_DIR}/sources/ui/bomexportdialog.cpp
  ${QET_DIR}/sources/ui/bomexportdialog.h
  ${QET_DIR}/sources/ui/borderpropertieswidget.cpp
  ${QET_DIR}/sources/ui/borderpropertieswidget.h
  ${QET_DIR}/sources/ui/compositetexteditdialog.cpp
  ${QET_DIR}/sources/ui/compositetexteditdialog.h
  ${QET_DIR}/sources/ui/conductorpropertiesdialog.cpp
  ${QET_DIR}/sources/ui/conductorpropertiesdialog.h
  ${QET_DIR}/sources/ui/conductorpropertieswidget.cpp
  ${QET_DIR}/sources/ui/conductorpropertieswidget.h
  ${QET_DIR}/sources/ui/configsaveloaderwidget.cpp
  ${QET_DIR}/sources/ui/configsaveloaderwidget.h
  ${QET_DIR}/sources/ui/diagramcontextwidget.cpp
  ${QET_DIR}/sources/ui/diagramcontextwidget.h
  ${QET_DIR}/sources/ui/diagrampropertiesdialog.cpp
  ${QET_DIR}/sources/ui/diagrampropertiesdialog.h
  ${QET_DIR}/sources/ui/diagrampropertieseditordockwidget.cpp
  ${QET_DIR}/sources/ui/diagrampropertieseditordockwidget.h
  ${QET_DIR}/sources/ui/diagramselection.cpp
  ${QET_DIR}/sources/ui/diagramselection.h
  ${QET_DIR}/sources/ui/dialogwaiting.cpp
  ${QET_DIR}/sources/ui/dialogwaiting.h
  ${QET_DIR}/sources/ui/dynamicelementtextitemeditor.cpp
  ${QET_DIR}/sources/ui/dynamicelementtextitemeditor.h
  ${QET_DIR}/sources/ui/dynamicelementtextmodel.cpp
  ${QET_DIR}/sources/ui/dynamicelementtextmodel.h
  ${QET_DIR}/sources/ui/elementinfopartwidget.cpp
  ${QET_DIR}/sources/ui/elementinfopartwidget.h
  ${QET_DIR}/sources/ui/elementinfowidget.cpp
  ${QET_DIR}/sources/ui/elementinfowidget.h
  ${QET_DIR}/sources/ui/elementpropertieswidget.cpp
  ${QET_DIR}/sources/ui/elementpropertieswidget.h
  ${QET_DIR}/sources/ui/formulaassistantdialog.cpp
  ${QET_DIR}/sources/ui/formulaassistantdialog.h
  ${QET_DIR}/sources/ui/imagepropertieswidget.cpp
  ${QET_DIR}/sources/ui/imagepropertieswidget.h
  ${QET_DIR}/sources/ui/importelementdialog.cpp
  ${QET_DIR}/sources/ui/importelementdialog.h
  ${QET_DIR}/sources/ui/importelementtextpatterndialog.cpp
  ${QET_DIR}/sources/ui/importelementtextpatterndialog.h
  ${QET_DIR}/sources/ui/inditextpropertieswidget.cpp
  ${QET_DIR}/sources/ui/inditextpropertieswidget.h
  ${QET_DIR}/sources/ui/linksingleelementwidget.cpp
  ${QET_DIR}/sources/ui/linksingleelementwidget.h
  ${QET_DIR}/sources/ui/marginseditdialog.cpp
  ${QET_DIR}/sources/ui/marginseditdialog.h
  ${QET_DIR}/sources/ui/masterpropertieswidget.cpp
  ${QET_DIR}/sources/ui/masterpropertieswidget.h
  ${QET_DIR}/sources/ui/multipastedialog.cpp
  ${QET_DIR}/sources/ui/multipastedialog.h
  ${QET_DIR}/sources/ui/potentialselectordialog.cpp
  ${QET_DIR}/sources/ui/potentialselectordialog.h
  ${QET_DIR}/sources/ui/projectpropertiesdialog.cpp
  ${QET_DIR}/sources/ui/projectpropertiesdialog.h
  ${QET_DIR}/sources/ui/reportpropertiewidget.cpp
  ${QET_DIR}/sources/ui/reportpropertiewidget.h
  ${QET_DIR}/sources/ui/shapegraphicsitempropertieswidget.cpp
  ${QET_DIR}/sources/ui/shapegraphicsitempropertieswidget.h
  ${QET_DIR}/sources/ui/titleblockpropertieswidget.cpp
  ${QET_DIR}/sources/ui/titleblockpropertieswidget.h
  ${QET_DIR}/sources/ui/xrefpropertieswidget.cpp
  ${QET_DIR}/sources/ui/xrefpropertieswidget.h
  ${QET_DIR}/sources/ui/configpage/generalconfigurationpage.cpp

  ${QET_DIR}/sources/ui/configpage/generalconfigurationpage.h

  ${QET_DIR}/sources/undocommand/addelementtextcommand.cpp
  ${QET_DIR}/sources/undocommand/addelementtextcommand.h
  ${QET_DIR}/sources/undocommand/changeelementinformationcommand.cpp
  ${QET_DIR}/sources/undocommand/changeelementinformationcommand.h
  ${QET_DIR}/sources/undocommand/changetitleblockcommand.cpp
  ${QET_DIR}/sources/undocommand/changetitleblockcommand.h
  ${QET_DIR}/sources/undocommand/deleteqgraphicsitemcommand.cpp
  ${QET_DIR}/sources/undocommand/deleteqgraphicsitemcommand.h
  ${QET_DIR}/sources/undocommand/itemmodelcommand.cpp
  ${QET_DIR}/sources/undocommand/itemmodelcommand.h
  ${QET_DIR}/sources/undocommand/linkelementcommand.cpp
  ${QET_DIR}/sources/undocommand/linkelementcommand.h
  ${QET_DIR}/sources/undocommand/rotateselectioncommand.cpp
  ${QET_DIR}/sources/undocommand/rotateselectioncommand.h
  ${QET_DIR}/sources/undocommand/rotatetextscommand.cpp
  ${QET_DIR}/sources/undocommand/rotatetextscommand.h

  ${QET_DIR}/sources/utils/conductorcreator.cpp
  ${QET_DIR}/sources/utils/conductorcreator.h
  ${QET_DIR}/sources/utils/macosxopenevent.cpp
  ${QET_DIR}/sources/utils/macosxopenevent.h
  ${QET_DIR}/sources/utils/qetutils.cpp
  ${QET_DIR}/sources/utils/qetutils.h
  )

set(TS_FILES
    ${QET_DIR}/lang/qet_ca.ts
    ${QET_DIR}/lang/qet_de.ts
    ${QET_DIR}/lang/qet_es.ts
    ${QET_DIR}/lang/qet_fr.ts
    ${QET_DIR}/lang/qet_it.ts
    ${QET_DIR}/lang/qet_mn.ts
    ${QET_DIR}/lang/qet_pt_br.ts
    ${QET_DIR}/lang/qet_ru.ts
    ${QET_DIR}/lang/qet_sr.ts
    ${QET_DIR}/lang/qet_zh.ts
    ${QET_DIR}/lang/qet_be.ts
    ${QET_DIR}/lang/qet_da.ts
    ${QET_DIR}/lang/qet_en.ts
    ${QET_DIR}/lang/qet_fi.ts
    ${QET_DIR}/lang/qet_hu.ts
    ${QET_DIR}/lang/qet_nl.ts
    ${QET_DIR}/lang/qet_pl.ts
    ${QET_DIR}/lang/qet_ro.ts
    ${QET_DIR}/lang/qet_sk.ts
    ${QET_DIR}/lang/qet_ar.ts
    ${QET_DIR}/lang/qet_cs.ts
    ${QET_DIR}/lang/qet_el.ts
    ${QET_DIR}/lang/qet_hr.ts
    ${QET_DIR}/lang/qet_ja.ts
    ${QET_DIR}/lang/qet_nb.ts
    ${QET_DIR}/lang/qet_no.ts
    ${QET_DIR}/lang/qet_pt.ts
    ${QET_DIR}/lang/qet_sl.ts
    ${QET_DIR}/lang/qet_tr.ts)

  set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "lang")
  qt5_add_translation(QM_FILES ${TS_FILES})

if(DEFINED BUILD_QET_YES)
  message("!!  TESTING TESTING TESTING TESTING TESTING  !!")
add_executable(
  ${PROJECT_NAME}
  ${QET_SRC_FILES}
  ${QM_FILES}
  ${QET_DIR}/qelectrotech.qrc
  )

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Xml
  Qt${QT_VERSION_MAJOR}::Svg
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Sql
  Qt${QT_VERSION_MAJOR}::PrintSupport
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Concurrent
  KF5::WidgetsAddons
  KF5::CoreAddons)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    PRIVATE
    pugixml::pugixml
    SingleApplication::SingleApplication
    KF5::WidgetsAddons
    KF5::CoreAddons
    Qt5::Gui
    Qt5::Xml
    Qt5::Svg
    Qt5::Network
    Qt5::Sql
    Qt5::Widgets
    Qt5::Concurrent)

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
    ${QET_DIR}/sources/pugixml
    ${QET_DIR}/sources/dataBase
    ${QET_DIR}/sources/dataBase/ui
    ${QET_DIR}/sources/factory/ui
    ${QET_DIR}/sources/print
    )
  #    ${QET_DIR}/SingleApplication

else()
  message("!!  sorry now Cmake can't get QET builds yet  !!")
endif()

# Add sub directories
option(PACKAGE_TESTS "Build the tests" ON)

if(PACKAGE_TESTS)
    enable_testing()
    message("Add sub directorie tests")
    add_subdirectory(tests)
endif()

