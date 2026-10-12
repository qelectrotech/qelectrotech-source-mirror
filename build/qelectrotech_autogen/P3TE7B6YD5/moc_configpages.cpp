/****************************************************************************
** Meta object code from reading C++ file 'configpages.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/configpage/configpages.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'configpages.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14NewDiagramPageE_t {};
} // unnamed namespace

template <> constexpr inline auto NewDiagramPage::qt_create_metaobjectdata<qt_meta_tag_ZN14NewDiagramPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NewDiagramPage",
        "changeToAutoFolioTab",
        "",
        "setFolioAutonum",
        "saveCurrentTbp",
        "loadSavedTbp"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'changeToAutoFolioTab'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setFolioAutonum'
        QtMocHelpers::SlotData<void(QString)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'saveCurrentTbp'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'loadSavedTbp'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NewDiagramPage, qt_meta_tag_ZN14NewDiagramPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NewDiagramPage::staticMetaObject = { {
    QMetaObject::SuperData::link<ConfigPage::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14NewDiagramPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14NewDiagramPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14NewDiagramPageE_t>.metaTypes,
    nullptr
} };

void NewDiagramPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NewDiagramPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changeToAutoFolioTab(); break;
        case 1: _t->setFolioAutonum((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->saveCurrentTbp(); break;
        case 3: _t->loadSavedTbp(); break;
        default: ;
        }
    }
}

const QMetaObject *NewDiagramPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NewDiagramPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14NewDiagramPageE_t>.strings))
        return static_cast<void*>(this);
    return ConfigPage::qt_metacast(_clname);
}

int NewDiagramPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ConfigPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}
namespace {
struct qt_meta_tag_ZN16ExportConfigPageE_t {};
} // unnamed namespace

template <> constexpr inline auto ExportConfigPage::qt_create_metaobjectdata<qt_meta_tag_ZN16ExportConfigPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ExportConfigPage"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ExportConfigPage, qt_meta_tag_ZN16ExportConfigPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ExportConfigPage::staticMetaObject = { {
    QMetaObject::SuperData::link<ConfigPage::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ExportConfigPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ExportConfigPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16ExportConfigPageE_t>.metaTypes,
    nullptr
} };

void ExportConfigPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ExportConfigPage *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *ExportConfigPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ExportConfigPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ExportConfigPageE_t>.strings))
        return static_cast<void*>(this);
    return ConfigPage::qt_metacast(_clname);
}

int ExportConfigPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ConfigPage::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_tag_ZN15PrintConfigPageE_t {};
} // unnamed namespace

template <> constexpr inline auto PrintConfigPage::qt_create_metaobjectdata<qt_meta_tag_ZN15PrintConfigPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PrintConfigPage"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PrintConfigPage, qt_meta_tag_ZN15PrintConfigPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject PrintConfigPage::staticMetaObject = { {
    QMetaObject::SuperData::link<ConfigPage::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15PrintConfigPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15PrintConfigPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15PrintConfigPageE_t>.metaTypes,
    nullptr
} };

void PrintConfigPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PrintConfigPage *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *PrintConfigPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PrintConfigPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15PrintConfigPageE_t>.strings))
        return static_cast<void*>(this);
    return ConfigPage::qt_metacast(_clname);
}

int PrintConfigPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ConfigPage::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
