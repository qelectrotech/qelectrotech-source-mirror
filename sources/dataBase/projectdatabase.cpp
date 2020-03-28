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
#include "projectdatabase.h"
#include "qetapp.h"
#include "qetproject.h"
#include "elementprovider.h"
#include "element.h"

#include <QSqlError>

/**
 * @brief projectDataBase::projectDataBase
 * Default constructor
 * @param project : project from the database work
 * @param parent : parent QObject
 */
projectDataBase::projectDataBase(QETProject *project, QObject *parent) :
	QObject(parent),
	m_project(project)
{
	createDataBase();
}

/**
 * @brief projectDataBase::~projectDataBase
 * Destructor
 */
projectDataBase::~projectDataBase() {
	m_data_base.close();
}

/**
 * @brief projectDataBase::elementsInfoFromQuery
 * @param query
 * @return quickly return all information about elements from @query, ensure that all
 * record returned by the query can be casted to string.
 * Each item of the Qvector represent an element and each value of the QStringList represent the recorded value.
 */
QVector<QStringList> projectDataBase::elementsInfoFromQuery(const QString &query)
{
	QVector<QStringList> result_;

	QSqlQuery query_(query, m_data_base);
	if (!query_.exec()) {
		qDebug() << "Query error : " << query_.lastError();
	}

	while (query_.next())
	{
		QStringList record_;
		auto i=0;
		while (query_.value(i).isValid())
		{
			record_ << query_.value(i).toString();
			++i;
		}
		result_ << record_;
	}

	return result_;
}

/**
 * @brief projectDataBase::createDataBase
 * Create the data base
 * @return : true if the data base was successfully created.
 */
bool projectDataBase::createDataBase()
{
	QString connect_name("qet_project_db_" + m_project->uuid().toString());
	if (m_data_base.connectionNames().contains(connect_name)) {
		m_data_base = QSqlDatabase::database(connect_name);
	}
	else
	{
		m_data_base = QSqlDatabase::addDatabase("QSQLITE", connect_name);
		if(!m_data_base.open())
		{
			m_data_base.close();
			return false;
		}

			//Create the elements table
		QString elements_table("CREATE TABLE element_info(");
		bool first = true;
		for (auto string : elementsInfoKeys())
		{
			if (first) {
				first = false;
			} else {
				elements_table += ",";
			}

			elements_table += string += " VARCHAR(512)";
		}
		elements_table += ");";

		QSqlQuery query_(elements_table, m_data_base);
		query_.exec();
	}

	populateElementsTable();

	return true;
}

/**
 * @brief projectDataBase::populateElementsTable
 * Populate the elements table
 */
void projectDataBase::populateElementsTable()
{
		//Very ugly, in futur we should update the table instead of delete all
	QSqlQuery clear_table(m_data_base);
	if (!clear_table.exec("DELETE FROM element_info")) {
		qDebug() << "last error " << clear_table.lastError();
	}

		//Prepare the query used for insert new record
	QStringList bind_values;
	for (auto key : elementsInfoKeys()) {
		bind_values << key.prepend(":");
	}
	QString insert("INSERT INTO element_info (" +
				   elementsInfoKeys().join(", ") +
				   ") VALUES (" +
				   bind_values.join(", ") +
				   ")");

	QSqlQuery query(m_data_base);
	query.prepare(insert);


	for (auto *diagram : m_project->diagrams())
	{
		ElementProvider ep(diagram);
		QList<Element *> elements_list = ep.find(Element::Simple | Element::Terminale | Element::Master);

			//Insert all value into the database
		for (auto elmt : elements_list)
		{
			auto hash = elementInfoToString(elmt);
			for (auto key : hash.keys())
			{
				QString value = hash.value(key);
				QString bind = key.prepend(":");
				query.bindValue(bind, value);
			}

			query.bindValue(":element_type", elmt->linkTypeToString());
			query.bindValue(":element_subtype", elmt->kindInformations()["type"].toString());

			if (!query.exec()) {
				qDebug() << "projectDataBase::populateElementsTable insert error : " << query.lastError();
			}
		}
	}
}

/**
 * @brief projectDataBase::elementInfoToString
 * @param elmt
 * @return the element information in hash as key for the info name and value as the information value.
 */
QHash<QString, QString> projectDataBase::elementInfoToString(Element *elmt)
{
	QHash<QString, QString> hash; //Store the value for each columns
	for (auto key : QETApp::elementInfoKeys())
	{
		if (key == "label") {
			hash.insert(key, elmt->actualLabel());
		}
		else {
			hash.insert(key, elmt->elementInformations()[key].toString());
		}
	}

	return hash;
}

/**
 * @brief projectDataBase::elementsInfoKeys
 * @return QETApp::elementInfoKeys() + "element_type" and "element_subtype"
 */
QStringList projectDataBase::elementsInfoKeys() const
{
	auto keys_ = QETApp::elementInfoKeys();
	keys_<< "element_type" << "element_subtype";

	return keys_;
}
