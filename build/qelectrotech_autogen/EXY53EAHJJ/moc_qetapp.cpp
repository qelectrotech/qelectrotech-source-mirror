/****************************************************************************
** Meta object code from reading C++ file 'qetapp.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetapp.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qetapp.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6QETAppE_t {};
} // unnamed namespace

template <> constexpr inline auto QETApp::qt_create_metaobjectdata<qt_meta_tag_ZN6QETAppE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QETApp",
        "systray",
        "",
        "QSystemTrayIcon::ActivationReason",
        "reduceEveryEditor",
        "restoreEveryEditor",
        "reduceDiagramEditors",
        "restoreDiagramEditors",
        "reduceElementEditors",
        "restoreElementEditors",
        "reduceTitleBlockTemplateEditors",
        "restoreTitleBlockTemplateEditors",
        "newDiagramEditor",
        "newElementEditor",
        "closeEveryEditor",
        "setMainWindowVisible",
        "QMainWindow*",
        "invertMainWindowVisibility",
        "QWidget*",
        "useSystemPalette",
        "quitQET",
        "checkRemainingWindows",
        "openFiles",
        "QETArguments",
        "openProjectFiles",
        "openElementFiles",
        "openElementLocations",
        "QList<ElementsLocation>",
        "openTitleBlockTemplate",
        "TitleBlockTemplateLocation",
        "openTitleBlockTemplateFiles",
        "configureQET",
        "aboutQET",
        "showDiagnosticsReport",
        "receiveMessage",
        "instanceId",
        "message"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'systray'
        QtMocHelpers::SlotData<void(QSystemTrayIcon::ActivationReason)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'reduceEveryEditor'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'restoreEveryEditor'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'reduceDiagramEditors'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'restoreDiagramEditors'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'reduceElementEditors'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'restoreElementEditors'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'reduceTitleBlockTemplateEditors'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'restoreTitleBlockTemplateEditors'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'newDiagramEditor'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'newElementEditor'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'closeEveryEditor'
        QtMocHelpers::SlotData<bool()>(14, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'setMainWindowVisible'
        QtMocHelpers::SlotData<void(QMainWindow *, bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 2 }, { QMetaType::Bool, 2 },
        }}),
        // Slot 'invertMainWindowVisibility'
        QtMocHelpers::SlotData<void(QWidget *)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 2 },
        }}),
        // Slot 'useSystemPalette'
        QtMocHelpers::SlotData<void(bool)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'quitQET'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'checkRemainingWindows'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'openFiles'
        QtMocHelpers::SlotData<void(const QETArguments &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 23, 2 },
        }}),
        // Slot 'openProjectFiles'
        QtMocHelpers::SlotData<void(const QStringList &)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 2 },
        }}),
        // Slot 'openElementFiles'
        QtMocHelpers::SlotData<void(const QStringList &)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 2 },
        }}),
        // Slot 'openElementLocations'
        QtMocHelpers::SlotData<void(const QList<ElementsLocation> &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 27, 2 },
        }}),
        // Slot 'openTitleBlockTemplate'
        QtMocHelpers::SlotData<void(const TitleBlockTemplateLocation &, bool)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 29, 2 }, { QMetaType::Bool, 2 },
        }}),
        // Slot 'openTitleBlockTemplate'
        QtMocHelpers::SlotData<void(const TitleBlockTemplateLocation &)>(28, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 29, 2 },
        }}),
        // Slot 'openTitleBlockTemplate'
        QtMocHelpers::SlotData<void(const QString &)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'openTitleBlockTemplateFiles'
        QtMocHelpers::SlotData<void(const QStringList &)>(30, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 2 },
        }}),
        // Slot 'configureQET'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'aboutQET'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'showDiagnosticsReport'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'receiveMessage'
        QtMocHelpers::SlotData<void(int, QByteArray)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 35 }, { QMetaType::QByteArray, 36 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QETApp, qt_meta_tag_ZN6QETAppE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QETApp::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6QETAppE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6QETAppE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6QETAppE_t>.metaTypes,
    nullptr
} };

void QETApp::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QETApp *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->systray((*reinterpret_cast<std::add_pointer_t<QSystemTrayIcon::ActivationReason>>(_a[1]))); break;
        case 1: _t->reduceEveryEditor(); break;
        case 2: _t->restoreEveryEditor(); break;
        case 3: _t->reduceDiagramEditors(); break;
        case 4: _t->restoreDiagramEditors(); break;
        case 5: _t->reduceElementEditors(); break;
        case 6: _t->restoreElementEditors(); break;
        case 7: _t->reduceTitleBlockTemplateEditors(); break;
        case 8: _t->restoreTitleBlockTemplateEditors(); break;
        case 9: _t->newDiagramEditor(); break;
        case 10: _t->newElementEditor(); break;
        case 11: { bool _r = _t->closeEveryEditor();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 12: _t->setMainWindowVisible((*reinterpret_cast<std::add_pointer_t<QMainWindow*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 13: _t->invertMainWindowVisibility((*reinterpret_cast<std::add_pointer_t<QWidget*>>(_a[1]))); break;
        case 14: _t->useSystemPalette((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 15: _t->quitQET(); break;
        case 16: _t->checkRemainingWindows(); break;
        case 17: _t->openFiles((*reinterpret_cast<std::add_pointer_t<QETArguments>>(_a[1]))); break;
        case 18: _t->openProjectFiles((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 19: _t->openElementFiles((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 20: _t->openElementLocations((*reinterpret_cast<std::add_pointer_t<QList<ElementsLocation>>>(_a[1]))); break;
        case 21: _t->openTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplateLocation>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 22: _t->openTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplateLocation>>(_a[1]))); break;
        case 23: _t->openTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->openTitleBlockTemplateFiles((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 25: _t->configureQET(); break;
        case 26: _t->aboutQET(); break;
        case 27: _t->showDiagnosticsReport(); break;
        case 28: _t->receiveMessage((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 20:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<ElementsLocation> >(); break;
            }
            break;
        case 21:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplateLocation >(); break;
            }
            break;
        case 22:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplateLocation >(); break;
            }
            break;
        }
    }
}

const QMetaObject *QETApp::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QETApp::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6QETAppE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int QETApp::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    }
    return _id;
}
QT_WARNING_POP
