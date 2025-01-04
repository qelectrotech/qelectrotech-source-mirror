/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "../../QetGraphicsItemModeler/qetgraphicshandleritem.h"

DiagramEditorHandlerSizeWidget::DiagramEditorHandlerSizeWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DiagramEditorHandlerSizeWidget)
{
	ui->setupUi(this);

	if (auto editor = QETApp::instance()->diagramEditorAncestorOf(this))
	{
		const auto size =  editor->property("graphics_handler_size").toInt();

		if (size == 10 || size < 10)
			ui->comboBox->setCurrentIndex(0);
		else if (size == 20)
			ui->comboBox->setCurrentIndex(1);
		else if (size == 30 || size > 30)
			ui->comboBox->setCurrentIndex(2);
	}
}

DiagramEditorHandlerSizeWidget::~DiagramEditorHandlerSizeWidget()
{
	delete ui;
}

void DiagramEditorHandlerSizeWidget::on_comboBox_currentIndexChanged(int index)
{
	if (auto editor_ = QETApp::instance()->diagramEditorAncestorOf(this))
	{
		editor_->setProperty("graphics_handler_size", (index+1) * 10);
		if (auto project_view = editor_->currentProjectView()) {
			if (auto diagram_view = project_view->currentDiagram()) {
				if (auto diagram = diagram_view->diagram())
				{
					for (const auto item : diagram->items())
					{
						if (item->type() == QetGraphicsHandlerItem::Type)
						{
							auto handler = qgraphicsitem_cast<QetGraphicsHandlerItem *>(item);
							handler->setSize((index+1) * 10);
						}
					}
				}
			}
		}
	}
}

