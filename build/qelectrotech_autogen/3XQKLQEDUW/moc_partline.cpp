/****************************************************************************
** Meta object code from reading C++ file 'partline.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/graphicspart/partline.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'partline.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8PartLineE_t {};
} // unnamed namespace

template <> constexpr inline auto PartLine::qt_create_metaobjectdata<qt_meta_tag_ZN8PartLineE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PartLine",
        "lineChanged",
        "",
        "firstEndTypeChanged",
        "secondEndTypeChanged",
        "firstEndLengthChanged",
        "secondEndLengthChanged",
        "end1",
        "Qet::EndType",
        "end2",
        "length1",
        "length2",
        "line",
        "QLineF"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'lineChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'firstEndTypeChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'secondEndTypeChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'firstEndLengthChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'secondEndLengthChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'end1'
        QtMocHelpers::PropertyData<Qet::EndType>(7, 0x80000000 | 8, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'end2'
        QtMocHelpers::PropertyData<Qet::EndType>(9, 0x80000000 | 8, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'length1'
        QtMocHelpers::PropertyData<qreal>(10, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'length2'
        QtMocHelpers::PropertyData<qreal>(11, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'line'
        QtMocHelpers::PropertyData<QLineF>(12, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PartLine, qt_meta_tag_ZN8PartLineE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT static const QMetaObject::SuperData qt_meta_extradata_ZN8PartLineE[] = {
    QMetaObject::SuperData::link<Qet::staticMetaObject>(),
    nullptr
};

Q_CONSTINIT const QMetaObject PartLine::staticMetaObject = { {
    QMetaObject::SuperData::link<CustomElementGraphicPart::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PartLineE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PartLineE_t>.data,
    qt_static_metacall,
    qt_meta_extradata_ZN8PartLineE,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8PartLineE_t>.metaTypes,
    nullptr
} };

void PartLine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PartLine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->lineChanged(); break;
        case 1: _t->firstEndTypeChanged(); break;
        case 2: _t->secondEndTypeChanged(); break;
        case 3: _t->firstEndLengthChanged(); break;
        case 4: _t->secondEndLengthChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PartLine::*)()>(_a, &PartLine::lineChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartLine::*)()>(_a, &PartLine::firstEndTypeChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartLine::*)()>(_a, &PartLine::secondEndTypeChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartLine::*)()>(_a, &PartLine::firstEndLengthChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartLine::*)()>(_a, &PartLine::secondEndLengthChanged, 4))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<Qet::EndType*>(_v) = _t->firstEndType(); break;
        case 1: *reinterpret_cast<Qet::EndType*>(_v) = _t->secondEndType(); break;
        case 2: *reinterpret_cast<qreal*>(_v) = _t->firstEndLength(); break;
        case 3: *reinterpret_cast<qreal*>(_v) = _t->secondEndLength(); break;
        case 4: *reinterpret_cast<QLineF*>(_v) = _t->line(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setFirstEndType(*reinterpret_cast<Qet::EndType*>(_v)); break;
        case 1: _t->setSecondEndType(*reinterpret_cast<Qet::EndType*>(_v)); break;
        case 2: _t->setFirstEndLength(*reinterpret_cast<qreal*>(_v)); break;
        case 3: _t->setSecondEndLength(*reinterpret_cast<qreal*>(_v)); break;
        case 4: _t->setLine(*reinterpret_cast<QLineF*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *PartLine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PartLine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PartLineE_t>.strings))
        return static_cast<void*>(this);
    return CustomElementGraphicPart::qt_metacast(_clname);
}

int PartLine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CustomElementGraphicPart::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void PartLine::lineChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PartLine::firstEndTypeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PartLine::secondEndTypeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PartLine::firstEndLengthChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PartLine::secondEndLengthChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
