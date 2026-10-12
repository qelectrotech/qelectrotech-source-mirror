/****************************************************************************
** Meta object code from reading C++ file 'elementquerywidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/dataBase/ui/elementquerywidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementquerywidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN18ElementQueryWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementQueryWidget::qt_create_metaobjectdata<qt_meta_tag_ZN18ElementQueryWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementQueryWidget",
        "on_m_up_pb_clicked",
        "",
        "on_m_add_pb_clicked",
        "on_m_remove_pb_clicked",
        "on_m_down_pb_clicked",
        "on_m_edit_sql_query_cb_clicked",
        "on_m_filter_le_textEdited",
        "arg1",
        "on_m_filter_type_cb_activated",
        "index",
        "on_m_load_pb_clicked",
        "on_m_save_current_conf_pb_clicked",
        "on_m_save_name_le_textChanged",
        "on_m_choosen_list_currentItemChanged",
        "QListWidgetItem*",
        "current",
        "previous",
        "on_m_var_list_itemDoubleClicked",
        "item",
        "on_m_choosen_list_itemDoubleClicked",
        "updateQueryLine",
        "selectedKeys",
        "setUpItems",
        "FilterFor",
        "std::pair<int,QString>",
        "key",
        "fillSavedQuery",
        "reset"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_m_up_pb_clicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_add_pb_clicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_remove_pb_clicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_down_pb_clicked'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_edit_sql_query_cb_clicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_filter_le_textEdited'
        QtMocHelpers::SlotData<void(const QString &)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'on_m_filter_type_cb_activated'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'on_m_load_pb_clicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_save_current_conf_pb_clicked'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_save_name_le_textChanged'
        QtMocHelpers::SlotData<void(const QString &)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'on_m_choosen_list_currentItemChanged'
        QtMocHelpers::SlotData<void(QListWidgetItem *, QListWidgetItem *)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 16 }, { 0x80000000 | 15, 17 },
        }}),
        // Slot 'on_m_var_list_itemDoubleClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 19 },
        }}),
        // Slot 'on_m_choosen_list_itemDoubleClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 19 },
        }}),
        // Slot 'updateQueryLine'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'selectedKeys'
        QtMocHelpers::SlotData<QStringList() const>(22, 2, QMC::AccessPrivate, QMetaType::QStringList),
        // Slot 'setUpItems'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'FilterFor'
        QtMocHelpers::SlotData<QPair<int,QString>(const QString &) const>(24, 2, QMC::AccessPrivate, 0x80000000 | 25, {{
            { QMetaType::QString, 26 },
        }}),
        // Slot 'fillSavedQuery'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'reset'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ElementQueryWidget, qt_meta_tag_ZN18ElementQueryWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementQueryWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18ElementQueryWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18ElementQueryWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18ElementQueryWidgetE_t>.metaTypes,
    nullptr
} };

void ElementQueryWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ElementQueryWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_m_up_pb_clicked(); break;
        case 1: _t->on_m_add_pb_clicked(); break;
        case 2: _t->on_m_remove_pb_clicked(); break;
        case 3: _t->on_m_down_pb_clicked(); break;
        case 4: _t->on_m_edit_sql_query_cb_clicked(); break;
        case 5: _t->on_m_filter_le_textEdited((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->on_m_filter_type_cb_activated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->on_m_load_pb_clicked(); break;
        case 8: _t->on_m_save_current_conf_pb_clicked(); break;
        case 9: _t->on_m_save_name_le_textChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->on_m_choosen_list_currentItemChanged((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[2]))); break;
        case 11: _t->on_m_var_list_itemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 12: _t->on_m_choosen_list_itemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 13: _t->updateQueryLine(); break;
        case 14: { QStringList _r = _t->selectedKeys();
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 15: _t->setUpItems(); break;
        case 16: { std::pair<int,QString> _r = _t->FilterFor((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<std::pair<int,QString>*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->fillSavedQuery(); break;
        case 18: _t->reset(); break;
        default: ;
        }
    }
}

const QMetaObject *ElementQueryWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElementQueryWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18ElementQueryWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ElementQueryWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}
QT_WARNING_POP
