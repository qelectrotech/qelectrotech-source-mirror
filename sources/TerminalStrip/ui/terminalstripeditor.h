/*
	Copyright 2006-2021 The QElectroTech Team
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
#ifndef TERMINALSTRIPEDITOR_H
#define TERMINALSTRIPEDITOR_H

#include <QDialog>

namespace Ui {
	class TerminalStripEditor;
}

class QETProject;

class TerminalStripEditor : public QDialog
{
		Q_OBJECT

	public:
		explicit TerminalStripEditor(QETProject *project, QWidget *parent = nullptr);
		~TerminalStripEditor() override;

	private:
		Ui::TerminalStripEditor *ui;
		QETProject *m_project = nullptr;
};

#endif // TERMINALSTRIPEDITOR_H
