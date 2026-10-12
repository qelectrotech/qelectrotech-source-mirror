/****************************************************************************
** Meta object code from reading C++ file 'element.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/element.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'element.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7ElementE_t {};
} // unnamed namespace

template <> constexpr inline auto Element::qt_create_metaobjectdata<qt_meta_tag_ZN7ElementE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Element",
        "linkedElementChanged",
        "",
        "elementInfoChange",
        "DiagramContext",
        "old_info",
        "new_info",
        "textAdded",
        "DynamicElementTextItem*",
        "deti",
        "textRemoved",
        "textsGroupAdded",
        "ElementTextItemGroup*",
        "group",
        "textsGroupAboutToBeRemoved",
        "textAddedToGroup",
        "text",
        "textRemovedFromGroup",
        "updateConductorTexts"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'linkedElementChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'elementInfoChange'
        QtMocHelpers::SignalData<void(DiagramContext, DiagramContext)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 }, { 0x80000000 | 4, 6 },
        }}),
        // Signal 'textAdded'
        QtMocHelpers::SignalData<void(DynamicElementTextItem *)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
        // Signal 'textRemoved'
        QtMocHelpers::SignalData<void(DynamicElementTextItem *)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
        // Signal 'textsGroupAdded'
        QtMocHelpers::SignalData<void(ElementTextItemGroup *)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'textsGroupAboutToBeRemoved'
        QtMocHelpers::SignalData<void(ElementTextItemGroup *)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'textAddedToGroup'
        QtMocHelpers::SignalData<void(DynamicElementTextItem *, ElementTextItemGroup *)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 16 }, { 0x80000000 | 12, 13 },
        }}),
        // Signal 'textRemovedFromGroup'
        QtMocHelpers::SignalData<void(DynamicElementTextItem *, ElementTextItemGroup *)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 16 }, { 0x80000000 | 12, 13 },
        }}),
        // Slot 'updateConductorTexts'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Element, qt_meta_tag_ZN7ElementE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Element::staticMetaObject = { {
    QMetaObject::SuperData::link<QetGraphicsItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ElementE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ElementE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7ElementE_t>.metaTypes,
    nullptr
} };

void Element::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Element *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->linkedElementChanged(); break;
        case 1: _t->elementInfoChange((*reinterpret_cast<std::add_pointer_t<DiagramContext>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<DiagramContext>>(_a[2]))); break;
        case 2: _t->textAdded((*reinterpret_cast<std::add_pointer_t<DynamicElementTextItem*>>(_a[1]))); break;
        case 3: _t->textRemoved((*reinterpret_cast<std::add_pointer_t<DynamicElementTextItem*>>(_a[1]))); break;
        case 4: _t->textsGroupAdded((*reinterpret_cast<std::add_pointer_t<ElementTextItemGroup*>>(_a[1]))); break;
        case 5: _t->textsGroupAboutToBeRemoved((*reinterpret_cast<std::add_pointer_t<ElementTextItemGroup*>>(_a[1]))); break;
        case 6: _t->textAddedToGroup((*reinterpret_cast<std::add_pointer_t<DynamicElementTextItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<ElementTextItemGroup*>>(_a[2]))); break;
        case 7: _t->textRemovedFromGroup((*reinterpret_cast<std::add_pointer_t<DynamicElementTextItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<ElementTextItemGroup*>>(_a[2]))); break;
        case 8: _t->updateConductorTexts(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Element::*)()>(_a, &Element::linkedElementChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Element::*)(DiagramContext , DiagramContext )>(_a, &Element::elementInfoChange, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Element::*)(DynamicElementTextItem * )>(_a, &Element::textAdded, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Element::*)(DynamicElementTextItem * )>(_a, &Element::textRemoved, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Element::*)(ElementTextItemGroup * )>(_a, &Element::textsGroupAdded, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (Element::*)(ElementTextItemGroup * )>(_a, &Element::textsGroupAboutToBeRemoved, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (Element::*)(DynamicElementTextItem * , ElementTextItemGroup * )>(_a, &Element::textAddedToGroup, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (Element::*)(DynamicElementTextItem * , ElementTextItemGroup * )>(_a, &Element::textRemovedFromGroup, 7))
            return;
    }
}

const QMetaObject *Element::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Element::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7ElementE_t>.strings))
        return static_cast<void*>(this);
    return QetGraphicsItem::qt_metacast(_clname);
}

int Element::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QetGraphicsItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void Element::linkedElementChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Element::elementInfoChange(DiagramContext _t1, DiagramContext _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void Element::textAdded(DynamicElementTextItem * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void Element::textRemoved(DynamicElementTextItem * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void Element::textsGroupAdded(ElementTextItemGroup * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void Element::textsGroupAboutToBeRemoved(ElementTextItemGroup * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void Element::textAddedToGroup(DynamicElementTextItem * _t1, ElementTextItemGroup * _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}

// SIGNAL 7
void Element::textRemovedFromGroup(DynamicElementTextItem * _t1, ElementTextItemGroup * _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2);
}
QT_WARNING_POP
