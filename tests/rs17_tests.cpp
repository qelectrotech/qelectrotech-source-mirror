// SPDX-License-Identifier: GPL-2.0-or-later
// Production-code regression checks for QET Custom 1.7.
#include <QtWidgets>
#include <QtSql>
#include <QPdfWriter>
#include <iostream>
#include <stdexcept>
#include "qetproject.h"
#include "qetresult.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "diagram.h"
#include "ui/tableformat13.h"
#include "ui/tableeditor15.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "rs17_folio_tests.h"
#include "rs17_pe_tests.h"

namespace {
int checks=0;
void check(bool condition,const char *message) {
 if(!condition)throw std::runtime_error(message);
 ++checks;
}
QVariantMap tableFormat() {
 const QFont base("DejaVu Sans",10),column("DejaVu Sans",14,QFont::Bold),row("DejaVu Sans",20,QFont::Normal,true),heading("DejaVu Sans",13,QFont::Bold);
 const QVariantMap description{{"bodyStyle",QVariantMap{{"font",column.toString()},{"color","#ff1040c0"}}},{"bodyBackground","#fff1f6ff"},{"headerStyle",QVariantMap{{"font",heading.toString()},{"color","#ffffffff"}}},{"headerBackground","#ff244876"}};
 const QVariantMap rowOne{{"bodyStyle",QVariantMap{{"font",row.toString()},{"color","#ffe02020"}}},{"bodyBackground","#ffffedc0"}};
 const QVariantMap rowTwo{{"bodyStyle",QVariantMap{{"color","#80ff0000"}}}};
 return {{"enabled",true},{"title","PER-ROW / PER-COLUMN FORMATTING"},{"rows",3},
  {"widths",QVariantList{45,190,125}},{"rowHeight",26},{"headerHeight",26},{"titleHeight",30},
  {"paddingX",3},{"paddingY",3},{"autoRowHeight",true},{"autoHeaderHeight",true},
  {"wraps",QVariantList{false,true,true}},{"bodyStyle",QVariantMap{{"font",base.toString()},{"color","#ff000000"}}},
  {"headerStyle",QVariantMap{{"font",base.toString()},{"color","#ff000000"}}},
  {"bodyBackground","#ffffffff"},{"headerBackground","#ffd0d0d0"},
  {"columnOverrides17",QVariantList{QVariantMap(),description,QVariantMap()}},
  {"rowOverrides17",QVariantMap{{"1",rowOne},{"2",rowTwo}}}};
}
QImage renderFormat(const QVariantMap &fmt,const QAbstractItemModel *model,const QFont &fallback) {
 const auto layout=QetFormatting::tableLayout(fmt,model,0,model->rowCount(),fallback);
 QImage image((layout.width+32)*2,(layout.bodyHeight+layout.headerHeight+layout.titleHeight+32)*2,QImage::Format_ARGB32);
 image.fill(Qt::white);QPainter painter(&image);painter.scale(2,2);painter.translate(16,16+layout.headerHeight+layout.titleHeight);
 QetFormatting::paintTable(painter,fmt,model,0,model->rowCount(),fallback);return image;
}
QColor sampleBody(const QImage &image,const QetFormatting::TableLayout15 &layout,int column,int row) {
 int x=16+2;for(int c=0;c<column;++c)x+=layout.widths[c];
 int y=16+layout.headerHeight+layout.titleHeight+2;for(int r=0;r<row;++r)y+=layout.rowHeights[r];
 return image.pixelColor(x*2,y*2);
}
void tableUiChecks(const QString &out,QStandardItemModel &data,const QVariantMap &fmt,const QFont &fallback) {
 using namespace QetFormatting;
 auto columnDraft=columnOverride17(fmt,1);bool editorSeen=false,fontSeen=false;
 QTimer::singleShot(0,[&](){
  auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
  auto *fontEnabled=dialog->findChild<QCheckBox*>("bodyFontOverride17");auto *colourEnabled=dialog->findChild<QCheckBox*>("bodyColourOverride17");auto *fontButton=dialog->findChild<QPushButton*>("bodyFont17");auto *background=dialog->findChild<QCheckBox*>("bodyBackgroundOverride17");auto *backgroundButton=dialog->findChild<QPushButton*>("bodyBackgroundColour17");
  if(!fontEnabled||!colourEnabled||!fontButton||!background||!backgroundButton){dialog->reject();return;}editorSeen=true;fontEnabled->setChecked(true);colourEnabled->setChecked(true);
  QTimer::singleShot(0,[&](){if(auto *fontDialog=qobject_cast<QFontDialog*>(QApplication::activeModalWidget())){fontSeen=true;fontDialog->setCurrentFont(QFont("DejaVu Sans",17,QFont::Bold));fontDialog->accept();}});
  fontButton->click();
  if(auto *opacity=dialog->findChild<QSpinBox*>("bodyTextOpacity17"))opacity->setValue(40);
  background->setChecked(true);backgroundButton->setProperty("colour","#ffdcf6dc");
  dialog->grab().save(out+"/column-style-dialog.png");dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
 });
 check(editCellOverride17(nullptr,columnDraft,fmt,fallback,true),"individual column style dialog accepts changes");
 check(editorSeen&&fontSeen,"column editor exposes individual font and background controls");
 check(font(columnDraft.value("bodyStyle").toMap(),fallback).pointSize()==17,"font picker changes only the column draft font");
 check(color(columnDraft.value("bodyStyle").toMap()).alpha()==102,"column font opacity is persisted from its control");
 check(columnDraft.value("bodyBackground")=="#ffdcf6dc","column background override is saved independently");
 check(font(columnOverride17(fmt,1).value("bodyStyle").toMap(),fallback).pointSize()==14,"editing a draft does not mutate the input table format");
 auto rowDraft=rowOverride17(fmt,1),before=rowDraft;bool rowSeen=false;
 QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;rowSeen=dialog->objectName()=="rowStyle17Dialog";if(auto *fontEnabled=dialog->findChild<QCheckBox*>("bodyFontOverride17"))fontEnabled->setChecked(false);dialog->reject();});
 check(!editCellOverride17(nullptr,rowDraft,fmt,fallback,false)&&rowSeen,"individual row editor supports Cancel");
 check(rowDraft==before,"Cancel leaves row font and colours unchanged");
 auto fontOnly=rowOverride17(fmt,1);bool fontOnlySeen=false;
 QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *fontEnabled=dialog->findChild<QCheckBox*>("bodyFontOverride17");auto *colourEnabled=dialog->findChild<QCheckBox*>("bodyColourOverride17");if(!fontEnabled||!colourEnabled){dialog->reject();return;}fontOnlySeen=true;fontEnabled->setChecked(true);colourEnabled->setChecked(false);dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
 check(editCellOverride17(nullptr,fontOnly,fmt,fallback,false)&&fontOnlySeen,"row editor permits an independent font-only override");
 check(fontOnly.value("bodyStyle").toMap().contains("font")&&!fontOnly.value("bodyStyle").toMap().contains("color"),"font-only row does not manufacture a text-colour override");
 auto fontOnlyFormat=fmt;auto fontOnlyRows=fontOnlyFormat.value("rowOverrides17").toMap();fontOnlyRows["1"]=fontOnly;fontOnlyFormat["rowOverrides17"]=fontOnlyRows;
 check(font(bodyCellStyle17(fontOnlyFormat,1,1),fallback).pointSize()==20&&color(bodyCellStyle17(fontOnlyFormat,1,1))==QColor("#ff1040c0"),"font-only row preserves its column text colour");
 const auto beforeReopen=fontOnly;
 QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(dialog)dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
 check(editCellOverride17(nullptr,fontOnly,fmt,fallback,false)&&fontOnly==beforeReopen,"reopening font-only styling does not introduce unrelated keys");
 auto colourOnly=rowOverride17(fmt,1);bool colourOnlySeen=false;
 QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *fontEnabled=dialog->findChild<QCheckBox*>("bodyFontOverride17");auto *colourEnabled=dialog->findChild<QCheckBox*>("bodyColourOverride17");auto *colourButton=dialog->findChild<QPushButton*>("bodyTextColour17");auto *opacity=dialog->findChild<QSpinBox*>("bodyTextOpacity17");if(!fontEnabled||!colourEnabled||!colourButton||!opacity){dialog->reject();return;}colourOnlySeen=true;fontEnabled->setChecked(false);colourEnabled->setChecked(true);colourButton->setProperty("colour","#ff208040");opacity->setValue(80);dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
 check(editCellOverride17(nullptr,colourOnly,fmt,fallback,false)&&colourOnlySeen,"row editor permits an independent colour/opacity-only override");
 check(!colourOnly.value("bodyStyle").toMap().contains("font")&&color(colourOnly.value("bodyStyle").toMap()).alpha()==204,"colour-only row omits a font override and retains its opacity");
 auto colourOnlyFormat=fmt;auto colourOnlyRows=colourOnlyFormat.value("rowOverrides17").toMap();colourOnlyRows["1"]=colourOnly;colourOnlyFormat["rowOverrides17"]=colourOnlyRows;
 check(font(bodyCellStyle17(colourOnlyFormat,1,1),fallback).pointSize()==14,"colour-only row preserves its column font");
 auto edited=fmt;bool outerSeen=false;
 QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *page=dialog->findChild<QWidget*>("individualStyles17");auto *column=dialog->findChild<QComboBox*>("styleColumn17");auto *row=dialog->findChild<QSpinBox*>("styleRow17");auto *resetColumn=dialog->findChild<QPushButton*>("resetColumnStyle17");auto *resetRow=dialog->findChild<QPushButton*>("resetRowStyle17");
  if(!page||!column||!row||!resetColumn||!resetRow){dialog->reject();return;}outerSeen=true;dialog->findChild<QTabWidget*>()->setCurrentWidget(page);column->setCurrentIndex(1);row->setValue(2);dialog->grab().save(out+"/individual-row-column-controls.png");resetColumn->click();resetRow->click();dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
 check(editTableFormat(nullptr,edited,&data,{45,190,125},3,fallback),"main table format dialog accepts individual reset actions");
 check(outerSeen,"individual styling tab provides column and row selection");
 check(columnOverride17(edited,1).isEmpty()&&rowOverride17(edited,1).isEmpty(),"selected row and column reset to inherited formatting");
 check(!rowOverride17(edited,2).isEmpty(),"resetting one row preserves another row override");
 auto cancelled=fmt;
 QTimer::singleShot(0,[&](){auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;if(auto *b=dialog->findChild<QPushButton*>("resetAllColumnStyles17"))b->click();if(auto *b=dialog->findChild<QPushButton*>("resetAllRowStyles17"))b->click();dialog->reject();});
 check(!editTableFormat(nullptr,cancelled,&data,{45,190,125},3,fallback),"Cancel discards reset-all draft changes");
 check(cancelled==fmt,"Cancel preserves every original row and column override");
}
void tableChecks(const QString &out) {
 using namespace QetFormatting;
 const QFont fallback("DejaVu Sans",10);
 QStandardItemModel data(3,3);data.setHorizontalHeaderLabels({"No.","Description","Manufacturer"});
 const QString description="Magnetic safety sensor with a deliberately long description";
 for(int r=0;r<3;++r){data.setData(data.index(r,0),QString::number(r+1));data.setData(data.index(r,1),description);data.setData(data.index(r,2),"Schmersal");}
 auto fmt=tableFormat();auto old=fmt;old.remove("columnOverrides17");old.remove("rowOverrides17");
 check(font(bodyCellStyle17(fmt,0,0),fallback).pointSize()==10,"unmodified column inherits table body font");
 check(font(bodyCellStyle17(fmt,1,0),fallback).pointSize()==14,"column-specific font changes only its own column");
 check(color(bodyCellStyle17(fmt,1,0))==QColor("#ff1040c0"),"column-specific text colour is effective");
 check(font(bodyCellStyle17(fmt,1,1),fallback).pointSize()==20,"row font overrides the column font at their intersection");
 check(color(bodyCellStyle17(fmt,1,1))==QColor("#ffe02020"),"row text colour overrides the column colour");
 check(font(bodyCellStyle17(fmt,0,1),fallback).pointSize()==20,"row font applies across the complete body row");
 check(font(bodyCellStyle17(fmt,1,2),fallback).pointSize()==14,"partial row override retains the column font");
 check(color(bodyCellStyle17(fmt,1,2)).alpha()==128,"row text-opacity override survives effective-style merge");
 check(font(headerCellStyle17(fmt,1),fallback).pointSize()==13,"column header can use a different font");
 check(color(headerCellStyle17(fmt,1))==QColor(Qt::white),"column header text colour is independent of its body");
 check(headerCellBackground17(fmt,1)==QColor("#ff244876"),"column header background overrides the table header");
 check(bodyCellBackground17(fmt,1,0)==QColor("#fff1f6ff"),"column body background is effective");
 check(bodyCellBackground17(fmt,2,1)==QColor("#ffffedc0"),"row background reaches a column without its own override");
 check(bodyCellBackground17(fmt,1,1)==QColor("#ffffedc0"),"row background takes precedence over column background");
 check(bodyCellBackground17(fmt,2,0)==QColor(Qt::white),"unmodified cells retain the table background");
 auto invalid=fmt;auto columns=invalid.value("columnOverrides17").toList();auto column=columns[1].toMap();column["headerBackground"]="invalid colour";columns[1]=column;invalid["columnOverrides17"]=columns;
 check(headerCellBackground17(invalid,1)==QColor("#ffd0d0d0"),"invalid override colour falls back instead of painting invalid data");
 const auto layout=tableLayout(fmt,&data,0,3,fallback),legacy=tableLayout(old,&data,0,3,fallback);
 check(layout.rowHeights[0]>legacy.rowHeights[0],"column font participates in wrapped automatic row height");
 check(layout.rowHeights[1]>layout.rowHeights[0],"larger per-row font expands only that automatic row");
 auto tail=tableLayout(fmt,&data,1,1,fallback);
 check(tail.rowHeights.size()==1&&tail.rowHeights[0]==layout.rowHeights[1],"continuation-table offset uses absolute model-row overrides");
 const auto fit=fittedColumnWidths(fmt,&data,0,3,fallback),oldFit=fittedColumnWidths(old,&data,0,3,fallback);
 check(fit[1]>oldFit[1],"automatic column width measures individual fonts");
 auto fixed=fmt;fixed["autoRowHeight"]=false;fixed["rowHeight"]=26;
 check(!tableLayout(fixed,&data,0,3,fallback).warnings.isEmpty(),"manual-height cell reports clipping after a larger row font");
 auto reset=fmt;reset.remove("columnOverrides17");reset.remove("rowOverrides17");
 check(tableLayout(reset,&data,0,3,fallback).rowHeights==legacy.rowHeights,"clearing all overrides restores inherited table geometry");
 const auto image=renderFormat(fmt,&data,fallback);
 check(sampleBody(image,layout,1,0)==QColor("#fff1f6ff"),"painting uses the column body background");
 check(sampleBody(image,layout,2,1)==QColor("#ffffedc0"),"painting uses the row body background");
 check(sampleBody(image,layout,2,0)==QColor(Qt::white),"painting leaves an unmodified cell white");
 const int bodyTop=(16+layout.headerHeight+layout.titleHeight)*2;int blueInk=0,redInk=0;
 for(int y=bodyTop;y<bodyTop+layout.rowHeights[0]*2;++y)for(int x=(16+layout.widths[0])*2;x<(16+layout.widths[0]+layout.widths[1])*2;++x){const auto c=image.pixelColor(x,y);if(c.blue()>c.red()+50&&c.blue()>c.green()+30)++blueInk;}
 for(int y=bodyTop+layout.rowHeights[0]*2;y<bodyTop+(layout.rowHeights[0]+layout.rowHeights[1])*2;++y)for(int x=32;x<(16+layout.width)*2;++x){const auto c=image.pixelColor(x,y);if(c.red()>c.green()+70&&c.red()>c.blue()+70)++redInk;}
 check(blueInk>20,"painted text uses the individual column colour");check(redInk>20,"painted text uses the individual row colour");
 check(image.save(out+"/row-column-formatting.png"),"save row/column styling preview");
 tableUiChecks(out,data,fmt,fallback);
 {QPdfWriter pdf(out+"/row-column-formatting.pdf");pdf.setResolution(96);pdf.setPageSize(QPageSize(QPageSize::A4));QPainter painter(&pdf);painter.translate(20,70);paintTable(painter,fmt,&data,0,3,fallback);}
 check(QFileInfo(out+"/row-column-formatting.pdf").size()>1000,"create vector PDF with independent row/column formatting");
 const auto config=QetTableEditor15::configuration("nomenclature","SELECT 1 AS 'No.', 'Relay' AS 'Description', 'Maker' AS 'Manufacturer'",fmt);
 const auto json=QJsonDocument::fromVariant(config).toJson();const auto decoded=QJsonDocument::fromJson(json).toVariant().toMap();QString error;
 check(QetTableEditor15::validConfiguration(decoded,"nomenclature",&error),"full table configuration accepts the override metadata");
 check(decoded.value("format").toMap()==fmt,"configuration JSON roundtrip retains every row/column font and colour");
 QFile configFile(out+"/Row_Column_Formatting.qettable.json");check(configFile.open(QIODevice::WriteOnly),"open shareable override configuration");configFile.write(json);configFile.close();

 QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
 auto *model=new ProjectDBModel(&project,&project);model->setIdentifier("nomenclature");
 model->setQuery("SELECT 1 AS 'No.', 'Magnetic safety sensor with a deliberately long description' AS 'Description', 'Schmersal' AS 'Manufacturer' UNION ALL SELECT 2,'Magnetic safety sensor with a deliberately long description','Schmersal' UNION ALL SELECT 3,'Magnetic safety sensor with a deliberately long description','Schmersal'");
 auto *table=new QetGraphicsTableItem;page->addItem(table);table->setPos(80,120);table->setModel(model);table->setCustomFormat(fmt);table->setTableName("Independent row and column formatting");
 {QetTableEditor15::Dialog dialog(table);auto *query=dialog.findChild<ElementQueryWidget*>("tableDefinition15");check(query!=nullptr,"populated BOM dialog is available for column reorder");
  query->setQuery("SELECT 'Relay' AS 'Description', 1 AS 'No.', 'Maker' AS 'Manufacturer'");check(dialog.refreshPreview(),"changed column order can be previewed");
  const auto reordered=dialog.format();check(!columnOverride17(reordered,0).isEmpty()&&columnOverride17(reordered,1).isEmpty(),"column overrides follow matching output headings after reorder");check(rowOverride17(reordered,1)==rowOverride17(fmt,1),"column reorder leaves absolute body-row rules intact");dialog.reject();}
 check(table->customFormat()==fmt&&model->columnCount()==3,"Cancel of reordered preview leaves the real table unchanged");
 QDomDocument xml;auto element=table->toXml(xml);auto *copy=new QetGraphicsTableItem;page->addItem(copy);copy->fromXml(element);
 check(copy->customFormat()==fmt,"table XML retains row and column overrides");
 check(copy->size()==table->size(),"table XML reload recomputes effective-font geometry");page->removeItem(copy);delete copy;
 QUndoStack undo;undo.push(new QPropertyUndoCommand(table,"customFormat",fmt,old));check(table->customFormat()==old,"clear overrides is one undoable format change");undo.undo();check(table->customFormat()==fmt,"Undo restores all individual styles");undo.redo();check(table->customFormat()==old,"Redo restores inherited styles");undo.undo();
 const QString qet=out+"/Row_Column_Formatting.qet";check(project.writeCopyForOlderVersions(qet).isOk(),"save complete styling sample as plain QET");
 const QString qetz=out+"/Row_Column_Formatting.qetz";check(project.writeContainerFile(qetz,&error),qPrintable(error));
 for(const auto &path:QStringList{qet,qetz}){
  QETProject reopened(path);QETApp::registerProject(&reopened);check(reopened.state()==QETProject::Ok,"styling sample reopens in either QET or QETZ format");
  QetGraphicsTableItem *found=nullptr;for(auto *folio:reopened.diagrams())for(auto *item:folio->items())if(auto *t=dynamic_cast<QetGraphicsTableItem*>(item))found=t;
  check(found!=nullptr,"saved project retains the formatted table");check(found->customFormat()==fmt,"full project roundtrip retains individual row/column styles");check(found->size()==table->size(),"full project roundtrip retains matching table dimensions");QETApp::unregisterProject(&reopened);
 }
 QETApp::unregisterProject(&project);
}
void unifiedTableExample(const QString &out) {
 QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
 auto *model=new ProjectDBModel(&project,&project);model->setIdentifier("nomenclature");
 model->setQuery("SELECT 1 AS 'No.', 1 AS 'Qty', 'Magnetic safety sensor' AS 'Name', 'BNS 260-02Z-R 3,0M' AS 'Part number', '-S4' AS 'References', 'Schmersal' AS 'Manufacturer' UNION ALL SELECT 2,1,'Safety relay 24 V','PNOZ X2P','-K1','Pilz' UNION ALL SELECT 3,2,'Motor protection relay','2903350','-4K4, -4K5','Phoenix Contact'");
 const QFont base("DejaVu Sans",8),columnFont("DejaVu Sans",9,QFont::Normal,true),rowFont("DejaVu Sans",10,QFont::Bold);
 const QVariantMap nameColumn{{"bodyStyle",QVariantMap{{"font",columnFont.toString()},{"color","#ff1040a0"}}},{"bodyBackground","#ffeaf2ff"}};
 const QVariantMap highlightedRow{{"bodyStyle",QVariantMap{{"font",rowFont.toString()},{"color","#ff9c2010"}}},{"bodyBackground","#ffffedc0"}};
 QVariantMap fmt{{"enabled",true},{"title","Example BOM - one table, six columns"},{"rows",0},
  {"widths",QVariantList{35,40,260,175,130,160}},{"rowHeight",32},{"headerHeight",26},{"titleHeight",30},
  {"autoRowHeight",true},{"autoHeaderHeight",true},{"paddingX",5},{"paddingY",3},
  {"wraps",QVariantList{false,false,true,true,true,true}},
  {"alignments",QVariantList{int(Qt::AlignHCenter),int(Qt::AlignHCenter),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft)}},
  {"bodyStyle",QVariantMap{{"font",base.toString()},{"color","#ff000000"}}},
  {"headerStyle",QVariantMap{{"font",QFont("DejaVu Sans",8,QFont::Bold).toString()},{"color","#ffffffff"}}},
  {"headerBackground","#ff244876"},{"titleStyle",QVariantMap{{"font",QFont("DejaVu Sans",10,QFont::Bold).toString()}}},
  {"columnOverrides17",QVariantList{QVariantMap(),QVariantMap(),nameColumn,QVariantMap(),QVariantMap(),QVariantMap()}},
  {"rowOverrides17",QVariantMap{{"1",highlightedRow}}}};
 auto *table=new QetGraphicsTableItem;page->addItem(table);table->setPos(70,150);table->setModel(model);table->setCustomFormat(fmt);table->setTableName("One unified BOM example");
 auto *note=new IndependentTextItem("Custom 1.7 example: Name column has its own style; body row 2 overrides column styles. Example data only.");page->addItem(note);note->setPos(70,70);note->setFont(base);note->setTextWidth(850);
 QString error;check(project.writeContainerFile(out+"/Table_Styles_1.7.qetz",&error),qPrintable(error));
 check(model->columnCount()==6&&model->rowCount()==3,"editable example is one table with six BOM columns");
 check(renderFormat(fmt,model,base).save(out+"/Table_Styles_1.7.png"),"save unified table example preview");
 {QPdfWriter pdf(out+"/Table_Styles_1.7.pdf");pdf.setResolution(96);pdf.setPageSize(QPageSize(QPageSize::A4));pdf.setPageOrientation(QPageLayout::Landscape);pdf.setPageMargins(QMarginsF(10,10,10,10));QPainter painter(&pdf);painter.translate(15,70);QetFormatting::paintTable(painter,fmt,model,0,3,base);}
 check(QFileInfo(out+"/Table_Styles_1.7.pdf").size()>1000,"save unified table example vector PDF");
 QETApp::unregisterProject(&project);
}
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);QCoreApplication::setOrganizationName("RS17Regression");QCoreApplication::setApplicationName("RS17Regression");
 QTemporaryDir temp;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,temp.path());QSettings().clear();QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 const QString out=argc>1?QString::fromLocal8Bit(argv[1]):temp.path();QDir().mkpath(out);
 const QString source=argc>2?QString::fromLocal8Bit(argv[2]):QDir::currentPath();
 const QString real=argc>3?QString::fromLocal8Bit(argv[3]):QString();
 try {
  tableChecks(out);
  unifiedTableExample(out);
  Rs17Folio::run(out,real,check);
  runPe17Tests(check,out,source,real);
  std::cout<<"PASS "<<checks<<" Custom 1.7 regression checks\n";return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";return 1;}
}
