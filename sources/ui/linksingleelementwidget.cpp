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
#include "linksingleelementwidget.h"
#include "ui_linksingleelementwidget.h"
#include "diagram.h"
#include "elementprovider.h"
#include "linkelementcommand.h"
#include "diagramposition.h"
#include "conductor.h"

#include <QTreeWidgetItem>

/**
 * @brief LinkSingleElementWidget::LinkSingleElementWidget
 * Default constructor
 * @param elmt
 * the edited element
 * @param parent
 * the parent widget
 */
LinkSingleElementWidget::LinkSingleElementWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent),
	ui(new Ui::LinkSingleElementWidget)
{
	ui->setupUi(this);
	
	ui->m_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	m_context_menu  = new QMenu(this);
	m_link_action   = new QAction(tr("Lier l'élément"), this);
	m_show_qtwi     = new QAction(tr("Montrer l'élément"), this);
	m_show_element  = new QAction(tr("Montrer l'élément esclave"), this);
	m_save_header_state = new QAction(tr("Enregistrer la disposition"), this);
	
	connect(m_show_qtwi, &QAction::triggered, [this]() {this->on_m_tree_widget_itemDoubleClicked(this->m_qtwi_at_context_menu, 0);});
	connect(m_link_action, &QAction::triggered, this, &LinkSingleElementWidget::linkTriggered);
	
	connect(m_show_element,  &QAction::triggered, [this]()
	{
		this->m_element->diagram()->showMe();
		this->m_element->setHighlighted(true);
		if(this->m_showed_element)
			m_showed_element->setHighlighted(false);
	});
	
	QHeaderView *qhv = ui->m_tree_widget->header();
	qhv->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(qhv, &QHeaderView::customContextMenuRequested, this, &LinkSingleElementWidget::headerCustomContextMenuRequested);
	connect(m_save_header_state, &QAction::triggered, [this, qhv]()
	{
		QByteArray qba = qhv->saveState();
		QSettings settings;
		
		if (this->m_element->linkType() & Element::AllReport)
			settings.setValue("link-element-widget/report-state", qba);
		else if (this->m_element->linkType() == Element::Slave)
			settings.setValue("link-element-widget/slave-state", qba);
	});
			
	setElement(elmt);
}

/**
 * @brief LinkSingleElementWidget::~LinkSingleElementWidget
 * Default destructor
 */
LinkSingleElementWidget::~LinkSingleElementWidget()
{
	if(m_showed_element)
		m_showed_element->setHighlighted(false);
	
	m_element->setHighlighted(false);
	
	if (!m_element->isFree())
		m_element->linkedElements().first()->setHighlighted(false);
	delete ui;
}

/**
 * @brief LinkSingleElementWidget::setElement
 * Set element to be the edited element.
 * @param element
 */
void LinkSingleElementWidget::setElement(Element *element)
{
	if (m_element == element)
		return;

		//Remove connection of previous edited element
	if (m_element)
	{
		disconnect(m_element->diagram()->project(), &QETProject::diagramRemoved, this, &LinkSingleElementWidget::diagramWasRemovedFromProject);
		disconnect(m_element, &Element::linkedElementChanged, this, &LinkSingleElementWidget::updateUi);
		m_element->setHighlighted(false);
	}
	
	if(m_showed_element)
		m_showed_element->setHighlighted(false);
	
	m_unlink = false;
	m_showed_element = nullptr;
	m_element_to_link = nullptr;
	m_pending_qtwi = nullptr;

		//Setup the new element, connection and ui
	m_element = element;

	if (m_element->linkType() & Element::Slave)
		m_filter = Element::Master;
	else if (m_element->linkType() & Element::AllReport)
		m_filter = m_element->linkType() == Element::NextReport? Element::PreviousReport : Element::NextReport;
	else
		m_filter = Element::Simple;

	connect(m_element->diagram()->project(), &QETProject::diagramRemoved, this, &LinkSingleElementWidget::diagramWasRemovedFromProject);
	connect(m_element, &Element::linkedElementChanged, this, &LinkSingleElementWidget::updateUi, Qt::QueuedConnection);

	updateUi();
}

/**
 * @brief LinkSingleElementWidget::apply
 * Apply the new property of the edited element by pushing
 * the associated undo command to parent project undo stack
 */
void LinkSingleElementWidget::apply()
{
	QUndoCommand *undo = associatedUndo();
	if (undo)
		m_element->diagram()->undoStack().push(undo);
	
	m_unlink = false;
	m_element_to_link = nullptr;
	m_pending_qtwi = nullptr;
}

