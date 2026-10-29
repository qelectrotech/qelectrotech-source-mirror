// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS18_RESIZE_TESTS_H
#define RS18_RESIZE_TESTS_H
#include "qetproject.h"
#include "qetapp.h"
#include "diagram.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "ui/tableformat13.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPdfWriter>

namespace Rs18Resize {
inline void mouse(Diagram *page,QetGraphicsTableItem *table,QEvent::Type type,const QPointF &position,QWidget *viewport=nullptr) {
    QGraphicsSceneMouseEvent event(type);event.setPos(position);event.setScenePos(table->mapToScene(position));event.setWidget(viewport);
    event.setButton(type==QEvent::GraphicsSceneMouseMove?Qt::NoButton:Qt::LeftButton);
    event.setButtons(type==QEvent::GraphicsSceneMouseRelease?Qt::NoButton:Qt::LeftButton);
    event.setButtonDownPos(Qt::LeftButton,position);event.setButtonDownScenePos(Qt::LeftButton,table->mapToScene(position));
    page->sendEvent(table,&event);
}
}

template<class Check> void runResize18Tests(Check check,const QString &out,const QString &,const QString &) {
    using namespace QetFormatting;
    QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
    auto *model=new ProjectDBModel(&project,&project);model->setIdentifier("nomenclature");
    model->setQuery("SELECT 1 AS 'No.', 'A long description that should wrap on several lines' AS 'Description' UNION ALL SELECT 2,'Short' UNION ALL SELECT 3,'Another description'");
    auto *table=new QetGraphicsTableItem;page->addItem(table);table->setModel(model);table->setPos(60,100);
    const QFont base("DejaVu Sans",10);model->setData(model->index(0,0),base,Qt::FontRole);
    QVariantMap format{{"enabled",true},{"rows",3},{"widths",QVariantList{60,130}},
        {"autoWidths",QVariantList{true,false}},{"wraps",QVariantList{false,true}},
        {"rowHeight",20},{"headerHeight",24},{"autoRowHeight",true},
        {"bodyStyle",QVariantMap{{"font",base.toString()},{"color","#ff000000"}}}};
    table->setCustomFormat(format);table->setSelected(true);page->undoStack().clear();
    check(table->resizeColumn18(0,95),"resize18 public column resize applies");
    check(table->customFormat().value("widths").toList()[0].toInt()==95&&!columnFlag(table->customFormat(),"autoWidths",0),"resize18 manual column width disables only its automatic sizing");
    check(!columnFlag(table->customFormat(),"autoWidths",1),"resize18 another column width mode is preserved");
    check(page->undoStack().count()==1,"resize18 public column resize is one Undo");
    page->undoStack().undo();check(table->customFormat()==format,"resize18 Undo restores the whole initial format");
    page->undoStack().redo();check(table->customFormat().value("widths").toList()[0].toInt()==95,"resize18 Redo restores manual width");
    const auto firstLayout=tableLayout(table->customFormat(),model,0,3,base);
    check(table->resizeRow18(1,53),"resize18 public row resize applies");
    const auto rows=tableLayout(table->customFormat(),model,0,3,base);
    check(rows.rowHeights[1]==53&&rows.rowHeights[0]==firstLayout.rowHeights[0]&&table->customFormat().value("autoRowHeight").toBool(),"resize18 one manual row preserves automatic sizing of other rows");
    check(rowHeightOverride18(table->customFormat(),1)==53,"resize18 manual height is stored by absolute model row");
    check(table->autoFitRow18(1)&&rowHeightOverride18(table->customFormat(),1)==fittedRowHeight18(table->customFormat(),model,1,rows.widths,base),"resize18 individual row auto-fit uses its effective font and wrapping");
    check(table->autoFitColumn18(0)&&table->customFormat().value("widths").toList()[0].toInt()==fittedColumnWidths(table->customFormat(),model,0,3,base)[0],"resize18 individual column auto-fit uses measured text");
    check(table->resizeColumn18(0,-20)&&table->customFormat().value("widths").toList()[0].toInt()==10,"resize18 column minimum width is enforced");
    check(table->resizeRow18(2,-20)&&rowHeightOverride18(table->customFormat(),2)==6,"resize18 row minimum height is enforced");
    table->resizeColumn18(0,95,false);
    QGraphicsView view(page);view.resize(700,450);view.scale(2,2);view.show();QApplication::processEvents();
    auto geometry=tableLayout(table->customFormat(),model,0,3,base);const auto initialPosition=table->pos();
    auto hover=[&](const QPointF &p){QGraphicsSceneHoverEvent event(QEvent::GraphicsSceneHoverMove);event.setPos(p);event.setScenePos(table->mapToScene(p));event.setWidget(view.viewport());page->sendEvent(table,&event);};
    hover(QPointF(geometry.widths[0]+1,-10));check(table->cursor().shape()==Qt::SplitHCursor,"resize18 column separator has resize cursor at 200 percent zoom");
    hover(QPointF(geometry.widths[0]+4,-10));check(table->cursor().shape()!=Qt::SplitHCursor,"resize18 hit radius stays in screen pixels at zoom");
    const auto beforeDrag=table->customFormat();const int beforeUndo=page->undoStack().index();
    const QPointF columnPoint(geometry.widths[0],-10);
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMousePress,columnPoint,view.viewport());
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseMove,columnPoint+QPointF(38,0),view.viewport());
    check(table->customFormat().value("widths").toList()[0].toInt()==geometry.widths[0]+38&&page->undoStack().index()==beforeUndo,"resize18 mouse drag previews width without intermediate Undo commands");
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseRelease,columnPoint+QPointF(38,0),view.viewport());
    check(page->undoStack().index()==beforeUndo+1&&table->pos()==initialPosition,"resize18 mouse drag commits one Undo and does not move the table");
    page->undoStack().undo();check(table->customFormat()==beforeDrag,"resize18 drag Undo restores width and modes");page->undoStack().redo();
    geometry=tableLayout(table->customFormat(),model,0,3,base);
    const QPointF rowPoint(20,geometry.rowHeights[0]);const int rowUndo=page->undoStack().index();
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMousePress,rowPoint,view.viewport());
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseMove,rowPoint+QPointF(0,17),view.viewport());
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseRelease,rowPoint+QPointF(0,17),view.viewport());
    check(rowHeightOverride18(table->customFormat(),0)==geometry.rowHeights[0]+17&&page->undoStack().index()==rowUndo+1,"resize18 body-row mouse drag changes only the targeted row");
    const auto firstRowDrag=table->customFormat();const int secondRowUndo=page->undoStack().index();
    geometry=tableLayout(firstRowDrag,model,0,3,base);const QPointF secondRowPoint(20,geometry.rowHeights[0]);
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMousePress,secondRowPoint,view.viewport());
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseMove,secondRowPoint+QPointF(0,11),view.viewport());
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseRelease,secondRowPoint+QPointF(0,11),view.viewport());
    const auto secondRowDrag=table->customFormat();
    check(rowHeightOverride18(secondRowDrag,0)==geometry.rowHeights[0]+11&&page->undoStack().index()==secondRowUndo+1,"resize18 consecutive row drags remain separate Undo actions");
    page->undoStack().undo();check(table->customFormat()==firstRowDrag,"resize18 one Undo reverses only the most recent row drag");
    page->undoStack().redo();check(table->customFormat()==secondRowDrag,"resize18 Redo restores only the most recent row drag");
    const auto beforeEscape=table->customFormat();const int escapeUndo=page->undoStack().index();geometry=tableLayout(beforeEscape,model,0,3,base);
    const QPointF escapePoint(geometry.widths[0],-10);
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMousePress,escapePoint,view.viewport());
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseMove,escapePoint+QPointF(20,0),view.viewport());
    QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);page->sendEvent(table,&escape);
    check(table->customFormat()==beforeEscape&&page->undoStack().index()==escapeUndo,"resize18 Escape cancels a live resize without Undo history");
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseMove,escapePoint+QPointF(50,0),view.viewport());
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseRelease,escapePoint+QPointF(50,0),view.viewport());
    check(table->pos()==initialPosition&&table->customFormat()==beforeEscape&&page->undoStack().index()==escapeUndo,"resize18 cancelled drag cannot turn into a table move before mouse release");
    geometry=tableLayout(table->customFormat(),model,0,3,base);
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseDoubleClick,QPointF(geometry.widths[0],-10),view.viewport());
    check(table->customFormat().value("widths").toList()[0].toInt()==fittedColumnWidths(table->customFormat(),model,0,3,base)[0],"resize18 double-click separator auto-fits its column");
    table->resizeColumn18(0,95,false);geometry=tableLayout(table->customFormat(),model,0,3,base);
    Rs18Resize::mouse(page,table,QEvent::GraphicsSceneMouseDoubleClick,QPointF(20,geometry.rowHeights[0]),view.viewport());
    check(rowHeightOverride18(table->customFormat(),0)==fittedRowHeight18(table->customFormat(),model,0,geometry.widths,base),"resize18 double-click body separator auto-fits its row");
    auto savedFormat=table->customFormat();project.setReadOnly(true);
    check(!table->resizeColumn18(0,200)&&!table->resizeRow18(0,200)&&table->customFormat()==savedFormat,"resize18 read-only project rejects size changes");project.setReadOnly(false);
    table->setMovable(false);check(!table->resizeColumn18(0,200),"resize18 locked table rejects size changes");table->setMovable(true);
    auto *continuation=new QetGraphicsTableItem;page->addItem(continuation);continuation->setModel(model);
    auto firstFormat=table->customFormat();firstFormat["rows"]=2;table->setCustomFormat(firstFormat);
    auto continuationFormat=firstFormat;continuationFormat["rows"]=1;continuation->setCustomFormat(continuationFormat);continuation->setPreviousTable(table);
    check(continuation->resizeRow18(2,64),"resize18 continuation row can be manually sized");
    check(tableLayout(continuation->customFormat(),model,2,1,base).rowHeights[0]==64,"resize18 continuation uses absolute model row indices");
    QVariantMap numeric=table->customFormat();bool numericSeen=false;
    QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *row=dialog->findChild<QSpinBox*>("styleRow17");auto *enabled=dialog->findChild<QCheckBox*>("rowHeightOverride18");auto *height=dialog->findChild<QSpinBox*>("individualRowHeight18");if(row&&enabled&&height){numericSeen=true;row->setValue(3);enabled->setChecked(true);height->setValue(79);dialog->accept();}else dialog->reject();});
    check(editTableFormat(nullptr,numeric,model,QVector<int>{95,130},3,base)&&numericSeen&&rowHeightOverride18(numeric,2)==79,"resize18 numeric row-height editor stores the selected model row");
    const auto numericBefore=numeric;
    QTimer::singleShot(0,[&](){if(auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget())){dialog->findChild<QSpinBox*>("styleRow17")->setValue(1);dialog->findChild<QCheckBox*>("rowHeightOverride18")->setChecked(true);dialog->findChild<QSpinBox*>("individualRowHeight18")->setValue(120);dialog->reject();}});
    check(!editTableFormat(nullptr,numeric,model,QVector<int>{95,130},3,base)&&numeric==numericBefore,"resize18 cancelling numeric row-height changes preserves the original format");
    QDomDocument xml;const auto element=table->toXml(xml);
    check(decode(element.attribute("format-v13"))==table->customFormat(),"resize18 width and per-row heights persist in table XML");
    QString error;check(project.writeContainerFile(out+"/Mouse_Resize_1.8.qetz",&error),qPrintable(error));
    {QETProject reopened(out+"/Mouse_Resize_1.8.qetz");QETApp::registerProject(&reopened);bool found=false;for(auto *p:reopened.diagrams())for(auto *item:p->items())if(auto *copy=dynamic_cast<QetGraphicsTableItem*>(item))if(copy->customFormat().value("rowHeights18").toMap()==table->customFormat().value("rowHeights18").toMap())found=true;check(found,"resize18 actual QETZ save/reopen retains manual row heights");QETApp::unregisterProject(&reopened);}
    QETApp::unregisterProject(&project);
}
#endif
