/****************************************************************************
** Meta object code from reading C++ file 'masterpropertieswidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/masterpropertieswidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'masterpropertieswidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN22MasterPropertiesWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto MasterPropertiesWidget::qt_create_metaobjectdata<qt_meta_tag_ZN22MasterPropertiesWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MasterPropertiesWidget",
        "updateUi",
        "",
        "headerCustomContextMenuRequested",
        "QPoint",
        "pos",
        "on_link_button_clicked",
        "on_unlink_button_clicked",
        "showElementFromTWI",
        "QTreeWidgetItem*",
        "qtwi",
        "column",
        "showedElementWasDeleted",
        "diagramWasdeletedFromProject",
        "customContextMenu",
        "i",
        "plcPasteFromClipboard",
        "setCellFromValue",
        "row",
        "col",
        "val",
        "plcAddRow",
        "plcRemoveRow",
        "plcMoveRowUp",
        "plcMoveRowDown",
        "plcIOCellChanged",
        "plcUpdateDisplaySettings",
        "plcShowTableContextMenu"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'updateUi'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'headerCustomContextMenuRequested'
        QtMocHelpers::SlotData<void(const QPoint &)>(3, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Slot 'on_link_button_clicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_unlink_button_clicked'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showElementFromTWI'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *, int)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 9, 10 }, { QMetaType::Int, 11 },
        }}),
        // Slot 'showedElementWasDeleted'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'diagramWasdeletedFromProject'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'customContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &, int)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 4, 5 }, { QMetaType::Int, 15 },
        }}),
        // Slot 'customContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(14, 2, QMC::AccessPrivate | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Slot 'plcPasteFromClipboard'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'setCellFromValue'
        QtMocHelpers::SlotData<void(int, int, const QString &)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 18 }, { QMetaType::Int, 19 }, { QMetaType::QString, 20 },
        }}),
        // Slot 'plcAddRow'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'plcRemoveRow'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'plcMoveRowUp'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'plcMoveRowDown'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'plcIOCellChanged'
        QtMocHelpers::SlotData<void(int, int)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 18 }, { QMetaType::Int, 11 },
        }}),
        // Slot 'plcUpdateDisplaySettings'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'plcShowTableContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MasterPropertiesWidget, qt_meta_tag_ZN22MasterPropertiesWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MasterPropertiesWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<AbstractElementPropertiesEditorWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22MasterPropertiesWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22MasterPropertiesWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN22MasterPropertiesWidgetE_t>.metaTypes,
    nullptr
} };

void MasterPropertiesWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MasterPropertiesWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->updateUi(); break;
        case 1: _t->headerCustomContextMenuRequested((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 2: _t->on_link_button_clicked(); break;
        case 3: _t->on_unlink_button_clicked(); break;
        case 4: _t->showElementFromTWI((*reinterpret_cast<std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->showedElementWasDeleted(); break;
        case 6: _t->diagramWasdeletedFromProject(); break;
        case 7: _t->customContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 8: _t->customContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 9: _t->plcPasteFromClipboard(); break;
        case 10: _t->setCellFromValue((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 11: _t->plcAddRow(); break;
        case 12: _t->plcRemoveRow(); break;
        case 13: _t->plcMoveRowUp(); break;
        case 14: _t->plcMoveRowDown(); break;
        case 15: _t->plcIOCellChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 16: _t->plcUpdateDisplaySettings(); break;
        case 17: _t->plcShowTableContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *MasterPropertiesWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MasterPropertiesWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22MasterPropertiesWidgetE_t>.strings))
        return static_cast<void*>(this);
    return AbstractElementPropertiesEditorWidget::qt_metacast(_clname);
}

int MasterPropertiesWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = AbstractElementPropertiesEditorWidget::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
