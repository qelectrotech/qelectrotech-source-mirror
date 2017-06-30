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
	QET_COMMON_TBT_PATH        = 'share/qelectrotech/titleblocks/'
	QET_LANG_PATH              = 'share/qelectrotech/lang/'
	QET_EXAMPLES_PATH          = 'share/qelectrotech/examples/'
	QET_LICENSE_PATH           = 'doc/qelectrotech/'
	QET_MIME_XML_PATH          = '../share/mime/application/'
	QET_MIME_DESKTOP_PATH      = '../share/mimelnk/application/'
	QET_MIME_PACKAGE_PATH      = '../share/mime/packages/'
	QET_DESKTOP_PATH           = 'share/applications/'
	QET_ICONS_PATH             = 'share/icons/hicolor/'
	QET_MAN_PATH               = 'man/'
	QET_APPDATA_PATH           = 'share/appdata'
}
win32 {
	# Chemins Windows
	COMPIL_PREFIX              = './'
	INSTALL_PREFIX             = './'
	QET_BINARY_PATH            = './'
	QET_COMMON_COLLECTION_PATH = 'elements/'
	QET_COMMON_TBT_PATH        = 'titleblocks/'
	QET_LANG_PATH              = 'lang/'
	QET_LICENSE_PATH           = './'
}
macx {
	# Chemins MacOS X
	COMPIL_PREFIX              = './'
	INSTALL_PREFIX             = '/usr/local/'
	QET_BINARY_PATH            = 'bin/'
	QET_COMMON_COLLECTION_PATH = '../Resources/elements/'
	QET_COMMON_TBT_PATH        = '../Resources/titleblocks/'
	QET_LANG_PATH              = '../Resources/lang/'
	QET_EXAMPLES_PATH          = 'share/qelectrotech/examples/'
	QET_LICENSE_PATH           = 'doc/qelectrotech/'
	QET_MIME_XML_PATH          = '../share/mime/application/'
	QET_MIME_DESKTOP_PATH      = '../share/mimelnk/application/'
	QET_DESKTOP_PATH           = 'share/applications/'
	QET_ICONS_PATH             = 'share/icons/hicolor/'
	QET_MAN_PATH               = 'man/'
	ICON                       = 'ico/mac_icon/qelectrotech.icns'

}

# Commenter la ligne ci-dessous pour desactiver l'option --common-elements-dir
DEFINES += QET_ALLOW_OVERRIDE_CED_OPTION

# Comment the line below to disable the --common-tbt-dir option
DEFINES += QET_ALLOW_OVERRIDE_CTBTD_OPTION

# Commenter la ligne ci-dessous pour desactiver l'option --config-dir
DEFINES += QET_ALLOW_OVERRIDE_CD_OPTION

# warn on *any* usage of deprecated APIs
#DEFINES += QT_DEPRECATED_WARNINGS


######################################################################

include(sources/PropertiesEditor/PropertiesEditor.pri)
include(sources/QetGraphicsItemModeler/QetGraphicsItemModeler.pri)
include(sources/QPropertyUndoCommand/QPropertyUndoCommand.pri)

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += sources \
               sources/titleblock \
               sources/ui sources/qetgraphicsitem \
               sources/richtext \
               sources/factory \
               sources/properties \
               sources/dvevent \
               sources/editor \
               sources/editor/esevent \
               sources/editor/graphicspart \
               sources/undocommand \
               sources/diagramevent \
               sources/ElementsCollection \
               sources/ElementsCollection/ui \
               sources/autoNum \
               sources/autoNum/ui


