QT += widgets xml sql network concurrent svg printsupport testlib
CONFIG += console testcase c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_smart_device

# Link the real application objects, built with the same Qt and no_kf5 config.
isEmpty(QET_BUILD_DIR): QET_BUILD_DIR = $$clean_path($$PWD/../../build-debug)
INCLUDEPATH += $$PWD/../.. $$PWD/../../sources $$PWD/../../SingleApplication
INCLUDEPATH += $$PWD/../../sources/ui $$PWD/../../sources/TerminalStrip/ui
INCLUDEPATH += $$PWD/../../sources/ui/nokde $$QET_BUILD_DIR
DEFINES += BUILD_WITHOUT_KF BUILD_WITHOUT_KF5 QAPPLICATION_CLASS=QApplication
SOURCES += $$PWD/tst_smart_device.cpp
win32 {
    QET_OBJECTS = $$files($$QET_BUILD_DIR/debug/*.o)
    QET_OBJECTS -= $$QET_BUILD_DIR/debug/main.o
    LIBS += -lsqlite3 -ladvapi32
} else {
    QET_OBJECTS = $$files($$QET_BUILD_DIR/*.o)
    QET_OBJECTS -= $$QET_BUILD_DIR/main.o
    LIBS += -lsqlite3
}
isEmpty(QET_OBJECTS): error(Build the application before running these tests)
OBJECTS += $$QET_OBJECTS
