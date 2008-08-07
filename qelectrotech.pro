######################################################################
#                   Fichier projet de QElectroTech                   #
######################################################################

# Chemins utilises pour la compilation et l'installation de QET
!win32 {
	# Chemins UNIX
	COMPIL_PREFIX              = '/usr/local/'
	INSTALL_PREFIX             = '/usr/local/'
	QET_BINARY_PATH            = 'bin/'
	QET_COMMON_COLLECTION_PATH = 'share/qelectrotech/elements/'
	QET_LANG_PATH              = 'share/qelectrotech/lang/'
	QET_LICENSE_PATH           = 'doc/qelectrotech/'
	QET_MIME_XML_PATH          = '../share/mime/applications/'
	QET_MIME_DESKTOP_PATH      = '../share/mimelnk/applications/'
	QET_DESKTOP_PATH           = 'share/applications/'
	QET_ICONS_PATH             = 'share/icons/'
} else {
	# Chemins Windows
	COMPIL_PREFIX              = './'
	INSTALL_PREFIX             = './'
	QET_BINARY_PATH            = './'
	QET_COMMON_COLLECTION_PATH = 'elements/'
	QET_LANG_PATH              = 'lang/'
	QET_LICENSE_PATH           = './'
}

# Commenter la ligne ci-dessous pour desactiver l'option --common-elements-dir
DEFINES += QET_ALLOW_OVERRIDE_CED_OPTION

# Commenter la ligne ci-dessous pour desactiver l'option --config-dir
DEFINES += QET_ALLOW_OVERRIDE_CD_OPTION

######################################################################

TEMPLATE = app
DEPENDPATH += . sources sources/editor lang
INCLUDEPATH += sources sources/editor

# Fichiers sources
HEADERS += sources/aboutqet.h \
           sources/borderinset.h \
           sources/conductor.h \
           sources/conductorprofile.h \
           sources/conductorproperties.h \
           sources/conductorpropertieswidget.h \
           sources/conductorsegment.h \
           sources/conductorsegmentprofile.h \
           sources/configdialog.h \
           sources/configpages.h \
           sources/customelement.h \
           sources/diagram.h \
           sources/diagramcommands.h \
           sources/diagramcontent.h \
           sources/diagramprintdialog.h \
           sources/diagramview.h \
           sources/diagramtextitem.h \
           sources/element.h \
           sources/elementdeleter.h \
           sources/elementscategorieslist.h \
           sources/elementscategorieswidget.h \
           sources/elementscategory.h \
           sources/elementscategorydeleter.h \
           sources/elementscategoryeditor.h \
           sources/elementspanel.h \
           sources/elementspanelwidget.h \
           sources/elementtextitem.h \
           sources/exportdialog.h \
           sources/fixedelement.h \
           sources/hotspoteditor.h \
           sources/insetproperties.h \
           sources/insetpropertieswidget.h \
           sources/nameslist.h \
           sources/nameslistwidget.h \
           sources/newelementwizard.h \
           sources/orientationset.h \
           sources/orientationsetwidget.h \
           sources/qet.h \
           sources/qetapp.h \
           sources/qetarguments.h \
           sources/qetdiagrameditor.h \
           sources/qetsingleapplication.h \
           sources/qgimanager.h \
           sources/recentfiles.h \
           sources/terminal.h \
           sources/editor/arceditor.h \
           sources/editor/circleeditor.h \
           sources/editor/customelementgraphicpart.h \
           sources/editor/customelementpart.h \
           sources/editor/editorcommands.h \
           sources/editor/elementitemeditor.h \
           sources/editor/elementscene.h \
           sources/editor/elementview.h \
           sources/editor/ellipseeditor.h \
           sources/editor/lineeditor.h \
           sources/editor/partarc.h \
           sources/editor/partcircle.h \
           sources/editor/partellipse.h \
           sources/editor/partline.h \
           sources/editor/partpolygon.h \
           sources/editor/partterminal.h \
           sources/editor/parttext.h \
           sources/editor/parttextfield.h \
           sources/editor/polygoneditor.h \
           sources/editor/qetelementeditor.h \
           sources/editor/styleeditor.h \
           sources/editor/terminaleditor.h \
           sources/editor/texteditor.h \
           sources/editor/textfieldeditor.h
