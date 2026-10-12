/****************************************************************************
** Meta object code from reading C++ file 'elementspanelwidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/elementspanelwidget.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementspanelwidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19ElementsPanelWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementsPanelWidget::qt_create_metaobjectdata<qt_meta_tag_ZN19ElementsPanelWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementsPanelWidget",
        "requestForProject",
        "",
        "QETProject*",
        "requestForNewDiagram",
        "requestForNewDiagramAt",
        "requestForProjectClosing",
        "requestForProjectPropertiesEdition",
        "requestForDiagramPropertiesEdition",
        "Diagram*",
        "requestForDiagramDeletion",
        "requestForDiagramsDeletion",
        "QList<Diagram*>",
        "diagrams",
        "requestForDiagramMoveUp",
        "requestForDiagramMoveDown",
        "requestForDiagramMoveUpTop",
        "requestForDiagramMoveUpx10",
        "requestForDiagramMoveUpx100",
        "requestForDiagramMoveDownx10",
        "requestForDiagramMoveDownx100",
        "openDirectoryForSelectedItem",
        "copyPathForSelectedItem",
        "reloadAndFilter",
        "activateProject",
        "closeProject",
        "editProjectProperties",
        "editDiagramProperties",
        "newDiagram",
        "insertDiagramAbove",
        "insertDiagramBelow",
        "deleteDiagram",
        "duplicateDiagram",
        "moveDiagramUp",
        "moveDiagramDown",
        "moveDiagramUpTop",
        "moveDiagramUpx10",
        "moveDiagramUpx100",
        "moveDiagramDownx10",
        "moveDiagramDownx100",
        "addTitleBlockTemplate",
        "editTitleBlockTemplate",
        "removeTitleBlockTemplate",
        "updateButtons",
        "handleContextMenu",
        "QPoint",
        "filterEdited"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'requestForProject'
        QtMocHelpers::SignalData<void(QETProject *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'requestForNewDiagram'
        QtMocHelpers::SignalData<void(QETProject *)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'requestForNewDiagramAt'
        QtMocHelpers::SignalData<void(QETProject *, int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::Int, 2 },
        }}),
        // Signal 'requestForProjectClosing'
        QtMocHelpers::SignalData<void(QETProject *)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'requestForProjectPropertiesEdition'
        QtMocHelpers::SignalData<void(QETProject *)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'requestForDiagramPropertiesEdition'
        QtMocHelpers::SignalData<void(Diagram *)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Signal 'requestForDiagramDeletion'
        QtMocHelpers::SignalData<void(Diagram *)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Signal 'requestForDiagramsDeletion'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'requestForDiagramMoveUp'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'requestForDiagramMoveDown'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'requestForDiagramMoveUpTop'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'requestForDiagramMoveUpx10'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'requestForDiagramMoveUpx100'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'requestForDiagramMoveDownx10'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'requestForDiagramMoveDownx100'
        QtMocHelpers::SignalData<void(const QList<Diagram*> &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'openDirectoryForSelectedItem'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'copyPathForSelectedItem'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'reloadAndFilter'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'activateProject'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'closeProject'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editProjectProperties'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editDiagramProperties'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'newDiagram'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'insertDiagramAbove'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'insertDiagramBelow'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'deleteDiagram'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'duplicateDiagram'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramUp'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramDown'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramUpTop'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramUpx10'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramUpx100'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramDownx10'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'moveDiagramDownx100'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addTitleBlockTemplate'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editTitleBlockTemplate'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'removeTitleBlockTemplate'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateButtons'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(44, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 45, 2 },
        }}),
        // Slot 'filterEdited'
        QtMocHelpers::SlotData<void(const QString &)>(46, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ElementsPanelWidget, qt_meta_tag_ZN19ElementsPanelWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementsPanelWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ElementsPanelWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ElementsPanelWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN19ElementsPanelWidgetE_t>.metaTypes,
    nullptr
} };

void ElementsPanelWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ElementsPanelWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->requestForProject((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 1: _t->requestForNewDiagram((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 2: _t->requestForNewDiagramAt((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->requestForProjectClosing((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 4: _t->requestForProjectPropertiesEdition((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 5: _t->requestForDiagramPropertiesEdition((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 6: _t->requestForDiagramDeletion((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 7: _t->requestForDiagramsDeletion((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 8: _t->requestForDiagramMoveUp((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 9: _t->requestForDiagramMoveDown((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 10: _t->requestForDiagramMoveUpTop((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 11: _t->requestForDiagramMoveUpx10((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 12: _t->requestForDiagramMoveUpx100((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 13: _t->requestForDiagramMoveDownx10((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 14: _t->requestForDiagramMoveDownx100((*reinterpret_cast<std::add_pointer_t<QList<Diagram*>>>(_a[1]))); break;
        case 15: _t->openDirectoryForSelectedItem(); break;
        case 16: _t->copyPathForSelectedItem(); break;
        case 17: _t->reloadAndFilter(); break;
        case 18: _t->activateProject(); break;
        case 19: _t->closeProject(); break;
        case 20: _t->editProjectProperties(); break;
        case 21: _t->editDiagramProperties(); break;
        case 22: _t->newDiagram(); break;
        case 23: _t->insertDiagramAbove(); break;
        case 24: _t->insertDiagramBelow(); break;
        case 25: _t->deleteDiagram(); break;
        case 26: _t->duplicateDiagram(); break;
        case 27: _t->moveDiagramUp(); break;
        case 28: _t->moveDiagramDown(); break;
        case 29: _t->moveDiagramUpTop(); break;
        case 30: _t->moveDiagramUpx10(); break;
        case 31: _t->moveDiagramUpx100(); break;
        case 32: _t->moveDiagramDownx10(); break;
        case 33: _t->moveDiagramDownx100(); break;
        case 34: _t->addTitleBlockTemplate(); break;
        case 35: _t->editTitleBlockTemplate(); break;
        case 36: _t->removeTitleBlockTemplate(); break;
        case 37: _t->updateButtons(); break;
        case 38: _t->handleContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 39: _t->filterEdited((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(QETProject * )>(_a, &ElementsPanelWidget::requestForProject, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(QETProject * )>(_a, &ElementsPanelWidget::requestForNewDiagram, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(QETProject * , int )>(_a, &ElementsPanelWidget::requestForNewDiagramAt, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(QETProject * )>(_a, &ElementsPanelWidget::requestForProjectClosing, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(QETProject * )>(_a, &ElementsPanelWidget::requestForProjectPropertiesEdition, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(Diagram * )>(_a, &ElementsPanelWidget::requestForDiagramPropertiesEdition, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(Diagram * )>(_a, &ElementsPanelWidget::requestForDiagramDeletion, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramsDeletion, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramMoveUp, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramMoveDown, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramMoveUpTop, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramMoveUpx10, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramMoveUpx100, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramMoveDownx10, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanelWidget::*)(const QList<Diagram*> & )>(_a, &ElementsPanelWidget::requestForDiagramMoveDownx100, 14))
            return;
    }
}

const QMetaObject *ElementsPanelWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElementsPanelWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ElementsPanelWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ElementsPanelWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 40)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 40;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 40)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 40;
    }
    return _id;
}

// SIGNAL 0
void ElementsPanelWidget::requestForProject(QETProject * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ElementsPanelWidget::requestForNewDiagram(QETProject * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ElementsPanelWidget::requestForNewDiagramAt(QETProject * _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void ElementsPanelWidget::requestForProjectClosing(QETProject * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ElementsPanelWidget::requestForProjectPropertiesEdition(QETProject * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void ElementsPanelWidget::requestForDiagramPropertiesEdition(Diagram * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void ElementsPanelWidget::requestForDiagramDeletion(Diagram * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void ElementsPanelWidget::requestForDiagramsDeletion(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void ElementsPanelWidget::requestForDiagramMoveUp(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void ElementsPanelWidget::requestForDiagramMoveDown(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void ElementsPanelWidget::requestForDiagramMoveUpTop(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void ElementsPanelWidget::requestForDiagramMoveUpx10(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void ElementsPanelWidget::requestForDiagramMoveUpx100(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1);
}

// SIGNAL 13
void ElementsPanelWidget::requestForDiagramMoveDownx10(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1);
}

// SIGNAL 14
void ElementsPanelWidget::requestForDiagramMoveDownx100(const QList<Diagram*> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 14, nullptr, _t1);
}
QT_WARNING_POP
