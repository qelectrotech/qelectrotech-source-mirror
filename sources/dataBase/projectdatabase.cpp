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
QStringList projectDataBase::headersFromElementNomenclatureViewQuery(const QString &query)
{
	QStringList header_string;
	if (!query.startsWith("SELECT ") && !query.contains("FROM")) {
		return header_string;
	}

	auto header = query;
	header.remove(0, 7); //Remove SELECT from the string;
	header.truncate(header.indexOf("FROM")); //Now we only have the string between SELECT and FROM
	header.replace(" ", ""); //remove white space
	QStringList list = header.split(","); //split each column

	if (list.isEmpty()) {
		return header_string;
	}

	for (int i=0 ; i<list.size() ; ++i)
	{
		auto str = list.at(i);

		if (str == "position") {
			header_string.append(tr("Position"));
		} else if (str == "diagram_position") {
			header_string.append(tr("Position du folio"));
		} else {
			auto elmt_str = QETApp::elementTranslatedInfoKey(str);
			if (!elmt_str.isEmpty()) {
				header_string.append(elmt_str);
				continue;
			}
			auto diagram_str = QETApp::diagramTranslatedInfoKey(str);
			if (!diagram_str.isEmpty()) {
				header_string.append(diagram_str);
				continue;
			}
			else {
				header_string.append(str);
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
	populateDiagramTable();
	populateDiagramInfoTable();
	populateElementTable();
	populateElementInfoTable();
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

	QString connect_name = connection_name;
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
		if(!m_data_base.open()) {
			m_data_base.close();
			return false;
		}

		m_data_base.exec("PRAGMA temp_store = MEMORY");
		m_data_base.exec("PRAGMA journal_mode = MEMORY");
		m_data_base.exec("PRAGMA synchronous = OFF");

		QSqlQuery query_(m_data_base);
		bool first_ = true;

			//Create diagram table
		QString diagram_table("CREATE TABLE diagram ("
							  "uuid VARCHAR(50) PRIMARY KEY NOT NULL,"
							  "pos INTEGER)");
		if (!query_.exec(diagram_table)) {
			qDebug() << "diagram_table query : "<< query_.lastError();
		}

			//Create the table element
		QString element_table("CREATE TABLE element"
									"( "
									"uuid VARCHAR(50) PRIMARY KEY NOT NULL, "
									"diagram_uuid VARCHAR(50) NOT NULL,"
									"pos VARCHAR(6) NOT NULL,"
									"type VARCHAR(50),"
									"sub_type VARCHAR(50),"
									"FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid)"
									")");
		if (!query_.exec(element_table)) {
			qDebug() <<" element_table query : "<< query_.lastError();
		}

			//Create the diagram info table
		QString diagram_info_table("CREATE TABLE diagram_info (diagram_uuid VARCHAR(50) PRIMARY KEY NOT NULL, ");
		first_ = true;
		for (auto string : QETApp::diagramInfoKeys())
		{
			if (first_) {
				first_ = false;
			} else {
				diagram_info_table += ", ";
			}
			diagram_info_table += string += string=="date" ? " DATE" : " VARCHAR(100)";
		}
		diagram_info_table += ", FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid))";
		if (!query_.exec(diagram_info_table)) {
			qDebug() << "diagram_info_table query : " << query_.lastError();
		}

			//Create the element info table
		QString element_info_table("CREATE TABLE element_info(element_uuid VARCHAR(50) PRIMARY KEY NOT NULL,");
		first_=true;
		for (auto string : QETApp::elementInfoKeys())
		{
			if (first_) {
				first_ = false;
			} else {
				element_info_table += ",";
			}

			element_info_table += string += " VARCHAR(100)";
		}
		element_info_table += ", FOREIGN KEY (element_uuid) REFERENCES element (uuid));";

		if (!query_.exec(element_info_table)) {
			qDebug() << " element_info_table query : " << query_.lastError();
		}

		createElementNomenclatureView();
	}

	updateDB();
	return true;
}

void projectDataBase::createElementNomenclatureView()
{
	QString create_view ("CREATE VIEW element_nomenclature_view AS SELECT "
						 "ei.label AS label,"
						 "ei.plant AS plant,"
						 "ei.location AS location,"
						 "ei.comment AS comment,"
						 "ei.function AS function,"
						 "ei.tension_protocol AS tension_protocol,"
						 "ei.auxiliary1 AS auxiliary1,"
						 "ei.auxiliary2 AS auxiliary2,"
						 "ei.description AS description,"
						 "ei.designation AS designation,"
						 "ei.manufacturer AS manufacturer,"
						 "ei.manufacturer_reference AS manufacturer_reference,"
						 "ei.machine_manufacturer_reference AS machine_manufacturer_reference,"
						 "ei.supplier AS supplier,"
						 "ei.quantity AS quantity,"
						 "ei.unity AS unity,"
						 "d.pos AS diagram_position,"
						 "e.type AS element_type,"
						 "e.sub_type AS element_sub_type,"
						 "di.title AS title,"
						 "di.folio AS folio,"
						 "e.pos AS position "
						 " FROM element_info ei, diagram_info di, element e, diagram d"
						 " WHERE ei.element_uuid = e.uuid AND e.diagram_uuid = d.uuid AND di.diagram_uuid = d.uuid");

	QSqlQuery query(m_data_base);
	if (!query.exec(create_view)) {
		qDebug() << query.lastError();
	}
}

void projectDataBase::populateDiagramTable()
{
	QSqlQuery query_(m_data_base);
	query_.exec("DELETE FROM diagram");

	QString insert_("INSERT INTO diagram (uuid, pos) VALUES (:uuid, :pos)");
	query_.prepare(insert_);
	for (auto diagram : m_project->diagrams())
	{
		query_.bindValue(":uuid", diagram->uuid().toString());
		query_.bindValue(":pos", m_project->folioIndex(diagram));
		if(!query_.exec()) {
			qDebug() << "projectDataBase::populateDiagramTable insert error : " << query_.lastError();
		}
	}
}

/**
 * @brief projectDataBase::populateElementTable
 * Populate the element table
 */
void projectDataBase::populateElementTable()
{
	QSqlQuery query_(m_data_base);
	query_.exec("DELETE FROM element");

	QString insert_("INSERT INTO element (uuid, diagram_uuid, pos, type, sub_type) VALUES (:uuid, :diagram_uuid, :pos, :type, :sub_type)");
	query_.prepare(insert_);

	for (auto diagram : m_project->diagrams())
	{
		ElementProvider ep(diagram);
		QList<Element *> elements_list = ep.find(Element::Simple | Element::Terminale | Element::Master);
			//Insert all value into the database
		for (auto elmt : elements_list)
		{
			query_.bindValue(":uuid", elmt->uuid().toString());
			query_.bindValue(":diagram_uuid", diagram->uuid().toString());
			query_.bindValue(":pos", diagram->convertPosition(elmt->scenePos()).toString());
			query_.bindValue(":type", elmt->linkTypeToString());
			query_.bindValue(":sub_type", elmt->kindInformations()["type"].toString());
			if (!query_.exec()) {
				qDebug() << "projectDataBase::populateElementTable insert error : " << query_.lastError();
			}
		}
	}
}

/**
 * @brief projectDataBase::populateElementsTable
 * Populate the elements table
 */
void projectDataBase::populateElementInfoTable()
{
	QSqlQuery query(m_data_base);
	query.exec("DELETE FROM element_info");


		//Prepare the query used for insert new record
	QStringList bind_values;
	for (auto key : QETApp::elementInfoKeys()) {
		bind_values << key.prepend(":");
	}
	QString insert("INSERT INTO element_info (element_uuid," +
				   QETApp::elementInfoKeys().join(", ") +
				   ") VALUES (:uuid," +
				   bind_values.join(", ") +
				   ")");

	query.prepare(insert);

	for (auto *diagram : m_project->diagrams())
	{
		ElementProvider ep(diagram);
		QList<Element *> elements_list = ep.find(Element::Simple | Element::Terminale | Element::Master);

			//Insert all value into the database
		for (auto elmt : elements_list)
		{
			query.bindValue(":uuid", elmt->uuid().toString());
			auto hash = elementInfoToString(elmt);
			for (auto key : hash.keys())
			{
				QString value = hash.value(key);
				QString bind = key.prepend(":");
				query.bindValue(bind, value);
			}

			if (!query.exec()) {
				qDebug() << "projectDataBase::populateElementInfoTable insert error : " << query.lastError();
			}
		}
	}
}

void projectDataBase::populateDiagramInfoTable()
{
	QSqlQuery query(m_data_base);
	query.exec("DELETE FROM diagram_info");

		//Prepare the query used for insert new record
	QStringList bind_values;
	for (auto key : QETApp::diagramInfoKeys()) {
		bind_values << key.prepend(":");
	}
	QString insert("INSERT INTO diagram_info (diagram_uuid, " +
				   QETApp::diagramInfoKeys().join(", ") +
				   ") VALUES (:uuid, " +
				   bind_values.join(", ") +
				   ")");

	query.prepare(insert);

	for (auto *diagram : m_project->diagrams())
	{
		query.bindValue(":uuid", diagram->uuid());

		auto infos = diagram->border_and_titleblock.titleblockInformation();
		for (auto key : QETApp::diagramInfoKeys())
		{
			if (key == "date") {
				query.bindValue(":date", QDate::fromString(infos.value("date").toString(), Qt::SystemLocaleShortDate));
			} else {
				auto value = infos.value(key);
				auto bind = key.prepend(":");
				query.bindValue(bind, value);
			}
		}

		if (!query.exec()) {
			qDebug() << "projectDataBase::populateDiagramInfoTable insert error : " << query.lastError();
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
	QString connection_name("export_project_db_" + db->project()->uuid().toString());
	projectDataBase file_db(db->project(), connection_name, path_);
	QSqlDatabase::removeDatabase(connection_name);
}
