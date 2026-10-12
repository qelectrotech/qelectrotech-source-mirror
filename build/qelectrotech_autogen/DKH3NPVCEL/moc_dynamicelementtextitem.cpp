/****************************************************************************
** Meta object code from reading C++ file 'dynamicelementtextitem.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/dynamicelementtextitem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dynamicelementtextitem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN22DynamicElementTextItemE_t {};
} // unnamed namespace

template <> constexpr inline auto DynamicElementTextItem::qt_create_metaobjectdata<qt_meta_tag_ZN22DynamicElementTextItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DynamicElementTextItem",
        "textChanged",
        "",
        "text",
        "textFromChanged",
        "DynamicElementTextItem::TextFrom",
        "text_from",
        "infoNameChanged",
        "info",
        "compositeTextChanged",
        "frameChanged",
        "frame",
        "plainTextChanged",
        "textWidthChanged",
        "width",
        "keepVisualRotationChanged",
        "keep",
        "rotationPointCenterChanged",
        "center",
        "textFrom",
        "TextFrom",
        "infoName",
        "compositeText",
        "textWidth",
        "keepVisualRotation",
        "rotationPointCenter",
        "UserText",
        "ElementInfo",
        "CompositeText"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'textChanged'
        QtMocHelpers::SignalData<void(QString)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'textFromChanged'
        QtMocHelpers::SignalData<void(DynamicElementTextItem::TextFrom)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Signal 'infoNameChanged'
        QtMocHelpers::SignalData<void(QString)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'compositeTextChanged'
        QtMocHelpers::SignalData<void(QString)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'frameChanged'
        QtMocHelpers::SignalData<void(bool)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 11 },
        }}),
        // Signal 'plainTextChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'textWidthChanged'
        QtMocHelpers::SignalData<void(qreal)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QReal, 14 },
        }}),
        // Signal 'keepVisualRotationChanged'
        QtMocHelpers::SignalData<void(bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 16 },
        }}),
        // Signal 'rotationPointCenterChanged'
        QtMocHelpers::SignalData<void(bool)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 18 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'text'
        QtMocHelpers::PropertyData<QString>(3, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'textFrom'
        QtMocHelpers::PropertyData<enum TextFrom>(19, 0x80000000 | 20, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 1),
        // property 'infoName'
        QtMocHelpers::PropertyData<QString>(21, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 2),
        // property 'compositeText'
        QtMocHelpers::PropertyData<QString>(22, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'frame'
        QtMocHelpers::PropertyData<bool>(11, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'textWidth'
        QtMocHelpers::PropertyData<qreal>(23, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
        // property 'keepVisualRotation'
        QtMocHelpers::PropertyData<bool>(24, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 7),
        // property 'rotationPointCenter'
        QtMocHelpers::PropertyData<bool>(25, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 8),
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'TextFrom'
        QtMocHelpers::EnumData<enum TextFrom>(20, 20, QMC::EnumFlags{}).add({
            {   26, TextFrom::UserText },
            {   27, TextFrom::ElementInfo },
            {   28, TextFrom::CompositeText },
        }),
    };
    return QtMocHelpers::metaObjectData<DynamicElementTextItem, qt_meta_tag_ZN22DynamicElementTextItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DynamicElementTextItem::staticMetaObject = { {
    QMetaObject::SuperData::link<DiagramTextItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22DynamicElementTextItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22DynamicElementTextItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN22DynamicElementTextItemE_t>.metaTypes,
    nullptr
} };

void DynamicElementTextItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DynamicElementTextItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->textChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->textFromChanged((*reinterpret_cast<std::add_pointer_t<DynamicElementTextItem::TextFrom>>(_a[1]))); break;
        case 2: _t->infoNameChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->compositeTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->frameChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 5: _t->plainTextChanged(); break;
        case 6: _t->textWidthChanged((*reinterpret_cast<std::add_pointer_t<qreal>>(_a[1]))); break;
        case 7: _t->keepVisualRotationChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->rotationPointCenterChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(QString )>(_a, &DynamicElementTextItem::textChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(DynamicElementTextItem::TextFrom )>(_a, &DynamicElementTextItem::textFromChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(QString )>(_a, &DynamicElementTextItem::infoNameChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(QString )>(_a, &DynamicElementTextItem::compositeTextChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(bool )>(_a, &DynamicElementTextItem::frameChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)()>(_a, &DynamicElementTextItem::plainTextChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(qreal )>(_a, &DynamicElementTextItem::textWidthChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(bool )>(_a, &DynamicElementTextItem::keepVisualRotationChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (DynamicElementTextItem::*)(bool )>(_a, &DynamicElementTextItem::rotationPointCenterChanged, 8))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->text(); break;
        case 1: *reinterpret_cast<enum TextFrom*>(_v) = _t->textFrom(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->infoName(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->compositeText(); break;
        case 4: *reinterpret_cast<bool*>(_v) = _t->frame(); break;
        case 5: *reinterpret_cast<qreal*>(_v) = _t->textWidth(); break;
        case 6: *reinterpret_cast<bool*>(_v) = _t->keepVisualRotation(); break;
        case 7: *reinterpret_cast<bool*>(_v) = _t->rotationPointCenter(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setText(*reinterpret_cast<QString*>(_v)); break;
        case 1: _t->setTextFrom(*reinterpret_cast<enum TextFrom*>(_v)); break;
        case 2: _t->setInfoName(*reinterpret_cast<QString*>(_v)); break;
        case 3: _t->setCompositeText(*reinterpret_cast<QString*>(_v)); break;
        case 4: _t->setFrame(*reinterpret_cast<bool*>(_v)); break;
        case 5: _t->setTextWidth(*reinterpret_cast<qreal*>(_v)); break;
        case 6: _t->setKeepVisualRotation(*reinterpret_cast<bool*>(_v)); break;
        case 7: _t->setRotationPointCenter(*reinterpret_cast<bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *DynamicElementTextItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DynamicElementTextItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22DynamicElementTextItemE_t>.strings))
        return static_cast<void*>(this);
    return DiagramTextItem::qt_metacast(_clname);
}

int DynamicElementTextItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DiagramTextItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void DynamicElementTextItem::textChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void DynamicElementTextItem::textFromChanged(DynamicElementTextItem::TextFrom _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void DynamicElementTextItem::infoNameChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void DynamicElementTextItem::compositeTextChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void DynamicElementTextItem::frameChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void DynamicElementTextItem::plainTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void DynamicElementTextItem::textWidthChanged(qreal _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void DynamicElementTextItem::keepVisualRotationChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void DynamicElementTextItem::rotationPointCenterChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}
QT_WARNING_POP
