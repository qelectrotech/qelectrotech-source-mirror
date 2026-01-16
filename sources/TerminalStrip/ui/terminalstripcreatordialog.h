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
#ifndef TERMINALSTRIPCREATORDIALOG_H
#define TERMINALSTRIPCREATORDIALOG_H

#include <QDialog>

class TerminalStrip;
class QETProject;

namespace Ui {
	class TerminalStripCreatorDialog;
}

/**
 * @brief The TerminalStripCreatorDialog class
 * A simple dialog for create a new terminal strip
 */
class TerminalStripCreatorDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit TerminalStripCreatorDialog(QETProject *project, QWidget *parent = nullptr);
		~TerminalStripCreatorDialog() override;

		void setInstallation(const QString &installation);
		void setLocation(const QString &location);
		TerminalStrip *generatedTerminalStrip() const;

	private:
		void setCursorToEmptyLine();

	private:
		Ui::TerminalStripCreatorDialog *ui;
		QETProject *m_project = nullptr;
};

#endif // TERMINALSTRIPCREATORDIALOG_H
