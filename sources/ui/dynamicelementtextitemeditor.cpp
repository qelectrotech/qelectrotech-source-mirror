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

#include <QTreeView>
#include <QUndoCommand>

DynamicElementTextItemEditor::DynamicElementTextItemEditor(Element *element, QWidget *parent) :
    AbstractElementPropertiesEditorWidget(parent),
    ui(new Ui::DynamicElementTextItemEditor)
{
    ui->setupUi(this);
    m_tree_view = new QTreeView(this);
    m_tree_view->header()->setDefaultSectionSize(150);
    m_tree_view->setItemDelegate(new DynamicTextItemDelegate(m_tree_view));
	m_tree_view->setAlternatingRowColors(true);
	m_tree_view->setEditTriggers(QAbstractItemView::CurrentChanged);
    ui->verticalLayout->addWidget(m_tree_view);
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
    m_model = new DynamicElementTextModel(m_tree_view);
	connect(m_model, &DynamicElementTextModel::dataForTextChanged, this, &DynamicElementTextItemEditor::dataEdited);
    
    for (DynamicElementTextItem *deti : m_element->dynamicTextItems())
        m_model->addText(deti);
    
    m_tree_view->setModel(m_model);
    
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
	for (DynamicElementTextItem *deti : m_element->dynamicTextItems())
	{
		QUndoCommand *undo = m_model->undoForEditedText(deti);
		if(undo->childCount())
			undo_list << undo;
		else
			delete undo;
	}
	
	for (DynamicElementTextItem *deti : m_element->dynamicTextItems())
		deti->blockSignals(true);
	
	if(!undo_list.isEmpty() && m_element->diagram())
	{
		if (undo_list.size() == 1)
			m_element->diagram()->undoStack().push(undo_list.first());
		else
		{
			QUndoStack &us = m_element->diagram()->undoStack();
			us.beginMacro(tr("Modifier des textes d'élément"));
			for (QUndoCommand *quc : undo_list)
				us.push(quc);
			us.endMacro();
		}
	}
	
	for (DynamicElementTextItem *deti : m_element->dynamicTextItems())
		deti->blockSignals(false);
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
	
	m_tree_view->expand(index);
	m_tree_view->setCurrentIndex(index);
}

QUndoCommand *DynamicElementTextItemEditor::associatedUndo() const
{
	QUndoCommand *parent_undo = new QUndoCommand(tr("Modifier un texte d'élément"));
	for (DynamicElementTextItem *deti : m_element->dynamicTextItems())
		m_model->undoForEditedText(deti, parent_undo);
	
	if(parent_undo->childCount() >= 1)
	{
		if(parent_undo->childCount() >= 2)
			parent_undo->setText(tr("Modifier %1 textes d'élément").arg(QString::number(parent_undo->childCount())));
		return parent_undo;
	}
	else
		return nullptr;
}

void DynamicElementTextItemEditor::dataEdited(DynamicElementTextItem *deti)
{
	Q_UNUSED(deti)
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
	
    DynamicElementTextItem *deti = new DynamicElementTextItem(m_element);
	if (m_element->diagram())
	{
		m_element->diagram()->undoStack().push(new AddElementTextCommand(m_element, deti));
		m_model->addText(deti);
	}
	else
	{
		delete deti;
	}
}

/**
 * @brief DynamicElementTextItemEditor::on_m_remove_text_clicked
 * Remove the selected text field
 */
void DynamicElementTextItemEditor::on_m_remove_text_clicked()
{
    DynamicElementTextItem *deti = m_model->textFromIndex(m_tree_view->currentIndex());
    if(deti)
    {
		if(m_element->diagram())
		{
			DiagramContent dc;
			dc.m_element_texts << deti;
			m_element->diagram()->undoStack().push(new DeleteQGraphicsItemCommand(m_element->diagram(), dc));
			m_model->removeText(deti);
		}
    }
}
