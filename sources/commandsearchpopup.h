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
#ifndef COMMANDSEARCHPOPUP_H
#define COMMANDSEARCHPOPUP_H

#include <QFrame>
#include <QList>

class QAction;
class QLineEdit;
class QListWidget;

/**
	@brief Type part of a command's name, press Enter to run it.

	Lists every command the owning window registered with ShortcutManager,
	best match first, with its key if it has one. Matching ignores case and
	accents, so "editer" finds "Éditer". Disabled commands are listed,
	greyed, but cannot be run.
*/
class CommandSearchPopup : public QFrame
{
	Q_OBJECT

	public:
		explicit CommandSearchPopup(QWidget *owner);

		void popUpAt(const QPoint &global_pos);
		static QString fold(const QString &text);

	protected:
		void keyPressEvent(QKeyEvent *event) override;

	private:
		void collect();
		void filter();
		void runCurrent();

		struct Command {
			QAction *action;
			QString text;     ///< as shown
			QString folded;   ///< for matching
		};

		QLineEdit *m_search = nullptr;
		QListWidget *m_list = nullptr;
		QList<Command> m_commands;
};

#endif // COMMANDSEARCHPOPUP_H
