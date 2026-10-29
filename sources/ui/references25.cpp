// SPDX-License-Identifier: GPL-2.0-or-later
#include "references25.h"
#include "../diagram.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/diagramtextitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../undocommand/adddiagramcommand.h"
#include "../undocommand/linkelementcommand.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"

namespace QetReferences25 {
static QVariantList rectData(const QRectF &r){return {r.x(),r.y(),r.width(),r.height()};}
static QRectF rect(const QVariant &v){auto a=v.toList();return a.size()==4?QRectF(a[0].toDouble(),a[1].toDouble(),a[2].toDouble(),a[3].toDouble()):QRectF();}
static Diagram *sheet(QETProject *p,const QString &id){for(auto *d:p->diagrams())if(d->uuid().toString()==id)return d;return nullptr;}
static QString identity(QGraphicsItem *i){
 if(auto *text=dynamic_cast<DiagramTextItem*>(i))return text->languageId16();
 if(auto *e=dynamic_cast<Element*>(i))return e->uuid().toString();
 if(auto *image=dynamic_cast<DiagramImageItem*>(i))return image->uuid().toString();
 if(auto *shape=dynamic_cast<QetShapeItem*>(i))return shape->uuid().toString();
 return {};
}
static QString description(QGraphicsItem *i){
 QString text;if(auto *t=dynamic_cast<DiagramTextItem*>(i))text=t->toPlainText().simplified().left(80);
 else if(auto *e=dynamic_cast<Element*>(i))text=e->elementInformations().value("label").toString();
 else if(dynamic_cast<DiagramImageItem*>(i))text=QObject::tr("Image");
 else text=QObject::tr("Shape");
 if(text.isEmpty())text=QObject::tr("Unnamed object");
 if(auto *d=qobject_cast<Diagram*>(i->scene()))text+=" - "+d->convertPosition(i->sceneBoundingRect().center()).toString();
 return text;
}
static QGraphicsItem *item(Diagram *d,const QString &id){
 if(!d||id.isEmpty())return nullptr;
 for(auto *i:d->items()){
  if(identity(i)==id)return i;
  if(auto *image=dynamic_cast<DiagramImageItem*>(i))if(image->uuid().toString()==id)return image;
  if(auto *text=dynamic_cast<DiagramTextItem*>(i))if(text->languageId16()==id)return text;
  if(auto *e=dynamic_cast<Element*>(i))if(e->uuid().toString()==id)return e;
 }
 return nullptr;
}
QList<Link> links(Diagram *d){
 QList<Link> result;if(!d||!d->project())return result;
 for(auto value:d->project()->languageData().value("navigation25").toList()){
  auto v=value.toMap();if(v.value("sourceSheet").toString()!=d->uuid().toString())continue;
  auto *target=sheet(d->project(),v.value("targetSheet").toString());if(!target)continue;
  QRectF source=rect(v.value("sourceRect")),destination=v.value("targetSheetWhole").toBool()?target->border_and_titleblock.insideBorderRect():rect(v.value("targetRect"));
  if(v.contains("sourceItem")){auto *i=item(d,v.value("sourceItem").toString());if(!i)continue;source=i->mapRectToScene(v.value("sourceWhole").toBool()?i->boundingRect():source);}
  if(v.contains("targetItem")){auto *i=item(target,v.value("targetItem").toString());if(!i)continue;destination=i->mapRectToScene(v.value("targetWhole").toBool()?i->boundingRect():destination);}
  if(source.isValid())result<<Link{source,target,destination};
 }
 return result;
}
bool follow(Diagram *d,const QPointF &point){
 for(auto link:links(d))if(link.source.contains(point)){
  QTransform previous;if(!d->views().isEmpty())previous=d->views().first()->transform();
  link.target->showMe();
  auto mode=QSettings().value("pdf25/view","page").toString();
  for(auto *view:link.target->views()){
   if(mode=="page")view->fitInView(link.target->border_and_titleblock.insideBorderRect(),Qt::KeepAspectRatio);
   else if(mode=="area"){qreal margin=QSettings().value("pdf25/padding",25).toDouble();view->fitInView(link.destination.adjusted(-margin,-margin,margin,margin),Qt::KeepAspectRatio);}
   else {if(mode=="keep")view->setTransform(previous);if(mode=="custom"){view->resetTransform();qreal z=QSettings().value("pdf25/zoom",100).toDouble()/100.0;view->scale(z,z);}view->centerOn(link.destination.center());}
  }
  return true;
 }
 return false;
}
void settings(QWidget *parent){
 QDialog d(parent);d.setWindowTitle(QObject::tr("Link destination view"));auto *f=new QFormLayout(&d);
 auto *mode=new QComboBox;mode->addItem(QObject::tr("Whole page"),"page");mode->addItem(QObject::tr("Keep current zoom"),"keep");mode->addItem(QObject::tr("Custom zoom"),"custom");mode->addItem(QObject::tr("Fit component area"),"area");mode->setCurrentIndex(qMax(0,mode->findData(QSettings().value("pdf25/view","page"))));f->addRow(QObject::tr("View"),mode);
 auto *zoom=new QSpinBox;zoom->setRange(10,1600);zoom->setSuffix(" %");zoom->setValue(QSettings().value("pdf25/zoom",100).toInt());f->addRow(QObject::tr("Custom zoom"),zoom);
 auto *padding=new QSpinBox;padding->setRange(0,1000);padding->setValue(QSettings().value("pdf25/padding",25).toInt());f->addRow(QObject::tr("Component area margin"),padding);
 auto *b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);f->addRow(b);QObject::connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);QObject::connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);
 if(d.exec()==QDialog::Accepted){QSettings s;s.setValue("pdf25/view",mode->currentData());s.setValue("pdf25/zoom",zoom->value());s.setValue("pdf25/padding",padding->value());}
}
static void saveLinks(QETProject *p,const QVariantList &added){
 auto before=p->languageData(),after=before;auto values=after.value("navigation25").toList();for(auto v:added){auto link=v.toMap();if(link.value("sourceWhole").toBool())values.removeIf([&](const QVariant &old){auto o=old.toMap();return o.value("sourceWhole").toBool()&&o.value("sourceSheet")==link.value("sourceSheet")&&o.value("sourceItem")==link.value("sourceItem");});if(!values.contains(v))values<<v;}after["navigation25"]=values;
 if(before!=after){auto *command=new QUndoCommand(QObject::tr("Save schematic navigation"));new QPropertyUndoCommand(p,"languageData",before,after,command);p->undoStack()->push(command);}
}
bool assign(DiagramTextItem *source,Diagram *target,QGraphicsItem *object){
 if(!source||!source->diagram()||source->diagram()->isReadOnly())return false;
 auto *project=source->diagram()->project();if(!project)return false;
 if(target&&(target->project()!=project||(object&&(object->scene()!=target||identity(object).isEmpty()))))return false;
 if(!target&&object)return false;
 auto before=project->languageData(),after=before;auto values=after.value("navigation25").toList();
 const QString sourceId=source->languageId16(),sourceSheet=source->diagram()->uuid().toString();
 values.removeIf([&](const QVariant &value){auto v=value.toMap();return v.value("sourceSheet").toString()==sourceSheet&&v.value("sourceItem").toString()==sourceId&&v.value("sourceWhole").toBool();});
 if(target){QVariantMap v{{"sourceSheet",sourceSheet},{"sourceItem",sourceId},{"sourceWhole",true},{"targetSheet",target->uuid().toString()}};if(object){v["targetItem"]=identity(object);v["targetWhole"]=true;}else v["targetSheetWhole"]=true;values<<v;}
 after["navigation25"]=values;if(before==after)return false;
 auto *command=new QUndoCommand(target?QObject::tr("Assign reference destination"):QObject::tr("Remove reference navigation"));new QPropertyUndoCommand(project,"languageData",before,after,command);project->undoStack()->push(command);return true;
}
void editSelected(Diagram *current,QWidget *parent){
 if(!current||!current->project()||current->isReadOnly())return;
 QList<DiagramTextItem*> sources;for(auto *i:current->selectedItems())if(auto *t=dynamic_cast<DiagramTextItem*>(i))sources<<t;
 if(sources.size()!=1){QMessageBox::information(parent,QObject::tr("Assign reference destination"),QObject::tr("Select one reference text on the sheet, such as /2.1, then use this command."));return;}
 auto *source=sources.first();auto *project=current->project();QVariantMap existing;
 for(auto value:project->languageData().value("navigation25").toList()){auto v=value.toMap();if(v.value("sourceItem").toString()==source->languageId16()&&v.value("sourceWhole").toBool())existing=v;}
 QDialog dialog(parent);dialog.setObjectName("referenceDestination25");dialog.setWindowTitle(QObject::tr("Assign reference destination"));dialog.resize(680,250);auto *layout=new QFormLayout(&dialog);
 auto *note=new QLabel(QObject::tr("Navigation only: this does not create an electrical relationship. Ctrl-click the source text to follow the link."));note->setWordWrap(true);layout->addRow(note);layout->addRow(QObject::tr("Source text"),new QLabel(source->toPlainText()));
 auto *sheets=new QComboBox;sheets->setObjectName("referenceSheet25");for(auto *d:project->diagrams())sheets->addItem(d->border_and_titleblock.finalfolio()+" - "+d->border_and_titleblock.title(),d->uuid().toString());layout->addRow(QObject::tr("Drawing sheet"),sheets);
 auto *objects=new QComboBox;objects->setObjectName("referenceObject25");layout->addRow(QObject::tr("Destination object (optional)"),objects);
 auto populate=[&]{objects->clear();objects->addItem(QObject::tr("Whole sheet"),QString());auto *d=sheet(project,sheets->currentData().toString());if(!d)return;QList<QPair<QString,QString>> labels;for(auto *i:d->items())if(i!=source&&!identity(i).isEmpty())labels<<qMakePair(description(i),identity(i));std::sort(labels.begin(),labels.end());for(auto entry:labels)objects->addItem(entry.first,entry.second);};
 QObject::connect(sheets,QOverload<int>::of(&QComboBox::currentIndexChanged),&dialog,[&]{populate();});sheets->setCurrentIndex(qMax(0,sheets->findData(existing.value("targetSheet",current->uuid().toString()))));populate();objects->setCurrentIndex(qMax(0,objects->findData(existing.value("targetItem",QString()))));
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);auto *remove=buttons->addButton(QObject::tr("Remove navigation link"),QDialogButtonBox::DestructiveRole);remove->setEnabled(!existing.isEmpty());layout->addRow(buttons);
 QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);QObject::connect(remove,&QPushButton::clicked,&dialog,[&]{dialog.done(2);});
 int result=dialog.exec();if(result==2){assign(source,nullptr);return;}if(result!=QDialog::Accepted)return;auto *target=sheet(project,sheets->currentData().toString());if(target)assign(source,target,item(target,objects->currentData().toString()));
}
static void detect(Diagram *current,QWidget *parent){
 auto *project=current->project();QDialog d(parent);d.setWindowTitle(QObject::tr("Review printed references"));d.resize(760,450);auto *layout=new QVBoxLayout(&d);
 auto *note=new QLabel(QObject::tr("Resolve /sheet.column using drawing sheet labels. Choose a destination for each reference. Ctrl-click a linked reference on the sheet to follow it."));note->setWordWrap(true);layout->addWidget(note);
 auto *table=new QTableWidget(0,3);table->setHorizontalHeaderLabels({QObject::tr("Use"),QObject::tr("Reference"),QObject::tr("Destination sheet")});layout->addWidget(table);
 QList<QVariantMap> candidates;QRegularExpression re("/([A-Za-z0-9_-]+)\\.([0-9]+)");
 for(auto *source:project->diagrams())for(auto *i:source->items())if(auto *text=dynamic_cast<DiagramTextItem*>(i)){
  auto match=re.match(text->toPlainText());if(!match.hasMatch())continue;
  // Several references in one text box cannot share its entire click area.
  auto matches=re.globalMatch(text->toPlainText());int count=0;while(matches.hasNext()){matches.next();++count;}if(count!=1)continue;
  int row=table->rowCount();table->insertRow(row);auto *check=new QTableWidgetItem;check->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);check->setCheckState(Qt::Unchecked);table->setItem(row,0,check);table->setItem(row,1,new QTableWidgetItem(match.captured()));
  auto *dest=new QComboBox;dest->addItem(QObject::tr("Unresolved - choose sheet"),QString());int unique=-1,number=0;
  for(auto *target:project->diagrams()){dest->addItem(target->border_and_titleblock.finalfolio()+" - "+target->border_and_titleblock.title(),target->uuid().toString());if(target->border_and_titleblock.finalfolio()==match.captured(1)||target->border_and_titleblock.folio()==match.captured(1)){unique=dest->count()-1;++number;}}
  if(number==1){dest->setCurrentIndex(unique);check->setCheckState(Qt::Checked);}
  table->setCellWidget(row,2,dest);
  candidates<<QVariantMap{{"sourceSheet",source->uuid().toString()},{"sourceItem",text->languageId16()},{"sourceWhole",true},{"column",match.captured(2).toInt()}};
 }
 table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);table->setColumnWidth(2,350);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
 if(d.exec()!=QDialog::Accepted)return;
 QVariantList added;
 for(int r=0;r<candidates.size();++r)if(table->item(r,0)->checkState()==Qt::Checked){QString id=static_cast<QComboBox*>(table->cellWidget(r,2))->currentData().toString();auto *target=sheet(project,id);if(!target)continue;auto v=candidates[r];auto area=target->border_and_titleblock.insideBorderRect();int c=v.take("column").toInt();if(c<1||c>target->border_and_titleblock.columnsCount())continue;qreal width=target->border_and_titleblock.columnsWidth();v["targetSheet"]=id;v["targetRect"]=rectData(QRectF(area.left()+(c-1)*width,area.top(),width,area.height()));added<<v;}
 saveLinks(project,added);
}
static void reviewUnresolved(Diagram *current,QWidget *parent){
 auto *project=current->project();auto before=project->languageData();auto issues=before.value("navigationIssues25").toList();
 if(issues.isEmpty()){QMessageBox::information(parent,QObject::tr("Unresolved references"),QObject::tr("There are no saved unresolved PDF references."));return;}
 QDialog dialog(parent);dialog.setWindowTitle(QObject::tr("Review unresolved PDF references"));dialog.resize(950,450);auto *layout=new QVBoxLayout(&dialog);
 auto *note=new QLabel(QObject::tr("Choose a destination and its drawing column count to resolve a printed reference. Unresolved entries remain saved in this project. External or unsupported PDF actions are listed for information only."));note->setWordWrap(true);layout->addWidget(note);
 auto *table=new QTableWidget(issues.size(),4);table->setHorizontalHeaderLabels({QObject::tr("Reference / reason"),QObject::tr("Source sheet"),QObject::tr("Destination sheet"),QObject::tr("Columns")});layout->addWidget(table);
 for(int r=0;r<issues.size();++r){auto v=issues[r].toMap();auto *source=sheet(project,v.value("sourceSheet").toString());table->setItem(r,0,new QTableWidgetItem(v.value("text").toString()+" "+v.value("reason").toString()));table->setItem(r,1,new QTableWidgetItem(source?source->border_and_titleblock.title():QObject::tr("Removed sheet")));auto *choice=new QComboBox;choice->addItem(QObject::tr("Keep unresolved"),QString());for(auto *d:project->diagrams())choice->addItem(d->border_and_titleblock.finalfolio()+" - "+d->border_and_titleblock.title(),d->uuid().toString());choice->setEnabled(source&&v.contains("column"));table->setCellWidget(r,2,choice);auto *count=new QSpinBox;count->setRange(1,100);count->setValue(10);table->setCellWidget(r,3,count);}
 table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);table->setColumnWidth(2,280);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);if(dialog.exec()!=QDialog::Accepted)return;
 QVariantList pending,added;
 for(int r=0;r<issues.size();++r){auto v=issues[r].toMap();auto *target=sheet(project,static_cast<QComboBox*>(table->cellWidget(r,2))->currentData().toString());int col=v.value("column").toInt(),columns=static_cast<QSpinBox*>(table->cellWidget(r,3))->value();if(!target||!v.contains("column")||col<1||col>columns){pending<<v;continue;}auto area=target->border_and_titleblock.insideBorderRect();v["targetSheet"]=target->uuid().toString();v["targetRect"]=rectData(QRectF(area.left()+area.width()*(col-1)/columns,area.top(),area.width()/columns,area.height()));v.remove("column");v.remove("reason");v.remove("text");added<<v;}
 auto after=before;auto values=after.value("navigation25").toList();values.append(added);after["navigation25"]=values;after["navigationIssues25"]=pending;
 if(before!=after){auto *command=new QUndoCommand(QObject::tr("Resolve saved PDF references"));new QPropertyUndoCommand(project,"languageData",before,after,command);project->undoStack()->push(command);}
}
void importPdf(Diagram *current,QWidget *parent,const QString &fileName){
 if(!current||!current->project()||current->project()->isReadOnly())return;
 QString path=fileName.isEmpty()?QFileDialog::getOpenFileName(parent,QObject::tr("Import PDF with navigation"),{},QObject::tr("PDF files (*.pdf)")):fileName;if(path.isEmpty())return;
 QString base=QCoreApplication::applicationDirPath();QString python=base+"/pdf-runtime/python.exe",script=base+"/tools/pdf_references25.py";
 if(!QFile::exists(python)||!QFile::exists(script)){QMessageBox::warning(parent,QObject::tr("PDF import"),QObject::tr("The PDF import runtime is missing from this portable application."));return;}
 QTemporaryDir tmp;if(!tmp.isValid())return;
 QProcess process;process.start(python,{script,path,tmp.path()});if(!process.waitForStarted(5000)){QMessageBox::warning(parent,QObject::tr("PDF import"),QObject::tr("The PDF reader could not start: %1").arg(process.errorString()));return;}QProgressDialog progress(QObject::tr("Reading PDF pages and internal hyperlinks..."),QObject::tr("Cancel"),0,0,parent);progress.setWindowModality(Qt::WindowModal);
 while(process.state()!=QProcess::NotRunning&&!process.waitForFinished(100)){QApplication::processEvents();if(progress.wasCanceled()){process.kill();process.waitForFinished();return;}}progress.close();
 if(process.exitStatus()!=QProcess::NormalExit||process.exitCode()!=0){QMessageBox::warning(parent,QObject::tr("PDF import"),QString::fromUtf8(process.readAllStandardError()));return;}
 QFile manifest(tmp.path()+"/manifest.json");if(!manifest.open(QIODevice::ReadOnly))return;auto data=QJsonDocument::fromJson(manifest.readAll()).toVariant().toMap();auto pages=data.value("pages").toList();if(pages.isEmpty())return;
 QDialog review(parent);review.setWindowTitle(QObject::tr("PDF pages and sheet labels"));review.resize(700,450);auto *layout=new QVBoxLayout(&review);
 auto *note=new QLabel(QObject::tr("Pages are imported as page images with saved navigation. Enter drawing sheet labels and column counts to resolve printed /sheet.column references. Existing internal PDF hyperlinks already identify their destination pages."));note->setWordWrap(true);layout->addWidget(note);
 auto *table=new QTableWidget(pages.size(),3);table->setHorizontalHeaderLabels({QObject::tr("PDF page"),QObject::tr("Drawing sheet label"),QObject::tr("Drawing columns")});layout->addWidget(table);
 for(int i=0;i<pages.size();++i){auto page=pages[i].toMap();auto *n=new QTableWidgetItem(QString::number(i+1));n->setFlags(Qt::ItemIsEnabled);table->setItem(i,0,n);table->setItem(i,1,new QTableWidgetItem(page.value("label").toString()));auto *count=new QSpinBox;count->setRange(1,100);count->setValue(10);table->setCellWidget(i,2,count);}
 auto *preserve=new QCheckBox(QObject::tr("Preserve existing internal hyperlinks"));preserve->setChecked(true);layout->addWidget(preserve);
 auto *printed=new QCheckBox(QObject::tr("Detect printed /sheet.column references using these labels"));printed->setChecked(true);layout->addWidget(printed);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&review,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&review,&QDialog::reject);if(review.exec()!=QDialog::Accepted)return;
 QList<QPixmap> pixels;for(auto p:pages){QPixmap pix(tmp.path()+"/"+p.toMap().value("image").toString());if(pix.isNull()){QMessageBox::warning(parent,QObject::tr("PDF import"),QObject::tr("A rendered page could not be read."));return;}pixels<<pix;}
 auto *project=current->project();auto *command=new QUndoCommand(QObject::tr("Import PDF pages and navigation"));QList<Diagram*> sheets;QList<DiagramImageItem*> images;QHash<QString,QList<int>> labels;
 for(int i=0;i<pages.size();++i){
  auto page=pages[i].toMap();auto *d=project->createDetachedDiagram25();auto *image=new DiagramImageItem(pixels[i]);d->addItem(image);image->setScaleFactorX(0.5);image->setScaleFactorY(0.5);image->setPos(-image->mapRectToParent(image->imageRect()).topLeft());
  // QET requires at least three grid rows and columns, even when hidden.
  BorderProperties border;border.columns_count=3;border.columns_width=(page.value("width").toDouble()-border.rows_header_width)/3;border.rows_count=3;border.rows_height=(page.value("height").toDouble()-border.columns_header_height)/3;border.display_columns=border.display_rows=false;
  d->border_and_titleblock.importBorder(border);image->setPos(image->pos()+d->border_and_titleblock.insideBorderRect().topLeft()-image->mapRectToScene(image->imageRect()).topLeft());d->border_and_titleblock.displayBorder(false);d->border_and_titleblock.displayTitleBlock(false);
  auto title=project->defaultTitleBlockProperties();title.title=QFileInfo(path).baseName()+" - "+QString::number(i+1);title.folio=table->item(i,1)->text().trimmed();d->border_and_titleblock.importTitleBlock(title);
  new AddDiagramCommand(project,d,-1,command);sheets<<d;images<<image;if(!title.folio.isEmpty())labels[title.folio]<<i;
 }
 QVariantList added,issues;int unresolved=0;QHash<int,QList<QRectF>> nativeAreas;
 auto sourceData=[&](int source,const QVariantList &coords){QVariantMap v;if(source<0||source>=sheets.size())return v;v["sourceSheet"]=sheets[source]->uuid().toString();v["sourceItem"]=images[source]->uuid().toString();if(coords.size()==4){QRectF src(coords[0].toDouble()*2,coords[1].toDouble()*2,(coords[2].toDouble()-coords[0].toDouble())*2,(coords[3].toDouble()-coords[1].toDouble())*2);src.translate(images[source]->imageRect().topLeft());v["sourceRect"]=rectData(src);}return v;};
 for(auto value:data.value("issues").toList()){auto issue=value.toMap();auto v=sourceData(issue.value("page").toInt(),{});v["reason"]=issue.value("reason");issues<<v;}
 auto add=[&](int source,int target,const QVariantList &coords,const QPointF &point){
  if(source<0||source>=sheets.size()||target<0||target>=sheets.size()||coords.size()!=4){++unresolved;return;}
  QRectF src(coords[0].toDouble()*2,coords[1].toDouble()*2,(coords[2].toDouble()-coords[0].toDouble())*2,(coords[3].toDouble()-coords[1].toDouble())*2);
  src.translate(images[source]->imageRect().topLeft());
  added<<QVariantMap{{"sourceSheet",sheets[source]->uuid().toString()},{"sourceItem",images[source]->uuid().toString()},{"sourceRect",rectData(src)},{"targetSheet",sheets[target]->uuid().toString()},{"targetItem",images[target]->uuid().toString()},{"targetRect",rectData(QRectF(images[target]->imageRect().topLeft()+point*2,QSizeF(2,2)))}};
 };
 if(preserve->isChecked())for(auto value:data.value("links").toList()){auto l=value.toMap();auto p=l.value("targetPoint").toList(),r=l.value("sourceRect").toList();int source=l.value("sourcePage").toInt();add(source,l.value("targetPage").toInt(),r,QPointF(p.value(0).toDouble(),p.value(1).toDouble()));if(r.size()==4)nativeAreas[source]<<QRectF(QPointF(r[0].toDouble(),r[1].toDouble()),QPointF(r[2].toDouble(),r[3].toDouble()));}
 if(printed->isChecked())for(auto value:data.value("printed").toList()){auto l=value.toMap();int source=l.value("sourcePage").toInt();auto coords=l.value("sourceRect").toList();bool covered=false;if(coords.size()==4){QPointF centre((coords[0].toDouble()+coords[2].toDouble())/2,(coords[1].toDouble()+coords[3].toDouble())/2);for(auto area:nativeAreas.value(source))if(area.contains(centre)){covered=true;break;}}if(covered)continue;
 auto matches=labels.value(l.value("sheet").toString());int col=l.value("column").toInt();QString reason;if(matches.size()!=1)reason=QObject::tr("Drawing sheet label is missing or ambiguous.");else if(col<1||col>static_cast<QSpinBox*>(table->cellWidget(matches.first(),2))->value())reason=QObject::tr("Column is outside the destination drawing.");
 if(!reason.isEmpty()){auto v=sourceData(source,coords);v["column"]=col;v["text"]=l.value("text");v["reason"]=reason;issues<<v;continue;}int target=matches.first(),cols=static_cast<QSpinBox*>(table->cellWidget(target,2))->value();auto p=pages[target].toMap();add(source,target,coords,QPointF(p.value("width").toDouble()*(col-0.5)/cols,p.value("height").toDouble()/2));}
 unresolved+=issues.size();
 auto before=project->languageData(),after=before;auto values=after.value("navigation25").toList();values.append(added);after["navigation25"]=values;auto savedIssues=after.value("navigationIssues25").toList();savedIssues.append(issues);after["navigationIssues25"]=savedIssues;
 new QPropertyUndoCommand(project,"languageData",before,after,command);project->undoStack()->push(command);sheets.first()->showMe();
 QMessageBox::information(parent,QObject::tr("PDF navigation imported"),QObject::tr("%1 pages and %2 navigation links imported. %3 references could not be resolved. Review them later using Review unresolved PDF references. Ctrl-click a linked area to navigate.").arg(sheets.size()).arg(added.size()).arg(unresolved));
}

