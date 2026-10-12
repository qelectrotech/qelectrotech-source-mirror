/****************************************************************************
** Meta object code from reading C++ file 'elementview.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/elementview.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementview.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11ElementViewE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementView::qt_create_metaobjectdata<qt_meta_tag_ZN11ElementViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementView",
        "modeChanged",
        "",
        "setVisualisationMode",
        "setSelectionMode",
        "zoomIn",
        "zoomOut",
        "zoomInSlowly",
        "zoomOutSlowly",
        "zoomFit",
        "zoomReset",
        "adjustSceneRect",
        "resetSceneRect",
        "cut",
        "copy",
        "paste",
        "pasteInArea",
        "getPasteArea",
        "QRectF",
        "pasteAreaDefined",
        "ElementContent",
        "QPointF",
        "QDomDocument",
        "pasteWithOffset"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modeChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setVisualisationMode'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setSelectionMode'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomIn'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomOut'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomInSlowly'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomOutSlowly'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomFit'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomReset'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'adjustSceneRect'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'resetSceneRect'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'cut'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'copy'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'paste'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'pasteInArea'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'getPasteArea'
        QtMocHelpers::SlotData<void(const QRectF &)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 18, 2 },
        }}),
        // Slot 'pasteAreaDefined'
        QtMocHelpers::SlotData<ElementContent(const QRectF &)>(19, 2, QMC::AccessPrivate, 0x80000000 | 20, {{
            { 0x80000000 | 18, 2 },
        }}),
        // Slot 'paste'
        QtMocHelpers::SlotData<ElementContent(const QPointF &)>(15, 2, QMC::AccessPrivate, 0x80000000 | 20, {{
            { 0x80000000 | 21, 2 },
        }}),
        // Slot 'paste'
        QtMocHelpers::SlotData<ElementContent(const QDomDocument &, const QPointF &)>(15, 2, QMC::AccessPrivate, 0x80000000 | 20, {{
            { 0x80000000 | 22, 2 }, { 0x80000000 | 21, 2 },
        }}),
        // Slot 'pasteWithOffset'
        QtMocHelpers::SlotData<ElementContent(const QDomDocument &)>(23, 2, QMC::AccessPrivate, 0x80000000 | 20, {{
            { 0x80000000 | 22, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ElementView, qt_meta_tag_ZN11ElementViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ElementViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ElementViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11ElementViewE_t>.metaTypes,
    nullptr
} };

void ElementView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ElementView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modeChanged(); break;
        case 1: _t->setVisualisationMode(); break;
        case 2: _t->setSelectionMode(); break;
        case 3: _t->zoomIn(); break;
        case 4: _t->zoomOut(); break;
        case 5: _t->zoomInSlowly(); break;
        case 6: _t->zoomOutSlowly(); break;
        case 7: _t->zoomFit(); break;
        case 8: _t->zoomReset(); break;
        case 9: _t->adjustSceneRect(); break;
        case 10: _t->resetSceneRect(); break;
        case 11: _t->cut(); break;
        case 12: _t->copy(); break;
        case 13: _t->paste(); break;
        case 14: _t->pasteInArea(); break;
        case 15: _t->getPasteArea((*reinterpret_cast<std::add_pointer_t<QRectF>>(_a[1]))); break;
        case 16: { ElementContent _r = _t->pasteAreaDefined((*reinterpret_cast<std::add_pointer_t<QRectF>>(_a[1])));
            if (_a[0]) *reinterpret_cast<ElementContent*>(_a[0]) = std::move(_r); }  break;
        case 17: { ElementContent _r = _t->paste((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1])));
            if (_a[0]) *reinterpret_cast<ElementContent*>(_a[0]) = std::move(_r); }  break;
        case 18: { ElementContent _r = _t->paste((*reinterpret_cast<std::add_pointer_t<QDomDocument>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2])));
            if (_a[0]) *reinterpret_cast<ElementContent*>(_a[0]) = std::move(_r); }  break;
        case 19: { ElementContent _r = _t->pasteWithOffset((*reinterpret_cast<std::add_pointer_t<QDomDocument>>(_a[1])));
            if (_a[0]) *reinterpret_cast<ElementContent*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ElementView::*)()>(_a, &ElementView::modeChanged, 0))
            return;
    }
}

const QMetaObject *ElementView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElementView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ElementViewE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int ElementView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void ElementView::modeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
