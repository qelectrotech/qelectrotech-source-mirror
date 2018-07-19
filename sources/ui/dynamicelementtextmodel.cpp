/*
	Copyright 2006-2017 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "dynamicelementtextmodel.h"
#include "dynamicelementtextitem.h"
#include <QStandardItem>
#include <QHash>
#include <QColorDialog>
#include <QModelIndex>
#include <QComboBox>
#include <QUndoCommand>
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "qetapp.h"
#include "element.h"
#include "compositetexteditdialog.h"
#include "terminal.h"
#include "conductor.h"
#include "elementtextitemgroup.h"
#include "qeticons.h"
#include "diagram.h"
#include "addelementtextcommand.h"
#include "alignmenttextdialog.h"

int src_txt_row   = 0;
int usr_txt_row   = 1;
int info_txt_row  = 2;
int compo_txt_row = 3;
int size_txt_row  = 4;
int color_txt_row = 5;
int frame_txt_row = 6;
int width_txt_row = 7;
int x_txt_row     = 8;
int y_txt_row     = 9;
int rot_txt_row   = 10;
int align_txt_row = 11;

int align_grp_row          = 0;
int x_grp_row              = 1;
int y_grp_row              = 2;
int rot_grp_row            = 3;
int adjust_grp_row         = 4;
int frame_grp_row          = 5;
int hold_to_bottom_grp_row = 6;

DynamicElementTextModel::DynamicElementTextModel(Element *element, QObject *parent) :
	QStandardItemModel(parent),
	m_element(element)
{
    setColumnCount(2);
    setHeaderData(0, Qt::Horizontal, tr("Propriété"), Qt::DisplayRole);
    setHeaderData(1, Qt::Horizontal, tr("Valeur"), Qt::DisplayRole);
    
	connect(this, &DynamicElementTextModel::itemChanged, this, &DynamicElementTextModel::itemDataChanged);
	
	connect(m_element.data(), &Element::textsGroupAdded,            this, &DynamicElementTextModel::addGroup,            Qt::DirectConnection);
	connect(m_element.data(), &Element::textsGroupAboutToBeRemoved, this, &DynamicElementTextModel::removeGroup,         Qt::DirectConnection);
	connect(m_element.data(), &Element::textRemoved,                this, &DynamicElementTextModel::removeText,          Qt::DirectConnection);
	connect(m_element.data(), &Element::textRemovedFromGroup,       this, &DynamicElementTextModel::removeTextFromGroup, Qt::DirectConnection);
	connect(m_element.data(), &Element::textAdded,                  this, &DynamicElementTextModel::addText,             Qt::DirectConnection);
	connect(m_element.data(), &Element::textAddedToGroup,           this, &DynamicElementTextModel::addTextToGroup,      Qt::DirectConnection);
	
	for (ElementTextItemGroup *grp : m_element.data()->textGroups())
		addGroup(grp);
	
    for (DynamicElementTextItem *deti : m_element.data()->dynamicTextItems())
		this->appendRow(itemsForText(deti));
}

DynamicElementTextModel::~DynamicElementTextModel()
{
		//Connection is not destroy automaticaly,
		//because was not connected to a slot, but a lambda
	for(DynamicElementTextItem *deti : m_hash_text_connect.keys())
		setConnection(deti, false);
	for(ElementTextItemGroup *group : m_hash_group_connect.keys())
		setConnection(group, false);
}

/**
 * @brief DynamicElementTextModel::indexIsInGroup
 * @param index
 * @return True if the index represent a group or an item in a group
 */
bool DynamicElementTextModel::indexIsInGroup(const QModelIndex &index) const
{
	QStandardItem *item = itemFromIndex(index);
	if(item)
	{
		while (item->parent())
			item = item->parent();
		
		if(m_groups_list.values().contains(item))
			return true;
		else
			return false;
	}
	return false;
}

/**
 * @brief DynamicElementTextModel::itemsForText
 * @param deti
 * @return The items for the text @deti, if the text@deti is already managed by this model
 * the returned list is empty
 * The returned items haven't got the same number of childs if the text is in a group or not.
 */
QList<QStandardItem *> DynamicElementTextModel::itemsForText(DynamicElementTextItem *deti)
{
	QList <QStandardItem *> qsi_list;
	
	if(m_texts_list.keys().contains(deti))
        return qsi_list;
    
	QStandardItem *qsi = new QStandardItem(deti->toPlainText());
    qsi->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |Qt::ItemIsDragEnabled);
	qsi->setIcon(QET::Icons::PartText);
	
	
        //Source of text
    QStandardItem *src = new QStandardItem(tr("Source du texte"));
    src->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
	QString title;
	if (deti->textFrom() == DynamicElementTextItem::UserText) title = tr("Texte utilisateur");
	else if (deti->textFrom() == DynamicElementTextItem::ElementInfo) title = tr("Information de l'élément");
	else title =  tr("Texte composé");
    auto *srca = new QStandardItem(title);
    srca->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    srca->setData(textFrom, Qt::UserRole+1);
    
    qsi_list << src << srca;
    qsi->appendRow(qsi_list);
	
		//User text
	QStandardItem *usr = new QStandardItem(tr("Texte"));
    usr->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    QStandardItem *usra = new QStandardItem(deti->toPlainText());
    usra->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    usra->setData(DynamicElementTextModel::userText, Qt::UserRole+1);
   
	qsi_list.clear();
    qsi_list << usr << usra;
    qsi->appendRow(qsi_list);
	
		//Info text
	QStandardItem *info = new QStandardItem(tr("Information"));
    info->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    QStandardItem *infoa = new QStandardItem(QETApp::elementTranslatedInfoKey(deti->infoName()));
    infoa->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    infoa->setData(DynamicElementTextModel::infoText, Qt::UserRole+1); //Use to know the edited thing
	infoa->setData(deti->infoName(), Qt::UserRole+2); //Use to know to element info name
    
	qsi_list.clear();
    qsi_list << info << infoa;
    qsi->appendRow(qsi_list);
	
		//Composite text
	QStandardItem *composite = new QStandardItem(tr("Texte composé"));
	composite->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	DiagramContext dc;
	if(deti->elementUseForInfo())
		dc = deti->elementUseForInfo()->elementInformations();
	QStandardItem *compositea = new QStandardItem(deti->compositeText().isEmpty() ?
													  tr("Mon texte composé") :
													  autonum::AssignVariables::replaceVariable(deti->compositeText(), dc));
	compositea->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	compositea->setData(DynamicElementTextModel::compositeText, Qt::UserRole+1); //Use to know the edited thing
	compositea->setData(deti->compositeText(), Qt::UserRole+2); //Use to know to element composite formula
	
	qsi_list.clear();
	qsi_list << composite << compositea;
	qsi->appendRow(qsi_list);
    
        //Size
	QStandardItem *size = new QStandardItem(tr("Taille"));
    size->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    auto *siza = new QStandardItem();
    siza->setData(deti->fontSize(), Qt::EditRole);
    siza->setData(DynamicElementTextModel::size, Qt::UserRole+1);
    siza->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    
    qsi_list.clear();
    qsi_list << size << siza;
	qsi->appendRow(qsi_list);

        //Color
    QStandardItem *color = new QStandardItem(tr("Couleur"));
    color->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    auto *colora = new QStandardItem;
    colora->setData(deti->color(), Qt::ForegroundRole);
    colora->setData(deti->color(), Qt::EditRole);
    colora->setData(DynamicElementTextModel::color, Qt::UserRole+1);
    colora->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    
    qsi_list.clear();
    qsi_list << color << colora;
	qsi->appendRow(qsi_list);
	
		//Frame
	QStandardItem *frame = new QStandardItem(tr("Cadre"));
	frame->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	auto *frame_a = new QStandardItem;
	frame_a->setCheckable(true);
	frame_a->setCheckState(deti->frame()? Qt::Checked : Qt::Unchecked);
	frame_a->setData(DynamicElementTextModel::frame, Qt::UserRole+1);
	frame_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
	
	qsi_list.clear();
	qsi_list << frame << frame_a;
	qsi->appendRow(qsi_list);
	
		//Width
	QStandardItem *width = new QStandardItem(tr("Largeur"));
	width->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	auto *width_a = new QStandardItem;
	width_a->setData(deti->textWidth(), Qt::EditRole);
	width_a->setData(DynamicElementTextModel::textWidth, Qt::UserRole+1);
	width_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	
	qsi_list.clear();
	qsi_list << width << width_a;
	qsi->appendRow(qsi_list);
	
	if(deti->parentGroup() == nullptr)
	{
			//X pos
		QStandardItem *x_pos = new QStandardItem(tr("Position X"));
		x_pos->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		
		auto *x_pos_a = new QStandardItem;
		x_pos_a->setData(deti->pos().x(), Qt::EditRole);
		x_pos_a->setData(DynamicElementTextModel::pos, Qt::UserRole+1);
		x_pos_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		
		qsi_list.clear();
		qsi_list << x_pos << x_pos_a;
		qsi->appendRow(qsi_list);
		
			//Y pos
		QStandardItem *y_pos = new QStandardItem(tr("Position Y"));
		y_pos->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		
		auto *y_pos_a = new QStandardItem;
		y_pos_a->setData(deti->pos().y(), Qt::EditRole);
		y_pos_a->setData(DynamicElementTextModel::pos, Qt::UserRole+1);
		y_pos_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		
		qsi_list.clear();
		qsi_list << y_pos << y_pos_a;
		qsi->appendRow(qsi_list);
		
			//Rotation
		QStandardItem *rot = new QStandardItem(tr("Rotation"));
		rot->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		
		auto *rot_a = new QStandardItem;
		rot_a->setData(deti->rotation(), Qt::EditRole);
		rot_a->setData(DynamicElementTextModel::rotation, Qt::UserRole+1);
		rot_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		
		qsi_list.clear();;
		qsi_list << rot << rot_a;
		qsi->appendRow(qsi_list);
		
			//Alignment
		QStandardItem *alignment = new QStandardItem(tr("Alignement"));
		alignment->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		
		QStandardItem *alignmenta = new QStandardItem(tr("Éditer"));
		alignmenta->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		alignmenta->setData(DynamicElementTextModel::txtAlignment, Qt::UserRole+1);
		alignmenta->setData(QVariant::fromValue(deti->alignment()), Qt::UserRole+2);
		
		qsi_list.clear();
		qsi_list << alignment << alignmenta;
		qsi->appendRow(qsi_list);
	}
	
	
	qsi_list.clear();
	auto *empty_qsi = new QStandardItem(0);
	empty_qsi->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	qsi_list << qsi << empty_qsi;
	
    m_texts_list.insert(deti, qsi);
	blockSignals(true);
	enableSourceText(deti, deti->textFrom());
	blockSignals(false);
	setConnection(deti, true);
	
	return qsi_list;
}

