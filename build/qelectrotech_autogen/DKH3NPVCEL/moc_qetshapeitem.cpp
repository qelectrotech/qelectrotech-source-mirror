/****************************************************************************
** Meta object code from reading C++ file 'qetshapeitem.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../sources/qetgraphicsitem/qetshapeitem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qetshapeitem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12QetShapeItemE_t {};
} // unnamed namespace

template <> constexpr inline auto QetShapeItem::qt_create_metaobjectdata<qt_meta_tag_ZN12QetShapeItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QetShapeItem",
        "penChanged",
        "",
        "brushChanged",
        "closeChanged",
        "XRadiusChanged",
        "YRadiusChanged",
        "transformChanged",
        "arcChanged",
        "geometryChanged",
        "pen",
        "QPen",
        "brush",
        "QBrush",
        "rect",
        "QRectF",
        "line",
        "QLineF",
        "polygon",
        "QPolygonF",
        "close",
        "xRadius",
        "yRadius",
        "rotation",
        "skewX",
        "skewY",
        "scaleFactorX",
        "scaleFactorY",
        "pivot",
        "QPointF",
        "startAngle",
        "endAngle",
        "ShapeType",
        "Line",
        "Rectangle",
        "Ellipse",
        "Polygon",
        "Path",
        "ArcClosure",
        "NoClosure",
        "Chord",
        "Pie"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'penChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'brushChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'closeChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'XRadiusChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'YRadiusChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'transformChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'arcChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'geometryChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'pen'
        QtMocHelpers::PropertyData<QPen>(10, 0x80000000 | 11, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 0),
        // property 'brush'
        QtMocHelpers::PropertyData<QBrush>(12, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 1),
        // property 'rect'
        QtMocHelpers::PropertyData<QRectF>(14, 0x80000000 | 15, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
        // property 'line'
        QtMocHelpers::PropertyData<QLineF>(16, 0x80000000 | 17, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
        // property 'polygon'
        QtMocHelpers::PropertyData<QPolygonF>(18, 0x80000000 | 19, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet),
        // property 'close'
        QtMocHelpers::PropertyData<bool>(20, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable, 2),
        // property 'xRadius'
        QtMocHelpers::PropertyData<qreal>(21, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'yRadius'
        QtMocHelpers::PropertyData<qreal>(22, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'rotation'
        QtMocHelpers::PropertyData<qreal>(23, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'skewX'
        QtMocHelpers::PropertyData<qreal>(24, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'skewY'
        QtMocHelpers::PropertyData<qreal>(25, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'scaleFactorX'
        QtMocHelpers::PropertyData<qreal>(26, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'scaleFactorY'
        QtMocHelpers::PropertyData<qreal>(27, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'pivot'
        QtMocHelpers::PropertyData<QPointF>(28, 0x80000000 | 29, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 5),
        // property 'startAngle'
        QtMocHelpers::PropertyData<qreal>(30, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
        // property 'endAngle'
        QtMocHelpers::PropertyData<qreal>(31, QMetaType::QReal, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'ShapeType'
        QtMocHelpers::EnumData<enum ShapeType>(32, 32, QMC::EnumFlags{}).add({
            {   33, ShapeType::Line },
            {   34, ShapeType::Rectangle },
            {   35, ShapeType::Ellipse },
            {   36, ShapeType::Polygon },
            {   37, ShapeType::Path },
        }),
        // enum 'ArcClosure'
        QtMocHelpers::EnumData<enum ArcClosure>(38, 38, QMC::EnumFlags{}).add({
            {   39, ArcClosure::NoClosure },
            {   40, ArcClosure::Chord },
            {   41, ArcClosure::Pie },
        }),
    };
    return QtMocHelpers::metaObjectData<QetShapeItem, qt_meta_tag_ZN12QetShapeItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QetShapeItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QetGraphicsItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12QetShapeItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12QetShapeItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12QetShapeItemE_t>.metaTypes,
    nullptr
} };

void QetShapeItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QetShapeItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->penChanged(); break;
        case 1: _t->brushChanged(); break;
        case 2: _t->closeChanged(); break;
        case 3: _t->XRadiusChanged(); break;
        case 4: _t->YRadiusChanged(); break;
        case 5: _t->transformChanged(); break;
        case 6: _t->arcChanged(); break;
        case 7: _t->geometryChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::penChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::brushChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::closeChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::XRadiusChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::YRadiusChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::transformChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::arcChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (QetShapeItem::*)()>(_a, &QetShapeItem::geometryChanged, 7))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QPen*>(_v) = _t->pen(); break;
        case 1: *reinterpret_cast<QBrush*>(_v) = _t->brush(); break;
        case 2: *reinterpret_cast<QRectF*>(_v) = _t->rect(); break;
        case 3: *reinterpret_cast<QLineF*>(_v) = _t->line(); break;
        case 4: *reinterpret_cast<QPolygonF*>(_v) = _t->polygon(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->isClosed(); break;
        case 6: *reinterpret_cast<qreal*>(_v) = _t->XRadius(); break;
        case 7: *reinterpret_cast<qreal*>(_v) = _t->YRadius(); break;
        case 8: *reinterpret_cast<qreal*>(_v) = _t->rotation(); break;
        case 9: *reinterpret_cast<qreal*>(_v) = _t->skewX(); break;
        case 10: *reinterpret_cast<qreal*>(_v) = _t->skewY(); break;
        case 11: *reinterpret_cast<qreal*>(_v) = _t->scaleFactorX(); break;
        case 12: *reinterpret_cast<qreal*>(_v) = _t->scaleFactorY(); break;
        case 13: *reinterpret_cast<QPointF*>(_v) = _t->pivot(); break;
        case 14: *reinterpret_cast<qreal*>(_v) = _t->startAngle(); break;
        case 15: *reinterpret_cast<qreal*>(_v) = _t->endAngle(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setPen(*reinterpret_cast<QPen*>(_v)); break;
        case 1: _t->setBrush(*reinterpret_cast<QBrush*>(_v)); break;
        case 2: _t->setRect(*reinterpret_cast<QRectF*>(_v)); break;
        case 3: _t->setLine(*reinterpret_cast<QLineF*>(_v)); break;
        case 4: _t->setPolygon(*reinterpret_cast<QPolygonF*>(_v)); break;
        case 5: _t->setClosed(*reinterpret_cast<bool*>(_v)); break;
        case 6: _t->setXRadius(*reinterpret_cast<qreal*>(_v)); break;
        case 7: _t->setYRadius(*reinterpret_cast<qreal*>(_v)); break;
        case 8: _t->setRotation(*reinterpret_cast<qreal*>(_v)); break;
        case 9: _t->setSkewX(*reinterpret_cast<qreal*>(_v)); break;
        case 10: _t->setSkewY(*reinterpret_cast<qreal*>(_v)); break;
        case 11: _t->setScaleFactorX(*reinterpret_cast<qreal*>(_v)); break;
        case 12: _t->setScaleFactorY(*reinterpret_cast<qreal*>(_v)); break;
        case 13: _t->setPivot(*reinterpret_cast<QPointF*>(_v)); break;
        case 14: _t->setStartAngle(*reinterpret_cast<qreal*>(_v)); break;
        case 15: _t->setEndAngle(*reinterpret_cast<qreal*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *QetShapeItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QetShapeItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12QetShapeItemE_t>.strings))
        return static_cast<void*>(this);
    return QetGraphicsItem::qt_metacast(_clname);
}

int QetShapeItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QetGraphicsItem::qt_metacall(_c, _id, _a);
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
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void QetShapeItem::penChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void QetShapeItem::brushChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void QetShapeItem::closeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void QetShapeItem::XRadiusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void QetShapeItem::YRadiusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void QetShapeItem::transformChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void QetShapeItem::arcChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void QetShapeItem::geometryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