SOURCES += sources/aboutqet.cpp \
           sources/borderinset.cpp \
           sources/conductor.cpp \
           sources/conductorprofile.cpp \
           sources/conductorproperties.cpp \
           sources/conductorpropertieswidget.cpp \
           sources/conductorsegment.cpp \
           sources/customelement.cpp \
           sources/configdialog.cpp \
           sources/configpages.cpp \
           sources/diagram.cpp \
           sources/diagramcommands.cpp \
           sources/diagramcontent.cpp \
           sources/diagramprintdialog.cpp \
           sources/diagramtextitem.cpp \
           sources/diagramview.cpp \
           sources/element.cpp \
           sources/elementdeleter.cpp \
           sources/elementscategorieslist.cpp \
           sources/elementscategorieswidget.cpp \
           sources/elementscategory.cpp \
           sources/elementscategorydeleter.cpp \
           sources/elementscategoryeditor.cpp \
           sources/elementspanel.cpp \
           sources/elementspanelwidget.cpp \
           sources/elementtextitem.cpp \
           sources/exportdialog.cpp \
           sources/fixedelement.cpp \
           sources/hotspoteditor.cpp \
           sources/insetproperties.cpp \
           sources/insetpropertieswidget.cpp \
           sources/main.cpp \
           sources/nameslist.cpp \
           sources/nameslistwidget.cpp \
           sources/newelementwizard.cpp \
           sources/orientationset.cpp \
           sources/orientationsetwidget.cpp \
           sources/qet.cpp \
           sources/qetapp.cpp \
           sources/qetarguments.cpp \
           sources/qetdiagrameditor.cpp \
           sources/qetsingleapplication.cpp \
           sources/qgimanager.cpp \
           sources/recentfiles.cpp \
           sources/terminal.cpp \
           sources/editor/arceditor.cpp \
           sources/editor/circleeditor.cpp \
           sources/editor/customelementgraphicpart.cpp \
           sources/editor/customelementpart.cpp \
           sources/editor/editorcommands.cpp \
           sources/editor/elementitemeditor.cpp \
           sources/editor/elementscene.cpp \
           sources/editor/elementview.cpp \
           sources/editor/ellipseeditor.cpp \
           sources/editor/lineeditor.cpp \
           sources/editor/partarc.cpp \
           sources/editor/partcircle.cpp \
           sources/editor/partellipse.cpp \
           sources/editor/partline.cpp \
           sources/editor/partpolygon.cpp \
           sources/editor/partterminal.cpp \
           sources/editor/parttext.cpp \
           sources/editor/parttextfield.cpp \
           sources/editor/polygoneditor.cpp \
           sources/editor/qetelementeditor.cpp \
           sources/editor/styleeditor.cpp \
           sources/editor/terminaleditor.cpp \
           sources/editor/texteditor.cpp \
           sources/editor/textfieldeditor.cpp
RESOURCES += qelectrotech.qrc
TRANSLATIONS += lang/qet_en.ts lang/qt_fr.ts
RC_FILE = ico/windows_icon/application_icon/qelectrotech.rc
QT += xml svg network
CONFIG += debug_and_release warn_on
TARGET = qelectrotech

# Description de l'installation
target.path     = $$join(INSTALL_PREFIX,,,$${QET_BINARY_PATH})

elements.path   = $$join(INSTALL_PREFIX,,,$${QET_COMMON_COLLECTION_PATH})
elements.files  = elements/*

lang.path       = $$join(INSTALL_PREFIX,,,$${QET_LANG_PATH})
lang.files      = $$replace(TRANSLATIONS, '.ts', '.qm')

copyright.path  = $$join(INSTALL_PREFIX,,,$${QET_LICENSE_PATH})
copyright.files = LICENSE \
                  CREDIT \
                  README

mime_xml.path      = $$join(INSTALL_PREFIX,,,$${QET_MIME_XML_PATH})
mime_xml.files     = misc/x-qet-*.xml

mime_desktop.path  = $$join(INSTALL_PREFIX,,,$${QET_MIME_DESKTOP_PATH})
mime_desktop.files = misc/x-qet-*.desktop

desktop.path   = $$join(INSTALL_PREFIX,,,$${QET_DESKTOP_PATH})
desktop.files  = misc/qelectrotech.desktop

icons.path     = $$join(INSTALL_PREFIX,,,$${QET_ICONS_PATH})
icons.files    = ico/qet.png

# L'installation comprend la copie du binaire, des elements, des fichiers de langue et du fichier LICENSE
INSTALLS += target elements lang copyright
# Sous Unix, on installe egalement l'icone et un fichier .desktop
unix {
	INSTALLS += desktop mime_xml mime_desktop icons
}

# Options de compilation
# Chemin des fichiers de traduction ; par defaut : lang/ dans le repertoire d'execution
DEFINES += QET_LANG_PATH=$$join(COMPIL_PREFIX,,,$${QET_LANG_PATH})

# Chemin de la collection commune ; par defaut : elements/ dans le repertoire d'execution
DEFINES += QET_COMMON_COLLECTION_PATH=$$join(COMPIL_PREFIX,,,$${QET_COMMON_COLLECTION_PATH})
