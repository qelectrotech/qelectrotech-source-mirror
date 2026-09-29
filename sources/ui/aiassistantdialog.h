/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef AIASSISTANTDIALOG_H
#define AIASSISTANTDIALOG_H

#include "aiassistantsetup.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

/**
	@brief The AiAssistantDialog class
	Help > Connect an AI assistant: shows the configuration an assistant
	needs to start QElectroTech's MCP server, with this installation's
	paths filled in, for the user to copy into that assistant. It writes
	nothing and starts nothing.
*/
class AiAssistantDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit AiAssistantDialog(QWidget *parent = nullptr);

	private:
		void refresh();
		void chooseWorkspace();
		void copy();
		QString whereItGoes(AiAssistantSetup::Client client) const;

		AiAssistantSetup::Paths m_paths;
		QComboBox *m_client = nullptr;
		QLineEdit *m_workspace = nullptr;
		QCheckBox *m_allow_edit = nullptr;
		QLabel *m_where = nullptr;
		QPlainTextEdit *m_text = nullptr;
		QPushButton *m_copy = nullptr;
};

#endif // AIASSISTANTDIALOG_H
