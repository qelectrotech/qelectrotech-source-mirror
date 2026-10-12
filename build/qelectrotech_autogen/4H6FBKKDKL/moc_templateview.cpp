/****************************************************************************
** Meta object code from reading C++ file 'templateview.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/titleblock/templateview.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'templateview.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN22TitleBlockTemplateViewE_t {};
} // unnamed namespace

template <> constexpr inline auto TitleBlockTemplateView::qt_create_metaobjectdata<qt_meta_tag_ZN22TitleBlockTemplateViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TitleBlockTemplateView",
        "selectedCellsChanged",
        "",
        "QList<TitleBlockCell*>",
        "gridModificationRequested",
        "TitleBlockTemplateCommand*",
        "previewWidthChanged",
        "setTitleBlockTemplate",
        "TitleBlockTemplate*",
        "selectionChanged",
        "zoomIn",
        "zoomOut",
        "zoomFit",
        "zoomReset",
        "cut",
        "copy",
        "mayPaste",
        "pastedCells",
        "QList<TitleBlockCell>",
        "paste",
        "addColumnAtEnd",
        "addRowAtEnd",
        "addColumnBefore",
        "addRowBefore",
        "addColumnAfter",
        "addRowAfter",
        "editColumn",
        "HelperCell*",
        "editRow",
        "deleteColumn",
        "deleteRow",
        "mergeSelectedCells",
        "splitSelectedCell",
        "refresh",
        "changePreviewWidth",
        "setPreviewWidth",
        "updateLayout",
        "rowsDimensionsChanged",
        "columnsDimensionsChanged",
        "updateDisplayedMinMaxWidth",
        "setReadOnly",
        "applyColumnsWidths",
        "applyRowsHeights",
        "updateRowsHelperCells",
        "updateColumnsHelperCells",
        "updateLastContextMenuCell",
        "adjustSceneRect"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'selectedCellsChanged'
        QtMocHelpers::SignalData<void(QList<TitleBlockCell*>)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'gridModificationRequested'
        QtMocHelpers::SignalData<void(TitleBlockTemplateCommand *)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Signal 'previewWidthChanged'
        QtMocHelpers::SignalData<void(int, int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'setTitleBlockTemplate'
        QtMocHelpers::SlotData<void(TitleBlockTemplate *)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 2 },
        }}),
        // Slot 'selectionChanged'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomIn'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomOut'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomFit'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomReset'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'cut'
        QtMocHelpers::SlotData<QList<TitleBlockCell*>()>(14, 2, QMC::AccessPublic, 0x80000000 | 3),
        // Slot 'copy'
        QtMocHelpers::SlotData<QList<TitleBlockCell*>()>(15, 2, QMC::AccessPublic, 0x80000000 | 3),
        // Slot 'mayPaste'
        QtMocHelpers::SlotData<bool()>(16, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'pastedCells'
        QtMocHelpers::SlotData<QList<TitleBlockCell>()>(17, 2, QMC::AccessPublic, 0x80000000 | 18),
        // Slot 'paste'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addColumnAtEnd'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addRowAtEnd'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addColumnBefore'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addRowBefore'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addColumnAfter'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addRowAfter'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editColumn'
        QtMocHelpers::SlotData<void(HelperCell *)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 27, 2 },
        }}),
        // Slot 'editColumn'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Slot 'editRow'
        QtMocHelpers::SlotData<void(HelperCell *)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 27, 2 },
        }}),
        // Slot 'editRow'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Slot 'deleteColumn'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'deleteRow'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'mergeSelectedCells'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'splitSelectedCell'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'refresh'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'changePreviewWidth'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setPreviewWidth'
        QtMocHelpers::SlotData<void(int)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'updateLayout'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'rowsDimensionsChanged'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'columnsDimensionsChanged'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateDisplayedMinMaxWidth'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setReadOnly'
        QtMocHelpers::SlotData<void(bool)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'applyColumnsWidths'
        QtMocHelpers::SlotData<void(bool)>(41, 2, QMC::AccessProtected, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'applyColumnsWidths'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessProtected | QMC::MethodCloned, QMetaType::Void),
        // Slot 'applyRowsHeights'
        QtMocHelpers::SlotData<void(bool)>(42, 2, QMC::AccessProtected, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'applyRowsHeights'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessProtected | QMC::MethodCloned, QMetaType::Void),
        // Slot 'updateRowsHelperCells'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessProtected, QMetaType::Void),
        // Slot 'updateColumnsHelperCells'
        QtMocHelpers::SlotData<void()>(44, 2, QMC::AccessProtected, QMetaType::Void),
        // Slot 'updateLastContextMenuCell'
        QtMocHelpers::SlotData<void(HelperCell *)>(45, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 27, 2 },
        }}),
        // Slot 'adjustSceneRect'
        QtMocHelpers::SlotData<void()>(46, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TitleBlockTemplateView, qt_meta_tag_ZN22TitleBlockTemplateViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TitleBlockTemplateView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22TitleBlockTemplateViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22TitleBlockTemplateViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN22TitleBlockTemplateViewE_t>.metaTypes,
    nullptr
} };

void TitleBlockTemplateView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TitleBlockTemplateView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->selectedCellsChanged((*reinterpret_cast<std::add_pointer_t<QList<TitleBlockCell*>>>(_a[1]))); break;
        case 1: _t->gridModificationRequested((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplateCommand*>>(_a[1]))); break;
        case 2: _t->previewWidthChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->setTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplate*>>(_a[1]))); break;
        case 4: _t->selectionChanged(); break;
        case 5: _t->zoomIn(); break;
        case 6: _t->zoomOut(); break;
        case 7: _t->zoomFit(); break;
        case 8: _t->zoomReset(); break;
        case 9: { QList<TitleBlockCell*> _r = _t->cut();
            if (_a[0]) *reinterpret_cast<QList<TitleBlockCell*>*>(_a[0]) = std::move(_r); }  break;
        case 10: { QList<TitleBlockCell*> _r = _t->copy();
            if (_a[0]) *reinterpret_cast<QList<TitleBlockCell*>*>(_a[0]) = std::move(_r); }  break;
        case 11: { bool _r = _t->mayPaste();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 12: { QList<TitleBlockCell> _r = _t->pastedCells();
            if (_a[0]) *reinterpret_cast<QList<TitleBlockCell>*>(_a[0]) = std::move(_r); }  break;
        case 13: _t->paste(); break;
        case 14: _t->addColumnAtEnd(); break;
        case 15: _t->addRowAtEnd(); break;
        case 16: _t->addColumnBefore(); break;
        case 17: _t->addRowBefore(); break;
        case 18: _t->addColumnAfter(); break;
        case 19: _t->addRowAfter(); break;
        case 20: _t->editColumn((*reinterpret_cast<std::add_pointer_t<HelperCell*>>(_a[1]))); break;
        case 21: _t->editColumn(); break;
        case 22: _t->editRow((*reinterpret_cast<std::add_pointer_t<HelperCell*>>(_a[1]))); break;
        case 23: _t->editRow(); break;
        case 24: _t->deleteColumn(); break;
        case 25: _t->deleteRow(); break;
        case 26: _t->mergeSelectedCells(); break;
        case 27: _t->splitSelectedCell(); break;
        case 28: _t->refresh(); break;
        case 29: _t->changePreviewWidth(); break;
        case 30: _t->setPreviewWidth((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 31: _t->updateLayout(); break;
        case 32: _t->rowsDimensionsChanged(); break;
        case 33: _t->columnsDimensionsChanged(); break;
        case 34: _t->updateDisplayedMinMaxWidth(); break;
        case 35: _t->setReadOnly((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 36: _t->applyColumnsWidths((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 37: _t->applyColumnsWidths(); break;
        case 38: _t->applyRowsHeights((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 39: _t->applyRowsHeights(); break;
        case 40: _t->updateRowsHelperCells(); break;
        case 41: _t->updateColumnsHelperCells(); break;
        case 42: _t->updateLastContextMenuCell((*reinterpret_cast<std::add_pointer_t<HelperCell*>>(_a[1]))); break;
        case 43: _t->adjustSceneRect(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplate* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TitleBlockTemplateView::*)(QList<TitleBlockCell*> )>(_a, &TitleBlockTemplateView::selectedCellsChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TitleBlockTemplateView::*)(TitleBlockTemplateCommand * )>(_a, &TitleBlockTemplateView::gridModificationRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (TitleBlockTemplateView::*)(int , int )>(_a, &TitleBlockTemplateView::previewWidthChanged, 2))
            return;
    }
}

const QMetaObject *TitleBlockTemplateView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TitleBlockTemplateView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22TitleBlockTemplateViewE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int TitleBlockTemplateView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 44)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 44;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 44)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 44;
    }
    return _id;
}

// SIGNAL 0
void TitleBlockTemplateView::selectedCellsChanged(QList<TitleBlockCell*> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void TitleBlockTemplateView::gridModificationRequested(TitleBlockTemplateCommand * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void TitleBlockTemplateView::previewWidthChanged(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}
QT_WARNING_POP
