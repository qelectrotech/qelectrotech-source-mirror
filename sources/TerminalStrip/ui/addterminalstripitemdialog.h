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
#ifndef ADDTERMINALSTRIPITEMDIALOG_H
#define ADDTERMINALSTRIPITEMDIALOG_H

#include <QDialog>
#include <QPointer>

class Diagram;
class QETDiagramEditor;
class QETProject;
class TerminalStrip;

namespace Ui {
	class AddTerminalStripItemDialog;
}

class AddTerminalStripItemDialog : public QDialog
{
		Q_OBJECT

	public:
		static void openDialog(Diagram *diagram, QWidget *parent = nullptr);

	private:
		explicit AddTerminalStripItemDialog(QETProject *project, QWidget *parent = nullptr);
		~AddTerminalStripItemDialog();
		TerminalStrip *selectedTerminalStrip() const;
		void fillComboBox();

	private:
		QPointer<QETProject> m_project;
		Ui::AddTerminalStripItemDialog *ui;
};

#endif // ADDTERMINALSTRIPITEMDIALOG_H
