######################################################################
#                   Fichier projet de QElectroTech                   #
######################################################################

# Chemins utilises pour la compilation et l'installation de QET
unix {
	# Chemins UNIX
	COMPIL_PREFIX              = '/usr/local/'
	INSTALL_PREFIX             = '/usr/local/'
	QET_BINARY_PATH            = 'bin/'
	QET_COMMON_COLLECTION_PATH = 'share/qelectrotech/elements/'
	QET_LANG_PATH              = 'share/qelectrotech/lang/'
	QET_EXAMPLES_PATH          = 'share/qelectrotech/examples/'
	QET_LICENSE_PATH           = 'doc/qelectrotech/'
	QET_MIME_XML_PATH          = '../share/mime/application/'
	QET_MIME_DESKTOP_PATH      = '../share/mimelnk/application/'
	QET_MIME_PACKAGE_PATH      = '../share/mime/packages/'
	QET_DESKTOP_PATH           = 'share/applications/'
	QET_ICONS_PATH             = 'share/icons/hicolor/'
	QET_MAN_PATH               = 'man/'
}
win32 {
	# Chemins Windows
	COMPIL_PREFIX              = './'
	INSTALL_PREFIX             = './'
	QET_BINARY_PATH            = './'
	QET_COMMON_COLLECTION_PATH = 'elements/'
	QET_LANG_PATH              = 'lang/'
	QET_LICENSE_PATH           = './'
}
macx {
	# Chemins MacOS X
	COMPIL_PREFIX              = '/usr/local/'
	INSTALL_PREFIX             = '/usr/local/'
	QET_BINARY_PATH            = 'bin/'
	QET_COMMON_COLLECTION_PATH = 'share/qelectrotech/elements/'
	QET_LANG_PATH              = 'share/qelectrotech/lang/'
	QET_EXAMPLES_PATH          = 'share/qelectrotech/examples/'
	QET_LICENSE_PATH           = 'doc/qelectrotech/'
	QET_MIME_XML_PATH          = '../share/mime/application/'
	QET_MIME_DESKTOP_PATH      = '../share/mimelnk/application/'
	QET_DESKTOP_PATH           = 'share/applications/'
	QET_ICONS_PATH             = 'share/icons/hicolor/'
	QET_MAN_PATH               = 'man/'
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
           sources/basicmoveelementshandler.h \
           sources/borderinset.h \
           sources/borderproperties.h \
           sources/borderpropertieswidget.h \
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
           sources/diagramschooser.h \
           sources/diagramtextitem.h \
           sources/diagramview.h \
           sources/element.h \
           sources/elementdefinition.h \
           sources/elementdeleter.h \
           sources/elementdialog.h \
           sources/elementscategorieslist.h \
           sources/elementscategorieswidget.h \
           sources/elementscategory.h \
           sources/elementscategorydeleter.h \
           sources/elementscategoryeditor.h \
           sources/elementscollection.h \
           sources/elementscollectionitem.h \
           sources/elementslocation.h \
           sources/elementspanel.h \
           sources/elementspanelwidget.h \
           sources/elementtextitem.h \
           sources/exportdialog.h \
           sources/fileelementscategory.h \
           sources/fileelementscollection.h \
           sources/fileelementdefinition.h \
           sources/fixedelement.h \
           sources/ghostelement.h \
           sources/hotspoteditor.h \
           sources/insetproperties.h \
           sources/insetpropertieswidget.h \
           sources/integrationmoveelementshandler.h \
           sources/interactivemoveelementshandler.h \
           sources/moveelementsdescription.h \
           sources/moveelementshandler.h \
           sources/nameslist.h \
           sources/nameslistwidget.h \
           sources/newelementwizard.h \
           sources/orientationset.h \
           sources/orientationsetwidget.h \
           sources/projectview.h \
           sources/qet.h \
           sources/qetapp.h \
           sources/qetarguments.h \
           sources/qetdiagrameditor.h \
           sources/qeticons.h \
           sources/qetproject.h \
           sources/qetprintpreviewdialog.h \
           sources/qetregexpvalidator.h \
           sources/qettabbar.h \
           sources/qettabwidget.h \
           sources/qetsingleapplication.h \
           sources/qfilenameedit.h \
           sources/qgimanager.h \
           sources/recentfiles.h \
           sources/terminal.h \
           sources/xmlelementdefinition.h \
           sources/xmlelementscategory.h \
           sources/xmlelementscollection.h \
           sources/editor/arceditor.h \
           sources/editor/circleeditor.h \
           sources/editor/customelementgraphicpart.h \
           sources/editor/customelementpart.h \
           sources/editor/editorcommands.h \
           sources/editor/elementcontent.h \
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
           sources/editor/partrectangle.h \
           sources/editor/partterminal.h \
           sources/editor/parttext.h \
           sources/editor/parttextfield.h \
           sources/editor/polygoneditor.h \
           sources/editor/qetelementeditor.h \
           sources/editor/rectangleeditor.h \
           sources/editor/styleeditor.h \
           sources/editor/terminaleditor.h \
           sources/editor/texteditor.h \
           sources/editor/textfieldeditor.h
SOURCES += sources/aboutqet.cpp \
           sources/basicmoveelementshandler.cpp \
           sources/borderinset.cpp \
           sources/borderproperties.cpp \
           sources/borderpropertieswidget.cpp \
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
           sources/diagramschooser.cpp \
           sources/diagramtextitem.cpp \
           sources/diagramview.cpp \
           sources/element.cpp \
           sources/elementdefinition.cpp \
           sources/elementdeleter.cpp \
           sources/elementdialog.cpp \
           sources/elementscategorieslist.cpp \
           sources/elementscategorieswidget.cpp \
           sources/elementscategory.cpp \
           sources/elementscategorydeleter.cpp \
           sources/elementscategoryeditor.cpp \
           sources/elementscollection.cpp \
           sources/elementslocation.cpp \
           sources/elementspanel.cpp \
           sources/elementspanelwidget.cpp \
           sources/elementtextitem.cpp \
           sources/exportdialog.cpp \
           sources/fixedelement.cpp \
           sources/fileelementscategory.cpp \
           sources/fileelementscollection.cpp \
           sources/fileelementdefinition.cpp \
           sources/ghostelement.cpp \
           sources/hotspoteditor.cpp \
           sources/insetproperties.cpp \
           sources/insetpropertieswidget.cpp \
           sources/integrationmoveelementshandler.cpp \
           sources/interactivemoveelementshandler.cpp \
           sources/main.cpp \
           sources/moveelementsdescription.cpp \
           sources/nameslist.cpp \
           sources/nameslistwidget.cpp \
           sources/newelementwizard.cpp \
           sources/orientationset.cpp \
           sources/orientationsetwidget.cpp \
           sources/projectview.cpp \
           sources/qet.cpp \
           sources/qetapp.cpp \
           sources/qetarguments.cpp \
           sources/qetdiagrameditor.cpp \
           sources/qeticons.cpp \
           sources/qetproject.cpp \
           sources/qetprintpreviewdialog.cpp \
           sources/qetregexpvalidator.cpp \
           sources/qettabbar.cpp \
           sources/qettabwidget.cpp \
           sources/qetsingleapplication.cpp \
           sources/qfilenameedit.cpp \
           sources/qgimanager.cpp \
           sources/recentfiles.cpp \
           sources/terminal.cpp \
           sources/xmlelementdefinition.cpp \
           sources/xmlelementscategory.cpp \
           sources/xmlelementscollection.cpp \
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
           sources/editor/partrectangle.cpp \
           sources/editor/partterminal.cpp \
           sources/editor/parttext.cpp \
           sources/editor/parttextfield.cpp \
           sources/editor/polygoneditor.cpp \
           sources/editor/qetelementeditor.cpp \
           sources/editor/rectangleeditor.cpp \
           sources/editor/styleeditor.cpp \
           sources/editor/terminaleditor.cpp \
           sources/editor/texteditor.cpp \
           sources/editor/textfieldeditor.cpp
RESOURCES += qelectrotech.qrc
TRANSLATIONS += lang/qet_en.ts lang/qet_es.ts lang/qet_fr.ts lang/qet_ru.ts
TRANSLATIONS +=                lang/qt_es.ts  lang/qt_fr.ts  lang/qt_ru.ts
RC_FILE = ico/windows_icon/qelectrotech.rc
QT += xml svg network
CONFIG += debug_and_release warn_on
TARGET = qelectrotech

# Description de l'installation
target.path        = $$join(INSTALL_PREFIX,,,$${QET_BINARY_PATH})

elements.path      = $$join(INSTALL_PREFIX,,,$${QET_COMMON_COLLECTION_PATH})
elements.files     = elements/*

lang.path          = $$join(INSTALL_PREFIX,,,$${QET_LANG_PATH})
lang.files         = $$replace(TRANSLATIONS, '.ts', '.qm')

examples.path      = $$join(INSTALL_PREFIX,,,$${QET_EXAMPLES_PATH})
examples.files     = examples/*

copyright.path     = $$join(INSTALL_PREFIX,,,$${QET_LICENSE_PATH})
copyright.files    = LICENSE CREDIT README ChangeLog

mime_xml.path      = $$join(INSTALL_PREFIX,,,$${QET_MIME_XML_PATH})
mime_xml.files     = misc/x-qet-*.xml

mime_desktop.path  = $$join(INSTALL_PREFIX,,,$${QET_MIME_DESKTOP_PATH})
mime_desktop.files = misc/x-qet-*.desktop

mime_package.path  = $$join(INSTALL_PREFIX,,,$${QET_MIME_PACKAGE_PATH})
mime_package.files = misc/qelectrotech.xml

desktop.path       = $$join(INSTALL_PREFIX,,,$${QET_DESKTOP_PATH})
desktop.files      = misc/qelectrotech.desktop

icons.path         = $$join(INSTALL_PREFIX,,,$${QET_ICONS_PATH})
icons.files        = ico/oxygen-icons/16x16   \
                     ico/oxygen-icons/22x22   \
                     ico/oxygen-icons/32x32   \
                     ico/oxygen-icons/48x48   \
                     ico/oxygen-icons/64x64   \
                     ico/oxygen-icons/128x128 \
                     ico/oxygen-icons/256x256

man.path           = $$join(INSTALL_PREFIX,,,$${QET_MAN_PATH})
man.files          = man/files/*
man.extra          = sh man/compress_man_pages.sh

# L'installation comprend la copie du binaire, des elements, des fichiers de langue et du fichier LICENSE
INSTALLS += target elements lang copyright
# Sous Unix, on installe egalement l'icone, un fichier .desktop, des fichiers mime et les pages de manuel
unix {
	INSTALLS += desktop mime_xml mime_desktop mime_package icons man examples
}

# Options de compilation
# Chemin des fichiers de traduction ; par defaut : lang/ dans le repertoire d'execution
DEFINES += QET_LANG_PATH=$$join(COMPIL_PREFIX,,,$${QET_LANG_PATH})

# Chemin de la collection commune ; par defaut : elements/ dans le repertoire d'execution
DEFINES += QET_COMMON_COLLECTION_PATH=$$join(COMPIL_PREFIX,,,$${QET_COMMON_COLLECTION_PATH})
