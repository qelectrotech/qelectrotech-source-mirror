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
#ifndef TERMINALSTRIPTREEDOCKWIDGET_H
#define TERMINALSTRIPTREEDOCKWIDGET_H

#include <QDockWidget>

class QETProject;
class QTreeWidgetItem;
class TerminalStrip;
class RealTerminal;

namespace Ui {
	class TerminalStripTreeDockWidget;
}

class TerminalStripTreeDockWidget : public QDockWidget
{
		Q_OBJECT

	public:
		explicit TerminalStripTreeDockWidget(QETProject *project, QWidget *parent = nullptr);
		~TerminalStripTreeDockWidget();

		void buildTree();

	private:
		QTreeWidgetItem* addTerminalStrip(TerminalStrip *terminal_strip);
		void addFreeTerminal();
		void setupUndoConnections();

	private:
		Ui::TerminalStripTreeDockWidget *ui;
		QETProject *m_project = nullptr;

		QHash<QTreeWidgetItem *, TerminalStrip *> m_item_strip_H;
		QHash<QUuid, QSharedPointer<RealTerminal>> m_uuid_terminal_H;
		QHash<QUuid, QPointer<TerminalStrip>> m_uuid_strip_H;
};

#endif // TERMINALSTRIPTREEDOCKWIDGET_H
