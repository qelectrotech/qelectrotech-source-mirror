// SPDX-License-Identifier: GPL-2.0-or-later
// Integration checks for the 1.5 table workflow. Links the production objects.
#include <QtWidgets>
#include <QtSql>
#include <QPdfWriter>
#include <iostream>
#include "qetproject.h"
#include "diagram.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "ui/tableformat13.h"
#include "ui/tableeditor15.h"
#include "ui/tablequery15.h"
#include "dataBase/ui/elementquerywidget.h"
#include "dataBase/ui/summaryquerywidget.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

namespace {
int checks=0;
void check(bool condition,const char *message) {
 if(!condition) throw std::runtime_error(message);
 ++checks;
}
QStringList selected(QWidget &widget) {
 auto *list=widget.findChild<QListWidget*>("m_choosen_list");
 check(list!=nullptr,"selected-column list exists");
 QStringList result;
 for(int i=0;i<list->count();++i) result<<list->item(i)->data(Qt::UserRole).toString();
 return result;
}
void choose(QWidget &widget,const QString &availableName,const QString &expression) {
 auto *available=widget.findChild<QListWidget*>(availableName);
 check(available!=nullptr,"available-column list exists");QListWidgetItem *found=nullptr;
 for(int i=0;i<available->count();++i)if(available->item(i)->data(Qt::UserRole).toString()==expression)found=available->item(i);
 check(found!=nullptr,qPrintable(QString("available field: %1").arg(expression)));available->setCurrentItem(found);
 check(QMetaObject::invokeMethod(&widget,"on_m_add_pb_clicked",Qt::DirectConnection),"add chosen field through production slot");
}
void queryChecks(const QString &out) {
 using namespace QetTableQuery15;
 check(splitColumns("label /* note, comma */ AS x, manufacturer -- note, comma\n AS y").size()==2,"SQL comments containing commas do not create extra columns");
 const QString unionQuery="SELECT 1 AS 'Value' UNION ALL SELECT 2";
 check(selectColumns(unionQuery)==QStringList{"1 AS 'Value'"},"no-FROM UNION exposes only the first result projection");
 check(selectColumns("SELECT 'a;b' AS value;")==QStringList{"'a;b' AS value"},"semicolon inside a string is retained and final delimiter omitted");
 check(displayName("1 AS 'Bob''s value'")=="Bob's value","single-quoted aliases unescape doubled apostrophes");
 ElementQueryWidget constants;constants.setQuery(unionQuery);check(constants.queryStr()==unionQuery&&constants.selectedFieldExpressions()==QStringList{"1 AS 'Value'"},"no-FROM UNION reopens without query or field loss");
 SummaryQueryWidget duplicates;const QString repeated="SELECT title, title FROM project_summary_view ORDER BY pos";duplicates.setQuery(repeated);
 check(duplicates.selectedFieldExpressions()==QStringList{"title","title"},"duplicate projections remain separate selected columns");duplicates.setQuery(duplicates.queryStr());
 check(duplicates.selectedFieldExpressions()==QStringList{"title","title"},"reopening duplicate projections does not drop or accumulate columns");
 const QString complex="WITH source AS (SELECT 'x,y' AS description) SELECT COALESCE(description, 'a,b') AS \"Description\", printf('%s,%s', description, 'z') AS \"Pair\" FROM source ORDER BY 2 DESC";
 auto projections=selectColumns(complex);check(projections.size()==2,"nested and quoted commas do not split projection fields");
 check(displayName(projections[0])=="Description"&&displayName(projections[1])=="Pair","projection aliases restored");
 SummaryQueryWidget advanced;advanced.setQuery(complex);check(advanced.queryStr()==complex,"complex SQL remains byte-identical");
 check(advanced.selectedFieldExpressions()==projections,"complex SQL selected fields remain visible");
 check(advanced.findChild<QCheckBox*>("m_edit_sql_query_cb")->isChecked(),"complex SQL explicitly remains Advanced");
 const QString bomSql="SELECT COALESCE(description, 'a,b') AS \"Description\", manufacturer FROM element_nomenclature_view WHERE quantity <> '' GROUP BY manufacturer ORDER BY manufacturer DESC";
 ElementQueryWidget bomAdvanced;bomAdvanced.setQuery(bomSql);check(bomAdvanced.queryStr()==bomSql,"custom BOM SQL sorting and grouping preserved");
 check(bomAdvanced.selectedFieldExpressions().size()==2,"custom BOM SQL restores two visible fields");
 ElementQueryWidget bom;check(bom.addCustomField("Combined description","Display description","COALESCE(description, 'a,b')",false),"define project custom BOM field");
 const QString custom="COALESCE(description, 'a,b') AS \"Display description\"";
 choose(bom,"m_var_list","label");choose(bom,"m_var_list",custom);
 bom.setGroupBy("manufacturer");bom.setCount("COUNT(*) AS item_count");bom.setExtraFilter("quantity <> ''");
 auto *chosen=bom.findChild<QListWidget*>("m_choosen_list");chosen->setCurrentRow(1);check(QMetaObject::invokeMethod(&bom,"on_m_up_pb_clicked",Qt::DirectConnection),"change selected-field order");
 const auto order=bom.selectedFieldExpressions();const QString saved=bom.queryStr();auto savedState=state(saved);
 check(order.size()==2&&order[0]==custom,"custom field reorder takes effect");
 check(!savedState.isEmpty()&&savedState.value("context")=="nomenclature","graphical BOM definition includes project-contained state");
 check(savedState.value("groupBy").toString().contains("manufacturer")&&savedState.value("count").toString().contains("item_count"),"grouping and count saved as editor settings");
 check(savedState.value("extraFilter").toString().contains("quantity"),"extra filter saved as editor setting");
 QSettings().remove("table15/fields");ElementQueryWidget reopened;reopened.setQuery(saved);
 check(reopened.selectedFieldExpressions()==order,"BOM reopen restores exact selected-field order");
 check(withoutState(reopened.queryStr())==withoutState(saved),"BOM reopen restores filters, grouping and query");
 check(!reopened.findChild<QCheckBox*>("m_edit_sql_query_cb")->isChecked(),"recognized saved definition reopens graphically");
 check(state(reopened.queryStr()).value("customFields").toList().size()==1,"project-contained custom field survives without local presets");
 ElementQueryWidget remembered;check(remembered.addCustomField("Persistent custom","Reference copy","label",true),"save custom field for other projects");
 ElementQueryWidget nextProject;auto *available=nextProject.findChild<QListWidget*>("m_var_list");bool present=false;for(int i=0;i<available->count();++i)if(available->item(i)->text().startsWith("Persistent custom"))present=true;check(present,"new query editor loads reusable local custom field");
 check(!remembered.addCustomField("Bad","Bad","label; DROP TABLE element",false),"custom field rejects additional SQL statements");
 check(!remembered.addCustomField("","Heading","label",false),"custom field requires a name");
 SummaryQueryWidget summary;check(summary.addCustomField("Title and author","Description","COALESCE(title, '') || ', ' || COALESCE(author, '')",false),"define custom contents field");
 choose(summary,"m_available_list","folio");const QString summaryCustom="COALESCE(title, '') || ', ' || COALESCE(author, '') AS \"Description\"";choose(summary,"m_available_list",summaryCustom);
 const QString summarySaved=summary.queryStr();SummaryQueryWidget summaryReopened;summaryReopened.setQuery(summarySaved);
 check(summaryReopened.selectedFieldExpressions()==summary.selectedFieldExpressions(),"contents reopen restores selected custom fields");
 check(withoutState(summaryReopened.queryStr())==withoutState(summarySaved),"contents reopen preserves query and folio order");
 QETProject project;QETApp::registerProject(&project);project.addNewDiagram();ProjectDBModel model(&project);model.setIdentifier("nomenclature");model.setQuery(saved);check(model.columnCount()==3,"generated custom-field BOM query executes against the real project database");
 QDomDocument modelDoc;auto modelXml=model.toXml(modelDoc);ProjectDBModel persisted(&project);persisted.fromXml(modelXml);
 check(persisted.queryString()==saved,"generated query metadata survives model XML save/reopen");
 check(persisted.identifier()=="nomenclature"&&persisted.columnCount()==3,"model XML retains table type and result columns");
 ElementQueryWidget xmlReopened;xmlReopened.setQuery(persisted.queryString());
 check(xmlReopened.selectedFieldExpressions()==order,"model XML restores selected-field order on another editor");
 auto restoredFields=state(xmlReopened.queryStr()).value("customFields").toList();bool containsProjectFields=true;for(const auto &field:savedState.value("customFields").toList())if(!restoredFields.contains(field))containsProjectFields=false;
 check(containsProjectFields,"model XML preserves every project-contained custom field alongside local reusable fields");
 model.setIdentifier("summary");model.setQuery(summarySaved);check(model.columnCount()==2&&model.rowCount()==1,"generated custom contents query executes against the real project database");
 model.setQuery("SELECT 1 AS 'First', 2 AS 'Second' WHERE 0");check(model.columnCount()==2&&model.rowCount()==0,"empty SELECT retains its column definitions");
 model.setQuery("SELECT NULL AS 'First', 'kept' AS 'Second'");check(model.columnCount()==2&&model.rowCount()==1&&model.index(0,1).data().toString()=="kept","NULL first field does not discard later columns");
 QETApp::unregisterProject(&project);
 reopened.resize(1000,650);reopened.grab().save(out+"/bom-query-dialog.png");summaryReopened.resize(900,600);summaryReopened.grab().save(out+"/contents-query-dialog.png");
}
void continuationChecks() {
 QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
 auto *model=new ProjectDBModel(&project,&project);model->setIdentifier("summary");
 const QString before="SELECT 1 AS 'No.', 'First row' AS 'Description' UNION ALL SELECT 2, 'Second row'";
 model->setQuery(before);auto *first=new QetGraphicsTableItem;auto *second=new QetGraphicsTableItem;page->addItem(first);page->addItem(second);first->setModel(model);second->setModel(model);first->setDisplayNRow(1);second->setDisplayNRow(1);second->setPreviousTable(first);
 QPointer<QetGraphicsTableItem> continuation=second;
 check(first->nextTable()==second&&second->previousTable()==first,"linked continuation fixture established");
 QUndoStack undo;undo.push(new QetTableEditor15::Change(first,"SELECT 1 AS 'No.', 'Only one row' AS 'Description'",QVariantMap()));
 QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
 check(model->rowCount()==1&&!continuation.isNull(),"shrinking the query does not delete its empty continuation");
 check(first->nextTable()==continuation&&continuation->previousTable()==first,"query edit retains the continuation chain");
 undo.undo();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
 check(model->queryString()==before&&model->rowCount()==2,"Undo restores both original query rows");
 check(!continuation.isNull()&&first->nextTable()==continuation&&continuation->previousTable()==first,"Undo retains both linked table identities");
 QETApp::unregisterProject(&project);
}
void layoutChecks(const QString &out) {
 QStandardItemModel model(2,2);
 model.setHorizontalHeaderLabels({"No.","Description"});
 model.setData(model.index(0,0),"1");model.setData(model.index(1,0),"2");
 model.setData(model.index(0,1),"Magnetic safety sensor with a deliberately long description");
 model.setData(model.index(1,1),"Relay");
 QFont f("DejaVu Sans",10);
 QVariantMap fmt{{"enabled",true},{"rows",2},{"widths",QVariantList{35,115}},
  {"rowHeight",20},{"headerHeight",22},{"autoRowHeight",true},
  {"autoWidths",QVariantList{false,false}},{"wraps",QVariantList{false,true}},
  {"paddingX",3},{"paddingY",2},{"bodyStyle",QVariantMap{{"font",f.toString()}}}};
 auto wrapped=QetFormatting::tableLayout(fmt,&model,0,2,f);
 check(wrapped.widths[0]==35&&wrapped.widths[1]==115,"manual widths are retained with wrapping");
 check(wrapped.rowHeights[0]>wrapped.rowHeights[1],"wrapped long description increases only its row height");
 check(wrapped.rowHeights[1]>=20,"automatic rows respect the minimum row height");
 auto tail=QetFormatting::tableLayout(fmt,&model,1,1,f);
 check(tail.rowHeights.size()==1&&tail.rowHeights[0]==wrapped.rowHeights[1],"linked-table offset measures the displayed data");
 auto fixed=fmt;fixed["autoRowHeight"]=false;
 auto clipped=QetFormatting::tableLayout(fixed,&model,0,2,f);
 check(clipped.rowHeights[0]==20&&clipped.rowHeights[1]==20,"manual row height stays fixed");
 check(!clipped.warnings.isEmpty(),"insufficient fixed row height produces a warning");
 auto automatic=fmt;automatic["autoWidths"]=QVariantList{false,true};
 auto fitted=QetFormatting::tableLayout(automatic,&model,0,2,f);
 check(fitted.widths[0]==35&&fitted.widths[1]>115,"automatic width fits the long text independently per column");
 auto padded=fmt;padded["paddingX"]=12;padded["paddingY"]=8;
 auto padding=QetFormatting::tableLayout(padded,&model,0,2,f);
 check(padding.rowHeights[0]>wrapped.rowHeights[0],"cell padding participates in wrapping and height");
 QImage image(700,450,QImage::Format_ARGB32);image.fill(Qt::white);
 {QPainter p(&image);p.translate(20,60);p.scale(2,2);QetFormatting::paintTable(p,fmt,&model,0,2,f);}
 check(image.save(out+"/wrapping-preview.png"),"save wrapping preview");
 {QPdfWriter pdf(out+"/wrapping-preview.pdf");pdf.setResolution(96);QPainter p(&pdf);p.translate(20,60);QetFormatting::paintTable(p,fmt,&model,0,2,f);}
 // Deliberate overflow remains visible; clipping stays inside the cell.
 QStandardItemModel overflowModel(1,1);overflowModel.setHorizontalHeaderLabels({"H"});overflowModel.setData(overflowModel.index(0,0),"WWWWWWWWWWWWWWWWWWWW");
 QVariantMap narrow{{"enabled",true},{"widths",QVariantList{40}},{"rowHeight",24},{"headerHeight",20},{"bodyStyle",QVariantMap{{"font",f.toString()}}},{"gridColor","#00000000"}};
 auto overflow=narrow;overflow["overflowModes"]=QVariantList{"overflow"};
 auto overGeometry=QetFormatting::tableLayout(overflow,&overflowModel,0,1,f);
 check(overGeometry.bounds.width()>overGeometry.width,"overflow expands redraw bounds beyond the table");
 auto renderCell=[&](const QVariantMap &v){QImage img(450,80,QImage::Format_RGB32);img.fill(Qt::white);QPainter p(&img);p.translate(10,30);QetFormatting::paintTable(p,v,&overflowModel,0,1,f);return img;};
 auto clipImage=renderCell(narrow),overflowImage=renderCell(overflow);int clippedOutside=0,overflowOutside=0;
 for(int y=31;y<54;++y)for(int x=56;x<400;++x){if(qGray(clipImage.pixel(x,y))<100)++clippedOutside;if(qGray(overflowImage.pixel(x,y))<100)++overflowOutside;}
 check(clippedOutside==0,"clipping prevents text outside its cell");check(overflowOutside>20,"allowed overflow paints outside its cell");
 check(overflowImage.save(out+"/overflow-preview.png"),"save overflow preview");
 // Operate the real format dialog: auto-fit selected columns, fit rows,
 // inspect the live preview warning, then accept its persisted settings.
 auto formatCopy=fmt;bool formatSeen=false,selectedOnly=false,rowFit=false,widthWarning=false;
 QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *columns=dialog->findChild<QTableWidget*>("formatColumns15");if(!columns){dialog->reject();return;}
  formatSeen=true;columns->selectRow(0);QPushButton *fitColumn=nullptr,*fitRow=nullptr;for(auto *button:dialog->findChildren<QPushButton*>()){if(button->text()=="Auto-fit selected columns now")fitColumn=button;if(button->text()=="Auto-fit row height now")fitRow=button;}
  if(!fitColumn||!fitRow){dialog->reject();return;}fitColumn->click();
  selectedOnly=static_cast<QSpinBox*>(columns->cellWidget(0,1))->value()==QetFormatting::fittedColumnWidths(fmt,&model,0,2,f)[0]&&static_cast<QSpinBox*>(columns->cellWidget(1,1))->value()==115;
  fitRow->click();rowFit=dialog->findChild<QSpinBox*>("rowHeight")->value()>20&&!dialog->findChild<QCheckBox*>("autoRowHeight")->isChecked();
  auto *tabs=dialog->findChild<QTabWidget*>();tabs->setCurrentIndex(2);for(auto *label:dialog->findChildren<QLabel*>())if(label->text().contains("available sheet width"))widthWarning=true;dialog->grab().save(out+"/format-preview-dialog.png");tabs->setCurrentIndex(0);dialog->grab().save(out+"/format-controls-dialog.png");dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
 });
 check(QetFormatting::editTableFormat(nullptr,formatCopy,&model,{35,115},2,f,50),"format dialog accepts sizing edits");
 check(formatSeen&&selectedOnly,"auto-fit selected columns leaves other widths unchanged");check(rowFit,"auto-fit rows sets a sufficient fixed height");check(widthWarning,"preview warns when table exceeds sheet space");
 check(formatCopy.value("widths").toList()[1].toInt()==115&&!formatCopy.value("autoRowHeight").toBool(),"accepted dialog retains fitted sizing choices");
 // Use the actual project model and placed table to verify persistence, undo and refresh.
 QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
 auto *dbModel=new ProjectDBModel(&project,&project);dbModel->setIdentifier("nomenclature");
 dbModel->setQuery("SELECT 1 AS 'No.', 'Magnetic safety sensor with a deliberately long description' AS 'Description' UNION ALL SELECT 2,'Relay'");
 auto *table=new QetGraphicsTableItem;page->addItem(table);table->setModel(dbModel);table->setCustomFormat(fmt);
 int height=table->size().height();check(height>40,"placed-table geometry uses variable row heights");
 QDomDocument doc;auto xml=table->toXml(doc);auto *restored=new QetGraphicsTableItem;page->addItem(restored);restored->fromXml(xml);
 check(restored->customFormat()==fmt,"wrapping and automatic sizing XML roundtrip");
 check(restored->size()==table->size(),"restored table recomputes matching geometry");
 page->removeItem(restored);delete restored;
 QUndoStack undo;undo.push(new QPropertyUndoCommand(table,"customFormat",fmt,fixed));
 check(table->size().height()==40,"manual-height change updates placed geometry");undo.undo();
 check(table->customFormat()==fmt&&table->size().height()==height,"Undo restores sizing and wrapping");undo.redo();undo.undo();undo.clear();
 dbModel->setQuery("SELECT 1 AS 'No.', 'A much longer translated description for the magnetic safety sensor with additional words that require more wrapped lines' AS 'Description' UNION ALL SELECT 2,'Relay'");
 check(table->customFormat()==fmt,"data refresh preserves layout policy");
 check(table->size().height()>height,"automatic row height recalculates for longer refreshed text");
 // Reusable definition files contain both the query and the formatting.
 auto config=QetTableEditor15::configuration("nomenclature",dbModel->queryString(),fmt);
 QString error;check(QetTableEditor15::validConfiguration(config,"nomenclature",&error),qPrintable(error));
 check(!QetTableEditor15::validConfiguration(config,"summary",&error),"BOM configuration cannot silently replace contents configuration");
 auto json=QJsonDocument::fromVariant(config).toJson();auto jsonConfig=QJsonDocument::fromJson(json).toVariant().toMap();
 check(QetTableEditor15::validConfiguration(jsonConfig,"nomenclature",&error),"configuration survives JSON save/reopen");
 auto unsafe=QetTableEditor15::configuration("nomenclature","DELETE FROM element",fmt);
 check(!QetTableEditor15::validConfiguration(unsafe,"nomenclature",&error),"configuration rejects destructive SQL");
 auto beforeQuery=dbModel->queryString();auto beforeFormat=table->customFormat();
 {QetTableEditor15::Dialog dialog(table);check(dialog.query()==beforeQuery,"definition dialog loads the existing query");check(dialog.format()==beforeFormat,"definition dialog loads existing formatting");check(dialog.refreshPreview(),"definition preview executes the current query");dialog.reject();}
 check(dbModel->queryString()==beforeQuery&&table->customFormat()==beforeFormat,"Cancel leaves the table unchanged");
 auto edited=fmt;edited["title"]="EDITED TABLE";
 auto editConfig=QetTableEditor15::configuration("nomenclature","SELECT 7 AS 'No.', 'Edited description' AS 'Description'",edited);
 bool modalSeen=false;QString acceptedQuery;
 QTimer::singleShot(0,[&](){auto *dialog=dynamic_cast<QetTableEditor15::Dialog*>(QApplication::activeModalWidget());if(!dialog)return;QString err;if(!dialog->loadConfiguration(editConfig,&err)){dialog->reject();return;}modalSeen=true;acceptedQuery=dialog->query();dialog->grab().save(out+"/table-definition-dialog.png");dialog->findChild<QTabWidget*>()->setCurrentIndex(1);dialog->grab().save(out+"/table-preview-dialog.png");dialog->applyChanges();dialog->accept();});
 check(QetTableEditor15::edit(table),"table definition accepted");check(modalSeen,"table properties opens the populated modal workflow");
 check(dbModel->queryString()==acceptedQuery&&table->customFormat()==edited,"Apply updates query and formatting together");
 page->undoStack().undo();check(dbModel->queryString()==beforeQuery&&table->customFormat()==beforeFormat,"one Undo restores query and formatting together");
 page->undoStack().redo();check(dbModel->queryString()==acceptedQuery&&table->customFormat()==edited,"Redo reapplies table definition");
 QETApp::unregisterProject(&project);

}
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);QCoreApplication::setOrganizationName("RS15Regression");QCoreApplication::setApplicationName("RS15Regression");
 QTemporaryDir temp;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,temp.path());QSettings().clear();QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 const QString out=argc>1?QString::fromLocal8Bit(argv[1]):temp.path();QDir().mkpath(out);
 try {
  queryChecks(out);
  layoutChecks(out);
  continuationChecks();
  std::cout<<"PASS "<<checks<<" table-workflow checks\n";return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";return 1;}
}
