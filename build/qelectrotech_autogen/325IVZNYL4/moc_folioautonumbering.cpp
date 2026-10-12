/****************************************************************************
** Meta object code from reading C++ file 'folioautonumbering.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/autoNum/ui/folioautonumbering.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'folioautonumbering.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19FolioAutonumberingWE_t {};
} // unnamed namespace

template <> constexpr inline auto FolioAutonumberingW::qt_create_metaobjectdata<qt_meta_tag_ZN19FolioAutonumberingWE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "FolioAutonumberingW",
        "applyPressed",
        "",
        "m_autonumber_tabs_rb_clicked",
        "on_m_create_new_tabs_rb_clicked",
        "on_m_autonumber_tabs_rb_clicked",
        "on_m_new_tabs_sb_valueChanged",
        "on_buttonBox_clicked",
        "QAbstractButton*",
        "on_m_from_cb_currentIndexChanged",
        "applyEnable"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'applyPressed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'm_autonumber_tabs_rb_clicked'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'on_m_create_new_tabs_rb_clicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_autonumber_tabs_rb_clicked'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_new_tabs_sb_valueChanged'
        QtMocHelpers::SlotData<void(int)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_buttonBox_clicked'
        QtMocHelpers::SlotData<void(QAbstractButton *)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 8, 2 },
        }}),
        // Slot 'on_m_from_cb_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'applyEnable'
        QtMocHelpers::SlotData<void(bool)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'applyEnable'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate | QMC::MethodCloned, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<FolioAutonumberingW, qt_meta_tag_ZN19FolioAutonumberingWE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject FolioAutonumberingW::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19FolioAutonumberingWE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19FolioAutonumberingWE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN19FolioAutonumberingWE_t>.metaTypes,
    nullptr
} };

void FolioAutonumberingW::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FolioAutonumberingW *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->applyPressed(); break;
        case 1: _t->m_autonumber_tabs_rb_clicked(); break;
        case 2: _t->on_m_create_new_tabs_rb_clicked(); break;
        case 3: _t->on_m_autonumber_tabs_rb_clicked(); break;
        case 4: _t->on_m_new_tabs_sb_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->on_buttonBox_clicked((*reinterpret_cast<std::add_pointer_t<QAbstractButton*>>(_a[1]))); break;
        case 6: _t->on_m_from_cb_currentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->applyEnable((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->applyEnable(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (FolioAutonumberingW::*)()>(_a, &FolioAutonumberingW::applyPressed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (FolioAutonumberingW::*)()>(_a, &FolioAutonumberingW::m_autonumber_tabs_rb_clicked, 1))
            return;
    }
}

const QMetaObject *FolioAutonumberingW::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FolioAutonumberingW::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19FolioAutonumberingWE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int FolioAutonumberingW::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void FolioAutonumberingW::applyPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void FolioAutonumberingW::m_autonumber_tabs_rb_clicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
