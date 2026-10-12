/****************************************************************************
** Meta object code from reading C++ file 'qettemplateeditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/titleblock/qettemplateeditor.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qettemplateeditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN27QETTitleBlockTemplateEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto QETTitleBlockTemplateEditor::qt_create_metaobjectdata<qt_meta_tag_ZN27QETTitleBlockTemplateEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QETTitleBlockTemplateEditor",
        "readSettings",
        "",
        "readSettingsState",
        "writeSettings",
        "selectedCellsChanged",
        "QList<TitleBlockCell*>",
        "duplicateCurrentLocation",
        "edit",
        "TitleBlockTemplateLocation",
        "QETProject*",
        "editCopyOf",
        "const TitleBlockTemplate*",
        "TitleBlockTemplate*",
        "editLogos",
        "newTemplate",
        "open",
        "openFromFile",
        "save",
        "saveAs",
        "saveAsFile",
        "setReadOnly",
        "quit",
        "savePreviewWidthToApplicationSettings",
        "editTemplateInformation",
        "getTitleBlockTemplateLocationFromUser",
        "existing_only",
        "pushCellUndoCommand",
        "ModifyTitleBlockCellCommand*",
        "pushGridUndoCommand",
        "TitleBlockTemplateCommand*",
        "pushUndoCommand",
        "QUndoCommand*",
        "updateEditorTitle",
        "updateActions"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'readSettings'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'readSettingsState'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'writeSettings'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectedCellsChanged'
        QtMocHelpers::SlotData<void(const QList<TitleBlockCell*> &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 2 },
        }}),
        // Slot 'duplicateCurrentLocation'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'edit'
        QtMocHelpers::SlotData<bool(const TitleBlockTemplateLocation &)>(8, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Slot 'edit'
        QtMocHelpers::SlotData<bool(QETProject *, const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 10, 2 }, { QMetaType::QString, 2 },
        }}),
        // Slot 'edit'
        QtMocHelpers::SlotData<bool(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'editCopyOf'
        QtMocHelpers::SlotData<bool(const TitleBlockTemplate *)>(11, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 12, 2 },
        }}),
        // Slot 'edit'
        QtMocHelpers::SlotData<bool(TitleBlockTemplate *)>(8, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 13, 2 },
        }}),
        // Slot 'editLogos'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'newTemplate'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'open'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'openFromFile'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'save'
        QtMocHelpers::SlotData<bool()>(18, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'saveAs'
        QtMocHelpers::SlotData<bool()>(19, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'saveAsFile'
        QtMocHelpers::SlotData<bool()>(20, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'setReadOnly'
        QtMocHelpers::SlotData<void(bool)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'quit'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'savePreviewWidthToApplicationSettings'
        QtMocHelpers::SlotData<void(int, int)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'editTemplateInformation'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'getTitleBlockTemplateLocationFromUser'
        QtMocHelpers::SlotData<TitleBlockTemplateLocation(const QString &, bool)>(25, 2, QMC::AccessPrivate, 0x80000000 | 9, {{
            { QMetaType::QString, 2 }, { QMetaType::Bool, 26 },
        }}),
        // Slot 'getTitleBlockTemplateLocationFromUser'
        QtMocHelpers::SlotData<TitleBlockTemplateLocation(const QString &)>(25, 2, QMC::AccessPrivate | QMC::MethodCloned, 0x80000000 | 9, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'getTitleBlockTemplateLocationFromUser'
        QtMocHelpers::SlotData<TitleBlockTemplateLocation()>(25, 2, QMC::AccessPrivate | QMC::MethodCloned, 0x80000000 | 9),
        // Slot 'pushCellUndoCommand'
        QtMocHelpers::SlotData<void(ModifyTitleBlockCellCommand *)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 28, 2 },
        }}),
        // Slot 'pushGridUndoCommand'
        QtMocHelpers::SlotData<void(TitleBlockTemplateCommand *)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 30, 2 },
        }}),
        // Slot 'pushUndoCommand'
        QtMocHelpers::SlotData<void(QUndoCommand *)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 32, 2 },
        }}),
        // Slot 'updateEditorTitle'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateActions'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'saveAs'
        QtMocHelpers::SlotData<bool(const TitleBlockTemplateLocation &)>(19, 2, QMC::AccessPrivate, QMetaType::Bool, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Slot 'saveAs'
        QtMocHelpers::SlotData<bool(const QString &)>(19, 2, QMC::AccessPrivate, QMetaType::Bool, {{
            { QMetaType::QString, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QETTitleBlockTemplateEditor, qt_meta_tag_ZN27QETTitleBlockTemplateEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QETTitleBlockTemplateEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QETMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN27QETTitleBlockTemplateEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN27QETTitleBlockTemplateEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN27QETTitleBlockTemplateEditorE_t>.metaTypes,
    nullptr
} };

void QETTitleBlockTemplateEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QETTitleBlockTemplateEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->readSettings(); break;
        case 1: _t->readSettingsState(); break;
        case 2: _t->writeSettings(); break;
        case 3: _t->selectedCellsChanged((*reinterpret_cast<std::add_pointer_t<QList<TitleBlockCell*>>>(_a[1]))); break;
        case 4: _t->duplicateCurrentLocation(); break;
        case 5: { bool _r = _t->edit((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplateLocation>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->edit((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->edit((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->editCopyOf((*reinterpret_cast<std::add_pointer_t<const TitleBlockTemplate*>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->edit((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplate*>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 10: _t->editLogos(); break;
        case 11: _t->newTemplate(); break;
        case 12: _t->open(); break;
        case 13: _t->openFromFile(); break;
        case 14: { bool _r = _t->save();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 15: { bool _r = _t->saveAs();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 16: { bool _r = _t->saveAsFile();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->setReadOnly((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 18: _t->quit(); break;
        case 19: _t->savePreviewWidthToApplicationSettings((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 20: _t->editTemplateInformation(); break;
        case 21: { TitleBlockTemplateLocation _r = _t->getTitleBlockTemplateLocationFromUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])));
            if (_a[0]) *reinterpret_cast<TitleBlockTemplateLocation*>(_a[0]) = std::move(_r); }  break;
        case 22: { TitleBlockTemplateLocation _r = _t->getTitleBlockTemplateLocationFromUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<TitleBlockTemplateLocation*>(_a[0]) = std::move(_r); }  break;
        case 23: { TitleBlockTemplateLocation _r = _t->getTitleBlockTemplateLocationFromUser();
            if (_a[0]) *reinterpret_cast<TitleBlockTemplateLocation*>(_a[0]) = std::move(_r); }  break;
        case 24: _t->pushCellUndoCommand((*reinterpret_cast<std::add_pointer_t<ModifyTitleBlockCellCommand*>>(_a[1]))); break;
        case 25: _t->pushGridUndoCommand((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplateCommand*>>(_a[1]))); break;
        case 26: _t->pushUndoCommand((*reinterpret_cast<std::add_pointer_t<QUndoCommand*>>(_a[1]))); break;
        case 27: _t->updateEditorTitle(); break;
        case 28: _t->updateActions(); break;
        case 29: { bool _r = _t->saveAs((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplateLocation>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 30: { bool _r = _t->saveAs((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplateLocation >(); break;
            }
            break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplate* >(); break;
            }
            break;
        case 29:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplateLocation >(); break;
            }
            break;
        }
    }
}

const QMetaObject *QETTitleBlockTemplateEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QETTitleBlockTemplateEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN27QETTitleBlockTemplateEditorE_t>.strings))
        return static_cast<void*>(this);
    return QETMainWindow::qt_metacast(_clname);
}

int QETTitleBlockTemplateEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QETMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 31)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 31;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 31)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 31;
    }
    return _id;
}
QT_WARNING_POP
