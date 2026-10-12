/****************************************************************************
** Meta object code from reading C++ file 'diagramimageitem.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/diagramimageitem.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'diagramimageitem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16DiagramImageItemE_t {};
} // unnamed namespace

template <> constexpr inline auto DiagramImageItem::qt_create_metaobjectdata<qt_meta_tag_ZN16DiagramImageItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DiagramImageItem",
        "pixmapChanged",
        "",
        "transformChanged",
        "pixmap",
        "QPixmap",
        "scaleFactorX",
        "scaleFactorY",
        "rotationAngle",
        "skewX",
        "skewY",
        "pivot",
        "QPointF",
        "rawPivot"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'pixmapChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'transformChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'pixmap'
        QtMocHelpers::PropertyData<QPixmap>(4, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 0),
        // property 'scaleFactorX'
        QtMocHelpers::PropertyData<qreal>(6, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'scaleFactorY'
        QtMocHelpers::PropertyData<qreal>(7, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'rotationAngle'
        QtMocHelpers::PropertyData<qreal>(8, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'skewX'
        QtMocHelpers::PropertyData<qreal>(9, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'skewY'
        QtMocHelpers::PropertyData<qreal>(10, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'pivot'
        QtMocHelpers::PropertyData<QPointF>(11, 0x80000000 | 12, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 1),
        // property 'rawPivot'
        QtMocHelpers::PropertyData<QPointF>(13, 0x80000000 | 12, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag, 1),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DiagramImageItem, qt_meta_tag_ZN16DiagramImageItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DiagramImageItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QetGraphicsItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16DiagramImageItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16DiagramImageItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16DiagramImageItemE_t>.metaTypes,
    nullptr
} };

void DiagramImageItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DiagramImageItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->pixmapChanged(); break;
        case 1: _t->transformChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DiagramImageItem::*)()>(_a, &DiagramImageItem::pixmapChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DiagramImageItem::*)()>(_a, &DiagramImageItem::transformChanged, 1))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QPixmap*>(_v) = _t->pixmap(); break;
        case 1: *reinterpret_cast<qreal*>(_v) = _t->scaleFactorX(); break;
        case 2: *reinterpret_cast<qreal*>(_v) = _t->scaleFactorY(); break;
        case 3: *reinterpret_cast<qreal*>(_v) = _t->rotationAngle(); break;
        case 4: *reinterpret_cast<qreal*>(_v) = _t->skewX(); break;
        case 5: *reinterpret_cast<qreal*>(_v) = _t->skewY(); break;
        case 6: *reinterpret_cast<QPointF*>(_v) = _t->pivot(); break;
        case 7: *reinterpret_cast<QPointF*>(_v) = _t->pivot(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setPixmap(*reinterpret_cast<QPixmap*>(_v)); break;
        case 1: _t->setScaleFactorX(*reinterpret_cast<qreal*>(_v)); break;
        case 2: _t->setScaleFactorY(*reinterpret_cast<qreal*>(_v)); break;
        case 3: _t->setRotationAngle(*reinterpret_cast<qreal*>(_v)); break;
        case 4: _t->setSkewX(*reinterpret_cast<qreal*>(_v)); break;
        case 5: _t->setSkewY(*reinterpret_cast<qreal*>(_v)); break;
        case 6: _t->setPivot(*reinterpret_cast<QPointF*>(_v)); break;
        case 7: _t->setPivotRaw(*reinterpret_cast<QPointF*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *DiagramImageItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DiagramImageItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16DiagramImageItemE_t>.strings))
        return static_cast<void*>(this);
    return QetGraphicsItem::qt_metacast(_clname);
}

int DiagramImageItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QetGraphicsItem::qt_metacall(_c, _id, _a);
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
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void DiagramImageItem::pixmapChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DiagramImageItem::transformChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
