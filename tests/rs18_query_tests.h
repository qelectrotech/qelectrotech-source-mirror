// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS18_QUERY_TESTS_H
#define RS18_QUERY_TESTS_H
#include <QtWidgets>
#include <QtSql>
#include "ui/tablefields18.h"
#include "dataBase/ui/elementquerywidget.h"
#include "dataBase/ui/summaryquerywidget.h"
#include "qetproject.h"
#include "dataBase/projectdatabase.h"
#include "qetapp.h"

template<class Check> void runQuery18Tests(Check check,const QString &out,const QString &sourceDir,const QString &realProject) {
 Q_UNUSED(sourceDir)
 using namespace QetTableFields18;
 const QString connection="rs18-query-fixture";
 {
  auto db=QSqlDatabase::addDatabase("QSQLITE",connection);db.setDatabaseName(":memory:");check(db.open(),"query18 opens isolated SQLite fixture");QSqlQuery q(db);
  QStringList columns{"element_uuid TEXT","manufacturer TEXT","manufacturer_reference TEXT","designation TEXT","description TEXT","label TEXT","unity TEXT","quantity TEXT","diagram_position INT","position TEXT"};
  for(int i=1;i<=4;++i)for(const QString &key:QStringList{"manufacturer","manufacturer_reference","designation","description","unity","quantity"})columns<<key+"_auxiliary"+QString::number(i)+" TEXT";
  check(q.exec("CREATE TABLE element_nomenclature_view ("+columns.join(',')+")"),"query18 creates realistic metadata/UUID fixture");
  auto add=[&](const QString &id,const QString &article,const QString &description,const QString &reference,const QString &qty){q.prepare("INSERT INTO element_nomenclature_view (element_uuid,manufacturer,manufacturer_reference,designation,description,label,unity,quantity,diagram_position,position) VALUES (?, 'Maker', ?, 'Legacy designation', ?, ?, 'pc', ?, 1, 'A1')");q.addBindValue(id);q.addBindValue(article);q.addBindValue(description);q.addBindValue(reference);q.addBindValue(qty);check(q.exec(),"query18 inserts component fixture");};
  add("uuid-a","ARTICLE-1","Readable relay description","-K1","1");add("uuid-b","ARTICLE-1","Readable relay description","-K2","2");
  add("uuid-c","","Loose component","-X1","1");add("uuid-d","","Loose component","-X2","1");
  check(q.exec("INSERT INTO element_nomenclature_view(element_uuid, diagram_position, position) VALUES('border-1',1,'A1'),('border-2',1,'A1')"),"query18 adds same-grid decorative objects");
  const QString grouped=QetFormatting::groupedBomQuery(true,true,true,false);
  check(q.exec(grouped),qPrintable(q.lastError().text()));int rows=0;bool description=false,summed=false;while(q.next()){++rows;if(q.value(3)=="ARTICLE-1"){description=q.value(2)=="Readable relay description";summed=q.value(1).toDouble()==3;}}
  check(rows==3,"query18 default BOM excludes graphics and keeps unknown-article UUIDs separate");check(description,"query18 BOM Name prefers description instead of designation part number");check(summed,"query18 grouped quantity sums component quantities");
  check(q.exec(QetFormatting::groupedBomQuery(true,true,true,false,false)),qPrintable(q.lastError().text()));rows=0;while(q.next())++rows;check(rows==5,"query18 explicit all-object mode does not combine same-grid decorative UUIDs");
  check(q.exec("UPDATE element_nomenclature_view SET manufacturer_auxiliary1='Maker',manufacturer_reference_auxiliary1='ACC',description_auxiliary1='Readable accessory',quantity_auxiliary1='2' WHERE element_uuid='uuid-a'"),"query18 adds one auxiliary article");
  check(q.exec(QetFormatting::groupedBomQuery(true,true,true,true)),qPrintable(q.lastError().text()));rows=0;while(q.next())++rows;check(rows==4,"query18 includes actual auxiliary data without empty auxiliary rows");

  ElementQueryWidget bom;bom.setQuery(grouped);auto *sql=bom.findChild<QCheckBox*>("m_edit_sql_query_cb");auto *chosen=bom.findChild<QListWidget*>("m_choosen_list");
  check(sql&&chosen&&!sql->isChecked(),"query18 grouped BOM opens with editable graphical output fields");check(chosen->count()==7&&chosen->item(0)->text()=="No."&&chosen->item(1)->text()=="Qty","query18 No and Qty are ordinary displayed grouped fields");
  sql->setChecked(true);check(QMetaObject::invokeMethod(&bom,"on_m_edit_sql_query_cb_clicked",Qt::DirectConnection),"query18 enters advanced SQL through actual slot");
  check(QetTableQuery15::withoutState(bom.queryStr())==grouped,"query18 entering SQL preserves exact nested grouped query");
  sql->setChecked(false);check(QMetaObject::invokeMethod(&bom,"on_m_edit_sql_query_cb_clicked",Qt::DirectConnection),"query18 returns to graphical columns through actual slot");check(q.exec(bom.queryStr()),"query18 SQL toggle does not create no-such-column article error");
  chosen->setCurrentRow(3);check(QMetaObject::invokeMethod(&bom,"on_m_remove_pb_clicked",Qt::DirectConnection),"query18 removes grouped output field through real UI slot");
  check(q.exec(bom.queryStr())&&q.record().count()==6,"query18 changed field projection executes against nested output source");
  const QString saved=bom.queryStr();ElementQueryWidget reopened;reopened.setQuery(saved);check(reopened.selectedFieldExpressions()==bom.selectedFieldExpressions(),"query18 nested graphical field selection survives save/reopen");check(q.exec(reopened.queryStr())&&q.record().count()==6,"query18 reopened nested graphical query remains valid");
  reopened.resize(900,630);check(reopened.grab().save(out+"/predefined-bom-fields18.png"),"query18 saves friendly grouped fields example");

  const QString duplicates="SELECT /*qet-bom13 qty=1*/ 1 AS x, 2 AS x";ElementQueryWidget duplicate;duplicate.setQuery(duplicates);auto *duplicateSql=duplicate.findChild<QCheckBox*>("m_edit_sql_query_cb");check(duplicateSql->isChecked(),"query18 duplicate output headings stay Advanced even with a grouped-source marker");duplicateSql->setChecked(false);check(QMetaObject::invokeMethod(&duplicate,"on_m_edit_sql_query_cb_clicked",Qt::DirectConnection),"query18 tries duplicate output conversion through actual slot");check(duplicateSql->isChecked()&&duplicate.queryStr()==duplicates,"query18 refuses lossy duplicate-column graphical conversion");
  const QString imported="WITH source AS (SELECT 'Readable' AS description, 'ARTICLE' AS article) SELECT description AS \"Name\", article AS \"Part number\" FROM source";
  ElementQueryWidget nested;nested.setQuery(imported);auto *nestedSql=nested.findChild<QCheckBox*>("m_edit_sql_query_cb");check(nestedSql->isChecked(),"query18 unknown imported SQL initially remains advanced");
  nestedSql->setChecked(false);check(QMetaObject::invokeMethod(&nested,"on_m_edit_sql_query_cb_clicked",Qt::DirectConnection),"query18 safely enables graphical fields for imported CTE");check(q.exec(nested.queryStr())&&q.next()&&q.value(1)=="ARTICLE","query18 imported CTE keeps its actual source after graphical conversion");
  auto *nestedChosen=nested.findChild<QListWidget*>("m_choosen_list");nestedChosen->setCurrentRow(0);check(QMetaObject::invokeMethod(&nested,"on_m_down_pb_clicked",Qt::DirectConnection),"query18 reorders imported query result fields");check(q.exec(nested.queryStr())&&q.next()&&q.value(0)=="ARTICLE","query18 reordered imported projection remains valid");
  ElementQueryWidget presets;check(presets.findChild<QComboBox*>("table18Recipe")&&presets.findChild<QPushButton*>("table18UseRecipe"),"query18 BOM exposes ready-made recipes without SQL input");
  auto *available=presets.findChild<QListWidget*>("m_var_list");bool no=false,qty=false,legacy=false;for(int i=0;i<available->count();++i){no|=available->item(i)->text().startsWith("No.");qty|=available->item(i)->text().startsWith("Qty");legacy|=available->item(i)->text().contains("legacy mapping");}check(no&&qty&&legacy,"query18 available fields include No Qty and explicit legacy part mapping");
  presets.usePredefined18("components");check(q.exec(presets.queryStr())&&q.record().count()==7,"query18 component-list recipe creates one full table query");
  presets.usePredefined18("grouped-aux");check(q.exec(presets.queryStr())&&q.record().count()==7,"query18 grouped auxiliary recipe executes without user SQL");

  bool exampleWorked=false;QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *operation=dialog->findChild<QComboBox*>("table18ExampleOperation");auto *literal=dialog->findChild<QLineEdit*>("table18ExampleLiteral");auto *use=dialog->findChild<QPushButton*>("table18UseExample");if(!operation||!literal||!use){dialog->reject();return;}operation->setCurrentIndex(3);literal->setText("Customer's note; A -- B /* wording */");use->click();auto *expression=dialog->findChild<QLineEdit*>("customFieldExpression");exampleWorked=expression&&expression->text()=="'Customer''s note; A -- B /* wording */'";dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
  QVariantMap field;check(QetTableQuery15::editField(nullptr,field,"nomenclature")&&exampleWorked,"query18 no-SQL example builder quotes fixed text punctuation and supplies names");

  SummaryQueryWidget summary;summary.usePredefined18("contents");check(!summary.findChild<QCheckBox*>("m_edit_sql_query_cb")->isChecked(),"query18 default Contents recipe opens graphically");check(summary.selectedFieldExpressions().size()==4,"query18 default Contents is one query with four fields");summary.usePredefined18("contents-full");check(summary.selectedFieldExpressions().size()==6,"query18 extended Contents adds Author and Revision without SQL");
  summary.resize(900,520);check(summary.grab().save(out+"/predefined-contents-fields18.png"),"query18 saves friendly contents recipe example");
  const QString summaryNested="WITH source AS (SELECT 'A' AS page, 'Readable content' AS title) SELECT page AS \"Page\", title AS \"Page content\" FROM source";
  summary.setQuery(summaryNested);auto *summarySql=summary.findChild<QCheckBox*>("m_edit_sql_query_cb");summarySql->setChecked(false);check(QMetaObject::invokeMethod(&summary,"on_m_edit_sql_query_cb_clicked",Qt::DirectConnection),"query18 summary imported CTE graphical conversion uses actual slot");check(q.exec(summary.queryStr())&&q.next()&&q.value(1)=="Readable content","query18 summary projection preserves imported source");
 }
 QSqlDatabase::removeDatabase(connection);
 if(!realProject.isEmpty()&&QFileInfo::exists(realProject)) {
  QETProject actual(realProject);QETApp::registerProject(&actual);check(actual.state()==QETProject::Ok,"query18 actual QETZ project opens");
  auto q=actual.dataBase()->newQuery(defaultQuery18("nomenclature"));check(q.isActive()&&!q.lastError().isValid(),"query18 corrected default grouped BOM runs on actual QETZ metadata");
  int rows=0;bool sawDescription=false,blank=false,sequential=true,relayQty=false,redLedQty=false,actuator=false;while(q.next()){++rows;blank|=q.value(2).toString().trimmed().isEmpty()&&q.value(3).toString().trimmed().isEmpty();sawDescription|=q.value(2).toString().contains("MCB");sequential&=q.value(0).toInt()==rows;const QString part=q.value(3).toString();if(part=="2903350")relayQty=q.value(1).toDouble()==2;if(part=="M22-LED-R")redLedQty=q.value(1).toDouble()==2;actuator|=q.value(2).toString()=="Magnetic actuator";}
  // The original manually assigned item number 11 is shared by green and
  // red LED articles. Its highest printed number is not the line count:
  // 20 main component instances - 2 duplicate articles = 18 main groups,
  // plus the separately orderable magnetic actuator = 19 useful lines.
  QFile countReport(out+"/actual-bom-count18.txt");if(countReport.open(QIODevice::WriteOnly|QIODevice::Text)){QTextStream stream(&countReport);stream<<"Useful grouped BOM lines: "<<rows<<" (18 main article groups + 1 auxiliary actuator).\n";}
  check(rows==19,"query18 actual grouped BOM has 19 useful lines: 18 main groups plus auxiliary actuator");check(!blank&&sawDescription,"query18 actual BOM shows component descriptions and no blank decorative rows");check(sequential&&relayQty&&redLedQty&&actuator,"query18 actual No/Qty remain sequential with both repeated articles summed and actuator included");
  auto contents=actual.dataBase()->newQuery(defaultQuery18("summary"));check(contents.isActive()&&contents.record().count()==4,"query18 ready-made Contents query has four fields on actual QETZ");
  QETApp::unregisterProject(&actual);
 }
}
#endif