static void deviceRelationships(Diagram *current,QWidget *parent){
 auto *project=current->project();QList<Element*> masters,contacts;
 for(auto *d:project->diagrams())for(auto *i:d->items())if(auto *e=dynamic_cast<Element*>(i)){if(e->linkType()==Element::Master)masters<<e;else if(e->linkType()==Element::Slave&&e->linkedElements().isEmpty())contacts<<e;}
 auto label=[](Element *e){return e->elementInformations().value("label").toString().trimmed();};
 auto qualified=[&](Element *e){auto info=e->elementInformations();return label(e)+"|"+info.value("installation").toString()+"|"+info.value("location").toString();};
 QDialog dialog(parent);dialog.setWindowTitle(QObject::tr("Review contact / coil relationships"));dialog.resize(800,450);auto *layout=new QVBoxLayout(&dialog);
 auto *note=new QLabel(QObject::tr("Only native symbols with supported contact / master types can be linked. Unique matching device labels are suggested. Review each destination before applying. PDF page images contain navigation only."));note->setWordWrap(true);layout->addWidget(note);
 auto *table=new QTableWidget(contacts.size(),3);table->setHorizontalHeaderLabels({QObject::tr("Use"),QObject::tr("Contact"),QObject::tr("Coil / master")});layout->addWidget(table);
 for(int row=0;row<contacts.size();++row){auto *contact=contacts[row];auto *check=new QTableWidgetItem;check->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);check->setCheckState(Qt::Unchecked);table->setItem(row,0,check);table->setItem(row,1,new QTableWidgetItem(label(contact)+" - "+contact->diagram()->border_and_titleblock.finalfolio()));
  auto *choice=new QComboBox;choice->addItem(QObject::tr("Choose master"),-1);int matches=0,index=0;
  for(int n=0;n<masters.size();++n)if(LinkElementCommand::isLinkable(contact,masters[n])){choice->addItem(label(masters[n])+" - "+masters[n]->diagram()->border_and_titleblock.finalfolio(),n);if(!label(contact).isEmpty()&&qualified(contact)==qualified(masters[n])){++matches;index=choice->count()-1;}}
  if(matches==1){choice->setCurrentIndex(index);check->setCheckState(Qt::Checked);}table->setCellWidget(row,2,choice);
 }
 table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);table->setColumnWidth(2,360);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);if(dialog.exec()!=QDialog::Accepted)return;
 auto *command=new QUndoCommand(QObject::tr("Link reviewed contacts to masters"));
 for(int row=0;row<contacts.size();++row)if(table->item(row,0)->checkState()==Qt::Checked){int n=static_cast<QComboBox*>(table->cellWidget(row,2))->currentData().toInt();if(n<0||n>=masters.size())continue;auto *link=new LinkElementCommand(contacts[row],command);link->setLink(masters[n]);}
 if(command->childCount())project->undoStack()->push(command);else delete command;
}
void show(Diagram *d,QWidget *parent){
 if(!d||!d->project()||d->project()->isReadOnly())return;
 QDialog dialog(parent);dialog.setWindowTitle(QObject::tr("Import / rebuild schematic references"));auto *layout=new QVBoxLayout(&dialog);
 auto *pdf=new QPushButton(QObject::tr("Import PDF pages and hyperlinks..."));auto *detectButton=new QPushButton(QObject::tr("Detect printed references in this project..."));auto *view=new QPushButton(QObject::tr("Link destination view..."));
 layout->addWidget(pdf);layout->addWidget(detectButton);layout->addWidget(view);
 auto *manual=new QPushButton(QObject::tr("Assign destination to selected reference text..."));layout->addWidget(manual);QObject::connect(manual,&QPushButton::clicked,&dialog,[&]{dialog.accept();editSelected(d,parent);});
 auto *unresolvedButton=new QPushButton(QObject::tr("Review unresolved PDF references..."));layout->addWidget(unresolvedButton);QObject::connect(unresolvedButton,&QPushButton::clicked,&dialog,[&]{dialog.accept();reviewUnresolved(d,parent);});
 auto *devices=new QPushButton(QObject::tr("Review native contact / coil relationships..."));layout->addWidget(devices);QObject::connect(devices,&QPushButton::clicked,&dialog,[&]{dialog.accept();deviceRelationships(d,parent);});
 QObject::connect(pdf,&QPushButton::clicked,&dialog,[&]{dialog.accept();importPdf(d,parent);});
 QObject::connect(detectButton,&QPushButton::clicked,&dialog,[&]{dialog.accept();detect(d,parent);});
 QObject::connect(view,&QPushButton::clicked,&dialog,[&]{settings(&dialog);});dialog.exec();
}
}

