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
#ifndef SCRIPTMANAGERDIALOG_H
#define SCRIPTMANAGERDIALOG_H

#include <QDialog>
#include <functional>

class QComboBox;
class QKeySequenceEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QToolButton;

/**
	@brief The ScriptManagerDialog class
	Projet > Scripts > Gérer les scripts… : write a stored script, give its
	button a name, icon, tooltip, shortcut and context, try it, delete it.

	It only ever reads and writes the files in ScriptLibrary::folder(),
	the same files a text editor or the qet MCP server would write: the
	library's watcher turns them into buttons. So the dialog, a hand-written
	file and an assistant can never disagree about what a script is.
*/
class ScriptManagerDialog : public QDialog
{
	Q_OBJECT

	public:
		using Runner = std::function<void(const QString &path, const QString &name)>;
		ScriptManagerDialog(Runner runner, QWidget *parent = nullptr);

	protected:
		void closeEvent(QCloseEvent *event) override;

	private:
		void reload();
		void showScript(const QString &path);
		void clearForm();
		bool save();
		void newScript();
		void deleteScript();
		void testScript();
		void chooseIcon();
		void updateIconPreview();
		void setModified(bool modified);
		bool confirmDiscard();

		Runner m_runner;
		QString m_path;		///< the file shown, empty for a new script not yet saved
		bool m_modified = false;
		bool m_loading = false;

		QListWidget *m_list;
		QLineEdit *m_name;
		QLineEdit *m_icon;
		QToolButton *m_icon_preview;
		QLineEdit *m_tooltip;
		QKeySequenceEdit *m_shortcut;
		QComboBox *m_context;
		QPlainTextEdit *m_body;
		QLabel *m_status;
		QPushButton *m_save;
		QPushButton *m_delete;
		QPushButton *m_test;
};

#endif // SCRIPTMANAGERDIALOG_H
