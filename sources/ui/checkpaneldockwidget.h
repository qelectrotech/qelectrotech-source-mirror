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
#ifndef CHECKPANELDOCKWIDGET_H
#define CHECKPANELDOCKWIDGET_H

#include "../projectcheck.h"

#include <QDockWidget>
#include <QHash>
#include <QMetaObject>
#include <QPointer>

class QETProject;
class QLabel;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The CheckPanelDockWidget class
	The Check panel (#1878 B1): runs every check of ProjectCheck on the
	current project when asked and lists the findings, grouped by check.
	Double-clicking a finding shows it on its folio. The results are not
	kept up to date while the project is edited: Run checks again.
	Options holds the choices of the checks which have some (what symbols
	with no wire leaves out), kept in the settings.
*/
class CheckPanelDockWidget : public QDockWidget
{
	Q_OBJECT

	public:
		explicit CheckPanelDockWidget(QWidget *parent = nullptr);

			/// Follow @p project; the results of another project are cleared
		void setProject(QETProject *project);
		QETProject *project() const {return m_project;}
			/// The number of findings of the last run
		int findingCount() const {return m_finding_count;}
		QTreeWidget *tree() const {return m_tree;}
		QMenu *optionsMenu() const {return m_options;}

	public slots:
		void run();
		bool showRow(QTreeWidgetItem *row);

	private:
		void clear(const QString &summary);
		void buildFolios();

		QPointer<QETProject> m_project;
			/// While the folios are built (events are processed), a new
			/// project waits here; run() takes it when the build is done
		bool m_building = false;
		bool m_has_pending = false;
		QPointer<QETProject> m_pending;
		QMetaObject::Connection m_destroyed;
		QPushButton *m_run = nullptr;
		QLabel *m_summary = nullptr;
		QMenu *m_options = nullptr;
		QTreeWidget *m_tree = nullptr;
		QHash<QTreeWidgetItem *, ProjectCheck::Place> m_places;
		int m_finding_count = 0;
};

#endif // CHECKPANELDOCKWIDGET_H
