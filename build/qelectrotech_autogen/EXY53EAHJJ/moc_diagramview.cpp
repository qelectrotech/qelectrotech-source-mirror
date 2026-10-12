/****************************************************************************
** Meta object code from reading C++ file 'diagramview.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/diagramview.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'diagramview.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11DiagramViewE_t {};
} // unnamed namespace

template <> constexpr inline auto DiagramView::qt_create_metaobjectdata<qt_meta_tag_ZN11DiagramViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DiagramView",
        "modeChanged",
        "",
        "titleChanged",
        "DiagramView*",
        "findElementRequired",
        "ElementsLocation",
        "showDiagram",
        "Diagram*",
        "freeRubberBandChanged",
        "QPolygonF",
        "polygon",
        "setVisualisationMode",
        "setSelectionMode",
        "zoom",
        "zoom_factor",
        "zoomFit",
        "zoomContent",
        "zoomReset",
        "cut",
        "copy",
        "paste",
        "QPointF",
        "QClipboard::Mode",
        "pasteHere",
        "adjustSceneRect",
        "updateWindowTitle",
        "resetConductors",
        "adjustGridToZoom",
        "applyReadOnly",
        "createTemplateFromSelection"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modeChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'titleChanged'
        QtMocHelpers::SignalData<void(DiagramView *, const QString &)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 2 }, { QMetaType::QString, 2 },
        }}),
        // Signal 'findElementRequired'
        QtMocHelpers::SignalData<void(const ElementsLocation &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 2 },
        }}),
        // Signal 'showDiagram'
        QtMocHelpers::SignalData<void(Diagram *)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 2 },
        }}),
        // Signal 'freeRubberBandChanged'
        QtMocHelpers::SignalData<void(QPolygonF)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Slot 'setVisualisationMode'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setSelectionMode'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoom'
        QtMocHelpers::SlotData<void(const qreal)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QReal, 15 },
        }}),
        // Slot 'zoomFit'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomContent'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomReset'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'cut'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'copy'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'paste'
        QtMocHelpers::SlotData<void(const QPointF &, QClipboard::Mode)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 22, 2 }, { 0x80000000 | 23, 2 },
        }}),
        // Slot 'paste'
        QtMocHelpers::SlotData<void(const QPointF &)>(21, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 22, 2 },
        }}),
        // Slot 'paste'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Slot 'pasteHere'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'adjustSceneRect'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateWindowTitle'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'resetConductors'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'adjustGridToZoom'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'applyReadOnly'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'createTemplateFromSelection'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DiagramView, qt_meta_tag_ZN11DiagramViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DiagramView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11DiagramViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11DiagramViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11DiagramViewE_t>.metaTypes,
    nullptr
} };

void DiagramView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DiagramView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modeChanged(); break;
        case 1: _t->titleChanged((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->findElementRequired((*reinterpret_cast<std::add_pointer_t<ElementsLocation>>(_a[1]))); break;
        case 3: _t->showDiagram((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 4: _t->freeRubberBandChanged((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1]))); break;
        case 5: _t->setVisualisationMode(); break;
        case 6: _t->setSelectionMode(); break;
        case 7: _t->zoom((*reinterpret_cast<std::add_pointer_t<qreal>>(_a[1]))); break;
        case 8: _t->zoomFit(); break;
        case 9: _t->zoomContent(); break;
        case 10: _t->zoomReset(); break;
        case 11: _t->cut(); break;
        case 12: _t->copy(); break;
        case 13: _t->paste((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QClipboard::Mode>>(_a[2]))); break;
        case 14: _t->paste((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 15: _t->paste(); break;
        case 16: _t->pasteHere(); break;
        case 17: _t->adjustSceneRect(); break;
        case 18: _t->updateWindowTitle(); break;
        case 19: _t->resetConductors(); break;
        case 20: _t->adjustGridToZoom(); break;
        case 21: _t->applyReadOnly(); break;
        case 22: _t->createTemplateFromSelection(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< DiagramView* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ElementsLocation >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DiagramView::*)()>(_a, &DiagramView::modeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramView::*)(DiagramView * , const QString & )>(_a, &DiagramView::titleChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramView::*)(const ElementsLocation & )>(_a, &DiagramView::findElementRequired, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramView::*)(Diagram * )>(_a, &DiagramView::showDiagram, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramView::*)(QPolygonF )>(_a, &DiagramView::freeRubberBandChanged, 4))
            return;
    }
}

const QMetaObject *DiagramView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DiagramView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11DiagramViewE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int DiagramView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    return _id;
}

// SIGNAL 0
void DiagramView::modeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DiagramView::titleChanged(DiagramView * _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void DiagramView::findElementRequired(const ElementsLocation & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void DiagramView::showDiagram(Diagram * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void DiagramView::freeRubberBandChanged(QPolygonF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