/**
 * @brief DynamicElementTextModel::addText
 * @param deti
 */
void DynamicElementTextModel::addText(DynamicElementTextItem *deti)
{
	this->appendRow(itemsForText(deti));
}

/**
 * @brief DynamicElementTextModel::removeText
 * @param deti
 */
void DynamicElementTextModel::removeText(DynamicElementTextItem *deti)
{
	if (!m_texts_list.contains(deti))
        return;
    
    QModelIndex text_index = m_texts_list.value(deti)->index();
    this->removeRow(text_index.row(), text_index.parent());
    m_texts_list.remove(deti);
	setConnection(deti, false);
}

/**
 * @brief DynamicElementTextModel::textFromIndex
 * @param index
 * @return the text associated with @index. Returned value can be nullptr
 * @Index can be a child of an index associated with a text and can be the column 0 or 1. 
 */
DynamicElementTextItem *DynamicElementTextModel::textFromIndex(const QModelIndex &index) const
{
    if(!index.isValid())
        return nullptr;
    
    if (QStandardItem *item = itemFromIndex(index))
        return textFromItem(item);
    else
        return nullptr;
}

/**
 * @brief DynamicElementTextModel::textFromItem
 * @param item
 * @return the text associated with @item. Return value can be nullptr
 * @item can be a child of an item associated with a text and can be the column 0 or 1. 
 * Note can return nullptr
 */
DynamicElementTextItem *DynamicElementTextModel::textFromItem(QStandardItem *item) const
{
		//Get the item of the column 0
	if(item->column() == 1)
	{
		if(item->parent())
			item = item->parent()->child(item->row(), 0);
		else
			item = itemFromIndex(index(item->row(),0));
	}
		//Item haven't got parent, so they can be only a text or a group
	if(!item->parent()) 
	{
		if(m_texts_list.values().contains(item))
			return m_texts_list.key(item);
		else
			return nullptr;
	}
	
	
	
	QStandardItem *text_item = item;
	while (text_item->parent())
		text_item = text_item->parent();
	
	if (m_texts_list.values().contains(text_item)) //The item is a text
		return m_texts_list.key(text_item);
	else if (m_groups_list.values().contains(text_item)) //The item is a group
	{
		QStandardItem *previous = item;
		QStandardItem *top = item;
			//At the end of the while, previous must be the text
			//and top the group
		while(top->parent())
		{
			previous = top;
			top = top->parent();
		}
		
		if(m_texts_list.values().contains(previous))
			return m_texts_list.key(previous);
		else
			return nullptr;
	}
	else
		return nullptr;
}

/**
 * @brief DynamicElementTextModel::indexFromText
 * @param text
 * @return the QModelIndex for @text, or a default QModelIndex if not match
 */
QModelIndex DynamicElementTextModel::indexFromText(DynamicElementTextItem *text) const
{
	if(m_texts_list.contains(text))
		return m_texts_list.value(text)->index();
	else
		return QModelIndex();
}

/**
 * @brief DynamicElementTextModel::undoForEditedText
 * @param deti
 * @return A QUndoCommand that describe all changes made for @deti.
 * Each change made for @deti is append as a child of the returned QUndoCommand.
 * In other word, if the returned QUndoCommand have no child, that mean there is no change.
 */