# Fichiers sources
HEADERS += $$files(sources/*.h) $$files(sources/ui/*.h) $$files(sources/editor/*.h) $$files(sources/titleblock/*.h) $$files(sources/richtext/*.h) $$files(sources/qetgraphicsitem/*.h) $$files(sources/factory/*.h) \
           $$files(sources/properties/*.h) \
           $$files(sources/editor/ui/*.h) \
           $$files(sources/editor/esevent/*.h) \
           $$files(sources/editor/graphicspart/*.h) \
           $$files(sources/dvevent/*.h) \
           $$files(sources/undocommand/*.h) \
           $$files(sources/diagramevent/*.h) \
           $$files(sources/ElementsCollection/*.h) \
           $$files(sources/ElementsCollection/ui/*.h) \
           $$files(sources/autoNum/*.h) \
           $$files(sources/autoNum/ui/*.h)

SOURCES += $$files(sources/*.cpp) $$files(sources/editor/*.cpp) $$files(sources/titleblock/*.cpp) $$files(sources/richtext/*.cpp) $$files(sources/ui/*.cpp) $$files(sources/qetgraphicsitem/*.cpp) $$files(sources/factory/*.cpp) \
           $$files(sources/properties/*.cpp) \
           $$files(sources/editor/ui/*.cpp) \
           $$files(sources/editor/esevent/*.cpp) \
           $$files(sources/editor/graphicspart/*.cpp) \
           $$files(sources/dvevent/*.cpp) \
           $$files(sources/undocommand/*.cpp) \
           $$files(sources/diagramevent/*.cpp) \
           $$files(sources/ElementsCollection/*.cpp) \
           $$files(sources/ElementsCollection/ui/*.cpp) \
           $$files(sources/autoNum/*.cpp) \
           $$files(sources/autoNum/ui/*.cpp)

# Liste des fichiers qui seront incorpores au binaire en tant que ressources Qt
RESOURCES += qelectrotech.qrc

# Liste des ressources Windows
#RC_FILE = ico/windows_icon/qelectrotech.rc

# Fichiers de traduction qui seront installes
TRANSLATIONS += lang/qet_en.ts lang/qet_es.ts lang/qet_fr.ts lang/qet_ru.ts lang/qet_pt.ts lang/qet_cs.ts lang/qet_pl.ts lang/qet_de.ts lang/qet_ro.ts lang/qet_it.ts lang/qet_el.ts lang/qet_nl.ts lang/qet_be.ts

# Modules Qt utilises par l'application
QT += xml svg network sql widgets printsupport concurrent

# UI DESIGNER FILES AND GENERATION SOURCES FILES
FORMS += $$files(sources/richtext/*.ui) \
         $$files(sources/ui/*.ui) \
         $$files(sources/editor/ui/*.ui) \
         $$files(sources/ElementsCollection/ui/*.ui) \
         $$files(sources/autoNum/ui/*.ui)

UI_SOURCES_DIR = sources/ui/
UI_HEADERS_DIR = sources/ui/

# Configuration de la compilation
CONFIG += c++11 debug_and_release warn_on

# Nom du binaire genere par la compilation
TARGET = qelectrotech

# Ajustement des bibliotheques utilisees lors de l'edition des liens
unix:QMAKE_LIBS_THREAD -= -lpthread

# Enable C++11
QMAKE_CXXFLAGS += -std=c++11

# Description de l'installation
target.path        = $$join(INSTALL_PREFIX,,,$${QET_BINARY_PATH})

elements.path      = $$join(INSTALL_PREFIX,,,$${QET_COMMON_COLLECTION_PATH})
elements.files     = elements/*

tbt.path           = $$join(INSTALL_PREFIX,,,$${QET_COMMON_TBT_PATH})
tbt.files          = titleblocks/*

lang.path          = $$join(INSTALL_PREFIX,,,$${QET_LANG_PATH})
lang.files         = $$replace(TRANSLATIONS, '.ts', '.qm')

examples.path      = $$join(INSTALL_PREFIX,,,$${QET_EXAMPLES_PATH})
examples.files     = examples/*

copyright.path     = $$join(INSTALL_PREFIX,,,$${QET_LICENSE_PATH})
copyright.files    = LICENSE ELEMENTS.LICENSE CREDIT README ChangeLog

mime_xml.path      = $$join(INSTALL_PREFIX,,,$${QET_MIME_XML_PATH})
mime_xml.files     = misc/x-qet-*.xml

mime_desktop.path  = $$join(INSTALL_PREFIX,,,$${QET_MIME_DESKTOP_PATH})
mime_desktop.files = misc/x-qet-*.desktop

mime_package.path  = $$join(INSTALL_PREFIX,,,$${QET_MIME_PACKAGE_PATH})
mime_package.files = misc/qelectrotech.xml

desktop.path       = $$join(INSTALL_PREFIX,,,$${QET_DESKTOP_PATH})
desktop.files      = misc/qelectrotech.desktop

appdata.path       = $$join(INSTALL_PREFIX,,,$${QET_APPDATA_PATH})
appdata.files      = misc/qelectrotech.appdata.xml

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
INSTALLS += target elements tbt lang copyright
# Sous Unix, on installe egalement l'icone, un fichier .desktop, des fichiers mime et les pages de manuel
unix {
	INSTALLS += desktop mime_xml mime_desktop mime_package icons man examples appdata
}

# Options de compilation communes a Unix et MacOS X
unix {
	# Chemin des fichiers de traduction ; par defaut : lang/ dans le repertoire d'execution
	DEFINES += QET_LANG_PATH=$$join(COMPIL_PREFIX,,,$${QET_LANG_PATH})
	
	# Chemin de la collection commune ; par defaut : elements/ dans le repertoire d'execution
	DEFINES += QET_COMMON_COLLECTION_PATH=$$join(COMPIL_PREFIX,,,$${QET_COMMON_COLLECTION_PATH})
	
	DEFINES += QET_COMMON_TBT_PATH=$$join(COMPIL_PREFIX,,,$${QET_COMMON_TBT_PATH})
}

# Options de compilation specifiques a MacOS X
macx {
	# les chemins definis precedemment sont relatifs au dossier contenant le binaire executable
	DEFINES += QET_LANG_PATH_RELATIVE_TO_BINARY_PATH
	DEFINES += QET_COMMON_COLLECTION_PATH_RELATIVE_TO_BINARY_PATH
}

# Compilers-specific options
unix {
	QMAKE_COPY_DIR = 'cp -f -r --preserve=timestamps'
}
