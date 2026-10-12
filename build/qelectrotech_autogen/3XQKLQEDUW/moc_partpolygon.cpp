/****************************************************************************
** Meta object code from reading C++ file 'partpolygon.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/graphicspart/partpolygon.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'partpolygon.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11PartPolygonE_t {};
} // unnamed namespace

template <> constexpr inline auto PartPolygon::qt_create_metaobjectdata<qt_meta_tag_ZN11PartPolygonE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PartPolygon",
        "closedChange",
        "",
        "polygonChanged",
        "closed",
        "polygon",
        "QPolygonF"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'closedChange'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'polygonChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'closed'
        QtMocHelpers::PropertyData<bool>(4, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet),
        // property 'polygon'
        QtMocHelpers::PropertyData<QPolygonF>(5, 0x80000000 | 6, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PartPolygon, qt_meta_tag_ZN11PartPolygonE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject PartPolygon::staticMetaObject = { {
    QMetaObject::SuperData::link<CustomElementGraphicPart::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11PartPolygonE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11PartPolygonE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11PartPolygonE_t>.metaTypes,
    nullptr
} };

void PartPolygon::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PartPolygon *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->closedChange(); break;
        case 1: _t->polygonChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PartPolygon::*)()>(_a, &PartPolygon::closedChange, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartPolygon::*)()>(_a, &PartPolygon::polygonChanged, 1))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->isClosed(); break;
        case 1: *reinterpret_cast<QPolygonF*>(_v) = _t->polygon(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setClosed(*reinterpret_cast<bool*>(_v)); break;
        case 1: _t->setPolygon(*reinterpret_cast<QPolygonF*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *PartPolygon::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PartPolygon::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11PartPolygonE_t>.strings))
        return static_cast<void*>(this);
    return CustomElementGraphicPart::qt_metacast(_clname);
}

int PartPolygon::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CustomElementGraphicPart::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void PartPolygon::closedChange()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PartPolygon::polygonChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
