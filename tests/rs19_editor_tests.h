// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS19_EDITOR_TESTS_H
#define RS19_EDITOR_TESTS_H
#include "ui/tableeditor15.h"
#include "ui/tablefields18.h"
#include "qetapp.h"
#include <QPdfWriter>

namespace Rs19Editor {
inline void waitForPreview() {QEventLoop loop;QTimer::singleShot(240,&loop,&QEventLoop::quit);loop.exec();}
inline QetGraphicsTableItem *preview(QetTableEditor15::Dialog &dialog) {
 auto *view=dialog.findChild<QGraphicsView*>("tablePreview15");
 if(view&&view->scene())for(auto *item:view->scene()->items())if(auto *table=dynamic_cast<QetGraphicsTableItem*>(item))return table;
 return nullptr;
}
template<class Check>
void datePresentation(Check check,QETProject &project,Diagram *page,const QString &out) {
 const QString iso="2026-10-28";
 auto expect=[&](const QString &sql,const QString &wanted,const char *message){
  ProjectDBModel model(&project);model.setQuery(sql);
  check(model.rowCount()==1&&model.columnCount()==1&&model.index(0,0).data().toString()==wanted,message);
 };
 const QString formatted="SELECT strftime('%Y-%m-%d', date) AS Date FROM project_summary_view ORDER BY pos";
 expect(formatted,iso,"editor19 explicit SQL date formatting preserves the requested ISO spelling");
 expect("SELECT '2026-10-28' AS Date",iso,"editor19 a fixed text value that looks like a date remains literal text");
 expect("WITH articles AS (SELECT '2026-10-28' AS article) SELECT article FROM articles",iso,"editor19 a bare article code that looks like a date is not locale-formatted");
 expect("WITH dates AS (SELECT strftime('%Y-%m-%d',date) AS Date FROM project_summary_view) SELECT Date FROM dates",iso,"editor19 a computed date behind an opaque CTE keeps its SQL formatting");
 expect("WITH dates AS (SELECT strftime('%Y-%m-%d',date) AS Date FROM project_summary_view), projected AS (SELECT Date AS Shown FROM dates) SELECT Shown FROM projected",iso,"editor19 multiple opaque CTE levels preserve a computed date's spelling");
 expect("SELECT x.Date FROM project_summary_view AS s JOIN (SELECT strftime('%Y-%m-%d','2026-10-28') AS Date) AS x ON 1=1",iso,"editor19 a joined computed Date is not mistaken for the legacy summary date");
 const auto projected=QetTableQuery15::withState(QetTableFields18::projectOutputs18(formatted,{"\"Date\" AS \"Shown\""}),{{"sourceSql18",formatted}});
 expect(projected,iso,"editor19 known source projection preserves explicit upstream date formatting");
 const auto projectedAgain=QetTableQuery15::withState(QetTableFields18::projectOutputs18(projected,{"\"Shown\" AS \"Again\""}),{{"sourceSql18",projected}});
 expect(projectedAgain,iso,"editor19 nested known source projections retain formatted dates");
 const QString legacy=QLocale::system().toString(QDate(2026,10,28),QLocale::ShortFormat);
 expect("SELECT date FROM project_summary_view",legacy,"editor19 the legacy direct summary date retains its system-locale presentation");
 expect("SELECT s.date AS Date FROM project_summary_view AS s",legacy,"editor19 a qualified direct summary date retains legacy locale presentation");
 auto *formattedModel=new ProjectDBModel(&project,&project);formattedModel->setIdentifier("summary");formattedModel->setQuery(formatted);
 ProjectDBModel copied(*formattedModel);check(copied.index(0,0).data().toString()==iso,"editor19 copying a model preserves its explicit formatted date");
 QDomDocument document;document.appendChild(formattedModel->toXml(document));QFile xml(out+"/Formatted_Date_Model_1.9.xml");
 check(xml.open(QIODevice::WriteOnly)&&xml.write(document.toByteArray())>0,"editor19 saves a formatted-date model XML fixture");xml.close();check(xml.open(QIODevice::ReadOnly),"editor19 reopens its formatted-date XML fixture");QDomDocument reopened;
 check(reopened.setContent(xml.readAll()),"editor19 formatted-date model XML parses after save");xml.close();ProjectDBModel restored(&project);restored.fromXml(reopened.documentElement());
 check(restored.index(0,0).data().toString()==iso&&restored.queryString()==formatted,"editor19 model XML roundtrip preserves the date formula and its requested display");
 auto *savedTable=new QetGraphicsTableItem;page->addItem(savedTable);savedTable->setModel(formattedModel);savedTable->setTableName("Formatted ISO date");savedTable->setPos(65,85);
 QString error;const QString path=out+"/Formatted_Date_Presentation_1.9.qetz";check(project.writeContainerFile(path,&error),qPrintable(error));
 {
  QETProject saved(path);QETApp::registerProject(&saved);check(saved.state()==QETProject::Ok,"editor19 the actual project containing a formatted-date table reopens");bool kept=false;
  for(auto *folio:saved.diagrams())for(auto *item:folio->items())if(auto *table=dynamic_cast<QetGraphicsTableItem*>(item))if(table->tableName()=="Formatted ISO date")kept=table->model()->index(0,0).data().toString()==iso;
  check(kept,"editor19 a QETZ project roundtrip preserves the explicitly formatted date table");QETApp::unregisterProject(&saved);
 }
}
}
template<class Check>
void runEditor19Tests(Check check,const QString &out,const QString &,const QString &) {
 QSettings().remove("table15/defaults");
 QETProject project;QETApp::registerProject(&project);
 auto *page=project.addNewDiagram();auto title=page->border_and_titleblock.exportTitleBlock();title.title="Editor live-preview example";title.folio="A";title.date=QDate(2026,10,28);title.useDate=TitleBlockProperties::UseDateValue;page->border_and_titleblock.importTitleBlock(title);
 // Headless fixtures have no ProjectView editing path. Rebuild from the
 // actual folio properties before creating the live summary model.
 project.dataBase()->updateDB();
 Rs19Editor::datePresentation(check,project,page,out);
 SummaryQueryWidget definition;definition.setQuery(QetTableFields18::defaultQuery18("summary"));
 auto *model=new ProjectDBModel(&project,&project);model->setIdentifier("summary");model->setQuery(definition.queryStr());
 auto *table=new QetGraphicsTableItem;page->addItem(table);table->setModel(model);
 const QVariantMap dateStyle{{"bodyStyle",QVariantMap{{"font",QFont("DejaVu Sans",13,QFont::Bold).toString()},{"color","#ff244876"}}}};
 const QVariantMap initialFormat{{"enabled",true},{"widths",QVariantList{55,310,400,123}},
  {"headers",QVariantList{"Page","Page content","Comments","Date"}},
  {"columnOverrides17",QVariantList{QVariantMap(),QVariantMap(),QVariantMap(),dateStyle}},
  {"wraps",QVariantList{false,true,true,false}},{"bodyStyle",QVariantMap{{"font",QFont("DejaVu Sans",8).toString()}}}};
 table->setCustomFormat(initialFormat);const auto originalQuery=model->queryString();const auto originalIndex=page->undoStack().index();
 {
  QetTableEditor15::Dialog dialog(table);auto *widget=dialog.findChild<SummaryQueryWidget*>("tableDefinition15");auto *timer=dialog.findChild<QTimer*>("tableLivePreviewTimer19");
  auto *status=dialog.findChild<QLabel*>("tableStatus15");auto *preview=Rs19Editor::preview(dialog);
  check(widget&&timer&&status&&preview,"editor19 exposes a live-preview timer, graphical field widget and preview table");
  check(timer->isSingleShot()&&timer->interval()>0,"editor19 coalesces field edits with a non-blocking preview timer");
  check(model->queryString()==originalQuery&&table->customFormat()==initialFormat,"editor19 opening Properties keeps the original table intact");
  widget->setDateFormat19("%Y-%m-%d");
  check(timer->isActive(),"editor19 a graphical date-format change schedules live preview");
  Rs19Editor::waitForPreview();
  auto *draft=qobject_cast<ProjectDBModel*>(preview->model());
  if(!draft||draft->rowCount()!=1||draft->index(0,3).data().toString()!="2026-10-28")qWarning()<<"editor19 date preview diagnostic:"<<(draft?draft->rowCount():-1)<<(draft?draft->columnCount():-1)<<(draft?draft->index(0,3).data():QVariant())<<status->text();
  check(draft&&draft->rowCount()==1&&draft->index(0,3).data().toString()=="2026-10-28","editor19 live preview reflects date formatting without switching tabs or pressing Apply");
  check(dialog.format().value("widths").toList().value(3).toInt()==123&&dialog.format().value("columnOverrides17").toList().value(3).toMap()==dateStyle,"editor19 date-format change retains column width and font/colour overrides by field identity");
  check(model->queryString()==originalQuery&&table->customFormat()==initialFormat&&page->undoStack().index()==originalIndex,"editor19 automatic preview changes neither document nor Undo history");
  auto *selected=widget->findChild<QListWidget*>("m_choosen_list");check(selected&&selected->count()==4,"editor19 reopens all four existing selected fields");
  auto *date=selected->item(3);const auto expression=QetTableQuery15::baseExpression(date->data(Qt::UserRole).toString());
  date->setData(Qt::UserRole,expression+" AS \"Issued on\"");date->setText("Issued on");
  check(QMetaObject::invokeMethod(widget,"definitionChanged19",Qt::DirectConnection),"editor19 accepts a graphical heading edit notification");Rs19Editor::waitForPreview();
  check(draft->headerData(3,Qt::Horizontal).toString()=="Issued on"&&dialog.format().value("widths").toList().value(3).toInt()==123,"editor19 renaming a date heading preserves its width");
  check(dialog.format().value("columnOverrides17").toList().value(3).toMap()==dateStyle,"editor19 renaming a heading preserves its per-column text style");
  check(dialog.format().value("headers").toList().value(3).toString()=="Issued on","editor19 a stored automatic caption follows the edited SQL heading");
  check(QetFormatting::columnHeading(preview->customFormat(),draft,3)=="Issued on","editor19 the actual preview paint caption uses the edited heading rather than a stale stored Date caption");
  {
   QPdfWriter pdf(out+"/Table_Header_Rename_1.9.pdf");pdf.setResolution(96);pdf.setPageSize(QPageSize(QPageSize::A4));pdf.setPageOrientation(QPageLayout::Landscape);
   QPainter painter(&pdf);preview->scene()->render(&painter,QRectF(0,0,painter.viewport().width(),painter.viewport().height()),preview->sceneBoundingRect(),Qt::KeepAspectRatio);
  }
  selected->insertItem(0,selected->takeItem(3));QMetaObject::invokeMethod(widget,"definitionChanged19",Qt::DirectConnection);Rs19Editor::waitForPreview();
  check(draft->headerData(0,Qt::Horizontal).toString()=="Issued on"&&dialog.format().value("widths").toList().value(0).toInt()==123,"editor19 reordering a field moves its width with the field");
  check(dialog.format().value("columnOverrides17").toList().value(0).toMap()==dateStyle,"editor19 reordering a field moves its per-column font and colour with it");
  check(dialog.format().value("headers").toList().value(0).toString()=="Issued on"&&QetFormatting::columnHeading(preview->customFormat(),draft,0)=="Issued on","editor19 reordering retains the edited rendered header caption with its field");
  const QString lastValid=draft->queryString();const int lastColumns=draft->columnCount();
  while(selected->count()){selected->setCurrentRow(0);check(QMetaObject::invokeMethod(widget,"on_m_remove_pb_clicked",Qt::DirectConnection),"editor19 removes a selected column through the production control");}
  Rs19Editor::waitForPreview();
  check(!widget->validationMessage19().isEmpty()&&!status->text().contains("SELECT FROM"),"editor19 empty selection shows a friendly validation message instead of raw invalid SQL");
  check(draft->queryString()==lastValid&&draft->columnCount()==lastColumns,"editor19 an invalid current selection preserves the last valid preview");
  check(!dialog.refreshPreview(),"editor19 empty field selection cannot be applied");
  check(model->queryString()==originalQuery&&table->customFormat()==initialFormat,"editor19 invalid edits leave the production table unchanged");
  dialog.resize(1080,820);dialog.grab().save(out+"/table-empty-selection-validation19.png");dialog.reject();
 }
 check(model->queryString()==originalQuery&&table->customFormat()==initialFormat&&page->undoStack().index()==originalIndex,"editor19 Cancel discards every unapplied draft and formatting change");
 auto explicitFormat=initialFormat;explicitFormat["headers"]=QVariantList{"Page","Page content","Comments","Customer date"};table->setCustomFormat(explicitFormat);
 {
  QetTableEditor15::Dialog dialog(table);auto *widget=dialog.findChild<SummaryQueryWidget*>("tableDefinition15");auto *selected=widget?widget->findChild<QListWidget*>("m_choosen_list"):nullptr;
  check(selected&&selected->count()==4,"editor19 opens the explicitly customised header fixture");
  auto *date=selected->item(3);date->setData(Qt::UserRole,QetTableQuery15::baseExpression(date->data(Qt::UserRole).toString())+" AS \"Issued on\"");date->setText("Issued on");
  QMetaObject::invokeMethod(widget,"definitionChanged19",Qt::DirectConnection);Rs19Editor::waitForPreview();
  auto *preview=Rs19Editor::preview(dialog);auto *draft=preview?qobject_cast<ProjectDBModel*>(preview->model()):nullptr;
  check(draft&&draft->headerData(3,Qt::Horizontal).toString()=="Issued on","editor19 an SQL heading edit updates the model even with an explicit display caption");
  check(dialog.format().value("headers").toList().value(3).toString()=="Customer date"&&preview&&QetFormatting::columnHeading(preview->customFormat(),draft,3)=="Customer date","editor19 an independently customised display caption stays unchanged after an SQL heading edit");
  dialog.reject();
 }
 check(model->queryString()==originalQuery&&table->customFormat()==explicitFormat,"editor19 cancelling the explicit-caption preview preserves the original table");
 table->setCustomFormat(initialFormat);
 QETApp::unregisterProject(&project);
}
#endif
