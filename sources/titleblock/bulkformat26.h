// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "templatecommands.h"
#include "templateview.h"
#include "../titleblocktemplate.h"
#include "../ui/textstyle13.h"
namespace QetTitleBulk26 {
inline QVariantMap mergeStyle(QVariantMap style,const QFont &fallback,const QFont &chosen,const QSet<QString> &fields,const QColor &chosenColor,int opacity){
 QFont f=QetFormatting::font(style,fallback);
 if(fields.contains("family"))f.setFamily(chosen.family());if(fields.contains("size"))f.setPointSizeF(chosen.pointSizeF());
 if(fields.contains("bold"))f.setBold(chosen.bold());if(fields.contains("italic"))f.setItalic(chosen.italic());
 if(fields.contains("family")||fields.contains("size")||fields.contains("bold")||fields.contains("italic"))style["font"]=f.toString();
 QColor c=QetFormatting::color(style);if(fields.contains("color")){auto alpha=c.alpha();c=chosenColor;c.setAlpha(alpha);}if(fields.contains("opacity"))c.setAlphaF(opacity/100.0);
 if(fields.contains("color")||fields.contains("opacity"))style["color"]=c.name(QColor::HexArgb);return style;
}
inline void show(TitleBlockTemplate *t,TitleBlockTemplateView *view,QUndoStack *stack,QWidget *parent){
 if(!t)return;QDialog d(parent);d.setMinimumWidth(460);d.setObjectName("bulkTitleFormat26");d.setWindowTitle(QObject::tr("Apply title-block text formatting"));auto *l=new QFormLayout(&d);
 auto *note=new QLabel(QObject::tr("Ctrl-click cells to add/remove them, or drag a selection rectangle. Only checked properties change. Captions and values keep their own unselected properties. Alignment applies to the whole cell."));note->setWordWrap(true);l->addRow(note);
 auto *scope=new QComboBox;scope->addItems({QObject::tr("Selected cells"),QObject::tr("All text cells")});if(view->selectedCells().isEmpty())scope->setCurrentIndex(1);l->addRow(QObject::tr("Apply to"),scope);
 auto *parts=new QComboBox;parts->addItems({QObject::tr("Captions and values"),QObject::tr("Captions only"),QObject::tr("Values only")});l->addRow(QObject::tr("Text part"),parts);
 QFont f=QApplication::font();QColor color=Qt::black;auto *font=new QPushButton(QObject::tr("Choose font..."));l->addRow(font);
 QObject::connect(font,&QPushButton::clicked,&d,[&]{bool ok=false;auto chosen=QFontDialog::getFont(&ok,f,&d);if(ok){f=chosen;font->setText(f.family()+" "+QString::number(f.pointSizeF()));}});
 const QMap<QString,QString> names{{"family",QObject::tr("Font family")},{"size",QObject::tr("Font size")},{"bold",QObject::tr("Bold")},{"italic",QObject::tr("Italic")},{"color",QObject::tr("Text colour")},{"opacity",QObject::tr("Opacity")},{"alignment",QObject::tr("Horizontal alignment")}};
 QMap<QString,QCheckBox*> fields;for(auto key:QStringList{"family","size","bold","italic","color","opacity","alignment"}){auto *c=new QCheckBox(names.value(key));c->setObjectName("bulk_"+key);fields[key]=c;l->addRow(c);}fields["family"]->setChecked(true);
 auto *colour=new QPushButton(QObject::tr("Choose colour..."));l->addRow(colour);QObject::connect(colour,&QPushButton::clicked,&d,[&]{auto c=QColorDialog::getColor(color,&d);if(c.isValid()){color=c;colour->setText(c.name());fields["color"]->setChecked(true);}});
 auto *opacity=new QSpinBox;opacity->setRange(0,100);opacity->setValue(100);opacity->setSuffix(" %");l->addRow(QObject::tr("Opacity"),opacity);
 auto *alignment=new QComboBox;alignment->addItem(QObject::tr("Left"),int(Qt::AlignLeft));alignment->addItem(QObject::tr("Centre"),int(Qt::AlignHCenter));alignment->addItem(QObject::tr("Right"),int(Qt::AlignRight));l->addRow(QObject::tr("Horizontal alignment"),alignment);
 auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l->addRow(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
 auto cells=view->selectedCells();if(scope->currentIndex()==1){cells.clear();for(int row=0;row<t->rowsCount();++row)for(int col=0;col<t->columnsCount();++col)cells<<t->cell(row,col);}
 QSet<QString> checked;for(auto it=fields.cbegin();it!=fields.cend();++it)if(it.value()->isChecked())checked.insert(it.key());if(checked.isEmpty())return;
 auto *command=new QUndoCommand(QObject::tr("Format title-block cells"));QSet<TitleBlockCell*> seen;
 for(auto *cell:cells){if(!cell||seen.contains(cell)||cell->spanner_cell||cell->cell_type!=TitleBlockCell::TextCell)continue;seen.insert(cell);auto *c=new ModifyTitleBlockCellCommand(cell,command);c->setView(view);
  if(parts->currentIndex()!=2)c->addModification("label_style",mergeStyle(cell->label_style,TitleBlockTemplate::fontForCell(*cell),f,checked,color,opacity->value()));
  if(parts->currentIndex()!=1)c->addModification("value_style",mergeStyle(cell->value_style,TitleBlockTemplate::fontForCell(*cell),f,checked,color,opacity->value()));
  if(checked.contains("alignment"))c->addModification("alignment",(cell->alignment&~int(Qt::AlignHorizontal_Mask))|alignment->currentData().toInt());
 }
 if(command->childCount())stack->push(command);else delete command;
}
}
