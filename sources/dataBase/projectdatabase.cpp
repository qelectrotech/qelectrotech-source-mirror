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
#include <QLocale>

#if defined(Q_OS_LINUX) || defined(Q_OS_WINDOWS)
#include <QSqlDriver>
#include <sqlite3.h>
#endif

/**
	@brief projectDataBase::projectDataBase
	Default constructor
	@param project : project from the database work
	@param parent : parent QObject
*/
projectDataBase::projectDataBase(QETProject *project, QObject *parent) :
	QObject(parent),
	m_project(project)
{
	createDataBase();
}

/**
	@brief projectDataBase::~projectDataBase
	Destructor
*/
projectDataBase::~projectDataBase()
{
	m_data_base.close();
}

/**
	@brief projectDataBase::updateDB
	Up to date the content of the data base.
	Emit the signal dataBaseUpdated
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
	@brief projectDataBase::project
	@return the project of this  database
*/
QETProject *projectDataBase::project() const
{
	return m_project;
}

/**
	@brief projectDataBase::newQuery
	@return a QSqlquery with query as query
	and the internal database of this class as database to use.
*/
QSqlQuery projectDataBase::newQuery(const QString &query) {
	return QSqlQuery(query, m_data_base);
}

/**
	@brief projectDataBase::addElement
	@param element
*/
void projectDataBase::addElement(Element *element)
{
	m_insert_elements_query.bindValue(":uuid", element->uuid().toString());
	m_insert_elements_query.bindValue(":diagram_uuid", element->diagram()->uuid().toString());
	m_insert_elements_query.bindValue(":pos", element->diagram()->convertPosition(element->scenePos()).toString());
	m_insert_elements_query.bindValue(":type", element->linkTypeToString());
	m_insert_elements_query.bindValue(":sub_type", element->kindInformations()["type"].toString());
	if (!m_insert_elements_query.exec()) {
		qDebug() << "projectDataBase::addElement insert element error : " << m_insert_elements_query.lastError();
	}

	m_insert_element_info_query.bindValue(":uuid", element->uuid().toString());
	auto hash = elementInfoToString(element);
	for (auto key : hash.keys())
	{
		QString value = hash.value(key);
		QString bind = key.prepend(":");
		m_insert_element_info_query.bindValue(bind, value);
	}

	if (!m_insert_element_info_query.exec()) {
		qDebug() << "projectDataBase::addElement insert element info error : " << m_insert_element_info_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

/**
	@brief projectDataBase::removeElement
	@param element
*/
void projectDataBase::removeElement(Element *element)
{
	m_remove_element_query.bindValue(":uuid", element->uuid().toString());
	if(!m_remove_element_query.exec()) {
		qDebug() << "projectDataBase::removeElement remove error : " << m_remove_element_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

/**
	@brief projectDataBase::elementInfoChanged
	@param element
*/
void projectDataBase::elementInfoChanged(Element *element)
{
	auto hash = elementInfoToString(element);
	for (auto str : QETApp::elementInfoKeys()) {
		m_update_element_query.bindValue(":" + str, hash.value(str));
	}
	m_update_element_query.bindValue(":uuid", element->uuid().toString());
	if (!m_update_element_query.exec()) {
		qDebug() << "projectDataBase::elementInfoChanged update error : " << m_update_element_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

void projectDataBase::addDiagram(Diagram *diagram)
{
	m_insert_diagram_query.bindValue(":uuid", diagram->uuid().toString());
	m_insert_diagram_query.bindValue(":pos", m_project->folioIndex(diagram));
	if(!m_insert_diagram_query.exec()) {
		qDebug() << "projectDataBase::addDiagram insert error : " << m_insert_diagram_query.lastError();
	}


	m_insert_diagram_info_query.bindValue(":uuid", diagram->uuid());
	auto infos = diagram->border_and_titleblock.titleblockInformation();
	for (auto key : QETApp::diagramInfoKeys())
	{
		if (key == "date")
		{
			m_insert_diagram_info_query.bindValue( ":date",
												   QLocale::system().toDate(infos.value("date").toString(),
																			QLocale::ShortFormat));
		} else {
			auto value = infos.value(key);
			auto bind = key.prepend(":");
			m_insert_diagram_info_query.bindValue(bind, value);
		}
	}

	if (!m_insert_diagram_info_query.exec()) {
		qDebug() << "projectDataBase::addDiagram insert info error : " << m_insert_diagram_info_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

void projectDataBase::removeDiagram(Diagram *diagram)
{
	m_remove_diagram_query.bindValue(":uuid", diagram->uuid().toString());
	if (!m_remove_diagram_query.exec()) {
		qDebug() << "projectDataBase::removeDiagram delete error : " << m_remove_diagram_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

/**
	@brief projectDataBase::createDataBase
	Create the data base
	@return : true if the data base was successfully created.
*/
bool projectDataBase::createDataBase()
{
	m_data_base = QSqlDatabase::addDatabase("QSQLITE", "qet_project_db_" + m_project->uuid().toString());
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
	createSummaryView();
	prepareQuery();
	updateDB();
	return true;
}

/**
	@brief projectDataBase::createElementNomenclatureView
*/
void projectDataBase::createElementNomenclatureView()
{
	QString create_view ("CREATE VIEW element_nomenclature_view AS SELECT "
						 "ei.label AS label,"
						 "ei.plant AS plant,"
						 "ei.location AS location,"
						 "ei.comment AS comment,"
						 "ei.function AS function,"
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

/**
	@brief projectDataBase::createSummaryView
*/
void projectDataBase::createSummaryView()
{
	QString create_view ("CREATE VIEW project_summary_view AS SELECT "
						 "di.title AS title,"
						 "di.author AS author,"
						 "di.folio AS folio,"
						 "di.plant AS plant,"
						 "di.locmach AS locmach,"
						 "di.indexrev AS indexrev,"
						 "di.date AS date,"
						 "d.pos AS pos"
						 " FROM diagram_info di, diagram d"
						 " WHERE di.diagram_uuid = d.uuid");

	QSqlQuery query(m_data_base);
	if (!query.exec(create_view)) {
		qDebug() << query.lastError();
	}
}

void projectDataBase::populateDiagramTable()
{
	QSqlQuery query_(m_data_base);
	query_.exec("DELETE FROM diagram");

	for (auto diagram : m_project->diagrams())
	{
		m_insert_diagram_query.bindValue(":uuid", diagram->uuid().toString());
		m_insert_diagram_query.bindValue(":pos", m_project->folioIndex(diagram));
		if(!m_insert_diagram_query.exec()) {
			qDebug() << "projectDataBase::populateDiagramTable insert error : " << m_insert_diagram_query.lastError();
		}
	}
}

/**
	@brief projectDataBase::populateElementTable
	Populate the element table
*/
void projectDataBase::populateElementTable()
{
	QSqlQuery query_(m_data_base);
	query_.exec("DELETE FROM element");

	for (auto diagram : m_project->diagrams())
	{
		ElementProvider ep(diagram);
		QList<Element *> elements_list = ep.find(Element::Simple | Element::Terminale | Element::Master);
			//Insert all value into the database
		for (auto elmt : elements_list)
		{
			m_insert_elements_query.bindValue(":uuid", elmt->uuid().toString());
			m_insert_elements_query.bindValue(":diagram_uuid", diagram->uuid().toString());
			m_insert_elements_query.bindValue(":pos", diagram->convertPosition(elmt->scenePos()).toString());
			m_insert_elements_query.bindValue(":type", elmt->linkTypeToString());
			m_insert_elements_query.bindValue(":sub_type", elmt->kindInformations()["type"].toString());
			if (!m_insert_elements_query.exec()) {
				qDebug() << "projectDataBase::populateElementTable insert error : " << m_insert_elements_query.lastError();
			}
		}
	}
}

/**
	@brief projectDataBase::populateElementsTable
	Populate the elements table
*/
void projectDataBase::populateElementInfoTable()
{
	QSqlQuery query(m_data_base);
	query.exec("DELETE FROM element_info");

	for (auto *diagram : m_project->diagrams())
	{
		ElementProvider ep(diagram);
		QList<Element *> elements_list = ep.find(Element::Simple | Element::Terminale | Element::Master);

			//Insert all value into the database
		for (auto elmt : elements_list)
		{
			m_insert_element_info_query.bindValue(":uuid", elmt->uuid().toString());
			auto hash = elementInfoToString(elmt);
			for (auto key : hash.keys())
			{
				QString value = hash.value(key);
				QString bind = key.prepend(":");
				m_insert_element_info_query.bindValue(bind, value);
			}

			if (!m_insert_element_info_query.exec()) {
				qDebug() << "projectDataBase::populateElementInfoTable insert error : " << m_insert_element_info_query.lastError();
			}
		}
	}
}

void projectDataBase::populateDiagramInfoTable()
{
	QSqlQuery query(m_data_base);
	query.exec("DELETE FROM diagram_info");

	for (auto *diagram : m_project->diagrams())
	{
		m_insert_diagram_info_query.bindValue(":uuid", diagram->uuid());

		auto infos = diagram->border_and_titleblock.titleblockInformation();
		for (auto key : QETApp::diagramInfoKeys())
		{
			if (key == "date") {
				m_insert_diagram_info_query.bindValue( ":date",
													   QLocale::system().toDate(infos.value("date").toString(),
																				QLocale::ShortFormat));
			} else {
				auto value = infos.value(key);
				auto bind = key.prepend(":");
				m_insert_diagram_info_query.bindValue(bind, value);
			}
		}

		if (!m_insert_diagram_info_query.exec()) {
			qDebug() << "projectDataBase::populateDiagramInfoTable insert error : " << m_insert_diagram_info_query.lastError();
		}
	}
}

void projectDataBase::prepareQuery()
{
		//INSERT DIAGRAM
	m_insert_diagram_query = QSqlQuery(m_data_base);
	m_insert_diagram_query.prepare("INSERT INTO diagram (uuid, pos) VALUES (:uuid, :pos)");

		//REMOVE DIAGRAM
	m_remove_diagram_query = QSqlQuery(m_data_base);
	m_remove_diagram_query.prepare("DELETE FROM diagram WHERE uuid=:uuid");

		//INSERT DIAGRAM INFO
	m_insert_diagram_info_query = QSqlQuery(m_data_base);
	QStringList bind_diag_info_values;
	for (auto key : QETApp::diagramInfoKeys()) {
		bind_diag_info_values << key.prepend(":");
	}
	QString insert_diag_info("INSERT INTO diagram_info (diagram_uuid, " +
				   QETApp::diagramInfoKeys().join(", ") +
				   ") VALUES (:uuid, " +
				   bind_diag_info_values.join(", ") +
				   ")");
	m_insert_diagram_info_query.prepare(insert_diag_info);

		//INSERT ELEMENT
	QString insert_element_query("INSERT INTO element (uuid, diagram_uuid, pos, type, sub_type) VALUES (:uuid, :diagram_uuid, :pos, :type, :sub_type)");
	m_insert_elements_query = QSqlQuery(m_data_base);
	m_insert_elements_query.prepare(insert_element_query);


		//INSERT ELEMENT INFO
	QStringList bind_values;
	for (auto key : QETApp::elementInfoKeys()) {
		bind_values << key.prepend(":");
	}
	QString insert_element_info("INSERT INTO element_info (element_uuid," +
				   QETApp::elementInfoKeys().join(", ") +
				   ") VALUES (:uuid," +
				   bind_values.join(", ") +
				   ")");
	m_insert_element_info_query = QSqlQuery(m_data_base);
	m_insert_element_info_query.prepare(insert_element_info);

		//REMOVE ELEMENT
	QString remove_element("DELETE FROM element WHERE uuid=:uuid");
	m_remove_element_query = QSqlQuery(m_data_base);
	m_remove_element_query.prepare(remove_element);

		//UPDATE ELEMENT INFO
	QString update_str("UPDATE element_info SET ");
	for (auto string : QETApp::elementInfoKeys()) {
		update_str.append(string + " = :" + string + ", ");
	}
	update_str.remove(update_str.length()-2, 2); //Remove the last ", "
	update_str.append(" WHERE element_uuid = :uuid");
	m_update_element_query = QSqlQuery(m_data_base);
	m_update_element_query.prepare(update_str);
}

/**
	@brief projectDataBase::elementInfoToString
	@param elmt
	@return the element information in hash as key for the info name and value as the information value.
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

#ifdef QET_EXPORT_PROJECT_DB
/**
	@brief projectDataBase::sqliteHandle
	@param db
	@return the sqlite3 handler class used internally by db
*/
sqlite3 *projectDataBase::sqliteHandle(QSqlDatabase *db)
{
	sqlite3 *handle = nullptr;

	QVariant v = db->driver()->handle();
	if (v.isValid() && qstrcmp(v.typeName(), "sqlite3*") == 0) {
		handle = *static_cast<sqlite3 **>(v.data());
	}

	return handle;
}


/**
 * @brief projectDataBase::exportDb
 * Export the db, to a file.
 * @param db : database to export
 * @param parent : parent widget of a QDialog used in this function
 * @param caption : Title of the QDialog used in this function
 * @param dir : Default directory where the database must be saved.
 */
void projectDataBase::exportDb(projectDataBase *db,
			       QWidget *parent,
			       const QString &caption,
			       const QString &dir)
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

	QString connection_name("export_project_db_" + db->project()->uuid().toString());

	if (true) //Enter in a scope only to nicely use QSqlDatabase::removeDatabase just after the end of the scope
	{
		auto file_db = QSqlDatabase::addDatabase("QSQLITE", connection_name);
		file_db.setDatabaseName(path_);
		if (!file_db.open()) {
			return;
		}

		auto memory_db_handle = sqliteHandle(&db->m_data_base);
		auto file_db_handle = sqliteHandle(&file_db);

		auto sqlite_backup = sqlite3_backup_init(file_db_handle, "main", memory_db_handle, "main");
		if (sqlite_backup)
		{
			sqlite3_backup_step(sqlite_backup, -1);
			sqlite3_backup_finish(sqlite_backup);
		}
		file_db.close();
	}
	QSqlDatabase::removeDatabase(connection_name);
}
#endif
