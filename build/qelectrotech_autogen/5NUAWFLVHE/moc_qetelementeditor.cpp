/****************************************************************************
** Meta object code from reading C++ file 'qetelementeditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/ui/qetelementeditor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qetelementeditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16QETElementEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto QETElementEditor::qt_create_metaobjectdata<qt_meta_tag_ZN16QETElementEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QETElementEditor",
        "saveToLocation",
        "",
        "ElementsLocation",
        "loc",
        "on_m_save_action_triggered",
        "on_m_save_as_action_triggered",
        "on_m_select_all_act_triggered",
        "on_m_edit_element_properties_action_triggered",
        "on_m_new_action_triggered",
        "on_m_open_action_triggered",
        "on_m_open_from_file_action_triggered",
        "on_m_save_as_file_action_triggered",
        "on_m_export_svg_action_triggered",
        "on_m_reload_action_triggered",
        "on_m_quit_action_triggered",
        "on_m_deselect_all_action_triggered",
        "on_m_cut_action_triggered",
        "on_m_copy_action_triggered",
        "on_m_paste_action_triggered",
        "on_m_paste_in_area_action_triggered",
        "on_m_paste_from_file_action_triggered",
        "on_m_paste_from_element_action_triggered",
        "on_m_revert_selection_action_triggered",
        "on_m_delete_action_triggered",
        "on_m_edit_names_action_triggered",
        "on_m_edit_author_action_triggered",
        "on_m_zoom_in_action_triggered",
        "on_m_zoom_out_action_triggered",
        "on_m_zoom_fit_best_action_triggered",
        "on_m_zoom_original_action_triggered",
        "on_m_about_qet_action_triggered",
        "on_m_online_manual_triggered",
        "on_m_youtube_action_triggered",
        "on_m_donate_action_triggered",
        "on_m_about_qt_action_triggered",
        "on_m_import_dxf_triggered",
        "on_m_import_scaled_element_triggered"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'saveToLocation'
        QtMocHelpers::SignalData<void(ElementsLocation)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'on_m_save_action_triggered'
        QtMocHelpers::SlotData<bool()>(5, 2, QMC::AccessPrivate, QMetaType::Bool),
        // Slot 'on_m_save_as_action_triggered'
        QtMocHelpers::SlotData<bool()>(6, 2, QMC::AccessPrivate, QMetaType::Bool),
        // Slot 'on_m_select_all_act_triggered'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_edit_element_properties_action_triggered'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_new_action_triggered'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_open_action_triggered'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_open_from_file_action_triggered'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_save_as_file_action_triggered'
        QtMocHelpers::SlotData<bool()>(12, 2, QMC::AccessPrivate, QMetaType::Bool),
        // Slot 'on_m_export_svg_action_triggered'
        QtMocHelpers::SlotData<bool()>(13, 2, QMC::AccessPrivate, QMetaType::Bool),
        // Slot 'on_m_reload_action_triggered'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_quit_action_triggered'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_deselect_all_action_triggered'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_cut_action_triggered'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_copy_action_triggered'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_paste_action_triggered'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_paste_in_area_action_triggered'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_paste_from_file_action_triggered'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_paste_from_element_action_triggered'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_revert_selection_action_triggered'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_delete_action_triggered'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_edit_names_action_triggered'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_edit_author_action_triggered'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_zoom_in_action_triggered'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_zoom_out_action_triggered'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_zoom_fit_best_action_triggered'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_zoom_original_action_triggered'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_about_qet_action_triggered'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_online_manual_triggered'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_youtube_action_triggered'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_donate_action_triggered'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_about_qt_action_triggered'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_import_dxf_triggered'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_import_scaled_element_triggered'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QETElementEditor, qt_meta_tag_ZN16QETElementEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QETElementEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16QETElementEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16QETElementEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16QETElementEditorE_t>.metaTypes,
    nullptr
} };

void QETElementEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QETElementEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->saveToLocation((*reinterpret_cast<std::add_pointer_t<ElementsLocation>>(_a[1]))); break;
        case 1: { bool _r = _t->on_m_save_action_triggered();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 2: { bool _r = _t->on_m_save_as_action_triggered();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->on_m_select_all_act_triggered(); break;
        case 4: _t->on_m_edit_element_properties_action_triggered(); break;
        case 5: _t->on_m_new_action_triggered(); break;
        case 6: _t->on_m_open_action_triggered(); break;
        case 7: _t->on_m_open_from_file_action_triggered(); break;
        case 8: { bool _r = _t->on_m_save_as_file_action_triggered();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->on_m_export_svg_action_triggered();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 10: _t->on_m_reload_action_triggered(); break;
        case 11: _t->on_m_quit_action_triggered(); break;
        case 12: _t->on_m_deselect_all_action_triggered(); break;
        case 13: _t->on_m_cut_action_triggered(); break;
        case 14: _t->on_m_copy_action_triggered(); break;
        case 15: _t->on_m_paste_action_triggered(); break;
        case 16: _t->on_m_paste_in_area_action_triggered(); break;
        case 17: _t->on_m_paste_from_file_action_triggered(); break;
        case 18: _t->on_m_paste_from_element_action_triggered(); break;
        case 19: _t->on_m_revert_selection_action_triggered(); break;
        case 20: _t->on_m_delete_action_triggered(); break;
        case 21: _t->on_m_edit_names_action_triggered(); break;
        case 22: _t->on_m_edit_author_action_triggered(); break;
        case 23: _t->on_m_zoom_in_action_triggered(); break;
        case 24: _t->on_m_zoom_out_action_triggered(); break;
        case 25: _t->on_m_zoom_fit_best_action_triggered(); break;
        case 26: _t->on_m_zoom_original_action_triggered(); break;
        case 27: _t->on_m_about_qet_action_triggered(); break;
        case 28: _t->on_m_online_manual_triggered(); break;
        case 29: _t->on_m_youtube_action_triggered(); break;
        case 30: _t->on_m_donate_action_triggered(); break;
        case 31: _t->on_m_about_qt_action_triggered(); break;
        case 32: _t->on_m_import_dxf_triggered(); break;
        case 33: _t->on_m_import_scaled_element_triggered(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ElementsLocation >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QETElementEditor::*)(ElementsLocation )>(_a, &QETElementEditor::saveToLocation, 0))
            return;
    }
}

const QMetaObject *QETElementEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QETElementEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16QETElementEditorE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int QETElementEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 34)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 34;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 34)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 34;
    }
    return _id;
}

// SIGNAL 0
void QETElementEditor::saveToLocation(ElementsLocation _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}
QT_WARNING_POP
