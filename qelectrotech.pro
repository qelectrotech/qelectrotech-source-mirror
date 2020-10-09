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
        # Liste des ressources Windows
    #RC_FILE = qelectrotech.rc
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

#comment the line below to disable the project database export
DEFINES += QET_EXPORT_PROJECT_DB

# warn on *any* usage of deprecated APIs
#DEFINES += QT_DEPRECATED_WARNINGS


######################################################################

include(sources/PropertiesEditor/PropertiesEditor.pri)
include(sources/QetGraphicsItemModeler/QetGraphicsItemModeler.pri)
include(sources/QPropertyUndoCommand/QPropertyUndoCommand.pri)
include(SingleApplication/singleapplication.pri)
include(sources/QWidgetAnimation/QWidgetAnimation.pri)

DEFINES += QAPPLICATION_CLASS=QApplication
DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += GIT_COMMIT_SHA="\\\"$(shell git -C \""$$_PRO_FILE_PWD_"\" rev-parse --verify HEAD)\\\""

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000 # disables all the APIs deprecated before Qt 6.0.0

# You can make your code warn on compile time for the TODO's
# In order to do so, uncomment the following line.
#DEFINES += TODO_LIST

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += sources \
               sources/titleblock \
               sources/ui \
               sources/qetgraphicsitem \
               sources/qetgraphicsitem/ViewItem \
               sources/qetgraphicsitem/ViewItem/ui \
               sources/richtext \
               sources/factory \
               sources/properties \
               sources/dvevent \
               sources/editor \
               sources/editor/esevent \
               sources/editor/graphicspart \
               sources/editor/ui \
               sources/editor/UndoCommand \
               sources/undocommand \
               sources/diagramevent \
               sources/ElementsCollection \
               sources/ElementsCollection/ui \
               sources/autoNum \
               sources/autoNum/ui \
               sources/ui/configpage \
               sources/SearchAndReplace \
               sources/SearchAndReplace/ui \
               sources/NameList \
               sources/NameList/ui \
               sources/utils \
               sources/pugixml \
               sources/dataBase \
               sources/dataBase/ui \
               sources/factory/ui \
               sources/print