QUndoCommand *DynamicElementTextModel::undoForEditedText(DynamicElementTextItem *deti, QUndoCommand *parent_undo) const
{
	
	QUndoCommand *undo = nullptr;
	if(parent_undo)
		undo = parent_undo;
	else
		undo = new QUndoCommand(tr("Éditer un texte d'élément"));
	
	if (!m_texts_list.contains(deti))
		return undo;
	
	QStandardItem *text_qsi = m_texts_list.value(deti);
	
	QString from = text_qsi->child(src_txt_row,1)->data(Qt::DisplayRole).toString();
	if ((from == tr("Texte utilisateur")) && (deti->textFrom() != DynamicElementTextItem::UserText))
		new QPropertyUndoCommand(deti, "textFrom", QVariant(deti->textFrom()), QVariant(DynamicElementTextItem::UserText), undo);
	else if ((from == tr("Information de l'élément")) && (deti->textFrom() != DynamicElementTextItem::ElementInfo))
		new QPropertyUndoCommand(deti, "textFrom", QVariant(deti->textFrom()), QVariant(DynamicElementTextItem::ElementInfo), undo);
	else if ((from == tr("Texte composé")) && (deti->textFrom() != DynamicElementTextItem::CompositeText))
		new QPropertyUndoCommand(deti, "textFrom", QVariant(deti->textFrom()), QVariant(DynamicElementTextItem::CompositeText), undo);
	
	if(from == tr("Texte utilisateur"))
	{
		QString text = text_qsi->child(usr_txt_row,1)->data(Qt::DisplayRole).toString();
		if (text != deti->text())
			new QPropertyUndoCommand(deti, "text", QVariant(deti->text()), QVariant(text), undo);
	}
	else if (from == tr("Information de l'élément"))
	{
		QString info_name = text_qsi->child(info_txt_row,1)->data(Qt::UserRole+2).toString();
		if(info_name != deti->infoName())
			new QPropertyUndoCommand(deti, "infoName", QVariant(deti->infoName()), QVariant(info_name), undo);
	}
	else if (from == tr("Texte composé"))
	{
		QString composite_text = text_qsi->child(compo_txt_row,1)->data(Qt::UserRole+2).toString();
		if(composite_text != deti->compositeText())
			new QPropertyUndoCommand(deti, "compositeText", QVariant(deti->compositeText()), QVariant(composite_text), undo);
	}
	
	int fs = text_qsi->child(size_txt_row,1)->data(Qt::EditRole).toInt();
	if (fs != deti->fontSize())
	{
		QPropertyUndoCommand *quc = new QPropertyUndoCommand(deti, "fontSize", QVariant(deti->fontSize()), QVariant(fs), undo);
		quc->setAnimated(true, false);
		quc->setText(tr("Modifier la taille d'un texte d'élément"));
	}
	
	QColor color = text_qsi->child(color_txt_row,1)->data(Qt::EditRole).value<QColor>();
	if(color != deti->color())
	{
		QUndoCommand *quc = new QPropertyUndoCommand(deti, "color", QVariant(deti->color()), QVariant(color), undo);
		quc->setText(tr("Modifier la couleur d'un texte d'élément"));
	}
	
	bool frame = text_qsi->child(frame_txt_row,1)->checkState() == Qt::Checked? true : false;
	if(frame != deti->frame())
	{
		QUndoCommand *quc = new QPropertyUndoCommand(deti, "frame", QVariant(deti->frame()), QVariant(frame), undo);
		quc->setText(tr("Modifier le cadre d'un texte d'élément"));
	}
	
	qreal text_width = text_qsi->child(width_txt_row, 1)->data(Qt::EditRole).toDouble();
	if(text_width != deti->textWidth())
	{
		QPropertyUndoCommand *quc = new QPropertyUndoCommand(deti, "textWidth", QVariant(deti->textWidth()), QVariant(text_width), undo);
		quc->setAnimated(true, false);
		quc->setText(tr("Modifier la largeur d'un texte d'élément"));
	}
	
		//When text is in a group, they're isn't item for position of the text
	if(text_qsi->child(x_txt_row,1) && text_qsi->child(y_txt_row,1))
	{
		QPointF p(text_qsi->child(x_txt_row,1)->data(Qt::EditRole).toDouble(),
				  text_qsi->child(y_txt_row,1)->data(Qt::EditRole).toDouble());
		if(p != deti->pos())
		{
			QPropertyUndoCommand *quc = new QPropertyUndoCommand(deti, "pos", QVariant(deti->pos()), QVariant(p), undo);
			quc->setAnimated(true, false);
			quc->setText(tr("Déplacer un texte d'élément"));
		}
	}
		//When text is in a group, they're isn't item for the rotation of the text
	if(text_qsi->child(rot_txt_row,1))
	{
		qreal rot = text_qsi->child(rot_txt_row,1)->data(Qt::EditRole).toDouble();
		rot = QET::correctAngle(rot);
		if(rot != deti->rotation())
		{
			QPropertyUndoCommand *quc = new QPropertyUndoCommand(deti, "rotation", QVariant(deti->rotation()), QVariant(rot), undo);
			quc->setAnimated(true, false);
			quc->setText(tr("Pivoter un texte d'élément"));
		}
	}
		
		//When text is in a groupe, they're isn't item for alignment of the text
	if(text_qsi->child(align_txt_row, 1))
	{
		Qt::Alignment alignment = text_qsi->child(align_txt_row, 1)->data(Qt::UserRole+2).value<Qt::Alignment>();
		if (alignment != deti->alignment())
		{
			QPropertyUndoCommand *quc = new QPropertyUndoCommand(deti, "alignment", QVariant(deti->alignment()), QVariant(alignment), undo);
			quc->setText(tr("Modifier l'alignement d'un texte d'élément"));
		}
	}
	
	return undo;
}

/**
 * @brief DynamicElementTextModel::undoForEditedGroup
 * @param group
 * @param parent_undo
 * @return A QUndoCommand that describe all changes made for @group.
 * Each change made for @group is append as a child of the returned QUndoCommand.
 * In other word, if the returned QUndoCommand have no child, that mean there is no change.
 */
QUndoCommand *DynamicElementTextModel::undoForEditedGroup(ElementTextItemGroup *group, QUndoCommand *parent_undo) const
{
	QUndoCommand *undo = nullptr;
	if(parent_undo)
		undo = parent_undo;
	else
		undo = new QUndoCommand(tr("Éditer un groupe de textes"));
	
	if (!m_groups_list.contains(group))
		return undo;
	
	QStandardItem *group_qsi = m_groups_list.value(group);
	
	QString alignment = group_qsi->child(align_grp_row,1)->data(Qt::DisplayRole).toString();
	if((alignment == tr("Gauche")) && (group->alignment() != Qt::AlignLeft))
		new QPropertyUndoCommand(group, "alignment", QVariant(group->alignment()), QVariant(Qt::AlignLeft), undo);
	else if((alignment == tr("Droite")) && (group->alignment() != Qt::AlignRight))
		new QPropertyUndoCommand(group, "alignment", QVariant(group->alignment()), QVariant(Qt::AlignRight), undo);
	else if((alignment == tr("Centre")) && (group->alignment() != Qt::AlignVCenter))
		new QPropertyUndoCommand(group, "alignment", QVariant(group->alignment()), QVariant(Qt::AlignVCenter), undo);
	
	
	if (group_qsi->child(hold_to_bottom_grp_row, 1)->checkState() == Qt::Unchecked)
	{
		QPointF pos(group_qsi->child(x_grp_row,1)->data(Qt::EditRole).toDouble(),
					group_qsi->child(y_grp_row,1)->data(Qt::EditRole).toDouble());
		if(group->pos() != pos)
		{
			QPropertyUndoCommand *qpuc = new QPropertyUndoCommand(group, "pos", QVariant(group->pos()), QVariant(pos), undo);
			qpuc->setAnimated(true, false);
		}
		
		qreal rotation = group_qsi->child(rot_grp_row,1)->data(Qt::EditRole).toDouble();
		if(group->rotation() != rotation)
		{
			QPropertyUndoCommand *qpuc = new QPropertyUndoCommand(group, "rotation", QVariant(group->rotation()), QVariant(rotation), undo);
			qpuc->setAnimated(true, false);
		}
	}
	
	int v_adjustment = group_qsi->child(adjust_grp_row,1)->data(Qt::EditRole).toInt();
	if(group->verticalAdjustment() != v_adjustment)
		new QPropertyUndoCommand(group, "verticalAdjustment", QVariant(group->verticalAdjustment()), QVariant(v_adjustment), undo);
	
	QString name = group_qsi->data(Qt::DisplayRole).toString();
	if(group->name() != name)
		new QPropertyUndoCommand(group, "name", QVariant(group->name()), QVariant(name), undo);
	
	bool hold_to_bottom = group_qsi->child(hold_to_bottom_grp_row, 1)->checkState() == Qt::Checked? true : false;
	if(group->holdToBottomPage() != hold_to_bottom)
		new QPropertyUndoCommand(group, "holdToBottomPage", QVariant(group->holdToBottomPage()), QVariant(hold_to_bottom), undo);
	
	bool frame_ = group_qsi->child(frame_grp_row, 1)->checkState() == Qt::Checked? true : false;
	if(group->frame() != frame_)
		new QPropertyUndoCommand(group, "frame", QVariant(group->frame()), QVariant(frame_), undo);
		
	return undo;
}

