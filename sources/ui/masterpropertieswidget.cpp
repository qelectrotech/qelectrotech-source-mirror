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
	connect(ui->free_list,		SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(showElementFromLWI(QListWidgetItem*)));
	connect(ui->linked_list,	SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(showElementFromLWI(QListWidgetItem*)));
	setElement(elmt);
}

/**
 * @brief MasterPropertiesWidget::~MasterPropertiesWidget
 * Destructor
 */
MasterPropertiesWidget::~MasterPropertiesWidget()
{
	if (m_showed_element) m_showed_element->setHighlighted(false);
	delete ui;
}

/**
 * @brief MasterPropertiesWidget::setElement
 * Set the element to be edited
 * @param element
 */
void MasterPropertiesWidget::setElement(Element *element)
{
	if (m_element == element) return;
	if (m_showed_element) {m_showed_element->setHighlighted(false); m_showed_element = nullptr;}
	if (m_project) disconnect(m_project, SIGNAL(diagramRemoved(QETProject*,Diagram*)), this, SLOT(diagramWasdeletedFromProject()));

	if(Q_LIKELY(element->diagram() && element->diagram()->project()))
	{
		m_project = element->diagram()->project();
		connect(m_project, SIGNAL(diagramRemoved(QETProject*,Diagram*)), this, SLOT(diagramWasdeletedFromProject()));
	}
	else m_project = nullptr;

		//Keep up to date this widget when the linked elements of m_element change
	if (m_element) disconnect(m_element, &Element::linkedElementChanged, this, &MasterPropertiesWidget::updateUi);
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
void MasterPropertiesWidget::reset() {
	foreach (QListWidgetItem *lwi, lwi_hash.keys()) {
		delete lwi;
	}
	lwi_hash.clear();
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

	for (int i=0; i<ui->linked_list->count(); i++)
		to_link << lwi_hash[ui->linked_list->item(i)];

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
	ui->free_list->clear();
	ui->linked_list->clear();
	lwi_hash.clear();

	if (Q_UNLIKELY(!m_project)) return;

	ElementProvider elmt_prov(m_project);

		//Build the list of free available element
	foreach(Element *elmt, elmt_prov.freeElement(Element::Slave))
	{
		//label for list widget
		QString widget_text;
		QString title = elmt->diagram()->title();
		if (title.isEmpty()) title = tr("Sans titre");
		widget_text += QString(tr("Folio  %1 (%2), position %3.")).arg(elmt->diagram()->folioIndex() + 1)
																	  .arg(title)
																	  .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString());
		QListWidgetItem *lwi_ = new QListWidgetItem(elmt->pixmap(), widget_text);
		lwi_hash.insert(lwi_, elmt);
		ui->free_list->addItem(lwi_);
	}

		//Build the list of already linked element
	foreach(Element *elmt, m_element->linkedElements())
	{
		//label for list widget
		QString widget_text;
		QString title = elmt->diagram()->title();
		if (title.isEmpty()) title = tr("Sans titre");
		widget_text += QString(tr("Folio  %1 (%2), position %3.")).arg(elmt->diagram()->folioIndex() + 1)
																	  .arg(title)
																	  .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString());
		QListWidgetItem *lwi_ = new QListWidgetItem(elmt->pixmap(), widget_text);
		lwi_hash.insert(lwi_, elmt);
		ui->linked_list->addItem(lwi_);
	}
}

/**
 * @brief MasterPropertiesWidget::on_link_button_clicked
 * move curent item in the free_list to linked_list
 */
void MasterPropertiesWidget::on_link_button_clicked()
{
		//take the curent item from free_list and push it to linked_list
	ui->linked_list->addItem(
				ui->free_list->takeItem(
					ui->free_list->currentRow()));

	if(m_live_edit) apply();
}

/**
 * @brief MasterPropertiesWidget::on_unlink_button_clicked
 * move curent item in linked_list to free_list
 */
void MasterPropertiesWidget::on_unlink_button_clicked()
{
		//take the curent item from linked_list and push it to free_list
	ui->free_list->addItem(
				ui->linked_list->takeItem(
					ui->linked_list->currentRow()));

	if(m_live_edit) apply();
}

/**
 * @brief MasterPropertiesWidget::showElementFromLWI
 * Show the element corresponding to the given QListWidgetItem
 * @param lwi
 */
void MasterPropertiesWidget::showElementFromLWI(QListWidgetItem *lwi)
{
	if (m_showed_element)
	{
		disconnect(m_showed_element, SIGNAL(destroyed()), this, SLOT(showedElementWasDeleted()));
		m_showed_element -> setHighlighted(false);
	}

	m_showed_element = lwi_hash[lwi];
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
