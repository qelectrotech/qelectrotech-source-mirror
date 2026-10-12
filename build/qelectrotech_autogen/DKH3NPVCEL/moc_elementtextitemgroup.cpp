/****************************************************************************
** Meta object code from reading C++ file 'elementtextitemgroup.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/elementtextitemgroup.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementtextitemgroup.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20ElementTextItemGroupE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementTextItemGroup::qt_create_metaobjectdata<qt_meta_tag_ZN20ElementTextItemGroupE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementTextItemGroup",
        "rotationChanged",
        "",
        "verticalAdjustmentChanged",
        "alignmentChanged",
        "Qt::Alignment",
        "nameChanged",
        "holdToBottomPageChanged",
        "xChanged",
        "yChanged",
        "frameChanged",
        "frame",
        "pos",
        "QPointF",
        "rotation",
        "verticalAdjustment",
        "alignment",
        "name",
        "holdToBottomPage"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'rotationChanged'
        QtMocHelpers::SignalData<void(qreal)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QReal, 2 },
        }}),
        // Signal 'verticalAdjustmentChanged'
        QtMocHelpers::SignalData<void(int)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Signal 'alignmentChanged'
        QtMocHelpers::SignalData<void(Qt::Alignment)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Signal 'nameChanged'
        QtMocHelpers::SignalData<void(QString)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Signal 'holdToBottomPageChanged'
        QtMocHelpers::SignalData<void(bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Signal 'xChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'yChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'frameChanged'
        QtMocHelpers::SignalData<void(bool)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 11 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'pos'
        QtMocHelpers::PropertyData<QPointF>(12, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
        // property 'rotation'
        QtMocHelpers::PropertyData<qreal>(14, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'verticalAdjustment'
        QtMocHelpers::PropertyData<int>(15, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'alignment'
        QtMocHelpers::PropertyData<Qt::Alignment>(16, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 2),
        // property 'name'
        QtMocHelpers::PropertyData<QString>(17, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'holdToBottomPage'
        QtMocHelpers::PropertyData<bool>(18, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'frame'
        QtMocHelpers::PropertyData<bool>(11, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 7),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ElementTextItemGroup, qt_meta_tag_ZN20ElementTextItemGroupE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementTextItemGroup::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20ElementTextItemGroupE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20ElementTextItemGroupE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN20ElementTextItemGroupE_t>.metaTypes,
    nullptr
} };

void ElementTextItemGroup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ElementTextItemGroup *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->rotationChanged((*reinterpret_cast<std::add_pointer_t<qreal>>(_a[1]))); break;
        case 1: _t->verticalAdjustmentChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->alignmentChanged((*reinterpret_cast<std::add_pointer_t<Qt::Alignment>>(_a[1]))); break;
        case 3: _t->nameChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->holdToBottomPageChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 5: _t->xChanged(); break;
        case 6: _t->yChanged(); break;
        case 7: _t->frameChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)(qreal )>(_a, &ElementTextItemGroup::rotationChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)(int )>(_a, &ElementTextItemGroup::verticalAdjustmentChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)(Qt::Alignment )>(_a, &ElementTextItemGroup::alignmentChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)(QString )>(_a, &ElementTextItemGroup::nameChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)(bool )>(_a, &ElementTextItemGroup::holdToBottomPageChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)()>(_a, &ElementTextItemGroup::xChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)()>(_a, &ElementTextItemGroup::yChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementTextItemGroup::*)(bool )>(_a, &ElementTextItemGroup::frameChanged, 7))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QPointF*>(_v) = _t->pos(); break;
        case 1: *reinterpret_cast<qreal*>(_v) = _t->rotation(); break;
        case 2: *reinterpret_cast<int*>(_v) = _t->verticalAdjustment(); break;
        case 3: *reinterpret_cast<Qt::Alignment*>(_v) = _t->alignment(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->name(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->holdToBottomPage(); break;
        case 6: *reinterpret_cast<bool*>(_v) = _t->frame(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setPos(*reinterpret_cast<QPointF*>(_v)); break;
        case 1: _t->setRotation(*reinterpret_cast<qreal*>(_v)); break;
        case 2: _t->setVerticalAdjustment(*reinterpret_cast<int*>(_v)); break;
        case 3: _t->setAlignment(*reinterpret_cast<Qt::Alignment*>(_v)); break;
        case 4: _t->setName(*reinterpret_cast<QString*>(_v)); break;
        case 5: _t->setHoldToBottomPage(*reinterpret_cast<bool*>(_v)); break;
        case 6: _t->setFrame(*reinterpret_cast<bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *ElementTextItemGroup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElementTextItemGroup::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20ElementTextItemGroupE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QGraphicsItemGroup"))
        return static_cast< QGraphicsItemGroup*>(this);
    return QObject::qt_metacast(_clname);
}

int ElementTextItemGroup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ElementTextItemGroup::rotationChanged(qreal _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ElementTextItemGroup::verticalAdjustmentChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ElementTextItemGroup::alignmentChanged(Qt::Alignment _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ElementTextItemGroup::nameChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ElementTextItemGroup::holdToBottomPageChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void ElementTextItemGroup::xChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ElementTextItemGroup::yChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ElementTextItemGroup::frameChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
