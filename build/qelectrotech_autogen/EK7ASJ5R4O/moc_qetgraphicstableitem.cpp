/****************************************************************************
** Meta object code from reading C++ file 'qetgraphicstableitem.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qetgraphicstableitem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20QetGraphicsTableItemE_t {};
} // unnamed namespace

template <> constexpr inline auto QetGraphicsTableItem::qt_create_metaobjectdata<qt_meta_tag_ZN20QetGraphicsTableItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QetGraphicsTableItem",
        "size",
        "QSize",
        "displayNRow"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
        // property 'size'
        QtMocHelpers::PropertyData<QSize>(1, 0x80000000 | 2, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
        // property 'displayNRow'
        QtMocHelpers::PropertyData<int>(3, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QetGraphicsTableItem, qt_meta_tag_ZN20QetGraphicsTableItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QetGraphicsTableItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QetGraphicsItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20QetGraphicsTableItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20QetGraphicsTableItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN20QetGraphicsTableItemE_t>.metaTypes,
    nullptr
} };

void QetGraphicsTableItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QetGraphicsTableItem *>(_o);
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QSize*>(_v) = _t->size(); break;
        case 1: *reinterpret_cast<int*>(_v) = _t->displayNRow(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setSize(*reinterpret_cast<QSize*>(_v)); break;
        case 1: _t->setDisplayNRow(*reinterpret_cast<int*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *QetGraphicsTableItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QetGraphicsTableItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20QetGraphicsTableItemE_t>.strings))
        return static_cast<void*>(this);
    return QetGraphicsItem::qt_metacast(_clname);
}

int QetGraphicsTableItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QetGraphicsItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_WARNING_POP
