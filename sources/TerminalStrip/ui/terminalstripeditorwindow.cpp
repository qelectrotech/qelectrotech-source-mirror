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
#include "ui_terminalstripeditorwindow.h"

#include "../UndoCommand/addterminalstripcommand.h"
#include "freeterminaleditor.h"
#include "../../qetapp.h"
#include "../../qetdiagrameditor.h"
#include "../../qetproject.h"
#include "../realterminal.h"
#include "../terminalstrip.h"
#include "terminalstripcreatordialog.h"
#include "terminalstripeditor.h"
#include "terminalstripeditorwindow.h"
#include "terminalstriptreedockwidget.h"

QPointer<TerminalStripEditorWindow> TerminalStripEditorWindow::window_;

static const int EMPTY_PAGE = 0;
static const int FREE_TERMINAL_PAGE = 1;
static const int TERMINAL_STRIP_PAGE = 2;
/**
 * @brief TerminalStripEditorWindow::TerminalStripEditorWindow
 * @param project
 * @param parent
 */
void TerminalStripEditorWindow::edit(TerminalStrip *strip)
{
	if (const auto project_ = strip->project())
	{
		auto editor_  = TerminalStripEditorWindow::instance(project_, QETApp::diagramEditor(project_));
		editor_->setCurrentStrip(strip);
		editor_->show();
	}
}

TerminalStripEditorWindow::TerminalStripEditorWindow(QETProject *project, QWidget *parent) :
	QMainWindow(parent),
    ui(new Ui::TerminalStripEditorWindow),
    m_project(project)
{
	ui->setupUi(this);
    if (auto diagram_editor = QETApp::diagramEditor(project)) {
        ui->m_tool_bar->addSeparator();
        ui->m_tool_bar->addAction(diagram_editor->undo);
        ui->m_tool_bar->addAction(diagram_editor->redo);
    }
	ui->m_remove_terminal->setDisabled(true);
	addTreeDockWidget();

	m_free_terminal_editor = new FreeTerminalEditor(m_project, this);
	m_terminal_strip_editor = new TerminalStripEditor{m_project, this};

	connect(m_tree_dock, &TerminalStripTreeDockWidget::currentStripChanged, this, &TerminalStripEditorWindow::currentStripChanged);

	ui->m_stacked_widget->insertWidget(EMPTY_PAGE, new QWidget(ui->m_stacked_widget));
	ui->m_stacked_widget->insertWidget(FREE_TERMINAL_PAGE, m_free_terminal_editor);
	ui->m_stacked_widget->insertWidget(TERMINAL_STRIP_PAGE, m_terminal_strip_editor);
}

/**
 * @brief TerminalStripEditorWindow::~TerminalStripEditorWindow
 */
TerminalStripEditorWindow::~TerminalStripEditorWindow()
{
	delete ui;
}

/**
 * @brief TerminalStripEditorWindow::setProject
 * @param project
 */
void TerminalStripEditorWindow::setProject(QETProject *project)
{
    m_project = project;
    m_tree_dock->setProject(project);
    m_free_terminal_editor->setProject(project);
    m_terminal_strip_editor->setProject(project);
}

void TerminalStripEditorWindow::setCurrentStrip(TerminalStrip *strip) {
	m_tree_dock->setSelectedStrip(strip);
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
	updateUi();

}

void TerminalStripEditorWindow::updateUi()
{
	ui->m_remove_terminal->setEnabled(m_tree_dock->currentIsStrip());

	ui->m_stacked_widget->setCurrentIndex(EMPTY_PAGE);

	if (auto real_terminal = m_tree_dock->currentRealTerminal())
	{
		if (!real_terminal->parentStrip())
		{
			ui->m_stacked_widget->setCurrentIndex(FREE_TERMINAL_PAGE);
			m_free_terminal_editor->reload();
		}
	} else if (auto strip_ = m_tree_dock->currentStrip()) {
		ui->m_stacked_widget->setCurrentIndex(TERMINAL_STRIP_PAGE);
		m_terminal_strip_editor->setCurrentStrip(strip_);
	}
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
}

/**
 * @brief TerminalStripEditorWindow::on_m_reload_triggered
 */
void TerminalStripEditorWindow::on_m_reload_triggered() {
	m_tree_dock->reload();
	m_terminal_strip_editor->reload();
	m_free_terminal_editor->reload();
}

/**
 * @brief TerminalStripEditorWindow::on_m_button_box_clicked
 * Action when user click on the apply/reset button
 * @param button
 */
void TerminalStripEditorWindow::on_m_button_box_clicked(QAbstractButton *button)
{
	auto role_{ui->m_button_box->buttonRole(button)};

	if (role_ == QDialogButtonBox::ApplyRole)
	{
		switch (ui->m_stacked_widget->currentIndex()) {
			case FREE_TERMINAL_PAGE:
				m_free_terminal_editor->apply();
				break;
			case TERMINAL_STRIP_PAGE:
				m_terminal_strip_editor->apply();
				break;
			default:
				break;
		}
	}
	else if (role_ == QDialogButtonBox::ResetRole)
	{
		m_terminal_strip_editor->reload();
		m_free_terminal_editor->reload();
	}
}


void TerminalStripEditorWindow::on_m_stacked_widget_currentChanged(int arg1) {
	ui->m_button_box->setHidden(arg1 == EMPTY_PAGE);
}
