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
#include "masterpropertieswidget.h"
#include "ui_masterpropertieswidget.h"
#include <QListWidgetItem>
#include <diagramposition.h>
#include <elementprovider.h>
#include <diagram.h>
#include <element.h>
#include <linkelementcommand.h>

/**
 * @brief MasterPropertiesWidget::MasterPropertiesWidget
 * Default constructor
 * @param elmt
 * @param parent
 */
MasterPropertiesWidget::MasterPropertiesWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent),
	ui(new Ui::MasterPropertiesWidget),
	m_showed_element (nullptr),
	m_project(nullptr)
{
	ui->setupUi(this);
	
	ui->m_free_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->m_link_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	
	m_context_menu  = new QMenu(this);
	m_link_action   = new QAction(tr("Lier l'élément"), this);
	m_unlink_action = new QAction(tr("Délier l'élément"), this);
	m_show_qtwi     = new QAction(tr("Montrer l'élément"), this);
	m_show_element  = new QAction(tr("Montrer l'élément maître"), this);
	m_save_header_state = new QAction(tr("Enregistrer la disposition"), this);
	
	connect(ui->m_free_tree_widget, &QTreeWidget::itemDoubleClicked, this, &MasterPropertiesWidget::showElementFromTWI);
	connect(ui->m_link_tree_widget, &QTreeWidget::itemDoubleClicked, this, &MasterPropertiesWidget::showElementFromTWI);
	
	connect(ui->m_free_tree_widget, &QTreeWidget::customContextMenuRequested, [this](QPoint point) {this->customContextMenu(point, 1);});
	connect(ui->m_link_tree_widget, &QTreeWidget::customContextMenuRequested, [this](QPoint point) {this->customContextMenu(point, 2);});
	
	connect(m_link_action,   &QAction::triggered, this, &MasterPropertiesWidget::on_link_button_clicked);
	connect(m_unlink_action, &QAction::triggered, this, &MasterPropertiesWidget::on_unlink_button_clicked);
	connect(m_show_qtwi,     &QAction::triggered, [this]() {this->showElementFromTWI(this->m_qtwi_at_context_menu,0);});
	
	connect(m_show_element,  &QAction::triggered, [this]()
	{
		this->m_element->diagram()->showMe();
		this->m_element->setHighlighted(true);
		if(this->m_showed_element)
			m_showed_element->setHighlighted(false);
	});
	
	QHeaderView *qhv = ui->m_free_tree_widget->header();
	qhv->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(qhv, &QHeaderView::customContextMenuRequested, this, &MasterPropertiesWidget::headerCustomContextMenuRequested);
	connect(m_save_header_state, &QAction::triggered, [this, qhv]()
	{
		QByteArray qba = qhv->saveState();
		QSettings settings;
		settings.setValue("link-element-widget/master-state", qba);
	});
	
	setElement(elmt);
}

/**
 * @brief MasterPropertiesWidget::~MasterPropertiesWidget
 * Destructor
 */
MasterPropertiesWidget::~MasterPropertiesWidget()
{
	if (m_showed_element)
		m_showed_element->setHighlighted(false);
	
	m_element->setHighlighted(false);
	
	delete ui;
}

/**
 * @brief MasterPropertiesWidget::setElement
 * Set the element to be edited
 * @param element
 */
void MasterPropertiesWidget::setElement(Element *element)
{
	if (m_element == element)
		return;
	
	if (m_showed_element)
	{
		m_showed_element->setHighlighted(false);
		m_showed_element = nullptr;
	}
	if (m_element)
		m_element->setHighlighted(false);
	
	if (m_project)
		disconnect(m_project, SIGNAL(diagramRemoved(QETProject*,Diagram*)), this, SLOT(diagramWasdeletedFromProject()));

	if(Q_LIKELY(element->diagram() && element->diagram()->project()))
	{
		m_project = element->diagram()->project();
		connect(m_project, SIGNAL(diagramRemoved(QETProject*,Diagram*)), this, SLOT(diagramWasdeletedFromProject()));
	}
	else
		m_project = nullptr;

		//Keep up to date this widget when the linked elements of m_element change
	if (m_element)
		disconnect(m_element, &Element::linkedElementChanged, this, &MasterPropertiesWidget::updateUi);
	
	m_element = element;
	connect(m_element, &Element::linkedElementChanged, this, &MasterPropertiesWidget::updateUi);

	updateUi();
}

/**
 * @brief MasterPropertiesWidget::apply
 * If link betwen edited element and other change,
 * apply the change with a QUndoCommand (got with method associatedUndo)
 * pushed to the stack of element project.
 * Return true if link change, else false
 */
