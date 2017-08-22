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

DynamicElementTextModel::DynamicElementTextModel(QObject *parent) :
QStandardItemModel(parent)
{
    setColumnCount(2);
    setHeaderData(0, Qt::Horizontal, tr("Propriété"), Qt::DisplayRole);
    setHeaderData(1, Qt::Horizontal, tr("Valeur"), Qt::DisplayRole);
    
    connect(this, &DynamicElementTextModel::itemChanged, this, &DynamicElementTextModel::dataEdited);
}

DynamicElementTextModel::~DynamicElementTextModel()
{
		//Connection is not destroy automaticaly,
		//because was not connected to a slot, but a lambda
	for(DynamicElementTextItem *deti : m_hash_text_connect.keys())
		setConnection(deti, false);
}

/**
 * @brief DynamicElementTextModel::addText
 * @param deti
 */
void DynamicElementTextModel::addText(DynamicElementTextItem *deti)
{
    if(m_texts_list.keys().contains(deti))
        return;
    
    QList <QStandardItem *> qsi_list;
    
	QStandardItem *qsi = new QStandardItem(deti->toPlainText());
    qsi->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	
        //Source of text
    QStandardItem *src = new QStandardItem(tr("Source du texte"));
    src->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
	QString title;
	if (deti->textFrom() == DynamicElementTextItem::UserText) title = tr("Texte utilisateur");
	else if (deti->textFrom() == DynamicElementTextItem::ElementInfo) title = tr("Information de l'élément");
	else title =  tr("Texte composé");
    QStandardItem *srca = new QStandardItem(title);
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
    src->appendRow(qsi_list);
	
		//Info text
	QStandardItem *info = new QStandardItem(tr("Information"));
    info->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    QStandardItem *infoa = new QStandardItem(QETApp::elementTranslatedInfoKey(deti->infoName()));
    infoa->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    infoa->setData(DynamicElementTextModel::infoText, Qt::UserRole+1); //Use to know the edited thing
	infoa->setData(deti->infoName(), Qt::UserRole+2); //Use to know to element info name
    
	qsi_list.clear();
    qsi_list << info << infoa;
    src->appendRow(qsi_list);
	
		//Composite text
	QStandardItem *composite = new QStandardItem(tr("Texte composé"));
	composite->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	
	QStandardItem *compositea = new QStandardItem(deti->compositeText().isEmpty() ?
													  tr("Mon texte composé") :
													  autonum::AssignVariables::replaceVariable(deti->compositeText(), deti->parentElement()->elementInformations()));
	compositea->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	compositea->setData(DynamicElementTextModel::compositeText, Qt::UserRole+1); //Use to know the edited thing
	compositea->setData(deti->compositeText(), Qt::UserRole+2); //Use to know to element composite formula
	
	qsi_list.clear();
	qsi_list << composite << compositea;
	src->appendRow(qsi_list);

    
        //Size
	QStandardItem *size = new QStandardItem(tr("Taille"));
    size->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    QStandardItem *siza = new QStandardItem();
    siza->setData(deti->fontSize(), Qt::EditRole);
    siza->setData(DynamicElementTextModel::size, Qt::UserRole+1);
    siza->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    
    qsi_list.clear();
    qsi_list << size << siza;
	qsi->appendRow(qsi_list);
    
        //Tagg
    QStandardItem *tagg = new QStandardItem(tr("Tagg"));
    tagg->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    QStandardItem *tagga = new QStandardItem(deti->tagg());
    tagga->setData(DynamicElementTextModel::tagg, Qt::UserRole+1);
    tagga->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    
    qsi_list.clear();
    qsi_list << tagg << tagga;
	qsi->appendRow(qsi_list);
    
        //Color
    QStandardItem *color = new QStandardItem(tr("Couleur"));
    color->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    QStandardItem *colora = new QStandardItem;
    colora->setData(deti->color(), Qt::ForegroundRole);
    colora->setData(deti->color(), Qt::EditRole);
    colora->setData(DynamicElementTextModel::color, Qt::UserRole+1);
    colora->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    
    qsi_list.clear();
    qsi_list << color << colora;
	qsi->appendRow(qsi_list);
    
	qsi_list.clear();
	QStandardItem *empty_qsi = new QStandardItem(0);
	empty_qsi->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	qsi_list << qsi << empty_qsi;
    this->appendRow(qsi_list);
	
    m_texts_list.insert(deti, qsi);
	blockSignals(true);
	enableSourceText(deti, deti->textFrom());
	blockSignals(false);
	setConnection(deti, true);
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
 * @return the text associated with @index. Return value can be nullptr
 * @Index can be a child of an index associated with a text
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
 * @item can be a child of an item associated with a text
 */
DynamicElementTextItem *DynamicElementTextModel::textFromItem(QStandardItem *item) const
{
	QStandardItem *text_item = item;
	while (text_item->parent())
		text_item = text_item->parent();
	
	if (m_texts_list.values().contains(text_item))
		return m_texts_list.key(text_item);
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
	if(!m_texts_list.contains(text))
		return QModelIndex();
	
	return m_texts_list.value(text)->index();
}

/**
 * @brief DynamicElementTextModel::undoForEditedText
 * @param deti
 * @return A QUndoCommand that describe all changes made for @deti.
 * Each change made for @deti is append as a child of the returned QUndoCommand.
 * In other word, if the returned QUndoCommand have no child, that mean there is no change.
 */
QUndoCommand *DynamicElementTextModel::undoForEditedText(DynamicElementTextItem *deti) const
{
	QUndoCommand *undo = new QUndoCommand(tr("Éditer un texte d'élément"));
	
	if (!m_texts_list.contains(deti))
		return undo;
	
	QStandardItem *text_qsi = m_texts_list.value(deti);
	
	QString from = text_qsi->child(0,1)->data(Qt::DisplayRole).toString();
	if ((from == tr("Texte utilisateur")) && (deti->textFrom() != DynamicElementTextItem::UserText))
		new QPropertyUndoCommand(deti, "textFrom", QVariant(deti->textFrom()), QVariant(DynamicElementTextItem::UserText), undo);
	else if ((from == tr("Information de l'élément")) && (deti->textFrom() != DynamicElementTextItem::ElementInfo))
		new QPropertyUndoCommand(deti, "textFrom", QVariant(deti->textFrom()), QVariant(DynamicElementTextItem::ElementInfo), undo);
	else if ((from == tr("Texte composé")) && (deti->textFrom() != DynamicElementTextItem::CompositeText))
		new QPropertyUndoCommand(deti, "textFrom", QVariant(deti->textFrom()), QVariant(DynamicElementTextItem::CompositeText), undo);
	
	if(from == tr("Texte utilisateur"))
	{
		QString text = text_qsi->child(0,0)->child(0,1)->data(Qt::DisplayRole).toString();
		if (text != deti->text())
			new QPropertyUndoCommand(deti, "text", QVariant(deti->text()), QVariant(text), undo);
	}
	else if (from == tr("Information de l'élément"))
	{
		QString info_name = text_qsi->child(0,0)->child(1,1)->data(Qt::UserRole+2).toString();
		if(info_name != deti->infoName())
			new QPropertyUndoCommand(deti, "infoName", QVariant(deti->infoName()), QVariant(info_name), undo);
	}
	else if (from == tr("Texte composé"))
	{
		QString composite_text = text_qsi->child(0,0)->child(2,1)->data(Qt::UserRole+2).toString();
		if(composite_text != deti->compositeText())
			new QPropertyUndoCommand(deti, "compositeText", QVariant(deti->compositeText()), QVariant(composite_text), undo);
	}
	
	int fs = text_qsi->child(1,1)->data(Qt::EditRole).toInt();
	if (fs != deti->fontSize()) 
		new QPropertyUndoCommand(deti, "fontSize", QVariant(deti->fontSize()), QVariant(fs), undo);
	
	QString tagg = text_qsi->child(2,1)->data(Qt::DisplayRole).toString();
	if(tagg != deti->tagg())
		new QPropertyUndoCommand(deti, "tagg", QVariant(deti->tagg()), QVariant(tagg), undo);
	
	QColor color = text_qsi->child(3,1)->data(Qt::EditRole).value<QColor>();
	if(color != deti->color())
		new QPropertyUndoCommand(deti, "color", QVariant(deti->color()), QVariant(color), undo);
	
	return undo;
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
	
	QStandardItem *qsi = m_texts_list.value(deti)->child(0,0);
	
	bool usr = false, info = false, compo = false;
	
	switch (tf) {
		case DynamicElementTextItem::UserText: usr = true; break;
		case DynamicElementTextItem::ElementInfo: info = true; break;
		case DynamicElementTextItem::CompositeText: compo = true; break;
	}
	
		//User text
	qsi->child(0,0)->setEnabled(usr);
	qsi->child(0,1)->setEnabled(usr);
		//Info text
	qsi->child(1,0)->setEnabled(info);
	qsi->child(1,1)->setEnabled(info);
		//Composite text
	qsi->child(2,0)->setEnabled(compo);
	qsi->child(2,1)->setEnabled(compo);
}

void DynamicElementTextModel::dataEdited(QStandardItem *qsi)
{
	DynamicElementTextItem *deti = textFromItem(qsi);
	if (!deti)
		return;
	
	blockSignals(true);
	
	if (qsi->data().toInt() == textFrom)
	{
		QString from = qsi->data(Qt::DisplayRole).toString();
		if (from == tr("Texte utilisateur"))
			enableSourceText(deti, DynamicElementTextItem::UserText);
		else if (from == tr("Information de l'élément"))
			enableSourceText(deti, DynamicElementTextItem::ElementInfo);
		else
			enableSourceText(deti, DynamicElementTextItem::CompositeText);
	}
	else if (qsi->data().toInt() == userText)
	{
		QString text = qsi->data(Qt::DisplayRole).toString();
		m_texts_list.value(deti)->setData(text, Qt::DisplayRole);
	}
	else if (qsi->data().toInt() == infoText && deti->parentElement())
	{
		QString info = qsi->data(Qt::UserRole+2).toString();
		m_texts_list.value(deti)->setData(deti->parentElement()->elementInformations().value(info), Qt::DisplayRole);
	}
	else if (qsi->data().toInt() == compositeText && deti->parentElement())
	{
		QString compo = qsi->data(Qt::UserRole+2).toString();
		m_texts_list.value(deti)->setData(autonum::AssignVariables::replaceVariable(compo, deti->parentElement()->elementInformations()), Qt::DisplayRole);
	}
	
	blockSignals(false);
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
		connection_list << connect(deti, &DynamicElementTextItem::taggChanged,     [deti,this](){this->updateDataFromText(deti, tagg);});
		connection_list << connect(deti, &DynamicElementTextItem::textChanged,     [deti,this](){this->updateDataFromText(deti, userText);});
		connection_list << connect(deti, &DynamicElementTextItem::textFromChanged, [deti,this](){this->updateDataFromText(deti, textFrom);});
		
		m_hash_text_connect.insert(deti, connection_list);
	}
	else
	{
		if(!m_hash_text_connect.keys().contains(deti))
			return;
		
		for (QMetaObject::Connection con : m_hash_text_connect.value(deti))
			disconnect(con);
		
		m_hash_text_connect.remove(deti);
	}
}

