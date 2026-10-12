/****************************************************************************
** Meta object code from reading C++ file 'templatecellwidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/titleblock/templatecellwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'templatecellwidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN28TitleBlockTemplateCellWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto TitleBlockTemplateCellWidget::qt_create_metaobjectdata<qt_meta_tag_ZN28TitleBlockTemplateCellWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TitleBlockTemplateCellWidget",
        "logoEditionRequested",
        "",
        "cellModified",
        "ModifyTitleBlockCellCommand*",
        "updateFormType",
        "edit",
        "TitleBlockCell*",
        "editType",
        "editName",
        "editLabelDisplayed",
        "editLabel",
        "editValue",
        "editAlignment",
        "editFontSize",
        "editAdjust",
        "editLogo",
        "updateLogosComboBox",
        "const TitleBlockTemplate*",
        "setReadOnly"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'logoEditionRequested'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'cellModified'
        QtMocHelpers::SignalData<void(ModifyTitleBlockCellCommand *) const>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 2 },
        }}),
        // Slot 'updateFormType'
        QtMocHelpers::SlotData<void(int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'edit'
        QtMocHelpers::SlotData<void(TitleBlockCell *)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 2 },
        }}),
        // Slot 'editType'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editName'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editLabelDisplayed'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editLabel'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editValue'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editAlignment'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editFontSize'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editAdjust'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'editLogo'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateLogosComboBox'
        QtMocHelpers::SlotData<void(const TitleBlockTemplate *)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 2 },
        }}),
        // Slot 'setReadOnly'
        QtMocHelpers::SlotData<void(bool)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TitleBlockTemplateCellWidget, qt_meta_tag_ZN28TitleBlockTemplateCellWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TitleBlockTemplateCellWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28TitleBlockTemplateCellWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28TitleBlockTemplateCellWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN28TitleBlockTemplateCellWidgetE_t>.metaTypes,
    nullptr
} };

void TitleBlockTemplateCellWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TitleBlockTemplateCellWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->logoEditionRequested(); break;
        case 1: _t->cellModified((*reinterpret_cast<std::add_pointer_t<ModifyTitleBlockCellCommand*>>(_a[1]))); break;
        case 2: _t->updateFormType((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->edit((*reinterpret_cast<std::add_pointer_t<TitleBlockCell*>>(_a[1]))); break;
        case 4: _t->editType(); break;
        case 5: _t->editName(); break;
        case 6: _t->editLabelDisplayed(); break;
        case 7: _t->editLabel(); break;
        case 8: _t->editValue(); break;
        case 9: _t->editAlignment(); break;
        case 10: _t->editFontSize(); break;
        case 11: _t->editAdjust(); break;
        case 12: _t->editLogo(); break;
        case 13: _t->updateLogosComboBox((*reinterpret_cast<std::add_pointer_t<const TitleBlockTemplate*>>(_a[1]))); break;
        case 14: _t->setReadOnly((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TitleBlockTemplateCellWidget::*)()>(_a, &TitleBlockTemplateCellWidget::logoEditionRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TitleBlockTemplateCellWidget::*)(ModifyTitleBlockCellCommand * ) const>(_a, &TitleBlockTemplateCellWidget::cellModified, 1))
            return;
    }
}

const QMetaObject *TitleBlockTemplateCellWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TitleBlockTemplateCellWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28TitleBlockTemplateCellWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TitleBlockTemplateCellWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void TitleBlockTemplateCellWidget::logoEditionRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TitleBlockTemplateCellWidget::cellModified(ModifyTitleBlockCellCommand * _t1)const
{
    QMetaObject::activate<void>(const_cast< TitleBlockTemplateCellWidget *>(this), &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
