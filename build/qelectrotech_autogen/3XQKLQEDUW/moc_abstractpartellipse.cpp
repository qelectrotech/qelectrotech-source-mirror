/****************************************************************************
** Meta object code from reading C++ file 'abstractpartellipse.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/graphicspart/abstractpartellipse.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'abstractpartellipse.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19AbstractPartEllipseE_t {};
} // unnamed namespace

template <> constexpr inline auto AbstractPartEllipse::qt_create_metaobjectdata<qt_meta_tag_ZN19AbstractPartEllipseE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AbstractPartEllipse",
        "rectChanged",
        "",
        "startAngleChanged",
        "spanAngleChanged",
        "startAngle",
        "spanAngle",
        "rect",
        "QRectF"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'rectChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'startAngleChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'spanAngleChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'startAngle'
        QtMocHelpers::PropertyData<int>(5, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet),
        // property 'spanAngle'
        QtMocHelpers::PropertyData<int>(6, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet),
        // property 'rect'
        QtMocHelpers::PropertyData<QRectF>(7, 0x80000000 | 8, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AbstractPartEllipse, qt_meta_tag_ZN19AbstractPartEllipseE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AbstractPartEllipse::staticMetaObject = { {
    QMetaObject::SuperData::link<CustomElementGraphicPart::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19AbstractPartEllipseE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19AbstractPartEllipseE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN19AbstractPartEllipseE_t>.metaTypes,
    nullptr
} };

void AbstractPartEllipse::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AbstractPartEllipse *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->rectChanged(); break;
        case 1: _t->startAngleChanged(); break;
        case 2: _t->spanAngleChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AbstractPartEllipse::*)()>(_a, &AbstractPartEllipse::rectChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AbstractPartEllipse::*)()>(_a, &AbstractPartEllipse::startAngleChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AbstractPartEllipse::*)()>(_a, &AbstractPartEllipse::spanAngleChanged, 2))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->startAngle(); break;
        case 1: *reinterpret_cast<int*>(_v) = _t->spanAngle(); break;
        case 2: *reinterpret_cast<QRectF*>(_v) = _t->rect(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setStartAngle(*reinterpret_cast<int*>(_v)); break;
        case 1: _t->setSpanAngle(*reinterpret_cast<int*>(_v)); break;
        case 2: _t->setRect(*reinterpret_cast<QRectF*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *AbstractPartEllipse::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AbstractPartEllipse::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19AbstractPartEllipseE_t>.strings))
        return static_cast<void*>(this);
    return CustomElementGraphicPart::qt_metacast(_clname);
}

int AbstractPartEllipse::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CustomElementGraphicPart::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void AbstractPartEllipse::rectChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AbstractPartEllipse::startAngleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AbstractPartEllipse::spanAngleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
