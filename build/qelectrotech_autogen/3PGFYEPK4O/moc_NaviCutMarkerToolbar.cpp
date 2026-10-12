/****************************************************************************
** Meta object code from reading C++ file 'NaviCutMarkerToolbar.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/navi_cut_markers/NaviCutMarkerToolbar.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NaviCutMarkerToolbar.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20NaviCutMarkerToolbarE_t {};
} // unnamed namespace

template <> constexpr inline auto NaviCutMarkerToolbar::qt_create_metaobjectdata<qt_meta_tag_ZN20NaviCutMarkerToolbarE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NaviCutMarkerToolbar",
        "placeNaviCutMarkerRequested",
        "",
        "NaviCutMarkerElement::NaviCutDirection",
        "dir",
        "NaviCutMarkerElement::NaviCutDisplayMode",
        "mode",
        "onActionTriggered"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'placeNaviCutMarkerRequested'
        QtMocHelpers::SignalData<void(NaviCutMarkerElement::NaviCutDirection, NaviCutMarkerElement::NaviCutDisplayMode)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 5, 6 },
        }}),
        // Slot 'onActionTriggered'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NaviCutMarkerToolbar, qt_meta_tag_ZN20NaviCutMarkerToolbarE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NaviCutMarkerToolbar::staticMetaObject = { {
    QMetaObject::SuperData::link<QToolBar::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20NaviCutMarkerToolbarE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20NaviCutMarkerToolbarE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN20NaviCutMarkerToolbarE_t>.metaTypes,
    nullptr
} };

void NaviCutMarkerToolbar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NaviCutMarkerToolbar *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->placeNaviCutMarkerRequested((*reinterpret_cast<std::add_pointer_t<NaviCutMarkerElement::NaviCutDirection>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<NaviCutMarkerElement::NaviCutDisplayMode>>(_a[2]))); break;
        case 1: _t->onActionTriggered(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NaviCutMarkerToolbar::*)(NaviCutMarkerElement::NaviCutDirection , NaviCutMarkerElement::NaviCutDisplayMode )>(_a, &NaviCutMarkerToolbar::placeNaviCutMarkerRequested, 0))
            return;
    }
}

const QMetaObject *NaviCutMarkerToolbar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NaviCutMarkerToolbar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20NaviCutMarkerToolbarE_t>.strings))
        return static_cast<void*>(this);
    return QToolBar::qt_metacast(_clname);
}

int NaviCutMarkerToolbar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QToolBar::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void NaviCutMarkerToolbar::placeNaviCutMarkerRequested(NaviCutMarkerElement::NaviCutDirection _t1, NaviCutMarkerElement::NaviCutDisplayMode _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}
QT_WARNING_POP
