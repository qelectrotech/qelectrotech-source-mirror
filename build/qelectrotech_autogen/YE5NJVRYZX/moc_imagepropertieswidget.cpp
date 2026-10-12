/****************************************************************************
** Meta object code from reading C++ file 'imagepropertieswidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/imagepropertieswidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'imagepropertieswidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN21ImagePropertiesWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto ImagePropertiesWidget::qt_create_metaobjectdata<qt_meta_tag_ZN21ImagePropertiesWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ImagePropertiesWidget",
        "on_m_width_sb_valueChanged",
        "",
        "value",
        "on_m_height_sb_valueChanged",
        "on_m_angle_sb_valueChanged",
        "on_m_skew_x_sb_valueChanged",
        "on_m_skew_y_sb_valueChanged",
        "on_m_lock_ratio_tb_toggled",
        "checked",
        "on_m_restore_ratio_pb_clicked",
        "on_m_lock_pos_cb_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_m_width_sb_valueChanged'
        QtMocHelpers::SlotData<void(double)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 3 },
        }}),
        // Slot 'on_m_height_sb_valueChanged'
        QtMocHelpers::SlotData<void(double)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 3 },
        }}),
        // Slot 'on_m_angle_sb_valueChanged'
        QtMocHelpers::SlotData<void(double)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 3 },
        }}),
        // Slot 'on_m_skew_x_sb_valueChanged'
        QtMocHelpers::SlotData<void(double)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 3 },
        }}),
        // Slot 'on_m_skew_y_sb_valueChanged'
        QtMocHelpers::SlotData<void(double)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 3 },
        }}),
        // Slot 'on_m_lock_ratio_tb_toggled'
        QtMocHelpers::SlotData<void(bool)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 9 },
        }}),
        // Slot 'on_m_restore_ratio_pb_clicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_lock_pos_cb_clicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ImagePropertiesWidget, qt_meta_tag_ZN21ImagePropertiesWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ImagePropertiesWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<PropertiesEditorWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21ImagePropertiesWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21ImagePropertiesWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN21ImagePropertiesWidgetE_t>.metaTypes,
    nullptr
} };

void ImagePropertiesWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ImagePropertiesWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_m_width_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 1: _t->on_m_height_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 2: _t->on_m_angle_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 3: _t->on_m_skew_x_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 4: _t->on_m_skew_y_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 5: _t->on_m_lock_ratio_tb_toggled((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->on_m_restore_ratio_pb_clicked(); break;
        case 7: _t->on_m_lock_pos_cb_clicked(); break;
        default: ;
        }
    }
}

const QMetaObject *ImagePropertiesWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImagePropertiesWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21ImagePropertiesWidgetE_t>.strings))
        return static_cast<void*>(this);
    return PropertiesEditorWidget::qt_metacast(_clname);
}

int ImagePropertiesWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PropertiesEditorWidget::qt_metacall(_c, _id, _a);
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
    return _id;
}
QT_WARNING_POP