/**
 * @brief DynamicElementTextModel::AddGroup
 * Add a text item group to this model
 * @param group
 */
void DynamicElementTextModel::addGroup(ElementTextItemGroup *group)
{
	if(m_groups_list.keys().contains(group))
		return;
	
		//Group
	QStandardItem *grp = new QStandardItem(group->name());
	grp->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
	grp->setIcon(QET::Icons::textGroup);
	
	auto *empty_qsi = new QStandardItem(0);
	empty_qsi->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	QList<QStandardItem *> qsi_list;
	qsi_list << grp << empty_qsi;
	
	this->insertRow(0, qsi_list);
	m_groups_list.insert(group, grp);
	
		//Alignment
	QStandardItem *alignment = new QStandardItem(tr("Alignement"));
	alignment->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	QString text;
	switch (group->alignment()) {
		case Qt::AlignLeft:    text = tr("Gauche"); break;
		case Qt::AlignRight:   text = tr("Droite"); break;
		case Qt::AlignVCenter: text = tr("Centre"); break;
		default: break;}
	
	auto *alignment_a = new QStandardItem(text);
	alignment_a->setData(DynamicElementTextModel::grpAlignment, Qt::UserRole+1);
	alignment_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	qsi_list.clear();
	qsi_list << alignment << alignment_a;
	grp->appendRow(qsi_list);
	
		//X pos
	QStandardItem *x_pos = new QStandardItem(tr("Position X"));
	x_pos->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	auto *x_pos_a = new QStandardItem;
	x_pos_a->setData(group->pos().x(), Qt::EditRole);
	x_pos_a->setData(DynamicElementTextModel::grpPos, Qt::UserRole+1);
	x_pos_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	
	qsi_list.clear();
	qsi_list << x_pos << x_pos_a;
	grp->appendRow(qsi_list);
	
		//Y pos
	QStandardItem *y_pos = new QStandardItem(tr("Position Y"));
	y_pos->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	auto *y_pos_a = new QStandardItem;
	y_pos_a->setData(group->pos().y(), Qt::EditRole);
	y_pos_a->setData(DynamicElementTextModel::grpPos, Qt::UserRole+1);
	y_pos_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	
	qsi_list.clear();
	qsi_list << y_pos << y_pos_a;
	grp->appendRow(qsi_list);
	
		//Rotation
	QStandardItem *rot = new QStandardItem(tr("Rotation"));
	rot->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	auto *rot_a = new QStandardItem;
	rot_a->setData(group->rotation(), Qt::EditRole);
	rot_a->setData(DynamicElementTextModel::grpRotation, Qt::UserRole+1);
	rot_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	qsi_list.clear();
	qsi_list << rot << rot_a;
	grp->appendRow(qsi_list);
	
		//Vertical adjustment
	QStandardItem *v_adj = new QStandardItem(tr("Ajustement vertical"));
	v_adj->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	
	auto *v_adj_a = new QStandardItem;
	v_adj_a->setData(group->verticalAdjustment(), Qt::EditRole);
	v_adj_a->setData(DynamicElementTextModel::grpVAdjust, Qt::UserRole+1);
	v_adj_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	qsi_list.clear();
	qsi_list << v_adj << v_adj_a;
	grp->appendRow(qsi_list);
	
		//Frame
	QStandardItem *frame_ = new QStandardItem(tr("Cadre"));
	frame_->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	
	auto *frame_a = new QStandardItem;
	frame_a->setCheckable(true);
	frame_a->setCheckState(group->frame()? Qt::Checked : Qt::Unchecked);
	frame_a->setData(DynamicElementTextModel::grpFrame, Qt::UserRole+1);
	frame_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
	qsi_list.clear();
	qsi_list << frame_ << frame_a;
	grp->appendRow(qsi_list);
	
	
		//Hold to the bottom of the page
	QStandardItem *hold_bottom = new QStandardItem(tr("Maintenir en bas de page"));
	hold_bottom->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	
	auto *hold_bottom_a = new QStandardItem();
	hold_bottom_a->setCheckable(true);
	hold_bottom_a->setCheckState(group->holdToBottomPage() ? Qt::Checked : Qt::Unchecked);
	hold_bottom_a->setData(DynamicElementTextModel::grpHoldBottom, Qt::UserRole+1);
	hold_bottom_a->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
	qsi_list.clear();
	qsi_list << hold_bottom << hold_bottom_a;
	grp->appendRow(qsi_list);
	

		//Add the texts of the group
	for(DynamicElementTextItem *deti : group->texts())
	{
		QStandardItem *group_item = m_groups_list.value(group);
		group_item->appendRow(itemsForText(deti));
	}
	setConnection(group, true);
	enableGroupRotationAndPos(group);
}

/**
 * @brief DynamicElementTextModel::removeGroup
 * Remove the text item group from this model
 * @param group
 */
void DynamicElementTextModel::removeGroup(ElementTextItemGroup *group)
{
	if(m_groups_list.keys().contains(group))
	{
		QModelIndex group_index = m_groups_list.value(group)->index();
		this->removeRow(group_index.row(), group_index.parent());
		m_groups_list.remove(group);
		setConnection(group, false);
	}
}

/**
 * @brief DynamicElementTextModel::textAddedToGroup
 * Add the text @text to the group @group
 * @param deti
 * @param group
 */
void DynamicElementTextModel::addTextToGroup(DynamicElementTextItem *deti, ElementTextItemGroup *group)
{
	QStandardItem *group_item = m_groups_list.value(group);
	group_item->appendRow(itemsForText(deti));
}

void DynamicElementTextModel::removeTextFromGroup(DynamicElementTextItem *deti, ElementTextItemGroup *group)
{
	Q_UNUSED(group)
	
	if(m_texts_list.keys().contains(deti))
	{
		QStandardItem *text_item = m_texts_list.value(deti);
		QModelIndex text_index = indexFromItem(text_item);
		removeRow(text_index.row(), text_index.parent());
		m_texts_list.remove(deti);
	}
}

/**
 * @brief DynamicElementTextModel::groupFromIndex
 * @param index
 * @return the group associated with @index. Return value can be nullptr
 * @Index can be a child of an index associated with a group and can be the column 0 or 1.
 */
ElementTextItemGroup *DynamicElementTextModel::groupFromIndex(const QModelIndex &index) const
{
	if(!index.isValid())
        return nullptr;
    
    if (QStandardItem *item = itemFromIndex(index))
        return groupFromItem(item);
    else
        return nullptr;
}

/**
 * @brief DynamicElementTextModel::groupFromItem
 * @param item
 * @return the group associated with @item. Return value can be nullptr
 * @item can be a child of an item associated with a group and can be the column 0 or 1.
 */
ElementTextItemGroup *DynamicElementTextModel::groupFromItem(QStandardItem *item) const
{
		//Get the item of the column 0
	if(item->column() == 1)
	{
		if(item->parent())
			item = item->parent()->child(item->row(), 0);
		else
			item = itemFromIndex(index(item->row(),0));
	}
	
	while (item->parent())
		item = item->parent();
	
	if(m_groups_list.values().contains(item))
		return m_groups_list.key(item);
	else
		return nullptr;
}

