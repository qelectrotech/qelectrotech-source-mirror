/****************************************************************************
** Meta object code from reading C++ file 'diagramtextitem.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/diagramtextitem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'diagramtextitem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15DiagramTextItemE_t {};
} // unnamed namespace

template <> constexpr inline auto DiagramTextItem::qt_create_metaobjectdata<qt_meta_tag_ZN15DiagramTextItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DiagramTextItem",
        "colorChanged",
        "",
        "QColor",
        "color",
        "alignmentChanged",
        "Qt::Alignment",
        "alignment",
        "textEdited",
        "old_str",
        "new_str",
        "fontChanged",
        "QFont",
        "font",
        "plainText"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'colorChanged'
        QtMocHelpers::SignalData<void(QColor)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'alignmentChanged'
        QtMocHelpers::SignalData<void(Qt::Alignment)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Signal 'textEdited'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 }, { QMetaType::QString, 10 },
        }}),
        // Signal 'fontChanged'
        QtMocHelpers::SignalData<void(QFont)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'color'
        QtMocHelpers::PropertyData<QColor>(4, 0x80000000 | 3, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 0),
        // property 'alignment'
        QtMocHelpers::PropertyData<Qt::Alignment>(7, 0x80000000 | 6, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 1),
        // property 'plainText'
        QtMocHelpers::PropertyData<QString>(14, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet),
        // property 'font'
        QtMocHelpers::PropertyData<QFont>(13, 0x80000000 | 12, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 3),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DiagramTextItem, qt_meta_tag_ZN15DiagramTextItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DiagramTextItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsTextItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15DiagramTextItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15DiagramTextItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15DiagramTextItemE_t>.metaTypes,
    nullptr
} };

void DiagramTextItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DiagramTextItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->colorChanged((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 1: _t->alignmentChanged((*reinterpret_cast<std::add_pointer_t<Qt::Alignment>>(_a[1]))); break;
        case 2: _t->textEdited((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->fontChanged((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DiagramTextItem::*)(QColor )>(_a, &DiagramTextItem::colorChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramTextItem::*)(Qt::Alignment )>(_a, &DiagramTextItem::alignmentChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramTextItem::*)(const QString & , const QString & )>(_a, &DiagramTextItem::textEdited, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramTextItem::*)(QFont )>(_a, &DiagramTextItem::fontChanged, 3))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QColor*>(_v) = _t->color(); break;
        case 1: *reinterpret_cast<Qt::Alignment*>(_v) = _t->alignment(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->toPlainText(); break;
        case 3: *reinterpret_cast<QFont*>(_v) = _t->font(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setColor(*reinterpret_cast<QColor*>(_v)); break;
        case 1: _t->setAlignment(*reinterpret_cast<Qt::Alignment*>(_v)); break;
        case 2: _t->setPlainText(*reinterpret_cast<QString*>(_v)); break;
        case 3: _t->setFont(*reinterpret_cast<QFont*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *DiagramTextItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DiagramTextItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15DiagramTextItemE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsTextItem::qt_metacast(_clname);
}

int DiagramTextItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsTextItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void DiagramTextItem::colorChanged(QColor _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void DiagramTextItem::alignmentChanged(Qt::Alignment _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void DiagramTextItem::textEdited(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void DiagramTextItem::fontChanged(QFont _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
