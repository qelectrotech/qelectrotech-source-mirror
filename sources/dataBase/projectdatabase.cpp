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
#include "diagram.h"
#include "diagramposition.h"

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

projectDataBase::projectDataBase(QETProject *project, const QString &connection_name, const QString &path, QObject *parent) :
	QObject(parent),
	m_project(project)
{
	createDataBase(connection_name, path);
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
 * @brief projectDataBase::headersFromElementsInfoQuery
 * @param query
 * @return the header according to @query.
 * Header can be false, notably when user create is own query.
 */
QStringList projectDataBase::headersFromElementsInfoQuery(const QString &query)
{
	QStringList header_string;
	if (query.startsWith("SELECT ") && query.contains("FROM"))
	{
		auto header = query;
		header.remove(0, 7); //Remove SELECT from the string;
		header.truncate(header.indexOf("FROM")); //Now we only have the string between SELECT and FROM
		header.replace(" ", ""); //remove white space
		QStringList list = header.split(",");

		if (!list.isEmpty())
		{
			for (int i=0 ; i<list.size() ; i++)
			{
				if (list.at(i) == "pos") {
					header_string.append(tr("Position"));
				} else {
					header_string.append(QETApp::elementTranslatedInfoKey(list.at(i)));
				}
			}
		}
	}

	return header_string;
}

/**
 * @brief projectDataBase::updateDB
 * Up to date the content of the data base.
 * Except at the creation of this class,
 * call this method each time you want to query the data base
 * to be sure that the content reflect the current state of the project.
 * Emit the singal dataBaseUpdated
 */
void projectDataBase::updateDB()
{
	populateElementsTable();
	emit dataBaseUpdated();
}

/**
 * @brief projectDataBase::project
 * @return the project of this  database
 */
QETProject *projectDataBase::project() const {
	return m_project;
}

/**
 * @brief projectDataBase::createDataBase
 * Create the data base
 * @return : true if the data base was successfully created.
 */
bool projectDataBase::createDataBase(const QString &connection_name, const QString &name)
{

	QString connect_name=connection_name;
	if (connect_name.isEmpty()) {
		connect_name = "qet_project_db_" + m_project->uuid().toString();
	}
	if (m_data_base.connectionNames().contains(connect_name)) {
		m_data_base = QSqlDatabase::database(connect_name);
	}
	else
	{
		m_data_base = QSqlDatabase::addDatabase("QSQLITE", connect_name);
		m_data_base.setDatabaseName(name);
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

	updateDB();
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
			query.bindValue(":pos", elmt->diagram()->convertPosition(elmt->scenePos()).toString());

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
QStringList projectDataBase::elementsInfoKeys()
{
	auto keys_ = QETApp::elementInfoKeys();
	keys_<< "element_type" << "subtype" << "pos";

	return keys_;
}

/**
 * @brief projectDataBase::exportDb
 * @param parent
 * @param caption
 * @param dir
 * @param filter
 * @param selectedFilter
 * @param options
 */
void projectDataBase::exportDb(projectDataBase *db, QWidget *parent, const QString &caption, const QString &dir)
{
	auto caption_ = caption;
	if (caption_.isEmpty()) {
		caption_ = tr("Exporter la base de données interne du projet");
	}

	auto dir_ = dir;
	if(dir_.isEmpty()) {
		dir_ = db->project()->filePath();
		if (dir_.isEmpty()) {
			dir_ = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
			dir_ += QString("/") += tr("sans_nom") += ".sqlite";
		} else {
			dir_.remove(".qet");
			dir_.append(".sqlite");
		}
	}

	auto path_ = QFileDialog::getSaveFileName(parent, caption_, dir_, "*.sqlite");
	if (path_.isNull()) {
		return;
	}

		//Database is filled at creation, work is done.
	projectDataBase file_db(db->project(), "export_project_db_" + db->project()->uuid().toString(), path_);
}