# Fichiers sources
HEADERS += $$files(sources/*.h) $$files(sources/ui/*.h) \
           $$files(sources/editor/*.h) \
           $$files(sources/titleblock/*.h) \
           $$files(sources/richtext/*.h) \
           $$files(sources/qetgraphicsitem/*.h) \
           $$files(sources/qetgraphicsitem/ViewItem/*.h) \
           $$files(sources/qetgraphicsitem/ViewItem/ui/*.h) \
           $$files(sources/factory/*.h) \
           $$files(sources/properties/*.h) \
           $$files(sources/editor/ui/*.h) \
           $$files(sources/editor/esevent/*.h) \
           $$files(sources/editor/graphicspart/*.h) \
           $$files(sources/editor/UndoCommand/*h) \
           $$files(sources/dvevent/*.h) \
           $$files(sources/undocommand/*.h) \
           $$files(sources/diagramevent/*.h) \
           $$files(sources/ElementsCollection/*.h) \
           $$files(sources/ElementsCollection/ui/*.h) \
           $$files(sources/autoNum/*.h) \
           $$files(sources/autoNum/ui/*.h) \
           $$files(sources/ui/configpage/*.h) \
           $$files(sources/SearchAndReplace/*.h) \
           $$files(sources/SearchAndReplace/ui/*.h) \
           $$files(sources/NameList/*.h) \
           $$files(sources/NameList/ui/*.h) \
           $$files(sources/utils/*.h) \
           $$files(sources/pugixml/*.hpp) \
           $$files(sources/dataBase/*.h) \
           $$files(sources/dataBase/ui/*.h) \
           $$files(sources/factory/ui/*.h) \
           $$files(sources/print/*.h)

SOURCES += $$files(sources/*.cpp) \
           $$files(sources/editor/*.cpp) \
           $$files(sources/titleblock/*.cpp) \
           $$files(sources/richtext/*.cpp) \
           $$files(sources/ui/*.cpp) \
           $$files(sources/qetgraphicsitem/*.cpp) \
           $$files(sources/qetgraphicsitem/ViewItem/*.cpp) \
           $$files(sources/qetgraphicsitem/ViewItem/ui/*.cpp) \
           $$files(sources/factory/*.cpp) \
           $$files(sources/properties/*.cpp) \
           $$files(sources/editor/ui/*.cpp) \
           $$files(sources/editor/esevent/*.cpp) \
           $$files(sources/editor/graphicspart/*.cpp) \
           $$files(sources/editor/UndoCommand/*cpp) \
           $$files(sources/dvevent/*.cpp) \
           $$files(sources/undocommand/*.cpp) \
           $$files(sources/diagramevent/*.cpp) \
           $$files(sources/ElementsCollection/*.cpp) \
           $$files(sources/ElementsCollection/ui/*.cpp) \
           $$files(sources/autoNum/*.cpp) \
           $$files(sources/autoNum/ui/*.cpp) \
           $$files(sources/ui/configpage/*.cpp) \
           $$files(sources/SearchAndReplace/*.cpp) \
           $$files(sources/SearchAndReplace/ui/*.cpp) \
           $$files(sources/NameList/*.cpp) \
           $$files(sources/NameList/ui/*.cpp) \
           $$files(sources/utils/*.cpp) \
           $$files(sources/pugixml/*.cpp) \
           $$files(sources/dataBase/*.cpp) \
           $$files(sources/dataBase/ui/*.cpp) \
           $$files(sources/factory/ui/*.cpp) \
           $$files(sources/print/*.cpp)

# Liste des fichiers qui seront incorpores au binaire en tant que ressources Qt
RESOURCES += qelectrotech.qrc

# Liste des ressources Windows
#RC_FILE = ico/windows_icon/qelectrotech.rc

# Fichiers de traduction qui seront installes
TRANSLATIONS += lang/*.ts

# Modules Qt utilises par l'application
QT += xml svg network sql widgets printsupport concurrent KWidgetsAddons KCoreAddons

# UI DESIGNER FILES AND GENERATION SOURCES FILES
FORMS += $$files(sources/richtext/*.ui) \
         $$files(sources/ui/*.ui) \
         $$files(sources/editor/ui/*.ui) \
         $$files(sources/ElementsCollection/ui/*.ui) \
         $$files(sources/autoNum/ui/*.ui) \
         $$files(sources/ui/configpage/*.ui) \
         $$files(sources/SearchAndReplace/ui/*.ui) \
         $$files(sources/NameList/ui/*.ui) \
         $$files(sources/qetgraphicsitem/ViewItem/ui/*.ui) \
         $$files(sources/dataBase/ui/*.ui) \
         $$files(sources/factory/ui/*.ui) \
         $$files(sources/print/*.ui)

UI_SOURCES_DIR = sources/ui/
UI_HEADERS_DIR = sources/ui/

# Configuration de la compilation
CONFIG += c++17 debug_and_release warn_on link_pkgconfig

# Nom du binaire genere par la compilation
TARGET = qelectrotech

# Ajustement des bibliotheques utilisees lors de l'edition des liens
unix:QMAKE_LIBS_THREAD -= -lpthread
unix|win32: PKGCONFIG += sqlite3

# Enable C++17
QMAKE_CXXFLAGS += -std=c++17

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
icons.files        = ico/breeze-icons/16x16   \
                     ico/breeze-icons/22x22   \
                     ico/breeze-icons/32x32   \
                     ico/breeze-icons/48x48   \
                     ico/breeze-icons/64x64   \
                     ico/breeze-icons/128x128 \
                     ico/breeze-icons/256x256

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
