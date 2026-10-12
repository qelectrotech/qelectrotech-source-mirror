/****************************************************************************
** Meta object code from reading C++ file 'qetproject.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetproject.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qetproject.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10QETProjectE_t {};
} // unnamed namespace

template <> constexpr inline auto QETProject::qt_create_metaobjectdata<qt_meta_tag_ZN10QETProjectE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QETProject",
        "projectFilePathChanged",
        "",
        "QETProject*",
        "projectTitleChanged",
        "projectInformationsChanged",
        "diagramAdded",
        "Diagram*",
        "diagramRemoved",
        "projectModified",
        "projectDiagramsOrderChanged",
        "diagramUsedTemplate",
        "TitleBlockTemplatesCollection*",
        "readOnlyChanged",
        "reportPropertiesChanged",
        "old_str",
        "new_str",
        "XRefPropertiesChanged",
        "addAutoNumDiagram",
        "elementAutoNumAdded",
        "name",
        "elementAutoNumRemoved",
        "conductorAutoNumAdded",
        "conductorAutoNumRemoved",
        "folioAutoNumAdded",
        "autoNumContextUpdated",
        "folioAutoNumRemoved",
        "defaultTitleBlockPropertiesChanged",
        "conductorAutoNumChanged",
        "addNewDiagram",
        "pos",
        "removeDiagram",
        "diagramOrderChanged",
        "setModified",
        "updateDiagramsFolioData",
        "updateDiagramsTitleBlockTemplate",
        "removeDiagramsTitleBlockTemplate",
        "usedTitleBlockTemplateChanged",
        "undoStackChanged",
        "a",
        "autoConductor",
        "autoBreakConductor"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'projectFilePathChanged'
        QtMocHelpers::SignalData<void(QETProject *, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::QString, 2 },
        }}),
        // Signal 'projectTitleChanged'
        QtMocHelpers::SignalData<void(QETProject *, const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::QString, 2 },
        }}),
        // Signal 'projectInformationsChanged'
        QtMocHelpers::SignalData<void(QETProject *)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'diagramAdded'
        QtMocHelpers::SignalData<void(QETProject *, Diagram *)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { 0x80000000 | 7, 2 },
        }}),
        // Signal 'diagramRemoved'
        QtMocHelpers::SignalData<void(QETProject *, Diagram *)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { 0x80000000 | 7, 2 },
        }}),
        // Signal 'projectModified'
        QtMocHelpers::SignalData<void(QETProject *, bool)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::Bool, 2 },
        }}),
        // Signal 'projectDiagramsOrderChanged'
        QtMocHelpers::SignalData<void(QETProject *, int, int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Signal 'diagramUsedTemplate'
        QtMocHelpers::SignalData<void(TitleBlockTemplatesCollection *, const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 2 }, { QMetaType::QString, 2 },
        }}),
        // Signal 'readOnlyChanged'
        QtMocHelpers::SignalData<void(QETProject *, bool)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::Bool, 2 },
        }}),
        // Signal 'reportPropertiesChanged'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 },
        }}),
        // Signal 'XRefPropertiesChanged'
        QtMocHelpers::SignalData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'addAutoNumDiagram'
        QtMocHelpers::SignalData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'elementAutoNumAdded'
        QtMocHelpers::SignalData<void(QString)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Signal 'elementAutoNumRemoved'
        QtMocHelpers::SignalData<void(QString)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Signal 'conductorAutoNumAdded'
        QtMocHelpers::SignalData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'conductorAutoNumRemoved'
        QtMocHelpers::SignalData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'folioAutoNumAdded'
        QtMocHelpers::SignalData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'autoNumContextUpdated'
        QtMocHelpers::SignalData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'folioAutoNumRemoved'
        QtMocHelpers::SignalData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'defaultTitleBlockPropertiesChanged'
        QtMocHelpers::SignalData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'conductorAutoNumChanged'
        QtMocHelpers::SignalData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'addNewDiagram'
        QtMocHelpers::SlotData<Diagram *(int)>(29, 2, QMC::AccessPublic, 0x80000000 | 7, {{
            { QMetaType::Int, 30 },
        }}),
        // Slot 'addNewDiagram'
        QtMocHelpers::SlotData<Diagram *()>(29, 2, QMC::AccessPublic | QMC::MethodCloned, 0x80000000 | 7),
        // Slot 'removeDiagram'
        QtMocHelpers::SlotData<void(Diagram *)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 2 },
        }}),
        // Slot 'diagramOrderChanged'
        QtMocHelpers::SlotData<void(int, int)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Slot 'setModified'
        QtMocHelpers::SlotData<void(bool)>(33, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'updateDiagramsFolioData'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateDiagramsTitleBlockTemplate'
        QtMocHelpers::SlotData<void(TitleBlockTemplatesCollection *, const QString &)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 12, 2 }, { QMetaType::QString, 2 },
        }}),
        // Slot 'removeDiagramsTitleBlockTemplate'
        QtMocHelpers::SlotData<void(TitleBlockTemplatesCollection *, const QString &)>(36, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 12, 2 }, { QMetaType::QString, 2 },
        }}),
        // Slot 'usedTitleBlockTemplateChanged'
        QtMocHelpers::SlotData<void(const QString &)>(37, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'undoStackChanged'
        QtMocHelpers::SlotData<void(bool)>(38, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 39 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'autoConductor'
        QtMocHelpers::PropertyData<bool>(40, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet),
        // property 'autoBreakConductor'
        QtMocHelpers::PropertyData<bool>(41, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QETProject, qt_meta_tag_ZN10QETProjectE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QETProject::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10QETProjectE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10QETProjectE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10QETProjectE_t>.metaTypes,
    nullptr
} };

void QETProject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QETProject *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->projectFilePathChanged((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->projectTitleChanged((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->projectInformationsChanged((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1]))); break;
        case 3: _t->diagramAdded((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[2]))); break;
        case 4: _t->diagramRemoved((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[2]))); break;
        case 5: _t->projectModified((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 6: _t->projectDiagramsOrderChanged((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 7: _t->diagramUsedTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->readOnlyChanged((*reinterpret_cast<std::add_pointer_t<QETProject*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 9: _t->reportPropertiesChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->XRefPropertiesChanged(); break;
        case 11: _t->addAutoNumDiagram(); break;
        case 12: _t->elementAutoNumAdded((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->elementAutoNumRemoved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->conductorAutoNumAdded(); break;
        case 15: _t->conductorAutoNumRemoved(); break;
        case 16: _t->folioAutoNumAdded(); break;
        case 17: _t->autoNumContextUpdated(); break;
        case 18: _t->folioAutoNumRemoved(); break;
        case 19: _t->defaultTitleBlockPropertiesChanged(); break;
        case 20: _t->conductorAutoNumChanged(); break;
        case 21: { Diagram* _r = _t->addNewDiagram((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<Diagram**>(_a[0]) = std::move(_r); }  break;
        case 22: { Diagram* _r = _t->addNewDiagram();
            if (_a[0]) *reinterpret_cast<Diagram**>(_a[0]) = std::move(_r); }  break;
        case 23: _t->removeDiagram((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 24: _t->diagramOrderChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 25: _t->setModified((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 26: _t->updateDiagramsFolioData(); break;
        case 27: _t->updateDiagramsTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 28: _t->removeDiagramsTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<TitleBlockTemplatesCollection*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 29: _t->usedTitleBlockTemplateChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 30: _t->undoStackChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
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
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Diagram* >(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Diagram* >(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplatesCollection* >(); break;
            }
            break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QETProject* >(); break;
            }
            break;
        case 23:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Diagram* >(); break;
            }
            break;
        case 27:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplatesCollection* >(); break;
            }
            break;
        case 28:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TitleBlockTemplatesCollection* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * , const QString & )>(_a, &QETProject::projectFilePathChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * , const QString & )>(_a, &QETProject::projectTitleChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * )>(_a, &QETProject::projectInformationsChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * , Diagram * )>(_a, &QETProject::diagramAdded, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * , Diagram * )>(_a, &QETProject::diagramRemoved, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * , bool )>(_a, &QETProject::projectModified, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * , int , int )>(_a, &QETProject::projectDiagramsOrderChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(TitleBlockTemplatesCollection * , const QString & )>(_a, &QETProject::diagramUsedTemplate, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QETProject * , bool )>(_a, &QETProject::readOnlyChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(const QString & , const QString & )>(_a, &QETProject::reportPropertiesChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::XRefPropertiesChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::addAutoNumDiagram, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QString )>(_a, &QETProject::elementAutoNumAdded, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)(QString )>(_a, &QETProject::elementAutoNumRemoved, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::conductorAutoNumAdded, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::conductorAutoNumRemoved, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::folioAutoNumAdded, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::autoNumContextUpdated, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::folioAutoNumRemoved, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::defaultTitleBlockPropertiesChanged, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (QETProject::*)()>(_a, &QETProject::conductorAutoNumChanged, 20))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->autoConductor(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->autoBreakConductor(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setAutoConductor(*reinterpret_cast<bool*>(_v)); break;
        case 1: _t->setAutoBreakConductor(*reinterpret_cast<bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *QETProject::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QETProject::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10QETProjectE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int QETProject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void QETProject::projectFilePathChanged(QETProject * _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void QETProject::projectTitleChanged(QETProject * _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void QETProject::projectInformationsChanged(QETProject * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void QETProject::diagramAdded(QETProject * _t1, Diagram * _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void QETProject::diagramRemoved(QETProject * _t1, Diagram * _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void QETProject::projectModified(QETProject * _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void QETProject::projectDiagramsOrderChanged(QETProject * _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3);
}

// SIGNAL 7
void QETProject::diagramUsedTemplate(TitleBlockTemplatesCollection * _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2);
}

// SIGNAL 8
void QETProject::readOnlyChanged(QETProject * _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}

// SIGNAL 9
void QETProject::reportPropertiesChanged(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1, _t2);
}

// SIGNAL 10
void QETProject::XRefPropertiesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void QETProject::addAutoNumDiagram()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void QETProject::elementAutoNumAdded(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1);
}

// SIGNAL 13
void QETProject::elementAutoNumRemoved(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1);
}

// SIGNAL 14
void QETProject::conductorAutoNumAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void QETProject::conductorAutoNumRemoved()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void QETProject::folioAutoNumAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void QETProject::autoNumContextUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 17, nullptr);
}

// SIGNAL 18
void QETProject::folioAutoNumRemoved()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}

// SIGNAL 19
void QETProject::defaultTitleBlockPropertiesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 19, nullptr);
}

// SIGNAL 20
void QETProject::conductorAutoNumChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}
QT_WARNING_POP
