/****************************************************************************
** Meta object code from reading C++ file 'projectview.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/projectview.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'projectview.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11ProjectViewE_t {};
} // unnamed namespace

template <> constexpr inline auto ProjectView::qt_create_metaobjectdata<qt_meta_tag_ZN11ProjectViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ProjectView",
        "diagramAdded",
        "",
        "DiagramView*",
        "diagramRemoved",
        "diagramActivated",
        "projectClosed",
        "ProjectView*",
        "errorEncountered",
        "findElementRequired",
        "ElementsLocation",
        "removeDiagram",
        "diagram_view",
        "silent",
        "Diagram*",
        "diagram",
        "showDiagram",
        "editProjectProperties",
        "editCurrentDiagramProperties",
        "editDiagramProperties",
        "moveDiagramUp",
        "moveDiagramDown",
        "moveDiagramUpTop",
        "moveDiagramUpx10",
        "moveDiagramDownx10",
        "moveDiagramUpx100",
        "moveDiagramDownx100",
        "exportProject",
        "save",
        "QETResult",
        "saveAs",
        "doSave",
        "cleanProject",
        "updateWindowTitle",
        "updateTabTitle",
        "updateAllTabsTitle",
        "tabMoved",
        "tabChanged",
        "tabDoubleClicked",
        "setDisplayFallbackWidget",
        "adjustReadOnlyState"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'diagramAdded'
        QtMocHelpers::SignalData<void(DiagramView *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'diagramRemoved'
        QtMocHelpers::SignalData<void(DiagramView *)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'diagramActivated'
        QtMocHelpers::SignalData<void(DiagramView *)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'projectClosed'
        QtMocHelpers::SignalData<void(ProjectView *)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 2 },
        }}),
        // Signal 'errorEncountered'
        QtMocHelpers::SignalData<void(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Signal 'findElementRequired'
        QtMocHelpers::SignalData<void(const ElementsLocation &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'removeDiagram'
        QtMocHelpers::SlotData<void(DiagramView *, bool)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 12 }, { QMetaType::Bool, 13 },
        }}),
        // Slot 'removeDiagram'
        QtMocHelpers::SlotData<void(DiagramView *)>(11, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 3, 12 },
        }}),
        // Slot 'removeDiagram'
        QtMocHelpers::SlotData<void(Diagram *, bool)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 15 }, { QMetaType::Bool, 13 },
        }}),
        // Slot 'removeDiagram'
        QtMocHelpers::SlotData<void(Diagram *)>(11, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'showDiagram'
        QtMocHelpers::SlotData<void(DiagramView *)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'showDiagram'
        QtMocHelpers::SlotData<void(Diagram *)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'editProjectProperties'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editCurrentDiagramProperties'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editDiagramProperties'
        QtMocHelpers::SlotData<void(DiagramView *)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'editDiagramProperties'
        QtMocHelpers::SlotData<void(Diagram *)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'moveDiagramUp'
        QtMocHelpers::SlotData<void(DiagramView *)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'moveDiagramUp'
        QtMocHelpers::SlotData<void(Diagram *)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'moveDiagramDown'
        QtMocHelpers::SlotData<void(DiagramView *)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'moveDiagramDown'
        QtMocHelpers::SlotData<void(Diagram *)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'moveDiagramUpTop'
        QtMocHelpers::SlotData<void(DiagramView *)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'moveDiagramUpTop'
        QtMocHelpers::SlotData<void(Diagram *)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'moveDiagramUpx10'
        QtMocHelpers::SlotData<void(DiagramView *)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'moveDiagramUpx10'
        QtMocHelpers::SlotData<void(Diagram *)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'moveDiagramDownx10'
        QtMocHelpers::SlotData<void(DiagramView *)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'moveDiagramDownx10'
        QtMocHelpers::SlotData<void(Diagram *)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'moveDiagramUpx100'
        QtMocHelpers::SlotData<void(DiagramView *)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'moveDiagramUpx100'
        QtMocHelpers::SlotData<void(Diagram *)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'moveDiagramDownx100'
        QtMocHelpers::SlotData<void(DiagramView *)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'moveDiagramDownx100'
        QtMocHelpers::SlotData<void(Diagram *)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 2 },
        }}),
        // Slot 'exportProject'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'save'
        QtMocHelpers::SlotData<QETResult()>(28, 2, QMC::AccessPublic, 0x80000000 | 29),
        // Slot 'saveAs'
        QtMocHelpers::SlotData<QETResult()>(30, 2, QMC::AccessPublic, 0x80000000 | 29),
        // Slot 'doSave'
        QtMocHelpers::SlotData<QETResult()>(31, 2, QMC::AccessPublic, 0x80000000 | 29),
        // Slot 'cleanProject'
        QtMocHelpers::SlotData<int()>(32, 2, QMC::AccessPublic, QMetaType::Int),
        // Slot 'updateWindowTitle'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateTabTitle'
        QtMocHelpers::SlotData<void(DiagramView *)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'updateAllTabsTitle'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'tabMoved'
        QtMocHelpers::SlotData<void(int, int)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'tabChanged'
        QtMocHelpers::SlotData<void(int)>(37, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'tabDoubleClicked'
        QtMocHelpers::SlotData<void(int)>(38, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'setDisplayFallbackWidget'
        QtMocHelpers::SlotData<void(bool)>(39, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'adjustReadOnlyState'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'diagramAdded'
        QtMocHelpers::SlotData<void(Diagram *)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'diagramRemoved'
        QtMocHelpers::SlotData<void(Diagram *)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ProjectView, qt_meta_tag_ZN11ProjectViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ProjectView::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ProjectViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ProjectViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11ProjectViewE_t>.metaTypes,
    nullptr
} };

void ProjectView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ProjectView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->diagramAdded((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 1: _t->diagramRemoved((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 2: _t->diagramActivated((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 3: _t->projectClosed((*reinterpret_cast<std::add_pointer_t<ProjectView*>>(_a[1]))); break;
        case 4: _t->errorEncountered((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->findElementRequired((*reinterpret_cast<std::add_pointer_t<ElementsLocation>>(_a[1]))); break;
        case 6: _t->removeDiagram((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 7: _t->removeDiagram((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 8: _t->removeDiagram((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 9: _t->removeDiagram((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 10: _t->showDiagram((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 11: _t->showDiagram((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 12: _t->editProjectProperties(); break;
        case 13: _t->editCurrentDiagramProperties(); break;
        case 14: _t->editDiagramProperties((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 15: _t->editDiagramProperties((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 16: _t->moveDiagramUp((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 17: _t->moveDiagramUp((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 18: _t->moveDiagramDown((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 19: _t->moveDiagramDown((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 20: _t->moveDiagramUpTop((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 21: _t->moveDiagramUpTop((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 22: _t->moveDiagramUpx10((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 23: _t->moveDiagramUpx10((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 24: _t->moveDiagramDownx10((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 25: _t->moveDiagramDownx10((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 26: _t->moveDiagramUpx100((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 27: _t->moveDiagramUpx100((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 28: _t->moveDiagramDownx100((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 29: _t->moveDiagramDownx100((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 30: _t->exportProject(); break;
        case 31: { QETResult _r = _t->save();
            if (_a[0]) *reinterpret_cast<QETResult*>(_a[0]) = std::move(_r); }  break;
        case 32: { QETResult _r = _t->saveAs();
            if (_a[0]) *reinterpret_cast<QETResult*>(_a[0]) = std::move(_r); }  break;
        case 33: { QETResult _r = _t->doSave();
            if (_a[0]) *reinterpret_cast<QETResult*>(_a[0]) = std::move(_r); }  break;
        case 34: { int _r = _t->cleanProject();
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 35: _t->updateWindowTitle(); break;
        case 36: _t->updateTabTitle((*reinterpret_cast<std::add_pointer_t<DiagramView*>>(_a[1]))); break;
        case 37: _t->updateAllTabsTitle(); break;
        case 38: _t->tabMoved((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 39: _t->tabChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 40: _t->tabDoubleClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 41: _t->setDisplayFallbackWidget((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 42: _t->adjustReadOnlyState(); break;
        case 43: _t->diagramAdded((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 44: _t->diagramRemoved((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ProjectView* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ProjectView::*)(DiagramView * )>(_a, &ProjectView::diagramAdded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectView::*)(DiagramView * )>(_a, &ProjectView::diagramRemoved, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectView::*)(DiagramView * )>(_a, &ProjectView::diagramActivated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectView::*)(ProjectView * )>(_a, &ProjectView::projectClosed, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectView::*)(const QString & )>(_a, &ProjectView::errorEncountered, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectView::*)(const ElementsLocation & )>(_a, &ProjectView::findElementRequired, 5))
            return;
    }
}

const QMetaObject *ProjectView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProjectView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ProjectViewE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ProjectView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 45)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 45;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 45)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 45;
    }
    return _id;
}

// SIGNAL 0
void ProjectView::diagramAdded(DiagramView * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ProjectView::diagramRemoved(DiagramView * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ProjectView::diagramActivated(DiagramView * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ProjectView::projectClosed(ProjectView * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ProjectView::errorEncountered(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void ProjectView::findElementRequired(const ElementsLocation & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}
QT_WARNING_POP
