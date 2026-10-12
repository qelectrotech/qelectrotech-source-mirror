/****************************************************************************
** Meta object code from reading C++ file 'autonumberingdockwidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/autoNum/ui/autonumberingdockwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'autonumberingdockwidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN23AutoNumberingDockWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto AutoNumberingDockWidget::qt_create_metaobjectdata<qt_meta_tag_ZN23AutoNumberingDockWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AutoNumberingDockWidget",
        "setActive",
        "",
        "setConductorActive",
        "DiagramView*",
        "on_m_conductor_cb_activated",
        "on_m_element_cb_activated",
        "on_m_folio_cb_activated",
        "conductorAutoNumChanged",
        "elementAutoNumChanged",
        "folioAutoNumChanged",
        "clear",
        "projectClosed",
        "refreshValueFields",
        "on_m_configure_pb_clicked",
        "on_m_conductor_reset_start_pb_clicked",
        "on_m_element_reset_start_pb_clicked",
        "on_m_folio_reset_start_pb_clicked",
        "on_m_conductor_value_le_editingFinished",
        "on_m_element_value_le_editingFinished",
        "on_m_folio_value_le_editingFinished",
        "on_m_conductor_increase_sb_valueChanged",
        "on_m_element_increase_sb_valueChanged",
        "on_m_folio_increase_sb_valueChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'setActive'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setConductorActive'
        QtMocHelpers::SlotData<void(DiagramView *)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 2 },
        }}),
        // Slot 'on_m_conductor_cb_activated'
        QtMocHelpers::SlotData<void(int)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_m_element_cb_activated'
        QtMocHelpers::SlotData<void(int)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_m_folio_cb_activated'
        QtMocHelpers::SlotData<void(int)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'conductorAutoNumChanged'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'elementAutoNumChanged'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'folioAutoNumChanged'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'clear'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'projectClosed'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'refreshValueFields'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_configure_pb_clicked'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_conductor_reset_start_pb_clicked'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_element_reset_start_pb_clicked'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_folio_reset_start_pb_clicked'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_conductor_value_le_editingFinished'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_element_value_le_editingFinished'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_folio_value_le_editingFinished'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_conductor_increase_sb_valueChanged'
        QtMocHelpers::SlotData<void(int)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_m_element_increase_sb_valueChanged'
        QtMocHelpers::SlotData<void(int)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_m_folio_increase_sb_valueChanged'
        QtMocHelpers::SlotData<void(int)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AutoNumberingDockWidget, qt_meta_tag_ZN23AutoNumberingDockWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AutoNumberingDockWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23AutoNumberingDockWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23AutoNumberingDockWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN23AutoNumberingDockWidgetE_t>.metaTypes,
    nullptr
} };

void AutoNumberingDockWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AutoNumberingDockWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->setActive(); break;
        case 1: _t->setConductorActive((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 2: _t->on_m_conductor_cb_activated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->on_m_element_cb_activated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->on_m_folio_cb_activated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->conductorAutoNumChanged(); break;
        case 6: _t->elementAutoNumChanged(); break;
        case 7: _t->folioAutoNumChanged(); break;
        case 8: _t->clear(); break;
        case 9: _t->projectClosed(); break;
        case 10: _t->refreshValueFields(); break;
        case 11: _t->on_m_configure_pb_clicked(); break;
        case 12: _t->on_m_conductor_reset_start_pb_clicked(); break;
        case 13: _t->on_m_element_reset_start_pb_clicked(); break;
        case 14: _t->on_m_folio_reset_start_pb_clicked(); break;
        case 15: _t->on_m_conductor_value_le_editingFinished(); break;
        case 16: _t->on_m_element_value_le_editingFinished(); break;
        case 17: _t->on_m_folio_value_le_editingFinished(); break;
        case 18: _t->on_m_conductor_increase_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 19: _t->on_m_element_increase_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 20: _t->on_m_folio_increase_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *AutoNumberingDockWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AutoNumberingDockWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23AutoNumberingDockWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int AutoNumberingDockWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 21;
    }
    return _id;
}
QT_WARNING_POP
