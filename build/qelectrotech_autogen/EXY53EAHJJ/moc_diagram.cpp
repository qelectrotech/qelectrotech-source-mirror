/****************************************************************************
** Meta object code from reading C++ file 'diagram.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/diagram.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'diagram.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7DiagramE_t {};
} // unnamed namespace

template <> constexpr inline auto Diagram::qt_create_metaobjectdata<qt_meta_tag_ZN7DiagramE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Diagram",
        "showDiagram",
        "",
        "Diagram*",
        "usedTitleBlockTemplateChanged",
        "diagramTitleChanged",
        "findElementRequired",
        "ElementsLocation",
        "diagramActivated",
        "diagramInformationChanged",
        "adjustSceneRect",
        "titleChanged",
        "titleBlockTemplateChanged",
        "titleBlockTemplateRemoved",
        "setTitleBlockTemplate",
        "loadElmtFolioSeq",
        "loadCndFolioSeq",
        "selectAll",
        "deselectAll",
        "invertSelection"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'showDiagram'
        QtMocHelpers::SignalData<void(Diagram *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'usedTitleBlockTemplateChanged'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Signal 'diagramTitleChanged'
        QtMocHelpers::SignalData<void(Diagram *)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'findElementRequired'
        QtMocHelpers::SignalData<void(const ElementsLocation &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 2 },
        }}),
        // Signal 'diagramActivated'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'diagramInformationChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'adjustSceneRect'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'titleChanged'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'titleBlockTemplateChanged'
        QtMocHelpers::SlotData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'titleBlockTemplateRemoved'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 }, { QMetaType::QString, 2 },
        }}),
        // Slot 'titleBlockTemplateRemoved'
        QtMocHelpers::SlotData<void(const QString &)>(13, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'setTitleBlockTemplate'
        QtMocHelpers::SlotData<void(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'loadElmtFolioSeq'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'loadCndFolioSeq'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectAll'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'deselectAll'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'invertSelection'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Diagram, qt_meta_tag_ZN7DiagramE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Diagram::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsScene::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7DiagramE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7DiagramE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7DiagramE_t>.metaTypes,
    nullptr
} };

void Diagram::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Diagram *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->showDiagram((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 1: _t->usedTitleBlockTemplateChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->diagramTitleChanged((*reinterpret_cast<std::add_pointer_t<Diagram*>>(_a[1]))); break;
        case 3: _t->findElementRequired((*reinterpret_cast<std::add_pointer_t<ElementsLocation>>(_a[1]))); break;
        case 4: _t->diagramActivated(); break;
        case 5: _t->diagramInformationChanged(); break;
        case 6: _t->adjustSceneRect(); break;
        case 7: _t->titleChanged(); break;
        case 8: _t->titleBlockTemplateChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->titleBlockTemplateRemoved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->titleBlockTemplateRemoved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->setTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->loadElmtFolioSeq(); break;
        case 13: _t->loadCndFolioSeq(); break;
        case 14: _t->selectAll(); break;
        case 15: _t->deselectAll(); break;
        case 16: _t->invertSelection(); break;
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
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Diagram* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Diagram* >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ElementsLocation >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Diagram::*)(Diagram * )>(_a, &Diagram::showDiagram, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Diagram::*)(const QString & )>(_a, &Diagram::usedTitleBlockTemplateChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Diagram::*)(Diagram * )>(_a, &Diagram::diagramTitleChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Diagram::*)(const ElementsLocation & )>(_a, &Diagram::findElementRequired, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Diagram::*)()>(_a, &Diagram::diagramActivated, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (Diagram::*)()>(_a, &Diagram::diagramInformationChanged, 5))
            return;
    }
}

const QMetaObject *Diagram::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Diagram::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7DiagramE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsScene::qt_metacast(_clname);
}

int Diagram::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsScene::qt_metacall(_c, _id, _a);
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
void Diagram::showDiagram(Diagram * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void Diagram::usedTitleBlockTemplateChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void Diagram::diagramTitleChanged(Diagram * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void Diagram::findElementRequired(const ElementsLocation & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void Diagram::diagramActivated()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void Diagram::diagramInformationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
