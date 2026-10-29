// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QtWidgets>
#include "../qetproject.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/crossrefitem.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
namespace QetXref26 {
inline QVariantMap options(const Element *e){
 if(!e||!e->diagram())return {};auto data=e->diagram()->project()->languageData();
 auto v=data.value("xrefTypes26").toMap().value(e->kindInformations().value("type").toString()).toMap();
 auto own=data.value("xrefDevices26").toMap().value(e->uuid().toString()).toMap();for(auto it=own.cbegin();it!=own.cend();++it)v[it.key()]=it.value();return v;
}
inline void save(Element *e,const QVariantMap &v,bool all=false,bool reset=false){
 auto *p=e->diagram()->project();if(p->isReadOnly())return;auto before=p->languageData(),after=before;
 QString field=all?"xrefTypes26":"xrefDevices26",key=all?e->kindInformations().value("type").toString():e->uuid().toString();auto map=after.value(field).toMap();if(reset)map.remove(key);else map[key]=v;after[field]=map;
 if(before==after)return;auto *cmd=new QUndoCommand(QObject::tr("Change relay contact display"));new QPropertyUndoCommand(p,"languageData",before,after,cmd);p->undoStack()->push(cmd);
}
inline void show(Diagram *d,QWidget *parent){
 if(!d||d->isReadOnly())return;QSet<Element*> selected;for(auto *i:d->selectedItems()){for(auto *p=i;p;p=p->parentItem())if(auto *e=dynamic_cast<Element*>(p)){if(e->linkType()==Element::Master)selected.insert(e);break;}}
 if(selected.size()!=1){QMessageBox::information(parent,QObject::tr("Relay contact display"),QObject::tr("Select one master relay or its label first."));return;}auto *e=*selected.begin();
 if(e->elementData().m_master_type==ElementData::PLC){QMessageBox::information(parent,QObject::tr("Relay contact display"),QObject::tr("PLC IO tables use their symbol's table layout."));return;}
 auto v=options(e);auto xp=CrossRefItem::propertiesFor26(e);QDialog dialog(parent);dialog.setMinimumWidth(460);dialog.setObjectName("relayDisplay26");dialog.setWindowTitle(QObject::tr("Relay contact display"));auto *f=new QFormLayout(&dialog);
 auto *note=new QLabel(QObject::tr("The contact display keeps its electrical links. Manual position: Shift-drag the display on the sheet, or enter its top-left sheet coordinates below. Device label position is independent. Type defaults preserve existing device overrides."));note->setWordWrap(true);f->addRow(note);
 auto *scope=new QComboBox;scope->addItems({QObject::tr("This device"),QObject::tr("Default for this device type")});f->addRow(QObject::tr("Apply to"),scope);
 auto *appearance=new QComboBox;appearance->addItems({QObject::tr("Cross"),QObject::tr("Contact symbols")});appearance->setCurrentIndex(xp.displayHas()==XRefProperties::Contacts);f->addRow(QObject::tr("Appearance"),appearance);
 auto *pos=new QComboBox;pos->addItem(QObject::tr("Under device label"),"label");pos->addItem(QObject::tr("Below symbol"),"symbol");pos->addItem(QObject::tr("Sheet footer"),"footer");pos->addItem(QObject::tr("Manual position"),"manual");pos->setCurrentIndex(qMax(0,pos->findData(v.value("position",xp.snapTo()==XRefProperties::Label?"label":"footer"))));f->addRow(QObject::tr("Position"),pos);
 auto spin=[&](const QString &label,double value,double low,double high){auto *s=new QDoubleSpinBox;s->setRange(low,high);s->setDecimals(2);s->setSuffix(" du");s->setValue(value);f->addRow(label,s);return s;};
 auto *width=spin(QObject::tr("Line thickness"),v.value("width",.5).toDouble(),.1,10);auto *gap=spin(QObject::tr("Gap below symbol"),v.value("gap",10).toDouble(),0,500);
 auto b=e->sceneBoundingRect();auto *x=spin(QObject::tr("Manual X"),v.value("x",b.left()).toDouble(),-10000,10000);auto *y=spin(QObject::tr("Manual Y"),v.value("y",b.bottom()+10).toDouble(),-10000,10000);
 auto updateFields=[&]{gap->setEnabled(pos->currentData()=="symbol");x->setEnabled(pos->currentData()=="manual");y->setEnabled(pos->currentData()=="manual");};QObject::connect(pos,&QComboBox::currentIndexChanged,&dialog,updateFields);updateFields();
 auto *all=new QCheckBox(QObject::tr("Show defined contacts even when not linked"));all->setChecked(v.value("all",xp.showAllConfiguredSlaves()).toBool());f->addRow(all);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);auto *reset=buttons->addButton(QObject::tr("Restore inherited settings"),QDialogButtonBox::ResetRole);f->addRow(buttons);bool restore=false;
 QObject::connect(reset,&QPushButton::clicked,&dialog,[&]{restore=true;dialog.accept();});QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
 if(dialog.exec()!=QDialog::Accepted)return;
 if(scope->currentIndex()==1&&pos->currentData()=="manual"&&!restore){QMessageBox::information(parent,QObject::tr("Manual position"),QObject::tr("Manual coordinates apply to one device. Choose This device."));return;}
 v["appearance"]=appearance->currentIndex();v["position"]=pos->currentData();v["width"]=width->value();v["gap"]=gap->value();v["x"]=x->value();v["y"]=y->value();v["all"]=all->isChecked();save(e,v,scope->currentIndex()==1,restore);
}
}