/**
 * @brief DynamicElementTextModel::indexFromGroup
 * @param group
 * @return The index associated to the group @group
 * or a default QModelIndex if not match
 */
QModelIndex DynamicElementTextModel::indexFromGroup(ElementTextItemGroup *group) const
{
	if(m_groups_list.keys().contains(group))
		return m_groups_list.value(group)->index();
	else
		return QModelIndex();
}

/**
 * @brief DynamicElementTextModel::indexIsText
 * @param index
 * @return True if @index represente a text, both for the column 0 and 1.
 * Return false if @index is a child of an index associated to a text.
 */
bool DynamicElementTextModel::indexIsText(const QModelIndex &index) const
{
	QStandardItem *item = nullptr;
	
		//The item represent the second column
	if(index.column() == 1)
	{
		if(index.parent().isValid())
			item = itemFromIndex(index.parent().child(index.row(),0));
		else
			item = itemFromIndex(this->index(index.row(),0));
	}
	else
		item = itemFromIndex(index);

	if(item && m_texts_list.values().contains(item))
		return true;
	else
		return false;
}

/**
 * @brief DynamicElementTextModel::indexIsGroup
 * @param index
 * @return True if @index represente a group, both for the column 0 and 1.
 * Return false if @index is a child of an index associated to a group.
 */
bool DynamicElementTextModel::indexIsGroup(const QModelIndex &index) const
{
	QStandardItem *item = nullptr;
	
		//The item represent the second column
	if(index.column() == 1)
	{
		if(index.parent().isValid())
			item = itemFromIndex(index.parent().child(index.row(),0));
		else
			item = itemFromIndex(this->index(index.row(),0));
	}
	else
		item = itemFromIndex(index);

	if(item && m_groups_list.values().contains(item))
		return true;
	else
		return false;
}

bool DynamicElementTextModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
	Q_UNUSED(action);

	if(data->hasFormat("application/x-qet-element-text-uuid"))
	{
		QModelIndex index;
		if(parent.isValid() && row != -1 && column !=1) //Insert in child of parent
			index = parent.child(row, column);
		else if (parent.isValid() && row == -1 && column == -1) //Drop in parent
			index = parent;
		
		QUuid uuid(data->text());

			//The data is drop in a group
		if(indexIsInGroup(index))
		{
				//Data is dragged from a text direct child of element
			for(DynamicElementTextItem *text : m_element.data()->dynamicTextItems())
				if(text->uuid() == uuid)
					return true;
				//Data is dragged from a text in a group
			for(ElementTextItemGroup *group : m_element.data()->textGroups())
			{
				for(DynamicElementTextItem *text : group->texts())
					if(text->uuid() == uuid)
						return true;
			}
			
			return false;
						
		}
		else //The data is not drop in a group, then the action must be a drag of text from a group to the element
		{
			for(ElementTextItemGroup *group : m_element.data()->textGroups())
			{
				for(DynamicElementTextItem *text : group->texts())
					if(text->uuid() == uuid)
						return true;
			}
		}
	}
	
	return false;
}

/**
 * @brief DynamicElementTextModel::dropMimeData
 * @param data
 * @param action
 * @param row
 * @param column
 * @param parent
 * @return In any case return false, for overwrite the default behavior of model.
 */
bool DynamicElementTextModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(action)
	
	if(data->hasFormat("application/x-qet-element-text-uuid"))
	{		
		QUuid uuid(data->text());
		DynamicElementTextItem *deti = nullptr;
		ElementTextItemGroup *group = nullptr;
		
		QModelIndex index;
		if(parent.isValid() && row != -1 && column !=1) //Insert in child of parent
			index = parent.child(row, column);
		else if (parent.isValid() && row == -1 && column == -1) //Drop in parent
			index = parent;

			//Darg and drop in a group of text
		if(indexIsInGroup(index))
		{
				//The dragged text is a direct child of element
			for(DynamicElementTextItem *text : m_element.data()->dynamicTextItems())
			{
				if(text->uuid() == uuid)
				{
					deti = text;
					group = groupFromIndex(index);
					
					if(group && deti) //Text successfully added in a group	
					{
						m_element.data()->diagram()->undoStack().push(new AddTextToGroupCommand(deti, group));
						return true;
					}
				}
			}
			
				//The dragged text is in a group
			for(ElementTextItemGroup *grp : m_element.data()->textGroups())
			{
				for(DynamicElementTextItem *text : grp->texts())
				{
					if(text->uuid() == uuid)
					{
						deti = text;
						group = groupFromIndex(index);
						
							//Text successfully moved from a group to another group
						if(group && deti)
						{
							QUndoStack &stack = m_element.data()->diagram()->undoStack();
							stack.beginMacro(tr("Déplacer un texte dans un autre groupe"));
							stack.push(new RemoveTextFromGroupCommand(deti, grp));
							stack.push(new AddTextToGroupCommand(deti, group));
							stack.endMacro();
							
							return true;
						}
					}
				}
			}
				
			return false;
		}
		else //Drag and drop in anaother place 
		{
				//Get the dropped text
			for(ElementTextItemGroup *grp : m_element.data()->textGroups())
			{
				for(DynamicElementTextItem *text : grp->texts())
				{
					if(text->uuid() == uuid)
					{
						deti = text;
						group = grp;
						break;
					}
				}
				if(deti)
					break;
			}
			
			if(deti && group) //Text successfully removed from group
			{
				m_element.data()->diagram()->undoStack().push((new RemoveTextFromGroupCommand(deti, group)));
				return true;
			}
			
			return false;
		}
	}
	
	return false;
}

QMimeData *DynamicElementTextModel::mimeData(const QModelIndexList &indexes) const
{	
	QModelIndex index = indexes.first();
	if (index.isValid())
	{
		QStandardItem *item =  itemFromIndex(index);
		if(item)
		{
			DynamicElementTextItem *deti = m_texts_list.key(item);
			if(deti)
			{
				auto *mime_data = new QMimeData();
				mime_data->setText(deti->uuid().toString());
				mime_data->setData("application/x-qet-element-text-uuid", deti->uuid().toString().toLatin1());
				return mime_data;
			}
		}
	}
	
	return QStandardItemModel::mimeData(indexes);
}

/**
 * @brief DynamicElementTextModel::mimeTypes
 * @return 
 */
QStringList DynamicElementTextModel::mimeTypes() const
{
	QStringList mime_list = QAbstractItemModel::mimeTypes();
	mime_list << "application/x-qet-element-text-uuid";
	return mime_list;
}

/**
 * @brief DynamicElementTextModel::enableSourceText
 * Enable the good field, according to the current source of text, for the edited text @deti
 * @param deti
 * @param tf
 */
void DynamicElementTextModel::enableSourceText(DynamicElementTextItem *deti, DynamicElementTextItem::TextFrom tf)
{
	if (!m_texts_list.contains(deti))
		return;

	QStandardItem *qsi = m_texts_list.value(deti);
	
	bool usr = false, info = false, compo = false;
	
	switch (tf) {
		case DynamicElementTextItem::UserText: usr = true; break;
		case DynamicElementTextItem::ElementInfo: info = true; break;
		case DynamicElementTextItem::CompositeText: compo = true;break;
	}

		//User text
	qsi->child(usr_txt_row,0)->setEnabled(usr);
	qsi->child(usr_txt_row,1)->setEnabled(usr);
		//Info text
	qsi->child(info_txt_row,0)->setEnabled(info);
	qsi->child(info_txt_row,1)->setEnabled(info);
		//Composite text
	qsi->child(compo_txt_row,0)->setEnabled(compo);
	qsi->child(compo_txt_row,1)->setEnabled(compo);
}