/**
 * @brief LinkSingleElementWidget::associatedUndo
 * @return the undo command associated to the current edition
 * if there isn't change, return nulptr
 */
QUndoCommand *LinkSingleElementWidget::associatedUndo() const
{
	LinkElementCommand *undo = new LinkElementCommand(m_element);

	if (m_element_to_link || m_unlink)
	{
		if (m_element_to_link)
			undo->setLink(m_element_to_link);
		else if (m_unlink)
			undo->unlinkAll();

		return undo;
	}

	return nullptr;
}

/**
 * @brief LinkSingleElementWidget::title
 * @return the title used for this editor
 */
QString LinkSingleElementWidget::title() const
{
	if (m_element->linkType() & Element::AllReport)
		return tr("Report de folio");
	else
		return tr("Référence croisée (esclave)");
}

/**
 * @brief LinkSingleElementWidget::updateUi
 * Update the content of this widget
 */
void LinkSingleElementWidget::updateUi()
{
	m_unlink = false;

		//Update the behavior of link/unlink button
	if (m_element->isFree())
		hideButtons();
	else
		showButtons();
	
	buildTree();
}

/**
 * @brief LinkSingleElementWidget::buildTree
 * Build the content of the QTreeWidget
 */
void LinkSingleElementWidget::buildTree()
{
	clearTreeWidget();
	setUpHeaderLabels();
	ui->m_tree_widget->header()->resetDefaultSectionSize();
	
	if (m_element->linkType() == Element::Slave)
	{
		foreach(Element *elmt, availableElements())
		{
			QStringList search_list;
			QStringList str_list;
			str_list << elmt->elementInformations()["label"].toString();
			if (!str_list.last().isEmpty())
				search_list << str_list.last();
			
			str_list << elmt->elementInformations()["comment"].toString();
			if (!str_list.last().isEmpty())
				search_list << str_list.last();
			
			if (Diagram *diag = elmt->diagram())
			{
				str_list << QString::number(diag->folioIndex() + 1);
				autonum::sequentialNumbers seq;
				QString F =autonum::AssignVariables::formulaToLabel(diag->border_and_titleblock.folio(), seq, diag, elmt);
				str_list << F;
				str_list << diag->convertPosition(elmt->scenePos()).toString();
				str_list << diag->title();
			}
			else
			{
				qDebug() << "In method void LinkSingleElementWidget::updateUi(), provied element must have be in a diagram";
			}
			
			QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_tree_widget, str_list);
			m_qtwi_elmt_hash.insert(qtwi, elmt);
			m_qtwi_strl_hash.insert(qtwi, search_list);
		}
		
		QSettings settings;
		QVariant v = settings.value("link-element-widget/slave-state");
		if(!v.isNull())
			ui->m_tree_widget->header()->restoreState(v.toByteArray());
	}
	
	else if (m_element->linkType() & Element::AllReport)
	{	
		foreach(Element *elmt, availableElements())
		{
			QStringList search_list;
			QStringList str_list;
			
			if (elmt->conductors().size())
			{
				ConductorProperties cp = elmt->conductors().first()->properties();
				str_list << cp.text;
				if (!str_list.last().isEmpty())
					search_list << str_list.last();
				str_list << cp.m_function;
				if (!str_list.last().isEmpty())
					search_list << str_list.last();
				str_list << cp.m_tension_protocol;
				if (!str_list.last().isEmpty())
					search_list << str_list.last();
			}
			else
				str_list << "" << "" << "";
			
			if (Diagram *diag = elmt->diagram())
			{
				str_list << QString::number(diag->folioIndex() + 1);
				autonum::sequentialNumbers seq;
				QString F =autonum::AssignVariables::formulaToLabel(diag->border_and_titleblock.folio(), seq, diag, elmt);
				str_list << F;
				str_list << diag->convertPosition(elmt->scenePos()).toString();
				str_list << diag->title();
			}
			else
			{
				qDebug() << "In method void LinkSingleElementWidget::updateUi(), provied element must have be in a diagram";
			}
			
			QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_tree_widget, str_list);
			m_qtwi_elmt_hash.insert(qtwi, elmt);
			m_qtwi_strl_hash.insert(qtwi, search_list);
		}
		
		QSettings settings;
		QVariant v = settings.value("link-element-widget/report-state");
		if(!v.isNull())
			ui->m_tree_widget->header()->restoreState(v.toByteArray());
	}

	setUpCompleter();
}

