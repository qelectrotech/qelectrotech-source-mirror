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

#include "sqlreadonly.h"

#include "../autoNum/assignvariables.h"
#include "../borderproperties.h"
#include "../bordertitleblock.h"
#include "../diagram.h"
#include "../diagramposition.h"
#include "../elementprovider.h"
#include "../itemgroups.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../qet.h"
#include "../titleblockproperties.h"
#include "../ElementsCollection/xmlelementcollection.h"
#include "../properties/elementdata.h"

#include <QLocale>
#include <QDate>
#include <QDomDocument>

#include <functional>
#include <QMetaEnum>
#include <QTextDocument>
#include <QFile>
#include <QRegularExpression>
#include <QSqlDriver>
#include <QSqlError>




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
		m_content_changed = true;
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
		//A bulk operation is in progress and updates the database itself once
		//it is done : rebuilding now would only be thrown away by that final
		//rebuild. @see setUpdateBlocked().
	if (m_update_blocked) {
		return;
	}

		//Nothing in the project has changed since the last rebuild, so
		//repopulating would insert exactly the rows that are already there.
		//The signal is still emitted : callers and models rely on it to
		//refresh, and what they read back is unchanged either way.
	if (!m_content_changed)
	{
		flushDrawingItems();
		emit dataBaseUpdated();
		return;
	}

	populateDiagramTable();
	populateDiagramInfoTable();
	populateElementTable();
	populateElementInfoTable();
	populateConductorTable();
	populateDrawingItemTables();
	m_content_changed = false;

	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::updateDB
	updateDB() for a project just read from @p document.

	The diagram, diagram_info, element, element_info, terminal and conductor
	tables are filled from the document itself when it carries everything
	they need -- see populateFromDocument() -- and from the built folios
	otherwise, as updateDB() does. Shapes, texts and pictures always come from
	the built folios: their boxes need the fonts and pens a folio renders with.

	The two fills give the same tables (tst_databasefromdocument checks it on
	the shipped examples). Reading the document instead of the folios is what
	the database needs before a project can be opened without building every
	folio; see DB-FROM-XML-SCOPE.md in qelectrotech-docker.
*/
void projectDataBase::updateDB(const QDomDocument &document)
{
	if (m_update_blocked || !m_content_changed) {
		updateDB();
		return;
	}
		//QET_DATABASE_FROM_FOLIOS=1 keeps the fill from the built folios:
		//a way back should the two ever disagree, and what the test that
		//says they do not compares against.
	QString why;
	if (qEnvironmentVariableIntValue("QET_DATABASE_FROM_FOLIOS") == 1) {
		why = QStringLiteral("QET_DATABASE_FROM_FOLIOS is set");
	} else if (populateFromDocument(document, &why)) {
		qInfo() << "Project database filled from the document";
		populateDrawingItemTables();
		m_content_changed = false;
		emit dataBaseUpdated();
		return;
	}
	qInfo().noquote() << "Project database filled from the folios:" << why;
	updateDB();
}

namespace {
struct DocumentTerminal
{
	QString uuid;
	QString name;
	bool master_label = false;
	QVariant index;
};

	//The index each of @p points has in Element::terminals() -- the index
	//the scripting API's addConductor() and conductor calls take -- or a
	//null QVariant for a point another terminal shares. Element::
	//parseTerminal() sorts the list top to bottom, then left to right, on
	//each terminal's position in its definition, and the sort is not
	//stable, so which of two terminals at the same point comes first is
	//not defined: no index is given rather than one that can change.
QList<QVariant> terminalIndexes(const QList<QPointF> &points)
{
	QList<int> order;
	for (int i = 0 ; i < points.size() ; ++i) {
		order << i;
	}
	std::stable_sort(order.begin(), order.end(), [&points](int a, int b) {
		if (points.at(a).y() == points.at(b).y()) {
			return points.at(a).x() < points.at(b).x();
		}
		return points.at(a).y() < points.at(b).y();
	});
	QList<QVariant> indexes(points.size());
	for (int i = 0 ; i < order.size() ; ++i) {
		if (points.count(points.at(order.at(i))) == 1) {
			indexes[order.at(i)] = i;
		}
	}
	return indexes;
}

struct DocumentDefinition
{
	QString type;
	QString sub_type;
	QHash<QUuid, DocumentTerminal> terminals;
};

struct DocumentElement
{
	QString uuid;
	QString diagram_uuid;
	QString pos;
	QString type;
	QString sub_type;
	QVariant group;
	DiagramContext informations;
	QString label;
	QHash<QUuid, DocumentTerminal> terminals;
};

	//The sequential values an element or a conductor was saved with, as
	//Element::fromXml() and Conductor::fromXml() read them -- false for
	//the attributes files written before <sequentialNumbers> carry.
bool readSequence(const QDomElement &item, autonum::sequentialNumbers *sequence)
{
	for (const char *name : {"sequ_1", "sequf_1", "seqt_1", "seqtf_1", "seqh_1"}) {
		if (item.hasAttribute(QLatin1String(name))) {
			return false;
		}
	}
	sequence->fromXml(item.firstChildElement(QStringLiteral("sequentialNumbers")));
	return true;
}

struct DocumentConductor
{
	QString uuid;
	QString diagram_uuid;
	QString element1, terminal1, element2, terminal2;
	QString text;
};
}

