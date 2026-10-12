/****************************************************************************
** Meta object code from reading C++ file 'templatescollection.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/titleblock/templatescollection.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'templatescollection.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN29TitleBlockTemplatesCollectionE_t {};
} // unnamed namespace

template <> constexpr inline auto TitleBlockTemplatesCollection::qt_create_metaobjectdata<qt_meta_tag_ZN29TitleBlockTemplatesCollectionE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TitleBlockTemplatesCollection",
        "changed",
        "",
        "TitleBlockTemplatesCollection*",
        "aboutToRemove"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'changed'
        QtMocHelpers::SignalData<void(TitleBlockTemplatesCollection *, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::QString, 2 },
        }}),
        // Signal 'changed'
        QtMocHelpers::SignalData<void(TitleBlockTemplatesCollection *)>(1, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'aboutToRemove'
        QtMocHelpers::SignalData<void(TitleBlockTemplatesCollection *, const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::QString, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TitleBlockTemplatesCollection, qt_meta_tag_ZN29TitleBlockTemplatesCollectionE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TitleBlockTemplatesCollection::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN29TitleBlockTemplatesCollectionE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN29TitleBlockTemplatesCollectionE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN29TitleBlockTemplatesCollectionE_t>.metaTypes,
    nullptr
} };

void TitleBlockTemplatesCollection::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TitleBlockTemplatesCollection *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changed((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->changed((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1]))); break;
        case 2: _t->aboutToRemove((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplatesCollection* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplatesCollection* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplatesCollection* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TitleBlockTemplatesCollection::*)(TitleBlockTemplatesCollection * , const QString & )>(_a, &TitleBlockTemplatesCollection::changed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TitleBlockTemplatesCollection::*)(TitleBlockTemplatesCollection * , const QString & )>(_a, &TitleBlockTemplatesCollection::aboutToRemove, 2))
            return;
    }
}

const QMetaObject *TitleBlockTemplatesCollection::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TitleBlockTemplatesCollection::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN29TitleBlockTemplatesCollectionE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TitleBlockTemplatesCollection::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void TitleBlockTemplatesCollection::changed(TitleBlockTemplatesCollection * _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 2
void TitleBlockTemplatesCollection::aboutToRemove(TitleBlockTemplatesCollection * _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}
namespace {
struct qt_meta_tag_ZN36TitleBlockTemplatesProjectCollectionE_t {};
} // unnamed namespace

template <> constexpr inline auto TitleBlockTemplatesProjectCollection::qt_create_metaobjectdata<qt_meta_tag_ZN36TitleBlockTemplatesProjectCollectionE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TitleBlockTemplatesProjectCollection"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TitleBlockTemplatesProjectCollection, qt_meta_tag_ZN36TitleBlockTemplatesProjectCollectionE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TitleBlockTemplatesProjectCollection::staticMetaObject = { {
    QMetaObject::SuperData::link<TitleBlockTemplatesCollection::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN36TitleBlockTemplatesProjectCollectionE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN36TitleBlockTemplatesProjectCollectionE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN36TitleBlockTemplatesProjectCollectionE_t>.metaTypes,
    nullptr
} };

void TitleBlockTemplatesProjectCollection::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TitleBlockTemplatesProjectCollection *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *TitleBlockTemplatesProjectCollection::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TitleBlockTemplatesProjectCollection::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN36TitleBlockTemplatesProjectCollectionE_t>.strings))
        return static_cast<void*>(this);
    return TitleBlockTemplatesCollection::qt_metacast(_clname);
}

int TitleBlockTemplatesProjectCollection::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TitleBlockTemplatesCollection::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_tag_ZN34TitleBlockTemplatesFilesCollectionE_t {};
} // unnamed namespace

template <> constexpr inline auto TitleBlockTemplatesFilesCollection::qt_create_metaobjectdata<qt_meta_tag_ZN34TitleBlockTemplatesFilesCollectionE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TitleBlockTemplatesFilesCollection",
        "fileSystemChanged",
        "",
        "str"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'fileSystemChanged'
        QtMocHelpers::SlotData<void(const QString &)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TitleBlockTemplatesFilesCollection, qt_meta_tag_ZN34TitleBlockTemplatesFilesCollectionE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TitleBlockTemplatesFilesCollection::staticMetaObject = { {
    QMetaObject::SuperData::link<TitleBlockTemplatesCollection::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN34TitleBlockTemplatesFilesCollectionE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN34TitleBlockTemplatesFilesCollectionE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN34TitleBlockTemplatesFilesCollectionE_t>.metaTypes,
    nullptr
} };

void TitleBlockTemplatesFilesCollection::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TitleBlockTemplatesFilesCollection *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->fileSystemChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *TitleBlockTemplatesFilesCollection::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TitleBlockTemplatesFilesCollection::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN34TitleBlockTemplatesFilesCollectionE_t>.strings))
        return static_cast<void*>(this);
    return TitleBlockTemplatesCollection::qt_metacast(_clname);
}

int TitleBlockTemplatesFilesCollection::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TitleBlockTemplatesCollection::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
QT_WARNING_POP