/**
 * @brief LinkSingleElementWidget::setLiveEdit
 * @param live_edit
 * @return
 */
bool LinkSingleElementWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit)
		return true;
	
	m_live_edit = live_edit;
	
	return true;
}

/**
 * @brief LinkSingleElementWidget::availableElements
 * @return A QList with all available element
 * to be linked with the edited element.
 * This methode take care of the combo box "find in diagram"
 */
QList <Element *> LinkSingleElementWidget::availableElements()
{
	QList <Element *> elmt_list;
		//if element isn't free and unlink isn't pressed, return an empty list
	if (!m_element->isFree() && !m_unlink)
		return elmt_list;

	if (!m_element->diagram() || !m_element->diagram()->project()) return elmt_list;
	
	ElementProvider ep(m_element->diagram()->project());
	if (m_filter & Element::AllReport)
		elmt_list = ep.freeElement(m_filter);
	else
		elmt_list = ep.find(m_filter);
	
		//If element is linked, remove is parent from the list
	if(!m_element->isFree()) elmt_list.removeAll(m_element->linkedElements().first());

	return elmt_list;
}

/**
 * @brief LinkSingleElementWidget::setUpCompleter
 * Setup the completer of search_field
 */
void LinkSingleElementWidget::setUpCompleter()
{
	ui->m_search_field->clear();
	if(ui->m_search_field->completer())
		delete ui->m_search_field->completer();
	
	QStringList search;
	foreach(QStringList strl , m_qtwi_strl_hash.values())
		search.append(strl);
	
	QCompleter *c = new QCompleter(search, ui->m_search_field);
	c->setCaseSensitivity(Qt::CaseInsensitive);
	ui->m_search_field->setCompleter(c);
}

/**
 * @brief LinkSingleElementWidget::clearTreeWidget
 * Clear the tree widget.
 * Delete all QTreeWidget (in the tree widget and in the hash).
 * Clear the hash.
 */
void LinkSingleElementWidget::clearTreeWidget()
{
	while(ui->m_tree_widget->topLevelItemCount())
	{
		QTreeWidgetItem *qtwi = ui->m_tree_widget->takeTopLevelItem(0);
		if (!m_qtwi_elmt_hash.contains(qtwi))
			delete qtwi;
	}
	
	foreach(QTreeWidgetItem *qtwi, m_qtwi_elmt_hash.keys())
		delete qtwi;
	
	m_qtwi_elmt_hash.clear();
	m_qtwi_strl_hash.clear();
}

void LinkSingleElementWidget::setUpHeaderLabels()
{
	QStringList list;
	if (m_element->linkType() == Element::Slave)
		list << tr("Label") << tr("Commentaire") << tr("N° de folio") << tr("Label de folio") << tr("Position") << tr("Titre de folio");
	else if (m_element->linkType() & Element::AllReport)
		list << tr("N° de fil") << tr("Fonction") << tr("Tension / Protocole") << tr("N° de folio") << tr("Label de folio") << tr("Position") << tr("Titre de folio");
	
	ui->m_tree_widget->setHeaderLabels(list);
}

/**
 * @brief LinkSingleElementWidget::diagramWasRemovedFromProject
 *  * This slot is called when a diagram is removed from the parent project of edited element
 * to update the content of this widget
 */
void LinkSingleElementWidget::diagramWasRemovedFromProject()
{
		//We use a timer because if the removed diagram contain the master element linked to the edited element
		//we must to wait for this elements be unlinked, else the list of available master isn't up to date
	QTimer::singleShot(10, this, SLOT(updateUi()));
}

void LinkSingleElementWidget::showedElementWasDeleted()
{
	m_showed_element = nullptr;
}

/**
 * @brief LinkSingleElementWidget::linkTriggered
 * Action linkis triggered
 */
void LinkSingleElementWidget::linkTriggered()
{
	if(!m_qtwi_at_context_menu)
		return;
	
	m_element_to_link = m_qtwi_elmt_hash.value(m_qtwi_at_context_menu);
	
	if(m_live_edit)
	{
		apply();
		updateUi();
	}
	else
	{
			//In no live edit mode, we set the background of the qtwi green, to inform the user
			//which element will be linked when he press the apply button
		if (m_pending_qtwi)
		{
			QBrush brush(Qt::white, Qt::NoBrush);
			for(int i=0 ; i<6 ; i++)
			{
				m_pending_qtwi->setBackground(i,brush);
			}
		}
		
		for (int i=0 ; i<6 ; i++)
		{
			m_qtwi_at_context_menu->setBackgroundColor(i, Qt::green);
		}
		m_pending_qtwi = m_qtwi_at_context_menu;
	}
	
}

