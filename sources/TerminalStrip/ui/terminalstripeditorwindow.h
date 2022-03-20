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
#ifndef TERMINALSTRIPEDITORWINDOW_H
#define TERMINALSTRIPEDITORWINDOW_H

#include <QMainWindow>

class QETProject;
class TerminalStripTreeDockWidget;
class TerminalStrip;
class FreeTerminalEditor;
class TerminalStripEditor;

namespace Ui {
	class TerminalStripEditorWindow;
}

class TerminalStripEditorWindow : public QMainWindow
{
		Q_OBJECT

	public:
		explicit TerminalStripEditorWindow(QETProject *project, QWidget *parent = nullptr);
		~TerminalStripEditorWindow();

	private slots:
		void on_m_add_terminal_strip_triggered();
		void on_m_remove_terminal_triggered();
		void on_m_reload_triggered();

	private:
		void addTreeDockWidget();
		void currentStripChanged(TerminalStrip *strip);
		void updateUi();

	private:
		Ui::TerminalStripEditorWindow *ui{nullptr};
		QETProject *m_project {nullptr};
		TerminalStripTreeDockWidget *m_tree_dock{nullptr};
		FreeTerminalEditor *m_free_terminal_editor {nullptr};
		TerminalStripEditor *m_terminal_strip_editor {nullptr};
};

#endif // TERMINALSTRIPEDITORWINDOW_H
