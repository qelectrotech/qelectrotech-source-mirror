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
class Conductor;
class Terminal;

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
			/**
				Suppress the full rebuild performed by updateDB().

				While blocked, updateDB() returns immediately instead of
				repopulating every table. Meant for bulk operations -- notably
				loading a project, where each table model re-queries the
				database as it is built and would otherwise trigger one
				complete rebuild of it. The caller unblocks and calls
				updateDB() once when done; @see QETProject::readProjectXml().
			*/
		void setUpdateBlocked(bool blocked);
		QETProject *project() const;
		QSqlQuery newQuery(const QString &query = QString(), QString *error = nullptr);
		static bool isReadOnlySelect(const QString &query, QString *error = nullptr);

			/**
				The most rows any caller reads out of one query result.

				A SELECT is not bounded by how much data the project holds:
				SQLite produces rows lazily, so a query that never stops
				producing them makes the loop that reads them never stop
				either. A recursive CTE does exactly that in one line, and
				a <graphics_table>'s <query> is stored in the .qet and run
				on load -- so the text can arrive from a file rather than
				from the person at the keyboard, and opening that file is
				the whole attack.

				100000 is far above any real result: the largest table in
				the shipped examples is 396 rows. It is a backstop, not a
				page size -- a caller that hits it has almost certainly
				been handed something it should not run to completion, and
				says so rather than truncating quietly.
			*/
		static constexpr int MaxResultRows = 100000;
		QSqlDatabase database() const {return m_data_base;}
		int excludedConductorCount() const;

		void addElement         (Element *element);
		void removeElement      (Element *element);
		void elementInfoChanged (Element *element);
		void elementInfoChanged (QList<Element *> elements);

		void addDiagram         (Diagram *diagram);
		void removeDiagram      (Diagram *diagram);
		void diagramInfoChanged (Diagram *diagram);
		void diagramOrderChanged();

		void addConductor       (Conductor *conductor);
		void removeConductor    (Conductor *conductor);
		void updateConductor    (Conductor *conductor);

	private slots:
			//Refresh the sender()'s row after Conductor::setProperties().
		void conductorPropertiesChanged();

	public:

	signals:
		void dataBaseUpdated();

	private:
		bool createDataBase();
		void createElementNomenclatureView();
		void createSummaryView();
		void createWiringListView();
		void populateDiagramTable();
		void populateElementTable();
		void populateElementInfoTable();
		void populateDiagramInfoTable();
		void populateConductorTable();
		void bindConductorValues(QSqlQuery &query, Conductor *conductor, Diagram *diagram);
		void watchConductor(Conductor *conductor);
		void insertTerminal(Terminal *terminal);
		void prepareQuery();
		static QHash<QString, QString> elementInfoToString(
				Element *elmt);
		void bindDiagramInfoValues(QSqlQuery &query, Diagram *diagram);
		static void bindElementValues(QSqlQuery &query, Element *element, Diagram *diagram);
		static void bindElementInfoValues(QSqlQuery &query, Element *element);

	private:
		QPointer<QETProject> m_project;
		bool m_update_blocked = false;
			//Starts true : the database is empty until the first rebuild.
			//Set by every method of this class that writes rows, cleared by
			//updateDB(). Callers reach the database from outside only through
			//newQuery(), and every such call site reads.
		bool m_content_changed = true;
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
				  m_diagram_info_order_changed,
				  m_insert_terminal_query,
				  m_insert_conductor_query,
				  m_update_conductor_query,
				  m_remove_conductor_query,
				  m_cascade_remove_element_info_query,
				  m_cascade_remove_terminal_query,
				  m_cascade_remove_conductor_query,
				  m_cascade_remove_element_query;

#ifdef QET_EXPORT_PROJECT_DB
	public:
		static void exportDb(projectDataBase *db,
				     QWidget *parent = nullptr,
				     const QString &caption = QString(),
				     const QString &dir = QString());
#endif
};

#endif // PROJECTDATABASE_H
