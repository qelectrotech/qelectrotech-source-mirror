// SPDX-License-Identifier: GPL-2.0-or-later
#include <QApplication>
#include <QComboBox>
#include <QTemporaryDir>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QStandardItemModel>
#include <cstdio>
#include <stdexcept>
#include <algorithm>
#include "../../sources/qetapp.h"
#include "../../sources/qetdiagrameditor.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetresult.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/projectview.h"
#include "../../sources/diagram.h"
#include "../../sources/utils/qetutils.h"
#include "../../sources/ui/diagrameditorhandlersizewidget.h"
#include "../../sources/QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "../../sources/qetgraphicsitem/independenttextitem.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"
#include "../../sources/qetgraphicsitem/diagramimageitem.h"
#include "../../sources/qetgraphicsitem/qetshapeitem.h"
#include "../../sources/editor/ui/qetelementeditor.h"
#include "../../sources/editor/elementscene.h"
#include "../../sources/editor/graphicspart/partrectangle.h"
#include "../../sources/editor/graphicspart/partdynamictextfield.h"
#include "../../sources/qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "../../sources/qetgraphicsitem/conductor.h"
static void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
static QString canonicalProjectXml(QDomDocument document) {
 // Scene index updates can reorder equal-z conductors. Compare their
 // complete serialized content in UUID order, rather than scene order.
 const auto groups=document.elementsByTagName("conductors");
 for(int i=0;i<groups.size();++i) {
  auto group=groups.at(i).toElement();QList<QDomElement> wires;
  for(auto wire=group.firstChildElement("conductor");!wire.isNull();wire=wire.nextSiblingElement("conductor"))wires<<wire;
  std::sort(wires.begin(),wires.end(),[](const QDomElement &a,const QDomElement &b){return a.attribute("uuid")<b.attribute("uuid");});
  for(auto wire:wires)group.appendChild(wire);
 }
 return document.toString();
}
static void mouse(Diagram *scene,QGraphicsItem *item,QEvent::Type type,const QPointF &point,const QPointF *press=nullptr) {
 QGraphicsSceneMouseEvent event(type);event.setButton(Qt::LeftButton);
 event.setButtons(type==QEvent::GraphicsSceneMouseRelease?Qt::NoButton:Qt::LeftButton);
 event.setScenePos(point);event.setPos(item->mapFromScene(point));
 event.setButtonDownScenePos(Qt::LeftButton,press?*press:point);scene->sendEvent(item,&event);
}
static QetGraphicsHandlerItem *bottomRightHandle(Diagram *scene) {
 QetGraphicsHandlerItem *found=nullptr;
 for(auto *item:scene->items())if(item->type()==QetGraphicsHandlerItem::Type&&item->isVisible()) {
  auto *handle=qgraphicsitem_cast<QetGraphicsHandlerItem *>(item);
  if(!found||handle->scenePos().x()+handle->scenePos().y()>found->scenePos().x()+found->scenePos().y())found=handle;
 }
 check(found,"mouse gesture handle present");return found;
}
static void drag(Diagram *scene,QGraphicsItem *item,const QPointF &end) {
 const auto start=item->scenePos();
 mouse(scene,item,QEvent::GraphicsSceneMousePress,start);
 mouse(scene,item,QEvent::GraphicsSceneMouseMove,end,&start);
 mouse(scene,item,QEvent::GraphicsSceneMouseRelease,end,&start);
}
int main(int argc,char **argv) {
 const QString fixture=QString::fromLocal8Bit(argv[1]);
 const QString report=QString::fromLocal8Bit(argv[2]);
 QTemporaryDir isolated;
 QByteArray config="--config-dir="+isolated.path().toUtf8();
 QByteArray data="--data-dir="+isolated.path().toUtf8();
 QByteArray collection="--common-elements-dir="+isolated.path().toUtf8();
 char *args[]={argv[0],config.data(),data.data(),collection.data()};int count=4;
 QApplication app(count,args);
 std::freopen(report.toLocal8Bit().constData(),"w",stdout);
 std::setbuf(stdout,nullptr);
 QSettings::setDefaultFormat(QSettings::IniFormat);
 QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,isolated.path());
 QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 try {
  auto *qet=new QETApp; Q_UNUSED(qet);
  auto editors=QETApp::diagramEditors();check(!editors.isEmpty(),"editor created");
  auto *editor=editors.first();
  auto *project=new QETProject(fixture);check(project->state()==QETProject::Ok,"fixture loaded");
  editor->addProjectView(new ProjectView(project));
  auto *scene=project->diagrams().first();
  auto widgets=editor->findChildren<DiagramEditorHandlerSizeWidget *>();check(!widgets.isEmpty(),"toolbar widget present");
  auto *combo=widgets.first()->findChild<QComboBox *>();check(combo&&combo->count()==6,"six choices");
  check(combo->currentIndex()==3&&editor->property("graphics_handler_size").toReal()==10,"default x1");
  auto *free=new IndependentTextItem;free->setPlainText("Mouse placement");scene->addItem(free);free->setSelected(true);
  QPixmap pixmap(30,20);pixmap.fill(Qt::red);auto *image=new DiagramImageItem(pixmap);scene->addItem(image);image->setSelected(true);
  auto *shape=new QetShapeItem({100,100},{150,140},QetShapeItem::Rectangle);scene->addItem(shape);shape->setSelected(true);
  auto *attached=scene->elements().first()->dynamicTextItems().first();attached->setSelected(true);
  auto *background=project->addNewDiagram();
  auto *backgroundHandle=new QetGraphicsHandlerItem;
  background->addItem(backgroundHandle);
  QApplication::processEvents();
  const QString xml=project->toXml().toString();
  const int undoCount=scene->undoStack().count();const int undoIndex=scene->undoStack().index();
  const bool dirty=project->projectWasModified();
  const qreal sizes[]={2.5,5,7.5,10,20,30};
  for(int i=0;i<6;++i) {
   combo->setCurrentIndex(i);QApplication::processEvents();
   check(editor->property("graphics_handler_size").toReal()==sizes[i],"fractional editor property");
   check(QETUtils::graphicsHandlerSize(image)==sizes[i],"new handle size lookup");
   check(backgroundHandle->property("currentSize").toReal()==sizes[i],"inactive folio handles updated");
   int handles=0;for(auto *item:scene->items())if(item->type()==QetGraphicsHandlerItem::Type){
    ++handles;auto *handle=qgraphicsitem_cast<QetGraphicsHandlerItem *>(item);
    check(handle->property("currentSize").toReal()==sizes[i],"existing handles updated immediately");
   }
   check(handles>=8,"multiple selected object handles tested");
   image->setSelected(false);image->setSelected(true);
   for(auto *item:scene->items())if(item->type()==QetGraphicsHandlerItem::Type)
    check(qgraphicsitem_cast<QetGraphicsHandlerItem *>(item)->property("currentSize").toReal()==sizes[i],"recreated handles retain fractions");
   DiagramEditorHandlerSizeWidget recreated(editor);
   check(recreated.findChild<QComboBox *>()->currentIndex()==i,"toolbar recreation preserves chosen size");
   check(project->toXml().toString()==xml,"handle size leaves project XML unchanged");
   check(scene->undoStack().count()==undoCount&&scene->undoStack().index()==undoIndex,"handle size leaves undo stack unchanged");
   check(project->projectWasModified()==dirty,"handle size leaves dirty state unchanged");
  }
  for(int i=0;i<6;++i) {
   combo->setCurrentIndex(i);
   scene->clearSelection();
   auto *picture=new DiagramImageItem(pixmap);scene->addItem(picture);picture->setPos(400,400);picture->setSelected(true);
   auto *handle=bottomRightHandle(scene);
   check(handle->property("currentSize").toReal()==sizes[i],"image mouse handle size");
   drag(scene,handle,handle->scenePos()+QPointF(20,20));
   check(picture->scaleFactorX()!=1,"image resized by mouse events");
   const qreal changed=picture->scaleFactorX();scene->undoStack().undo();
   check(picture->scaleFactorX()==1,"image resize undo");scene->undoStack().redo();
   check(picture->scaleFactorX()==changed,"image resize redo");scene->undoStack().undo();
   const auto centre=picture->mapToScene(picture->imageRect().center());
   mouse(scene,picture,QEvent::GraphicsSceneMousePress,centre);
   mouse(scene,picture,QEvent::GraphicsSceneMouseRelease,centre);
   handle=bottomRightHandle(scene);
   const auto pivot=picture->mapToScene(picture->pivot());QTransform turn;turn.rotate(30);
   drag(scene,handle,pivot+turn.map(handle->scenePos()-pivot));
   check(qAbs(picture->rotationAngle())>1,"image rotated by mouse events");
   const qreal angle=picture->rotationAngle();scene->undoStack().undo();
   check(qAbs(picture->rotationAngle())<0.001,"image rotation undo");scene->undoStack().redo();
   check(qAbs(picture->rotationAngle()-angle)<0.001,"image rotation redo");scene->undoStack().undo();
   scene->undoStack().clear();delete picture;
   auto *rectangle=new QetShapeItem({0,0},{50,40},QetShapeItem::Rectangle);scene->addItem(rectangle);rectangle->setPos(400,400);rectangle->setSelected(true);
   const auto rect=rectangle->rect();handle=bottomRightHandle(scene);
   drag(scene,handle,handle->scenePos()+QPointF(20,20));check(rectangle->rect()!=rect,"shape resized by mouse events");
   const auto resized=rectangle->rect();scene->undoStack().undo();check(rectangle->rect()==rect,"shape resize undo");
   scene->undoStack().redo();check(rectangle->rect()==resized,"shape resize redo");scene->undoStack().undo();
   for(int click=0;click<2;++click) {
    const auto middle=rectangle->mapToScene(rect.center());
    mouse(scene,rectangle,QEvent::GraphicsSceneMousePress,middle);mouse(scene,rectangle,QEvent::GraphicsSceneMouseRelease,middle);
   }
   handle=bottomRightHandle(scene);const auto shapePivot=rectangle->mapToScene(rectangle->pivot());
   drag(scene,handle,shapePivot+turn.map(handle->scenePos()-shapePivot));
   check(qAbs(rectangle->rotation())>1,"shape rotated by mouse events");
   const qreal shapeAngle=rectangle->rotation();scene->undoStack().undo();check(qAbs(rectangle->rotation())<0.001,"shape rotation undo");
   scene->undoStack().redo();check(qAbs(rectangle->rotation()-shapeAngle)<0.001,"shape rotation redo");scene->undoStack().undo();
   scene->undoStack().clear();
   auto *companion=new DiagramImageItem(pixmap);scene->addItem(companion);companion->setPos(600,400);companion->setSelected(true);
   rectangle->setSelected(true);
   const auto rectanglePos=rectangle->pos();const auto companionPos=companion->pos();
   const auto start=companion->mapToScene(companion->imageRect().center());const auto end=start+QPointF(20,20);
   mouse(scene,companion,QEvent::GraphicsSceneMousePress,start);
   mouse(scene,companion,QEvent::GraphicsSceneMouseMove,end,&start);
   mouse(scene,companion,QEvent::GraphicsSceneMouseRelease,end,&start);
   check(companion->pos()==companionPos+QPointF(20,20)&&rectangle->pos()==rectanglePos+QPointF(20,20),"multiple objects moved by mouse events");
   scene->undoStack().undo();check(companion->pos()==companionPos&&rectangle->pos()==rectanglePos,"multiple object movement undo");
   scene->undoStack().redo();check(companion->pos()==companionPos+QPointF(20,20)&&rectangle->pos()==rectanglePos+QPointF(20,20),"multiple object movement redo");
   scene->undoStack().undo();scene->undoStack().clear();delete companion;delete rectangle;
  }
  std::puts("PASS: image and rectangle mouse resize/rotation, multiple object movement, and undo/redo at all six sizes");
  project->setFilePath(QString::fromLocal8Bit(argv[3])+"/handles.qet");
  check(project->write().isOk(),"save after handle size changes");
  QETProject reopened(project->filePath());check(reopened.state()==QETProject::Ok,"reopen after handle size changes");
  bool imageFound=false,textFound=false;
  for(auto *item:reopened.diagrams().first()->items()) {
   if(auto *loaded=qgraphicsitem_cast<DiagramImageItem *>(item))imageFound|=loaded->pixmap().size()==pixmap.size();
   if(auto *loaded=qgraphicsitem_cast<IndependentTextItem *>(item))textFound|=loaded->toPlainText()==free->toPlainText();
  }
  check(imageFound&&textFound,"object content survives reopen");
  auto *elementEditor=new QETElementEditor;elementEditor->show();
  auto *elementScene=elementEditor->elementScene();
  auto *part=new PartRectangle(elementEditor);part->setRect({0,0,50,40});elementScene->addItems({part});part->setSelected(true);
  QApplication::processEvents();
  auto checkElementHandles=[&] {
   int count=0;
   for(auto *item:elementScene->items())if(item->type()==QetGraphicsHandlerItem::Type&&item->isVisible()) {
    ++count;check(qgraphicsitem_cast<QetGraphicsHandlerItem *>(item)->property("currentSize").toReal()==10,"element editor retains default handle size");
   }
   check(count>=4,"element editor shared handles present");
  };
  for(int i=0;i<6;++i){combo->setCurrentIndex(i);checkElementHandles();check(part->rect()==QRectF(0,0,50,40),"element primitive geometry unchanged");}
  elementScene->clearSelection();
  auto *field=new PartDynamicTextField(elementEditor);field->setPlainText("Element field");elementScene->addItems({field});field->setSelected(true);
  QApplication::processEvents();checkElementHandles();
  std::puts("PASS: element editor rectangle and dynamic field keep their default shared handles");
  QStandardItemModel model(2,2);model.setData(model.index(0,0),"Table");
  auto *table=new QetGraphicsTableItem;table->setModel(&model);scene->addItem(table);
  const auto tableSize=table->size();
  for(int i=0;i<6;++i) {
   combo->setCurrentIndex(i);
   QGraphicsSceneHoverEvent enter(QEvent::GraphicsSceneHoverEnter);scene->sendEvent(table,&enter);
   check(bottomRightHandle(scene)->property("currentSize").toReal()==sizes[i],"table hover handle retains fraction");
   check(table->size()==tableSize,"table dimensions unchanged");
   QGraphicsSceneHoverEvent leave(QEvent::GraphicsSceneHoverLeave);scene->sendEvent(table,&leave);
  }
  delete table;std::puts("PASS: table hover handles retain all six sizes without changing the table");
  const auto wireFixture=QFileInfo(fixture).dir().absoluteFilePath("../../../examples/tremie_vibrante.qet");
  auto *wireProject=new QETProject(wireFixture);check(wireProject->state()==QETProject::Ok,"conductor fixture opens");
  editor->addProjectView(new ProjectView(wireProject));
  Diagram *wireScene=nullptr;for(auto *diagram:wireProject->diagrams())if(!diagram->conductors().isEmpty()){wireScene=diagram;break;}
  check(wireScene,"conductor scene present");auto *wire=wireScene->conductors().first();
  wire->setSelected(true);QApplication::processEvents();
  const auto wireXml=canonicalProjectXml(wireProject->toXml());
  for(int i=0;i<6;++i) {
   combo->setCurrentIndex(i);wire->setSelected(false);wire->setSelected(true);
   int count=0;for(auto *item:wireScene->items())if(item->type()==QetGraphicsHandlerItem::Type) {
    ++count;check(qgraphicsitem_cast<QetGraphicsHandlerItem *>(item)->property("currentSize").toReal()==sizes[i],"conductor handles retain fractions");
   }
   check(count>0,"conductor handles present");
   const auto after=canonicalProjectXml(wireProject->toXml());
   check(after==wireXml,"conductor project content unchanged");
  }
  std::puts("PASS: conductor handles retain all six sizes without changing their project");
  std::puts("PASS: six toolbar choices, default, fractions, existing/recreated handles, multi-selection, toolbar recreation, unchanged XML/undo/dirty state");
 }catch(const std::exception &error){std::printf("FAIL: %s\n",error.what());std::fflush(stdout);std::_Exit(1);}
 std::fflush(stdout);std::_Exit(0);
}