void DynamicElementTextModel::updateDataFromText(DynamicElementTextItem *deti, ValueType type)
{
	QStandardItem *qsi = m_texts_list.value(deti);
	if (!qsi)
		return;
	
	switch (type)
	{
		case textFrom:
		{
			switch (deti->textFrom()) {
				case DynamicElementTextItem::UserText: qsi->child(0,1)->setData(tr("Texte utilisateur"), Qt::DisplayRole); break;
				case DynamicElementTextItem::ElementInfo : qsi->child(0,1)->setData(tr("Information de l'élément"), Qt::DisplayRole); break;
				case DynamicElementTextItem::CompositeText : qsi->child(0,1)->setData(tr("Texte composé"), Qt::DisplayRole); break;
			}
			break;
		}
		case userText:
		{
			QStandardItem *qsia = qsi->child(0,0);
			qsia->child(0,1)->setData(deti->toPlainText(), Qt::DisplayRole);
			qsi->setData(deti->toPlainText(), Qt::DisplayRole);
			break;
		}
		case infoText:
			break;
		case compositeText:
			break;
		case size:
			qsi->child(1,1)->setData(deti->fontSize(), Qt::EditRole);
			break;
		case tagg:
			qsi->child(2,1)->setData(deti->tagg(), Qt::DisplayRole);
			break;
		case color:
		{
			qsi->child(3,1)->setData(deti->color(), Qt::EditRole);
			qsi->child(3,1)->setData(deti->color(), Qt::ForegroundRole);
			break;
		}
	}
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
			QComboBox *qcb = new QComboBox(parent);
			qcb->addItem(tr("Texte utilisateur"));
			qcb->addItem(tr("Information de l'élément"));
			qcb->addItem(tr("Texte composé"));
			return qcb;
		}
		case DynamicElementTextModel::infoText:
		{
			const DynamicElementTextModel *detm = static_cast<const DynamicElementTextModel *>(index.model());
			QStandardItem *qsi = detm->itemFromIndex(index);
			
			if(!qsi)
				break;
			
			DynamicElementTextItem *deti = detm->textFromIndex(index);
			if(!deti)
				break;
				
				//We use a QMap because the keys of the map are sorted, then no matter the curent local,
				//the value of the combo box are always alphabetically sorted
			QMap <QString, QString> info_map;
			for(QString str : availableInfo(deti)) {
				info_map.insert(QETApp::elementTranslatedInfoKey(str), str);
			}
			
			QComboBox *qcb = new QComboBox(parent);
			qcb->setObjectName("info_text");
			for (QString key : info_map.keys()) {
				qcb->addItem(key, info_map.value(key));
			}
			return qcb;
		}
		case DynamicElementTextModel::compositeText:
		{
			const DynamicElementTextModel *detm = static_cast<const DynamicElementTextModel *>(index.model());
			QStandardItem *qsi = detm->itemFromIndex(index);
			
			if(!qsi)
				break;
			
			DynamicElementTextItem *deti = detm->textFromIndex(index);
			if(!deti)
				break;
			
			CompositeTextEditDialog *cted = new CompositeTextEditDialog(deti);
			cted->setObjectName("composite_text");
			return cted;
		}
		case DynamicElementTextModel::color:
		{
			QColorDialog *cd = new QColorDialog(index.data(Qt::EditRole).value<QColor>());
			cd->setObjectName("color_dialog");
			return cd;
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
			if (QStandardItemModel *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					QColorDialog *cd = static_cast<QColorDialog *> (editor);
					qsi->setData(cd->selectedColor(), Qt::EditRole);
					qsi->setData(cd->selectedColor(), Qt::ForegroundRole);
					return;
				}
				
			}
		}
		else if (editor->objectName() == "info_text")
		{
			if (QStandardItemModel *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					QComboBox *cb = static_cast<QComboBox *>(editor);
					qsi->setData(cb->currentText(), Qt::DisplayRole);
					qsi->setData(cb->currentData(), Qt::UserRole+2);
					return;
				}
				
			}
		}
		else if (editor->objectName() == "composite_text")
		{
			if (QStandardItemModel *qsim = dynamic_cast<QStandardItemModel *>(model))
			{
				if(QStandardItem *qsi = qsim->itemFromIndex(index))
				{
					
					CompositeTextEditDialog *cted = static_cast<CompositeTextEditDialog *>(editor);
					
					QString edited_text = cted->plainText();
					QString assigned_text;
					
					const DynamicElementTextModel *detm = static_cast<const DynamicElementTextModel *>(index.model());
					DynamicElementTextItem *deti = detm->textFromIndex(index);
					if(deti)
					{
						DiagramContext dc = deti->parentElement()->elementInformations();
						assigned_text = autonum::AssignVariables::replaceVariable(edited_text, dc);
					}
					
					qsi->setData(assigned_text, Qt::DisplayRole);
					qsi->setData(edited_text, Qt::UserRole+2);
					return;
				}
			}
		}
	}
	
	QStyledItemDelegate::setModelData(editor, model, index);
}

/**
 * @brief DynamicTextItemDelegate::availableInfo
 * @param deti
 * @return A list of available info of element
 */
QStringList DynamicTextItemDelegate::availableInfo(DynamicElementTextItem *deti) const
{
	QStringList qstrl;
	Element *elmt = deti->parentElement();
	if(!elmt)
		return qstrl;
	
	QStringList info_list = QETApp::elementInfoKeys();
	info_list.removeAll("formula"); //No need to have formula
	DiagramContext dc = elmt->elementInformations();
	
	for(QString info : info_list)
	{
		if(dc.contains(info))
			qstrl << info;
	}
	
	return qstrl;
}
