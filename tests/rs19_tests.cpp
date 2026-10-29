// SPDX-License-Identifier: GPL-2.0-or-later
// Integration checks and editable examples for QET Custom 1.9.
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
#include "ui/tablequery15.h"
#include "ui/formulabuilder19.h"
#include "ui/tableeditor15.h"
#include "ui/textalignment18.h"
#include "dataBase/ui/elementquerywidget.h"
#include "dataBase/ui/summaryquerywidget.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "rs19_query_tests.h"
#include "rs19_formula_tests.h"
#include "rs19_align_tests.h"
#include "rs19_commands_tests.h"
#include "rs19_editor_tests.h"

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
int column(ProjectDBModel *model,const QString &heading) {
 for(int c=0;c<model->columnCount();++c)
  if(model->headerData(c,Qt::Horizontal).toString()==heading)return c;
 return -1;
}
// Keep the example independent of the user's production file. All symbols come
// from the upstream synthetic fixture and a local text-anchor definition.
QDomDocument exampleDocument(const QString &sourceDir) {
 QFile source(sourceDir+"/tests/qttest/fixtures/ten_sequential_numbers.qet");
 check(source.open(QIODevice::ReadOnly),"example opens the upstream synthetic symbol fixture");
 QDomDocument document;check(document.setContent(source.readAll()),"example fixture XML parses");
 auto root=document.documentElement();auto diagrams=document.elementsByTagName("diagram");
 check(diagrams.size()==1,"example starts with one synthetic folio");
 auto bom=diagrams.at(0).toElement();
 auto wires=bom.firstChildElement("conductors");if(!wires.isNull())bom.removeChild(wires);
 auto contents=bom.cloneNode(true).toElement();contents.setAttribute("uuid",QUuid::createUuid().toString());
 contents.setAttribute("order",1);contents.removeChild(contents.firstChildElement("elements"));root.insertBefore(contents,bom);
 bom.setAttribute("order",2);
 auto alignment=bom.cloneNode(true).toElement();alignment.setAttribute("uuid",QUuid::createUuid().toString());alignment.setAttribute("order",3);
 auto elements=alignment.firstChildElement("elements");
 for(auto e=elements.firstChildElement("element");!e.isNull();e=e.nextSiblingElement("element")) {
  e.setAttribute("uuid",QUuid::createUuid().toString());e.setAttribute("type","embed://custom19/TextAnchor.elmt");
  while(!e.firstChild().isNull())e.removeChild(e.firstChild());
  for(const auto &tag:{"terminals","inputs","elementInformations","dynamic_texts","texts_groups"})e.appendChild(document.createElement(tag));
 }
 root.appendChild(alignment);
 // A fifth unannotated graphic demonstrates meaningful-component exclusion.
 auto bomElements=bom.firstChildElement("elements");auto decorative=bomElements.firstChildElement("element").cloneNode(true).toElement();
 decorative.setAttribute("uuid",QUuid::createUuid().toString());
 // Legacy terminal IDs are unique per folio, even when the element UUID is
 // new. Cloning the graphic must not clone the source terminal addresses.
 int terminalId=200;
 for(auto terminal=decorative.firstChildElement("terminals").firstChildElement("terminal");!terminal.isNull();terminal=terminal.nextSiblingElement("terminal"))terminal.setAttribute("id",terminalId++);
 auto dynamic=decorative.firstChildElement("dynamic_texts");while(!dynamic.firstChild().isNull())dynamic.removeChild(dynamic.firstChild());
 decorative.removeChild(decorative.firstChildElement("elementInformations"));decorative.appendChild(document.createElement("elementInformations"));
 bomElements.appendChild(decorative);
 QDomDocument anchor;
 check(anchor.setContent(QStringLiteral(R"XML(<category name="custom19"><names><name lang="en">Custom 1.9 layout examples</name></names><element name="TextAnchor.elmt"><definition type="element" link_type="simple" version="0.100.0" width="1" height="1" hotspot_x="0" hotspot_y="0"><uuid uuid="{659beced-b933-4c81-a92c-80f95bb597c8}"/><names><name lang="en">Text-only layout anchor</name></names><description><line x1="0" y1="0" x2="1" y2="0" style="line-style:normal;line-weight:thin;filling:none;color:white"/></description></definition></element></category>)XML")),"example creates its text-only anchor definition");
 root.firstChildElement("collection").appendChild(document.importNode(anchor.documentElement(),true));
 return document;
}
void workflowExample(const QString &out,const QString &sourceDir) {
 QTemporaryDir temporary;const auto document=exampleDocument(sourceDir);
 const QString temporaryFile=temporary.filePath("example-source.qet");QFile saved(temporaryFile);
 check(saved.open(QIODevice::WriteOnly),"example prepares its private fixture");saved.write(document.toByteArray());saved.close();
 QETProject project(temporaryFile);QETApp::registerProject(&project);project.setTitle("Custom 1.9 table and alignment examples");
 check(project.state()==QETProject::Ok&&project.diagrams().size()==3,"example project opens all three editable folios");
 auto *contentsPage=project.diagrams().at(0),*bomPage=project.diagrams().at(1),*alignmentPage=project.diagrams().at(2);
 title(contentsPage,"1.9 - Contents and field formats",1);title(bomPage,"1.9 - grouped BOM and formulas",2);title(alignmentPage,"1.9 - normal text selection",3);
 auto contentsTitle=contentsPage->border_and_titleblock.exportTitleBlock();contentsTitle.folio="I";contentsPage->border_and_titleblock.importTitleBlock(contentsTitle);
 auto bomTitle=bomPage->border_and_titleblock.exportTitleBlock();bomTitle.folio="2A";bomPage->border_and_titleblock.importTitleBlock(bomTitle);
 QList<Element*> parts;for(auto *item:bomPage->items())if(auto *element=dynamic_cast<Element*>(item))parts<<element;
 std::sort(parts.begin(),parts.end(),[](Element *a,Element *b){return a->pos().x()<b->pos().x()||(a->pos().x()==b->pos().x()&&a->uuid().toString()<b->uuid().toString());});
 if(parts.size()!=5)qWarning()<<"example loaded component illustrations:"<<parts.size();
 check(parts.size()==5,"example retains five separate synthetic component illustrations");
 for(int i=0;i<parts.size();++i){
  DiagramContext info;
  if(i<4){
   info.addValue("label",i<2?QString("-S%1").arg(i+1):i==2?QString("-K1"):QString("-TEST"));
   info.addValue("description",i<2?"Magnetic safety switch":i==2?"Safety relay 24 V":"Excluded supplier example");
   info.addValue("designation",i<2?"Sensor model":i==2?"Relay model":"Do not order");
   info.addValue("manufacturer_reference",i<2?"SENSOR-01":i==2?"RELAY-24":"EXCLUDE-01");
   info.addValue("manufacturer",i<2?"Northern Controls GmbH":i==2?"Relay Works":"Unused supplier");
   info.addValue("quantity",i==3?"7":"1");info.addValue("unity","pcs");
  }
  parts[i]->setElementInformations(info);parts[i]->setPos(QPointF(120+i*165,450));
 }
 project.dataBase()->updateDB();
 note(bomPage,"SYNTHETIC TEST DATA: two sensors and one relay are ordered. A supplier filter excludes a fourth component; an unannotated graphic is excluded automatically. These illustrations are not a functional circuit.",{55,55});
 ElementQueryWidget bomDefinition; // Production controls generate and retain graphical state.
 bomDefinition.setQuery(QetTableFields18::defaultQuery18("nomenclature"));
 QString formulaError;
 const auto filterTree=QetFormula19::operation("ne",{QetFormula19::field("manufacturer"),QetFormula19::text("Unused supplier")});
 const auto supplierFilter=QetFormula19::compile(filterTree,{{"Manufacturer","manufacturer"}},false,&formulaError);
 check(!supplierFilter.isEmpty(),qPrintable(formulaError));bomDefinition.setFilter19(supplierFilter);
 // The expression builder stores a named result field that remains editable.
 const auto orderingTree=QetFormula19::operation("if",{
  QetFormula19::operation("gt",{QetFormula19::field("\"Qty\""),QetFormula19::number(1)}),QetFormula19::text("Multiple"),QetFormula19::text("Single")});
 const auto orderingExpression=QetFormula19::compile(orderingTree,{{"Qty","\"Qty\""}},false,&formulaError);
 check(!orderingExpression.isEmpty(),qPrintable(formulaError));
 check(bomDefinition.addCustomField("Ordering note","Ordering note",orderingExpression,false),"example adds a reusable computed result field");
 auto *available=bomDefinition.findChild<QListWidget*>("m_var_list");bool selectedFormula=false;
 if(available)for(int row=0;row<available->count();++row)if(available->item(row)->data(Qt::UserRole).toString()==orderingExpression+" AS \"Ordering note\""){
  available->setCurrentRow(row);selectedFormula=QMetaObject::invokeMethod(&bomDefinition,"on_m_add_pb_clicked",Qt::DirectConnection);break;
 }
 check(selectedFormula,"example selects the computed field through the production Add field control");
 auto *bomModel=new ProjectDBModel(&project,&project);bomModel->setIdentifier("nomenclature");bomModel->setQuery(bomDefinition.queryStr());
 check(bomModel->columnCount()==8&&bomModel->rowCount()==2,"live grouped BOM filters source components, excludes graphics and retains eight chosen columns");
 const int qty=column(bomModel,"Qty"),name=column(bomModel,"Name"),ordering=column(bomModel,"Ordering note");
 check(qty>=0&&name>=0&&ordering>=0,"BOM exposes quantity, description and formula headings");
 bool relayDescription=false,groupedSensors=false,formulaMultiple=false;
 for(int r=0;r<bomModel->rowCount();++r){relayDescription|=bomModel->index(r,name).data().toString()=="Safety relay 24 V";groupedSensors|=bomModel->index(r,qty).data().toDouble()==2;formulaMultiple|=bomModel->index(r,ordering).data().toString()=="Multiple";}
 check(relayDescription&&groupedSensors&&formulaMultiple,"live BOM has human descriptions, source-filtered grouped quantities and calculated results");
 auto bomFormat=format("Grouped BOM - filter and formula example",{35,40,235,125,100,145,45,100});
 bomFormat["wraps"]=QVariantList{false,false,true,true,true,true,false,true};
 bomFormat["alignments"]=QVariantList{int(Qt::AlignHCenter),int(Qt::AlignHCenter),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignHCenter),int(Qt::AlignHCenter)};
 table(bomPage,bomModel,bomFormat,"Grouped BOM with formula",{55,160});
 note(bomPage,"Open the table Properties. Explore available fields, element types and the filter builder. Ordering note demonstrates IF Qty > 1 THEN Multiple ELSE Single. Changes are previewed before Apply.",{55,535});
 SummaryQueryWidget contentsDefinition;contentsDefinition.setQuery(QetTableFields18::defaultQuery18("summary"));
 contentsDefinition.enableNumber19();contentsDefinition.setNumberStart19(1);contentsDefinition.setDateFormat19("%d.%m.%Y");
 auto *contentsModel=new ProjectDBModel(&project,&project);contentsModel->setIdentifier("summary");contentsModel->setQuery(contentsDefinition.queryStr());
 check(contentsModel->columnCount()==5&&contentsModel->rowCount()==3,"Contents includes automatic No. separately from Page and date format");
 const int no=column(contentsModel,"No."),page=column(contentsModel,"Page"),date=column(contentsModel,"Date");
 check(no>=0&&page>=0&&date>=0,"Contents has distinct automatic-number, page and date headings");
 check(contentsModel->index(0,page).data().toString()=="I"&&contentsModel->index(1,page).data().toString()=="2A","Contents keeps nonnumeric page designations independent of automatic item numbers");
 for(int r=0;r<3;++r)check(contentsModel->index(r,no).data().toInt()==r+1,"Contents numbers rows in displayed folio order");
 check(contentsModel->index(0,date).data().toString()=="28.10.2026","Contents date formatting is generated by the production controls");
 QVariantList contentsWidths,contentsWraps,contentsAlignments;
 for(int c=0;c<contentsModel->columnCount();++c){const auto heading=contentsModel->headerData(c,Qt::Horizontal).toString();
  contentsWidths<<(heading=="No."?35:heading=="Page"?45:heading=="Page content"?310:heading=="Comments"?390:95);
  contentsWraps<<(heading=="Page content"||heading=="Comments");contentsAlignments<<int(heading=="Page content"||heading=="Comments"?Qt::AlignLeft:Qt::AlignHCenter);
 }
 auto contentsFormat=format("Contents - numbered rows and formatted dates",contentsWidths);
 contentsFormat["wraps"]=contentsWraps;contentsFormat["alignments"]=contentsAlignments;
 table(contentsPage,contentsModel,contentsFormat,"Numbered Contents",{55,155});
 note(contentsPage,"No. is a sequential table number; Page is the actual sheet designation. Both remain editable through Properties. Date has one field with a format choice. Comments is intentionally blank in this example.",{55,55});
 // Four independently selectable text-only symbols reproduce the cover-sheet
 // workflow without using or copying the customer's production project.
 QList<Element*> anchors;for(auto *item:alignmentPage->items())if(auto *element=dynamic_cast<Element*>(item))anchors<<element;
 std::sort(anchors.begin(),anchors.end(),[](Element *a,Element *b){return a->pos().x()<b->pos().x();});
 check(anchors.size()==4,"alignment example contains four real selectable text-owning symbols");
 const QStringList descriptions{"Example Manufacturing GmbH","Sample street 16","23858 Example town","Demonstration project"};
 const QList<QPointF> positions{{285,205},{340,245},{315,285},{400,390}};
 for(int i=0;i<anchors.size();++i){
  anchors[i]->setElementInformations(DiagramContext());anchors[i]->setPos(positions[i]);
  auto *text=new DynamicElementTextItem(anchors[i]);anchors[i]->addDynamicTextItem(text);text->setText(descriptions[i]);text->setFont(QFont("DejaVu Sans",11));text->setPos(0,0);text->setTextWidth(-1);
 }
 auto *frame=new QetShapeItem(QPointF(80,140),QPointF(930,475),QetShapeItem::Rectangle);alignmentPage->addItem(frame);frame->setPen(QPen(QColor("#74879a"),0.7));
 note(alignmentPage,"Select the four labels normally on the sheet, then Edit > Text alignment and reference. Their symbol texts are included automatically. You may also select the frame; automatic references use only the checked texts.",{55,55});
 note(alignmentPage,"Try Right edges with the first text as fixed reference. Then try Horizontal centres with Page drawing area. Preview is temporary; Apply is one Undo. Text moves independently of the symbols. Toolbar commands can be added through Customize.",{55,535});
 auto *first=anchors.first();auto originalPosition=first->pos();first->setSelected(true);anchors.at(1)->setSelected(true);frame->setSelected(true);
 auto gathered=QetTextAlignment18::selectedTexts(alignmentPage);
 check(gathered.size()==2,"normal symbol selection discovers text targets while the selected frame remains a reference only");
 auto plan=QetTextAlignment18::planPositions19(alignmentPage,QetTextAlignment18::Right,QetTextAlignment18::TopmostObject,nullptr,gathered);
 check(plan.result.considered==2&&plan.anchor!=frame,"automatic text reference ignores the unrelated selected frame");
 check(first->pos()==originalPosition,"preparing an alignment preview does not move the symbol");
 alignmentPage->clearSelection();
 QString error;check(project.writeContainerFile(out+"/Table_Alignment_Workflows_1.9.qetz",&error),qPrintable(error));
 check(project.writeCopyForOlderVersions(out+"/Table_Alignment_Workflows_1.9.qet").isOk(),"example also writes a plain XML QET copy");
 {
  QETProject reopened(out+"/Table_Alignment_Workflows_1.9.qetz");QETApp::registerProject(&reopened);
  check(reopened.state()==QETProject::Ok&&reopened.diagrams().size()==3,"1.9 workflow QETZ reopens with all three editable folios");
  QetGraphicsTableItem *reloadedBom=nullptr,*reloadedContents=nullptr;
  for(auto *folio:reopened.diagrams())for(auto *item:folio->items())if(auto *t=dynamic_cast<QetGraphicsTableItem*>(item)){
   if(t->tableName()=="Grouped BOM with formula")reloadedBom=t;else if(t->tableName()=="Numbered Contents")reloadedContents=t;
  }
  check(reloadedBom&&reloadedBom->model()->rowCount()==2&&reloadedBom->model()->columnCount()==8,"QETZ roundtrip retains the live grouped BOM and calculated field");
  check(reloadedContents&&reloadedContents->model()->rowCount()==3&&reloadedContents->model()->columnCount()==5,"QETZ roundtrip retains numbered Contents and formatted dates");
  QETApp::unregisterProject(&reopened);
 }
 {
  QPdfWriter pdf(out+"/Table_Alignment_Workflows_1.9.pdf");pdf.setResolution(96);pdf.setPageSize(QPageSize(QPageSize::A4));pdf.setPageOrientation(QPageLayout::Landscape);pdf.setPageMargins(QMarginsF(10,10,10,10));
  QPainter painter(&pdf);int n=0;
  for(auto *folio:project.diagrams()){
   if(n++)pdf.newPage();const QRectF bounds=folio->border_and_titleblock.borderAndTitleBlockRect();
   folio->render(&painter,QRectF(0,0,painter.viewport().width(),painter.viewport().height()),bounds,Qt::KeepAspectRatio);
   QImage image(1500,1100,QImage::Format_ARGB32);image.fill(Qt::white);QPainter imagePainter(&image);
   folio->render(&imagePainter,QRectF(0,0,image.width(),image.height()),bounds,Qt::KeepAspectRatio);imagePainter.end();
   check(image.save(out+QString("/workflow-page-%1.png").arg(n)),"save a 1.9 workflow-page preview");
  }
 }
 check(QFileInfo(out+"/Table_Alignment_Workflows_1.9.pdf").size()>1000,"1.9 example produces a three-page vector PDF");
 QETApp::unregisterProject(&project);
}
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);QCoreApplication::setOrganizationName("RS19Regression");QCoreApplication::setApplicationName("RS19Regression");
 QTemporaryDir settings;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,settings.path());QSettings().clear();
 QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 const QString out=argc>1?QString::fromLocal8Bit(argv[1]):settings.path();QDir().mkpath(out);
 const QString source=argc>2?QString::fromLocal8Bit(argv[2]):QDir::currentPath();
 const QString real=argc>3?QString::fromLocal8Bit(argv[3]):QString();
 const QString feature=argc>4?QString::fromLocal8Bit(argv[4]):QStringLiteral("all");
 if(!QStringList{"all","query","formula","align","commands","editor","example"}.contains(feature)) {
  std::cerr<<"Unknown feature selector: "<<feature.toStdString()<<"\n";return 2;
 }
 try {
  if(feature=="all"||feature=="query")runQuery19Tests(check,out,source,real);
  if(feature=="all"||feature=="formula")runFormula19Tests(check,out,source,real);
  if(feature=="all"||feature=="align")runAlign19Tests(check,out,source,real);
  if(feature=="all"||feature=="commands")runCommands19Tests(check,out,source,real);
  if(feature=="all"||feature=="editor")runEditor19Tests(check,out,source,real);
  if(feature=="all"||feature=="example"){QSettings().clear();workflowExample(out,source);}
  std::cout<<"PASS "<<checks<<" Custom 1.9 regression checks ["<<feature.toStdString()<<"]\n";return 0;
 }catch(const std::exception &error){std::cerr<<"FAIL ["<<feature.toStdString()<<"] after "<<checks<<" checks: "<<error.what()<<"\n";return 1;}
}
