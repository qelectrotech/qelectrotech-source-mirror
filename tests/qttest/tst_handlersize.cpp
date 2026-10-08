// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"

class tst_handlersize : public QObject
{
 Q_OBJECT
private slots:
 void sizeChangeCancelsOldHoverAnimation() {
  QGraphicsScene scene;
  auto *handle=new QetGraphicsHandlerItem(30);scene.addItem(handle);
  QGraphicsSceneHoverEvent event(QEvent::GraphicsSceneHoverEnter);
  scene.sendEvent(handle,&event);
  handle->setSize(2.5);
  QTest::qWait(250);
  QCOMPARE(handle->property("currentSize").toReal(),qreal(2.5));
 }
 void fractionalGeometryAndPicking() {
  for (qreal size : {2.5, 5.0, 7.5, 10.0, 20.0, 30.0}) {
   QGraphicsScene scene;
   auto *object = scene.addRect(QRectF(0,0,100,60));
   object->setPos(80,40); object->setRotation(37); object->setSelected(true);
   const QRectF geometry=object->rect(); const auto transform=object->sceneTransform();
   auto handles=QetGraphicsHandlerItem::handlerForPoint({QPointF(120,80)},size);
   auto *handle=handles.first(); scene.addItem(handle); handle->setColor(Qt::darkGreen);
   QCOMPARE(handle->property("currentSize").toReal(),size);
   QCOMPARE(handle->boundingRect().width(),size+2);
   const auto position=handle->pos();
   for (qreal next : {2.5,5.0,7.5,10.0,20.0,30.0}) {
    handle->setSize(next);
    QCOMPARE(handle->property("currentSize").toReal(),next);
    QCOMPARE(handle->pos(),position); QVERIFY(handle->isVisible());
    QCOMPARE(object->rect(),geometry); QCOMPARE(object->sceneTransform(),transform);
    for (qreal zoom : {0.25,1.0,4.0}) {
     QGraphicsView view(&scene); view.scale(zoom,zoom);
     const auto device=handle->deviceTransform(view.viewportTransform());
     QCOMPARE(device.mapRect(handle->boundingRect()).width(),next+2);
     const QPointF centre=handle->scenePos();
     QVERIFY(scene.items(centre,Qt::IntersectsItemShape,Qt::DescendingOrder,view.viewportTransform()).contains(handle));
     const QPointF outside=centre+QPointF((next/2+2)/zoom,0);
     QVERIFY(!scene.items(outside,Qt::IntersectsItemShape,Qt::DescendingOrder,view.viewportTransform()).contains(handle));
    }
   }
   handle->hide(); QVERIFY(!scene.items(position).contains(handle));
  }
 }
};
QTEST_MAIN(tst_handlersize)
#include "tst_handlersize.moc"