/**
 * @brief LinkSingleElementWidget::hideButtons
 * Hide the button displayed when element is already linked
 */
void LinkSingleElementWidget::hideButtons()
{
	ui->m_label->hide();
	ui->m_unlink_pb->hide();
	ui->m_show_linked_pb->hide();
	ui->m_show_this_pb->hide();
	ui->m_search_field->show();
}

/**
 * @brief LinkSingleElementWidget::showButtons
 * Show the button displayed when element is already linked
 */
void LinkSingleElementWidget::showButtons()
{
	ui->m_label->show();
	ui->m_unlink_pb->show();
	ui->m_show_linked_pb->show();
	ui->m_show_this_pb->show();
	ui->m_search_field->hide();
}

void LinkSingleElementWidget::headerCustomContextMenuRequested(const QPoint &pos)
{
	m_context_menu->clear();
	m_context_menu->addAction(m_save_header_state);
	m_context_menu->popup(ui->m_tree_widget->header()->mapToGlobal(pos));
}

void LinkSingleElementWidget::on_m_unlink_pb_clicked()
{
	m_unlink = true;
	
	if(m_live_edit)
	{
		apply();
		updateUi();
	}
	else
		buildTree();
}

/**
 * @brief LinkSingleElementWidget::on_m_tree_widget_itemDoubleClicked
 * Highlight the element represented by @item
 * @param item
 * @param column
 */
void LinkSingleElementWidget::on_m_tree_widget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);
	
	if (m_showed_element)
	{
		disconnect(m_showed_element, SIGNAL(destroyed()), this, SLOT(showedElementWasDeleted()));
		m_showed_element->setHighlighted(false);
	}
	
	Element *elmt = m_qtwi_elmt_hash.value(item);
	elmt->diagram()->showMe();
	elmt->setHighlighted(true);
	m_showed_element = elmt;
	connect(m_showed_element, SIGNAL(destroyed()), this, SLOT(showedElementWasDeleted()));

}

void LinkSingleElementWidget::on_m_tree_widget_customContextMenuRequested(const QPoint &pos)
{
		//add the size of the header to display the topleft of the QMenu at the position of the mouse.
		//See doc about QWidget::customContextMenuRequested section related to QAbstractScrollArea 
	QPoint point = pos;
	point.ry()+=ui->m_tree_widget->header()->height();
	point = ui->m_tree_widget->mapToGlobal(point);
	
	m_context_menu->clear();
	
	if (ui->m_tree_widget->currentItem())
	{
		m_qtwi_at_context_menu = ui->m_tree_widget->currentItem();
		m_context_menu->addAction(m_link_action);
		m_context_menu->addAction(m_show_qtwi);
	}
	
	m_context_menu->addAction(m_show_element);
	m_context_menu->popup(point);
}

void LinkSingleElementWidget::on_m_show_linked_pb_clicked()
{
	if (!m_element->isFree())
	{
		Element *elmt = m_element->linkedElements().first();
		elmt->diagram()->showMe();
		elmt->setHighlighted(true);
	}
}

void LinkSingleElementWidget::on_m_show_this_pb_clicked()
{
	m_show_element->trigger();
}

/**
 * @brief LinkSingleElementWidget::on_m_search_field_textEdited
 * Search all items which match with @arg1 and shows it, other items is hidden.
 * If @arg1 is empty, show all items.
 * @param arg1
 */
void LinkSingleElementWidget::on_m_search_field_textEdited(const QString &arg1)
{
		//Show all items if arg1 is empty, if not hide all items
	foreach(QTreeWidgetItem *qtwi, m_qtwi_elmt_hash.keys())
		qtwi->setHidden(!arg1.isEmpty());
	
	QList <QTreeWidgetItem *> qtwi_list;
	
	foreach(QTreeWidgetItem *qtwi, m_qtwi_strl_hash.keys())
	{
		foreach(QString str, m_qtwi_strl_hash.value(qtwi))
		{
			if(str.contains(arg1, Qt::CaseInsensitive))
			{
				qtwi_list << qtwi;
				continue;
			}
		}
	}
	
		//Show items which match with arg1
	foreach(QTreeWidgetItem *qtwi, qtwi_list)
		qtwi->setHidden(false);
}
