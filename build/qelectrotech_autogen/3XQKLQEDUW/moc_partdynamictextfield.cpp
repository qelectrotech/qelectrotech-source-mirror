/****************************************************************************
** Meta object code from reading C++ file 'partdynamictextfield.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/graphicspart/partdynamictextfield.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'partdynamictextfield.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20PartDynamicTextFieldE_t {};
} // unnamed namespace

template <> constexpr inline auto PartDynamicTextField::qt_create_metaobjectdata<qt_meta_tag_ZN20PartDynamicTextFieldE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PartDynamicTextField",
        "taggChanged",
        "",
        "tagg",
        "textChanged",
        "text",
        "textFromChanged",
        "DynamicElementTextItem::TextFrom",
        "text_from",
        "infoNameChanged",
        "info",
        "compositeTextChanged",
        "colorChanged",
        "QColor",
        "color",
        "frameChanged",
        "frame",
        "textWidthChanged",
        "width",
        "alignmentChanged",
        "Qt::Alignment",
        "alignment",
        "fontChanged",
        "QFont",
        "font",
        "keepVisualRotationChanged",
        "keep",
        "rotationPointCenterChanged",
        "center",
        "textFrom",
        "infoName",
        "compositeText",
        "textWidth",
        "keepVisualRotation",
        "rotationPointCenter"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'taggChanged'
        QtMocHelpers::SignalData<void(QString)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'textChanged'
        QtMocHelpers::SignalData<void(QString)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'textFromChanged'
        QtMocHelpers::SignalData<void(DynamicElementTextItem::TextFrom)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'infoNameChanged'
        QtMocHelpers::SignalData<void(QString)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Signal 'compositeTextChanged'
        QtMocHelpers::SignalData<void(QString)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'colorChanged'
        QtMocHelpers::SignalData<void(QColor)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Signal 'frameChanged'
        QtMocHelpers::SignalData<void(bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 16 },
        }}),
        // Signal 'textWidthChanged'
        QtMocHelpers::SignalData<void(qreal)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QReal, 18 },
        }}),
        // Signal 'alignmentChanged'
        QtMocHelpers::SignalData<void(Qt::Alignment)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 20, 21 },
        }}),
        // Signal 'fontChanged'
        QtMocHelpers::SignalData<void(QFont)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 23, 24 },
        }}),
        // Signal 'keepVisualRotationChanged'
        QtMocHelpers::SignalData<void(bool)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 26 },
        }}),
        // Signal 'rotationPointCenterChanged'
        QtMocHelpers::SignalData<void(bool)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 28 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'text'
        QtMocHelpers::PropertyData<QString>(5, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'textFrom'
        QtMocHelpers::PropertyData<DynamicElementTextItem::TextFrom>(29, 0x80000000 | 7, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 2),
        // property 'infoName'
        QtMocHelpers::PropertyData<QString>(30, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'compositeText'
        QtMocHelpers::PropertyData<QString>(31, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'color'
        QtMocHelpers::PropertyData<QColor>(14, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 5),
        // property 'frame'
        QtMocHelpers::PropertyData<bool>(16, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
        // property 'textWidth'
        QtMocHelpers::PropertyData<qreal>(32, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 7),
        // property 'alignment'
        QtMocHelpers::PropertyData<Qt::Alignment>(21, 0x80000000 | 20, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 8),
        // property 'font'
        QtMocHelpers::PropertyData<QFont>(24, 0x80000000 | 23, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 9),
        // property 'keepVisualRotation'
        QtMocHelpers::PropertyData<bool>(33, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 10),
        // property 'rotationPointCenter'
        QtMocHelpers::PropertyData<bool>(34, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 11),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PartDynamicTextField, qt_meta_tag_ZN20PartDynamicTextFieldE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT static const QMetaObject::SuperData qt_meta_extradata_ZN20PartDynamicTextFieldE[] = {
    QMetaObject::SuperData::link<DynamicElementTextItem::staticMetaObject>(),
    nullptr
};

Q_CONSTINIT const QMetaObject PartDynamicTextField::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsTextItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20PartDynamicTextFieldE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20PartDynamicTextFieldE_t>.data,
    qt_static_metacall,
    qt_meta_extradata_ZN20PartDynamicTextFieldE,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN20PartDynamicTextFieldE_t>.metaTypes,
    nullptr
} };

void PartDynamicTextField::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PartDynamicTextField *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->taggChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->textChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->textFromChanged((*reinterpret_cast<std::add_pointer_t<DynamicElementTextItem::TextFrom>>(_a[1]))); break;
        case 3: _t->infoNameChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->compositeTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->colorChanged((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 6: _t->frameChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->textWidthChanged((*reinterpret_cast<std::add_pointer_t<qreal>>(_a[1]))); break;
        case 8: _t->alignmentChanged((*reinterpret_cast<std::add_pointer_t<Qt::Alignment>>(_a[1]))); break;
        case 9: _t->fontChanged((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1]))); break;
        case 10: _t->keepVisualRotationChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 11: _t->rotationPointCenterChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(QString )>(_a, &PartDynamicTextField::taggChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(QString )>(_a, &PartDynamicTextField::textChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(DynamicElementTextItem::TextFrom )>(_a, &PartDynamicTextField::textFromChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(QString )>(_a, &PartDynamicTextField::infoNameChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(QString )>(_a, &PartDynamicTextField::compositeTextChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(QColor )>(_a, &PartDynamicTextField::colorChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(bool )>(_a, &PartDynamicTextField::frameChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(qreal )>(_a, &PartDynamicTextField::textWidthChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(Qt::Alignment )>(_a, &PartDynamicTextField::alignmentChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(QFont )>(_a, &PartDynamicTextField::fontChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(bool )>(_a, &PartDynamicTextField::keepVisualRotationChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartDynamicTextField::*)(bool )>(_a, &PartDynamicTextField::rotationPointCenterChanged, 11))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->text(); break;
        case 1: *reinterpret_cast<DynamicElementTextItem::TextFrom*>(_v) = _t->textFrom(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->infoName(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->compositeText(); break;
        case 4: *reinterpret_cast<QColor*>(_v) = _t->color(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->frame(); break;
        case 6: *reinterpret_cast<qreal*>(_v) = _t->textWidth(); break;
        case 7: *reinterpret_cast<Qt::Alignment*>(_v) = _t->alignment(); break;
        case 8: *reinterpret_cast<QFont*>(_v) = _t->font(); break;
        case 9: *reinterpret_cast<bool*>(_v) = _t->keepVisualRotation(); break;
        case 10: *reinterpret_cast<bool*>(_v) = _t->rotationPointCenter(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setText(*reinterpret_cast<QString*>(_v)); break;
        case 1: _t->setTextFrom(*reinterpret_cast<DynamicElementTextItem::TextFrom*>(_v)); break;
        case 2: _t->setInfoName(*reinterpret_cast<QString*>(_v)); break;
        case 3: _t->setCompositeText(*reinterpret_cast<QString*>(_v)); break;
        case 4: _t->setColor(*reinterpret_cast<QColor*>(_v)); break;
        case 5: _t->setFrame(*reinterpret_cast<bool*>(_v)); break;
        case 6: _t->setTextWidth(*reinterpret_cast<qreal*>(_v)); break;
        case 7: _t->setAlignment(*reinterpret_cast<Qt::Alignment*>(_v)); break;
        case 8: _t->setFont(*reinterpret_cast<QFont*>(_v)); break;
        case 9: _t->setKeepVisualRotation(*reinterpret_cast<bool*>(_v)); break;
        case 10: _t->setRotationPointCenter(*reinterpret_cast<bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *PartDynamicTextField::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PartDynamicTextField::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20PartDynamicTextFieldE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "CustomElementPart"))
        return static_cast< CustomElementPart*>(this);
    return QGraphicsTextItem::qt_metacast(_clname);
}

int PartDynamicTextField::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsTextItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void PartDynamicTextField::taggChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void PartDynamicTextField::textChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void PartDynamicTextField::textFromChanged(DynamicElementTextItem::TextFrom _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void PartDynamicTextField::infoNameChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void PartDynamicTextField::compositeTextChanged(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void PartDynamicTextField::colorChanged(QColor _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void PartDynamicTextField::frameChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void PartDynamicTextField::textWidthChanged(qreal _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void PartDynamicTextField::alignmentChanged(Qt::Alignment _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void PartDynamicTextField::fontChanged(QFont _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void PartDynamicTextField::keepVisualRotationChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void PartDynamicTextField::rotationPointCenterChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}
QT_WARNING_POP
