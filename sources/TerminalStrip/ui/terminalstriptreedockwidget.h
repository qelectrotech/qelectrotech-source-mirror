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
#include <QPointer>

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

		void reload();
		bool currentIsStrip() const;
		TerminalStrip* currentStrip() const;
		QString currentInstallation() const;
		QString currentLocation() const;
		void setSelectedStrip(TerminalStrip *strip);
		QSharedPointer<RealTerminal> currentRealTerminal() const;

	signals:
		void currentStripChanged(TerminalStrip *strip);

	private slots:
		void on_m_tree_view_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

	private:
		void buildTree();
		QTreeWidgetItem* addTerminalStrip(TerminalStrip *terminal_strip);
		void addFreeTerminal();
		void setupUndoConnections();
		void setCurrentStrip(TerminalStrip *strip);

	private:
		Ui::TerminalStripTreeDockWidget *ui;
		QPointer<QETProject> m_project;
		QPointer<TerminalStrip> m_current_strip;

		QHash<QTreeWidgetItem *, TerminalStrip *> m_item_strip_H;
		QHash<QUuid, QSharedPointer<RealTerminal>> m_uuid_terminal_H;
		QHash<QUuid, QPointer<TerminalStrip>> m_uuid_strip_H;
};

#endif // TERMINALSTRIPTREEDOCKWIDGET_H
