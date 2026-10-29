// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtWidgets>
#include <QtPrintSupport>
#include <iostream>
#include <stdexcept>
#include "qetproject.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "qetresult.h"
#include "ui/labelclearance24.h"
#include "ui/languageui16.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/elementtextitemgroup.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#define private public
#include "print/projectprintwindow.h"
#undef private
using namespace QetLabelClearance24;
static int checks=0;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);++checks;}
class Move : public QUndoCommand {QPointer<DiagramTextItem> text;QPointF a,b;public:Move(DiagramTextItem *t,QPointF to):text(t),a(t->pos()),b(to){setText("Move test text");}void undo()override{if(text)text->setPos(a);}void redo()override{if(text)text->setPos(b);}};
void exportPdf(QETProject &p,const QString &file,bool links=true){
 auto *printer=new QPrinter(QPrinter::HighResolution);printer->setOutputFormat(QPrinter::PdfFormat);printer->setResolution(600);printer->setOutputFileName(file);printer->setPageSize(QPageSize(QPageSize::A4));printer->setPageOrientation(QPageLayout::Landscape);printer->setPageMargins(QMarginsF(10,12,15,14),QPageLayout::Millimeter);
 ProjectPrintWindow w(&p,printer);w.findChild<QCheckBox*>("m_fit_in_page_cb")->setChecked(true);auto *toggle=w.findChild<QCheckBox*>("clickableReferences24");check(toggle&&toggle->isEnabled(),"PDF links option available in fit-to-page mode");w.findChild<QCheckBox*>("m_fit_in_page_cb")->setChecked(false);check(!toggle->isEnabled(),"PDF links option explains tiled export limitation");w.findChild<QCheckBox*>("m_fit_in_page_cb")->setChecked(true);toggle->setChecked(links);w.m_printing=true;w.requestPaint();PdfLinks::convertUriToGoTo(file);PdfLinks::removeUnusedPdfxNamespace(file);
}
void render(Diagram *page,const QString &path,const QRectF &area={}){QImage image(1600,1050,QImage::Format_ARGB32);image.fill(Qt::white);QPainter paint(&image);page->render(&paint,QRectF(0,0,1600,1050),area.isEmpty()?page->border_and_titleblock.insideBorderRect():area);paint.end();check(image.save(path),"save layout evidence");}
int main(int argc,char **argv){
 int qtargc=1;char *qtargv[]={argv[0],nullptr};QApplication app(qtargc,qtargv);QCoreApplication::setOrganizationName("RS24Regression");QCoreApplication::setApplicationName("RS24Regression");QTemporaryDir prefs;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,prefs.path());QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 try{
 const QString out=argv[1],realFile=argv[2],csvPath=argv[3],source=argv[4];QDir().mkpath(out);
 check(Settings::fromMap({{"distance",-4},{"maxMove",999}}).distance==0,"negative clearance clamped");check(Settings::fromMap({{"distance","NaN"}}).distance==4,"nonfinite clearance uses safe default");
 QETProject fixture(source+"/tests/qttest/fixtures/ten_sequential_numbers.qet");QETApp::registerProject(&fixture);check(fixture.state()==QETProject::Ok,"fixture opens");auto *page=fixture.diagrams().first();auto wires=page->conductors();check(!wires.isEmpty(),"fixture has conductors");auto *wire=wires.first();
 QPainterPath path;path.moveTo(200,250);path.lineTo(700,250);wire->setPath(path);auto prop=wire->properties();prop.cond_size=4;wire->setProperties(prop);
 auto *text=new IndependentTextItem("Test description");page->addItem(text);text->setFont(QFont("DejaVu Sans",10));text->setPos(400,235);Settings s;s.keepSide=false;auto before=text->pos();auto html=text->toHtml();page->clearSelection();text->setSelected(true);
 check(textPath(text).intersects(wirePath(wire,s.distance)),"test label touches expanded wire edge");auto result=plan({page},s,true);check(!result.changes.isEmpty(),"selected-label correction found");check(text->pos()==before,"planning never moves source labels");int index=fixture.undoStack()->index();apply(&fixture,result,s);check(fixture.undoStack()->index()==index+1,"batch correction is one Undo");check(!textPath(text).intersects(wirePath(wire,s.distance)),"actual conductor width and requested clearance satisfied");check(text->toHtml()==html&&wire->properties()==prop&&wire->path()==path,"text and conductor data unchanged");const auto after=text->pos();check(QLineF(before,after).length()<=s.maxMove,"movement is bounded");fixture.undoStack()->undo();check(text->pos()==before,"Undo restores position");fixture.undoStack()->redo();check(text->pos()==after,"Redo restores correction");check(plan({page},s,true).changes.isEmpty(),"second correction is idempotent");
 text->setPos(before);text->setProperty("clearanceLocked24",true);result=plan({page},s,true);check(result.changes.isEmpty()&&result.locked==1,"locked labels remain in place and reported");text->setProperty("clearanceLocked24",false);
 Settings limited=s;limited.maxMove=1;result=plan({page},limited,true);check(result.changes.isEmpty()&&result.unresolved==1,"unresolvable conflicts reported without arbitrary relocation");
 s.descriptions=false;check(plan({page},s,true).considered==0,"description inclusion can be disabled");s.descriptions=true;
 // Rotated text / diagonal wires use scene paths, not horizontal bounding boxes.
 text->setRotation(90);text->setPos(450,220);path=QPainterPath();path.moveTo(350,150);path.lineTo(550,350);wire->setPath(path);result=plan({page},s,true);apply(&fixture,result,s);check(!textPath(text).intersects(wirePath(wire,s.distance))||result.unresolved>0,"diagonal geometry resolves or explicitly reports limitation");text->setRotation(0);
 // Aligned description words and a parent's cross-reference stay together.
 QList<Element*> anchors;for(auto *item:page->items())if(auto *e=dynamic_cast<Element*>(item))anchors<<e;
 check(!anchors.isEmpty(),"real symbol anchors available");auto *anchor=anchors.first();
 QList<DiagramTextItem*> stack;
 for(int i=0;i<3;++i){auto *word=new DynamicElementTextItem(anchor);anchor->addDynamicTextItem(word);word->setText(QStringList{"Motor","Protection","Lamp"}[i]);word->setFont(QFont("DejaVu Sans",10));word->setPos(anchor->mapFromScene(QPointF(350,350+i*17)));stack<<word;}
 path=QPainterPath();path.moveTo(200,365);path.lineTo(700,365);wire->setPath(path);page->clearSelection();stack.first()->setSelected(true);
 const auto difference=stack[1]->scenePos()-stack[0]->scenePos();result=plan({page},s,true);check(result.changes.size()==3,"selected word brings its aligned description stack");apply(&fixture,result,s);check(QLineF(stack[1]->scenePos()-stack[0]->scenePos(),difference).length()<.01,"relative multiline spacing retained");for(auto *word:stack){check(!textPath(word).intersects(wirePath(wire,s.distance)),"every word clears conductor after block movement");}
 for(auto *word:stack)word->setVisible(false);
 // Source text can have a translation position lock without being translatable.
 page->clearSelection();text->setSelected(true);text->setRotation(0);text->setPos(before);path=QPainterPath();path.moveTo(200,250);path.lineTo(700,250);wire->setPath(path);
 QVariantMap catalog;QetLanguagesUi16::scan(&fixture,catalog);auto entries=catalog["entries"].toMap();auto e=entries[text->languageId16()].toMap();e["variants"]=QVariantMap{{"en",QVariantMap{{"locked",true}}}};entries[text->languageId16()]=e;catalog["entries"]=entries;fixture.setLanguageData(catalog);check(plan({page},s,true).locked==1,"existing language position lock respected");e["variants"]=QVariantMap();entries[text->languageId16()]=e;catalog["entries"]=entries;fixture.setLanguageData(catalog);
 // Read-only sheets cannot be changed by project batch commands.
 const auto savedReadOnly=page->isReadOnly();fixture.setReadOnly(true);check(plan({page},s).changes.isEmpty(),"read-only sheet skipped");fixture.setReadOnly(savedReadOnly);
 // Automatic changes are coalesced, local, and do not fight Undo/Redo.
 path=QPainterPath();path.moveTo(200,250);path.lineTo(700,250);wire->setPath(path);text->setPos(400,200);s.automatic=true;setSettings(&fixture,s);fixture.undoStack()->push(new Move(text,before));flush(&fixture);check(!textPath(text).intersects(wirePath(wire,s.distance)),"automatic correction after user move");const auto automaticAfter=text->pos();const int autoCount=fixture.undoStack()->count();fixture.undoStack()->undo();flush(&fixture);check(text->pos()==before&&fixture.undoStack()->count()==autoCount&&fixture.undoStack()->canRedo(),"Undo automatic correction preserves redo branch and does not reapply");fixture.undoStack()->redo();flush(&fixture);check(text->pos()==automaticAfter,"Redo automatic correction stable");
 auto *otherPage=fixture.addNewDiagram();auto *far=new IndependentTextItem("Other folio");otherPage->addItem(far);far->setPos(200,250);const auto farBefore=far->pos();notify(text);flush(&fixture);check(far->pos()==farBefore,"local edit does not move other sheets");s.automatic=false;setSettings(&fixture,s);
 QString error;check(fixture.writeContainerFile(out+"/Clearance_fixture.qetz",&error),qPrintable(error));{QETProject loaded(out+"/Clearance_fixture.qetz");check(settings(&loaded).distance==s.distance&&!settings(&loaded).automatic,"project clearance settings survive QETZ reopen");}
 // Dialog preview is reversible and saving defaults does not apply the draft.
 const auto beforeDialog=text->pos();bool dialogSeen=false;QTimer::singleShot(0,[&]{auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *spin=dialog->findChild<QDoubleSpinBox*>("clearanceDistance24");if(!spin){dialog->reject();return;}dialogSeen=true;spin->setValue(7);dialog->findChild<QPushButton*>("clearancePreview24")->click();dialog->grab().save(out+"/Label_spacing_controls_2.4.png");dialog->findChild<QPushButton*>("clearanceSaveDefault24")->click();dialog->reject();});show(page);check(dialogSeen&&text->pos()==beforeDialog&&settings(&fixture).distance==4,"Cancel removes preview without changing project");{QETProject fresh;check(settings(&fresh).distance==7,"new project inherits saved default");}QSettings().remove("diagrameditor/labelClearance24");
 QETApp::unregisterProject(&fixture);
 // The real RS project, including contact labels, pin numbers and German text.
 QETProject real(realFile);QETApp::registerProject(&real);check(real.state()==QETProject::Ok,"actual RS safety project opens");auto pages=real.diagrams();real.buildFolios(pages);QHash<QString,QString> sources;QHash<Conductor*,QPainterPath> paths;QHash<Conductor*,ConductorProperties> properties;QHash<Element*,QPointF> elements;QHash<Element*,QList<Element*>> originalLinks;
 for(auto *d:pages)for(auto *item:d->items()){if(auto *t=dynamic_cast<DiagramTextItem*>(item))sources[t->languageId16()]=t->toHtml();if(auto *c=dynamic_cast<Conductor*>(item)){paths[c]=c->path();properties[c]=c->properties();}if(auto *e=dynamic_cast<Element*>(item)){elements[e]=e->pos();originalLinks[e]=e->linkedElements();}}
 QFile csv(csvPath);check(csv.open(QIODevice::ReadOnly),"German CSV available");QVariantMap data;QetLanguagesUi16::scan(&real,data);auto imported=QetLanguages16::importCsv(data,csv.readAll());check(imported.ok,"German translations import");data=imported.data;data["activeLanguage"]="de";QetLanguagesUi16::apply(&real,data,false);

 Settings actual;auto realPlan=plan(pages,actual);std::cout<<"Real project: "<<realPlan.considered<<" considered, "<<realPlan.changes.size()<<" moved, "<<realPlan.locked<<" locked, "<<realPlan.unresolved<<" unresolved, "<<realPlan.elapsedMs<<" ms"<<std::endl;
 check(realPlan.considered>50,"technical and descriptive labels are checked across project");apply(&real,realPlan,actual);
 for(auto *t:QetLanguagesUi16::texts(&real))if(sources.contains(t->languageId16()))check(t->toHtml()==sources[t->languageId16()],"spacing preserves all source text HTML");for(auto it=paths.begin();it!=paths.end();++it)check(it.key()->path()==it.value()&&it.key()->properties()==properties[it.key()],"spacing preserves wiring and conductor properties");for(auto it=elements.begin();it!=elements.end();++it)check(it.key()->pos()==it.value(),"spacing preserves symbol positions");
 for(auto it=originalLinks.begin();it!=originalLinks.end();++it)check(it.key()->linkedElements()==it.value(),"spacing retains all electrical object links");
 for(const auto &change:realPlan.changes){check(change.text&&QLineF(change.before,change.after).length()<=actual.maxMove+1,"real correction stays near label");for(auto *w:change.text->diagram()->conductors())check(!textPath(change.text).intersects(wirePath(w,actual.distance)),"all moved real labels satisfy clearance after final rendering");}
 // Both translated and technical labels retain their positions on reopen.
 QHash<QString,QPointF> movedPositions;for(const auto &change:realPlan.changes)movedPositions[change.text->languageId16()]=change.after;
 auto secondPass=plan(pages,actual);check(secondPass.changes.isEmpty(),"whole-project correction is stable on second pass");
 check(real.writeContainerFile(out+"/RS_NGX_Safety_1.7_DE_2.4_Test.qetz",&error),qPrintable(error));

 QJsonObject report{{"considered",realPlan.considered},{"moved",realPlan.changes.size()},{"locked",realPlan.locked},{"unresolved",realPlan.unresolved},{"fixed_symbol_text",realPlan.embeddedSymbols},{"elapsed_ms",realPlan.elapsedMs}};QJsonArray notes;for(const auto &issue:realPlan.issues)notes<<issue.message;report["review"]=notes;
 for(auto *d:pages)if(d->title()=="Control")render(d,out+"/Control_German_2.4.png");

 exportPdf(real,out+"/RS_NGX_Safety_2.4_Clickable.pdf",true);exportPdf(real,out+"/RS_NGX_Safety_2.4_No_links.pdf",false);

 {QETProject loaded(out+"/RS_NGX_Safety_1.7_DE_2.4_Test.qetz");QETApp::registerProject(&loaded);for(auto *item:QetLanguagesUi16::texts(&loaded))if(movedPositions.contains(item->languageId16()))check(QLineF(item->pos(),movedPositions[item->languageId16()]).length()<.02,"German corrected position survives reopen");QETApp::unregisterProject(&loaded);}
 // Native cross-reference relationships, including hyperlinks on moved labels.
 {QETProject demo(source+"/tests/qttest/fixtures/pdf_links24.qet");QETApp::registerProject(&demo);auto demoPages=demo.diagrams();demo.buildFolios(demoPages);check(demoPages.size()==2,"two-sheet hyperlink fixture opens");int links=0;for(auto *d:demoPages)for(auto *item:d->items())if(auto *e=dynamic_cast<Element*>(item)){links+=e->linkedElements().size();if(e->linkType()==Element::Slave)for(auto *t:e->dynamicTextItems())if(t->slaveXrefItem())t->setPos(t->pos()+QPointF(35,20));}check(links>=4,"coil/contact and folio partners linked in project");
 check(demo.writeContainerFile(out+"/PDF_Link_Demo_2.4.qetz",&error),"save linked demonstration project");
 exportPdf(demo,out+"/PDF_Link_Demo_2.4.pdf",true);exportPdf(demo,out+"/PDF_Link_Demo_2.4_Off.pdf",false);

 QETApp::unregisterProject(&demo);}

 {QETProject mixed(source+"/tests/qttest/fixtures/pdf_links24_mixed.qet");QETApp::registerProject(&mixed);auto sheets=mixed.diagrams();check(sheets.size()==2,"mixed demonstration opens");check(sheets[1]->border_and_titleblock.borderAndTitleBlockRect().height()>sheets[1]->border_and_titleblock.borderAndTitleBlockRect().width(),"mixed PDF target is portrait");for(auto *d:sheets)for(auto *item:d->items())if(auto *e=dynamic_cast<Element*>(item))if(e->linkType()==Element::Slave)for(auto *t:e->dynamicTextItems())if(t->slaveXrefItem())t->setPos(t->pos()+QPointF(35,20));exportPdf(mixed,out+"/Mixed_orientation_links_2.4.pdf",true);QETApp::unregisterProject(&mixed);}

 QFile json(out+"/validation24.json");check(json.open(QIODevice::WriteOnly),"write validation report");json.write(QJsonDocument(report).toJson());
 QETApp::unregisterProject(&real);std::cout<<"PASS "<<checks<<" Custom 2.4 spacing and PDF checks"<<std::endl;return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<std::endl;return 1;}
}
