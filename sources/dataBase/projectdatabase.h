/*
		Copyright 2006-2026 QElectroTech Team
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
#ifndef PROJECTDATABASE_H
#define PROJECTDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPointer>
#include <QFileDialog>

class Element;
class QETProject;
class Diagram;
class Terminal;
class sqlite3;

/**
	@brief The projectDataBase class
	This class wraps a sqlite data base where you can find several things
	about the content of a project.
 *
	@note this class is still in development.
*/
class projectDataBase : public QObject
{
	Q_OBJECT

	public:
		projectDataBase(QETProject *project, QObject *parent = nullptr);
		virtual ~projectDataBase() override;

		void updateDB();
		QETProject *project() const;
		QSqlQuery newQuery(const QString &query = QString());

		void addElement         (Element *element);
		void removeElement      (Element *element);
		void elementInfoChanged (Element *element);
		void elementInfoChanged (QList<Element *> elements);

		void addDiagram         (Diagram *diagram);
		void removeDiagram      (Diagram *diagram);
		void diagramInfoChanged (Diagram *diagram);
		void diagramOrderChanged();

	signals:
		void dataBaseUpdated();

	private:
		bool createDataBase();
		void createElementNomenclatureView();
		void createSummaryView();
		void createWireListView();
		QString terminalTid(Terminal *t) const;
		void populateTerminalTable();
		void populateConductorTable();
		void populateDiagramTable();
		void populateElementTable();
		void populateElementInfoTable();
		void populateDiagramInfoTable();
		void prepareQuery();
		static QHash<QString, QString> elementInfoToString(
				Element *elmt);
		void bindDiagramInfoValues(QSqlQuery &query, Diagram *diagram);

	private:
		QPointer<QETProject> m_project;
		QSqlDatabase m_data_base;
		QSqlQuery m_insert_elements_query,
				  m_insert_element_info_query,
				  m_remove_element_query,
				  m_update_element_query,
				  m_insert_diagram_query,
				  m_remove_diagram_query,
				  m_insert_diagram_info_query,
				  m_update_diagram_info_query,
				  m_diagram_order_changed,
				  m_diagram_info_order_changed;
			//Terminal -> tid, assigned in populateTerminalTable and reused by
			//populateConductorTable (same updateDB pass, same live objects) so
			//conductor endpoints join to the exact terminal row.
			QHash<Terminal *, QString> m_terminal_tid;

#ifdef QET_EXPORT_PROJECT_DB
	public:
		static sqlite3 *sqliteHandle(QSqlDatabase *db);
		static void exportDb(projectDataBase *db,
				     QWidget *parent = nullptr,
				     const QString &caption = QString(),
				     const QString &dir = QString());
#endif
};

#endif // PROJECTDATABASE_H
