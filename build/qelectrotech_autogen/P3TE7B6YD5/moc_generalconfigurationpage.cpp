/****************************************************************************
** Meta object code from reading C++ file 'generalconfigurationpage.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/configpage/generalconfigurationpage.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'generalconfigurationpage.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN24GeneralConfigurationPageE_t {};
} // unnamed namespace

template <> constexpr inline auto GeneralConfigurationPage::qt_create_metaobjectdata<qt_meta_tag_ZN24GeneralConfigurationPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GeneralConfigurationPage",
        "on_m_font_pb_clicked",
        "",
        "on_m_dyn_text_font_pb_clicked",
        "on_m_common_elmt_path_cb_currentIndexChanged",
        "index",
        "on_m_company_elmt_path_cb_currentIndexChanged",
        "on_m_custom_elmt_path_cb_currentIndexChanged",
        "on_m_company_tbt_path_cb_currentIndexChanged",
        "on_m_custom_tbt_path_cb_currentIndexChanged",
        "on_m_user_macros_path_cb_currentIndexChanged",
        "on_m_indi_text_font_pb_clicked",
        "on_MaxPartsElementEditorList_sb_valueChanged",
        "value",
        "on_DiagramEditor_Grid_PointSize_min_sb_valueChanged",
        "on_ElementEditor_Grid_PointSize_min_sb_valueChanged",
        "on_m_hdpi_round_cb_clicked",
        "checked"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_m_font_pb_clicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_dyn_text_font_pb_clicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_common_elmt_path_cb_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'on_m_company_elmt_path_cb_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'on_m_custom_elmt_path_cb_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'on_m_company_tbt_path_cb_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'on_m_custom_tbt_path_cb_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'on_m_user_macros_path_cb_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'on_m_indi_text_font_pb_clicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_MaxPartsElementEditorList_sb_valueChanged'
        QtMocHelpers::SlotData<void(int)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Slot 'on_DiagramEditor_Grid_PointSize_min_sb_valueChanged'
        QtMocHelpers::SlotData<void(int)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Slot 'on_ElementEditor_Grid_PointSize_min_sb_valueChanged'
        QtMocHelpers::SlotData<void(int)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Slot 'on_m_hdpi_round_cb_clicked'
        QtMocHelpers::SlotData<void(bool)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 17 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GeneralConfigurationPage, qt_meta_tag_ZN24GeneralConfigurationPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GeneralConfigurationPage::staticMetaObject = { {
    QMetaObject::SuperData::link<ConfigPage::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN24GeneralConfigurationPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN24GeneralConfigurationPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN24GeneralConfigurationPageE_t>.metaTypes,
    nullptr
} };

void GeneralConfigurationPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GeneralConfigurationPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_m_font_pb_clicked(); break;
        case 1: _t->on_m_dyn_text_font_pb_clicked(); break;
        case 2: _t->on_m_common_elmt_path_cb_currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->on_m_company_elmt_path_cb_currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->on_m_custom_elmt_path_cb_currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->on_m_company_tbt_path_cb_currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->on_m_custom_tbt_path_cb_currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->on_m_user_macros_path_cb_currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->on_m_indi_text_font_pb_clicked(); break;
        case 9: _t->on_MaxPartsElementEditorList_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->on_DiagramEditor_Grid_PointSize_min_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->on_ElementEditor_Grid_PointSize_min_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->on_m_hdpi_round_cb_clicked((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *GeneralConfigurationPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GeneralConfigurationPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN24GeneralConfigurationPageE_t>.strings))
        return static_cast<void*>(this);
    return ConfigPage::qt_metacast(_clname);
}

int GeneralConfigurationPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ConfigPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
