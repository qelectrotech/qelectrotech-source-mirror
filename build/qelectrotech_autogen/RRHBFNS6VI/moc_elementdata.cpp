/****************************************************************************
** Meta object code from reading C++ file 'elementdata.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/properties/elementdata.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'elementdata.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11ElementDataE_t {};
} // unnamed namespace

template <> constexpr inline auto ElementData::qt_create_metaobjectdata<qt_meta_tag_ZN11ElementDataE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ElementData",
        "Type",
        "Simple",
        "NextReport",
        "PreviousReport",
        "AllReport",
        "Master",
        "Slave",
        "Terminal",
        "Thumbnail",
        "ConductorDefinition",
        "MasterType",
        "Coil",
        "Protection",
        "Commutator",
        "PLC",
        "SlaveType",
        "SSimple",
        "Power",
        "DelayOn",
        "DelayOff",
        "delayOnOff",
        "PLCSlave",
        "SlaveState",
        "NO",
        "NC",
        "SW",
        "Other",
        "PlcIOType",
        "EntreeDigitale",
        "SortieDigitale",
        "EntreeAnalogique",
        "SortieAnalogique",
        "EntreeUniverselle",
        "SortieUniverselle",
        "TerminalType",
        "TTGeneric",
        "TTFuse",
        "TTSectional",
        "TTDiode",
        "TTGround",
        "TerminalFunction",
        "TFGeneric",
        "TFPhase",
        "TFNeutral"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'Type'
        QtMocHelpers::EnumData<enum Type>(1, 1, QMC::EnumFlags{}).add({
            {    2, Type::Simple },
            {    3, Type::NextReport },
            {    4, Type::PreviousReport },
            {    5, Type::AllReport },
            {    6, Type::Master },
            {    7, Type::Slave },
            {    8, Type::Terminal },
            {    9, Type::Thumbnail },
            {   10, Type::ConductorDefinition },
        }),
        // enum 'MasterType'
        QtMocHelpers::EnumData<enum MasterType>(11, 11, QMC::EnumFlags{}).add({
            {   12, MasterType::Coil },
            {   13, MasterType::Protection },
            {   14, MasterType::Commutator },
            {   15, MasterType::PLC },
        }),
        // enum 'SlaveType'
        QtMocHelpers::EnumData<enum SlaveType>(16, 16, QMC::EnumFlags{}).add({
            {   17, SlaveType::SSimple },
            {   18, SlaveType::Power },
            {   19, SlaveType::DelayOn },
            {   20, SlaveType::DelayOff },
            {   21, SlaveType::delayOnOff },
            {   22, SlaveType::PLCSlave },
        }),
        // enum 'SlaveState'
        QtMocHelpers::EnumData<enum SlaveState>(23, 23, QMC::EnumFlags{}).add({
            {   24, SlaveState::NO },
            {   25, SlaveState::NC },
            {   26, SlaveState::SW },
            {   27, SlaveState::Other },
        }),
        // enum 'PlcIOType'
        QtMocHelpers::EnumData<enum PlcIOType>(28, 28, QMC::EnumFlags{}).add({
            {   29, PlcIOType::EntreeDigitale },
            {   30, PlcIOType::SortieDigitale },
            {   31, PlcIOType::EntreeAnalogique },
            {   32, PlcIOType::SortieAnalogique },
            {   33, PlcIOType::EntreeUniverselle },
            {   34, PlcIOType::SortieUniverselle },
        }),
        // enum 'TerminalType'
        QtMocHelpers::EnumData<enum TerminalType>(35, 35, QMC::EnumFlags{}).add({
            {   36, TerminalType::TTGeneric },
            {   37, TerminalType::TTFuse },
            {   38, TerminalType::TTSectional },
            {   39, TerminalType::TTDiode },
            {   40, TerminalType::TTGround },
        }),
        // enum 'TerminalFunction'
        QtMocHelpers::EnumData<enum TerminalFunction>(41, 41, QMC::EnumFlags{}).add({
            {   42, TerminalFunction::TFGeneric },
            {   43, TerminalFunction::TFPhase },
            {   44, TerminalFunction::TFNeutral },
        }),
    };
    return QtMocHelpers::metaObjectData<ElementData, qt_meta_tag_ZN11ElementDataE_t>(QMC::PropertyAccessInStaticMetaCall, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ElementData::staticMetaObject = { {
    QtPrivate::MetaObjectForType<PropertiesInterface>::value,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ElementDataE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ElementDataE_t>.data,
    nullptr,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11ElementDataE_t>.metaTypes,
    nullptr
} };

QT_WARNING_POP