/**
	@brief projectDataBase::populateFromDocument
	Fill the diagram, diagram_info, element, element_info, terminal and
	conductor tables from @p document, the project as read from its file,
	with the same values the built folios give -- using the same code:
	a BorderTitleBlock read from each folio's XML gives the title-block
	values and each element's grid cell, the project's embedded collection
	gives each element's definition, and the binders are shared.

	Nothing is written unless the whole document can be read this way. It
	cannot -- and false is returned, for the caller to fill from the folios
	instead -- when a folio, element or conductor carries no saved uuid (the
	folios derive one on load), a conductor names its ends the older way,
	a folio number uses %autonum, a conductor ends on a terminal that shows
	its master's contact label, an element's definition is missing or
	not one the folios could build, two elements on a folio number their
	terminals alike, or sequential numbers are saved as the attributes
	older files carry. A file saved by a current QElectroTech carries
	everything else.

	A label or a conductor text made from a formula is worked out again,
	as the folios do, with the same AssignVariables code: the one saved
	in the file is what the formula gave when it was saved, and a folio
	added or moved since changes it. A frozen conductor text is left to the
	folios, which work it out part-way through loading.
	@return true if the tables were filled
*/
bool projectDataBase::populateFromDocument(const QDomDocument &document, QString *why)
{
	auto refuse = [why](const QString &reason) {
		if (why) *why = reason;
		return false;
	};
	if (!m_project) {
		return refuse(QStringLiteral("no project"));
	}

		//The folios, in the order the project read them (readDiagramsXml()),
		//each with its saved uuid -- the one the built folio has.
	const QDomNodeList diagram_nodes = document.elementsByTagName(QStringLiteral("diagram"));
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (diagram_nodes.size() != diagrams.size()) {
		return refuse(QStringLiteral("the folios are not the document's"));
	}

	const DiagramContext project_wide = m_project->projectWideProperties();
	const DiagramContext project_properties = m_project->projectProperties();
		//One border and title block read from each folio's XML in turn:
		//only what it gives is kept.
	BorderTitleBlock reader;
	QList<DiagramContext> diagram_infos;
	QList<QDate> diagram_dates;
	QList<QUuid> diagram_uuids;
	QList<DocumentElement> elements;
	QList<DocumentConductor> conductors;
	QSet<QUuid> element_uuids;
	QHash<QString, DocumentDefinition> definitions;   //by element type

		//The project's own definitions by "embed://" path, which is how an
		//element's type names them: one walk of the embedded collection
		//rather than resolving an ElementsLocation per type.
	QHash<QString, QDomElement> stored;
	std::function<void (const QDomElement &, const QString &)> walk =
			[&stored, &walk](const QDomElement &category, const QString &path) {
		for (QDomElement child = category.firstChildElement() ;
			 !child.isNull() ; child = child.nextSiblingElement()) {
			const QString name = path + child.attribute(QStringLiteral("name"));
			if (child.tagName() == QLatin1String("category")) {
				walk(child, name + QLatin1Char('/'));
			} else if (child.tagName() == QLatin1String("element")) {
				stored.insert(name, child.firstChildElement(QStringLiteral("definition")));
			}
		}
	};
	if (auto collection = m_project->embeddedElementCollection()) {
		walk(collection->root(), QStringLiteral("embed://"));
	}

	for (int i = 0 ; i < diagram_nodes.size() ; ++i)
	{
		const QDomElement diagram_xml = diagram_nodes.at(i).toElement();
		const QUuid diagram_uuid(diagram_xml.attribute(QStringLiteral("uuid")));
		if (diagram_uuid.isNull() || diagram_uuid != diagrams.at(i)->uuid()) {
			return refuse(QStringLiteral("a folio has no saved uuid"));
		}

		BorderTitleBlock *border = &reader;
			//As a new Diagram's border starts, before initFromXml() reads it
		border->importBorder(BorderProperties());
		border->importTitleBlock(TitleBlockProperties());
		border->titleBlockFromXml(diagram_xml);
		border->borderFromXml(diagram_xml);
		if (border->folio().contains(QStringLiteral("%autonum"))) {
			return refuse(QStringLiteral("a folio number uses %autonum"));
		}
		border->setFolioData(i + 1, int(diagram_nodes.size()), QString(), project_wide);

			//What a formula on this folio is worked out from, as
			//AssignVariables::formulaToLabel() reads it off a built folio.
		autonum::FormulaContext folio_context;
		folio_context.folio = border->folio();
		folio_context.folio_index = i;
		folio_context.folio_total = border->folioTotal();
		folio_context.plant = border->plant();
		folio_context.locmach = border->locmach();
		folio_context.title_block_fields = border->additionalFields();
		folio_context.project_properties = project_properties;

		QHash<QUuid, int> on_this_folio;   //element uuid -> index in elements
		QSet<int> terminal_ids;            //the older terminal ids used so far
		for (QDomElement element_xml : QET::findInDomElement(
				 diagram_xml, QStringLiteral("elements"), QStringLiteral("element")))
		{
				//Skipped by Diagram::fromXml() as well
			if (!Element::valideXml(element_xml)) {
				continue;
			}
			const QUuid uuid(element_xml.attribute(QStringLiteral("uuid")));
			if (uuid.isNull() || element_uuids.contains(uuid)) {
				return refuse(QStringLiteral("an element has no saved uuid, or shares one"));
			}
			element_uuids.insert(uuid);

				//Element::fromXml() refuses an element whose terminals are
				//numbered like those of one read before it on the folio;
				//which of them it matches needs their geometry, so leave any
				//such folio to the folios. A current file numbers them
				//across the folio.
			QSet<int> ids;
			for (QDomElement t : QET::findInDomElement(
					 element_xml, QStringLiteral("terminals"), QStringLiteral("terminal"))) {
				if (Terminal::valideXml(t)) {
					ids.insert(t.attribute(QStringLiteral("id")).toInt());
				}
			}
			if (terminal_ids.intersects(ids)) {
				return refuse(QStringLiteral("two elements on a folio number their terminals alike"));
			}
			terminal_ids.unite(ids);

				//The definition the folio builds the element from, with the
				//checks Element::buildFromXml() refuses an element on --
				//read once per type, as many elements share one.
			const QString type = element_xml.attribute(QStringLiteral("type"));
			auto known = definitions.constFind(type);
			if (known == definitions.constEnd())
			{
				const QDomElement definition = stored.value(type);
				if (definition.isNull()) {
					return refuse(QStringLiteral("an element's definition is not in the project"));
				}
				int number;
				if (definition.tagName() != QLatin1String("definition")
					|| definition.attribute(QStringLiteral("type")) != QLatin1String("element")
					|| !QET::attributeIsAnInteger(definition, QStringLiteral("width"), &number)
					|| !QET::attributeIsAnInteger(definition, QStringLiteral("height"), &number)
					|| !QET::attributeIsAnInteger(definition, QStringLiteral("hotspot_x"), &number)
					|| !QET::attributeIsAnInteger(definition, QStringLiteral("hotspot_y"), &number)
					|| definition.firstChild().isNull()) {
					return refuse(QStringLiteral("an element's definition cannot be built"));
				}
				DocumentDefinition read;
				ElementData data;
				data.fromXml(definition);
				read.type = data.typeToString();
				read.sub_type = data.masterTypeToString();
					//Every terminal the element is built with counts towards
					//the indexes, as Element::parseTerminal() keeps every one
					//whose position reads, in every <description>.
				QList<QDomElement> parsed;
				QList<QPointF> points;
				for (QDomElement d = definition.firstChildElement(QStringLiteral("description")) ;
					 !d.isNull() ; d = d.nextSiblingElement(QStringLiteral("description"))) {
					for (QDomElement t = d.firstChildElement(QStringLiteral("terminal")) ;
						 !t.isNull() ; t = t.nextSiblingElement(QStringLiteral("terminal")))
					{
						qreal x, y;
						if (QET::attributeIsAReal(t, QStringLiteral("x"), &x)
							&& QET::attributeIsAReal(t, QStringLiteral("y"), &y)) {
							parsed << t;
							points << QPointF(x, y);
						}
					}
				}
				const QList<QVariant> indexes = terminalIndexes(points);
				const QDomElement description = definition.firstChildElement(QStringLiteral("description"));
				for (QDomElement t = description.firstChildElement(QStringLiteral("terminal")) ;
					 !t.isNull() ; t = t.nextSiblingElement(QStringLiteral("terminal")))
				{
					const QUuid terminal_uuid(t.attribute(QStringLiteral("uuid")));
					if (terminal_uuid.isNull()) {
						continue;
					}
					DocumentTerminal terminal;
					terminal.uuid = terminal_uuid.toString();
					terminal.name = t.attribute(QStringLiteral("name"));
					terminal.master_label = t.attribute(QStringLiteral("use_master_label")) == QLatin1String("true");
					const int place = parsed.indexOf(t);
					if (place >= 0) {
						terminal.index = indexes.at(place);
					}
					read.terminals.insert(terminal_uuid, terminal);
				}
				known = definitions.insert(type, read);
			}

			DocumentElement element;
			element.uuid = uuid.toString();
			element.diagram_uuid = diagram_uuid.toString();
			DiagramPosition position = border->convertPosition(
						QPointF(element_xml.attribute(QStringLiteral("x")).toDouble(),
								element_xml.attribute(QStringLiteral("y")).toDouble()));
			element.pos = position.toString();
			element.type = known->type;
			element.sub_type = known->sub_type;
			element.terminals = known->terminals;
			const QUuid group = ItemGroups::read(element_xml);
			element.group = group.isNull() ? QVariant() : QVariant(group.toString());
			element.informations.fromXml(
						element_xml.firstChildElement(QStringLiteral("elementInformations")),
						QStringLiteral("elementInformation"));
				//Element::actualLabel()
			const QString formula = element.informations.value(QStringLiteral("formula")).toString();
			if (formula.isEmpty()) {
				element.label = element.informations.value(QStringLiteral("label")).toString();
			} else {
				autonum::sequentialNumbers sequence;
				if (!readSequence(element_xml, &sequence)) {
					return refuse(QStringLiteral("an element's sequential numbers are saved the older way"));
				}
				autonum::FormulaContext context = folio_context;
				context.has_element = true;
				context.element_position = position;
				context.element_prefix = element_xml.attribute(QStringLiteral("prefix"));
				element.label = autonum::AssignVariables::formulaToLabel(formula, sequence, context);
			}

			on_this_folio.insert(uuid, int(elements.size()));
			elements << element;
		}

			//The conductors the folio builds, and only those: an end that is
			//not found, a conductor from a terminal to itself and a second
			//conductor between the same two terminals are all dropped on load.
		QSet<QString> joined;
		for (QDomElement conductor_xml : QET::findInDomElement(
				 diagram_xml, QStringLiteral("conductors"), QStringLiteral("conductor")))
		{
			if (!Conductor::valideXml(conductor_xml)) {
				continue;
			}
			const QUuid uuid(conductor_xml.attribute(QStringLiteral("uuid")));
			if (uuid.isNull()
				|| !conductor_xml.hasAttribute(QStringLiteral("element1"))
				|| !conductor_xml.hasAttribute(QStringLiteral("element2"))) {
				return refuse(QStringLiteral("a conductor has no saved uuid, or names its ends the older way"));
			}
			QString ends[2][2];
			bool found = true;
			for (int n = 0 ; n < 2 ; ++n)
			{
				const QString index = QString::number(n + 1);
				const QUuid owner(conductor_xml.attribute(QStringLiteral("element") + index));
				const QUuid terminal(conductor_xml.attribute(QStringLiteral("terminal") + index));
				const int e = on_this_folio.value(owner, -1);
				if (e < 0) {
					found = false;
					break;
				}
				if (!elements.at(e).terminals.contains(terminal)) {
						//The folio would try the terminal's derived uuid;
						//leave that to it.
					return refuse(QStringLiteral("a conductor ends on a terminal not in its element's definition"));
				}
				const DocumentTerminal &t = elements.at(e).terminals.value(terminal);
				if (t.master_label) {
					return refuse(QStringLiteral("a conductor ends on a terminal showing its master's label"));
				}
				ends[n][0] = elements.at(e).uuid;
				ends[n][1] = t.uuid;
			}
			if (!found) {
				continue;
			}
			const QString a = ends[0][0] + ends[0][1], b = ends[1][0] + ends[1][1];
			if (a == b) {
				continue;
			}
			const QString pair = a < b ? a + b : b + a;
			if (joined.contains(pair)) {
				continue;
			}
			joined.insert(pair);

			DocumentConductor conductor;
			conductor.uuid = uuid.toString();
			conductor.diagram_uuid = diagram_uuid.toString();
			conductor.element1 = ends[0][0];
			conductor.terminal1 = ends[0][1];
			conductor.element2 = ends[1][0];
			conductor.terminal2 = ends[1][1];
				//ConductorProperties::fromXml()'s text, without reading the
				//rest of the properties -- or, as Conductor::refreshText()
				//makes it, what its formula gives.
			const QString formula = conductor_xml.attribute(QStringLiteral("formula"));
			if (formula.isEmpty()) {
				conductor.text = conductor_xml.attribute(QStringLiteral("num"));
			} else {
				autonum::sequentialNumbers sequence;
				if (conductor_xml.attribute(QStringLiteral("freezeLabel")) == QLatin1String("true")) {
					return refuse(QStringLiteral("a conductor's text made from a formula is frozen"));
				}
				if (!readSequence(conductor_xml, &sequence)) {
					return refuse(QStringLiteral("a conductor's sequential numbers are saved the older way"));
				}
				autonum::FormulaContext context = folio_context;
				context.has_conductor = true;
				context.wire_function = conductor_xml.attribute(QStringLiteral("function"));
				context.wire_tension_protocol = conductor_xml.attribute(QStringLiteral("tension_protocol"));
				context.wire_color = conductor_xml.attribute(QStringLiteral("conductor_color"));
				context.wire_section = conductor_xml.attribute(QStringLiteral("conductor_section"));
				conductor.text = autonum::AssignVariables::formulaToLabel(formula, sequence, context);
			}
			conductors << conductor;
		}

		diagram_uuids << diagram_uuid;
		diagram_infos << border->titleblockInformation();
		diagram_dates << border->date();
	}

		//Everything could be read: write it.
	QSqlQuery query(m_data_base);
	for (const QString &table : {QStringLiteral("diagram"), QStringLiteral("diagram_info"),
								 QStringLiteral("element"), QStringLiteral("element_info"),
								 QStringLiteral("conductor"), QStringLiteral("terminal")}) {
		query.exec(QStringLiteral("DELETE FROM ") + table);
	}

	for (int i = 0 ; i < diagram_uuids.size() ; ++i)
	{
		m_insert_diagram_query.bindValue(":uuid", diagram_uuids.at(i).toString());
		m_insert_diagram_query.bindValue(":pos", i + 1);
		if (!m_insert_diagram_query.exec()) {
			qDebug() << "projectDataBase::populateFromDocument diagram insert error : " << m_insert_diagram_query.lastError();
		}
		bindDiagramInfoValues(m_insert_diagram_info_query, diagram_uuids.at(i),
							  diagram_infos.at(i), diagram_dates.at(i));
		if (!m_insert_diagram_info_query.exec()) {
			qDebug() << "projectDataBase::populateFromDocument diagram_info insert error : " << m_insert_diagram_info_query.lastError();
		}
	}

	for (const DocumentElement &element : std::as_const(elements))
	{
		m_insert_elements_query.bindValue(QStringLiteral(":uuid"), element.uuid);
		m_insert_elements_query.bindValue(QStringLiteral(":diagram_uuid"), element.diagram_uuid);
		m_insert_elements_query.bindValue(QStringLiteral(":pos"), element.pos);
		m_insert_elements_query.bindValue(QStringLiteral(":type"), element.type);
		m_insert_elements_query.bindValue(QStringLiteral(":sub_type"), element.sub_type);
		m_insert_elements_query.bindValue(QStringLiteral(":group_uuid"), element.group);
		if (!m_insert_elements_query.exec()) {
			qDebug() << "projectDataBase::populateFromDocument element insert error : " << m_insert_elements_query.lastError();
		}
		bindElementInfoValues(m_insert_element_info_query, element.uuid, element.informations,
							  element.label);
		if (!m_insert_element_info_query.exec()) {
			qDebug() << "projectDataBase::populateFromDocument element_info insert error : " << m_insert_element_info_query.lastError();
		}
	}

	QHash<QString, int> element_index;
	for (int i = 0 ; i < elements.size() ; ++i) {
		element_index.insert(elements.at(i).uuid, i);
	}
	for (const DocumentConductor &conductor : std::as_const(conductors))
	{
		for (const auto &end : {std::make_pair(conductor.element1, conductor.terminal1),
								std::make_pair(conductor.element2, conductor.terminal2)}) {
			const DocumentElement &owner = elements.at(element_index.value(end.first));
			const DocumentTerminal &terminal = owner.terminals.value(QUuid(end.second));
			insertTerminal(end.second, end.first, terminal.name, terminal.index);
		}
		m_insert_conductor_query.bindValue(QStringLiteral(":uuid"), conductor.uuid);
		m_insert_conductor_query.bindValue(QStringLiteral(":diagram_uuid"), conductor.diagram_uuid);
		m_insert_conductor_query.bindValue(QStringLiteral(":terminal1_uuid"), conductor.terminal1);
		m_insert_conductor_query.bindValue(QStringLiteral(":terminal1_element_uuid"), conductor.element1);
		m_insert_conductor_query.bindValue(QStringLiteral(":terminal2_uuid"), conductor.terminal2);
		m_insert_conductor_query.bindValue(QStringLiteral(":terminal2_element_uuid"), conductor.element2);
		m_insert_conductor_query.bindValue(QStringLiteral(":text"), conductor.text);
		if (!m_insert_conductor_query.exec()) {
			qDebug() << "projectDataBase::populateFromDocument conductor insert error : " << m_insert_conductor_query.lastError();
		}
	}

		//While every folio is still built, their conductors are watched for
		//property changes as populateConductorTable() does -- the one walk
		//over the built folios left here.
	for (Diagram *diagram : diagrams) {
		for (Conductor *conductor : diagram->conductors()) {
			if (conductor->terminal1->parentElement() && conductor->terminal2->parentElement()) {
				watchConductor(conductor);
			}
		}
	}
	return true;
}

