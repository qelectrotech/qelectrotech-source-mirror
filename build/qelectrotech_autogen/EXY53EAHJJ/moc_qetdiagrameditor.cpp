/****************************************************************************
** Meta object code from reading C++ file 'qetdiagrameditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetdiagrameditor.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qetdiagrameditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16QETDiagramEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto QETDiagramEditor::qt_create_metaobjectdata<qt_meta_tag_ZN16QETDiagramEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QETDiagramEditor",
        "syncElementsPanel",
        "",
        "save",
        "saveAs",
        "newProject",
        "openProject",
        "openRecentFile",
        "closeProject",
        "ProjectView*",
        "QETProject*",
        "zoomGroupTriggered",
        "QAction*",
        "action",
        "selectGroupTriggered",
        "addItemGroupTriggered",
        "selectionGroupTriggered",
        "rowColumnGroupTriggered",
        "slot_updateActions",
        "slot_updateUndoStack",
        "slot_updateModeActions",
        "slot_updateComplexActions",
        "slot_updatePasteAction",
        "slot_updateWindowsMenu",
        "slot_updateAutoNumDock",
        "generateTerminalBlock",
        "setWindowedMode",
        "setTabbedMode",
        "readSettings",
        "readSettingsState",
        "writeSettings",
        "activateProject",
        "activateWidget",
        "QWidget*",
        "projectWasClosed",
        "editProjectProperties",
        "slot_terminalNumbering",
        "editDiagramProperties",
        "DiagramView*",
        "Diagram*",
        "addDiagramToProject",
        "addDiagramToProjectAt",
        "removeDiagram",
        "removeDiagrams",
        "QList<Diagram*>",
        "diagrams",
        "removeDiagramFromProject",
        "moveDiagramUp",
        "moveDiagramDown",
        "moveDiagramUpTop",
        "moveDiagramUpx10",
        "moveDiagramDownx10",
        "moveDiagramUpx100",
        "moveDiagramDownx100",
        "reloadOldElementPanel",
        "diagramWasAdded",
        "findElementInPanel",
        "ElementsLocation",
        "showError",
        "QETResult",
        "subWindowActivated",
        "QMdiSubWindow*",
        "subWindows",
        "copyTextFormat",
        "applyTextFormat",
        "selectionChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'syncElementsPanel'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'save'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'saveAs'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'newProject'
        QtMocHelpers::SlotData<bool()>(5, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'openProject'
        QtMocHelpers::SlotData<bool()>(6, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'openRecentFile'
        QtMocHelpers::SlotData<bool(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'closeProject'
        QtMocHelpers::SlotData<bool(ProjectView *)>(8, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Slot 'closeProject'
        QtMocHelpers::SlotData<bool(QETProject *)>(8, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'zoomGroupTriggered'
        QtMocHelpers::SlotData<void(QAction *)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'selectGroupTriggered'
        QtMocHelpers::SlotData<void(QAction *)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'addItemGroupTriggered'
        QtMocHelpers::SlotData<void(QAction *)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'selectionGroupTriggered'
        QtMocHelpers::SlotData<void(QAction *)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'rowColumnGroupTriggered'
        QtMocHelpers::SlotData<void(QAction *)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'slot_updateActions'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_updateUndoStack'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_updateModeActions'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_updateComplexActions'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_updatePasteAction'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_updateWindowsMenu'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_updateAutoNumDock'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'generateTerminalBlock'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setWindowedMode'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setTabbedMode'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'readSettings'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'readSettingsState'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'writeSettings'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'activateProject'
        QtMocHelpers::SlotData<void(QETProject *)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'activateProject'
        QtMocHelpers::SlotData<void(ProjectView *)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Slot 'activateWidget'
        QtMocHelpers::SlotData<void(QWidget *)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 33, 2 },
        }}),
        // Slot 'projectWasClosed'
        QtMocHelpers::SlotData<void(ProjectView *)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Slot 'editProjectProperties'
        QtMocHelpers::SlotData<void(ProjectView *)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Slot 'editProjectProperties'
        QtMocHelpers::SlotData<void(QETProject *)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'slot_terminalNumbering'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editDiagramProperties'
        QtMocHelpers::SlotData<void(DiagramView *)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 38, 2 },
        }}),
        // Slot 'editDiagramProperties'
        QtMocHelpers::SlotData<void(Diagram *)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 39, 2 },
        }}),
        // Slot 'addDiagramToProject'
        QtMocHelpers::SlotData<void(QETProject *)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'addDiagramToProjectAt'
        QtMocHelpers::SlotData<void(QETProject *, int)>(41, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'removeDiagram'
        QtMocHelpers::SlotData<void(Diagram *)>(42, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 39, 2 },
        }}),
        // Slot 'removeDiagrams'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(43, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'removeDiagramFromProject'
        QtMocHelpers::SlotData<void()>(46, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramUp'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(47, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'moveDiagramDown'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'moveDiagramUpTop'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(49, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'moveDiagramUpx10'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'moveDiagramDownx10'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(51, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'moveDiagramUpx100'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(52, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'moveDiagramDownx100'
        QtMocHelpers::SlotData<void(const QList<Diagram*> &)>(53, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'reloadOldElementPanel'
        QtMocHelpers::SlotData<void()>(54, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'diagramWasAdded'
        QtMocHelpers::SlotData<void(DiagramView *)>(55, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 38, 2 },
        }}),
        // Slot 'findElementInPanel'
        QtMocHelpers::SlotData<void(const ElementsLocation &)>(56, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 57, 2 },
        }}),
        // Slot 'showError'
        QtMocHelpers::SlotData<void(const QETResult &)>(58, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 59, 2 },
        }}),
        // Slot 'showError'
        QtMocHelpers::SlotData<void(const QString &)>(58, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'subWindowActivated'
        QtMocHelpers::SlotData<void(QMdiSubWindow *)>(60, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 61, 62 },
        }}),
        // Slot 'copyTextFormat'
        QtMocHelpers::SlotData<void()>(63, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'applyTextFormat'
        QtMocHelpers::SlotData<void()>(64, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectionChanged'
        QtMocHelpers::SlotData<void()>(65, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QETDiagramEditor, qt_meta_tag_ZN16QETDiagramEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QETDiagramEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QETMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16QETDiagramEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16QETDiagramEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16QETDiagramEditorE_t>.metaTypes,
    nullptr
} };

void QETDiagramEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QETDiagramEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->syncElementsPanel(); break;
        case 1: _t->save(); break;
        case 2: _t->saveAs(); break;
        case 3: { bool _r = _t->newProject();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 4: { bool _r = _t->openProject();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 5: { bool _r = _t->openRecentFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->closeProject((*reinterpret_cast<std::add_pointer_t<ProjectView*>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->closeProject((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->zoomGroupTriggered((*reinterpret_cast<std::add_pointer_t<QAction*>>(_a[1]))); break;
        case 9: _t->selectGroupTriggered((*reinterpret_cast<std::add_pointer_t<QAction*>>(_a[1]))); break;
        case 10: _t->addItemGroupTriggered((*reinterpret_cast<std::add_pointer_t<QAction*>>(_a[1]))); break;
        case 11: _t->selectionGroupTriggered((*reinterpret_cast<std::add_pointer_t<QAction*>>(_a[1]))); break;
        case 12: _t->rowColumnGroupTriggered((*reinterpret_cast<std::add_pointer_t<QAction*>>(_a[1]))); break;
        case 13: _t->slot_updateActions(); break;
        case 14: _t->slot_updateUndoStack(); break;
        case 15: _t->slot_updateModeActions(); break;
        case 16: _t->slot_updateComplexActions(); break;
        case 17: _t->slot_updatePasteAction(); break;
        case 18: _t->slot_updateWindowsMenu(); break;
        case 19: _t->slot_updateAutoNumDock(); break;
        case 20: _t->generateTerminalBlock(); break;
        case 21: _t->setWindowedMode(); break;
        case 22: _t->setTabbedMode(); break;
        case 23: _t->readSettings(); break;
        case 24: _t->readSettingsState(); break;
        case 25: _t->writeSettings(); break;
        case 26: _t->activateProject((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 27: _t->activateProject((*reinterpret_cast<std::add_pointer_t<ProjectView*>>(_a[1]))); break;
        case 28: _t->activateWidget((*reinterpret_cast<std::add_pointer_t<QWidget*>>(_a[1]))); break;
        case 29: _t->projectWasClosed((*reinterpret_cast<std::add_pointer_t<ProjectView*>>(_a[1]))); break;
        case 30: _t->editProjectProperties((*reinterpret_cast<std::add_pointer_t<ProjectView*>>(_a[1]))); break;
        case 31: _t->editProjectProperties((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 32: _t->slot_terminalNumbering(); break;
        case 33: _t->editDiagramProperties((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 34: _t->editDiagramProperties((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 35: _t->addDiagramToProject((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 36: _t->addDiagramToProjectAt((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 37: _t->removeDiagram((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 38: _t->removeDiagrams((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 39: _t->removeDiagramFromProject(); break;
        case 40: _t->moveDiagramUp((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 41: _t->moveDiagramDown((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 42: _t->moveDiagramUpTop((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 43: _t->moveDiagramUpx10((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 44: _t->moveDiagramDownx10((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 45: _t->moveDiagramUpx100((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 46: _t->moveDiagramDownx100((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 47: _t->reloadOldElementPanel(); break;
        case 48: _t->diagramWasAdded((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 49: _t->findElementInPanel((*reinterpret_cast<std::add_pointer_t<ElementsLocation>>(_a[1]))); break;
        case 50: _t->showError((*reinterpret_cast<std::add_pointer_t<QETResult>>(_a[1]))); break;
        case 51: _t->showError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 52: _t->subWindowActivated((*reinterpret_cast<std::add_pointer_t<QMdiSubWindow*>>(_a[1]))); break;
        case 53: _t->copyTextFormat(); break;
        case 54: _t->applyTextFormat(); break;
        case 55: _t->selectionChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        case 28:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QWidget* >(); break;
            }
            break;
        case 49:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ElementsLocation >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QETDiagramEditor::*)()>(_a, &QETDiagramEditor::syncElementsPanel, 0))
            return;
    }
}

const QMetaObject *QETDiagramEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QETDiagramEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16QETDiagramEditorE_t>.strings))
        return static_cast<void*>(this);
    return QETMainWindow::qt_metacast(_clname);
}

int QETDiagramEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QETMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 56)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 56;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 56)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 56;
    }
    return _id;
}

// SIGNAL 0
void QETDiagramEditor::syncElementsPanel()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