/**
 * @brief DynamicElementTextModel::enableGroupRotation
 * Enable/disable the item "group rotation" according the option hold to bottom 
 * @param group
 */
void DynamicElementTextModel::enableGroupRotationAndPos(ElementTextItemGroup *group)
{
	if(!m_groups_list.contains(group))
		return;
	
	QStandardItem *qsi = m_groups_list.value(group);
	
	if(group->holdToBottomPage())
	{
		qsi->child(x_grp_row, 0)->setFlags(Qt::ItemIsSelectable);
		qsi->child(x_grp_row, 1)->setFlags(Qt::ItemIsSelectable);
		qsi->child(y_grp_row, 0)->setFlags(Qt::ItemIsSelectable);
		qsi->child(y_grp_row, 1)->setFlags(Qt::ItemIsSelectable);
		qsi->child(rot_grp_row, 0)->setFlags(Qt::ItemIsSelectable);
		qsi->child(rot_grp_row, 1)->setFlags(Qt::ItemIsSelectable);
	}
	else
	{
		qsi->child(x_grp_row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		qsi->child(x_grp_row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		qsi->child(y_grp_row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		qsi->child(y_grp_row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		qsi->child(rot_grp_row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		qsi->child(rot_grp_row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	}
}

void DynamicElementTextModel::itemDataChanged(QStandardItem *qsi)
{
	DynamicElementTextItem *deti = textFromItem(qsi);
	ElementTextItemGroup *etig = groupFromItem(qsi);
	if (!deti && !etig)
		return;
	
	if(deti)
	{
		QStandardItem *text_qsi = m_texts_list.value(deti);
		DiagramContext dc;
		if(deti->elementUseForInfo())
			dc = deti->elementUseForInfo()->elementInformations();
		
		if (qsi->data().toInt() == textFrom)
		{
			QString from = qsi->data(Qt::DisplayRole).toString();
			
			if (from == tr("Texte utilisateur"))
			{
				enableSourceText(deti, DynamicElementTextItem::UserText);
				text_qsi->setData(text_qsi->child(usr_txt_row,1)->data(Qt::DisplayRole).toString());
			}
			else if (from == tr("Information de l'élément"))
			{
				enableSourceText(deti, DynamicElementTextItem::ElementInfo);
				QString info = text_qsi->child(info_txt_row,1)->data(Qt::UserRole+2).toString();
				text_qsi->setData(dc.value(info), Qt::DisplayRole);
			}
			else
			{
				enableSourceText(deti, DynamicElementTextItem::CompositeText);
				QString compo = text_qsi->child(compo_txt_row,1)->data(Qt::UserRole+2).toString();
				text_qsi->setData(autonum::AssignVariables::replaceVariable(compo, dc), Qt::DisplayRole);
			}
			
			
		}
		else if (qsi->data().toInt() == userText)
		{
			QString text = qsi->data(Qt::DisplayRole).toString();
			text_qsi->setData(text, Qt::DisplayRole);
		}
		else if (qsi->data().toInt() == infoText && deti->elementUseForInfo())
		{
			QString info = qsi->data(Qt::UserRole+2).toString();
			text_qsi->setData(dc.value(info), Qt::DisplayRole);
		}
		else if (qsi->data().toInt() == compositeText && deti->elementUseForInfo())
		{
			QString compo = qsi->data(Qt::UserRole+2).toString();
			text_qsi->setData(autonum::AssignVariables::replaceVariable(compo, dc), Qt::DisplayRole);
		}
	}
	
		//We emit the signal only if @qsi is in the second column, because the data are stored on this column
		//the first column is use only for display the title of the property, except for the name of texts group
	if((m_groups_list.values().contains(qsi) || qsi->column() == 1) && !m_block_dataChanged)
		emit dataChanged();
}

/**
 * @brief DynamicElementTextModel::setConnection
 * Set up the connection for @deti to keep up to date the data of this model and the text.
 * Is notably use with the use of QUndoCommand.
 * @param deti - text to setup connection
 * @param set - true = set connection - false unset connection
 */
void DynamicElementTextModel::setConnection(DynamicElementTextItem *deti, bool set)
{
	if(set)
	{
		if(m_hash_text_connect.keys().contains(deti))
			return;
		
		QList<QMetaObject::Connection> connection_list;
		connection_list << connect(deti, &DynamicElementTextItem::colorChanged,    [deti,this](){this->updateDataFromText(deti, color);});
		connection_list << connect(deti, &DynamicElementTextItem::fontSizeChanged, [deti,this](){this->updateDataFromText(deti, size);});
		connection_list << connect(deti, &DynamicElementTextItem::textFromChanged, [deti,this](){this->updateDataFromText(deti, textFrom);});
		connection_list << connect(deti, &DynamicElementTextItem::textChanged,     [deti,this](){this->updateDataFromText(deti, userText);});
		connection_list << connect(deti, &DynamicElementTextItem::infoNameChanged, [deti,this](){this->updateDataFromText(deti, infoText);});
		connection_list << connect(deti, &DynamicElementTextItem::xChanged,        [deti,this](){this->updateDataFromText(deti, pos);});
		connection_list << connect(deti, &DynamicElementTextItem::yChanged,        [deti,this](){this->updateDataFromText(deti, pos);});
		connection_list << connect(deti, &DynamicElementTextItem::frameChanged,    [deti,this](){this->updateDataFromText(deti, frame);});
		connection_list << connect(deti, &DynamicElementTextItem::rotationChanged, [deti,this](){this->updateDataFromText(deti, rotation);});
		connection_list << connect(deti, &DynamicElementTextItem::textWidthChanged,[deti,this](){this->updateDataFromText(deti, textWidth);});
		connection_list << connect(deti, &DynamicElementTextItem::compositeTextChanged, [deti, this]() {this->updateDataFromText(deti, compositeText);});
		m_hash_text_connect.insert(deti, connection_list);
	}
	else
	{
		if(!m_hash_text_connect.keys().contains(deti))
			return;
		
		for (const QMetaObject::Connection& con : m_hash_text_connect.value(deti))
			disconnect(con);
		
		m_hash_text_connect.remove(deti);
	}
}

/**
 * @brief DynamicElementTextModel::setConnection
 * Set up the connection for @group to keep up to date the data of this model and the group.
 * Is notably use with the use of QUndoCommand.
 * @param group group to setup the connection
 * @param set true = set connection - false unset connection
 */
void DynamicElementTextModel::setConnection(ElementTextItemGroup *group, bool set)
{
	if(set)
	{
		if(m_hash_group_connect.keys().contains(group))
			return;
		
		QList<QMetaObject::Connection> connection_list;
		connection_list << connect(group, &ElementTextItemGroup::alignmentChanged, [group, this]() {this->updateDataFromGroup(group, grpAlignment);});
		connection_list << connect(group, &ElementTextItemGroup::rotationChanged, [group, this]() {this->updateDataFromGroup(group, grpRotation);});
		connection_list << connect(group, &ElementTextItemGroup::verticalAdjustmentChanged, [group, this]() {this->updateDataFromGroup(group, grpVAdjust);});
		connection_list << connect(group, &ElementTextItemGroup::verticalAdjustmentChanged, [group, this]() {this->updateDataFromGroup(group, grpName);});
		connection_list << connect(group, &ElementTextItemGroup::holdToBottomPageChanged, [group, this]() {this->updateDataFromGroup(group, grpHoldBottom);});
		connection_list << connect(group, &ElementTextItemGroup::xChanged, [group, this]() {this->updateDataFromGroup(group, grpPos);});
		connection_list << connect(group, &ElementTextItemGroup::yChanged, [group, this]() {this->updateDataFromGroup(group, grpPos);});
		connection_list << connect(group, &ElementTextItemGroup::frameChanged, [group, this]() {this->updateDataFromGroup(group, grpFrame);});
		
		m_hash_group_connect.insert(group, connection_list);
	}
	else
	{
		if(!m_hash_group_connect.keys().contains(group))
			return;
		
		for (const QMetaObject::Connection& con : m_hash_group_connect.value(group))
			disconnect(con);
		
		m_hash_group_connect.remove(group);
	}
}

void DynamicElementTextModel::updateDataFromText(DynamicElementTextItem *deti, ValueType type)
{
	QStandardItem *qsi = m_texts_list.value(deti);
	if (!qsi)
		return;
	
	m_block_dataChanged = true;
	
	switch (type)
	{
		case textFrom:
		{
			switch (deti->textFrom())
			{
				case DynamicElementTextItem::UserText: qsi->child(0,1)->setData(tr("Texte utilisateur"), Qt::DisplayRole); break;
				case DynamicElementTextItem::ElementInfo : qsi->child(0,1)->setData(tr("Information de l'élément"), Qt::DisplayRole); break;
				case DynamicElementTextItem::CompositeText : qsi->child(0,1)->setData(tr("Texte composé"), Qt::DisplayRole); break;
			}
			enableSourceText(deti, deti->textFrom());
			qsi->setData(deti->toPlainText(), Qt::DisplayRole);
			break;
		}
		case userText:
		{
			qsi->setData(deti->toPlainText(), Qt::DisplayRole);
			qsi->child(usr_txt_row,1)->setData(deti->toPlainText(), Qt::DisplayRole);
			qsi->setData(deti->toPlainText(), Qt::DisplayRole);
			break;
		}
		case infoText:
		{
			qsi->setData(deti->toPlainText(), Qt::DisplayRole);
			QString info_name = deti->infoName();
			qsi->child(info_txt_row,1)->setData(info_name, Qt::UserRole+2);
			qsi->child(info_txt_row,1)->setData(QETApp::elementTranslatedInfoKey(info_name), Qt::DisplayRole);
			break;
		}
		case compositeText:
		{
			qsi->setData(deti->toPlainText(), Qt::DisplayRole);
			qsi->child(compo_txt_row,1)->setData(deti->compositeText(), Qt::UserRole+2);
			qsi->child(compo_txt_row,1)->setData(deti->toPlainText(), Qt::DisplayRole);
			qsi->setData(deti->toPlainText(), Qt::DisplayRole);
			break;
		}
		case size:
			qsi->child(size_txt_row,1)->setData(deti->fontSize(), Qt::EditRole);
			break;
		case color:
		{
			qsi->child(color_txt_row,1)->setData(deti->color(), Qt::EditRole);
			qsi->child(color_txt_row,1)->setData(deti->color(), Qt::ForegroundRole);
			break;
		}
		case pos:
		{
			if(qsi->child(x_txt_row,1))
				qsi->child(x_txt_row,1)->setData(deti->pos().x(), Qt::EditRole);
			if(qsi->child(y_txt_row,1))
				qsi->child(y_txt_row,1)->setData(deti->pos().y(), Qt::EditRole);
			break;
		}
		case frame:
		{
			qsi->child(frame_txt_row,1)->setCheckState(deti->frame()? Qt::Checked : Qt::Unchecked);
			break;
		}
		case rotation:
		{
			if(qsi->child(rot_txt_row,1))
				qsi->child(rot_txt_row,1)->setData(deti->rotation(), Qt::EditRole);
			break;
		}
		case textWidth:
		{
			qsi->child(width_txt_row,1)->setData(deti->textWidth(), Qt::EditRole);
			break;
		}
	default:break;
	}
	
	m_block_dataChanged = false;
}

void DynamicElementTextModel::updateDataFromGroup(ElementTextItemGroup *group, DynamicElementTextModel::ValueType type)
{
	QStandardItem *qsi = m_groups_list.value(group);
	if (!qsi)
		return;
	
	m_block_dataChanged = true;
	
	switch (type)
	{
		case grpAlignment:
		{
			switch (group->alignment())
			{
				case Qt::AlignLeft: qsi->child(align_grp_row,1)->setData(tr("Gauche"), Qt::DisplayRole); break;
				case Qt::AlignRight : qsi->child(align_grp_row,1)->setData(tr("Droite"), Qt::DisplayRole); break;
				case Qt::AlignVCenter : qsi->child(align_grp_row,1)->setData(tr("Centre"), Qt::DisplayRole); break;
				default: qsi->child(0,1)->setData("", Qt::DisplayRole); break;
			}
			 break;
		}
		case grpPos:
		{
			qsi->child(x_grp_row,1)->setData(group->pos().x(), Qt::EditRole);
			qsi->child(y_grp_row,1)->setData(group->pos().y(), Qt::EditRole);
		}
		case grpRotation:
			qsi->child(rot_grp_row,1)->setData(group->rotation(), Qt::EditRole);
			break;
		case grpVAdjust:
			qsi->child(adjust_grp_row,1)->setData(group->verticalAdjustment(), Qt::EditRole);
			break;
		case grpName:
			qsi->setData(group->name(), Qt::DisplayRole);
			break;
		case grpHoldBottom:
		{
			qsi->child(hold_to_bottom_grp_row,1)->setCheckState(group->holdToBottomPage()? Qt::Checked : Qt::Unchecked);
			enableGroupRotationAndPos(group);
			break;
		}
		case grpFrame:
			qsi->child(frame_grp_row, 1)->setCheckState(group->frame()? Qt::Checked : Qt::Unchecked);
			break;
		default:break;
	}
	
	m_block_dataChanged = false;
}


/***************************************************
 * A little delegate only for add a combobox and a color dialog,
 * for use with the model
 ***************************************************/

DynamicTextItemDelegate::DynamicTextItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{}

QWidget *DynamicTextItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	switch (index.data(Qt::UserRole+1).toInt())
	{
		case DynamicElementTextModel::textFrom:
		{
			auto *qcb = new QComboBox(parent);
			qcb->setObjectName("text_from");
			qcb->addItem(tr("Texte utilisateur"));
			qcb->addItem(tr("Information de l'élément"));
			qcb->addItem(tr("Texte composé"));
			return qcb;
		}
		case DynamicElementTextModel::infoText:
		{
			const auto *detm = static_cast<const DynamicElementTextModel *>(index.model());
			QStandardItem *qsi = detm->itemFromIndex(index);
			
			if(!qsi)
				break;
			
			DynamicElementTextItem *deti = detm->textFromIndex(index);
			if(!deti)
				break;
				
				//We use a QMap because the keys of the map are sorted, then no matter the curent local,
				//the value of the combo box are always alphabetically sorted
			QMap <QString, QString> info_map;
			for(const QString& str : availableInfo(deti)) {
				info_map.insert(QETApp::elementTranslatedInfoKey(str), str);
			}
			
			auto *qcb = new QComboBox(parent);
			qcb->setObjectName("info_text");
			for (const QString& key : info_map.keys()) {
				qcb->addItem(key, info_map.value(key));
			}
			return qcb;
		}
		case DynamicElementTextModel::compositeText:
		{
			const auto *detm = static_cast<const DynamicElementTextModel *>(index.model());
			QStandardItem *qsi = detm->itemFromIndex(index);
			
			if(!qsi)
				break;
			
			DynamicElementTextItem *deti = detm->textFromIndex(index);
			if(!deti)
				break;
			
			auto *cted = new CompositeTextEditDialog(deti, parent);
			cted->setObjectName("composite_text");
			return cted;
		}
		case DynamicElementTextModel::txtAlignment:
		{
			const auto *detm = static_cast<const DynamicElementTextModel *>(index.model());
			QStandardItem *qsi = detm->itemFromIndex(index);
			
			if(!qsi)
				break;
			
			DynamicElementTextItem *deti = detm->textFromIndex(index);
			if(!deti)
				break;
			
			AlignmentTextDialog *atd = new AlignmentTextDialog(deti->alignment(), parent);
			atd->setObjectName("alignment_text");
			return atd;
		}
		case DynamicElementTextModel::size:
		{
			auto *sb = new QSpinBox(parent);
			sb->setObjectName("font_size");
			sb->setFrame(false);
			return sb;
		}
		case DynamicElementTextModel::color:
		{
			QColorDialog *cd = new QColorDialog(index.data(Qt::EditRole).value<QColor>());
			cd->setObjectName("color_dialog");
			return cd;
		}
		case DynamicElementTextModel::pos:
		{
			auto *sb = new QSpinBox(parent);
			sb->setObjectName("pos_dialog");
			sb->setRange(-1000,10000);
			sb->setFrame(false);
			sb->setSuffix(" px");
			return sb;
		}
		case DynamicElementTextModel::rotation:
		{
			auto *sb = new QSpinBox(parent);
			sb->setObjectName("rot_spinbox");
			sb->setRange(0, 359);
			sb->setWrapping(true);
			sb->setFrame(false);
			sb->setSuffix(" °");
			return sb;
		}
		case DynamicElementTextModel::textWidth:
		{
			auto *sb = new QSpinBox(parent);
			sb->setObjectName("width_spinbox");
			sb->setRange(-1, 500);
			sb->setFrame(false);
			sb->setSuffix(" px");
			return sb;
		}
		case DynamicElementTextModel::grpAlignment:
		{
			auto *qcb = new QComboBox(parent);
			qcb->setFrame(false);
			qcb->setObjectName("group_alignment");
			qcb->addItem(tr("Gauche"));
			qcb->addItem(tr("Centre"));
			qcb->addItem(tr("Droite"));
			return qcb;
		}
		case DynamicElementTextModel::grpPos:
		{
			auto *sb = new QSpinBox(parent);
			sb->setObjectName("group_pos");
			sb->setRange(-1000,10000);
			sb->setFrame(false);
			sb->setSuffix(" px");
			return sb;
		}
		case DynamicElementTextModel::grpRotation:
		{
			auto *sb = new QSpinBox(parent);
			sb->setObjectName("group_rotation");
			sb->setRange(0, 359);
			sb->setWrapping(true);
			sb->setFrame(false);
			sb->setSuffix(" °");
			return sb;
		}
		case DynamicElementTextModel::grpVAdjust:
		{
			auto *sb = new QSpinBox(parent);
			sb->setObjectName("group_v_adjustment");
			sb->setRange(-20, 20);
			sb->setFrame(false);
			sb->setSuffix(" px");
			return sb;
		}
	}
	return QStyledItemDelegate::createEditor(parent, option, index);
}

void DynamicTextItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (index.isValid())
	{
		if(editor->objectName() == "color_dialog")
		{
			if (auto *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					auto *cd = static_cast<QColorDialog *> (editor);
					qsi->setData(cd->selectedColor(), Qt::EditRole);
					qsi->setData(cd->selectedColor(), Qt::ForegroundRole);
					return;
				}
				
			}
		}
		else if (editor->objectName() == "info_text")
		{
			if (auto *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					auto *cb = static_cast<QComboBox *>(editor);
					qsi->setData(cb->currentText(), Qt::DisplayRole);
					qsi->setData(cb->currentData(), Qt::UserRole+2);
					return;
				}
				
			}
		}
		else if (editor->objectName() == "composite_text")
		{
			if (auto *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					
					auto *cted = static_cast<CompositeTextEditDialog *>(editor);
					
					QString edited_text = cted->plainText();
					QString assigned_text;
					
					const auto *detm = static_cast<const DynamicElementTextModel *>(index.model());
					DynamicElementTextItem *deti = detm->textFromIndex(index);
					if(deti)
					{
						DiagramContext dc;
						if(deti->elementUseForInfo())
							dc = deti->elementUseForInfo()->elementInformations();
						assigned_text = autonum::AssignVariables::replaceVariable(edited_text, dc);
					}
					
					qsi->setData(assigned_text, Qt::DisplayRole);
					qsi->setData(edited_text, Qt::UserRole+2);
					return;
				}
			}
		}
		else if (editor->objectName() == "alignment_text")
		{	
			if(auto *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					auto *atd = static_cast<AlignmentTextDialog *>(editor);
					Qt::Alignment align = atd->alignment();
					qsi->setData(QVariant::fromValue(align), Qt::UserRole+2);
					return;
				}
			}
		}
		else if (editor->objectName() == "group_alignment")
		{
			if(auto *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					auto *cb = static_cast<QComboBox *>(editor);
					qsi->setData(cb->currentText(), Qt::DisplayRole);
				}
			}
		}
	}
	
	QStyledItemDelegate::setModelData(editor, model, index);
}

