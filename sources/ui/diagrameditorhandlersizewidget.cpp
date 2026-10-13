/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "diagrameditorhandlersizewidget.h"
#include "ui_diagrameditorhandlersizewidget.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../projectview.h"
#include "../diagramview.h"
#include "../diagram.h"
#include "../qetproject.h"
#include <QSignalBlocker>
#include "../QetGraphicsItemModeler/qetgraphicshandleritem.h"

DiagramEditorHandlerSizeWidget::DiagramEditorHandlerSizeWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DiagramEditorHandlerSizeWidget)
{
	ui->setupUi(this);
	const QSignalBlocker blocker(ui->comboBox);
	const qreal sizes[] = {2.5, 5.0, 7.5, 10.0, 20.0, 30.0};
	for (int i = 0; i < ui->comboBox->count(); ++i)
		ui->comboBox->setItemData(i, sizes[i]);
	ui->comboBox->setCurrentIndex(3); // x1 remains the default.

	if (auto editor = QETApp::instance()->diagramEditorAncestorOf(this))
	{
		const auto size = editor->property("graphics_handler_size").toReal();
		const int index = ui->comboBox->findData(size);
		if (index >= 0)
			ui->comboBox->setCurrentIndex(index);
	}
}

DiagramEditorHandlerSizeWidget::~DiagramEditorHandlerSizeWidget()
{
	delete ui;
}

void DiagramEditorHandlerSizeWidget::on_comboBox_currentIndexChanged(int index)
{
	if (index < 0 || !ui->comboBox->itemData(index).isValid())
		return;
	const qreal size = ui->comboBox->itemData(index).toReal();
	if (auto editor_ = QETApp::instance()->diagramEditorAncestorOf(this))
	{
		editor_->setProperty("graphics_handler_size", size);
		for (auto project_view : editor_->openedProjects()) {
				//Handles exist on built folios only
			for (auto diagram : project_view->project()->builtFolios()) {
				for (const auto item : diagram->items()) {
					if (item->type() == QetGraphicsHandlerItem::Type) {
						auto handler = qgraphicsitem_cast<QetGraphicsHandlerItem *>(item);
						handler->setSize(size);
					}
				}
			}
		}
	}
}

