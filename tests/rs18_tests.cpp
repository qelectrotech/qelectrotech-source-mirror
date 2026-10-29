// SPDX-License-Identifier: GPL-2.0-or-later
// Production-code regression checks for QET Custom 1.8 workflows.
#include <QtWidgets>
#include <QtSql>
#include <QPdfWriter>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include "qetproject.h"
#include "qetresult.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "ui/tableformat13.h"
#include "ui/tablefields18.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "rs18_query_tests.h"
#include "rs18_defaults_tests.h"
#include "rs18_resize_tests.h"
#include "rs18_align_tests.h"
#include "rs18_combine_tests.h"

namespace {
int checks=0;
void check(bool condition,const char *message) {
 if(!condition)throw std::runtime_error(message);
 ++checks;
}
void title(Diagram *page,const QString &name,int number) {
 auto properties=page->border_and_titleblock.exportTitleBlock();
 properties.title=name;properties.author="QET Custom";
 properties.folio=QString::number(number);properties.date=QDate(2026,10,28);
 properties.useDate=TitleBlockProperties::UseDateValue;
 page->border_and_titleblock.importTitleBlock(properties);
}
IndependentTextItem *note(Diagram *page,const QString &text,const QPointF &position,qreal width=900) {
 auto *item=new IndependentTextItem(text);page->addItem(item);item->setFont(QFont("DejaVu Sans",9));
 item->setPos(position);item->setTextWidth(width);return item;
}
QetGraphicsTableItem *table(Diagram *page,ProjectDBModel *model,const QVariantMap &format,const QString &name,const QPointF &position) {
 auto *item=new QetGraphicsTableItem;page->addItem(item);item->setModel(model);
 item->setPos(position);item->setTableName(name);item->setCustomFormat(format);return item;
}
QVariantMap format(const QString &name,const QVariantList &widths) {
 const auto body=QFont("DejaVu Sans",8),heading=QFont("DejaVu Sans",8,QFont::Bold);
 return {{"enabled",true},{"title",name},{"rows",0},{"widths",widths},
  {"rowHeight",30},{"headerHeight",28},{"titleHeight",32},{"paddingX",5},{"paddingY",3},
  {"autoRowHeight",true},{"autoHeaderHeight",true},
  {"bodyStyle",QVariantMap{{"font",body.toString()},{"color","#ff202020"}}},
  {"headerStyle",QVariantMap{{"font",heading.toString()},{"color","#ffffffff"}}},
  {"titleStyle",QVariantMap{{"font",QFont("DejaVu Sans",10,QFont::Bold).toString()},{"color","#ff173653"}}},
  {"headerBackground","#ff244876"},{"bodyBackground","#ffffffff"},
  {"titleBackground","#ffe9f0f7"},{"gridColor","#ff768390"}};
}
void workflowExample(const QString &out,const QString &sourceDir) {
 QTemporaryDir temporary;
 QFile source(sourceDir+"/tests/qttest/fixtures/ten_sequential_numbers.qet");
 check(source.open(QIODevice::ReadOnly),"example opens the upstream synthetic symbol fixture");
 QDomDocument document;check(document.setContent(source.readAll()),"example fixture XML parses");
 auto diagrams=document.elementsByTagName("diagram");check(diagrams.size()==1,"example starts with one synthetic folio");
 auto firstDiagram=diagrams.at(0).toElement();
 auto wires=firstDiagram.firstChildElement("conductors");if(!wires.isNull())firstDiagram.removeChild(wires);
 const QString temporaryFile=temporary.filePath("example-source.qet");QFile saved(temporaryFile);
 check(saved.open(QIODevice::WriteOnly),"example prepares its private fixture");saved.write(document.toByteArray());saved.close();
 QETProject project(temporaryFile);QETApp::registerProject(&project);project.setTitle("Custom 1.8 workflow examples");
 check(project.state()==QETProject::Ok,"example project opens with real editable symbols");
 auto *bomPage=project.diagrams().first();title(bomPage,"1.8 - BOM and mouse resizing",1);
 auto *contentsPage=project.addNewDiagram();title(contentsPage,"1.8 - one Contents table",2);
 auto *alignmentPage=project.addNewDiagram();title(alignmentPage,"1.8 - text alignment",3);
 QList<Element*> parts;for(auto *item:bomPage->items())if(auto *element=dynamic_cast<Element*>(item))parts<<element;
 std::sort(parts.begin(),parts.end(),[](Element *a,Element *b){return a->pos().x()<b->pos().x();});
 check(parts.size()==4,"example retains four independent synthetic component symbols");
 for(int i=0;i<parts.size();++i){
  DiagramContext info;
  if(i<3){
   info.addValue("label",i<2?QString("-S%1").arg(i+1):QString("-K1"));
   info.addValue("description",i<2?"Magnetic safety switch":"Safety relay 24 V");
   info.addValue("designation",i<2?"BNS 260-02Z-R 3,0M":"PNOZ X2P");
   info.addValue("manufacturer_reference",i<2?"BNS 260-02Z-R 3,0M":"PNOZ X2P");
   info.addValue("manufacturer",i<2?"Schmersal":"Pilz");
   info.addValue("quantity","1");info.addValue("unity","pcs");
  }
  parts[i]->setElementInformations(info);parts[i]->setPos(QPointF(120+i*180,440));
 }
 note(bomPage,"LIVE EXAMPLE DATA: two identical sensors and one relay. The fourth graphic has no BOM data and is excluded. This is a workflow example, not a functional circuit.",{55,55});
 auto *bomModel=new ProjectDBModel(&project,&project);bomModel->setIdentifier("nomenclature");
 bomModel->setQuery(QetTableFields18::defaultQuery18("nomenclature"));
 check(bomModel->columnCount()==7&&bomModel->rowCount()==2,"default live BOM groups the sensors and excludes the decorative graphic");
 bool relayDescription=false,groupedSensors=false;
 for(int r=0;r<bomModel->rowCount();++r){relayDescription|=bomModel->index(r,2).data().toString()=="Safety relay 24 V";groupedSensors|=bomModel->index(r,1).data().toDouble()==2;}
 check(relayDescription&&groupedSensors,"live BOM uses descriptions and the correct grouped quantity");
 auto bomFormat=format("One live BOM table - drag separators to resize",{35,40,280,170,130,145,55});
 bomFormat["wraps"]=QVariantList{false,false,true,true,true,true,false};
 bomFormat["alignments"]=QVariantList{int(Qt::AlignHCenter),int(Qt::AlignHCenter),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignHCenter)};
 auto *bom=table(bomPage,bomModel,bomFormat,"Unified live BOM",{55,175});
 check(bom->resizeColumn18(2,290,false),"example uses the production column resize operation");
 check(bom->resizeRow18(0,45,false),"example uses a persisted individual body-row height");
 note(bomPage,"Select the table. Drag a column/row boundary, double-click a boundary to auto-fit, or open Properties for exact dimensions. Save as default applies separately to BOM and Contents.",{55,535});
 auto *contentsModel=new ProjectDBModel(&project,&project);contentsModel->setIdentifier("summary");
 contentsModel->setQuery(QetTableFields18::defaultQuery18("summary"));
 check(contentsModel->columnCount()==4&&contentsModel->rowCount()==3,"default Contents contains all three example folios in one table");
 auto contentsFormat=format("Contents - one table with four columns",{45,310,425,95});
 contentsFormat["wraps"]=QVariantList{false,true,true,false};
 contentsFormat["alignments"]=QVariantList{int(Qt::AlignHCenter),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignHCenter)};
 table(contentsPage,contentsModel,contentsFormat,"Unified Contents",{55,155});
 note(contentsPage,"This Contents is one table object. Predefined fields, saved formatting defaults and column widths can be changed without entering SQL. Comments is an intentionally blank column in this example.",{55,55});
 note(alignmentPage,"Select the three descriptions below and use the text-alignment/reference command. Choose the topmost text as reference and align right. The reference stays fixed; text blocks move without moving any symbol.",{55,55});
 note(alignmentPage,"Sensor closed",{100,190},150);
 note(alignmentPage,"Dosing enabled\nMultiline description",{290,285},190);
 note(alignmentPage,"Powder flow alarm",{510,395},220);
 note(alignmentPage,"Position alignment and paragraph alignment are separate operations. Try Undo after each operation. These are independent sheet texts; the command also supports selected symbol descriptions.",{55,535});
 QString error;check(project.writeContainerFile(out+"/Table_Workflows_1.8.qetz",&error),qPrintable(error));
 check(project.writeCopyForOlderVersions(out+"/Table_Workflows_1.8.qet").isOk(),"example also writes a plain XML QET copy");
 {
  QETProject reopened(out+"/Table_Workflows_1.8.qetz");QETApp::registerProject(&reopened);
  check(reopened.state()==QETProject::Ok&&reopened.diagrams().size()==3,"combined workflow QETZ reopens with all three folios");
  QetGraphicsTableItem *reloadedBom=nullptr;
  for(auto *folio:reopened.diagrams())for(auto *item:folio->items())
   if(auto *t=dynamic_cast<QetGraphicsTableItem*>(item))if(t->tableName()=="Unified live BOM")reloadedBom=t;
  check(reloadedBom&&reloadedBom->model()->rowCount()==2,"QETZ roundtrip retains live meaningful-component BOM data");
  check(reloadedBom->customFormat().value("widths").toList().at(2).toInt()==290&&
        reloadedBom->customFormat().value("rowHeights18").toMap().value("0").toInt()==45,
        "QETZ roundtrip retains column width and individual row-height operations");
  QETApp::unregisterProject(&reopened);
 }
 {
  QPdfWriter pdf(out+"/Table_Workflows_1.8.pdf");pdf.setResolution(96);pdf.setPageSize(QPageSize(QPageSize::A4));pdf.setPageOrientation(QPageLayout::Landscape);pdf.setPageMargins(QMarginsF(10,10,10,10));
  QPainter painter(&pdf);int n=0;
  for(auto *page:project.diagrams()){
   if(n++)pdf.newPage();const QRectF bounds=page->border_and_titleblock.borderAndTitleBlockRect();
   page->render(&painter,QRectF(0,0,painter.viewport().width(),painter.viewport().height()),bounds,Qt::KeepAspectRatio);
   QImage image(1500,1100,QImage::Format_ARGB32);image.fill(Qt::white);QPainter imagePainter(&image);
   page->render(&imagePainter,QRectF(0,0,image.width(),image.height()),bounds,Qt::KeepAspectRatio);imagePainter.end();
   check(image.save(out+QString("/workflow-page-%1.png").arg(n)),"save a workflow-page preview");
  }
 }
 check(QFileInfo(out+"/Table_Workflows_1.8.pdf").size()>1000,"workflow example produces a multi-page vector PDF");
 QETApp::unregisterProject(&project);
}
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);QCoreApplication::setOrganizationName("RS18Regression");QCoreApplication::setApplicationName("RS18Regression");
 QTemporaryDir settings;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,settings.path());QSettings().clear();
 QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 const QString out=argc>1?QString::fromLocal8Bit(argv[1]):settings.path();QDir().mkpath(out);
 const QString source=argc>2?QString::fromLocal8Bit(argv[2]):QDir::currentPath();
 const QString real=argc>3?QString::fromLocal8Bit(argv[3]):QString();
 const QString feature=argc>4?QString::fromLocal8Bit(argv[4]):QStringLiteral("all");
 if(!QStringList{"all","query","defaults","resize","align","combine","example"}.contains(feature)) {
  std::cerr<<"Unknown feature selector: "<<feature.toStdString()<<"\n";return 2;
 }
 try {
  if(feature=="all"||feature=="query")runQuery18Tests(check,out,source,real);
  if(feature=="all"||feature=="defaults")runDefaults18Tests(check,out,source,real);
  if(feature=="all"||feature=="resize")runResize18Tests(check,out,source,real);
  if(feature=="all"||feature=="align")runAlign18Tests(check,out,source,real);
  if(feature=="all"||feature=="combine")runCombine18Tests(check,out,source,real);
  if(feature=="all"||feature=="example"){QSettings().clear();workflowExample(out,source);}
  std::cout<<"PASS "<<checks<<" Custom 1.8 regression checks ["<<feature.toStdString()<<"]\n";return 0;
 }catch(const std::exception &error){std::cerr<<"FAIL ["<<feature.toStdString()<<"] after "<<checks<<" checks: "<<error.what()<<"\n";return 1;}
}
