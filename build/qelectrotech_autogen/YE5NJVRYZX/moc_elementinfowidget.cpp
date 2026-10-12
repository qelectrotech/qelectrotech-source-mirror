/****************************************************************************
** Meta object code from reading C++ file 'elementinfowidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/elementinfowidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementinfowidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN17ElementInfoWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementInfoWidget::qt_create_metaobjectdata<qt_meta_tag_ZN17ElementInfoWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementInfoWidget",
        "firstActivated",
        "",
        "elementInfoChange",
        "addCustomProperty",
        "key",
        "value",
        "removeCustomProperty",
        "CustomElementInfoPartWidget*",
        "widget"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'firstActivated'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'elementInfoChange'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'addCustomProperty'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 5 }, { QMetaType::QString, 6 },
        }}),
        // Slot 'addCustomProperty'
        QtMocHelpers::SlotData<void(const QString &)>(4, 2, QMC::AccessPrivate | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Slot 'addCustomProperty'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate | QMC::MethodCloned, QMetaType::Void),
        // Slot 'removeCustomProperty'
        QtMocHelpers::SlotData<void(CustomElementInfoPartWidget *)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ElementInfoWidget, qt_meta_tag_ZN17ElementInfoWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementInfoWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<AbstractElementPropertiesEditorWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17ElementInfoWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17ElementInfoWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN17ElementInfoWidgetE_t>.metaTypes,
    nullptr
} };

void ElementInfoWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ElementInfoWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->firstActivated(); break;
        case 1: _t->elementInfoChange(); break;
        case 2: _t->addCustomProperty((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->addCustomProperty((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->addCustomProperty(); break;
        case 5: _t->removeCustomProperty((*reinterpret_cast<std::add_pointer_t<CustomElementInfoPartWidget*>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *ElementInfoWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElementInfoWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17ElementInfoWidgetE_t>.strings))
        return static_cast<void*>(this);
    return AbstractElementPropertiesEditorWidget::qt_metacast(_clname);
}

int ElementInfoWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = AbstractElementPropertiesEditorWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
