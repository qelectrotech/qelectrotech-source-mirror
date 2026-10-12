/****************************************************************************
** Meta object code from reading C++ file 'partterminal.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/editor/graphicspart/partterminal.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'partterminal.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12PartTerminalE_t {};
} // unnamed namespace

template <> constexpr inline auto PartTerminal::qt_create_metaobjectdata<qt_meta_tag_ZN12PartTerminalE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PartTerminal",
        "orientationChanged",
        "",
        "nameChanged",
        "terminalTypeChanged",
        "showNameChanged",
        "labelPosChanged",
        "labelFontChanged",
        "labelRotationChanged",
        "labelHAlignmentChanged",
        "labelVAlignmentChanged",
        "labelFrameChanged",
        "labelColorChanged",
        "useMasterLabelChanged",
        "masterLabelIndexChanged",
        "orientation",
        "Qet::Orientation",
        "terminal_name",
        "terminal_type",
        "TerminalData::Type",
        "show_name",
        "label_pos",
        "QPointF",
        "label_font",
        "QFont",
        "label_rotation",
        "label_halignment",
        "Qt::Alignment",
        "label_valignment",
        "label_frame",
        "label_color",
        "QColor",
        "use_master_label",
        "master_label_index"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'orientationChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'nameChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'terminalTypeChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showNameChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'labelPosChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'labelFontChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'labelRotationChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'labelHAlignmentChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'labelVAlignmentChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'labelFrameChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'labelColorChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'useMasterLabelChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'masterLabelIndexChanged'
        QtMocHelpers::SignalData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'orientation'
        QtMocHelpers::PropertyData<Qet::Orientation>(15, 0x80000000 | 16, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
        // property 'terminal_name'
        QtMocHelpers::PropertyData<QString>(17, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'terminal_type'
        QtMocHelpers::PropertyData<TerminalData::Type>(18, 0x80000000 | 19, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'show_name'
        QtMocHelpers::PropertyData<bool>(20, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'label_pos'
        QtMocHelpers::PropertyData<QPointF>(21, 0x80000000 | 22, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'label_font'
        QtMocHelpers::PropertyData<QFont>(23, 0x80000000 | 24, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'label_rotation'
        QtMocHelpers::PropertyData<qreal>(25, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'label_halignment'
        QtMocHelpers::PropertyData<Qt::Alignment>(26, 0x80000000 | 27, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'label_valignment'
        QtMocHelpers::PropertyData<Qt::Alignment>(28, 0x80000000 | 27, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'label_frame'
        QtMocHelpers::PropertyData<bool>(29, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'label_color'
        QtMocHelpers::PropertyData<QColor>(30, 0x80000000 | 31, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag),
        // property 'use_master_label'
        QtMocHelpers::PropertyData<bool>(32, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable),
        // property 'master_label_index'
        QtMocHelpers::PropertyData<int>(33, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PartTerminal, qt_meta_tag_ZN12PartTerminalE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT static const QMetaObject::SuperData qt_meta_extradata_ZN12PartTerminalE[] = {
    QMetaObject::SuperData::link<Qet::staticMetaObject>(),
    QMetaObject::SuperData::link<TerminalData::staticMetaObject>(),
    nullptr
};

Q_CONSTINIT const QMetaObject PartTerminal::staticMetaObject = { {
    QMetaObject::SuperData::link<CustomElementGraphicPart::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12PartTerminalE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12PartTerminalE_t>.data,
    qt_static_metacall,
    qt_meta_extradata_ZN12PartTerminalE,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12PartTerminalE_t>.metaTypes,
    nullptr
} };

void PartTerminal::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PartTerminal *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->orientationChanged(); break;
        case 1: _t->nameChanged(); break;
        case 2: _t->terminalTypeChanged(); break;
        case 3: _t->showNameChanged(); break;
        case 4: _t->labelPosChanged(); break;
        case 5: _t->labelFontChanged(); break;
        case 6: _t->labelRotationChanged(); break;
        case 7: _t->labelHAlignmentChanged(); break;
        case 8: _t->labelVAlignmentChanged(); break;
        case 9: _t->labelFrameChanged(); break;
        case 10: _t->labelColorChanged(); break;
        case 11: _t->useMasterLabelChanged(); break;
        case 12: _t->masterLabelIndexChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::orientationChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::nameChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::terminalTypeChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::showNameChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::labelPosChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::labelFontChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::labelRotationChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::labelHAlignmentChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::labelVAlignmentChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::labelFrameChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::labelColorChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::useMasterLabelChanged, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (PartTerminal::*)()>(_a, &PartTerminal::masterLabelIndexChanged, 12))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<Qet::Orientation*>(_v) = _t->orientation(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->terminalName(); break;
        case 2: *reinterpret_cast<TerminalData::Type*>(_v) = _t->terminalType(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->showName(); break;
        case 4: *reinterpret_cast<QPointF*>(_v) = _t->labelPos(); break;
        case 5: *reinterpret_cast<QFont*>(_v) = _t->labelFont(); break;
        case 6: *reinterpret_cast<qreal*>(_v) = _t->labelRotation(); break;
        case 7: *reinterpret_cast<Qt::Alignment*>(_v) = _t->labelHAlignment(); break;
        case 8: *reinterpret_cast<Qt::Alignment*>(_v) = _t->labelVAlignment(); break;
        case 9: *reinterpret_cast<bool*>(_v) = _t->labelFrame(); break;
        case 10: *reinterpret_cast<QColor*>(_v) = _t->labelColor(); break;
        case 11: *reinterpret_cast<bool*>(_v) = _t->useMasterLabel(); break;
        case 12: *reinterpret_cast<int*>(_v) = _t->masterLabelIndex(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setOrientation(*reinterpret_cast<Qet::Orientation*>(_v)); break;
        case 1: _t->setTerminalName(*reinterpret_cast<QString*>(_v)); break;
        case 2: _t->setTerminalType(*reinterpret_cast<TerminalData::Type*>(_v)); break;
        case 3: _t->setShowName(*reinterpret_cast<bool*>(_v)); break;
        case 4: _t->setLabelPos(*reinterpret_cast<QPointF*>(_v)); break;
        case 5: _t->setLabelFont(*reinterpret_cast<QFont*>(_v)); break;
        case 6: _t->setLabelRotation(*reinterpret_cast<qreal*>(_v)); break;
        case 7: _t->setLabelHAlignment(*reinterpret_cast<Qt::Alignment*>(_v)); break;
        case 8: _t->setLabelVAlignment(*reinterpret_cast<Qt::Alignment*>(_v)); break;
        case 9: _t->setLabelFrame(*reinterpret_cast<bool*>(_v)); break;
        case 10: _t->setLabelColor(*reinterpret_cast<QColor*>(_v)); break;
        case 11: _t->setUseMasterLabel(*reinterpret_cast<bool*>(_v)); break;
        case 12: _t->setMasterLabelIndex(*reinterpret_cast<int*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *PartTerminal::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PartTerminal::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12PartTerminalE_t>.strings))
        return static_cast<void*>(this);
    return CustomElementGraphicPart::qt_metacast(_clname);
}

int PartTerminal::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CustomElementGraphicPart::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 13;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void PartTerminal::orientationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PartTerminal::nameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PartTerminal::terminalTypeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PartTerminal::showNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PartTerminal::labelPosChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void PartTerminal::labelFontChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void PartTerminal::labelRotationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void PartTerminal::labelHAlignmentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void PartTerminal::labelVAlignmentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void PartTerminal::labelFrameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void PartTerminal::labelColorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void PartTerminal::useMasterLabelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void PartTerminal::masterLabelIndexChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}
QT_WARNING_POP
