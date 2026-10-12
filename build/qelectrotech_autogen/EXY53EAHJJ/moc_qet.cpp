/****************************************************************************
** Meta object code from reading C++ file 'qet.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qet.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qet.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN3QetE_t {};
} // unnamed namespace

template <> constexpr inline auto Qet::qt_create_metaobjectdata<qt_meta_tag_ZN3QetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Qet",
        "EndType",
        "None",
        "Simple",
        "Triangle",
        "Circle",
        "Diamond",
        "Orientation",
        "North",
        "East",
        "South",
        "West"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'EndType'
        QtMocHelpers::EnumData<enum EndType>(1, 1, QMC::EnumFlags{}).add({
            {    2, EndType::None },
            {    3, EndType::Simple },
            {    4, EndType::Triangle },
            {    5, EndType::Circle },
            {    6, EndType::Diamond },
        }),
        // enum 'Orientation'
        QtMocHelpers::EnumData<enum Orientation>(7, 7, QMC::EnumFlags{}).add({
            {    8, Orientation::North },
            {    9, Orientation::East },
            {   10, Orientation::South },
            {   11, Orientation::West },
        }),
    };
    return QtMocHelpers::metaObjectData<Qet, qt_meta_tag_ZN3QetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Qet::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN3QetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN3QetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN3QetE_t>.metaTypes,
    nullptr
} };

void Qet::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Qet *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *Qet::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Qet::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN3QetE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Qet::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