void MasterPropertiesWidget::apply() {
	if (QUndoCommand *undo = associatedUndo())
		m_element -> diagram() -> undoStack().push(undo);
}

/**
 * @brief MasterPropertiesWidget::reset
 * Reset curent widget, clear eveything and rebuild widget.
 */
void MasterPropertiesWidget::reset()
{
	foreach (QTreeWidgetItem *qtwi, m_qtwi_hash.keys())
		delete qtwi;
	
	m_qtwi_hash.clear();
	updateUi();
}

/**
 * @brief MasterPropertiesWidget::associatedUndo
 * If link between the edited element and other change,
 * return a QUndoCommand with this change.
 * If no change return nullptr.
 * @return
 */
QUndoCommand* MasterPropertiesWidget::associatedUndo() const
{
	QList <Element *> to_link;
	QList <Element *> linked_ = m_element->linkedElements();

	for (int i=0; i<ui->m_link_tree_widget->topLevelItemCount(); i++)
		to_link << m_qtwi_hash[ui->m_link_tree_widget->topLevelItem(i)];

		//The two list contain the same element, there is no change
	if (to_link.size() == linked_.size())
	{
		bool equal = true;

		foreach(Element *elmt, to_link)
			if (!linked_.contains(elmt))
				equal = false;

		if(equal)
			return nullptr;
	}

	LinkElementCommand *undo = new LinkElementCommand(m_element);

	if (to_link.isEmpty())
		undo->unlinkAll();
	else
		undo->setLink(to_link);

	return undo;
}

/**
 * @brief MasterPropertiesWidget::setLiveEdit
 * @param live_edit = true : live edit is enable
 * else false : live edit is disable.
 * @return always true because live edit is handled by this editor widget
 */
bool MasterPropertiesWidget::setLiveEdit(bool live_edit)
{
	m_live_edit = live_edit;
	return true;
}

/**
 * @brief MasterPropertiesWidget::updateUi
 * Build the interface of the widget
 */
void MasterPropertiesWidget::updateUi()
{
	ui->m_free_tree_widget->clear();
	ui->m_link_tree_widget->clear();
	m_qtwi_hash.clear();

	if (Q_UNLIKELY(!m_project))
		return;

	ElementProvider elmt_prov(m_project);

		//Build the list of free available element
	QList <QTreeWidgetItem *> items_list;
	foreach(Element *elmt, elmt_prov.freeElement(Element::Slave))
	{
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_free_tree_widget);
		qtwi->setIcon(0, elmt->pixmap());
		qtwi->setText(1, QString::number(elmt->diagram()->folioIndex() + 1));
		
		autonum::sequentialNumbers seq;
		QString F =autonum::AssignVariables::formulaToLabel(elmt->diagram()->border_and_titleblock.folio(), seq, elmt->diagram(), elmt);
		qtwi->setText(2, F);
		qtwi->setText(3, elmt->diagram()->title());
		qtwi->setText(4, elmt->diagram()->convertPosition(elmt->scenePos()).toString());
		items_list.append(qtwi);
		m_qtwi_hash.insert(qtwi, elmt);
	}
	
	ui->m_free_tree_widget->addTopLevelItems(items_list);
	items_list.clear();

		//Build the list of already linked element
	foreach(Element *elmt, m_element->linkedElements())
	{
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_link_tree_widget);
		qtwi->setIcon(0, elmt->pixmap());
		qtwi->setText(1, QString::number(elmt->diagram()->folioIndex() + 1));
		
		autonum::sequentialNumbers seq;
		QString F =autonum::AssignVariables::formulaToLabel(elmt->diagram()->border_and_titleblock.folio(), seq, elmt->diagram(), elmt);
		qtwi->setText(2, F);
		qtwi->setText(3, elmt->diagram()->title());
		qtwi->setText(4, elmt->diagram()->convertPosition(elmt->scenePos()).toString());
		items_list.append(qtwi);
		m_qtwi_hash.insert(qtwi, elmt);
	}
	if(items_list.count())
		ui->m_link_tree_widget->addTopLevelItems(items_list);
	
	QSettings settings;
	QVariant v = settings.value("link-element-widget/master-state");
	if(!v.isNull())
	{
		ui->m_free_tree_widget->header()->restoreState(v.toByteArray());
		ui->m_link_tree_widget->header()->restoreState(v.toByteArray());
	}
}

