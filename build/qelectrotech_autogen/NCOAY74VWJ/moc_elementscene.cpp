/****************************************************************************
** Meta object code from reading C++ file 'elementscene.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/elementscene.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementscene.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12ElementSceneE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementScene::qt_create_metaobjectdata<qt_meta_tag_ZN12ElementSceneE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementScene",
        "partsAdded",
        "",
        "partsRemoved",
        "partsZValueChanged",
        "pasteAreaDefined",
        "QRectF",
        "needZoomFit",
        "elementInfoChanged",
        "elementTypeChanged",
        "mouseMoved",
        "QPointF",
        "pos",
        "slot_select",
        "ElementContent",
        "slot_selectAll",
        "slot_deselectAll",
        "slot_invertSelection",
        "slot_delete",
        "slot_editNames",
        "slot_editAuthorInformations",
        "slot_editProperties",
        "managePrimitivesGroups",
        "stackAction",
        "ElementEditionCommand*"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'partsAdded'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'partsRemoved'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'partsZValueChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pasteAreaDefined'
        QtMocHelpers::SignalData<void(const QRectF &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 2 },
        }}),
        // Signal 'needZoomFit'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'elementInfoChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'elementTypeChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'mouseMoved'
        QtMocHelpers::SignalData<void(const QPointF &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Slot 'slot_select'
        QtMocHelpers::SlotData<void(const ElementContent &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'slot_selectAll'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_deselectAll'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_invertSelection'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_delete'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_editNames'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_editAuthorInformations'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_editProperties'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'managePrimitivesGroups'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'stackAction'
        QtMocHelpers::SlotData<void(ElementEditionCommand *)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 24, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ElementScene, qt_meta_tag_ZN12ElementSceneE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementScene::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsScene::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ElementSceneE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ElementSceneE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12ElementSceneE_t>.metaTypes,
    nullptr
} };

void ElementScene::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ElementScene *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->partsAdded(); break;
        case 1: _t->partsRemoved(); break;
        case 2: _t->partsZValueChanged(); break;
        case 3: _t->pasteAreaDefined((*reinterpret_cast<std::add_pointer_t<QRectF>>(_a[1]))); break;
        case 4: _t->needZoomFit(); break;
        case 5: _t->elementInfoChanged(); break;
        case 6: _t->elementTypeChanged(); break;
        case 7: _t->mouseMoved((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 8: _t->slot_select((*reinterpret_cast<std::add_pointer_t<ElementContent>>(_a[1]))); break;
        case 9: _t->slot_selectAll(); break;
        case 10: _t->slot_deselectAll(); break;
        case 11: _t->slot_invertSelection(); break;
        case 12: _t->slot_delete(); break;
        case 13: _t->slot_editNames(); break;
        case 14: _t->slot_editAuthorInformations(); break;
        case 15: _t->slot_editProperties(); break;
        case 16: _t->managePrimitivesGroups(); break;
        case 17: _t->stackAction((*reinterpret_cast<std::add_pointer_t<ElementEditionCommand*>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)()>(_a, &ElementScene::partsAdded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)()>(_a, &ElementScene::partsRemoved, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)()>(_a, &ElementScene::partsZValueChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)(const QRectF & )>(_a, &ElementScene::pasteAreaDefined, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)()>(_a, &ElementScene::needZoomFit, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)()>(_a, &ElementScene::elementInfoChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)()>(_a, &ElementScene::elementTypeChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementScene::*)(const QPointF & )>(_a, &ElementScene::mouseMoved, 7))
            return;
    }
}

const QMetaObject *ElementScene::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElementScene::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ElementSceneE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsScene::qt_metacast(_clname);
}

int ElementScene::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsScene::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void ElementScene::partsAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ElementScene::partsRemoved()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ElementScene::partsZValueChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ElementScene::pasteAreaDefined(const QRectF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ElementScene::needZoomFit()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ElementScene::elementInfoChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ElementScene::elementTypeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ElementScene::mouseMoved(const QPointF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
