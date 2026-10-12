/****************************************************************************
** Meta object code from reading C++ file 'graphicstablepropertieseditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/ViewItem/ui/graphicstablepropertieseditor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'graphicstablepropertieseditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN29GraphicsTablePropertiesEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto GraphicsTablePropertiesEditor::qt_create_metaobjectdata<qt_meta_tag_ZN29GraphicsTablePropertiesEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GraphicsTablePropertiesEditor",
        "on_m_header_font_pb_clicked",
        "",
        "on_m_table_font_pb_clicked",
        "updateUi",
        "updatePosWidget",
        "updateInfoLabel",
        "on_m_table_name_le_textEdited",
        "arg1",
        "on_m_previous_table_cb_activated",
        "index",
        "on_m_previous_pb_clicked",
        "on_m_next_pb_clicked",
        "on_m_auto_geometry_pb_clicked",
        "on_m_apply_geometry_to_linked_table_pb_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_m_header_font_pb_clicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_table_font_pb_clicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateUi'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updatePosWidget'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateInfoLabel'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_table_name_le_textEdited'
        QtMocHelpers::SlotData<void(const QString &)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'on_m_previous_table_cb_activated'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'on_m_previous_pb_clicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_next_pb_clicked'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_auto_geometry_pb_clicked'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_apply_geometry_to_linked_table_pb_clicked'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GraphicsTablePropertiesEditor, qt_meta_tag_ZN29GraphicsTablePropertiesEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GraphicsTablePropertiesEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<PropertiesEditorWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN29GraphicsTablePropertiesEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN29GraphicsTablePropertiesEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN29GraphicsTablePropertiesEditorE_t>.metaTypes,
    nullptr
} };

void GraphicsTablePropertiesEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GraphicsTablePropertiesEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_m_header_font_pb_clicked(); break;
        case 1: _t->on_m_table_font_pb_clicked(); break;
        case 2: _t->updateUi(); break;
        case 3: _t->updatePosWidget(); break;
        case 4: _t->updateInfoLabel(); break;
        case 5: _t->on_m_table_name_le_textEdited((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->on_m_previous_table_cb_activated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->on_m_previous_pb_clicked(); break;
        case 8: _t->on_m_next_pb_clicked(); break;
        case 9: _t->on_m_auto_geometry_pb_clicked(); break;
        case 10: _t->on_m_apply_geometry_to_linked_table_pb_clicked(); break;
        default: ;
        }
    }
}

const QMetaObject *GraphicsTablePropertiesEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GraphicsTablePropertiesEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN29GraphicsTablePropertiesEditorE_t>.strings))
        return static_cast<void*>(this);
    return PropertiesEditorWidget::qt_metacast(_clname);
}

int GraphicsTablePropertiesEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PropertiesEditorWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
