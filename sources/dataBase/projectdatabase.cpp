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
#include "projectdatabase.h"

#include "../diagram.h"
#include "../diagramposition.h"
#include "../elementprovider.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../qetproject.h"

#include <QLocale>
#include <QSqlError>

#include <QSqlDriver>
#include <sqlite3.h>


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
	connect(m_project, &QETProject::diagramAdded, [this](QETProject *, Diagram *diagram) {
		this->addDiagram(diagram);
	});
	connect(m_project, &QETProject::diagramRemoved, [this](QETProject *, Diagram *diagram) {
		this->removeDiagram(diagram);
	});
	connect(m_project, &QETProject::projectDiagramsOrderChanged, [this]()
	{
		for (auto diagram : m_project->diagrams())
		{
			m_diagram_order_changed.bindValue(":pos", m_project->folioIndex(diagram)+1);
			m_diagram_order_changed.bindValue(":uuid", diagram->uuid());
			m_diagram_order_changed.exec();


			m_diagram_info_order_changed.bindValue(":folio", diagram->border_and_titleblock.titleblockInformation().value("folio"));
			m_diagram_info_order_changed.bindValue(":uuid", diagram->uuid());
			m_diagram_info_order_changed.exec();

		}
		emit dataBaseUpdated();
	});
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
	populateConductorTable();
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
	@brief projectDataBase::excludedConductorCount
	@return how many conductors of the project are deliberately absent from
	the conductor table because at least one of their terminals has no uuid.

	Counted from the live scene rather than from the database, precisely
	because the database is where these conductors are *not*. See
	addConductor() for why they are omitted: a terminal uuid comes from the
	catalog .elmt definition, so an element whose definition predates that
	field yields terminals with no stable identity to key on.

	This is what lets a caller tell the user "N wires are missing and here
	is why", instead of silently presenting a short list as if it were
	complete.
*/
int projectDataBase::excludedConductorCount() const
{
	if (!m_project) {
		return 0;
	}

	int count = 0;
	for (auto *diagram : m_project->diagrams())
	{
		const auto conductor_list = diagram->conductors();
		for (auto *conductor : conductor_list)
		{
			if (conductor->terminal1->uuid().isNull()
				|| conductor->terminal2->uuid().isNull()) {
				++count;
			}
		}
	}
	return count;
}

