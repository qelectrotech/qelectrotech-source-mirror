/****************************************************************************
** Meta object code from reading C++ file 'parttext.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/graphicspart/parttext.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'parttext.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8PartTextE_t {};
} // unnamed namespace

template <> constexpr inline auto PartText::qt_create_metaobjectdata<qt_meta_tag_ZN8PartTextE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PartText",
        "fontChanged",
        "",
        "QFont",
        "font",
        "colorChanged",
        "QColor",
        "color",
        "plainTextChanged",
        "text",
        "alignmentChanged",
        "Qt::Alignment",
        "alignment",
        "adjustItemPosition",
        "setEditable",
        "startEdition",
        "endEdition",
        "real_size"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'fontChanged'
        QtMocHelpers::SignalData<void(const QFont &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'colorChanged'
        QtMocHelpers::SignalData<void(const QColor &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Signal 'plainTextChanged'
        QtMocHelpers::SignalData<void(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Signal 'alignmentChanged'
        QtMocHelpers::SignalData<void(Qt::Alignment)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Slot 'adjustItemPosition'
        QtMocHelpers::SlotData<void(int)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'adjustItemPosition'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Slot 'setEditable'
        QtMocHelpers::SlotData<void(bool)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'startEdition'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'endEdition'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'real_size'
        QtMocHelpers::PropertyData<qreal>(17, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'color'
        QtMocHelpers::PropertyData<QColor>(7, 0x80000000 | 6, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag, 1),
        // property 'text'
        QtMocHelpers::PropertyData<QString>(9, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable, 2),
        // property 'font'
        QtMocHelpers::PropertyData<QFont>(4, 0x80000000 | 3, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 0),
        // property 'alignment'
        QtMocHelpers::PropertyData<Qt::Alignment>(12, 0x80000000 | 11, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 3),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PartText, qt_meta_tag_ZN8PartTextE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject PartText::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsTextItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PartTextE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PartTextE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8PartTextE_t>.metaTypes,
    nullptr
} };

void PartText::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PartText *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->fontChanged((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1]))); break;
        case 1: _t->colorChanged((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 2: _t->plainTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->alignmentChanged((*reinterpret_cast<std::add_pointer_t<Qt::Alignment>>(_a[1]))); break;
        case 4: _t->adjustItemPosition((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->adjustItemPosition(); break;
        case 6: _t->setEditable((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->startEdition(); break;
        case 8: _t->endEdition(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PartText::*)(const QFont & )>(_a, &PartText::fontChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartText::*)(const QColor & )>(_a, &PartText::colorChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartText::*)(const QString & )>(_a, &PartText::plainTextChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartText::*)(Qt::Alignment )>(_a, &PartText::alignmentChanged, 3))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<qreal*>(_v) = _t->realSize(); break;
        case 1: *reinterpret_cast<QColor*>(_v) = _t->defaultTextColor(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->toPlainText(); break;
        case 3: *reinterpret_cast<QFont*>(_v) = _t->font(); break;
        case 4: *reinterpret_cast<Qt::Alignment*>(_v) = _t->alignment(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setRealSize(*reinterpret_cast<qreal*>(_v)); break;
        case 1: _t->setDefaultTextColor(*reinterpret_cast<QColor*>(_v)); break;
        case 2: _t->setPlainText(*reinterpret_cast<QString*>(_v)); break;
        case 3: _t->setFont(*reinterpret_cast<QFont*>(_v)); break;
        case 4: _t->setAlignment(*reinterpret_cast<Qt::Alignment*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *PartText::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PartText::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8PartTextE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "CustomElementPart"))
        return static_cast< CustomElementPart*>(this);
    return QGraphicsTextItem::qt_metacast(_clname);
}

int PartText::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsTextItem::qt_metacall(_c, _id, _a);
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
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void PartText::fontChanged(const QFont & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void PartText::colorChanged(const QColor & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void PartText::plainTextChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void PartText::alignmentChanged(Qt::Alignment _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
