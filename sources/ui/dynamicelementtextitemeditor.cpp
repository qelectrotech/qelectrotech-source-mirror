/*
	Copyright 2006-2017 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "dynamicelementtextitemeditor.h"
#include "ui_dynamicelementtextitemeditor.h"
#include "dynamicelementtextitem.h"
#include "element.h"
#include "dynamicelementtextmodel.h"
#include "diagram.h"
#include "undocommand/deleteqgraphicsitemcommand.h"
#include "undocommand/addelementtextcommand.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementtextitemgroup.h"
#include "deleteqgraphicsitemcommand.h"
#include "elementtextpattern.h"

#include <QTreeView>
#include <QUndoCommand>

DynamicElementTextItemEditor::DynamicElementTextItemEditor(Element *element, QWidget *parent) :
    AbstractElementPropertiesEditorWidget(parent),
    ui(new Ui::DynamicElementTextItemEditor)
{
    ui->setupUi(this);
	
    ui->m_tree_view->setItemDelegate(new DynamicTextItemDelegate(ui->m_tree_view));
	ui->m_remove_selection->setDisabled(true);
	
    setElement(element);
}

DynamicElementTextItemEditor::~DynamicElementTextItemEditor()
{
	delete ui;
}

void DynamicElementTextItemEditor::setElement(Element *element)
{
    if (m_element == element)
        return;
    
     m_element = element;
    
    DynamicElementTextModel *old_model = m_model;
    m_model = new DynamicElementTextModel(element, ui->m_tree_view);
	connect(m_model, &DynamicElementTextModel::dataChanged, this, &DynamicElementTextItemEditor::dataEdited);
    ui->m_tree_view->setModel(m_model);
    
    if(old_model)
		delete old_model;
}

bool DynamicElementTextItemEditor::setLiveEdit(bool live_edit)
{
	m_live_edit = live_edit;
	return true;
}

void DynamicElementTextItemEditor::apply()
{
	QList <QUndoCommand *> undo_list;
	
		//Get all dynamic text item of the element
	QList <DynamicElementTextItem *> deti_list;
	deti_list << m_element.data()->dynamicTextItems();
		for(ElementTextItemGroup *group : m_element.data()->textGroups())
			deti_list << group->texts();
		
	for (DynamicElementTextItem *deti : deti_list)
	{
		QUndoCommand *undo = m_model->undoForEditedText(deti);

		if (undo->childCount() == 1)
		{
			auto *quc = new QPropertyUndoCommand(static_cast<const QPropertyUndoCommand *>(undo->child(0)));
			if (quc->text().isEmpty())
				quc->setText(undo->text());
			undo_list << quc;
			delete undo;
		}
		else if(undo->childCount() > 1)
			undo_list << undo;
		else
			delete undo;
	}
	
		//Get all texts groups of the edited element
	for (ElementTextItemGroup *group : m_element.data()->textGroups())
	{
		QUndoCommand *undo = m_model->undoForEditedGroup(group);
		
		if (undo->childCount() == 1)
		{
			auto *quc = new QPropertyUndoCommand(static_cast<const QPropertyUndoCommand *>(undo->child(0)));
			if (quc->text().isEmpty())
				quc->setText(undo->text());
			undo_list << quc;
			delete undo;
		}
		else if(undo->childCount() > 1)
			undo_list << undo;
		else
			delete undo;
	}
	
	if(!undo_list.isEmpty() && m_element->diagram())
	{
		if (undo_list.size() == 1)
		{
			m_element->diagram()->undoStack().push(undo_list.first());
		}
		else
		{
			QUndoStack &us = m_element->diagram()->undoStack();
			us.beginMacro(tr("Modifier des textes d'élément"));
			for (QUndoCommand *quc : undo_list)
				us.push(quc);
			us.endMacro();
		}
	}
}

/**
 * @brief DynamicElementTextItemEditor::setCurrentText
 * Expand and select the item for text @text
 * @param text
 */
