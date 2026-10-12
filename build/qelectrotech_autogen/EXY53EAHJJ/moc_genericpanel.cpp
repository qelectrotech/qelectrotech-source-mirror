/****************************************************************************
** Meta object code from reading C++ file 'genericpanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/genericpanel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'genericpanel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12GenericPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto GenericPanel::qt_create_metaobjectdata<qt_meta_tag_ZN12GenericPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GenericPanel",
        "firstActivated",
        "",
        "panelContentChanged",
        "projectInformationsChanged",
        "QETProject*",
        "diagramAdded",
        "Diagram*",
        "diagramRemoved",
        "projectDiagramsOrderChanged",
        "diagramTitleChanged",
        "templatesCollectionChanged",
        "TitleBlockTemplatesCollection*",
        "diagramUsedTemplate",
        "emitFirstActivated"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'firstActivated'
        QtMocHelpers::SignalData<bool()>(1, 2, QMC::AccessPublic, QMetaType::Bool),
        // Signal 'panelContentChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'projectInformationsChanged'
        QtMocHelpers::SlotData<void(QETProject *)>(4, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Slot 'diagramAdded'
        QtMocHelpers::SlotData<void(QETProject *, Diagram *)>(6, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 5, 2 }, { 0x80000000 | 7, 2 },
        }}),
        // Slot 'diagramRemoved'
        QtMocHelpers::SlotData<void(QETProject *, Diagram *)>(8, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 5, 2 }, { 0x80000000 | 7, 2 },
        }}),
        // Slot 'projectDiagramsOrderChanged'
        QtMocHelpers::SlotData<void(QETProject *, int, int)>(9, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 5, 2 }, { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'diagramTitleChanged'
        QtMocHelpers::SlotData<void(Diagram *)>(10, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 7, 2 },
        }}),
        // Slot 'templatesCollectionChanged'
        QtMocHelpers::SlotData<void(TitleBlockTemplatesCollection *, const QString &)>(11, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 12, 2 }, { QMetaType::QString, 2 },
        }}),
        // Slot 'diagramUsedTemplate'
        QtMocHelpers::SlotData<void(TitleBlockTemplatesCollection *, const QString &)>(13, 2, QMC::AccessProtected, QMetaType::Void, {{
            { 0x80000000 | 12, 2 }, { QMetaType::QString, 2 },
        }}),
        // Slot 'emitFirstActivated'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GenericPanel, qt_meta_tag_ZN12GenericPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GenericPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QTreeWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12GenericPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12GenericPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12GenericPanelE_t>.metaTypes,
    nullptr
} };

void GenericPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GenericPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: { bool _r = _t->firstActivated();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 1: _t->panelContentChanged(); break;
        case 2: _t->projectInformationsChanged((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 3: _t->diagramAdded((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[2]))); break;
        case 4: _t->diagramRemoved((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[2]))); break;
        case 5: _t->projectDiagramsOrderChanged((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 6: _t->diagramTitleChanged((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 7: _t->templatesCollectionChanged((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->diagramUsedTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 9: _t->emitFirstActivated(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<bool (GenericPanel::*)()>(_a, &GenericPanel::firstActivated, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GenericPanel::*)()>(_a, &GenericPanel::panelContentChanged, 1))
            return;
    }
}

const QMetaObject *GenericPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GenericPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12GenericPanelE_t>.strings))
        return static_cast<void*>(this);
    return QTreeWidget::qt_metacast(_clname);
}

int GenericPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
bool GenericPanel::firstActivated()
{
    bool _t0{};
    QMetaObject::activate<bool>(this, &staticMetaObject, 0, std::addressof(_t0));
    return _t0;
}

// SIGNAL 1
void GenericPanel::panelContentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
