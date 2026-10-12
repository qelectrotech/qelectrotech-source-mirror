/****************************************************************************
** Meta object code from reading C++ file 'bordertitleblock.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/bordertitleblock.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'bordertitleblock.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16BorderTitleBlockE_t {};
} // unnamed namespace

template <> constexpr inline auto BorderTitleBlock::qt_create_metaobjectdata<qt_meta_tag_ZN16BorderTitleBlockE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "BorderTitleBlock",
        "borderChanged",
        "",
        "QRectF",
        "old_border",
        "new_border",
        "displayChanged",
        "titleBlockFolioChanged",
        "needFolioData",
        "needTitleBlockTemplate",
        "informationChanged",
        "titleBlockTemplateChanged",
        "titleBlockTemplateRemoved",
        "const TitleBlockTemplate*",
        "displayTitleBlock",
        "displayColumns",
        "displayRows",
        "displayBorder"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'borderChanged'
        QtMocHelpers::SignalData<void(QRectF, QRectF)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 3, 5 },
        }}),
        // Signal 'displayChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'titleBlockFolioChanged'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Signal 'needFolioData'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'needTitleBlockTemplate'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Signal 'informationChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'titleBlockTemplateChanged'
        QtMocHelpers::SlotData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'titleBlockTemplateRemoved'
        QtMocHelpers::SlotData<void(const QString &, const TitleBlockTemplate *)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 }, { 0x80000000 | 13, 2 },
        }}),
        // Slot 'titleBlockTemplateRemoved'
        QtMocHelpers::SlotData<void(const QString &)>(12, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'displayTitleBlock'
        QtMocHelpers::SlotData<void(bool)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'displayColumns'
        QtMocHelpers::SlotData<void(bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'displayRows'
        QtMocHelpers::SlotData<void(bool)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
        // Slot 'displayBorder'
        QtMocHelpers::SlotData<void(bool)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<BorderTitleBlock, qt_meta_tag_ZN16BorderTitleBlockE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject BorderTitleBlock::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16BorderTitleBlockE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16BorderTitleBlockE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16BorderTitleBlockE_t>.metaTypes,
    nullptr
} };

void BorderTitleBlock::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BorderTitleBlock *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->borderChanged((*reinterpret_cast<std::add_pointer_t<QRectF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QRectF>>(_a[2]))); break;
        case 1: _t->displayChanged(); break;
        case 2: _t->titleBlockFolioChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->needFolioData(); break;
        case 4: _t->needTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->informationChanged(); break;
        case 6: _t->titleBlockTemplateChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->titleBlockTemplateRemoved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<const TitleBlockTemplate*>>(_a[2]))); break;
        case 8: _t->titleBlockTemplateRemoved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->displayTitleBlock((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->displayColumns((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 11: _t->displayRows((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 12: _t->displayBorder((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (BorderTitleBlock::*)(QRectF , QRectF )>(_a, &BorderTitleBlock::borderChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (BorderTitleBlock::*)()>(_a, &BorderTitleBlock::displayChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (BorderTitleBlock::*)(const QString & )>(_a, &BorderTitleBlock::titleBlockFolioChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (BorderTitleBlock::*)()>(_a, &BorderTitleBlock::needFolioData, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (BorderTitleBlock::*)(const QString & )>(_a, &BorderTitleBlock::needTitleBlockTemplate, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (BorderTitleBlock::*)()>(_a, &BorderTitleBlock::informationChanged, 5))
            return;
    }
}

const QMetaObject *BorderTitleBlock::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BorderTitleBlock::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16BorderTitleBlockE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int BorderTitleBlock::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
    return _id;
}

// SIGNAL 0
void BorderTitleBlock::borderChanged(QRectF _t1, QRectF _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void BorderTitleBlock::displayChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BorderTitleBlock::titleBlockFolioChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void BorderTitleBlock::needFolioData()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void BorderTitleBlock::needTitleBlockTemplate(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void BorderTitleBlock::informationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
