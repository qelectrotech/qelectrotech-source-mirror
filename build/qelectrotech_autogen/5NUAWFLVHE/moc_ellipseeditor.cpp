/****************************************************************************
** Meta object code from reading C++ file 'ellipseeditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/ui/ellipseeditor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ellipseeditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13EllipseEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto EllipseEditor::qt_create_metaobjectdata<qt_meta_tag_ZN13EllipseEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "EllipseEditor",
        "on_m_x_sb_editingFinished",
        "",
        "on_m_y_sb_editingFinished",
        "on_m_horizontal_diameter_sb_editingFinished",
        "on_m_vertical_diameter_sb_editingFinished"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_m_x_sb_editingFinished'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_y_sb_editingFinished'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_horizontal_diameter_sb_editingFinished'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_vertical_diameter_sb_editingFinished'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<EllipseEditor, qt_meta_tag_ZN13EllipseEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject EllipseEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<ElementItemEditor::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13EllipseEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13EllipseEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13EllipseEditorE_t>.metaTypes,
    nullptr
} };

void EllipseEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<EllipseEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_m_x_sb_editingFinished(); break;
        case 1: _t->on_m_y_sb_editingFinished(); break;
        case 2: _t->on_m_horizontal_diameter_sb_editingFinished(); break;
        case 3: _t->on_m_vertical_diameter_sb_editingFinished(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *EllipseEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EllipseEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13EllipseEditorE_t>.strings))
        return static_cast<void*>(this);
    return ElementItemEditor::qt_metacast(_clname);
}

int EllipseEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ElementItemEditor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
