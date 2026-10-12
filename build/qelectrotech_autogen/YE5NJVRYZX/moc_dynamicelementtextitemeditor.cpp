/****************************************************************************
** Meta object code from reading C++ file 'dynamicelementtextitemeditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/dynamicelementtextitemeditor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dynamicelementtextitemeditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN28DynamicElementTextItemEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto DynamicElementTextItemEditor::qt_create_metaobjectdata<qt_meta_tag_ZN28DynamicElementTextItemEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DynamicElementTextItemEditor",
        "on_m_add_text_clicked",
        "",
        "on_m_remove_selection_clicked",
        "on_m_add_group_clicked",
        "on_m_tree_view_clicked",
        "QModelIndex",
        "index",
        "on_m_export_pb_clicked",
        "on_m_import_pb_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_m_add_text_clicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_remove_selection_clicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_add_group_clicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_tree_view_clicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Slot 'on_m_export_pb_clicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_import_pb_clicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DynamicElementTextItemEditor, qt_meta_tag_ZN28DynamicElementTextItemEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DynamicElementTextItemEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<AbstractElementPropertiesEditorWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28DynamicElementTextItemEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28DynamicElementTextItemEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN28DynamicElementTextItemEditorE_t>.metaTypes,
    nullptr
} };

void DynamicElementTextItemEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DynamicElementTextItemEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_m_add_text_clicked(); break;
        case 1: _t->on_m_remove_selection_clicked(); break;
        case 2: _t->on_m_add_group_clicked(); break;
        case 3: _t->on_m_tree_view_clicked((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 4: _t->on_m_export_pb_clicked(); break;
        case 5: _t->on_m_import_pb_clicked(); break;
        default: ;
        }
    }
}

const QMetaObject *DynamicElementTextItemEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DynamicElementTextItemEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28DynamicElementTextItemEditorE_t>.strings))
        return static_cast<void*>(this);
    return AbstractElementPropertiesEditorWidget::qt_metacast(_clname);
}

int DynamicElementTextItemEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = AbstractElementPropertiesEditorWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
