/****************************************************************************
** Meta object code from reading C++ file 'projectconfigpages.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/ui/configpage/projectconfigpages.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'projectconfigpages.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN17ProjectConfigPageE_t {};
} // unnamed namespace

template <> constexpr inline auto ProjectConfigPage::qt_create_metaobjectdata<qt_meta_tag_ZN17ProjectConfigPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ProjectConfigPage"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ProjectConfigPage, qt_meta_tag_ZN17ProjectConfigPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ProjectConfigPage::staticMetaObject = { {
    QMetaObject::SuperData::link<ConfigPage::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17ProjectConfigPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17ProjectConfigPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN17ProjectConfigPageE_t>.metaTypes,
    nullptr
} };

void ProjectConfigPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ProjectConfigPage *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *ProjectConfigPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProjectConfigPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17ProjectConfigPageE_t>.strings))
        return static_cast<void*>(this);
    return ConfigPage::qt_metacast(_clname);
}

int ProjectConfigPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ConfigPage::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_tag_ZN21ProjectMainConfigPageE_t {};
} // unnamed namespace

template <> constexpr inline auto ProjectMainConfigPage::qt_create_metaobjectdata<qt_meta_tag_ZN21ProjectMainConfigPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ProjectMainConfigPage",
        "resetUsageTracker",
        ""
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'resetUsageTracker'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ProjectMainConfigPage, qt_meta_tag_ZN21ProjectMainConfigPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ProjectMainConfigPage::staticMetaObject = { {
    QMetaObject::SuperData::link<ProjectConfigPage::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21ProjectMainConfigPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21ProjectMainConfigPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN21ProjectMainConfigPageE_t>.metaTypes,
    nullptr
} };

void ProjectMainConfigPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ProjectMainConfigPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->resetUsageTracker(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *ProjectMainConfigPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProjectMainConfigPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21ProjectMainConfigPageE_t>.strings))
        return static_cast<void*>(this);
    return ProjectConfigPage::qt_metacast(_clname);
}

int ProjectMainConfigPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProjectConfigPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
namespace {
struct qt_meta_tag_ZN24ProjectAutoNumConfigPageE_t {};
} // unnamed namespace

template <> constexpr inline auto ProjectAutoNumConfigPage::qt_create_metaobjectdata<qt_meta_tag_ZN24ProjectAutoNumConfigPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ProjectAutoNumConfigPage",
        "setAutoNum",
        "",
        "saveCurrentTbp",
        "loadSavedTbp",
        "updateContextConductor",
        "saveContextConductor",
        "removeContextConductor",
        "updateContextFolio",
        "saveContextFolio",
        "removeContextFolio",
        "updateContextElement",
        "saveContextElement",
        "removeContextElement",
        "applyAutoNum",
        "applyManagement"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'setAutoNum'
        QtMocHelpers::SignalData<void(QString)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Signal 'setAutoNum'
        QtMocHelpers::SignalData<void(int, int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Signal 'saveCurrentTbp'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'loadSavedTbp'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateContextConductor'
        QtMocHelpers::SlotData<void(const QString &)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'saveContextConductor'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'removeContextConductor'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateContextFolio'
        QtMocHelpers::SlotData<void(const QString &)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'saveContextFolio'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'removeContextFolio'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateContextElement'
        QtMocHelpers::SlotData<void(const QString &)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 2 },
        }}),
        // Slot 'saveContextElement'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'removeContextElement'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'applyAutoNum'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'applyManagement'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ProjectAutoNumConfigPage, qt_meta_tag_ZN24ProjectAutoNumConfigPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ProjectAutoNumConfigPage::staticMetaObject = { {
    QMetaObject::SuperData::link<ProjectConfigPage::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN24ProjectAutoNumConfigPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN24ProjectAutoNumConfigPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN24ProjectAutoNumConfigPageE_t>.metaTypes,
    nullptr
} };

void ProjectAutoNumConfigPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ProjectAutoNumConfigPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->setAutoNum((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->setAutoNum((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->saveCurrentTbp(); break;
        case 3: _t->loadSavedTbp(); break;
        case 4: _t->updateContextConductor((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->saveContextConductor(); break;
        case 6: _t->removeContextConductor(); break;
        case 7: _t->updateContextFolio((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->saveContextFolio(); break;
        case 9: _t->removeContextFolio(); break;
        case 10: _t->updateContextElement((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->saveContextElement(); break;
        case 12: _t->removeContextElement(); break;
        case 13: _t->applyAutoNum(); break;
        case 14: _t->applyManagement(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ProjectAutoNumConfigPage::*)(QString )>(_a, &ProjectAutoNumConfigPage::setAutoNum, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectAutoNumConfigPage::*)(int , int )>(_a, &ProjectAutoNumConfigPage::setAutoNum, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectAutoNumConfigPage::*)()>(_a, &ProjectAutoNumConfigPage::saveCurrentTbp, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProjectAutoNumConfigPage::*)()>(_a, &ProjectAutoNumConfigPage::loadSavedTbp, 3))
            return;
    }
}

const QMetaObject *ProjectAutoNumConfigPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProjectAutoNumConfigPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN24ProjectAutoNumConfigPageE_t>.strings))
        return static_cast<void*>(this);
    return ProjectConfigPage::qt_metacast(_clname);
}

int ProjectAutoNumConfigPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProjectConfigPage::qt_metacall(_c, _id, _a);
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
void ProjectAutoNumConfigPage::setAutoNum(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ProjectAutoNumConfigPage::setAutoNum(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void ProjectAutoNumConfigPage::saveCurrentTbp()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ProjectAutoNumConfigPage::loadSavedTbp()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
