/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QET_TABLEQUICK20_H
#define QET_TABLEQUICK20_H
#include <QtWidgets>
#include "tabledefaults18.h"
#include "tableformat13.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
namespace QetTableQuick20 {
inline void put(QVariantMap &map,const QString &key,const QVariant &value){map[key]=value;}
// Selected body rows/cells are absolute model positions, like existing row overrides.
inline void clearInherited25(QVariantMap &,const QString &,const QString &);
inline QVariantMap change(QVariantMap v,const QString &scope,int column,int row,const QSet<QPair<int,int>> &cells,const QString &key,const QVariant &value){
 if(scope=="all"||scope=="body"||scope=="header"||scope=="title")clearInherited25(v,scope,key);
 auto apply=[&](QVariantMap &target,bool cell,const QString &part){
  if(key=="font"||key=="color"){const QString name=cell?"style":part+"Style";auto style=target.value(name).toMap();style[key]=value;target[name]=style;}
  else if(key=="background")target[cell?"background":part+"Background"]=value;
  else {const QString name=part+"Alignment20";if(cell)target[key]=value;else{auto a=target.value(name).toMap();a[key]=value;target[name]=a;}}
 };
 if(scope=="cells"){auto overrides=v.value("cellOverrides20").toMap();for(const auto &cell:cells){if(cell.second==-2){apply(v,false,"title");continue;}QString id=QString::number(cell.second)+":"+QString::number(cell.first);auto o=overrides.value(id).toMap();apply(o,true,{});overrides[id]=o;}v["cellOverrides20"]=overrides;}
 else if(scope=="column"){auto overrides=v.value("columnOverrides17").toList();while(overrides.size()<=column)overrides<<QVariantMap();if(column>=0){auto o=overrides[column].toMap();apply(o,false,"body");apply(o,false,"header");overrides[column]=o;}v["columnOverrides17"]=overrides;}
 else if(scope=="row"){if(row>=0){auto overrides=v.value("rowOverrides17").toMap();auto o=overrides.value(QString::number(row)).toMap();apply(o,false,"body");overrides[QString::number(row)]=o;v["rowOverrides17"]=overrides;}}
 else if(scope=="all"){for(const auto &part:{"body","header","title"})apply(v,false,part);}
 else apply(v,false,scope);
 v["enabled"]=true;return v;
}

// Broad edits remove only the corresponding lower-level property.
inline void clearInherited25(QVariantMap &v,const QString &scope,const QString &key){
 auto clear=[&](QVariantMap &o,const QString &part,bool cell){
  if(key=="background")o.remove(cell?"background":part+"Background");
  else if(key=="font"||key=="color"){QString name=cell?"style":part+"Style";auto style=o.value(name).toMap();style.remove(key);o[name]=style;}
  else if(cell)o.remove(key);
  else {QString name=part+"Alignment20";auto a=o.value(name).toMap();a.remove(key);o[name]=a;}
 };
 bool body=scope=="body"||scope=="all",header=scope=="header"||scope=="all";
 auto columns=v.value("columnOverrides17").toList();
 for(auto &entry:columns){auto o=entry.toMap();if(body)clear(o,"body",false);if(header)clear(o,"header",false);entry=o;}v["columnOverrides17"]=columns;
 if(body){auto rows=v.value("rowOverrides17").toMap();for(auto it=rows.begin();it!=rows.end();++it){auto o=it.value().toMap();clear(o,"body",false);it.value()=o;}v["rowOverrides17"]=rows;}
 auto cells=v.value("cellOverrides20").toMap();for(auto it=cells.begin();it!=cells.end();++it){int row=it.key().section(':',0,0).toInt();if((row>=0&&body)||(row==-1&&header)||(row==-2&&(scope=="all"||scope=="title"))){auto o=it.value().toMap();clear(o,{},true);it.value()=o;}}v["cellOverrides20"]=cells;
}

class Panel : public QGroupBox {
 QPointer<QetGraphicsTableItem> table;QComboBox *scope,*column,*hAlign,*vAlign,*columnsMode,*rowsMode;QSpinBox *row;QFontComboBox *family;QDoubleSpinBox *size;QCheckBox *bold,*italic,*wrap,*remember;QLabel *status;bool syncing=false;
 QString kind()const{auto *m=table?qobject_cast<ProjectDBModel*>(table->model()):nullptr;return m?m->identifier():QString();}
 QVariantMap base()const{if(!table)return {};return QetTableDefaults18::snapshotFormat18(table,table->model(),table->customFormat());}
 void commit(const QVariantMap &after){if(!table||!table->diagram()||table->diagram()->isReadOnly())return;auto before=table->customFormat();if(after==before)return;auto *command=new QUndoCommand(tr("Quick format table"));new QPropertyUndoCommand(table,"customFormat",before,after,command);table->diagram()->undoStack().push(command);QetTableDefaults18::rememberFormat20(kind(),after);}
 void edit(const QString &key,const QVariant &value){if(syncing||!table)return;const auto target=scope->currentData().toString();if(target=="cells"&&table->selectedCells20().isEmpty()){status->setText(tr("Select cells on the sheet first."));return;}commit(change(base(),target,column->currentIndex(),row->value()-1,table->selectedCells20(),key,value));}
 void editFont(){QFont f=family->currentFont();f.setPointSizeF(size->value());f.setBold(bold->isChecked());f.setItalic(italic->isChecked());edit("font",f.toString());}
 void chooseColour(const QString &key){auto c=QColorDialog::getColor(Qt::black,this,key=="color"?tr("Text colour / opacity"):tr("Background colour / opacity"),QColorDialog::ShowAlphaChannel);if(c.isValid())edit(key,c.name(QColor::HexArgb));}
 public:
 explicit Panel(QetGraphicsTableItem *t,QWidget *parent=nullptr):QGroupBox(tr("Quick table formatting"),parent),table(t){
 setObjectName("tableQuick20");auto *form=new QFormLayout(this);scope=new QComboBox;scope->setObjectName("quickScope20");for(const auto &p:QList<QPair<QString,QString>>{{tr("Body"),"body"},{tr("Headers"),"header"},{tr("Title"),"title"},{tr("Whole table"),"all"},{tr("Column"),"column"},{tr("Body row"),"row"},{tr("Selected cells"),"cells"}})scope->addItem(p.first,p.second);form->addRow(tr("Apply to"),scope);
 column=new QComboBox;column->setObjectName("quickColumn20");for(int c=0;t&&t->model()&&c<t->model()->columnCount();++c)column->addItem(QString::number(c+1)+" — "+t->model()->headerData(c,Qt::Horizontal).toString());form->addRow(tr("Column"),column);row=new QSpinBox;row->setRange(1,10000);row->setObjectName("quickRow20");form->addRow(tr("Body row"),row);
 family=new QFontComboBox;family->setObjectName("quickFont20");form->addRow(tr("Font"),family);auto *fontRow=new QHBoxLayout;size=new QDoubleSpinBox;size->setRange(1,200);size->setObjectName("quickSize20");bold=new QCheckBox(tr("Bold"));italic=new QCheckBox(tr("Italic"));fontRow->addWidget(size);fontRow->addWidget(bold);fontRow->addWidget(italic);form->addRow(tr("Size"),fontRow);
 hAlign=new QComboBox;hAlign->setObjectName("quickHorizontal20");hAlign->addItem(tr("Left"),int(Qt::AlignLeft));hAlign->addItem(tr("Centre"),int(Qt::AlignHCenter));hAlign->addItem(tr("Right"),int(Qt::AlignRight));form->addRow(tr("Horizontal"),hAlign);
 vAlign=new QComboBox;vAlign->addItem(tr("Top"),int(Qt::AlignTop));vAlign->addItem(tr("Middle"),int(Qt::AlignVCenter));vAlign->addItem(tr("Bottom"),int(Qt::AlignBottom));form->addRow(tr("Vertical"),vAlign);wrap=new QCheckBox(tr("Wrap text"));form->addRow(wrap);
 auto *colours=new QHBoxLayout;auto *text=new QPushButton(tr("Text colour…"));auto *background=new QPushButton(tr("Background…"));colours->addWidget(text);colours->addWidget(background);form->addRow(colours);
 columnsMode=new QComboBox;columnsMode->setObjectName("quickColumnResize20");columnsMode->addItem(tr("Keep table width"),"keep");columnsMode->addItem(tr("Expand table"),"expand");form->addRow(tr("Column drag"),columnsMode);rowsMode=new QComboBox;rowsMode->addItem(tr("Expand table"),"expand");rowsMode->addItem(tr("Keep table height"),"keep");form->addRow(tr("Row drag"),rowsMode);

 auto *titleHeight=new QSpinBox;titleHeight->setObjectName("titleHeight25");titleHeight->setRange(6,10000);titleHeight->setValue(base().value("titleHeight",24).toInt());form->addRow(tr("Title height"),titleHeight);
 connect(titleHeight,&QSpinBox::editingFinished,this,[this,titleHeight](){auto v=base();v["titleHeight"]=titleHeight->value();commit(v);});
 auto *tableWidth=new QSpinBox;tableWidth->setObjectName("tableWidth25");tableWidth->setRange(10,100000);tableWidth->setValue(QetFormatting::tableWidth(base(),table->model()->columnCount()));form->addRow(tr("Table / title width"),tableWidth);
 connect(tableWidth,&QSpinBox::editingFinished,this,[this,tableWidth](){auto v=base();auto widths=QetFormatting::tableWidths(v,table->model()->columnCount());int old=0;for(int w:widths)old+=w;QVariantList next,automatic;for(int w:widths){next<<qBound(10,qRound(qreal(w)*tableWidth->value()/qMax(1,old)),10000);automatic<<false;}v["widths"]=next;v["autoWidths"]=automatic;commit(v);});
 auto *defaults=new QVBoxLayout;auto *fonts=new QPushButton(tr("Use fonts as default"));auto *full=new QPushButton(tr("Use configuration as default"));defaults->addWidget(fonts);defaults->addWidget(full);form->addRow(defaults);remember=new QCheckBox(tr("Remember last used formatting"));form->addRow(remember);fonts->setEnabled(QetTableDefaults18::supportedKind18(kind()));full->setEnabled(QetTableDefaults18::supportedKind18(kind()));remember->setEnabled(QetTableDefaults18::supportedKind18(kind()));status=new QLabel(tr("Select table, then click a cell; Ctrl-click adds cells, Shift-click selects a rectangle."));status->setWordWrap(true);form->addRow(status);
 auto *editText=new QPushButton(tr("Edit selected cell text…"));editText->setObjectName("editSelectedCell25");form->addRow(editText);
 connect(editText,&QPushButton::clicked,this,[this](){if(table->selectedCells20().size()!=1){status->setText(tr("Select one cell, then use Edit text or F2."));return;}auto cells=table->selectedCells20();auto cell=*cells.cbegin();table->editCellText25(cell.first,cell.second,this);});
 auto *reviewText=new QPushButton(tr("Review saved cell text…"));form->addRow(reviewText);connect(reviewText,&QPushButton::clicked,this,[this](){table->reviewCellText25(this);});
 auto *reset=new QPushButton(tr("Clear selected cell overrides"));form->addRow(reset);
 connect(reset,&QPushButton::clicked,this,[this](){auto v=base();auto overrides=v.value("cellOverrides20").toMap();for(auto cell:table->selectedCells20())overrides.remove(QString::number(cell.second)+":"+QString::number(cell.first));v["cellOverrides20"]=overrides;commit(v);});
 connect(scope,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){refresh();});connect(column,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){refresh();});connect(row,QOverload<int>::of(&QSpinBox::valueChanged),this,[this](){refresh();});
 connect(family,&QFontComboBox::currentFontChanged,this,[this](){editFont();});connect(size,&QDoubleSpinBox::editingFinished,this,[this](){editFont();});connect(bold,&QCheckBox::toggled,this,[this](){editFont();});connect(italic,&QCheckBox::toggled,this,[this](){editFont();});
 connect(hAlign,QOverload<int>::of(&QComboBox::activated),this,[this](){edit("hAlign",hAlign->currentData());});connect(vAlign,QOverload<int>::of(&QComboBox::activated),this,[this](){edit("vAlign",vAlign->currentData());});connect(wrap,&QCheckBox::toggled,this,[this](bool value){edit("wrap",value);});
 connect(text,&QPushButton::clicked,this,[this](){chooseColour("color");});connect(background,&QPushButton::clicked,this,[this](){chooseColour("background");});
 connect(columnsMode,QOverload<int>::of(&QComboBox::activated),this,[this](){auto v=base();v["columnResize20"]=columnsMode->currentData();commit(v);});connect(rowsMode,QOverload<int>::of(&QComboBox::activated),this,[this](){auto v=base();v["rowResize20"]=rowsMode->currentData();commit(v);});
 connect(fonts,&QPushButton::clicked,this,[this](){QetTableDefaults18::saveFonts20(kind(),base());status->setText(tr("Fonts saved for new tables of this type."));});
 connect(full,&QPushButton::clicked,this,[this](){auto *m=qobject_cast<ProjectDBModel*>(table->model());if(!m)return;auto value=QetTableDefaults18::softwareConfiguration18(kind());value["query"]=m->queryString();value["format"]=base();value["columns18"]=QetTableDefaults18::columnKeys18(m);QString error;status->setText(QetTableDefaults18::saveDefault18(kind(),value,&error)?tr("Configuration saved for new tables of this type."):error);});
 connect(remember,&QCheckBox::toggled,this,[this](bool checked){if(syncing)return;QSettings().setValue(QetTableDefaults18::preferenceKey20(kind(),"remember"),checked);if(checked)QetTableDefaults18::rememberFormat20(kind(),base());});
 if(t){if(t->model())connect(t->model(),&QAbstractItemModel::modelReset,this,[this](){refresh();});connect(t,&QetGraphicsTableItem::customFormatChanged18,this,[this](){refresh();});connect(t,&QetGraphicsTableItem::cellSelectionChanged20,this,[this](){syncing=true;scope->setCurrentIndex(scope->findData("cells"));syncing=false;refresh();});}refresh();
 }
 void refresh(){if(syncing||!table||!table->model())return;QScopedValueRollback<bool> guard(syncing,true);setEnabled(!table->diagram()||!table->diagram()->isReadOnly());auto v=base();int selectedColumn=column->currentIndex();column->clear();for(int i=0;i<table->model()->columnCount();++i)column->addItem(QString::number(i+1)+" — "+QetFormatting::columnHeading(v,table->model(),i));column->setCurrentIndex(qBound(0,selectedColumn,qMax(0,column->count()-1)));QString target=scope->currentData().toString();column->setVisible(target=="column");row->setVisible(target=="row");auto *form=qobject_cast<QFormLayout*>(layout());if(auto *label=form->labelForField(column))label->setVisible(target=="column");if(auto *label=form->labelForField(row))label->setVisible(target=="row");int c=column->currentIndex(),r=row->value()-1;auto style=v.value(target=="header"?"headerStyle":target=="title"?"titleStyle":"bodyStyle").toMap();int align=Qt::AlignLeft|Qt::AlignVCenter;bool wrapping=false;
 if(target=="column"||target=="row"||target=="cells"){if(target=="cells"&&!table->selectedCells20().isEmpty()){auto cells=table->selectedCells20().values();std::sort(cells.begin(),cells.end());c=cells.first().first;r=cells.first().second;}style=r==-2?v.value("titleStyle").toMap():r==-1?QetFormatting::headerCellStyle17(v,c):QetFormatting::bodyCellStyle17(v,c,r);align=QetFormatting::cellAlignment20(v,c,r,Qt::AlignLeft);wrapping=QetFormatting::cellWrap20(v,c,r);}
 else {r=target=="header"?-1:target=="title"?-2:0;align=QetFormatting::cellAlignment20(v,-1,r,target=="header"||target=="title"?Qt::AlignHCenter:Qt::AlignLeft);wrapping=QetFormatting::cellWrap20(v,0,r);}
 auto f=QetFormatting::font(style,QETApp::diagramTextsFont());family->setCurrentFont(f);size->setValue(f.pointSizeF()>0?f.pointSizeF():9);bold->setChecked(f.bold());italic->setChecked(f.italic());hAlign->setCurrentIndex(qMax(0,hAlign->findData(align&Qt::AlignHorizontal_Mask)));vAlign->setCurrentIndex(qMax(0,vAlign->findData(align&Qt::AlignVertical_Mask)));wrap->setChecked(wrapping);columnsMode->setCurrentIndex(qMax(0,columnsMode->findData(v.value("columnResize20","keep"))));rowsMode->setCurrentIndex(qMax(0,rowsMode->findData(v.value("rowResize20","expand"))));remember->setChecked(QetTableDefaults18::remember20(kind()));
 if(target=="cells")status->setText(tr("%1 selected cells. Mixed selections show the first cell's values. Row/cell styles follow row positions when sorted.").arg(table->selectedCells20().size()));
 if(auto *height=findChild<QSpinBox*>("titleHeight25"))if(!height->hasFocus())height->setValue(v.value("titleHeight",24).toInt());
 if(auto *width=findChild<QSpinBox*>("tableWidth25"))if(!width->hasFocus())width->setValue(QetFormatting::tableWidth(v,table->model()->columnCount()));
 auto keys=QetManualCells25::currentKeys(table->model(),table->formattedRows());int unmatched=0;auto edits=v.value("manualCells25").toMap();for(auto it=edits.cbegin();it!=edits.cend();++it)if(it.value().toMap().value("language").toString()==QetManualCells25::language(table->model())&&!keys.contains(it.key()))++unmatched;
 if(unmatched)status->setText(tr("%1 saved edits no longer match a unique source row. Use Review saved cell text.").arg(unmatched));
 }
};
}
#endif
