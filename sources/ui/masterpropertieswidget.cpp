/*
	Copyright 2006-2015 The QElectroTech Team
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
#include <diagramcommands.h>
#include <diagram.h>
#include <element.h>

/**
 * @brief MasterPropertiesWidget::MasterPropertiesWidget
 * Default constructor
 * @param elmt
 * @param parent
 */
MasterPropertiesWidget::MasterPropertiesWidget(Element *elmt, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::MasterPropertiesWidget),
	element_(elmt)
{
	ui->setupUi(this);
	buildInterface();
	connect(ui->free_list,		SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(showElementFromLWI(QListWidgetItem*)));
	connect(ui->linked_list,	SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(showElementFromLWI(QListWidgetItem*)));
}

/**
 * @brief MasterPropertiesWidget::~MasterPropertiesWidget
 * Destructor
 */
MasterPropertiesWidget::~MasterPropertiesWidget()
{
	foreach(Element *elmt, lwi_hash.values()) elmt->setHighlighted(false);
	delete ui;
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
		element_ -> diagram() -> undoStack().push(undo);
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
	buildInterface();
}

/**
 * @brief MasterPropertiesWidget::associatedUndo
 * If link between the edited element and other change,
 * return a QUndoCommand with this change.
 * If no change return nullptr.
 * @return
 */
QUndoCommand* MasterPropertiesWidget::associatedUndo() const {
	QList <Element *> to_link;
	QList <Element *> linked_ = element_->linkedElements();

	for (int i=0; i<ui->linked_list->count(); i++) {
		to_link << lwi_hash[ui->linked_list->item(i)];
	}

		//If same element are find in to_link and linked, that means
		// element are already linked, so we remove element on the two list
		//if linked_ contains element at the end of the operation,
		//that means this element must be unlinked from @element_
	foreach (Element *elmt, to_link) {
		if(linked_.contains(elmt)) {
			to_link.removeAll(elmt);
			linked_.removeAll(elmt);
		}
	}

		// if two list, contain element, we link and unlink @element_ with corresponding
		//undo command, and add first command for parent of the second, user see only one
		//undo command
	if (linked_.count() && to_link.count()) {
		LinkElementsCommand *lec = new LinkElementsCommand(element_, to_link);
		new unlinkElementsCommand(element_, linked_, lec);
		return lec;
	}
		//Else do the single undo command corresponding to the link.
	else if (to_link.count()) {
		return (new LinkElementsCommand(element_, to_link));
	}
	else if (linked_.count()) {
		return (new unlinkElementsCommand(element_, linked_));
	}
	else {
		return nullptr;
	}
}

/**
 * @brief MasterPropertiesWidget::buildInterface
 * Build the interface of the widget
 */
void MasterPropertiesWidget::buildInterface() {
	//build the free list
	ElementProvider elmt_prov(element_->diagram()->project());

	foreach(Element *elmt, elmt_prov.freeElement(Element::Slave)) {
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

	//build the linked list
	foreach(Element *elmt, element_->linkedElements()) {
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
void MasterPropertiesWidget::on_link_button_clicked() {
	//take the curent item from free_list and push it to linked_list
	ui->linked_list->addItem(
				ui->free_list->takeItem(
					ui->free_list->currentRow()));
}

/**
 * @brief MasterPropertiesWidget::on_unlink_button_clicked
 * move curent item in linked_list to free_list
 */
void MasterPropertiesWidget::on_unlink_button_clicked() {
	//take the curent item from linked_list and push it to free_list
	ui->free_list->addItem(
				ui->linked_list->takeItem(
					ui->linked_list->currentRow()));
}

/**
 * @brief MasterPropertiesWidget::showElementFromLWI
 * Show the element corresponding to the given QListWidgetItem
 * @param lwi
 */
void MasterPropertiesWidget::showElementFromLWI(QListWidgetItem *lwi) {
	foreach(Element *elmt, lwi_hash.values()) elmt->setHighlighted(false);
	Element *elmt = lwi_hash[lwi];
	elmt->diagram()->showMe();
	elmt->setHighlighted(true);
}
