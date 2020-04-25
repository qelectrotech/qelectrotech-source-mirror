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

/**
 * @brief The projectDataBase class
 * This class wrap a sqlite data base where you can find several thing about
 * the content of a project.
 *
 * NOTE this class is still in developement.
 */
class projectDataBase : public QObject
{
	Q_OBJECT

	public:
		projectDataBase(QETProject *project, QObject *parent = nullptr);
	private:
		projectDataBase(QETProject *project, const QString &connection_name, const QString &path, QObject *parent = nullptr);
	public:
		virtual ~projectDataBase() override;

		QVector<QStringList> elementsInfoFromQuery(const QString &query);
		void updateDB();
		QETProject *project() const;

		static QStringList headersFromElementNomenclatureViewQuery(const QString &query);

	signals:
		void dataBaseUpdated();

	private:
		bool createDataBase(const QString &connection_name= QString(), const QString &name = QString());
		void createElementNomenclatureView();
		void populateDiagramTable();
		void populateElementTable();
		void populateElementInfoTable();
		void populateDiagramInfoTable();
		static QHash<QString, QString> elementInfoToString(Element *elmt);

	private:
		QPointer<QETProject> m_project;
		QSqlDatabase m_data_base;
		QSqlQuery m_insert_elements_query;

	public:
		static void exportDb(projectDataBase *db, QWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString());
};

#endif // PROJECTDATABASE_H
