/****************************************************************************
** Meta object code from reading C++ file 'terminaldata.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/properties/terminaldata.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'terminaldata.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12TerminalDataE_t {};
} // unnamed namespace

template <> constexpr inline auto TerminalData::qt_create_metaobjectdata<qt_meta_tag_ZN12TerminalDataE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TerminalData",
        "Type",
        "Generic",
        "Inner",
        "Outer",
        "No",
        "Nc",
        "Common"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'Type'
        QtMocHelpers::EnumData<enum Type>(1, 1, QMC::EnumFlags{}).add({
            {    2, Type::Generic },
            {    3, Type::Inner },
            {    4, Type::Outer },
            {    5, Type::No },
            {    6, Type::Nc },
            {    7, Type::Common },
        }),
    };
    return QtMocHelpers::metaObjectData<TerminalData, qt_meta_tag_ZN12TerminalDataE_t>(QMC::PropertyAccessInStaticMetaCall, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TerminalData::staticMetaObject = { {
    QtPrivate::MetaObjectForType<PropertiesInterface>::value,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12TerminalDataE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12TerminalDataE_t>.data,
    nullptr,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12TerminalDataE_t>.metaTypes,
    nullptr
} };

QT_WARNING_POP
