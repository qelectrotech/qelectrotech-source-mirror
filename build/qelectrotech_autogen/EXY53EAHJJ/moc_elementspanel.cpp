/****************************************************************************
** Meta object code from reading C++ file 'elementspanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/elementspanel.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementspanel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13ElementsPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementsPanel::qt_create_metaobjectdata<qt_meta_tag_ZN13ElementsPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementsPanel",
        "requestForProject",
        "",
        "QETProject*",
        "requestForTitleBlockTemplate",
        "TitleBlockTemplateLocation",
        "requestForProjectPropertiesEdition",
        "requestForDiagramPropertiesEdition",
        "slot_clicked",
        "QTreeWidgetItem*",
        "slot_doubleClick",
        "reload",
        "filter",
        "QET::Filtering",
        "projectWasOpened",
        "projectWasClosed",
        "buildFilterList",
        "applyCurrentFilter",
        "QList<QTreeWidgetItem*>",
        "ensureHierarchyIsVisible",
        "requestForItem",
        "keyPressEvent",
        "QKeyEvent*",
        "event",
        "panelContentChange"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'requestForProject'
        QtMocHelpers::SignalData<void(QETProject *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'requestForTitleBlockTemplate'
        QtMocHelpers::SignalData<void(const TitleBlockTemplateLocation &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Signal 'requestForProjectPropertiesEdition'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'requestForDiagramPropertiesEdition'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'slot_clicked'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *, int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'slot_doubleClick'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *, int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'reload'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'filter'
        QtMocHelpers::SlotData<void(const QString &, QET::Filtering)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 }, { 0x80000000 | 13, 2 },
        }}),
        // Slot 'filter'
        QtMocHelpers::SlotData<void(const QString &)>(12, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'projectWasOpened'
        QtMocHelpers::SlotData<void(QETProject *)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'projectWasClosed'
        QtMocHelpers::SlotData<void(QETProject *)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'buildFilterList'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'applyCurrentFilter'
        QtMocHelpers::SlotData<void(const QList<QTreeWidgetItem*> &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 2 },
        }}),
        // Slot 'ensureHierarchyIsVisible'
        QtMocHelpers::SlotData<void(const QList<QTreeWidgetItem*> &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 2 },
        }}),
        // Slot 'requestForItem'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 2 },
        }}),
        // Slot 'keyPressEvent'
        QtMocHelpers::SlotData<void(QKeyEvent *)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 22, 23 },
        }}),
        // Slot 'panelContentChange'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessProtected, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ElementsPanel, qt_meta_tag_ZN13ElementsPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementsPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<GenericPanel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13ElementsPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13ElementsPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13ElementsPanelE_t>.metaTypes,
    nullptr
} };

void ElementsPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ElementsPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->requestForProject((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 1: _t->requestForTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplateLocation>>(_a[1]))); break;
        case 2: _t->requestForProjectPropertiesEdition(); break;
        case 3: _t->requestForDiagramPropertiesEdition(); break;
        case 4: _t->slot_clicked((*reinterpret_cast<std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->slot_doubleClick((*reinterpret_cast<std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->reload(); break;
        case 7: _t->filter((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QET::Filtering>>(_a[2]))); break;
        case 8: _t->filter((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->projectWasOpened((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 10: _t->projectWasClosed((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 11: _t->buildFilterList(); break;
        case 12: _t->applyCurrentFilter((*reinterpret_cast<std::add_pointer_t<QList<QTreeWidgetItem*>>>(_a[1]))); break;
        case 13: _t->ensureHierarchyIsVisible((*reinterpret_cast<std::add_pointer_t<QList<QTreeWidgetItem*>>>(_a[1]))); break;
        case 14: _t->requestForItem((*reinterpret_cast<std::add_pointer_t<QTreeWidgetItem*>>(_a[1]))); break;
        case 15: _t->keyPressEvent((*reinterpret_cast<std::add_pointer_t<QKeyEvent*>>(_a[1]))); break;
        case 16: _t->panelContentChange(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplateLocation >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ElementsPanel::*)(QETProject * )>(_a, &ElementsPanel::requestForProject, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanel::*)(const TitleBlockTemplateLocation & )>(_a, &ElementsPanel::requestForTitleBlockTemplate, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanel::*)()>(_a, &ElementsPanel::requestForProjectPropertiesEdition, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ElementsPanel::*)()>(_a, &ElementsPanel::requestForDiagramPropertiesEdition, 3))
            return;
    }
}

const QMetaObject *ElementsPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElementsPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13ElementsPanelE_t>.strings))
        return static_cast<void*>(this);
    return GenericPanel::qt_metacast(_clname);
}

int ElementsPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = GenericPanel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void ElementsPanel::requestForProject(QETProject * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ElementsPanel::requestForTitleBlockTemplate(const TitleBlockTemplateLocation & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ElementsPanel::requestForProjectPropertiesEdition()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ElementsPanel::requestForDiagramPropertiesEdition()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