/**
	@brief projectDataBase::setUpdateBlocked
	@param blocked : whether updateDB() should skip the full rebuild
*/
void projectDataBase::setUpdateBlocked(bool blocked)
{
	m_update_blocked = blocked;
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
	@brief projectDataBase::isReadOnlySelect
	Every query that reaches newQuery() goes through this check first --
	including one loaded from a saved nomenclature/summary table's <query>
	element (ProjectDBModel::fromXml()), which makes this a defense against
	a crafted project file, not just a careless custom-SQL edit
	(qelectrotech-source-mirror#886 asked for read-only enforcement on the
	custom SQL reports feature; this covers every path into newQuery(), not
	just that one dialog).

	Deliberately simple rather than a real SQL parser: reject more than one
	statement (blocks stacking a write after a leading SELECT with `;`), and
	require the query to start with SELECT or WITH. A determined attacker
	with arbitrary SQL access to a local SQLite connection can still find
	tricks a simple prefix check won't catch; this is meant to stop the
	ordinary mistake and the obvious payload, not to be a security boundary
	against a hostile file assumed to already run in some other trust
	context.
	@param query the raw SQL text to check
	@param error set to a human-readable reason when this returns false
	@return true if @p query looks like a single read-only SELECT/WITH
*/
bool projectDataBase::isReadOnlySelect(const QString &query, QString *error)
{
	if (error) {
		error->clear();
	}

	QString trimmed = query.trimmed();
	if (trimmed.endsWith(QLatin1Char(';'))) {
		trimmed.chop(1);
		trimmed = trimmed.trimmed();
	}

	if (trimmed.isEmpty()) {
		if (error) {
			*error = projectDataBase::tr("La requête est vide.");
		}
		return false;
	}

	if (trimmed.contains(QLatin1Char(';'))) {
		if (error) {
			*error = projectDataBase::tr("Une seule requête SELECT est autorisée"
								  " (le caractère ';' ne peut apparaître"
								  " qu'à la toute fin).");
		}
		return false;
	}

	const int first_space = trimmed.indexOf(QRegularExpression(QStringLiteral("\\s")));
	const QString first_word = (first_space == -1 ? trimmed : trimmed.left(first_space)).toUpper();
	if (first_word != QLatin1String("SELECT") && first_word != QLatin1String("WITH")) {
		if (error) {
			*error = projectDataBase::tr("Seules les requêtes en lecture seule"
								  " (SELECT ou WITH ... SELECT) sont"
								  " autorisées.");
		}
		return false;
	}

	return true;
}

/**
	@brief projectDataBase::newQuery
	@param query the SQL text to run -- must be a single read-only
	SELECT/WITH statement, see isReadOnlySelect()
	@param error set to a human-readable reason when the query was rejected
	or failed
	@return the executed query, on the internal database of this class, or
	an empty, harmless QSqlQuery if the query was rejected or failed
*/
QSqlQuery projectDataBase::newQuery(const QString &query, QString *error) {
	QString reason;

		//Drawing-item rows are rewritten lazily, see drawingItemChanged().
		//Every read from outside comes through here, so this is the one
		//place the queue has to be emptied for a reader to see current rows.
	flushDrawingItems();

	// First gate: which kind of statement is acceptable here at all. A
	// textual check is the right tool for that and the wrong tool for
	// anything else -- see isReadOnlySelect()'s own comment. It is what
	// keeps ATTACH, BEGIN and PRAGMA out, none of which SQLite itself
	// considers writes.
	if (!isReadOnlySelect(query, &reason)) {
		qWarning().noquote() << "projectDataBase::newQuery: rejected query:" << reason << "--" << query;
		if (error) {
			*error = reason;
		}
		return QSqlQuery(m_data_base);
	}

	// Second gate, and the one that actually enforces read-only: SQLite
	// runs the statement with query_only set and refuses a write itself,
	// instead of the text being read for clues. The first gate cannot see
	// through a CTE prefix -- "WITH x AS (SELECT 1) DELETE FROM element"
	// starts with WITH, contains no semicolon, and deletes every row. That
	// matters beyond the custom-query box, because this path is reachable
	// from a file: a <graphics_table>'s saved <query> is read straight out
	// of the .qet by ProjectDBModel::fromXml() and executed by fillValue(),
	// so opening or exporting a project someone else produced would have
	// been enough.
	QSqlQuery result = QETSql::execReadOnly(m_data_base, query, &reason);
	if (!reason.isEmpty()) {
		qWarning().noquote() << "projectDataBase::newQuery: rejected query:" << reason << "--" << query;
		if (error) {
			*error = reason;
		}
	}
	return result;
}

/**
	@brief projectDataBase::excludedConductorCount
	@return how many conductors of the project are absent from the conductor
	table because an endpoint has no parent element to key on.

	Counted from the live scene rather than from the database, precisely
	because the database is where these conductors are *not*.

	This used to count conductors whose terminals had no uuid, which was most
	of them on most projects. Terminal::stableUuid() now derives an identity
	from the terminal's geometry when the definition provides no uuid, so that
	is no longer a reason to exclude anything, and this counts only the case
	that remains genuinely unkeyable.

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
				//Must match addConductor()'s guard exactly, or this reports
				//wires as missing that the list is in fact showing.
			if (!conductor->terminal1->parentElement()
				|| !conductor->terminal2->parentElement()) {
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
	m_content_changed = true;
	if (!element || !element->diagram()) {
		qDebug() << "projectDataBase::addElement: null element or diagram";
		return;
	}

	bindElementValues(m_insert_elements_query, element, element->diagram());
	if (!m_insert_elements_query.exec()) {
		qDebug() << "projectDataBase::addElement insert element error : " << m_insert_elements_query.lastError();
	}

	bindElementInfoValues(m_insert_element_info_query, element);
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
	m_content_changed = true;
	bool changed = false;

	m_remove_element_query.bindValue(":uuid", element->uuid().toString());
	if (m_remove_element_query.exec()) {
		changed = true;
	} else {
		qDebug() << "projectDataBase::removeElement remove error : " << m_remove_element_query.lastError();
	}

	m_remove_element_info_query.bindValue(":uuid", element->uuid().toString());
	if (m_remove_element_info_query.exec()) {
		changed = true;
	} else {
		qDebug() << "projectDataBase::removeElement remove element_info error : " << m_remove_element_info_query.lastError();
	}

	if (changed) {
		emit dataBaseUpdated();
	}
}

/**
	@brief projectDataBase::elementInfoChanged
	@param element
*/
void projectDataBase::elementInfoChanged(Element *element)
{
	m_content_changed = true;
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
	m_content_changed = true;
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
	m_content_changed = true;
	m_insert_diagram_query.bindValue(":uuid", diagram->uuid().toString());
	m_insert_diagram_query.bindValue(":pos", m_project->folioIndex(diagram)+1);
	if(!m_insert_diagram_query.exec()) {
		qDebug() << "projectDataBase::addDiagram insert error : " << m_insert_diagram_query.lastError();
	}

	bindDiagramInfoValues(m_insert_diagram_info_query, diagram);

	if (!m_insert_diagram_info_query.exec()) {
		qDebug() << "projectDataBase::addDiagram insert info error : " << m_insert_diagram_info_query.lastError();
	}

		//A folio put back by undoing its removal comes with its items already
		//on it, and their rows went with it: queue them again.
	const QList<QGraphicsItem *> items = diagram->items();
	for (QGraphicsItem *item : items) {
		addDrawingItem(item);
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
	m_content_changed = true;
	const QString uuid_str = diagram->uuid().toString();

		//Order matters: element_info and terminal are scoped through a
		//subquery on element, so they must run before element itself is
		//deleted below. The whole cascade runs in one transaction and is
		//rolled back on the first error, so a mid-cascade failure (e.g. a
		//locked DB) can't leave the diagram row deleted while its
		//element/terminal/element_info/conductor rows survive.
	m_data_base.transaction();

	m_cascade_remove_element_info_query.bindValue(":uuid", uuid_str);
	if (!m_cascade_remove_element_info_query.exec()) {
		qDebug() << "projectDataBase::removeDiagram element_info cascade error : "
				 << m_cascade_remove_element_info_query.lastError();
		m_data_base.rollback();
		return;
	}

	m_cascade_remove_terminal_query.bindValue(":uuid", uuid_str);
	if (!m_cascade_remove_terminal_query.exec()) {
		qDebug() << "projectDataBase::removeDiagram terminal cascade error : "
				 << m_cascade_remove_terminal_query.lastError();
		m_data_base.rollback();
		return;
	}

	m_cascade_remove_conductor_query.bindValue(":uuid", uuid_str);
	if (!m_cascade_remove_conductor_query.exec()) {
		qDebug() << "projectDataBase::removeDiagram conductor cascade error : "
				 << m_cascade_remove_conductor_query.lastError();
		m_data_base.rollback();
		return;
	}

	m_cascade_remove_element_query.bindValue(":uuid", uuid_str);
	if (!m_cascade_remove_element_query.exec()) {
		qDebug() << "projectDataBase::removeDiagram element cascade error : "
				 << m_cascade_remove_element_query.lastError();
		m_data_base.rollback();
		return;
	}

	for (const QString &table : {QStringLiteral("shape"),
								 QStringLiteral("independent_text"),
								 QStringLiteral("image")})
	{
		QSqlQuery cascade(m_data_base);
		cascade.prepare(QStringLiteral("DELETE FROM %1 WHERE diagram_uuid = :uuid").arg(table));
		cascade.bindValue(QStringLiteral(":uuid"), uuid_str);
		if (!cascade.exec()) {
			qDebug() << "projectDataBase::removeDiagram" << table << "cascade error : "
					 << cascade.lastError();
			m_data_base.rollback();
			return;
		}
	}
		//The folio's items keep existing (the removal can be undone), but
		//their rows are gone: forget them, so that nothing is deleted or
		//skipped later on the strength of a row that no longer exists.
	const QList<QObject *> tracked = m_drawing_item_row.keys();
	for (QObject *object : tracked) {
		auto *item = dynamic_cast<QGraphicsItem *>(object);
		if (item && item->scene() == diagram) {
			forgetDrawingItem(object);
		}
	}

	m_remove_diagram_query.bindValue(":uuid", uuid_str);
	if (!m_remove_diagram_query.exec()) {
		qDebug() << "projectDataBase::removeDiagram delete error : " << m_remove_diagram_query.lastError();
		m_data_base.rollback();
		return;
	}

	m_data_base.commit();
	emit dataBaseUpdated();
}

void projectDataBase::diagramInfoChanged(Diagram *diagram)
{
	m_content_changed = true;
	bindDiagramInfoValues(m_update_diagram_info_query, diagram);

	if (!m_update_diagram_info_query.exec()) {
		qDebug() << "projectDataBase::diagramInfoChanged update error : " << m_update_diagram_info_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

void projectDataBase::diagramOrderChanged()
{
	m_content_changed = true;
}

/**
	@brief projectDataBase::addConductor
	@param conductor
*/
void projectDataBase::addConductor(Conductor *conductor)
{
	m_content_changed = true;
	if (!conductor || !conductor->diagram()) {
		qDebug() << "projectDataBase::addConductor: null conductor or diagram";
		return;
	}

		//Both endpoints must belong to an element: the terminal table is keyed
		//on (terminal, element) and a terminal with no parent has no identity
		//to key on. Terminals whose *definition* predates terminal uuids are
		//fine -- Terminal::stableUuid() derives one from the terminal's local
		//position, which is what the project format itself matches on.
	if (!conductor->terminal1->parentElement()
		|| !conductor->terminal2->parentElement()) {
		return;
	}

	insertTerminal(conductor->terminal1);
	insertTerminal(conductor->terminal2);

	watchConductor(conductor);
	bindConductorValues(m_insert_conductor_query, conductor, conductor->diagram());
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
	m_content_changed = true;
	m_remove_conductor_query.bindValue(":uuid", conductor->uuid().toString());
	if (!m_remove_conductor_query.exec()) {
		qDebug() << "projectDataBase::removeConductor delete error : " << m_remove_conductor_query.lastError();
	} else {
		emit dataBaseUpdated();
	}
}

/**
	@brief projectDataBase::updateConductor
	Refresh the mutable columns of an already-inserted conductor.

	Only the text (the wire number) can change without the conductor being
	removed and re-added: its endpoints are fixed for its lifetime. Without
	this, renaming a wire left the database holding the old number and the
	wiring list showed a stale value until the next full repopulate.
	@param conductor
*/
void projectDataBase::updateConductor(Conductor *conductor)
{
	m_content_changed = true;
	if (!conductor) {
		return;
	}

	m_update_conductor_query.bindValue(QStringLiteral(":uuid"), conductor->uuid().toString());
	m_update_conductor_query.bindValue(QStringLiteral(":text"), conductor->properties().text);
	if (!m_update_conductor_query.exec()) {
		qDebug() << "projectDataBase::updateConductor update error : " << m_update_conductor_query.lastError();
	}

		//Deliberately no dataBaseUpdated() here, unlike add/remove. The only
		//column this touches is the wire text, which no view watched by
		//ProjectDBModel displays -- the nomenclature shows elements, and its
		//wire_count changes when a conductor appears or disappears, not when
		//it is renamed. Emitting would make every ProjectDBModel re-run its
		//query, and auto-numbering renames every conductor in the project in
		//one pass.
}

/**
	@brief projectDataBase::watchConductor
	Keep this conductor's row in step with its properties.

	Conductor::setProperties() has a dozen call sites (auto-numbering, the
	properties dialog, element moves, deletion re-links...), so listening to
	the signal it already emits is the only way to catch them all -- and the
	only way to catch the ones added later. Qt::UniqueConnection makes a
	repeated insert or a full repopulate harmless.
	@param conductor
*/
void projectDataBase::watchConductor(Conductor *conductor)
{
	connect(conductor, &Conductor::propertiesChange,
			this, &projectDataBase::conductorPropertiesChanged,
			Qt::UniqueConnection);
}

/**
	@brief projectDataBase::conductorPropertiesChanged
*/
void projectDataBase::conductorPropertiesChanged()
{
	m_content_changed = true;
	if (auto *conductor = qobject_cast<Conductor *>(sender())) {
		updateConductor(conductor);
	}
}

/**
	@brief projectDataBase::bindConductorValues
	One binder for both insert paths, so a conductor added to a live diagram
	and one read from a file can never drift apart -- the same reason
	bindElementValues() exists for elements.
	@param query
	@param conductor
	@param diagram : the diagram the conductor belongs to
*/
void projectDataBase::bindConductorValues(QSqlQuery &query, Conductor *conductor, Diagram *diagram)
{
	query.bindValue(QStringLiteral(":uuid"), conductor->uuid().toString());
	query.bindValue(QStringLiteral(":diagram_uuid"), diagram->uuid().toString());
	query.bindValue(QStringLiteral(":terminal1_uuid"), conductor->terminal1->stableUuid().toString());
	query.bindValue(QStringLiteral(":terminal1_element_uuid"), conductor->terminal1->parentElement()->uuid().toString());
	query.bindValue(QStringLiteral(":terminal2_uuid"), conductor->terminal2->stableUuid().toString());
	query.bindValue(QStringLiteral(":terminal2_element_uuid"), conductor->terminal2->parentElement()->uuid().toString());
	query.bindValue(QStringLiteral(":text"), conductor->properties().text);
}

namespace {

/// Table holding @p object's row, or an empty string if it has none.
QString drawingItemTable(QObject *object)
{
	if (qobject_cast<QetShapeItem *>(object))        return QStringLiteral("shape");
	if (qobject_cast<IndependentTextItem *>(object)) return QStringLiteral("independent_text");
	if (qobject_cast<DiagramImageItem *>(object))    return QStringLiteral("image");
	return QString();
}

QUuid drawingItemUuid(QObject *object)
{
	if (auto s = qobject_cast<QetShapeItem *>(object))        return s->uuid();
	if (auto t = qobject_cast<IndependentTextItem *>(object)) return t->uuid();
	if (auto i = qobject_cast<DiagramImageItem *>(object))    return i->uuid();
	return QUuid();
}

/// The group_uuid column of @p item's row: its group, or NULL.
QVariant groupValue(const QGraphicsItem *item)
{
	const QUuid group = ItemGroups::groupOf(item);
	return group.isNull() ? QVariant() : QVariant(group.toString());
}

} // namespace

/**
	@brief projectDataBase::addDrawingItem
	Start keeping a row for a shape, an independent text or an image that
	was just added to a folio. Anything else is ignored, so Diagram::addItem()
	can pass every item it gets.

	The row is not written here but queued, like every later change to it:
	see drawingItemChanged() for why.
	@param item
*/
void projectDataBase::addDrawingItem(QGraphicsItem *item)
{
	QGraphicsObject *object = item ? item->toGraphicsObject() : nullptr;
	if (!object || drawingItemTable(object).isEmpty()) {
		return;
	}

	const auto unique = Qt::UniqueConnection;
	connect(object, &QGraphicsObject::xChanged,        this, &projectDataBase::drawingItemChanged, unique);
	connect(object, &QGraphicsObject::yChanged,        this, &projectDataBase::drawingItemChanged, unique);
	connect(object, &QGraphicsObject::rotationChanged, this, &projectDataBase::drawingItemChanged, unique);
	connect(object, &QObject::destroyed,               this, &projectDataBase::drawingItemDestroyed, unique);

	if (auto shape = qobject_cast<QetShapeItem *>(object)) {
		connect(shape, &QetShapeItem::uuidChanged,      this, &projectDataBase::drawingItemChanged, unique);
		connect(shape, &QetShapeItem::geometryChanged,  this, &projectDataBase::drawingItemChanged, unique);
		connect(shape, &QetShapeItem::transformChanged, this, &projectDataBase::drawingItemChanged, unique);
		connect(shape, &QetShapeItem::penChanged,       this, &projectDataBase::drawingItemChanged, unique);
		connect(shape, &QetShapeItem::brushChanged,     this, &projectDataBase::drawingItemChanged, unique);
	}
	else if (auto text = qobject_cast<IndependentTextItem *>(object)) {
		connect(text, &IndependentTextItem::uuidChanged, this, &projectDataBase::drawingItemChanged, unique);
			//Sent by the document, not the item: drawingItemChanged() walks
			//back up to the item. It is the one signal that catches every way
			//the text changes -- typing, undo, a script's setTextContent().
		connect(text->document(), &QTextDocument::contentsChanged,
				this, &projectDataBase::drawingItemChanged, unique);
	}
	else if (auto image = qobject_cast<DiagramImageItem *>(object)) {
		connect(image, &DiagramImageItem::uuidChanged,      this, &projectDataBase::drawingItemChanged, unique);
		connect(image, &DiagramImageItem::transformChanged, this, &projectDataBase::drawingItemChanged, unique);
		connect(image, &DiagramImageItem::pixmapChanged,    this, &projectDataBase::drawingItemChanged, unique);
	}

	m_dirty_drawing_items.insert(object);
}

/**
	@brief projectDataBase::removeDrawingItem
	Drop the row of a shape, independent text or image taken off its folio.
	The item itself usually lives on in the undo stack, so it is also
	disconnected: a change to it there must not bring its row back.
	@param item
*/
void projectDataBase::removeDrawingItem(QGraphicsItem *item)
{
	QGraphicsObject *object = item ? item->toGraphicsObject() : nullptr;
	if (!object || drawingItemTable(object).isEmpty()) {
		return;
	}

	disconnect(object, nullptr, this, nullptr);
	if (auto text = qobject_cast<IndependentTextItem *>(object)) {
		disconnect(text->document(), nullptr, this, nullptr);
	}

	const QUuid row = m_drawing_item_row.value(object);
	if (!row.isNull() && m_drawing_row_owner.value(row) == object)
	{
		QSqlQuery remove(m_data_base);
		remove.prepare(QStringLiteral("DELETE FROM %1 WHERE uuid = :uuid")
					   .arg(drawingItemTable(object)));
		remove.bindValue(QStringLiteral(":uuid"), row.toString());
		if (!remove.exec()) {
			qDebug() << "projectDataBase::removeDrawingItem delete error : " << remove.lastError();
		}
	}
	forgetDrawingItem(object);
}

/**
	@brief projectDataBase::itemGroupChanged
	@a item joined or left a group (discussion #1070). An element's row is
	updated at once; a drawing item's is queued like any other change to it.
	@param item
*/
void projectDataBase::itemGroupChanged(QGraphicsItem *item)
{
	if (auto element = qgraphicsitem_cast<Element *>(item))
	{
		QSqlQuery update(m_data_base);
		update.prepare(QStringLiteral("UPDATE element SET group_uuid = :group_uuid WHERE uuid = :uuid"));
		update.bindValue(QStringLiteral(":group_uuid"), groupValue(element));
		update.bindValue(QStringLiteral(":uuid"), element->uuid().toString());
		if (!update.exec()) {
			qDebug() << "projectDataBase::itemGroupChanged update error : " << update.lastError();
		}
		m_content_changed = true;
		return;
	}

	QGraphicsObject *object = item ? item->toGraphicsObject() : nullptr;
	if (object && !drawingItemTable(object).isEmpty()) {
		m_dirty_drawing_items.insert(object);
	}
}

/**
	@brief projectDataBase::drawingItemChanged
	Queue the sender's row to be rewritten.

	Queued rather than written: a move sends xChanged/yChanged for every
	mouse step of every selected item, and nothing reads the rows between
	two steps. newQuery() and updateDB() flush the queue before anything
	does, so a reader never sees a stale row -- a queued write only costs a
	set insertion.
*/
void projectDataBase::drawingItemChanged()
{
	QObject *object = sender();
		//QTextDocument::contentsChanged: the item is an ancestor of the
		//document (item -> text control -> document).
	while (object && drawingItemTable(object).isEmpty()) {
		object = object->parent();
	}
	if (object) {
		m_dirty_drawing_items.insert(object);
	}
}

/**
	@brief projectDataBase::drawingItemDestroyed
	An item deleted while still on its folio (e.g. by the scene's own
	destructor) never went through removeDrawingItem(). Its type can no
	longer be asked -- this runs from QObject's destructor -- so its row,
	if it wrote one, is looked for in all three tables.
	@param object
*/
void projectDataBase::drawingItemDestroyed(QObject *object)
{
	const QUuid row = m_drawing_item_row.value(object);
	if (!row.isNull() && m_drawing_row_owner.value(row) == object)
	{
		for (const QString &table : {QStringLiteral("shape"),
									 QStringLiteral("independent_text"),
									 QStringLiteral("image")})
		{
			QSqlQuery remove(m_data_base);
			remove.prepare(QStringLiteral("DELETE FROM %1 WHERE uuid = :uuid").arg(table));
			remove.bindValue(QStringLiteral(":uuid"), row.toString());
			remove.exec();
		}
	}
	forgetDrawingItem(object);
}

void projectDataBase::forgetDrawingItem(QObject *object)
{
	const QUuid row = m_drawing_item_row.take(object);
	if (!row.isNull() && m_drawing_row_owner.value(row) == object) {
		m_drawing_row_owner.remove(row);
	}
	m_dirty_drawing_items.remove(object);
}

/**
	@brief projectDataBase::writeDrawingItem
	Write @p object's row under its current uuid.
	@return false if the row must wait: another live item still owns that
	uuid. That is a pasted copy in the moment between being added to the
	folio and PasteDiagramCommand giving it its own uuid; writing then would
	overwrite its source's row with the copy's position. The copy's
	uuidChanged() queues it again once it has one.
*/
bool projectDataBase::writeDrawingItem(QObject *object)
{
	auto *item = dynamic_cast<QGraphicsItem *>(object);
	auto *diagram = item ? qobject_cast<Diagram *>(item->scene()) : nullptr;
	if (!diagram || !m_project || !m_project->diagrams().contains(diagram)) {
			//Not on a folio of this project (any more): nothing to write,
			//and nothing to wait for.
		return true;
	}

	const QUuid uuid = drawingItemUuid(object);
	QObject *owner = m_drawing_row_owner.value(uuid);
	if (owner && owner != object) {
		return false;
	}

	QSqlQuery *query = nullptr;
	const QRectF rect = item->sceneBoundingRect();
	if (auto shape = qobject_cast<QetShapeItem *>(object))
	{
		query = &m_insert_shape_query;
		const QMetaEnum type = QetShapeItem::staticMetaObject.enumerator(
					QetShapeItem::staticMetaObject.indexOfEnumerator("ShapeType"));
		query->bindValue(QStringLiteral(":type"), QString::fromLatin1(type.valueToKey(shape->shapeType())));
		query->bindValue(QStringLiteral(":color"), shape->pen().color().name());
		query->bindValue(QStringLiteral(":fill"), shape->brush().style() == Qt::NoBrush
						 ? QStringLiteral("none")
						 : shape->brush().color().name());
	}
	else if (auto text = qobject_cast<IndependentTextItem *>(object))
	{
		query = &m_insert_independent_text_query;
		query->bindValue(QStringLiteral(":text"), text->toPlainText());
		query->bindValue(QStringLiteral(":rotation"), text->rotation());
	}
	else if (auto image = qobject_cast<DiagramImageItem *>(object))
	{
		query = &m_insert_image_query;
		query->bindValue(QStringLiteral(":pixel_width"), image->pixmap().width());
		query->bindValue(QStringLiteral(":pixel_height"), image->pixmap().height());
	}
	if (!query) {
		return true;
	}

		//Renewed since its last write (a paste, a folio duplication): its
		//old row was its own, and describes nothing now.
	const QUuid previous = m_drawing_item_row.value(object);
	if (!previous.isNull() && previous != uuid
		&& m_drawing_row_owner.value(previous) == object)
	{
		QSqlQuery remove(m_data_base);
		remove.prepare(QStringLiteral("DELETE FROM %1 WHERE uuid = :uuid")
					   .arg(drawingItemTable(object)));
		remove.bindValue(QStringLiteral(":uuid"), previous.toString());
		remove.exec();
		m_drawing_row_owner.remove(previous);
	}

	query->bindValue(QStringLiteral(":uuid"), uuid.toString());
	query->bindValue(QStringLiteral(":diagram_uuid"), diagram->uuid().toString());
	query->bindValue(QStringLiteral(":pos"), diagram->convertPosition(rect.topLeft()).toString());
	query->bindValue(QStringLiteral(":x"), rect.x());
	query->bindValue(QStringLiteral(":y"), rect.y());
	query->bindValue(QStringLiteral(":width"), rect.width());
	query->bindValue(QStringLiteral(":height"), rect.height());
	query->bindValue(QStringLiteral(":group_uuid"), groupValue(item));
	if (!query->exec()) {
		qDebug() << "projectDataBase::writeDrawingItem error : " << query->lastError();
		return true;
	}

	m_drawing_item_row.insert(object, uuid);
	m_drawing_row_owner.insert(uuid, object);
	return true;
}

/**
	@brief projectDataBase::flushDrawingItems
	Write every queued drawing-item row. A row that has to wait for its uuid
	(see writeDrawingItem()) stays queued.
*/
void projectDataBase::flushDrawingItems()
{
	if (m_dirty_drawing_items.isEmpty()) {
		return;
	}

	const QSet<QObject *> dirty = m_dirty_drawing_items;
		//One transaction for the batch, unless a caller already holds one.
	const bool own_transaction = m_data_base.transaction();
	for (QObject *object : dirty) {
		if (writeDrawingItem(object)) {
			m_dirty_drawing_items.remove(object);
		}
	}
	if (own_transaction) {
		m_data_base.commit();
	}
}

/**
	@brief projectDataBase::populateDrawingItemTables
	Rebuild the shape, independent_text and image tables from every folio.
*/
void projectDataBase::populateDrawingItemTables()
{
	QSqlQuery query_(m_data_base);
	query_.exec(QStringLiteral("DELETE FROM shape"));
	query_.exec(QStringLiteral("DELETE FROM independent_text"));
	query_.exec(QStringLiteral("DELETE FROM image"));
	m_drawing_item_row.clear();
	m_drawing_row_owner.clear();
	m_dirty_drawing_items.clear();

		//Queued directly, not through addDrawingItem(): every one of them
		//came onto its folio through Diagram::addItem(), which connected it
		//already, and a full rebuild runs on every load.
	for (auto diagram : m_project->diagrams())
	{
		const QList<QGraphicsItem *> items = diagram->items();
		for (QGraphicsItem *item : items)
		{
			QGraphicsObject *object = item->toGraphicsObject();
			if (object && !drawingItemTable(object).isEmpty()) {
				m_dirty_drawing_items.insert(object);
			}
		}
	}
	flushDrawingItems();
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
						  "group_uuid VARCHAR(50),"
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
						  "terminal_index INTEGER,"
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

		//The element-facing columns are looked up per element row, not per
		//conductor row: element_nomenclature_view carries a correlated
		//subquery counting the wires touching each element. Without these
		//indexes each element row full-scans the conductor table, which grows
		//as elements x conductors.
	for (const QString &index_ : {
			QStringLiteral("CREATE INDEX idx_conductor_terminal1_element ON conductor (terminal1_element_uuid)"),
			QStringLiteral("CREATE INDEX idx_conductor_terminal2_element ON conductor (terminal2_element_uuid)"),
			QStringLiteral("CREATE INDEX idx_conductor_diagram ON conductor (diagram_uuid)") })
	{
		if (!query_.exec(index_)) {
			qDebug() << "conductor index query : " << query_.lastError();
		}
	}

		//The folio's drawing furniture: shapes, independent texts, images.
		//x, y, width and height are the item's bounding rect on the folio,
		//pos the folio cell of its top left corner, as for element.
	const QString drawing_columns(
				"uuid VARCHAR(50) PRIMARY KEY NOT NULL, "
				"diagram_uuid VARCHAR(50) NOT NULL, "
				"pos VARCHAR(6), "
				"x REAL, y REAL, width REAL, height REAL, "
				"group_uuid VARCHAR(50), ");
	for (const QString &table : {
			QStringLiteral("CREATE TABLE shape (") + drawing_columns +
				"type VARCHAR(20), color VARCHAR(20), fill VARCHAR(20), "
				"FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid))",
			QStringLiteral("CREATE TABLE independent_text (") + drawing_columns +
				"text TEXT, rotation REAL, "
				"FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid))",
			QStringLiteral("CREATE TABLE image (") + drawing_columns +
				"pixel_width INTEGER, pixel_height INTEGER, "
				"FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid))",
			QStringLiteral("CREATE INDEX idx_shape_diagram ON shape (diagram_uuid)"),
			QStringLiteral("CREATE INDEX idx_independent_text_diagram ON independent_text (diagram_uuid)"),
			QStringLiteral("CREATE INDEX idx_image_diagram ON image (diagram_uuid)") })
	{
		if (!query_.exec(table)) {
			qDebug() << "drawing item table query : " << query_.lastError();
		}
	}

	createElementNomenclatureView();
	createSummaryView();
	createWiringListView();
	createDrawingItemView();
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
						 "ei.model AS model,"
						 "ei.category AS category,"
						 "ei.voltage_rating AS voltage_rating,"
						 "ei.current_rating AS current_rating,"
						 "ei.notes AS notes,"
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
						 " WHERE ei.element_uuid = e.uuid AND e.diagram_uuid = d.uuid AND di.diagram_uuid = d.uuid"
						 " AND COALESCE(LOWER(TRIM(ei.exclude_from_bom)), '') NOT IN ('true', '1', 'yes', 'on')"
							//The element table holds every element of the project; which
							//kinds belong in a nomenclature is this view's business, not
							//the table's. Kept identical to the mask populateElementTable()
							//used to apply, so what this view returns does not change --
							//a slave element (a relay contact) is still not a line item.
							 //Slave is here because an auxiliary contact block is
							 //separately orderable hardware with its own part
							 //number, even though it shares its master's BMK.
							 //Anything that should not be ordered -- a relay's
							 //own auxiliary contact, say -- is kept out by
							 //exclude_from_bom above, not by its base type.
							 //See discussion #847.
						 " AND e.type IN ('simple', 'terminal', 'master', 'slave', 'thumbnail')");

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

	- diagram is LEFT joined for the same reason. It should
	  always match, since QETProject::diagramAdded is wired to addDiagram()
	  and a conductor cannot exist before its folio -- but an inner join here
	  would make that an assumption the view silently enforces, and a wire
	  missing from a wiring list is the one failure this view must not have.

	The result is that this view returns exactly as many rows as the
	conductor table holds -- what is already excluded upstream (conductors
	on legacy terminals without uuids) stays excluded, and nothing new is
	dropped here. Only the terminal joins are inner, and both are guaranteed
	by insertTerminal() running for each endpoint before the conductor row
	is written.
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
						 "d.pos AS diagram_position,"
						 "t1.uuid AS from_terminal_uuid,"
						 "t1.terminal_index AS from_terminal_index,"
						 "t2.uuid AS to_terminal_uuid,"
						 "t2.terminal_index AS to_terminal_index"
						 " FROM conductor c"
						 " JOIN terminal t1 ON c.terminal1_uuid = t1.uuid AND c.terminal1_element_uuid = t1.element_uuid"
						 " JOIN terminal t2 ON c.terminal2_uuid = t2.uuid AND c.terminal2_element_uuid = t2.element_uuid"
						 " LEFT JOIN element_info ei1 ON t1.element_uuid = ei1.element_uuid"
						 " LEFT JOIN element_info ei2 ON t2.element_uuid = ei2.element_uuid"
						 " LEFT JOIN diagram d ON c.diagram_uuid = d.uuid");

	QSqlQuery query(m_data_base);
	if (!query.exec(create_view)) {
		qDebug() << query.lastError();
	}
}

/**
	@brief projectDataBase::createDrawingItemView
	One row per shape, independent text and image, whichever table holds it:
	find anything by uuid without knowing its kind first. folio is the
	folio's position in the project, starting at 1; description is the
	shape type, the text, or empty for an image.
*/
void projectDataBase::createDrawingItemView()
{
	QSqlQuery query(m_data_base);
	const QString create_view(
				"CREATE VIEW drawing_item_view AS "
				"SELECT i.uuid, i.kind, d.pos AS folio, i.diagram_uuid, i.pos, "
				"i.x, i.y, i.width, i.height, i.description FROM ("
				"SELECT uuid, 'shape' AS kind, diagram_uuid, pos, x, y, width, height, "
				"type AS description FROM shape "
				"UNION ALL SELECT uuid, 'text', diagram_uuid, pos, x, y, width, height, "
				"text FROM independent_text "
				"UNION ALL SELECT uuid, 'image', diagram_uuid, pos, x, y, width, height, "
				"'' FROM image"
				") AS i LEFT JOIN diagram AS d ON d.uuid = i.diagram_uuid");
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
	@brief allElementTypes
	Every ElementData::Type, i.e. no filtering at all.

	The element table used to be populated with only
	Simple|Terminal|Master|Thumbnail, which quietly made it "the elements a
	nomenclature cares about" rather than "the elements of the project".
	Anything else reading the table -- the wiring list, and terminal plans
	later -- then could not see slave elements (relay contacts) or report
	elements, which are ordinary conductor endpoints. The filter now lives in
	element_nomenclature_view, where it belongs; see createElementNomenclatureView().
*/
static ElementData::Types allElementTypes()
{
	return ElementData::Simple
		   | ElementData::NextReport
		   | ElementData::PreviousReport
		   | ElementData::Master
		   | ElementData::Slave
		   | ElementData::Terminal
		   | ElementData::Thumbnail
		   | ElementData::ConductorDefinition;
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
		const auto elmt_vector = ep.find(allElementTypes());
			//Insert all values into the database
		for (const auto &elmt : elmt_vector)
		{
			bindElementValues(m_insert_elements_query, elmt, diagram);
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
		const auto elmt_vector = ep.find(allElementTypes());

			//Insert all values into the database
		for (const auto &elmt : elmt_vector)
		{
			bindElementInfoValues(m_insert_element_info_query, elmt);
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
				//See addConductor(): only a terminal with no parent element is
				//skipped. A missing terminal uuid is handled by stableUuid().
			if (!conductor->terminal1->parentElement()
				|| !conductor->terminal2->parentElement()) {
				continue;
			}

			insertTerminal(conductor->terminal1);
			insertTerminal(conductor->terminal2);

			watchConductor(conductor);
			bindConductorValues(m_insert_conductor_query, conductor, diagram);
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
	const QList<Terminal *> terminals = terminal->parentElement()->terminals();
	QList<QPointF> points;
	for (const Terminal *t : terminals) {
		points << terminal->parentElement()->mapFromScene(t->dockConductor());
	}
	insertTerminal(terminal->stableUuid().toString(),
				   terminal->parentElement()->uuid().toString(),
				   terminal->name(),
				   terminalIndexes(points).value(terminals.indexOf(terminal)));
}

/**
	@brief projectDataBase::insertTerminal
	insertTerminal(Terminal *) from values rather than a live terminal.
*/
void projectDataBase::insertTerminal(const QString &uuid, const QString &element_uuid,
									 const QString &name, const QVariant &index)
{
	m_insert_terminal_query.bindValue(":uuid", uuid);
	m_insert_terminal_query.bindValue(":element_uuid", element_uuid);
	m_insert_terminal_query.bindValue(":name", name);
	m_insert_terminal_query.bindValue(":terminal_index", index);
	if (!m_insert_terminal_query.exec()) {
		qDebug() << "projectDataBase::insertTerminal insert error : " << m_insert_terminal_query.lastError();
	}
}

void projectDataBase::prepareQuery()
{
		//INSERT DIAGRAM
	m_insert_diagram_query = QSqlQuery(m_data_base);
	m_insert_diagram_query.prepare("INSERT INTO diagram (uuid, pos) VALUES (:uuid, :pos)");

		//REMOVE DIAGRAM (cascade first: element_info and terminal have no
		//diagram_uuid column of their own, so both are scoped through
		//element while the element rows for this diagram still exist).
	m_cascade_remove_element_info_query = QSqlQuery(m_data_base);
	m_cascade_remove_element_info_query.prepare(
		"DELETE FROM element_info WHERE element_uuid IN "
		"(SELECT uuid FROM element WHERE diagram_uuid = :uuid)");

	m_cascade_remove_terminal_query = QSqlQuery(m_data_base);
	m_cascade_remove_terminal_query.prepare(
		"DELETE FROM terminal WHERE element_uuid IN "
		"(SELECT uuid FROM element WHERE diagram_uuid = :uuid)");

	m_cascade_remove_conductor_query = QSqlQuery(m_data_base);
	m_cascade_remove_conductor_query.prepare(
		"DELETE FROM conductor WHERE diagram_uuid = :uuid");

	m_cascade_remove_element_query = QSqlQuery(m_data_base);
	m_cascade_remove_element_query.prepare(
		"DELETE FROM element WHERE diagram_uuid = :uuid");

	m_remove_diagram_query = QSqlQuery(m_data_base);
	m_remove_diagram_query.prepare("DELETE FROM diagram WHERE uuid=:uuid");

		//DRAWING ITEMS. OR REPLACE: a row is rewritten in place on every
		//change, see writeDrawingItem().
	const QString drawing_columns("uuid, diagram_uuid, pos, x, y, width, height, group_uuid");
	const QString drawing_values(":uuid, :diagram_uuid, :pos, :x, :y, :width, :height, :group_uuid");
	m_insert_shape_query = QSqlQuery(m_data_base);
	m_insert_shape_query.prepare("INSERT OR REPLACE INTO shape (" + drawing_columns +
								 ", type, color, fill) VALUES (" + drawing_values +
								 ", :type, :color, :fill)");
	m_insert_independent_text_query = QSqlQuery(m_data_base);
	m_insert_independent_text_query.prepare("INSERT OR REPLACE INTO independent_text (" + drawing_columns +
											", text, rotation) VALUES (" + drawing_values +
											", :text, :rotation)");
	m_insert_image_query = QSqlQuery(m_data_base);
	m_insert_image_query.prepare("INSERT OR REPLACE INTO image (" + drawing_columns +
								 ", pixel_width, pixel_height) VALUES (" + drawing_values +
								 ", :pixel_width, :pixel_height)");

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
	QString insert_element_query("INSERT INTO element (uuid, diagram_uuid, pos, type, sub_type, group_uuid) VALUES (:uuid, :diagram_uuid, :pos, :type, :sub_type, :group_uuid)");
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

		//REMOVE ELEMENT INFO
		//element_info has no ON DELETE CASCADE (foreign keys aren't
		//enforced by this connection), so removeElement() must clear it
		//explicitly. Without this, the row is orphaned under the removed
		//element's uuid, and re-adding an element with that same uuid
		//later -- undo of this same removal, or a redo replaying it --
		//hits element_info's PRIMARY KEY constraint on element_uuid: the
		//element re-add succeeds, but its element_info insert silently
		//fails and is lost. removeDiagram()'s cascade already clears this
		//table when a whole folio goes, but that does not run for a
		//single element removed on its own.
	QString remove_element_info("DELETE FROM element_info WHERE element_uuid=:uuid");
	m_remove_element_info_query = QSqlQuery(m_data_base);
	m_remove_element_info_query.prepare(remove_element_info);

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
	m_insert_terminal_query.prepare("INSERT OR IGNORE INTO terminal (uuid, element_uuid, name, terminal_index) VALUES (:uuid, :element_uuid, :name, :terminal_index)");

		//INSERT CONDUCTOR
	m_insert_conductor_query = QSqlQuery(m_data_base);
	m_insert_conductor_query.prepare("INSERT INTO conductor (uuid, diagram_uuid, terminal1_uuid, terminal1_element_uuid, terminal2_uuid, terminal2_element_uuid, text) "
					  "VALUES (:uuid, :diagram_uuid, :terminal1_uuid, :terminal1_element_uuid, :terminal2_uuid, :terminal2_element_uuid, :text)");

		//UPDATE CONDUCTOR
	m_update_conductor_query = QSqlQuery(m_data_base);
	m_update_conductor_query.prepare(QStringLiteral("UPDATE conductor SET text = :text WHERE uuid = :uuid"));

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

/**
	@brief projectDataBase::bindElementValues
	Bind one element's row for the element table.

	Shared by addElement() (a single element added to a live diagram) and
	populateElementTable() (a full rebuild), because those two used to bind
	the same row differently: the incremental path wrote
	kindInformations()["type"] into sub_type while the bulk path wrote
	elementData().masterTypeToString(). The element table therefore held
	different values depending on whether the project had been reloaded
	since the element was placed. One binder means live and reloaded agree
	by construction rather than by coincidence.

	The bulk path's values are the ones kept: they are what every already
	saved project contains, so nothing a reload produces changes.
	@param query : prepared insert query to bind into
	@param element : element to bind
	@param diagram : diagram holding @element
*/
void projectDataBase::bindElementValues(QSqlQuery &query, Element *element, Diagram *diagram)
{
	const auto element_data = element->elementData();
	query.bindValue(QStringLiteral(":uuid"), element->uuid().toString());
	query.bindValue(QStringLiteral(":diagram_uuid"), diagram->uuid().toString());
	query.bindValue(QStringLiteral(":pos"), diagram->convertPosition(element->scenePos()).toString());
	query.bindValue(QStringLiteral(":type"), element_data.typeToString());
	query.bindValue(QStringLiteral(":sub_type"), element_data.masterTypeToString());
	query.bindValue(QStringLiteral(":group_uuid"), groupValue(element));
}

/**
	@brief projectDataBase::bindElementInfoValues
	Bind one element's row for the element info table.
	Shared by addElement() and populateElementInfoTable() for the same
	reason as bindElementValues().
	@param query : prepared insert query to bind into
	@param element : element to bind
*/
void projectDataBase::bindElementInfoValues(QSqlQuery &query, Element *element)
{
	query.bindValue(QStringLiteral(":uuid"), element->uuid().toString());
	const auto hash = elementInfoToString(element);
	for (const auto &key : hash.keys()) {
		query.bindValue(QStringLiteral(":") + key, hash.value(key));
	}
}

/**
	@brief projectDataBase::bindElementInfoValues
	bindElementInfoValues(QSqlQuery &, Element *) from values: the element's
	information and the label to store for it.
*/
void projectDataBase::bindElementInfoValues(QSqlQuery &query, const QString &element_uuid,
											const DiagramContext &informations,
											const QString &label)
{
	query.bindValue(QStringLiteral(":uuid"), element_uuid);
	for (const auto &key : QETInformation::elementInfoKeys()) {
		query.bindValue(QStringLiteral(":") + key,
						key == QLatin1String("label") ? label
													  : informations[key].toString());
	}
}

void projectDataBase::bindDiagramInfoValues(QSqlQuery &query, Diagram *diagram)
{
	bindDiagramInfoValues(query, diagram->uuid(), diagram->border_and_titleblock);
}

/**
	@brief projectDataBase::bindDiagramInfoValues
	bindDiagramInfoValues(QSqlQuery &, Diagram *) from a folio's uuid and its
	border and title block, which need not belong to a built folio.
*/
void projectDataBase::bindDiagramInfoValues(QSqlQuery &query, const QUuid &diagram_uuid,
											const BorderTitleBlock &border)
{
	bindDiagramInfoValues(query, diagram_uuid, border.titleblockInformation(), border.date());
}

/**
	@brief projectDataBase::bindDiagramInfoValues
	The same from a title block's information and date.
*/
void projectDataBase::bindDiagramInfoValues(QSqlQuery &query, const QUuid &diagram_uuid,
											const DiagramContext &infos, const QDate &date)
{
	query.bindValue(":uuid", diagram_uuid);

	for (auto key : QETInformation::diagramInfoKeys())
	{
		if (key == "date") {
				//The folio's own date, not the title block's text read
				//back: that text is the locale's short format, and where
				//it has a two-digit year (en_US "M/d/yy") toDate() reads
				//2010 back as 1910.
			query.bindValue(QStringLiteral(":date"), date);
		} else {
			auto value = infos.value(key);
			auto bind = key.prepend(":");
			query.bindValue(bind, value);
		}
	}
}

#ifdef QET_EXPORT_PROJECT_DB

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

	// VACUUM INTO requires the destination not to exist. QFileDialog may ask
	// about overwriting, but it does not remove the existing file for us.
	if (QFile::exists(path_) && !QFile::remove(path_)) {
		qWarning() << "Unable to replace project database export:" << path_;
		return;
	}

	// VACUUM INTO creates a standalone copy of the current database without
	// requiring access to the SQLite driver's native connection handle.
	const auto escaped_path = path_.replace("'", "''");
	db->flushDrawingItems();
	QSqlQuery query(db->m_data_base);
	if (!query.exec("VACUUM INTO '" % escaped_path % "'")) {
		qWarning() << "Unable to export project database:" << query.lastError().text();
	}
}
#endif