/**
	@brief projectDataBase::addElement
	@param element
*/
void projectDataBase::addElement(Element *element)
{
	if (!element || !element->diagram()) {
		qDebug() << "projectDataBase::addElement: null element or diagram";
		return;
	}

	m_insert_elements_query.bindValue(":uuid", element->uuid().toString());
	m_insert_elements_query.bindValue(":diagram_uuid", element->diagram()->uuid().toString());
	m_insert_elements_query.bindValue(":pos", element->diagram()->convertPosition(element->scenePos()).toString());
	m_insert_elements_query.bindValue(":type", element->elementData().typeToString());
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
	for (auto str : QETInformation::elementInfoKeys()) {
		m_update_element_query.bindValue(":" + str, hash.value(str));
	}
	m_update_element_query.bindValue(":uuid", element->uuid().toString());
	if (!m_update_element_query.exec()) {
		qDebug() << "projectDataBase::elementInfoChanged update error : " << m_update_element_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

void projectDataBase::elementInfoChanged(QList<Element *> elements)
{
	this->blockSignals(true);
		//Block signal for not emit dataBaseUpdated at
		//each call of the method elementInfoChanged(Element *element)

	m_data_base.transaction();	
	for (auto elmt : elements) {
		elementInfoChanged(elmt);
	}
	m_data_base.commit();

	this->blockSignals(false);
	emit dataBaseUpdated();
}

void projectDataBase::addDiagram(Diagram *diagram)
{
	m_insert_diagram_query.bindValue(":uuid", diagram->uuid().toString());
	m_insert_diagram_query.bindValue(":pos", m_project->folioIndex(diagram)+1);
	if(!m_insert_diagram_query.exec()) {
		qDebug() << "projectDataBase::addDiagram insert error : " << m_insert_diagram_query.lastError();
	}

	bindDiagramInfoValues(m_insert_diagram_info_query, diagram);

	if (!m_insert_diagram_info_query.exec()) {
		qDebug() << "projectDataBase::addDiagram insert info error : " << m_insert_diagram_info_query.lastError();
	}

		//The information "folio" of other existing diagram can have the variable %total,
		//so when a new diagram is added this variable change.
		//We need to update this information in the database.
	for (auto diagram : project()->diagrams())
	{
		m_diagram_info_order_changed.bindValue(":folio", diagram->border_and_titleblock.titleblockInformation().value("folio"));
		m_diagram_info_order_changed.bindValue(":uuid", diagram->uuid());
		if (!m_diagram_info_order_changed.exec()) {
			qDebug() << "projectDataBase::addDiagram update diagram infp order error : " << m_diagram_info_order_changed.lastError();
		}
	}
	emit dataBaseUpdated();
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

void projectDataBase::diagramInfoChanged(Diagram *diagram)
{
	bindDiagramInfoValues(m_update_diagram_info_query, diagram);

	if (!m_update_diagram_info_query.exec()) {
		qDebug() << "projectDataBase::diagramInfoChanged update error : " << m_update_diagram_info_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

void projectDataBase::diagramOrderChanged()
{
}

/**
	@brief projectDataBase::addConductor
	@param conductor
*/
void projectDataBase::addConductor(Conductor *conductor)
{
	if (!conductor || !conductor->diagram()) {
		qDebug() << "projectDataBase::addConductor: null conductor or diagram";
		return;
	}

		//A conductor whose terminal(s) predate terminal uuids (legacy
		//elements not yet re-saved by a uuid-aware element editor) can't
		//be given a stable identity here -- omitted the same way
		//element_nomenclature_view already omits exclude_from_bom elements,
		//rather than fabricating one.
	if (conductor->terminal1->uuid().isNull() || conductor->terminal2->uuid().isNull()) {
		return;
	}

	insertTerminal(conductor->terminal1);
	insertTerminal(conductor->terminal2);

	m_insert_conductor_query.bindValue(":uuid", conductor->uuid().toString());
	m_insert_conductor_query.bindValue(":diagram_uuid", conductor->diagram()->uuid().toString());
	m_insert_conductor_query.bindValue(":terminal1_uuid", conductor->terminal1->uuid().toString());
	m_insert_conductor_query.bindValue(":terminal1_element_uuid", conductor->terminal1->parentElement()->uuid().toString());
	m_insert_conductor_query.bindValue(":terminal2_uuid", conductor->terminal2->uuid().toString());
	m_insert_conductor_query.bindValue(":terminal2_element_uuid", conductor->terminal2->parentElement()->uuid().toString());
	m_insert_conductor_query.bindValue(":text", conductor->properties().text);
	if (!m_insert_conductor_query.exec()) {
		qDebug() << "projectDataBase::addConductor insert error : " << m_insert_conductor_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

/**
	@brief projectDataBase::removeConductor
	@param conductor
*/
void projectDataBase::removeConductor(Conductor *conductor)
{
	m_remove_conductor_query.bindValue(":uuid", conductor->uuid().toString());
	if (!m_remove_conductor_query.exec()) {
		qDebug() << "projectDataBase::removeConductor delete error : " << m_remove_conductor_query.lastError();
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

	QSqlQuery(m_data_base).exec("PRAGMA temp_store = MEMORY");
	QSqlQuery(m_data_base).exec("PRAGMA journal_mode = MEMORY");
	QSqlQuery(m_data_base).exec("PRAGMA synchronous = OFF");
	
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
	for (auto string : QETInformation::diagramInfoKeys())
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
	for (auto string : QETInformation::elementInfoKeys())
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

	//Create the terminal table.
	//Terminal::uuid() is the terminal-position id baked into the catalog
	//.elmt definition (e.g. "the top terminal") -- identical across every
	//placed instance of that catalog element, not a per-instance id. A
	//terminal instance is only uniquely identified by (uuid, element_uuid)
	//together, so that pair is the primary key here, not uuid alone.
	QString terminal_table("CREATE TABLE terminal"
						  "( "
						  "uuid VARCHAR(50) NOT NULL, "
						  "element_uuid VARCHAR(50) NOT NULL,"
						  "name VARCHAR(50),"
						  "PRIMARY KEY (uuid, element_uuid),"
						  "FOREIGN KEY (element_uuid) REFERENCES element (uuid)"
						  ")");
	if (!query_.exec(terminal_table)) {
		qDebug() << "terminal_table query : "<< query_.lastError();
	}

	//Create the conductor table
	QString conductor_table("CREATE TABLE conductor"
						  "( "
						  "uuid VARCHAR(50) PRIMARY KEY NOT NULL, "
						  "diagram_uuid VARCHAR(50) NOT NULL,"
						  "terminal1_uuid VARCHAR(50) NOT NULL,"
						  "terminal1_element_uuid VARCHAR(50) NOT NULL,"
						  "terminal2_uuid VARCHAR(50) NOT NULL,"
						  "terminal2_element_uuid VARCHAR(50) NOT NULL,"
						  "text VARCHAR(100),"
						  "FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid),"
						  "FOREIGN KEY (terminal1_uuid, terminal1_element_uuid) REFERENCES terminal (uuid, element_uuid),"
						  "FOREIGN KEY (terminal2_uuid, terminal2_element_uuid) REFERENCES terminal (uuid, element_uuid)"
						  ")");
	if (!query_.exec(conductor_table)) {
		qDebug() << "conductor_table query : "<< query_.lastError();
	}

	createElementNomenclatureView();
	createSummaryView();
	createWiringListView();
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
						 "ei.description AS description,"
						 "ei.designation AS designation,"
						 "ei.manufacturer AS manufacturer,"
						 "ei.manufacturer_reference AS manufacturer_reference,"
						 "ei.machine_manufacturer_reference AS machine_manufacturer_reference,"
						 "ei.supplier AS supplier,"
						 "ei.quantity AS quantity,"
						 "ei.unity AS unity,"
						 "ei.auxiliary1 AS auxiliary1,"
						 "ei.description_auxiliary1 AS description_auxiliary1,"
						 "ei.designation_auxiliary1 AS designation_auxiliary1,"
						 "ei.manufacturer_auxiliary1 AS manufacturer_auxiliary1,"
						 "ei.manufacturer_reference_auxiliary1 AS manufacturer_reference_auxiliary1,"
						 "ei.machine_manufacturer_reference_auxiliary1 AS machine_manufacturer_reference_auxiliary1,"
						 "ei.supplier_auxiliary1 AS supplier_auxiliary1,"
						 "ei.quantity_auxiliary1 AS quantity_auxiliary1,"
						 "ei.unity_auxiliary1 AS unity_auxiliary1,"
						 
						 "ei.auxiliary2 AS auxiliary2,"
						 "ei.description_auxiliary2 AS description_auxiliary2,"
						 "ei.designation_auxiliary2 AS designation_auxiliary2,"
						 "ei.manufacturer_auxiliary2 AS manufacturer_auxiliary2,"
						 "ei.manufacturer_reference_auxiliary2 AS manufacturer_reference_auxiliary2,"
						 "ei.machine_manufacturer_reference_auxiliary2 AS machine_manufacturer_reference_auxiliary2,"
						 "ei.supplier_auxiliary2 AS supplier_auxiliary2,"
						 "ei.quantity_auxiliary2 AS quantity_auxiliary2,"
						 "ei.unity_auxiliary2 AS unity_auxiliary2,"
						 
						 "ei.auxiliary3 AS auxiliary3,"
						 "ei.description_auxiliary3 AS description_auxiliary3,"
						 "ei.designation_auxiliary3 AS designation_auxiliary3,"
						 "ei.manufacturer_auxiliary3 AS manufacturer_auxiliary3,"
						 "ei.manufacturer_reference_auxiliary3 AS manufacturer_reference_auxiliary3,"
						 "ei.machine_manufacturer_reference_auxiliary3 AS machine_manufacturer_reference_auxiliary3,"
						 "ei.supplier_auxiliary3 AS supplier_auxiliary3,"
						 "ei.quantity_auxiliary3 AS quantity_auxiliary3,"
						 "ei.unity_auxiliary3 AS unity_auxiliary3,"
						 
						 "ei.auxiliary4 AS auxiliary4,"
						 "ei.description_auxiliary4 AS description_auxiliary4,"
						 "ei.designation_auxiliary4 AS designation_auxiliary4,"
						 "ei.manufacturer_auxiliary4 AS manufacturer_auxiliary4,"
						 "ei.manufacturer_reference_auxiliary4 AS manufacturer_reference_auxiliary4,"
						 "ei.machine_manufacturer_reference_auxiliary4 AS machine_manufacturer_reference_auxiliary4,"
						 "ei.supplier_auxiliary4 AS supplier_auxiliary4,"
						 "ei.quantity_auxiliary4 AS quantity_auxiliary4,"
						 "ei.unity_auxiliary4 AS unity_auxiliary4,"
					 "ei.exclude_from_bom AS exclude_from_bom,"
					 
					 "ei.plc_type AS plc_type,"
					 "ei.plc_address AS plc_address,"
					 "ei.plc_function AS plc_function,"
					 "ei.plc_comment AS plc_comment,"
					 "ei.plc_crossref AS plc_crossref,"
					
					 "d.pos AS diagram_position,"
						 "e.type AS element_type,"
						 "e.sub_type AS element_sub_type,"
						 "di.title AS title,"
						 "di.folio AS folio,"
						 "e.pos AS position "
						 " FROM element_info ei, diagram_info di, element e, diagram d"
						 " WHERE ei.element_uuid = e.uuid AND e.diagram_uuid = d.uuid AND di.diagram_uuid = d.uuid AND (ei.exclude_from_bom IS NOT 'true')");

	QSqlQuery query(m_data_base);
	if (!query.exec(create_view)) {
		qDebug() << query.lastError();
	}
	
	QSqlQuery query_version{m_data_base};
	query_version.exec("select sqlite_version();");
	query_version.next();
	QString version = query_version.value("sqlite_version()").toString();
	query_version.finish();
	
	qInfo() << "SQLite version: " << version;
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

/**
	@brief projectDataBase::createWiringListView
	A from-to wiring list: one row per conductor, each endpoint resolved to
	its element label and terminal name.

	Two deliberate differences from an ordinary inner-join view like
	element_nomenclature_view:

	- No join to the element table. A terminal row already carries its
	  element_uuid, so joining element back just to read the same uuid adds
	  nothing -- and would actively drop rows, because populateElementTable()
	  only inserts elements matching Simple|Terminal|Master|Thumbnail. Slave
	  elements (relay contacts and the like, extremely common at the end of a
	  wire) and report elements are absent from that table after a project
	  load, so an inner join through it silently loses their conductors.
	- element_info is LEFT joined for the same reason. A wire whose endpoint
	  element carries no info row still belongs in a wiring list; it comes
	  back with an empty label rather than vanishing. Losing a wire from a
	  wiring list is a worse failure than showing one with a blank end.

	The result is that this view returns exactly as many rows as the
	conductor table holds -- what is already excluded upstream (conductors
	on legacy terminals without uuids) stays excluded, and nothing new is
	dropped here.
*/
void projectDataBase::createWiringListView()
{
	QString create_view ("CREATE VIEW wiring_list_view AS SELECT "
						 "c.uuid AS conductor_uuid,"
						 "c.text AS wire_number,"
						 "t1.element_uuid AS from_element_uuid,"
						 "ei1.label AS from_element_label,"
						 "t1.name AS from_terminal,"
						 "t2.element_uuid AS to_element_uuid,"
						 "ei2.label AS to_element_label,"
						 "t2.name AS to_terminal,"
						 "d.pos AS diagram_position"
						 " FROM conductor c"
						 " JOIN terminal t1 ON c.terminal1_uuid = t1.uuid AND c.terminal1_element_uuid = t1.element_uuid"
						 " JOIN terminal t2 ON c.terminal2_uuid = t2.uuid AND c.terminal2_element_uuid = t2.element_uuid"
						 " LEFT JOIN element_info ei1 ON t1.element_uuid = ei1.element_uuid"
						 " LEFT JOIN element_info ei2 ON t2.element_uuid = ei2.element_uuid"
						 " JOIN diagram d ON c.diagram_uuid = d.uuid");

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
		m_insert_diagram_query.bindValue(":pos", m_project->folioIndex(diagram)+1);
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
		const ElementProvider ep(diagram);
		const auto elmt_vector = ep.find(ElementData::Simple | ElementData::Terminal | ElementData::Master | ElementData::Thumbnail);
			//Insert all values into the database
		for (const auto &elmt : elmt_vector)
		{
			const auto elmt_data = elmt->elementData();
			m_insert_elements_query.bindValue(":uuid", elmt->uuid().toString());
			m_insert_elements_query.bindValue(":diagram_uuid", diagram->uuid().toString());
			m_insert_elements_query.bindValue(":pos", diagram->convertPosition(elmt->scenePos()).toString());
			m_insert_elements_query.bindValue(":type", elmt_data.typeToString());
			m_insert_elements_query.bindValue(":sub_type", elmt_data.masterTypeToString());
			if (!m_insert_elements_query.exec()) {
				qDebug() << "projectDataBase::populateElementTable insert error : " << m_insert_elements_query.lastError();
			}
		}
	}
}

/**
	@brief projectDataBase::populateElementInfoTable
	Populate the element info table
*/
void projectDataBase::populateElementInfoTable()
{
	QSqlQuery query(m_data_base);
	query.exec(QStringLiteral("DELETE FROM element_info"));

	for (const auto &diagram : m_project->diagrams())
	{
		const ElementProvider ep(diagram);
		const auto elmt_vector = ep.find(ElementData::Simple | ElementData::Terminal | ElementData::Master | ElementData::Thumbnail);

			//Insert all values into the database
		for (const auto &elmt : elmt_vector)
		{
			m_insert_element_info_query.bindValue(QStringLiteral(":uuid"), elmt->uuid().toString());
			const auto hash = elementInfoToString(elmt);
			for (const auto &key : hash.keys())
			{
				QString value = hash.value(key);
				QString bind = QStringLiteral(":") + key;
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
		bindDiagramInfoValues(m_insert_diagram_info_query, diagram);

		if (!m_insert_diagram_info_query.exec()) {
			qDebug() << "projectDataBase::populateDiagramInfoTable insert error : " << m_insert_diagram_info_query.lastError();
		}
	}
}

/**
	@brief projectDataBase::populateConductorTable
	Populate the terminal and conductor tables. Terminals only matter here
	in the context of a conductor referencing them, so their population is
	folded into this method rather than tracked independently.
*/
void projectDataBase::populateConductorTable()
{
	QSqlQuery query(m_data_base);
	query.exec(QStringLiteral("DELETE FROM conductor"));
	query.exec(QStringLiteral("DELETE FROM terminal"));

	for (auto *diagram : m_project->diagrams())
	{
		const auto conductor_list = diagram->conductors();
		for (auto *conductor : conductor_list)
		{
				//See addConductor() for why terminals without a uuid
				//(legacy elements) are omitted rather than fabricating one.
			if (conductor->terminal1->uuid().isNull() || conductor->terminal2->uuid().isNull()) {
				continue;
			}

			insertTerminal(conductor->terminal1);
			insertTerminal(conductor->terminal2);

			m_insert_conductor_query.bindValue(":uuid", conductor->uuid().toString());
			m_insert_conductor_query.bindValue(":diagram_uuid", diagram->uuid().toString());
			m_insert_conductor_query.bindValue(":terminal1_uuid", conductor->terminal1->uuid().toString());
			m_insert_conductor_query.bindValue(":terminal1_element_uuid", conductor->terminal1->parentElement()->uuid().toString());
			m_insert_conductor_query.bindValue(":terminal2_uuid", conductor->terminal2->uuid().toString());
			m_insert_conductor_query.bindValue(":terminal2_element_uuid", conductor->terminal2->parentElement()->uuid().toString());
			m_insert_conductor_query.bindValue(":text", conductor->properties().text);
			if (!m_insert_conductor_query.exec()) {
				qDebug() << "projectDataBase::populateConductorTable insert error : " << m_insert_conductor_query.lastError();
			}
		}
	}
}

/**
	@brief projectDataBase::insertTerminal
	Insert (or, if already present -- e.g. a junction shared by several
	conductors -- silently keep) @terminal in the terminal table.
	@param terminal
*/
void projectDataBase::insertTerminal(Terminal *terminal)
{
	m_insert_terminal_query.bindValue(":uuid", terminal->uuid().toString());
	m_insert_terminal_query.bindValue(":element_uuid", terminal->parentElement()->uuid().toString());
	m_insert_terminal_query.bindValue(":name", terminal->name());
	if (!m_insert_terminal_query.exec()) {
		qDebug() << "projectDataBase::insertTerminal insert error : " << m_insert_terminal_query.lastError();
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
	for (auto key : QETInformation::diagramInfoKeys()) {
		bind_diag_info_values << key.prepend(":");
	}
	QString insert_diag_info("INSERT INTO diagram_info (diagram_uuid, " +
				   QETInformation::diagramInfoKeys().join(", ") +
				   ") VALUES (:uuid, " +
				   bind_diag_info_values.join(", ") +
				   ")");
	m_insert_diagram_info_query.prepare(insert_diag_info);

		//UPDATE DIAGRAM INFO
	QString update_diagram_str("UPDATE diagram_info SET ");
	for (auto str : QETInformation::diagramInfoKeys()) {
		update_diagram_str.append(str % " = :" % str % ", ");
	}
	update_diagram_str.remove(update_diagram_str.length()-2, 2); //Remove the last ", "
	update_diagram_str.append(" WHERE diagram_uuid = :uuid");
	m_update_diagram_info_query = QSqlQuery(m_data_base);
	m_update_diagram_info_query.prepare(update_diagram_str);

		//UPDATE DIAGRAM ORDER
	m_diagram_order_changed = QSqlQuery(m_data_base);
	m_diagram_order_changed.prepare("UPDATE diagram SET pos = :pos WHERE uuid = :uuid");
	m_diagram_info_order_changed = QSqlQuery(m_data_base);
	m_diagram_info_order_changed.prepare("UPDATE diagram_info SET folio = :folio WHERE diagram_uuid = :uuid");

		//INSERT ELEMENT
	QString insert_element_query("INSERT INTO element (uuid, diagram_uuid, pos, type, sub_type) VALUES (:uuid, :diagram_uuid, :pos, :type, :sub_type)");
	m_insert_elements_query = QSqlQuery(m_data_base);
	m_insert_elements_query.prepare(insert_element_query);


		//INSERT ELEMENT INFO
	QStringList bind_values;
	for (auto key : QETInformation::elementInfoKeys()) {
		bind_values << key.prepend(":");
	}
	QString insert_element_info("INSERT INTO element_info (element_uuid," +
				   QETInformation::elementInfoKeys().join(", ") +
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
	for (auto string : QETInformation::elementInfoKeys()) {
		update_str.append(string % " = :" % string % ", ");
	}
	update_str.remove(update_str.length()-2, 2); //Remove the last ", "
	update_str.append(" WHERE element_uuid = :uuid");
	m_update_element_query = QSqlQuery(m_data_base);
	m_update_element_query.prepare(update_str);

		//INSERT TERMINAL
	m_insert_terminal_query = QSqlQuery(m_data_base);
	m_insert_terminal_query.prepare("INSERT OR IGNORE INTO terminal (uuid, element_uuid, name) VALUES (:uuid, :element_uuid, :name)");

		//INSERT CONDUCTOR
	m_insert_conductor_query = QSqlQuery(m_data_base);
	m_insert_conductor_query.prepare("INSERT INTO conductor (uuid, diagram_uuid, terminal1_uuid, terminal1_element_uuid, terminal2_uuid, terminal2_element_uuid, text) "
					  "VALUES (:uuid, :diagram_uuid, :terminal1_uuid, :terminal1_element_uuid, :terminal2_uuid, :terminal2_element_uuid, :text)");

		//REMOVE CONDUCTOR
	m_remove_conductor_query = QSqlQuery(m_data_base);
	m_remove_conductor_query.prepare("DELETE FROM conductor WHERE uuid=:uuid");
}

/**
	@brief projectDataBase::elementInfoToString
	@param elmt
	@return the element information in hash as key for the info name and value as the information value.
*/
QHash<QString, QString> projectDataBase::elementInfoToString(Element *elmt)
{
	QHash<QString, QString> hash; //Store the value for each columns
	for (auto key : QETInformation::elementInfoKeys())
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

void projectDataBase::bindDiagramInfoValues(QSqlQuery &query, Diagram *diagram)
{
	query.bindValue(":uuid", diagram->uuid());

	auto infos = diagram->border_and_titleblock.titleblockInformation();
	for (auto key : QETInformation::diagramInfoKeys())
	{
		if (key == "date") {
			query.bindValue( ":date",
							 QLocale::system().toDate(infos.value("date").toString(),
													  QLocale::ShortFormat));
		} else {
			auto value = infos.value(key);
			auto bind = key.prepend(":");
			query.bindValue(bind, value);
		}
	}
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
			dir_ = QETApp::documentDir() % "/" % tr("sans_nom") % ".sqlite";
		} else {
			dir_.remove(".qet");
			dir_.append(".sqlite");
		}
	}

	auto path_ = QFileDialog::getSaveFileName(parent, caption_, dir_, "*.sqlite");
	if (path_.isNull()) {
		return;
	}

	QString connection_name("export_project_db_" % db->project()->uuid().toString());

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