void DynamicElementTextItemEditor::setCurrentText(DynamicElementTextItem *text)
{
	QModelIndex index = m_model->indexFromText(text);
	if(!index.isValid())
		return;
	
	ui->m_tree_view->expand(index);
	ui->m_tree_view->expand(index.child(0,0));
	ui->m_tree_view->setCurrentIndex(index);
	ui->m_remove_selection->setEnabled(true);
}

/**
 * @brief DynamicElementTextItemEditor::setCurrentGroup
 * Expand and select the item for group @group
 * @param group
 */
void DynamicElementTextItemEditor::setCurrentGroup(ElementTextItemGroup *group)
{
	QModelIndex index = m_model->indexFromGroup(group);
	if(!index.isValid())
		return;
	
	ui->m_tree_view->expand(index);
	ui->m_tree_view->setCurrentIndex(index);
	ui->m_remove_selection->setEnabled(true);
}

QUndoCommand *DynamicElementTextItemEditor::associatedUndo() const
{
	QUndoCommand *parent_undo = new QUndoCommand(tr("Modifier un texte d'élément"));
	for (DynamicElementTextItem *deti : m_element.data()->dynamicTextItems())
		m_model->undoForEditedText(deti, parent_undo);
	
	for (ElementTextItemGroup *grp : m_element.data()->textGroups())
		m_model->undoForEditedGroup(grp, parent_undo);
	
	if(parent_undo->childCount() >= 1)
	{
		if(parent_undo->childCount() >= 2)
			parent_undo->setText(tr("Modifier %1 textes d'élément").arg(QString::number(parent_undo->childCount())));
		return parent_undo;
	}
	else
		return nullptr;
}

void DynamicElementTextItemEditor::dataEdited()
{
	if (m_live_edit)
		apply();
}

/**
 * @brief DynamicElementTextItemEditor::on_m_add_text_clicked
 * Add a new dynamic text
 */
void DynamicElementTextItemEditor::on_m_add_text_clicked()
{
    if (!m_element)
        return;
	
    auto *deti = new DynamicElementTextItem(m_element);
	if (m_element->diagram())
	{
		m_element->diagram()->undoStack().push(new AddElementTextCommand(m_element, deti));
		setCurrentText(deti);
	}
	else
	{
		delete deti;
	}
}

/**
 * @brief DynamicElementTextItemEditor::on_m_remove_selection_clicked
 * Remove the selected item
 */
void DynamicElementTextItemEditor::on_m_remove_selection_clicked()
{
    DynamicElementTextItem *deti = m_model->textFromIndex(ui->m_tree_view->currentIndex());
    if(deti)
    {
		if(m_element->diagram())
		{
			DiagramContent dc;
			dc.m_element_texts << deti;
			m_element->diagram()->undoStack().push(new DeleteQGraphicsItemCommand(m_element->diagram(), dc));
		}
		return;
    }
	ElementTextItemGroup *group = m_model->groupFromIndex(ui->m_tree_view->currentIndex());
	if(group && m_element.data()->diagram())
		m_element.data()->diagram()->undoStack().push(new RemoveTextsGroupCommand(m_element.data(), group));
}

/**
 * @brief DynamicElementTextItemEditor::on_m_add_group_clicked
 * Add a new group
 */
void DynamicElementTextItemEditor::on_m_add_group_clicked()
{
	QString name = QInputDialog::getText(this, tr("Nom du groupe"), tr("Entrer le nom du nouveau groupe"));
	
	if(name.isEmpty())
		return;
	else if (m_element.data()->diagram())
		m_element.data()->diagram()->undoStack().push(new AddTextsGroupCommand(m_element, name));
}

void DynamicElementTextItemEditor::on_m_tree_view_clicked(const QModelIndex &index)
{
	if(m_model->indexIsText(index) || m_model->indexIsGroup(index))
		ui->m_remove_selection->setEnabled(true);
	else
		ui->m_remove_selection->setDisabled(true);
}

void DynamicElementTextItemEditor::on_m_export_pb_clicked()
{
    ExportElementTextPattern eetp(m_element);
}

void DynamicElementTextItemEditor::on_m_import_pb_clicked()
{
    ImportElementTextPattern ietp(m_element);
}
