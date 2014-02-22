/*
	Copyright 2006-2014 The QElectroTech Team
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

MasterPropertiesWidget::MasterPropertiesWidget(Element *elmt, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::MasterPropertiesWidget),
	element_(elmt)
{
	ui->setupUi(this);
	buildInterface();
}

MasterPropertiesWidget::~MasterPropertiesWidget()
{
	delete ui;
}

void MasterPropertiesWidget::buildInterface() {
	ElementProvider elmt_prov(element_->diagram()->project());

	foreach(Element *elmt, elmt_prov.freeElement(Element::Slave)) {
		//label for list widget
		QString widget_text;
		QString title = elmt->diagram()->title();
		if (title.isEmpty()) title = tr("Sans titre");
		widget_text += QString(tr("Folio\240 %1 (%2), position %3.")).arg(elmt->diagram()->folioIndex() + 1)
																	  .arg(title)
																	  .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString());
		QListWidgetItem *lwi_ = new QListWidgetItem(elmt->pixmap(), widget_text);
		ui->free_list->addItem(lwi_);
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

void MasterPropertiesWidget::on_unlink_button_clicked() {
	//take the curent item from linked_list and push it to free_list
	ui->free_list->addItem(
				ui->linked_list->takeItem(
					ui->linked_list->currentRow()));
}
