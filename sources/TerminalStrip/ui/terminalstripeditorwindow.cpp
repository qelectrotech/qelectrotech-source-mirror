/*
	Copyright 2006-2022 The QElectroTech Team
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
#include "terminalstripeditorwindow.h"
#include "ui_terminalstripeditorwindow.h"
#include "terminalstriptreedockwidget.h"
#include "../terminalstrip.h"
#include "terminalstripcreatordialog.h"
#include "../UndoCommand/addterminalstripcommand.h"
#include "../../qetproject.h"

/**
 * @brief TerminalStripEditorWindow::TerminalStripEditorWindow
 * @param project
 * @param parent
 */
TerminalStripEditorWindow::TerminalStripEditorWindow(QETProject *project, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::TerminalStripEditorWindow),
	m_project(project)
{
	ui->setupUi(this);
	ui->m_remove_terminal->setDisabled(true);
	addTreeDockWidget();

	connect(m_tree_dock, &TerminalStripTreeDockWidget::currentStripChanged, this, &TerminalStripEditorWindow::currentStripChanged);
}

/**
 * @brief TerminalStripEditorWindow::~TerminalStripEditorWindow
 */
TerminalStripEditorWindow::~TerminalStripEditorWindow()
{
	delete ui;
}

/**
 * @brief TerminalStripEditorWindow::addTreeDockWidget
 */
void TerminalStripEditorWindow::addTreeDockWidget()
{
	m_tree_dock = new TerminalStripTreeDockWidget(m_project, this);
	m_tree_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	addDockWidget(Qt::LeftDockWidgetArea, m_tree_dock);
}

/**
 * @brief TerminalStripEditorWindow::currentStripChanged
 * @param strip
 */
void TerminalStripEditorWindow::currentStripChanged(TerminalStrip *strip)
{
	Q_UNUSED(strip)
	ui->m_remove_terminal->setEnabled(m_tree_dock->currentIsStrip());
}

/**
 * @brief TerminalStripEditorWindow::on_m_add_terminal_strip_triggered
 * Action when user click on add terminal strip button
 */
void TerminalStripEditorWindow::on_m_add_terminal_strip_triggered()
{
	QScopedPointer<TerminalStripCreatorDialog> dialog(new TerminalStripCreatorDialog(m_project, this));

	dialog->setLocation(m_tree_dock->currentLocation());
	dialog->setInstallation(m_tree_dock->currentInstallation());

	if (dialog->exec() == QDialog::Accepted)
	{
		auto ts = dialog->generatedTerminalStrip();
		m_project->undoStack()->push(new AddTerminalStripCommand(ts, m_project));

		m_tree_dock->reload();
		m_tree_dock->setSelectedStrip(ts);
	}
}

/**
 * @brief TerminalStripEditorWindow::on_m_remove_terminal_triggered
 */
void TerminalStripEditorWindow::on_m_remove_terminal_triggered()
{
	if (m_tree_dock->currentIsStrip())
	{
		if (auto strip_ = m_tree_dock->currentStrip())
		{
			m_project->undoStack()->push(new RemoveTerminalStripCommand(strip_, m_project));
			m_tree_dock->reload();
		}

	}
//	auto item = ui->m_tree_view->currentItem();
//	if (auto strip = m_item_strip_H.value(item))
//	{
//		m_item_strip_H.remove(item);
//		m_uuid_strip_H.remove(strip->uuid());
//		delete item;

//		m_project->undoStack()->push(new RemoveTerminalStripCommand(strip, m_project));
//	}

//	on_m_reload_pb_clicked();

}


/**
 * @brief TerminalStripEditorWindow::on_m_reload_triggered
 */
void TerminalStripEditorWindow::on_m_reload_triggered() {
	m_tree_dock->reload();
}

