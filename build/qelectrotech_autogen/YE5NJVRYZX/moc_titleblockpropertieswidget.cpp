/****************************************************************************
** Meta object code from reading C++ file 'titleblockpropertieswidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/titleblockpropertieswidget.h"
#include <QtGui/qtextcursor.h>
#include <QtGui/qscreen.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'titleblockpropertieswidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN26TitleBlockPropertiesWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto TitleBlockPropertiesWidget::qt_create_metaobjectdata<qt_meta_tag_ZN26TitleBlockPropertiesWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TitleBlockPropertiesWidget",
        "set_auto_page_num",
        "",
        "openAutoNumFolioEditor",
        "editCurrentTitleBlockTemplate",
        "duplicateCurrentTitleBlockTemplate",
        "updateTemplateList",
        "changeCurrentTitleBlockTemplate",
        "on_m_date_now_pb_clicked",
        "on_m_edit_autofolionum_pb_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'set_auto_page_num'
        QtMocHelpers::SignalData<void() const>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'openAutoNumFolioEditor'
        QtMocHelpers::SignalData<void(QString)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'editCurrentTitleBlockTemplate'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'duplicateCurrentTitleBlockTemplate'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateTemplateList'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'changeCurrentTitleBlockTemplate'
        QtMocHelpers::SlotData<void(int)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_m_date_now_pb_clicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_m_edit_autofolionum_pb_clicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TitleBlockPropertiesWidget, qt_meta_tag_ZN26TitleBlockPropertiesWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TitleBlockPropertiesWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN26TitleBlockPropertiesWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN26TitleBlockPropertiesWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN26TitleBlockPropertiesWidgetE_t>.metaTypes,
    nullptr
} };

void TitleBlockPropertiesWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TitleBlockPropertiesWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->set_auto_page_num(); break;
        case 1: _t->openAutoNumFolioEditor((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->editCurrentTitleBlockTemplate(); break;
        case 3: _t->duplicateCurrentTitleBlockTemplate(); break;
        case 4: _t->updateTemplateList(); break;
        case 5: _t->changeCurrentTitleBlockTemplate((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->on_m_date_now_pb_clicked(); break;
        case 7: _t->on_m_edit_autofolionum_pb_clicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TitleBlockPropertiesWidget::*)() const>(_a, &TitleBlockPropertiesWidget::set_auto_page_num, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TitleBlockPropertiesWidget::*)(QString )>(_a, &TitleBlockPropertiesWidget::openAutoNumFolioEditor, 1))
            return;
    }
}

const QMetaObject *TitleBlockPropertiesWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TitleBlockPropertiesWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN26TitleBlockPropertiesWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TitleBlockPropertiesWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void TitleBlockPropertiesWidget::set_auto_page_num()const
{
    QMetaObject::activate(const_cast< TitleBlockPropertiesWidget *>(this), &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TitleBlockPropertiesWidget::openAutoNumFolioEditor(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
