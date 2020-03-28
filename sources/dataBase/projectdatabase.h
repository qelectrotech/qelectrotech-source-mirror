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

#include "qetproject.h"

class Element;

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
		virtual ~projectDataBase() override;

		QVector<QStringList> elementsInfoFromQuery(const QString &query);

	private:
		bool createDataBase();
		void populateElementsTable();
		static QHash<QString, QString> elementInfoToString(Element *elmt);
		QStringList elementsInfoKeys() const;

	private:
		QPointer<QETProject> m_project;
		QSqlDatabase m_data_base;
		QSqlQuery m_insert_elements_query;
};

#endif // PROJECTDATABASE_H
