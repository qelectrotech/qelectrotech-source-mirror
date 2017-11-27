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
	m_tree_view->installEventFilter(this);
    ui->verticalLayout->addWidget(m_tree_view);
	
	setUpAction();
	
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
    m_model = new DynamicElementTextModel(element, m_tree_view);
	connect(m_model, &DynamicElementTextModel::dataForTextChanged, this, &DynamicElementTextItemEditor::dataEdited);
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
			QPropertyUndoCommand *quc = new QPropertyUndoCommand(static_cast<const QPropertyUndoCommand *>(undo->child(0)));
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
	
	m_tree_view->expand(index);
	m_tree_view->expand(index.child(0,0));
	m_tree_view->setCurrentIndex(index);
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

/**
 * @brief DynamicElementTextItemEditor::eventFilter
 * Reimplemented for intercept the context menu event of the tree view
 * @param watched
 * @param event
 * @return 
 */
bool DynamicElementTextItemEditor::eventFilter(QObject *watched, QEvent *event)
{
	if(watched == m_tree_view && event->type() == QEvent::ContextMenu)
	{
		QContextMenuEvent *qcme = static_cast<QContextMenuEvent *>(event);
		QModelIndex index = m_tree_view->currentIndex();
		
		if(index.isValid())
		{
			for(QAction *action : m_actions_list)
				m_context_menu->removeAction(action);
			m_add_to_group->menu()->clear();
			
				//Pop up a context menu for a group or a text in a group
			if(m_model->indexIsInGroup(index))
			{
				QStandardItem *item = m_model->itemFromIndex(index);
				if(item)
				{
					if(m_model->textFromItem(item)) //User click on a text or a child item of a text
					{
						m_context_menu->addAction(m_remove_text_from_group);
						m_context_menu->addAction(m_remove_current_text);
					}
					else//User click on a group item
						m_context_menu->addAction(m_remove_current_group);
				}
			}
			else //Popup a context menu for a text not owned by a group
			{
				if(m_element.data()->textGroups().isEmpty())
					m_context_menu->addAction(m_new_group);
				else
				{
					m_context_menu->addAction(m_add_to_group);
					m_context_menu->addAction(m_new_group);
					m_context_menu->addAction(m_remove_current_text);
					
					for(ElementTextItemGroup *grp : m_element.data()->textGroups())
					{
						QAction *action = m_add_to_group->menu()->addAction(grp->name());
						connect(action, &QAction::triggered, m_signal_mapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
						m_signal_mapper->setMapping(action, grp->name());
					}
				}
			}
			
			m_context_menu->popup(qcme->globalPos());
			return true;
		}
	}
	return AbstractElementPropertiesEditorWidget::eventFilter(watched, event);
}

void DynamicElementTextItemEditor::setUpAction()
{
	m_context_menu = new QMenu(this);
	
		//Action add text to a group
	m_add_to_group = new QAction(tr("Ajouter au groupe"), m_context_menu);
	m_add_to_group->setMenu(new QMenu(m_context_menu));
	
	m_signal_mapper = new QSignalMapper(this);
	connect(m_signal_mapper, static_cast<void (QSignalMapper::*)(const QString &)>(&QSignalMapper::mapped), this, &DynamicElementTextItemEditor::addCurrentTextToGroup);
	
		//Action remove text from a group
	m_remove_text_from_group = new QAction(tr("Supprimer le texte de ce groupe"), m_context_menu);
	connect(m_remove_text_from_group, &QAction::triggered, [this]()
	{
		QAbstractItemModel *m = this->m_tree_view->model();
		if(m == nullptr)
			return;
		
		DynamicElementTextModel *model = static_cast<DynamicElementTextModel *>(m);
		if(model->indexIsInGroup(m_tree_view->currentIndex()))
		{
			DynamicElementTextItem *deti = m_model->textFromIndex(m_tree_view->currentIndex());
			if(deti && deti->parentGroup())
				m_element.data()->removeTextFromGroup(deti, deti->parentGroup());
		}
	});
	
		//Action create new group and the connection for open a dialog to edit the name
		//of the new group
	m_new_group = new QAction(tr("Nouveau groupe"), m_context_menu);
	connect(m_new_group, &QAction::triggered, [this]()
	{
		QAbstractItemModel *m = this->m_tree_view->model();
		if(m == nullptr)
			return;
		
		DynamicElementTextModel *model = static_cast<DynamicElementTextModel *>(m);
		if(model->indexIsInGroup(m_tree_view->currentIndex()))
			return;
		
		DynamicElementTextItem *deti = model->textFromIndex(m_tree_view->currentIndex());
		if(deti)
		{
			Element *element = deti->parentElement();
			QString name = QInputDialog::getText(this, tr("Nom du groupe"), tr("Entrer le nom du nouveau groupe"));
			
			if(name.isEmpty())
				return;
			else
				element->addTextGroup(name);
		}
	});
	
		//Action remove the selected text
	m_remove_current_text = new QAction(tr("Supprimer le texte"), m_context_menu);
	connect(m_remove_current_text, &QAction::triggered, [this]()
	{
		QAbstractItemModel *m = this->m_tree_view->model();
		if(m == nullptr)
			return;
		
		DynamicElementTextModel *model = static_cast<DynamicElementTextModel *>(m);
		if(DynamicElementTextItem *deti = model->textFromIndex(m_tree_view->currentIndex()))
		{
			if(m_element.data()->diagram() && m_element.data()->diagram()->project())
			{
				QUndoStack *us =m_element.data()->diagram()->project()->undoStack();
				DiagramContent dc;
				dc.m_element_texts << deti;
				us->push((new DeleteQGraphicsItemCommand(m_element.data()->diagram(), dc)));
			}
		}
	});
	
		//Action remove the selected group
	m_remove_current_group = new QAction(tr("Supprimer le groupe"), m_context_menu);
	connect(m_remove_current_group, &QAction::triggered, [this]()
	{
		QAbstractItemModel *m = this->m_tree_view->model();
		if(m == nullptr)
			return;
		
		DynamicElementTextModel *model = static_cast<DynamicElementTextModel *>(m);
		QModelIndex index = m_tree_view->currentIndex();
		if(model->indexIsInGroup(index) && !model->textFromIndex(index)) //Item is in group and is not a text, so item is the group
			m_element.data()->removeTextGroup(model->groupFromIndex(index));
	});
	
	m_actions_list << m_add_to_group \
				   << m_remove_text_from_group \
				   << m_new_group \
				   << m_remove_current_text \
				   << m_remove_current_group;
	
}

void DynamicElementTextItemEditor::dataEdited(DynamicElementTextItem *deti)
{
	Q_UNUSED(deti)
	if (m_live_edit)
		apply();
}

/**
 * @brief DynamicElementTextItemEditor::addCurrentTextToGroup
 * Add the current selected text to the group named @name
 * @param name
 */
void DynamicElementTextItemEditor::addCurrentTextToGroup(QString name)
{
	QModelIndex index = m_tree_view->currentIndex();
	DynamicElementTextModel *model = static_cast<DynamicElementTextModel *>(m_tree_view->model());
	
	DynamicElementTextItem *deti = model->textFromIndex(index);
	ElementTextItemGroup *group = m_element.data()->textGroup(name);
	
	if(deti && group)
	{
		if(deti->isSelected())
		{
			deti->setSelected(false);
			group->setSelected(true);
		}
		m_element.data()->addTextToGroup(deti, group);
	}
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
		setCurrentText(deti);
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
		}
		return;
    }
	ElementTextItemGroup *group = m_model->groupFromIndex(m_tree_view->currentIndex());
	if(group)
	{
		m_element.data()->removeTextGroup(group);
	}
}