void MasterPropertiesWidget::headerCustomContextMenuRequested(const QPoint &pos)
{
	m_context_menu->clear();
	m_context_menu->addAction(m_save_header_state);
	m_context_menu->popup(ui->m_free_tree_widget->header()->mapToGlobal(pos));
}

/**
 * @brief MasterPropertiesWidget::on_link_button_clicked
 * move curent item in the free_list to linked_list
 */
void MasterPropertiesWidget::on_link_button_clicked()
{
		//take the curent item from free_list and push it to linked_list
	QTreeWidgetItem *qtwi = ui->m_free_tree_widget->currentItem();
	if (qtwi)
	{
		ui->m_free_tree_widget->takeTopLevelItem(ui->m_free_tree_widget->indexOfTopLevelItem(qtwi));
		ui->m_link_tree_widget->insertTopLevelItem(0, qtwi);
		
		if(m_live_edit)
			apply();
	}
}

/**
 * @brief MasterPropertiesWidget::on_unlink_button_clicked
 * move curent item in linked_list to free_list
 */
void MasterPropertiesWidget::on_unlink_button_clicked()
{
		//take the curent item from linked_list and push it to free_list
	QTreeWidgetItem *qtwi = ui->m_link_tree_widget->currentItem();
	if(qtwi)
	{
		ui->m_link_tree_widget->takeTopLevelItem(ui->m_link_tree_widget->indexOfTopLevelItem(qtwi));
		ui->m_free_tree_widget->insertTopLevelItem(0, qtwi);

		if(m_live_edit)
			apply();
	}
}

/**
 * @brief MasterPropertiesWidget::showElementFromTWI
 * Show the element corresponding to the given QTreeWidgetItem
 * @param qtwi
 * @param column
 */
void MasterPropertiesWidget::showElementFromTWI(QTreeWidgetItem *qtwi, int column)
{
	Q_UNUSED(column);
	if (m_showed_element)
	{
		disconnect(m_showed_element, SIGNAL(destroyed()), this, SLOT(showedElementWasDeleted()));
		m_showed_element -> setHighlighted(false);
	}
	if (m_element)
		m_element->setHighlighted(false);

	m_showed_element = m_qtwi_hash[qtwi];
	m_showed_element->diagram()->showMe();
	m_showed_element->setHighlighted(true);
	connect(m_showed_element, SIGNAL(destroyed()), this, SLOT(showedElementWasDeleted()));
}

/**
 * @brief MasterPropertiesWidget::showedElementWasDeleted
 * Set to nullptr the current showed element when he was deleted
 */
void MasterPropertiesWidget::showedElementWasDeleted() {
	m_showed_element = nullptr;
}

/**
 * @brief MasterPropertiesWidget::diagramWasdeletedFromProject
 * This slot is called when a diagram is removed from the parent project of edited element
 * to update the content of this widget
 */
void MasterPropertiesWidget::diagramWasdeletedFromProject()
{
		//We use a timer because if the removed diagram contain slave element linked to the edited element
		//we must to wait for this elements be unlinked, else the linked list provide deleted elements.
	QTimer::singleShot(10, this, SLOT(updateUi()));
}

/**
 * @brief MasterPropertiesWidget::customContextMenu
 * Display a context menu
 * @param pos
 * @param i : the tree widget where the context menu was requested.
 */
void MasterPropertiesWidget::customContextMenu(const QPoint &pos, int i)
{
		//add the size of the header to display the topleft of the QMenu at the position of the mouse.
		//See doc about QWidget::customContextMenuRequested section related to QAbstractScrollArea 
	QPoint point = pos;
	point.ry()+=ui->m_free_tree_widget->header()->height();
	
	m_context_menu->clear();
	
	if (i == 1)
	{
		point = ui->m_free_tree_widget->mapToGlobal(point);
		
			//Context at for free tree widget
		if (ui->m_free_tree_widget->currentItem())
		{
			m_qtwi_at_context_menu = ui->m_free_tree_widget->currentItem();
			m_context_menu->addAction(m_link_action);
			m_context_menu->addAction(m_show_qtwi);
		}
	}
	else
	{
		point = ui->m_link_tree_widget->mapToGlobal(point);
		
			//context at for link tre widget
		if (ui->m_link_tree_widget->currentItem())
		{
			m_qtwi_at_context_menu = ui->m_link_tree_widget->currentItem();
			m_context_menu->addAction(m_unlink_action);
			m_context_menu->addAction(m_show_qtwi);
		}
	}
	
	m_context_menu->addAction(m_show_element);
	m_context_menu->popup(point);
}
