/*
		Copyright 2006-2020 QElectroTech Team
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
class sqlite3;

/**
	@brief The projectDataBase class
	This class wrap a sqlite data base where you can find several thing
	about the content of a project.
 *
	@note this class is still in developement.
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
		void addElement(Element *element);
		void removeElement(Element *element);
		void elementInfoChanged(Element *element);
		void addDiagram(Diagram *diagram);
		void removeDiagram(Diagram *diagram);

	signals:
		void dataBaseUpdated();

	private:
		bool createDataBase();
		void createElementNomenclatureView();
		void createSummaryView();
		void populateDiagramTable();
		void populateElementTable();
		void populateElementInfoTable();
		void populateDiagramInfoTable();
		void prepareQuery();
		static QHash<QString, QString> elementInfoToString(
				Element *elmt);

	private:
		QPointer<QETProject> m_project;
		QSqlDatabase m_data_base;
		QSqlQuery m_insert_elements_query,
				  m_insert_element_info_query,
				  m_remove_element_query,
				  m_update_element_query,
				  m_insert_diagram_query,
				  m_remove_diagram_query,
				  m_insert_diagram_info_query;

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