bool DynamicTextItemDelegate::eventFilter(QObject *object, QEvent *event)
{
	
		//This is a bad hack, for change the normal behavior :
		//in normal behavior, the value is commited when the spinbox lose focus or enter key is pressed
		//With this hack the value is commited each time the value change without the need to validate.
		//then the change is apply in live
	if(object->objectName() == "pos_dialog" || object->objectName() == "font_size" || object->objectName() == "rot_spinbox" || \
	   object->objectName() == "group_rotation" || object->objectName() == "group_v_adjustment" || object->objectName() == "width_spinbox" ||\
	   object->objectName() == "group_pos")
	{
		object->event(event);
		
		auto *sb = static_cast<QSpinBox *>(object);
		switch (event->type()) {
			case QEvent::KeyPress:
				emit commitData(sb); break;
			case QEvent::KeyRelease:
				emit commitData(sb); break;
			case QEvent::MouseButtonPress:
				emit commitData(sb); break;
			case QEvent::MouseButtonRelease:
				emit commitData(sb); break;
			case QEvent::Wheel:
				emit commitData(sb); break;
			default:break;
		}
		return true;
	}
	
		//Like the hack above, change the current index of the combobox, apply the change immediately, no need to lose focus or press enter.
	if((object->objectName() == "text_from" || object->objectName() == "info_text" || object->objectName() == "group_alignment") && event->type() == QEvent::FocusIn)
	{
		auto *qcb = static_cast<QComboBox *>(object);
		connect(qcb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this,qcb](){emit commitData(qcb);});
	}
	
	return QStyledItemDelegate::eventFilter(object, event);
}

/**
 * @brief DynamicTextItemDelegate::availableInfo
 * @param deti
 * @return A list of available info of element
 */
QStringList DynamicTextItemDelegate::availableInfo(DynamicElementTextItem *deti) const
{
	QStringList qstrl;
	
	if(deti->parentElement()->linkType() & Element::AllReport) //Special treatment for text owned by a folio report
	{
		qstrl << "label";
		
		if(!deti->m_watched_conductor.isNull())
		{
			Conductor *cond = deti->m_watched_conductor.data();
			if (!cond->properties().m_function.isEmpty())
				qstrl << "function";
			if(!cond->properties().m_tension_protocol.isEmpty())
				qstrl << "tension-protocol";
		}
		 
		 return qstrl;
	}
	else
	{
		Element *elmt = deti->elementUseForInfo();
		if(!elmt)
			return qstrl;
		
		
		QStringList info_list = QETApp::elementInfoKeys();
		info_list.removeAll("formula"); //No need to have formula
		DiagramContext dc = elmt->elementInformations();
		
		for(const QString& info : info_list)
		{
			if(dc.contains(info))
				qstrl << info;
		}
	}
	return qstrl;
}
