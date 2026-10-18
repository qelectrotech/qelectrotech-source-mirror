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
#include "../qetversion.h"

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
#include "../TerminalStrip/terminalstrip.h"
#include "../TerminalStrip/physicalterminal.h"
#include "../TerminalStrip/realterminal.h"

#include <QLocale>
#include <QDate>
#include <QDomDocument>

#include <functional>
#include <QMetaEnum>
#include <QTextDocument>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
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
		updateFolioPositions();
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
		flushLinks();
		flushElementPositions();
		flushConductorProperties();
		emit dataBaseUpdated();
		return;
	}

	keepUnbuiltFolioRows();
	populateDiagramTable();
	populateDiagramInfoTable();
	populateElementTable();
	populateElementInfoTable();
		//Not the spare_terminal table : it is not built from the folios, and
		//the database is made (and updateDB() called) by the constructor of the
		//project, before its terminal strips exist. It is filled when a strip
		//is added to the project or changed (spareTerminalsChanged()).
	populateConductorTable();
	populateLinkTable();
	populateDrawingItemTables();
	dropKeptRows();
	flushConductorProperties();
	m_content_changed = false;

	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::keepUnbuiltFolioRows
	Before the tables are rebuilt from the folios: keep the element,
	element_info, terminal, conductor and link rows of the folios not built
	yet (lazy folios). They came from the document, nothing of those
	folios can have changed since, and the rebuild puts them back where
	each folio comes (restoreKeptRows()). Nothing to do when every folio
	is built.
*/
void projectDataBase::keepUnbuiltFolioRows()
{
	m_kept_folios.clear();
	if (!m_project->unloadedFolioCount()) {
		return;
	}
		//Rows filled from the folios have none for a folio not built: it
		//is built (diagrams()) and the tables come from it
	if (!m_rows_from_document) {
		m_project->diagrams();
		return;
	}
	for (Diagram *folio : m_project->folios())
		if (!folio->isLoaded()) m_kept_folios.insert(folio);
	QSqlQuery query(m_data_base);
	query.exec(QStringLiteral("CREATE TEMP TABLE kept_folio (uuid VARCHAR(50) PRIMARY KEY)"));
	query.prepare(QStringLiteral("INSERT INTO kept_folio (uuid) VALUES (:uuid)"));
	for (Diagram *folio : std::as_const(m_kept_folios)) {
		query.bindValue(QStringLiteral(":uuid"), folio->uuid().toString());
		query.exec();
	}
	for (const QString &sql : {
		 QStringLiteral("CREATE TEMP TABLE kept_element AS SELECT * FROM element "
						"WHERE diagram_uuid IN (SELECT uuid FROM kept_folio) ORDER BY rowid"),
		 QStringLiteral("CREATE TEMP TABLE kept_element_info AS SELECT * FROM element_info "
						"WHERE element_uuid IN (SELECT uuid FROM kept_element) ORDER BY rowid"),
		 QStringLiteral("CREATE TEMP TABLE kept_terminal AS SELECT * FROM terminal "
						"WHERE element_uuid IN (SELECT uuid FROM kept_element) ORDER BY rowid"),
		 QStringLiteral("CREATE TEMP TABLE kept_conductor AS SELECT * FROM conductor "
						"WHERE diagram_uuid IN (SELECT uuid FROM kept_folio) ORDER BY rowid"),
		 QStringLiteral("CREATE TEMP TABLE kept_link AS SELECT * FROM link "
						"WHERE element_uuid IN (SELECT uuid FROM kept_element) ORDER BY rowid")}) {
		if (!query.exec(sql)) {
			qDebug() << "projectDataBase::keepUnbuiltFolioRows error : " << query.lastError();
		}
	}
}

/**
	@brief projectDataBase::restoreKeptRows
	Put back into @p table the rows kept for @p folio, not built
	(keepUnbuiltFolioRows()), in the order they had. @p where selects them
	in the kept table, with :folio for the folio's uuid.
*/
void projectDataBase::restoreKeptRows(const QString &table, Diagram *folio, const QString &where)
{
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("INSERT INTO %1 SELECT * FROM kept_%1 WHERE %2 ORDER BY rowid")
				  .arg(table, where));
	query.bindValue(QStringLiteral(":folio"), folio->uuid().toString());
	if (!query.exec()) {
		qDebug() << "projectDataBase::restoreKeptRows error : " << table << query.lastError();
	}
}

/**
	@brief projectDataBase::dropKeptRows
	The rows kept by keepUnbuiltFolioRows() are back: drop their copies
*/
void projectDataBase::dropKeptRows()
{
	if (m_kept_folios.isEmpty()) {
		return;
	}
	QSqlQuery query(m_data_base);
	for (const char *table : {"kept_folio", "kept_element", "kept_element_info",
							  "kept_terminal", "kept_conductor", "kept_link"})
		query.exec(QStringLiteral("DROP TABLE %1").arg(QLatin1String(table)));
	m_kept_folios.clear();
}

/**
	@brief projectDataBase::updateDB
	updateDB() for a project just read from @p document.

	The diagram, diagram_info, element, element_info, terminal, conductor
	and link tables are filled from the document itself when it carries everything
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
		m_rows_from_document = true;
		populateDrawingItemTables();
		flushConductorProperties();
		m_content_changed = false;
		emit dataBaseUpdated();
		return;
	}
	qInfo().noquote() << "Project database filled from the folios:" << why;
	updateDB();
}

/**
	@brief projectDataBase::documentRefusal
	@return why populateFromDocument() will refuse @p document, for the
	reasons it gives that the document alone can tell -- a folio, symbol
	or wire with no saved uuid, two symbols sharing one, a folio number
	using %autonum -- or an empty string. The other reasons need the
	project's definitions or every folio's links; a document refused for
	one of those is still filled from the folios, only later.
*/
QString projectDataBase::documentRefusal(const QDomDocument &document)
{
	if (qEnvironmentVariableIntValue("QET_DATABASE_FROM_FOLIOS") == 1) {
		return QStringLiteral("QET_DATABASE_FROM_FOLIOS is set");
	}
	QSet<QUuid> element_uuids;
	const QDomNodeList diagram_nodes = document.elementsByTagName(QStringLiteral("diagram"));
	for (int i = 0 ; i < diagram_nodes.size() ; ++i)
	{
		QDomElement diagram_xml = diagram_nodes.at(i).toElement();
		if (QUuid(diagram_xml.attribute(QStringLiteral("uuid"))).isNull()) {
			return QStringLiteral("a folio has no saved uuid");
		}
		if (diagram_xml.attribute(QStringLiteral("folio")).contains(QStringLiteral("%autonum"))) {
			return QStringLiteral("a folio number uses %autonum");
		}
		for (QDomElement element_xml : QET::findInDomElement(
				 diagram_xml, QStringLiteral("elements"), QStringLiteral("element")))
		{
				//Skipped by Diagram::fromXml() as well
			if (!Element::valideXml(element_xml)) {
				continue;
			}
			const QUuid uuid(element_xml.attribute(QStringLiteral("uuid")));
			if (uuid.isNull() || element_uuids.contains(uuid)) {
				return QStringLiteral("an element has no saved uuid, or shares one");
			}
			element_uuids.insert(uuid);
		}
		for (QDomElement conductor_xml : QET::findInDomElement(
				 diagram_xml, QStringLiteral("conductors"), QStringLiteral("conductor")))
		{
			if (Conductor::valideXml(conductor_xml)
				&& (QUuid(conductor_xml.attribute(QStringLiteral("uuid"))).isNull()
					|| !conductor_xml.hasAttribute(QStringLiteral("element1"))
					|| !conductor_xml.hasAttribute(QStringLiteral("element2")))) {
				return QStringLiteral("a conductor has no saved uuid, or names its ends the older way");
			}
		}
	}
	return QString();
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
	QString location;
	autonum::sequentialNumbers sequence;
	DiagramContext informations;
	QString label;
	QHash<QUuid, DocumentTerminal> terminals;
	QList<QPair<QString, int>> links;   //linked uuid, group index
};

	//The sequential values an element or a conductor was saved with, as
	//Element::fromXml() and Conductor::fromXml() read them -- false for
	//the attributes files written before <sequentialNumbers> carry.
bool readSequence(const QDomElement &item, autonum::sequentialNumbers *sequence)
{
	for (const char *name : {"sequ_1", "sequf_1", "seqt_1", "seqtf_1", "seqh_1", "seqhf_1"}) {
		if (item.hasAttribute(QLatin1String(name))) {
			return false;
		}
	}
	sequence->fromXml(item.firstChildElement(QStringLiteral("sequentialNumbers")));
	return true;
}

	//The sequential values an element was saved with, as Element::fromXml()
	//reads them, the older attributes included
autonum::sequentialNumbers elementSequence(const QDomElement &item)
{
	autonum::sequentialNumbers sequence;
	if (readSequence(item, &sequence)) {
		return sequence;
	}
	auto load = [&item](const QString &prefix, QStringList *list) {
		for (int i = 1 ; !item.attribute(prefix + QString::number(i)).isEmpty() ; ++i)
			list->append(item.attribute(prefix + QString::number(i)));
	};
	load(QStringLiteral("sequ_"),  &sequence.unit);
	load(QStringLiteral("sequf_"), &sequence.unit_folio);
	load(QStringLiteral("seqt_"),  &sequence.ten);
	load(QStringLiteral("seqtf_"), &sequence.ten_folio);
	load(QStringLiteral("seqh_"),  &sequence.hundred);
	load(QStringLiteral("seqhf_"), &sequence.hundred_folio);
	return sequence;
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
	Fill the diagram, diagram_info, element, element_info, terminal,
	conductor and link tables from @p document, the project as read from its file,
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
	terminals alike, sequential numbers are saved as the attributes
	older files carry, or a link is listed by one side only, joins kinds
	that cannot link, or gives a contact or a folio report a second
	partner. A file saved by a current QElectroTech carries
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
		//Built or not (lazy folios): only their uuids are read, and the
		//conductors of those built
	const QList<Diagram *> diagrams = m_project->folios();
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
	QSet<QUuid> unjoined;
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
			element.location = element_xml.attribute(QStringLiteral("type"));
			element.sequence = elementSequence(element_xml);
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

			for (const QDomElement &link : QET::findInDomElement(
					 element_xml, QStringLiteral("links_uuids"), QStringLiteral("link_uuid"))) {
				element.links << qMakePair(QUuid(link.attribute(QStringLiteral("uuid"))).toString(),
										   link.attribute(QStringLiteral("group_index"),
														  QStringLiteral("-1")).toInt());
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
						//Building the folio leaves this wire out and
						//says so (Diagram::wiresNotReconnected())
					unjoined.insert(diagram_uuid);
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

		//The links the folios make (Element::initLink()): a link to an
		//element the project does not have is dropped there too. Anything
		//else they would resolve one way or another -- a link only one side
		//lists, between kinds that cannot link, or a second coil for a
		//contact -- is left to them.
	QHash<QString, int> element_index;
	for (int i = 0 ; i < elements.size() ; ++i) {
		element_index.insert(elements.at(i).uuid, i);
	}
	auto canLink = [](const QString &a, const QString &b) {
		return (a == QLatin1String("master") && b == QLatin1String("slave"))
			|| (a == QLatin1String("slave") && b == QLatin1String("master"))
			|| (a == QLatin1String("next_report") && b == QLatin1String("previous_report"))
			|| (a == QLatin1String("previous_report") && b == QLatin1String("next_report"));
	};
	for (DocumentElement &element : elements)
	{
		QList<QPair<QString, int>> kept;
		QSet<QString> seen;
		for (const auto &link : std::as_const(element.links))
		{
			const int other = element_index.value(link.first, -1);
			if (other < 0) {
				continue;
			}
			const DocumentElement &partner = elements.at(other);
			bool listed_back = false;
			for (const auto &back : partner.links) {
				listed_back |= back.first == element.uuid;
			}
			if (seen.contains(link.first) || !listed_back
				|| !canLink(element.type, partner.type)) {
				return refuse(QStringLiteral("a link is not one the folios would make as saved"));
			}
			seen.insert(link.first);
			kept << link;
		}
		if (kept.size() > 1 && element.type != QLatin1String("master")) {
			return refuse(QStringLiteral("a link is not one the folios would make as saved"));
		}
		element.links = kept;
	}

		//Everything could be read: write it.
	QSqlQuery query(m_data_base);
	for (const QString &table : {QStringLiteral("diagram"), QStringLiteral("diagram_info"),
								 QStringLiteral("element"), QStringLiteral("element_info"),
								 QStringLiteral("conductor"), QStringLiteral("terminal"),
								 QStringLiteral("link")}) {
		query.exec(QStringLiteral("DELETE FROM ") + table);
	}
	m_dirty_link_elements.clear();
	m_moved_elements.clear();

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
		m_insert_elements_query.bindValue(QStringLiteral(":definition"), element.location);
		m_insert_elements_query.bindValue(QStringLiteral(":sequence"), sequenceValue(element.sequence));
		if (!m_insert_elements_query.exec()) {
			qDebug() << "projectDataBase::populateFromDocument element insert error : " << m_insert_elements_query.lastError();
		}
		bindElementInfoValues(m_insert_element_info_query, element.uuid, element.informations,
							  element.label);
		if (!m_insert_element_info_query.exec()) {
			qDebug() << "projectDataBase::populateFromDocument element_info insert error : " << m_insert_element_info_query.lastError();
		}
	}

	query.prepare(QStringLiteral("INSERT INTO link (element_uuid, linked_uuid, group_index, ord) "
								 "VALUES (:element_uuid, :linked_uuid, :group_index, :ord)"));
	for (const DocumentElement &element : std::as_const(elements)) {
		int ord = 0;
		for (const auto &link : element.links) {
			query.bindValue(QStringLiteral(":element_uuid"), element.uuid);
			query.bindValue(QStringLiteral(":linked_uuid"), link.first);
			query.bindValue(QStringLiteral(":group_index"), link.second >= 0 ? QVariant(link.second) : QVariant());
			query.bindValue(QStringLiteral(":ord"), ord++);
			if (!query.exec()) {
				qDebug() << "projectDataBase::populateFromDocument link insert error : " << query.lastError();
			}
		}
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
	m_folios_with_unjoined_wires = unjoined;
	return true;
}

/**
	@brief projectDataBase::foliosWithUnjoinedWires
	@return the uuids of the folios whose document has a wire that building
	them will leave out, an end of it not being found: those whose
	Diagram::wiresNotReconnected() will not be empty. Known when the
	database was last filled from the document, else empty.
*/
QSet<QUuid> projectDataBase::foliosWithUnjoinedWires() const
{
	return m_folios_with_unjoined_wires;
}

/**
	@brief projectDataBase::elementTypesOnFolios
	@return the kind (ElementData::typeToString()) of every symbol on the
	folios @p folios, by uuid, read from the element table as it is: for
	folios not built yet (lazy folios) its rows came from the document
	and nothing can have changed them. A symbol whose definition was not
	found has no row.
*/
QHash<QUuid, QString> projectDataBase::elementTypesOnFolios(const QSet<QUuid> &folios) const
{
	QHash<QUuid, QString> types;
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT uuid, type FROM element WHERE diagram_uuid = :folio"));
	for (const QUuid &folio : folios) {
		query.bindValue(QStringLiteral(":folio"), folio.toString());
		if (!query.exec()) {
			continue;
		}
		while (query.next()) {
			types.insert(QUuid(query.value(0).toString()), query.value(1).toString());
		}
	}
	return types;
}

/**
	@brief projectDataBase::symbolsOnFolios
	@return the symbols on the folios @p folios, folio by folio in that
	order and on each in the order of its rows -- the order the folio
	places them -- read from the element table as it is (see
	elementTypesOnFolios()). A symbol whose definition was not found has
	no row.
*/
QList<projectDataBase::FolioSymbol> projectDataBase::symbolsOnFolios(const QList<QUuid> &folios) const
{
	QList<FolioSymbol> symbols;
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT uuid, type, definition, sequence FROM element "
								 "WHERE diagram_uuid = :folio ORDER BY rowid"));
	for (const QUuid &folio : folios) {
		query.bindValue(QStringLiteral(":folio"), folio.toString());
		if (!query.exec()) {
			continue;
		}
		while (query.next()) {
			symbols.append({QUuid(query.value(0).toString()), folio, query.value(1).toString(),
							query.value(2).toString(), sequenceFromValue(query.value(3))});
		}
	}
	return symbols;
}

/**
	@brief projectDataBase::wiresOnFolios
	@return the wires on the folios @p folios, folio by folio in that order,
	read from the conductor table as it is (see elementTypesOnFolios())
*/
QList<projectDataBase::FolioWire> projectDataBase::wiresOnFolios(const QList<QUuid> &folios) const
{
	QList<FolioWire> wires;
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT c.text, e1.type, e2.type FROM conductor c "
								 "LEFT JOIN element e1 ON e1.uuid = c.terminal1_element_uuid "
								 "LEFT JOIN element e2 ON e2.uuid = c.terminal2_element_uuid "
								 "WHERE c.diagram_uuid = :folio ORDER BY c.rowid"));
	for (const QUuid &folio : folios) {
		query.bindValue(QStringLiteral(":folio"), folio.toString());
		if (!query.exec()) {
			continue;
		}
		while (query.next()) {
			wires.append({query.value(0).toString(), query.value(1).toString(),
						  query.value(2).toString()});
		}
	}
	return wires;
}

/**
	@brief projectDataBase::elementLabel
	@return the label the symbol @p element shows, read from the element
	info table as it is (see elementTypesOnFolios())
*/
QString projectDataBase::elementLabel(const QUuid &element) const
{
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT label FROM element_info WHERE element_uuid = :uuid"));
	query.bindValue(QStringLiteral(":uuid"), element.toString());
	return query.exec() && query.next() ? query.value(0).toString() : QString();
}

/**
	@brief projectDataBase::terminalName
	@return the name of the terminal @p terminal of the symbol @p element,
	read from the terminal table as it is (see elementTypesOnFolios()): a
	terminal a wire ends on has a row
*/
QString projectDataBase::terminalName(const QUuid &terminal, const QUuid &element) const
{
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT name FROM terminal WHERE uuid = :uuid AND element_uuid = :element"));
	query.bindValue(QStringLiteral(":uuid"), terminal.toString());
	query.bindValue(QStringLiteral(":element"), element.toString());
	return query.exec() && query.next() ? query.value(0).toString() : QString();
}

/**
	@brief projectDataBase::potentialWires
	@return the wires at the potential of @p wire, @p wire included, read
	from the conductor, element and link tables as they are (see
	elementTypesOnFolios()), followed as Conductor::relatedPotentialConductors()
	follows them over every folio: through a wire's ends, from a folio
	report to the terminals of the first symbol it is linked to, and from a
	terminal block's terminal to its others. Nothing when a terminal block
	on the way isolates its terminals (their groups are in its definition,
	not in the database) or @p wire has no row.
*/
std::optional<QSet<QUuid>> projectDataBase::potentialWires(const QUuid &wire) const
{
	using Node = std::pair<QString, QString>;   //element uuid, terminal uuid
	QHash<QString, std::pair<Node, Node>> ends;
	QHash<Node, QStringList> wires_at;
	QHash<QString, QList<Node>> nodes_of;
	{
		QSqlQuery query(m_data_base);
		query.exec(QStringLiteral("SELECT uuid, terminal1_element_uuid, terminal1_uuid, "
								  "terminal2_element_uuid, terminal2_uuid FROM conductor"));
		while (query.next()) {
			const Node a{query.value(1).toString(), query.value(2).toString()};
			const Node b{query.value(3).toString(), query.value(4).toString()};
			const QString uuid = query.value(0).toString();
			ends.insert(uuid, {a, b});
			for (const Node &n : {a, b}) {
				wires_at[n] << uuid;
				if (!nodes_of.value(n.first).contains(n)) nodes_of[n.first] << n;
			}
		}
	}
	const auto start = ends.constFind(wire.toString());
	if (start == ends.constEnd()) {
		return std::nullopt;
	}
	QHash<QString, ElementData::Type> kinds;
	{
		QSqlQuery query(m_data_base);
		query.exec(QStringLiteral("SELECT uuid, type FROM element"));
		while (query.next())
			kinds.insert(query.value(0).toString(), ElementData::typeFromString(query.value(1).toString()));
	}
	QSet<QUuid> wires;
	QSet<Node> seen;
	QList<Node> todo{start->first, start->second};
	while (!todo.isEmpty()) {
		const Node node = todo.takeLast();
		if (seen.contains(node)) continue;
		seen.insert(node);
		for (const QString &w : wires_at.value(node)) {
			if (wires.contains(QUuid(w))) continue;
			wires.insert(QUuid(w));
			const auto e = ends.value(w);
			todo << e.first << e.second;
		}
		const ElementData::Type kind = kinds.value(node.first, ElementData::Simple);
		if (kind & ElementData::AllReport) {
			const QList<QUuid> partners = linkedSymbols(QUuid(node.first));
			if (!partners.isEmpty())
				todo << nodes_of.value(partners.first().toString());
		} else if (kind & ElementData::Terminal) {
			if (QET::infoFlagIsTrue(m_element_information.value(QUuid(node.first))
									.value(QStringLiteral("potential_isolating")).toString()))
				return std::nullopt;
			todo << nodes_of.value(node.first);
		}
	}
	return wires;
}

/**
	@brief projectDataBase::linkedSymbols
	@return the symbols @p element is linked to, read from the link table
	as it is (see elementTypesOnFolios())
*/
QList<QUuid> projectDataBase::linkedSymbols(const QUuid &element) const
{
	QList<QUuid> linked;
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT linked_uuid FROM link WHERE element_uuid = :uuid ORDER BY ord, rowid"));
	query.bindValue(QStringLiteral(":uuid"), element.toString());
	if (query.exec()) {
		while (query.next())
			linked.append(QUuid(query.value(0).toString()));
	}
	return linked;
}

/**
	@brief projectDataBase::sequenceValue
	@return @p sequence as the element table's sequence column holds it:
	each part the file would save (sequentialNumbers::toXml()), by its tag,
	as JSON; null when the file would save none. sequenceFromValue() reads
	it back as reading the file would (sequentialNumbers::fromXml()).
*/
QVariant projectDataBase::sequenceValue(const autonum::sequentialNumbers &sequence)
{
	QDomDocument document;
	const QDomElement xml = sequence.toXml(document);
	if (!xml.hasChildNodes()) {
		return QVariant();
	}
	QJsonObject parts;
	for (QDomElement part = xml.firstChildElement() ; !part.isNull() ;
		 part = part.nextSiblingElement())
		parts.insert(part.tagName(), part.text());
	return QString::fromUtf8(QJsonDocument(parts).toJson(QJsonDocument::Compact));
}

/**
	@brief projectDataBase::sequenceFromValue
	@return the sequential numbers held by @p value (sequenceValue())
*/
autonum::sequentialNumbers projectDataBase::sequenceFromValue(const QVariant &value)
{
	autonum::sequentialNumbers sequence;
	if (value.isNull()) {
		return sequence;
	}
	const QJsonObject parts = QJsonDocument::fromJson(value.toString().toUtf8()).object();
	QDomDocument document;
	QDomElement xml = document.createElement(QStringLiteral("sequentialNumbers"));
	for (auto it = parts.constBegin() ; it != parts.constEnd() ; ++it) {
		QDomElement part = document.createElement(it.key());
		part.appendChild(document.createTextNode(it.value().toString()));
		xml.appendChild(part);
	}
	sequence.fromXml(xml);
	return sequence;
}

/**
	@brief projectDataBase::canEditUnbuilt
	@return true if the information of the symbol @p element can be changed
	through the store alone, its folio not built (lazy folios): it is
	placed once, on a folio not built, and nothing else shows its
	information -- no symbol built (alone or with its folio) holds it, no
	partner is linked to it, and it has no label formula, which could read
	what changes
*/
bool projectDataBase::canEditUnbuilt(const QUuid &element) const
{
	if (!hasElementInformation(element) || placedElementCount(element) != 0
			|| !linkedSymbols(element).isEmpty()
			|| !m_element_information.value(element).value(QStringLiteral("formula")).toString().isEmpty()) {
		return false;
	}
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT COUNT(*) FROM element WHERE uuid = :uuid"));
	query.bindValue(QStringLiteral(":uuid"), element.toString());
	return query.exec() && query.next() && query.value(0).toInt() == 1;
}

/**
	@brief projectDataBase::editUnbuiltElementInformation
	Change the information of the symbol @p element, on a folio not built
	(canEditUnbuilt()), to @p information, as Element::setElementInformations()
	would: the store and the symbol's element info row. Building the symbol
	takes it (takeUnbuiltEdit()); saving builds its folio first
	(foliosWithUnbuiltEdits()).
*/
void projectDataBase::editUnbuiltElementInformation(const QUuid &element, DiagramContext information)
{
	if (information.contains(QStringLiteral("formula_id"))
			&& information.value(QStringLiteral("formula")).toString().isEmpty()) {
		information.remove(QStringLiteral("formula_id"));
	}
	QUuid folio;
	{
		QSqlQuery query(m_data_base);
		query.prepare(QStringLiteral("SELECT diagram_uuid FROM element WHERE uuid = :uuid"));
		query.bindValue(QStringLiteral(":uuid"), element.toString());
		if (query.exec() && query.next()) folio = QUuid(query.value(0).toString());
	}
		//Whether the file has a block of information for it: a symbol
		//saves one when it has any (Element::toXml())
	const bool had_information = m_unbuilt_edits.contains(element)
			? m_unbuilt_edits.value(element).had_information
			: !m_element_information.value(element).keys().isEmpty();
	m_unbuilt_edits.insert(element, {information, folio, had_information});
	if (!storeElementInformation(element, information)) {
		return;
	}
	m_content_changed = true;
	if (!m_update_blocked) {
		bindElementInfoValues(m_update_element_query, element.toString(), information,
							  information.value(QStringLiteral("label")).toString());
		if (!m_update_element_query.exec()) {
			qDebug() << "projectDataBase::editUnbuiltElementInformation update error : " << m_update_element_query.lastError();
		}
	}
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::takeUnbuiltEdit
	@return the information @p element was given while its folio was not
	built (editUnbuiltElementInformation()), forgotten here: the symbol
	being built takes it. Nothing if it was given none.
*/
std::optional<DiagramContext> projectDataBase::takeUnbuiltEdit(const QUuid &element)
{
	const auto edit = m_unbuilt_edits.constFind(element);
	if (edit == m_unbuilt_edits.constEnd()) {
		return std::nullopt;
	}
	const DiagramContext information = edit->information;
	m_unbuilt_edits.erase(edit);
	return information;
}

/**
	@brief projectDataBase::foliosWithUnbuiltEdits
	@return the folios holding a symbol whose information was changed while
	they were not built (editUnbuiltElementInformation())
*/
QSet<QUuid> projectDataBase::foliosWithUnbuiltEdits() const
{
	QSet<QUuid> folios;
	for (const auto &edit : m_unbuilt_edits) folios.insert(edit.folio);
	for (const auto &edit : m_unbuilt_wire_edits) folios.insert(edit.folio);
	return folios;
}

/**
	@brief projectDataBase::unbuiltWire
	@return true if the wire @p wire is on a folio not built (lazy folios):
	it has a row and its properties are stored, and no wire built holds its
	uuid. @p folio is set to its folio.
*/
bool projectDataBase::unbuiltWire(const QUuid &wire, QUuid *folio) const
{
	if (placedConductorCount(wire) != 0 || !hasConductorProperties(wire)) {
		return false;
	}
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT diagram_uuid FROM conductor WHERE uuid = :uuid"));
	query.bindValue(QStringLiteral(":uuid"), wire.toString());
	if (!query.exec() || !query.next()) {
		return false;
	}
	if (folio) *folio = QUuid(query.value(0).toString());
	return !query.next();
}

/**
	@brief projectDataBase::editUnbuiltConductorProperties
	Give the wire @p wire, on a folio not built (unbuiltWire()), the
	properties @p properties as Conductor::setProperties() would: the store,
	its rows and its conductor row's text. Building it takes them
	(takeUnbuiltWireEdit()); a .qetz saves them in its database (QETProject::
	toXml()), anything else builds its folio to save.
*/
void projectDataBase::editUnbuiltConductorProperties(const QUuid &wire, const ConductorProperties &properties)
{
	QUuid folio;
	if (!unbuiltWire(wire, &folio)) {
		return;
	}
	m_unbuilt_wire_edits.insert(wire, {properties, folio});
	if (!storeConductorProperties(wire, properties)) {
		return;
	}
	m_content_changed = true;
	if (!m_update_blocked) {
		m_update_conductor_query.bindValue(QStringLiteral(":uuid"), wire.toString());
		m_update_conductor_query.bindValue(QStringLiteral(":text"), properties.text);
		if (!m_update_conductor_query.exec()) {
			qDebug() << "projectDataBase::editUnbuiltConductorProperties update error : " << m_update_conductor_query.lastError();
		}
	}
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::takeUnbuiltWireEdit
	@return the properties @p wire was given while its folio was not built,
	forgotten here: the wire being built takes them
*/
std::optional<ConductorProperties> projectDataBase::takeUnbuiltWireEdit(const QUuid &wire)
{
	const auto edit = m_unbuilt_wire_edits.constFind(wire);
	if (edit == m_unbuilt_wire_edits.constEnd()) {
		return std::nullopt;
	}
	const ConductorProperties properties = edit->properties;
	m_unbuilt_wire_edits.erase(edit);
	return properties;
}

/**
	@brief projectDataBase::unbuiltWireEditsOnFolio
	@return the properties given to the wires of the folio @p folio while it
	was not built, by wire
*/
QHash<QUuid, ConductorProperties> projectDataBase::unbuiltWireEditsOnFolio(const QUuid &folio) const
{
	QHash<QUuid, ConductorProperties> edits;
	for (auto it = m_unbuilt_wire_edits.cbegin() ; it != m_unbuilt_wire_edits.cend() ; ++it)
		if (it->folio == folio) edits.insert(it.key(), it->properties);
	return edits;
}

/**
	@brief projectDataBase::setPlacedConductorProperties
	Give the wire @p wire its properties @p properties: through the wire when
	it is built, through the store when it is not (unbuiltWire()). What
	undoing and redoing a change of wires not built does, the wires having
	perhaps been built since.
	@return false if neither can
*/
bool projectDataBase::setPlacedConductorProperties(const QUuid &wire, const ConductorProperties &properties)
{
	const QList<QPointer<Conductor>> placed = m_placed_conductors.values(wire);
	if (placed.size() == 1 && placed.first()) {
		placed.first()->setProperties(properties);
		return true;
	}
	if (placed.isEmpty() && unbuiltWire(wire)) {
		editUnbuiltConductorProperties(wire, properties);
		return true;
	}
	return false;
}

/**
	@brief projectDataBase::unbuiltEditsOnFolio
	@return the information changed through the store of the symbols on the
	folio @p folio, not built (editUnbuiltElementInformation()), by symbol.
	@p need_build is set when a .qetz cannot save one in place of the rows it
	was read with: the file has no block of information for the symbol, or
	the change empties it (a symbol with none saves no block)
*/
QHash<QUuid, DiagramContext> projectDataBase::unbuiltEditsOnFolio(const QUuid &folio, bool *need_build) const
{
	QHash<QUuid, DiagramContext> edits;
	if (need_build) *need_build = false;
	for (auto it = m_unbuilt_edits.cbegin() ; it != m_unbuilt_edits.cend() ; ++it) {
		if (it->folio != folio) continue;
		edits.insert(it.key(), it->information);
		if (need_build && (!it->had_information || it->information.keys().isEmpty()))
			*need_build = true;
	}
	return edits;
}

/**
	@brief projectDataBase::setPlacedElementInformation
	Give the symbol @p element its information @p information: through the
	symbol when it is built, through the store when it is not
	(canEditUnbuilt()). What undoing and redoing a change of a symbol not
	built does, the symbol having perhaps been built since.
	@return false if neither can
*/
bool projectDataBase::setPlacedElementInformation(const QUuid &element, const DiagramContext &information)
{
	const QList<QPointer<Element>> placed = m_placed_elements.values(element);
	if (placed.size() == 1 && placed.first()) {
		placed.first()->setElementInformations(information);
		elementInfoChanged(placed.first().data());
		return true;
	}
	if (placed.isEmpty() && m_unbuilt_edits.contains(element)) {
		editUnbuiltElementInformation(element, information);
		return true;
	}
	if (placed.isEmpty() && canEditUnbuilt(element)) {
		editUnbuiltElementInformation(element, information);
		return true;
	}
	return false;
}

/**
	@brief projectDataBase::folioUuidsOfElements
	@return the uuids of the folios the symbols @p elements are on, read
	from the element table as it is, without bringing it up to date: for a
	symbol on a folio not built yet (lazy folios) its row came from the
	document and nothing can have changed it.
*/
QSet<QUuid> projectDataBase::folioUuidsOfElements(const QSet<QUuid> &elements) const
{
	QSet<QUuid> folios;
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT diagram_uuid FROM element WHERE uuid = :uuid"));
	for (const QUuid &uuid : elements) {
		query.bindValue(QStringLiteral(":uuid"), uuid.toString());
		if (query.exec() && query.next()) {
			folios.insert(QUuid(query.value(0).toString()));
		}
	}
	return folios;
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
	@brief projectDataBase::setBuildingFolio
	@see the declaration. The stores still take what building writes.
*/
void projectDataBase::setBuildingFolio(bool building)
{
		//A folio built while another is (a caller of diagrams() reached
		//from building it) nests: only the outermost restores.
	if (building) {
		if (m_building_folios++ == 0) {
			m_changed_before_folio = m_content_changed;
			m_blocked_before_folio = m_update_blocked;
			m_update_blocked = true;
		}
	} else if (m_building_folios > 0 && --m_building_folios == 0) {
		m_update_blocked = m_blocked_before_folio;
		m_content_changed = m_changed_before_folio;
	}
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
			*error = projectDataBase::tr("The query is empty.");
		}
		return false;
	}

	if (trimmed.contains(QLatin1Char(';'))) {
		if (error) {
			*error = projectDataBase::tr("Only a single SELECT query is allowed "
								  "(the ';' character may only appear at "
								  "the very end).");
		}
		return false;
	}

	const int first_space = trimmed.indexOf(QRegularExpression(QStringLiteral("\\s")));
	const QString first_word = (first_space == -1 ? trimmed : trimmed.left(first_space)).toUpper();
	if (first_word != QLatin1String("SELECT") && first_word != QLatin1String("WITH")) {
		if (error) {
			*error = projectDataBase::tr("Only read-only queries (SELECT or "
								  "WITH ... SELECT) are allowed.");
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
		//The same goes for link rows, elements' folio cells and wires'
		//properties, see linksChanged(), elementMoved() and
		//storeConductorProperties().
		//The drawing tables hold what only built items know (a text's
		//laid-out size, a picture's pixels): with folios not built yet
		//(lazy folios) a read of them builds those first, and so does a
		//read of the view made of them. Every other table was filled from
		//the document.
	static const QRegularExpression drawing_tables(
			QStringLiteral("\\b(shape|independent_text|image|drawing_item_view)\\b"),
			QRegularExpression::CaseInsensitiveOption);
	if (m_project && m_project->unloadedFolioCount() && drawing_tables.match(query).hasMatch()) {
		m_project->loadFolios();
	}
	flushDrawingItems();
	flushLinks();
	flushElementPositions();
	flushConductorProperties();

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

		//A folio not built (lazy folios) has none: reading a wire
		//drops it unless both its ends are found (Conductor::fromXml())
	int count = 0;
	for (auto *diagram : m_project->builtFolios())
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

	placeElement(element);
	connect(element, &Element::linkedElementChanged,
			this, &projectDataBase::linksChanged, Qt::UniqueConnection);
	connect(element, &QGraphicsObject::xChanged, this, &projectDataBase::elementMoved, Qt::UniqueConnection);
	connect(element, &QGraphicsObject::yChanged, this, &projectDataBase::elementMoved, Qt::UniqueConnection);
	writeElementRows(element);
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::removeElement
	@param element
*/
void projectDataBase::removeElement(Element *element)
{
	m_content_changed = true;
	const QUuid uuid = element->uuid();
	removeElementRows(uuid);
	unplaceElement(element, uuid);
		//Another symbol still holds the uuid (F100): the rows are its own.
	if (Element *other = otherHolder(uuid, element)) {
		writeElementRows(other);
	}
	emit dataBaseUpdated();
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

/**
	@brief projectDataBase::spareTerminalsChanged
	The spare terminals of a terminal strip (on no folio) were added, removed
	or changed : fill the spare_terminal table again.
*/
void projectDataBase::spareTerminalsChanged()
{
	populateSpareTerminalTable();
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::populateSpareTerminalTable
	Fill the spare_terminal table with the spare terminals of the terminal
	strips of the project, lines of the nomenclature like the terminals
	drawn on the folios (see createElementNomenclatureView()).
*/
void projectDataBase::populateSpareTerminalTable()
{
	QSqlQuery query_(m_data_base);
	query_.exec(QStringLiteral("DELETE FROM spare_terminal"));
	if (!m_project) {
		return;
	}

	const auto keys_ = QETInformation::elementInfoKeys();
	QString insert_(QStringLiteral("INSERT INTO spare_terminal (uuid, diagram_position, title, folio, position"));
	QString values_(QStringLiteral(" VALUES (:uuid, :diagram_position, :title, :folio, :position"));
	for (const auto &key_ : keys_) {
		insert_ += QStringLiteral(", ") + key_;
		values_ += QStringLiteral(", :") + key_;
	}
	query_.prepare(insert_ + QStringLiteral(")") + values_ + QStringLiteral(")"));

		//After the folios, strip by strip
	int strip_index_ = 0;
	for (const auto &strip_ : m_project->terminalStrip())
	{
		++strip_index_;
		const auto physical_ = strip_->physicalTerminal();
		for (int pos_ = 0 ; pos_ < physical_.size() ; ++pos_)
		{
			for (const auto &real_t : physical_.at(pos_)->realTerminals())
			{
				if (!real_t || !real_t->isSpare()) {
					continue;
				}
				const auto informations_ = real_t->spareData().m_informations;
				query_.bindValue(QStringLiteral(":uuid"), real_t->elementUuid().toString());
				query_.bindValue(QStringLiteral(":diagram_position"), 1000000 + strip_index_);
				query_.bindValue(QStringLiteral(":title"), QVariant());
				query_.bindValue(QStringLiteral(":folio"), QVariant());
				query_.bindValue(QStringLiteral(":position"), pos_);
				for (const auto &key_ : keys_) {
					query_.bindValue(QStringLiteral(":") + key_, informations_.value(key_).toString());
				}
				if (!query_.exec()) {
					qDebug() << "projectDataBase::populateSpareTerminalTable insert error : " << query_.lastError();
				}
			}
		}
	}
}

void projectDataBase::addDiagram(Diagram *diagram)
{
	m_content_changed = true;
	placeFolio(diagram);
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
		//So do its symbols, their wires and the links to and from them
		//(F107).
	const QList<Element *> elements = diagram->elements();
	const bool own_transaction = !elements.isEmpty() && m_data_base.transaction();
		//One signal for the folio, not one per symbol and wire: each makes
		//every folio table query the database again.
	const bool blocked = blockSignals(true);
	for (Element *element : elements) {
		addElement(element);
		queueLinks(element);
		for (Element *linked : element->linkedElements())
			queueLinks(linked);
	}
	for (Conductor *conductor : diagram->conductors())
		addConductor(conductor);
	blockSignals(blocked);
	if (own_transaction)
		m_data_base.commit();

		//The folios after the new one moved down, and a folio number made
		//from %id or %total changed on every folio.
	updateFolioPositions();
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::elementMoved
	The sender() element moved: its folio cell is written again by the
	next flushElementPositions().
*/
void projectDataBase::elementMoved()
{
	auto *element = qobject_cast<Element *>(sender());
	if (element && !m_moved_elements.contains(element)) {
		m_moved_elements << element;
	}
}

/**
	@brief projectDataBase::flushElementPositions
	Write the folio cell of every element queued by elementMoved()
*/
void projectDataBase::flushElementPositions()
{
	if (m_moved_elements.isEmpty()) {
		return;
	}

	const auto moved = m_moved_elements;
	m_moved_elements.clear();
	const bool own_transaction = m_data_base.transaction();
	QSqlQuery update(m_data_base);
	update.prepare(QStringLiteral("UPDATE element SET pos = :pos WHERE uuid = :uuid"));
	for (const QPointer<Element> &element : moved)
	{
		if (!element || !element->diagram()) {
			continue;
		}
		update.bindValue(QStringLiteral(":pos"),
						 element->diagram()->convertPosition(element->scenePos()).toString());
		update.bindValue(QStringLiteral(":uuid"), element->uuid().toString());
		if (!update.exec()) {
			qDebug() << "projectDataBase::flushElementPositions error : " << update.lastError();
		}
	}
	if (own_transaction) {
		m_data_base.commit();
	}
}

/**
	@brief projectDataBase::updateFolioPositions
	Write every folio's position and folio number again, after a folio was
	added, removed or moved. A folio not built yet (lazy folios) has
	its position and title block already: it is not built for this.
*/
void projectDataBase::updateFolioPositions()
{
	for (auto diagram : m_project->folios())
	{
		m_diagram_order_changed.bindValue(":pos", m_project->folioIndex(diagram)+1);
		m_diagram_order_changed.bindValue(":uuid", diagram->uuid().toString());
		if (!m_diagram_order_changed.exec()) {
			qDebug() << "projectDataBase::updateFolioPositions position error : " << m_diagram_order_changed.lastError();
		}

		m_diagram_info_order_changed.bindValue(":folio", diagram->border_and_titleblock.titleblockInformation().value("folio"));
		m_diagram_info_order_changed.bindValue(":uuid", diagram->uuid().toString());
		if (!m_diagram_info_order_changed.exec()) {
			qDebug() << "projectDataBase::updateFolioPositions folio error : " << m_diagram_info_order_changed.lastError();
		}
	}
}

void projectDataBase::removeDiagram(Diagram *diagram)
{
	m_content_changed = true;
	const QString uuid_str = diagram->uuid().toString();
	unplaceFolio(diagram, diagram->uuid());
	for (Conductor *conductor : diagram->conductors())
		unplaceConductor(conductor, conductor->uuid());
		//Its symbols are no longer placed (they live on in the undo stack)
	for (Element *element : diagram->elements())
		unplaceElement(element, element->uuid());

		//Order matters: element_info and terminal are scoped through a
		//subquery on element, so they must run before element itself is
		//deleted below. The whole cascade runs in one transaction and is
		//rolled back on the first error, so a mid-cascade failure (e.g. a
		//locked DB) can't leave the diagram row deleted while its
		//element/terminal/element_info/conductor rows survive.
	m_data_base.transaction();

	QSqlQuery cascade_links(m_data_base);
	cascade_links.prepare(QStringLiteral(
			"DELETE FROM link WHERE element_uuid IN (SELECT uuid FROM element WHERE diagram_uuid = :uuid) "
			"OR linked_uuid IN (SELECT uuid FROM element WHERE diagram_uuid = :uuid)"));
	cascade_links.bindValue(QStringLiteral(":uuid"), uuid_str);
	if (!cascade_links.exec()) {
		qDebug() << "projectDataBase::removeDiagram link cascade error : "
				 << cascade_links.lastError();
		m_data_base.rollback();
		return;
	}

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

	QSqlQuery remove_info(m_data_base);
	remove_info.prepare(QStringLiteral("DELETE FROM diagram_info WHERE diagram_uuid = :uuid"));
	remove_info.bindValue(QStringLiteral(":uuid"), uuid_str);
	if (!remove_info.exec()) {
		qDebug() << "projectDataBase::removeDiagram diagram_info delete error : " << remove_info.lastError();
		m_data_base.rollback();
		return;
	}

	m_remove_diagram_query.bindValue(":uuid", uuid_str);
	if (!m_remove_diagram_query.exec()) {
		qDebug() << "projectDataBase::removeDiagram delete error : " << m_remove_diagram_query.lastError();
		m_data_base.rollback();
		return;
	}

	m_data_base.commit();
		//The folios after it moved up, and %id / %total changed.
	updateFolioPositions();
	emit dataBaseUpdated();
}

void projectDataBase::diagramInfoChanged(Diagram *diagram)
{
	m_content_changed = true;
		//Not while the project is being read: a folio's header read gives
		//it a folio number counted from the folios read so far ("1/1" for
		//the first of three); the rows are filled once every folio is
		//read, from the document or from the index a .qetz was saved with
	if (m_update_blocked) {
		return;
	}
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

	placeConductor(conductor);
	watchConductor(conductor);
	writeConductorRow(conductor);
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::writeConductorRow
	Write the conductor row of @p conductor, a placed wire, and the rows of
	the terminals it ends on. A uuid that already has a row keeps it, as in
	a full rebuild; nothing is written while updates are blocked (the
	rebuild that ends them writes every row).
*/
void projectDataBase::writeConductorRow(Conductor *conductor)
{
	if (m_update_blocked) {
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

	bindConductorValues(m_insert_conductor_query, conductor, conductor->diagram());
	if (!m_insert_conductor_query.exec()) {
		qDebug() << "projectDataBase::writeConductorRow insert error : " << m_insert_conductor_query.lastError();
	}
}

/**
	@brief projectDataBase::removeConductor
	@param conductor
*/
void projectDataBase::removeConductor(Conductor *conductor)
{
	m_content_changed = true;
	const QUuid uuid = conductor->uuid();
	removeConductorRow(uuid);
	unplaceConductor(conductor, uuid);
		//Another wire still holds the uuid: the row is its own.
	if (Conductor *other = otherConductorHolder(uuid, conductor)) {
		writeConductorRow(other);
	}
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::removeConductorRow
	Remove the conductor row of @p uuid, and the rows of the terminals it
	ended on unless another conductor ends there; nothing while updates are
	blocked, as writeConductorRow().
*/
void projectDataBase::removeConductorRow(const QUuid &uuid)
{
	if (m_update_blocked) {
		return;
	}
		//The terminal table lists the terminals a conductor ends on: the
		//conductor's two ends go with it unless another conductor ends there.
	QList<QPair<QString, QString>> ends;
	QSqlQuery read_ends(m_data_base);
	read_ends.prepare(QStringLiteral("SELECT terminal1_uuid, terminal1_element_uuid, "
									 "terminal2_uuid, terminal2_element_uuid "
									 "FROM conductor WHERE uuid = :uuid"));
	read_ends.bindValue(QStringLiteral(":uuid"), uuid.toString());
	if (read_ends.exec() && read_ends.next()) {
		ends << qMakePair(read_ends.value(0).toString(), read_ends.value(1).toString())
			 << qMakePair(read_ends.value(2).toString(), read_ends.value(3).toString());
	}

	m_remove_conductor_query.bindValue(":uuid", uuid.toString());
	if (!m_remove_conductor_query.exec()) {
		qDebug() << "projectDataBase::removeConductorRow delete error : " << m_remove_conductor_query.lastError();
		return;
	}

	QSqlQuery remove_end(m_data_base);
	remove_end.prepare(QStringLiteral(
			"DELETE FROM terminal WHERE uuid = :uuid AND element_uuid = :element_uuid "
			"AND NOT EXISTS (SELECT 1 FROM conductor WHERE "
			"(terminal1_uuid = :uuid AND terminal1_element_uuid = :element_uuid) OR "
			"(terminal2_uuid = :uuid AND terminal2_element_uuid = :element_uuid))"));
	for (const auto &end : std::as_const(ends)) {
		remove_end.bindValue(QStringLiteral(":uuid"), end.first);
		remove_end.bindValue(QStringLiteral(":element_uuid"), end.second);
		if (!remove_end.exec()) {
			qDebug() << "projectDataBase::removeConductorRow terminal delete error : " << remove_end.lastError();
		}
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
			//Also changes the height of the text, kept in the row
		connect(text, &IndependentTextItem::textWidthChanged, this, &projectDataBase::drawingItemChanged, unique);
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
	if (!diagram || !m_project || !m_project->folios().contains(diagram)) {
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
			//NULL for the automatic width
		query->bindValue(QStringLiteral(":text_width"), text->textWidth() > 0
						 ? QVariant(text->textWidth()) : QVariant());
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
		//already, and a full rebuild runs on every load. A folio not built
		//yet (lazy folios) has none: its items write their rows as it
		//is built.
	for (auto diagram : m_project->folios())
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
						  "definition TEXT,"
						  "sequence TEXT,"
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

		//Create the element_information table: the information of every
		//symbol placed in the project, in full -- each key with its value
		//and whether it is shown -- the store symbols write through
		//(storeElementInformation()). Unlike element_info, which has one
		//column per known key and is filled again on every rebuild, this
		//is kept by the edits themselves.
	if (!query_.exec(QStringLiteral(
			"CREATE TABLE element_information ("
			"element_uuid VARCHAR(50) NOT NULL, "
			"ord INTEGER NOT NULL, "
			"name TEXT NOT NULL, "
			"value TEXT, "
			"show INTEGER NOT NULL, "
			"PRIMARY KEY (element_uuid, ord))"))) {
		qDebug() << " element_information_table query : " << query_.lastError();
	}
		//Create the folio_titleblock table: the title block properties of
		//every folio, as a folio saves them, the store folios write
		//through (storeFolioTitleBlock()). One row per property, then one
		//per additional field ("custom:" and its name).
	if (!query_.exec(QStringLiteral(
			"CREATE TABLE folio_titleblock ("
			"diagram_uuid VARCHAR(50) NOT NULL, "
			"ord INTEGER NOT NULL, "
			"name TEXT NOT NULL, "
			"value TEXT, "
			"show INTEGER NOT NULL, "
			"PRIMARY KEY (diagram_uuid, ord))"))) {
		qDebug() << " folio_titleblock_table query : " << query_.lastError();
	}

		//Create the conductor_properties table: the properties of every
		//wire placed in the project, as the attributes a saved wire carries,
		//the store wires write through (storeConductorProperties()).
	if (!query_.exec(QStringLiteral(
			"CREATE TABLE conductor_properties ("
			"conductor_uuid VARCHAR(50) NOT NULL, "
			"name TEXT NOT NULL, "
			"value TEXT, "
			"PRIMARY KEY (conductor_uuid, name))"))) {
		qDebug() << " conductor_properties_table query : " << query_.lastError();
	}


		//Create the link table: one row per element and element it is
		//linked to -- a coil and its contacts, a pair of folio reports --
		//from each side, with the contact group the element saved for it,
		//and its place (ord) in the order the element saves its links.
	const QString link_table(QStringLiteral(
			"CREATE TABLE link ("
			"element_uuid VARCHAR(50) NOT NULL, "
			"linked_uuid VARCHAR(50) NOT NULL, "
			"group_index INTEGER, "
			"ord INTEGER NOT NULL DEFAULT 0, "
			"PRIMARY KEY (element_uuid, linked_uuid), "
			"FOREIGN KEY (element_uuid) REFERENCES element (uuid))"));
	if (!query_.exec(link_table)) {
		qDebug() << " link_table query : " << query_.lastError();
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
				"text TEXT, rotation REAL, text_width REAL, "
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

		//The spare terminals of the terminal strips : terminals on no folio,
		//lines of the nomenclature like the terminals drawn on the folios
		//(see createElementNomenclatureView()). One column by information,
		//like element_info.
	QString spare_terminal_table(QStringLiteral(
			"CREATE TABLE spare_terminal(uuid VARCHAR(50) PRIMARY KEY NOT NULL, "
			"diagram_position INTEGER, title VARCHAR(100), folio VARCHAR(100), position INTEGER"));
	for (const auto &key_ : QETInformation::elementInfoKeys()) {
		spare_terminal_table += QStringLiteral(", ") + key_ + QStringLiteral(" VARCHAR(100)");
	}
	spare_terminal_table += QStringLiteral(")");
	if (!query_.exec(spare_terminal_table)) {
		qDebug() << " spare_terminal_table query : " << query_.lastError();
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

		//The spare terminals of the terminal strips (on no folio) are lines of
		//the nomenclature too : the same columns, read in the spare_terminal
		//table. They come after the folios (diagram_position), in the order
		//of their strip (position).
	{
		const QString select_ = QStringLiteral(" AS SELECT ");
		const auto begin_ = create_view.indexOf(select_) + select_.size();
		const auto end_ = create_view.indexOf(QStringLiteral(" FROM element_info ei"));
		QString columns_ = create_view.mid(begin_, end_ - begin_);
		columns_.replace(QStringLiteral("ei."), QStringLiteral("st."));
		columns_.replace(QStringLiteral("d.pos AS diagram_position"), QStringLiteral("st.diagram_position AS diagram_position"));
		columns_.replace(QStringLiteral("e.type AS element_type"), QStringLiteral("'terminal' AS element_type"));
		columns_.replace(QStringLiteral("e.sub_type AS element_sub_type"), QStringLiteral("NULL AS element_sub_type"));
		columns_.replace(QStringLiteral("di.title AS title"), QStringLiteral("st.title AS title"));
		columns_.replace(QStringLiteral("di.folio AS folio"), QStringLiteral("st.folio AS folio"));
		columns_.replace(QStringLiteral("e.pos AS position"), QStringLiteral("st.position AS position"));
		create_view += QStringLiteral(" UNION ALL SELECT ") + columns_ +
					   QStringLiteral(" FROM spare_terminal st"
									  " WHERE COALESCE(LOWER(TRIM(st.exclude_from_bom)), '') NOT IN ('true', '1', 'yes', 'on')");
	}

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

		//The folios' own data: folios() builds nothing
	for (auto diagram : m_project->folios())
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
	m_moved_elements.clear();
	QSqlQuery query_(m_data_base);
	query_.exec("DELETE FROM element");

	for (auto diagram : m_project->folios())
	{
		if (m_kept_folios.contains(diagram)) {
			restoreKeptRows(QStringLiteral("element"), diagram, QStringLiteral("diagram_uuid = :folio"));
			continue;
		}
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
	Populate the element info table from the symbol information store
	in the order of the element table.
	The label column is the label a symbol shows, which its folio's
	numbering decides: the symbol holding the uuid gives it.

	A project where several symbols share a uuid (F100) is read from its
	folios as before: the first symbol on them owns the row, and the store
	does not know which one that is.
*/
void projectDataBase::populateElementInfoTable()
{
	QSqlQuery query(m_data_base);
	query.exec(QStringLiteral("DELETE FROM element_info"));

	for (auto it = m_placed_elements.constBegin() ; it != m_placed_elements.constEnd() ; ++it) {
		if (placedElementCount(it.key()) > 1) {
			populateElementInfoTableFromFolios();
			return;
		}
	}

	QSqlQuery uuids(m_data_base);
	if (!uuids.exec(QStringLiteral("SELECT uuid FROM element ORDER BY rowid"))) {
		qDebug() << "projectDataBase::populateElementInfoTable select error : " << uuids.lastError();
		return;
	}
	while (uuids.next())
	{
		const QString uuid_str = uuids.value(0).toString();
		const QUuid uuid(uuid_str);
		Element *holder = otherHolder(uuid, nullptr);
		if (!holder) {
				//A symbol on a folio not built: its row as it was
			if (!m_kept_folios.isEmpty()) {
				QSqlQuery kept(m_data_base);
				kept.prepare(QStringLiteral("INSERT INTO element_info SELECT * FROM kept_element_info "
											"WHERE element_uuid = :uuid"));
				kept.bindValue(QStringLiteral(":uuid"), uuid_str);
				kept.exec();
			}
			continue;
		}
		bindElementInfoValues(m_insert_element_info_query, uuid_str,
							  m_element_information.value(uuid), holder->actualLabel());
		if (!m_insert_element_info_query.exec()) {
			qDebug() << "projectDataBase::populateElementInfoTable insert error : " << m_insert_element_info_query.lastError();
		}
	}
}

/**
	@brief projectDataBase::populateElementInfoTableFromFolios
	populateElementInfoTable() from the symbols on the folios
*/
void projectDataBase::populateElementInfoTableFromFolios()
{
	for (const auto &diagram : m_project->folios())
	{
			//A folio not built: its rows as they were
		if (m_kept_folios.contains(diagram)) {
			restoreKeptRows(QStringLiteral("element_info"), diagram,
							QStringLiteral("element_uuid IN (SELECT uuid FROM kept_element "
										   "WHERE diagram_uuid = :folio)"));
			continue;
		}
		const ElementProvider ep(diagram);
		for (const auto &elmt : ep.find(allElementTypes()))
		{
			bindElementInfoValues(m_insert_element_info_query, elmt);
			if (!m_insert_element_info_query.exec()) {
				qDebug() << "projectDataBase::populateElementInfoTable insert error : " << m_insert_element_info_query.lastError();
			}
		}
	}
}

/**
	@brief projectDataBase::elementInformation
	@return the stored information of the placed symbol @p element, empty
	if it has none
*/
DiagramContext projectDataBase::elementInformation(const QUuid &element) const
{
	return m_element_information.value(element);
}

bool projectDataBase::hasElementInformation(const QUuid &element) const
{
	return m_element_information.contains(element);
}

int projectDataBase::placedElementCount(const QUuid &element) const
{
		//Without values(), which makes a list: this is asked on every read
		//of a placed symbol's information.
	int n = 0;
	for (auto it = m_placed_elements.constFind(element) ;
		 it != m_placed_elements.constEnd() && it.key() == element ; ++it)
		if (it.value()) ++n;
	return n;
}

/**
	@brief projectDataBase::storeElementInformation
	Store @p information as the information of the placed symbol @p element,
	in the element_information table and the cache in front of it.
	@return true if that changed what was stored
*/
bool projectDataBase::storeElementInformation(const QUuid &element, const DiagramContext &information)
{
	if (element.isNull()) return false;
	auto known = m_element_information.constFind(element);
	if (known != m_element_information.constEnd() && *known == information) return false;
	m_element_information.insert(element, information);

	const QString uuid = element.toString();
	m_store_remove_query.bindValue(QStringLiteral(":uuid"), uuid);
	if (!m_store_remove_query.exec()) {
		qDebug() << "projectDataBase::storeElementInformation remove error : " << m_store_remove_query.lastError();
	}
	QSqlQuery &insert = m_store_insert_query;
	int ord = 0;
	for (const QString &key : information.keys()) {
		insert.bindValue(QStringLiteral(":uuid"), uuid);
		insert.bindValue(QStringLiteral(":ord"), ord++);
		insert.bindValue(QStringLiteral(":name"), key);
		insert.bindValue(QStringLiteral(":value"), information.value(key).toString());
		insert.bindValue(QStringLiteral(":show"), information.keyMustShow(key) ? 1 : 0);
		if (!insert.exec()) {
			qDebug() << "projectDataBase::storeElementInformation insert error : " << insert.lastError();
		}
	}
	return true;
}

/**
	@brief projectDataBase::elementInformationChanged
	The placed symbol @p element now has @p information: the store keeps
	it, and its element_info row follows (F106: a pasted symbol's label is
	erased or numbered after its row is written, by
	Element::setElementInformations(), which tells no table).
	A uuid other symbols share (F100) keeps the row of the first of them.
*/
void projectDataBase::elementInformationChanged(Element *element, const DiagramContext &information)
{
	if (!storeElementInformation(element->uuid(), information)) {
		return;
	}
	m_content_changed = true;
	if (m_update_blocked || placedElementCount(element->uuid()) != 1) {
		return;
	}
	const auto hash = elementInfoToString(element);
	for (const auto &key : QETInformation::elementInfoKeys()) {
		m_update_element_query.bindValue(QStringLiteral(":") + key, hash.value(key));
	}
	m_update_element_query.bindValue(QStringLiteral(":uuid"), element->uuid().toString());
	if (!m_update_element_query.exec()) {
		qDebug() << "projectDataBase::elementInformationChanged update error : " << m_update_element_query.lastError();
	}
}

/**
	@brief projectDataBase::forgetElementInformation
	The symbol @p element is no longer placed: its stored information goes.
*/
void projectDataBase::forgetElementInformation(const QUuid &element)
{
	if (!m_element_information.remove(element)) return;
	m_store_remove_query.bindValue(QStringLiteral(":uuid"), element.toString());
	if (!m_store_remove_query.exec()) {
		qDebug() << "projectDataBase::forgetElementInformation error : " << m_store_remove_query.lastError();
	}
}

/**
	@brief projectDataBase::placeElement
	@p element is now placed: its information is stored under its uuid.
*/
void projectDataBase::placeElement(Element *element)
{
	if (!m_placed_elements.contains(element->uuid(), element))
		m_placed_elements.insert(element->uuid(), element);
	storeElementInformation(element->uuid(), element->ownInformations());
}

/**
	@brief projectDataBase::unplaceElement
	@p element no longer holds @p uuid. If another placed symbol still does
	(the original of a copy), the row is written again from it; otherwise
	it goes.
*/
void projectDataBase::unplaceElement(Element *element, const QUuid &uuid)
{
	m_placed_elements.remove(uuid, element);
	for (const QPointer<Element> &other : m_placed_elements.values(uuid)) {
		if (other) {
				//Its own copy: the store still holds this symbol's
			storeElementInformation(uuid, other->ownInformations());
			return;
		}
	}
	m_placed_elements.remove(uuid);
	forgetElementInformation(uuid);
}

/**
	@brief projectDataBase::elementUuidChanged
	The uuid of the placed symbol @p element changed from @p old_uuid (a
	paste or a folio copy renews it): its information and its element,
	element_info and link rows follow it. A copy is placed while it still
	carries its original's uuid, so its rows could not be written then
	(the uuid was taken): they are written now. The old uuid keeps its rows
	only if another symbol still holds it (F106).
*/
void projectDataBase::elementUuidChanged(Element *element, const QUuid &old_uuid)
{
	if (!m_placed_elements.contains(old_uuid, element) || old_uuid == element->uuid()) return;
	m_content_changed = true;
	Element *other = otherHolder(old_uuid, element);
	unplaceElement(element, old_uuid);
	removeElementRows(old_uuid);
	if (other) {
			//The old uuid's rows may have been written from this symbol
			//while both held it: write them again from the one keeping it.
		writeElementRows(other);
		queueLinks(other);
		for (Element *linked : other->linkedElements())
			queueLinks(linked);
	}
	placeElement(element);
	writeElementRows(element);
		//Its wires' rows name their ends' symbols: write them again
		//(a pasted wire is placed before its symbols get their uuids).
	for (Conductor *conductor : element->conductors()) {
		if (placedConductorCount(conductor->uuid()) == 1) {
			removeConductorRow(conductor->uuid());
			writeConductorRow(conductor);
		}
	}
	queueLinks(element);
	for (Element *linked : element->linkedElements())
		queueLinks(linked);
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::otherHolder
	@return a placed symbol other than @p element holding @p uuid, or
	nullptr. Two hold one uuid while a copy is being placed, and for good
	in files several symbols share a uuid in (F100).
*/
Element *projectDataBase::otherHolder(const QUuid &uuid, const Element *element) const
{
	for (const QPointer<Element> &other : m_placed_elements.values(uuid))
		if (other && other != element) return other;
	return nullptr;
}

/**
	@brief projectDataBase::writeElementRows
	Write the element and element_info rows of @p element, a placed symbol.
	A uuid that already has rows keeps them: the first symbol to hold a
	uuid owns its rows, as in a full rebuild.

	Nothing is written while updates are blocked (a project being opened,
	a paste being read): the rebuild that ends them writes every row.
*/
void projectDataBase::writeElementRows(Element *element)
{
	if (m_update_blocked) {
		return;
	}
	bindElementValues(m_insert_elements_query, element, element->diagram());
	if (!m_insert_elements_query.exec()) {
		qDebug() << "projectDataBase::writeElementRows insert element error : " << m_insert_elements_query.lastError();
	}
	bindElementInfoValues(m_insert_element_info_query, element);
	if (!m_insert_element_info_query.exec()) {
		qDebug() << "projectDataBase::writeElementRows insert element info error : " << m_insert_element_info_query.lastError();
	}
}

/**
	@brief projectDataBase::removeElementRows
	Remove the element, element_info and link rows of @p uuid; nothing
	while updates are blocked, as writeElementRows().
*/
void projectDataBase::removeElementRows(const QUuid &uuid)
{
	if (m_update_blocked) {
		return;
	}
	const QString uuid_str = uuid.toString();
	m_remove_element_query.bindValue(QStringLiteral(":uuid"), uuid_str);
	if (!m_remove_element_query.exec()) {
		qDebug() << "projectDataBase::removeElementRows remove error : " << m_remove_element_query.lastError();
	}
	m_remove_element_info_query.bindValue(QStringLiteral(":uuid"), uuid_str);
	if (!m_remove_element_info_query.exec()) {
		qDebug() << "projectDataBase::removeElementRows remove element_info error : " << m_remove_element_info_query.lastError();
	}
	m_remove_links_query.bindValue(QStringLiteral(":uuid"), uuid_str);
	if (!m_remove_links_query.exec()) {
		qDebug() << "projectDataBase::removeElementRows remove link error : " << m_remove_links_query.lastError();
	}
}

/**
	@brief projectDataBase::elementInformationMismatches
	Compare the store with every placed symbol, both ways: a symbol whose
	information differs or is missing, and a stored row no symbol has.
*/
QStringList projectDataBase::elementInformationMismatches() const
{
	QStringList out;
	QSet<QUuid> placed;
	for (Diagram *diagram : m_project->diagrams()) {
		for (Element *element : diagram->elements()) {
			placed.insert(element->uuid());
				//A uuid several symbols share has one row and cannot agree
				//with them all; those symbols answer from their own copy.
			if (placedElementCount(element->uuid()) > 1)
				continue;
			if (!m_element_information.contains(element->uuid()))
				out << element->uuid().toString() + QStringLiteral(": not stored");
			else if (m_element_information.value(element->uuid()) != element->elementInformations())
				out << element->uuid().toString() + QStringLiteral(": differs");
		}
	}
	for (auto it = m_element_information.constBegin() ; it != m_element_information.constEnd() ; ++it)
		if (!placed.contains(it.key()))
			out << it.key().toString() + QStringLiteral(": stored, not placed");
	QSqlQuery rows(m_data_base);
	if (rows.exec(QStringLiteral("SELECT count(*) FROM element_information")) && rows.next()) {
		int expected = 0;
		for (const DiagramContext &dc : m_element_information) expected += int(dc.keys().size());
		if (rows.value(0).toInt() != expected)
			out << QStringLiteral("table holds %1 rows, the cache %2").arg(rows.value(0).toInt()).arg(expected);
	}
	return out;
}

/**
	@brief projectDataBase::folioTitleBlock
	@return the stored title block properties of the folio @p folio
*/
TitleBlockProperties projectDataBase::folioTitleBlock(const QUuid &folio) const
{
	return m_folio_titleblocks.value(folio);
}

/**
	@brief projectDataBase::storedFolioTitleBlock
	@return the stored title block properties of the folio @p folio, or
	nullptr; valid until the store next changes
*/
const TitleBlockProperties *projectDataBase::storedFolioTitleBlock(const QUuid &folio) const
{
	auto it = m_folio_titleblocks.constFind(folio);
	return it == m_folio_titleblocks.constEnd() ? nullptr : &it.value();
}

/**
	@brief projectDataBase::placedFolio
	@return the folio of the project holding @p folio, or nullptr
*/
Diagram *projectDataBase::placedFolio(const QUuid &folio) const
{
	return m_placed_folios.value(folio);
}

/**
	@brief projectDataBase::storeFolioTitleBlock
	Store @p properties as the title block of the folio @p folio, in the
	folio_titleblock table and the cache in front of it.
*/
void projectDataBase::storeFolioTitleBlock(const QUuid &folio, const TitleBlockProperties &properties)
{
	if (folio.isNull()) return;
	auto known = m_folio_titleblocks.find(folio);
	if (known != m_folio_titleblocks.end() && *known == properties) return;
	m_folio_titleblocks.insert(folio, properties);

	const QString uuid = folio.toString();
	QSqlQuery remove(m_data_base);
	remove.prepare(QStringLiteral("DELETE FROM folio_titleblock WHERE diagram_uuid = :uuid"));
	remove.bindValue(QStringLiteral(":uuid"), uuid);
	if (!remove.exec()) {
		qDebug() << "projectDataBase::storeFolioTitleBlock remove error : " << remove.lastError();
	}
	QSqlQuery insert(m_data_base);
	insert.prepare(QStringLiteral("INSERT INTO folio_titleblock (diagram_uuid, ord, name, value, show) "
								  "VALUES (:uuid, :ord, :name, :value, :show)"));
	int ord = 0;
	auto row = [&](const QString &name, const QString &value, bool show) {
		insert.bindValue(QStringLiteral(":uuid"), uuid);
		insert.bindValue(QStringLiteral(":ord"), ord++);
		insert.bindValue(QStringLiteral(":name"), name);
		insert.bindValue(QStringLiteral(":value"), value);
		insert.bindValue(QStringLiteral(":show"), show ? 1 : 0);
		if (!insert.exec()) {
			qDebug() << "projectDataBase::storeFolioTitleBlock insert error : " << insert.lastError();
		}
	};
	row(QStringLiteral("title"), properties.title, true);
	row(QStringLiteral("author"), properties.author, true);
	row(QStringLiteral("date"), properties.date.isValid()
			? properties.date.toString(Qt::ISODate) : QString(), true);
	row(QStringLiteral("filename"), properties.filename, true);
	row(QStringLiteral("plant"), properties.plant, true);
	row(QStringLiteral("locmach"), properties.locmach, true);
	row(QStringLiteral("indexrev"), properties.indexrev, true);
	row(QStringLiteral("version"), properties.version, true);
	row(QStringLiteral("folio"), properties.folio, true);
	row(QStringLiteral("auto_page_num"), properties.auto_page_num, true);
	row(QStringLiteral("template_name"), properties.template_name, true);
	row(QStringLiteral("display_at"), properties.display_at == Qt::RightEdge
			? QStringLiteral("right") : QStringLiteral("bottom"), true);
	for (const QString &key : properties.context.keys()) {
		row(QStringLiteral("custom:") + key, properties.context.value(key).toString(),
			properties.context.keyMustShow(key));
	}
}

/**
	@brief projectDataBase::placeFolio
	@p folio is now a folio of the project: its title block is stored under
	its uuid.
*/
void projectDataBase::placeFolio(Diagram *folio)
{
	m_placed_folios.insert(folio->uuid(), folio);
	storeFolioTitleBlock(folio->uuid(), folio->border_and_titleblock.localTitleBlock());
}

/**
	@brief projectDataBase::unplaceFolio
	@p folio no longer holds @p uuid (it was removed, or its uuid changed):
	the title block stored under it goes.
*/
void projectDataBase::unplaceFolio(Diagram *folio, const QUuid &uuid)
{
	if (m_placed_folios.value(uuid) != folio) return;
	m_placed_folios.remove(uuid);
	m_folio_titleblocks.remove(uuid);
	QSqlQuery remove(m_data_base);
	remove.prepare(QStringLiteral("DELETE FROM folio_titleblock WHERE diagram_uuid = :uuid"));
	remove.bindValue(QStringLiteral(":uuid"), uuid.toString());
	if (!remove.exec()) {
		qDebug() << "projectDataBase::unplaceFolio error : " << remove.lastError();
	}
}

/**
	@brief projectDataBase::folioUuidChanged
	The uuid of the placed folio @p folio changed from @p old_uuid: its
	stored title block follows it.
*/
void projectDataBase::folioUuidChanged(Diagram *folio, const QUuid &old_uuid)
{
	if (old_uuid == folio->uuid() || m_placed_folios.value(old_uuid) != folio) return;
	unplaceFolio(folio, old_uuid);
	placeFolio(folio);
}

/**
	@brief projectDataBase::folioTitleBlockMismatches
	Compare the store with every folio's own title block, both ways.
*/
QStringList projectDataBase::folioTitleBlockMismatches() const
{
	QStringList out;
	QSet<QUuid> folios;
	for (Diagram *diagram : m_project->diagrams()) {
		folios.insert(diagram->uuid());
		if (m_placed_folios.value(diagram->uuid()) != diagram)
			out << diagram->uuid().toString() + QStringLiteral(": not stored");
		else if (TitleBlockProperties(m_folio_titleblocks.value(diagram->uuid()))
				 != diagram->border_and_titleblock.localTitleBlock())
			out << diagram->uuid().toString() + QStringLiteral(": differs");
	}
	for (auto it = m_folio_titleblocks.constBegin() ; it != m_folio_titleblocks.constEnd() ; ++it)
		if (!folios.contains(it.key()))
			out << it.key().toString() + QStringLiteral(": stored, not a folio");
	return out;
}

/**
	@brief projectDataBase::conductorProperties
	@return the stored properties of the placed wire @p conductor
*/
ConductorProperties projectDataBase::conductorProperties(const QUuid &conductor) const
{
	return m_conductor_properties.value(conductor);
}

bool projectDataBase::hasConductorProperties(const QUuid &conductor) const
{
	return m_conductor_properties.contains(conductor);
}

int projectDataBase::placedConductorCount(const QUuid &conductor) const
{
		//Without values(), as placedElementCount()
	int n = 0;
	for (auto it = m_placed_conductors.constFind(conductor) ;
		 it != m_placed_conductors.constEnd() && it.key() == conductor ; ++it)
		if (it.value()) ++n;
	return n;
}

/**
	@brief projectDataBase::storeConductorProperties
	Store @p properties as the properties of the placed wire @p conductor,
	in the conductor_properties table -- the attributes a saved wire
	carries -- and the cache in front of it.
	@return true if that changed what was stored
*/
bool projectDataBase::storeConductorProperties(const QUuid &conductor, const ConductorProperties &properties)
{
	if (conductor.isNull()) return false;
	auto known = m_conductor_properties.constFind(conductor);
	if (known != m_conductor_properties.constEnd() && *known == properties) return false;
	m_conductor_properties.insert(conductor, properties);
		//The rows are written by the next flushConductorProperties(): a
		//folio inserted or removed renumbers the wires of every folio after
		//it, one change each, and nothing reads them in between.
	m_dirty_conductor_properties.insert(conductor);
	return true;
}

/**
	@brief projectDataBase::flushConductorProperties
	Write the conductor_properties rows of every wire changed since the last
	flush; only the attributes that changed.
*/
void projectDataBase::flushConductorProperties()
{
	if (m_dirty_conductor_properties.isEmpty()) {
		return;
	}
	const QSet<QUuid> dirty = m_dirty_conductor_properties;
	m_dirty_conductor_properties.clear();
	const bool own_transaction = m_data_base.transaction();
	for (const QUuid &conductor : dirty)
	{
		const QString uuid = conductor.toString();
		QHash<QString, QString> attributes;
		auto known = m_conductor_properties.constFind(conductor);
		if (known != m_conductor_properties.constEnd()) {
			for (const auto &attribute : known->attributes())
				attributes.insert(attribute.first, attribute.second);
		}
		QHash<QString, QString> &rows = m_conductor_attributes[conductor];
		for (auto it = rows.constBegin() ; it != rows.constEnd() ; ++it) {
			if (attributes.contains(it.key())) continue;
			m_conductor_property_remove_query.bindValue(QStringLiteral(":uuid"), uuid);
			m_conductor_property_remove_query.bindValue(QStringLiteral(":name"), it.key());
			if (!m_conductor_property_remove_query.exec()) {
				qDebug() << "projectDataBase::flushConductorProperties remove error : " << m_conductor_property_remove_query.lastError();
			}
		}
		for (auto it = attributes.constBegin() ; it != attributes.constEnd() ; ++it) {
			auto row = rows.constFind(it.key());
			if (row != rows.constEnd() && *row == it.value()) continue;
			m_conductor_properties_insert_query.bindValue(QStringLiteral(":uuid"), uuid);
			m_conductor_properties_insert_query.bindValue(QStringLiteral(":name"), it.key());
			m_conductor_properties_insert_query.bindValue(QStringLiteral(":value"), it.value());
			if (!m_conductor_properties_insert_query.exec()) {
				qDebug() << "projectDataBase::flushConductorProperties insert error : " << m_conductor_properties_insert_query.lastError();
			}
		}
		if (attributes.isEmpty())
			m_conductor_attributes.remove(conductor);
		else
			rows = attributes;

			//The conductor row's text, if the row is this wire's alone
		if (known != m_conductor_properties.constEnd() && !m_update_blocked
			&& placedConductorCount(conductor) == 1) {
			m_update_conductor_query.bindValue(QStringLiteral(":uuid"), uuid);
			m_update_conductor_query.bindValue(QStringLiteral(":text"), known->text);
			if (!m_update_conductor_query.exec()) {
				qDebug() << "projectDataBase::flushConductorProperties update error : " << m_update_conductor_query.lastError();
			}
		}
	}
	if (own_transaction) {
		m_data_base.commit();
	}
}

/**
	@brief projectDataBase::conductorPropertiesStored
	The placed wire @p conductor now has @p properties: the store keeps
	them, and its conductor row's text follows at the next flush (a formula
	is numbered by Conductor::refreshText(), which tells no table).
*/
void projectDataBase::conductorPropertiesStored(Conductor *conductor, const ConductorProperties &properties)
{
	if (storeConductorProperties(conductor->uuid(), properties)) {
		m_content_changed = true;
	}
}

/**
	@brief projectDataBase::placeConductor
	@p conductor is now placed: its properties are stored under its uuid.
*/
void projectDataBase::placeConductor(Conductor *conductor)
{
	if (!m_placed_conductors.contains(conductor->uuid(), conductor))
		m_placed_conductors.insert(conductor->uuid(), conductor);
	storeConductorProperties(conductor->uuid(), conductor->ownProperties());
}

/**
	@brief projectDataBase::unplaceConductor
	@p conductor no longer holds @p uuid. If another placed wire still does
	(the original of a copy), its properties are stored again; otherwise
	they go.
*/
void projectDataBase::unplaceConductor(Conductor *conductor, const QUuid &uuid)
{
	m_placed_conductors.remove(uuid, conductor);
	if (Conductor *other = otherConductorHolder(uuid, nullptr)) {
		storeConductorProperties(uuid, other->ownProperties());
		return;
	}
	m_placed_conductors.remove(uuid);
	if (m_conductor_properties.remove(uuid))
		m_dirty_conductor_properties.insert(uuid);
}

/**
	@brief projectDataBase::otherConductorHolder
	@return a placed wire other than @p conductor holding @p uuid, or nullptr
*/
Conductor *projectDataBase::otherConductorHolder(const QUuid &uuid, const Conductor *conductor) const
{
	for (const QPointer<Conductor> &other : m_placed_conductors.values(uuid))
		if (other && other != conductor) return other;
	return nullptr;
}

/**
	@brief projectDataBase::conductorUuidChanged
	The uuid of the placed wire @p conductor changed from @p old_uuid (a
	paste renews it, a file gives it): its properties and its row follow
	it. A copy is placed while it still carries its original's uuid, so its
	row could not be written then; the original's is written again.
*/
void projectDataBase::conductorUuidChanged(Conductor *conductor, const QUuid &old_uuid)
{
	if (!m_placed_conductors.contains(old_uuid, conductor) || old_uuid == conductor->uuid()) return;
	m_content_changed = true;
	Conductor *other = otherConductorHolder(old_uuid, conductor);
	unplaceConductor(conductor, old_uuid);
	removeConductorRow(old_uuid);
	if (other) {
		writeConductorRow(other);
	}
	placeConductor(conductor);
	writeConductorRow(conductor);
	emit dataBaseUpdated();
}

/**
	@brief projectDataBase::conductorPropertiesMismatches
	Compare the store with every placed wire, both ways.
*/
QStringList projectDataBase::conductorPropertiesMismatches() const
{
	QStringList out;
	QSet<QUuid> placed;
	for (Diagram *diagram : m_project->diagrams()) {
		for (Conductor *conductor : diagram->conductors()) {
			placed.insert(conductor->uuid());
			if (placedConductorCount(conductor->uuid()) > 1)
				continue;
			if (!m_conductor_properties.contains(conductor->uuid()))
				out << conductor->uuid().toString() + QStringLiteral(": not stored");
			else if (m_conductor_properties.value(conductor->uuid()) != conductor->ownProperties())
				out << conductor->uuid().toString() + QStringLiteral(": differs");
		}
	}
	for (auto it = m_conductor_properties.constBegin() ; it != m_conductor_properties.constEnd() ; ++it)
		if (!placed.contains(it.key()))
			out << it.key().toString() + QStringLiteral(": stored, not placed");
	return out;
}

/**
	@brief projectDataBase::prefillFromDocument
	Fill the stores of symbol information, folio title blocks and wire
	properties from @p document, the project being opened, before its
	folios are built: what step 5 (lazy folios) needs, a folio's data
	without its scene. In a .qetz these blocks come from project.sqlite.

	Only what the document names by a uuid it holds once: a symbol, wire
	or folio without one gets its uuid while it is built, and a uuid
	several symbols share (F100) names none of them. Building then writes
	the same stores again; endPrefill() says how often it agreed.
*/
void projectDataBase::prefillFromDocument(const QDomDocument &document)
{
	m_prefilled_elements.clear();
	m_prefilled_folios.clear();
	m_prefilled_conductors.clear();
	for (PrefillTally &tally : m_prefill_tally) tally = PrefillTally();

	QHash<QUuid, int> seen;
	QHash<QUuid, DiagramContext> elements;
	QHash<QUuid, ConductorProperties> conductors;
	const QDomElement root = document.documentElement();
	for (QDomElement folio = root.firstChildElement(QStringLiteral("diagram")) ; !folio.isNull() ;
		 folio = folio.nextSiblingElement(QStringLiteral("diagram")))
	{
		const QUuid folio_uuid(folio.attribute(QStringLiteral("uuid")));
		if (!folio_uuid.isNull() && ++seen[folio_uuid] == 1) {
			TitleBlockProperties properties;
			properties.fromXml(folio);
				//What BorderTitleBlock::importTitleBlock() makes of them
			properties.version = QetVersion::displayedVersion();
			properties.collection = QET::QetCollection::Embedded;
			m_prefilled_folios.insert(folio_uuid, properties);
		}
		const QDomElement element_list = folio.firstChildElement(QStringLiteral("elements"));
		for (QDomElement element = element_list.firstChildElement(QStringLiteral("element")) ;
			 !element.isNull() ; element = element.nextSiblingElement(QStringLiteral("element")))
		{
			const QUuid uuid(element.attribute(QStringLiteral("uuid")));
			if (uuid.isNull() || ++seen[uuid] > 1) continue;
			DiagramContext information;
			information.fromXml(element.firstChildElement(QStringLiteral("elementInformations")),
								QStringLiteral("elementInformation"));
			elements.insert(uuid, information);
		}
		const QDomElement conductor_list = folio.firstChildElement(QStringLiteral("conductors"));
		for (QDomElement conductor = conductor_list.firstChildElement(QStringLiteral("conductor")) ;
			 !conductor.isNull() ; conductor = conductor.nextSiblingElement(QStringLiteral("conductor")))
		{
			const QUuid uuid(conductor.attribute(QStringLiteral("uuid")));
			if (uuid.isNull() || ++seen[uuid] > 1) continue;
			ConductorProperties properties;
			properties.fromXml(conductor);
			conductors.insert(uuid, properties);
		}
	}
		//A uuid held twice names nothing
	for (auto it = elements.constBegin() ; it != elements.constEnd() ; ++it)
		if (seen.value(it.key()) == 1) m_prefilled_elements.insert(it.key(), it.value());
	for (auto it = conductors.constBegin() ; it != conductors.constEnd() ; ++it)
		if (seen.value(it.key()) == 1) m_prefilled_conductors.insert(it.key(), it.value());
	for (auto it = m_prefilled_folios.begin() ; it != m_prefilled_folios.end() ; )
		it = seen.value(it.key()) == 1 ? std::next(it) : m_prefilled_folios.erase(it);

	for (auto it = m_prefilled_elements.constBegin() ; it != m_prefilled_elements.constEnd() ; ++it)
		storeElementInformation(it.key(), it.value());
	for (auto it = m_prefilled_folios.constBegin() ; it != m_prefilled_folios.constEnd() ; ++it)
		storeFolioTitleBlock(it.key(), it.value());
	for (auto it = m_prefilled_conductors.constBegin() ; it != m_prefilled_conductors.constEnd() ; ++it)
		storeConductorProperties(it.key(), it.value());
}

/**
	@brief projectDataBase::endPrefill
	The folios are built: compare what prefillFromDocument() stored with
	what building stored, and forget what nothing was placed for -- unless
	@p keep_unplaced, when some folios are not built yet (lazy folios):
	the store then answers for their items until they are.
*/
void projectDataBase::endPrefill(bool keep_unplaced)
{
	auto tally = [](PrefillTally &t, const QStringList &differing) {
		if (differing.isEmpty()) { ++t.agree; return; }
		++t.differ;
		for (const QString &field : differing) ++t.fields[field];
	};
	for (auto it = m_prefilled_elements.constBegin() ; it != m_prefilled_elements.constEnd() ; ++it) {
		if (!placedElementCount(it.key())) {
			++m_prefill_tally[0].unplaced;
			if (!keep_unplaced && !m_placed_elements.contains(it.key())) forgetElementInformation(it.key());
			continue;
		}
		const DiagramContext built = m_element_information.value(it.key());
		QStringList differing;
		const QList<QString> built_keys = built.keys();
		QSet<QString> names(built_keys.cbegin(), built_keys.cend());
		const QList<QString> prefilled_keys = it.value().keys();
		names.unite(QSet<QString>(prefilled_keys.cbegin(), prefilled_keys.cend()));
		for (const QString &name : names)
			if (built.value(name) != it.value().value(name)
				|| built.keyMustShow(name) != it.value().keyMustShow(name))
				differing << name;
		tally(m_prefill_tally[0], differing);
	}
	for (auto it = m_prefilled_folios.constBegin() ; it != m_prefilled_folios.constEnd() ; ++it) {
		if (!m_placed_folios.value(it.key())) {
			++m_prefill_tally[1].unplaced;
			if (!keep_unplaced) m_folio_titleblocks.remove(it.key());
			continue;
		}
		const TitleBlockProperties built = m_folio_titleblocks.value(it.key());
		const TitleBlockProperties &p = it.value();
		QStringList differing;
		if (built.title != p.title) differing << QStringLiteral("title");
		if (built.author != p.author) differing << QStringLiteral("author");
		if (built.date != p.date) differing << QStringLiteral("date");
		if (built.filename != p.filename) differing << QStringLiteral("filename");
		if (built.plant != p.plant) differing << QStringLiteral("plant");
		if (built.locmach != p.locmach) differing << QStringLiteral("locmach");
		if (built.indexrev != p.indexrev) differing << QStringLiteral("indexrev");
		if (built.version != p.version) differing << QStringLiteral("version");
		if (built.folio != p.folio) differing << QStringLiteral("folio");
		if (built.auto_page_num != p.auto_page_num) differing << QStringLiteral("auto_page_num");
		if (built.template_name != p.template_name) differing << QStringLiteral("template_name");
		if (!(built.context == p.context)) differing << QStringLiteral("context");
		if (built.display_at != p.display_at) differing << QStringLiteral("display_at");
		if (built.collection != p.collection) differing << QStringLiteral("collection");
		tally(m_prefill_tally[1], differing);
	}
	for (auto it = m_prefilled_conductors.constBegin() ; it != m_prefilled_conductors.constEnd() ; ++it) {
		if (!placedConductorCount(it.key())) {
			++m_prefill_tally[2].unplaced;
			if (!keep_unplaced && !m_placed_conductors.contains(it.key()) && m_conductor_properties.remove(it.key()))
				m_dirty_conductor_properties.insert(it.key());
			continue;
		}
		QHash<QString, QString> built, prefilled;
		for (const auto &a : m_conductor_properties.value(it.key()).attributes()) built.insert(a.first, a.second);
		for (const auto &a : it.value().attributes()) prefilled.insert(a.first, a.second);
		QStringList differing;
		QSet<QString> names(built.keyBegin(), built.keyEnd());
		names.unite(QSet<QString>(prefilled.keyBegin(), prefilled.keyEnd()));
		for (const QString &name : names)
			if (built.value(name) != prefilled.value(name)) differing << name;
		tally(m_prefill_tally[2], differing);
	}
	m_prefilled_elements.clear();
	m_prefilled_folios.clear();
	m_prefilled_conductors.clear();
}

/**
	@brief projectDataBase::prefillReport
	@return how the last prefillFromDocument() compared with building, one
	line: per store, values that agreed, differed (and in which fields),
	and were not placed
*/
QString projectDataBase::prefillReport() const
{
	static const char *names[] = {"symbols", "folios", "wires"};
	QStringList parts;
	for (int i = 0 ; i < 3 ; ++i) {
		const PrefillTally &t = m_prefill_tally[i];
		QStringList fields;
		for (auto it = t.fields.constBegin() ; it != t.fields.constEnd() ; ++it)
			fields << QStringLiteral("%1 %2").arg(it.key()).arg(it.value());
		parts << QStringLiteral("%1 %2 agree, %3 differ%4, %5 not placed")
				 .arg(QLatin1String(names[i])).arg(t.agree).arg(t.differ)
				 .arg(fields.isEmpty() ? QString() : QStringLiteral(" (") + fields.join(QStringLiteral(", ")) + QLatin1Char(')'))
				 .arg(t.unplaced);
	}
	return parts.join(QStringLiteral("; "));
}

/**
	@brief projectDataBase::populateLinkTable
	Populate the link table from the built folios
*/
void projectDataBase::populateLinkTable()
{
	m_dirty_link_elements.clear();
	const QSet<QUuid> waiting = waitingSymbols();
	QSqlQuery query(m_data_base);
	query.exec(QStringLiteral("DELETE FROM link"));
	query.prepare(QStringLiteral("INSERT INTO link (element_uuid, linked_uuid, group_index, ord) "
								 "VALUES (:element_uuid, :linked_uuid, :group_index, :ord)"));

	for (const auto &diagram : m_project->folios())
	{
			//A folio not built: its rows, but to a partner since removed
		if (m_kept_folios.contains(diagram)) {
			restoreKeptRows(QStringLiteral("link"), diagram,
							QStringLiteral("element_uuid IN (SELECT uuid FROM kept_element "
										   "WHERE diagram_uuid = :folio) "
										   "AND linked_uuid IN (SELECT uuid FROM element)"));
			continue;
		}
		const ElementProvider ep(diagram);
		for (const auto &elmt : ep.find(allElementTypes()))
			insertLinkRows(query, elmt, waiting);
	}
}

/**
	@brief projectDataBase::linksChanged
	A link of the sender() was made or undone: its rows are written again
	by the next flushLinks().
*/
void projectDataBase::linksChanged()
{
	queueLinks(qobject_cast<Element *>(sender()));
}

/**
	@brief projectDataBase::queueLinks
	The link rows of @p element are written again by the next flushLinks().
*/
void projectDataBase::queueLinks(Element *element)
{
	m_content_changed = true;
	if (element && !m_dirty_link_elements.contains(element)) {
		m_dirty_link_elements << element;
	}
}

/**
	@brief projectDataBase::linkRowMismatches
	A test and debugging aid (QET_CHECK_LINK_ROWS): the placed
	symbols whose link rows are not the links they save, in order
	(Element::linksToSave(), insertLinkRows())
	@return one entry per such symbol: its uuid
*/
QStringList projectDataBase::linkRowMismatches()
{
	flushLinks();
	const QSet<QUuid> waiting = waitingSymbols();
	QStringList mismatches;
	QSqlQuery query(m_data_base);
	query.prepare(QStringLiteral("SELECT linked_uuid, group_index FROM link "
								 "WHERE element_uuid = :uuid ORDER BY ord, rowid"));
	for (auto it = m_placed_elements.cbegin() ; it != m_placed_elements.cend() ; ++it)
	{
		Element *element = it.value();
		if (!element) {
			continue;
		}
		QList<std::pair<QString, int>> expected;
		for (const Element::SavedLink &link : element->linksToSave(waiting))
			if (!link.built || m_placed_elements.contains(link.uuid, link.built))
				expected.append({link.uuid.toString(), link.group_index});
		QList<std::pair<QString, int>> rows;
		query.bindValue(QStringLiteral(":uuid"), element->uuid().toString());
		if (query.exec())
			while (query.next())
				rows.append({query.value(0).toString(),
							 query.value(1).isNull() ? -1 : query.value(1).toInt()});
		if (rows != expected) {
			mismatches << element->uuid().toString();
		}
	}
	return mismatches;
}

/**
	@brief projectDataBase::waitingSymbols
	@return the symbols on folios not built yet (lazy folios): partners a
	symbol keeps its saved links to until they are built
*/
QSet<QUuid> projectDataBase::waitingSymbols() const
{
	return m_project && m_project->unloadedFolioCount() ? m_project->symbolsOnUnbuiltFolios()
														: QSet<QUuid>();
}

/**
	@brief projectDataBase::insertLinkRows
	Write @p element's link rows with @p insert (prepared with :element_uuid,
	:linked_uuid, :group_index and :ord): its partners as it saves them
	(Element::linksToSave()), in that order, so the rows and the file say
	the same: @p waiting is the symbols on folios not built, as a save
	takes them (waitingSymbols()). A built partner the project no longer has (on a removed
	folio, linked still for the undo) gets no row.
*/
void projectDataBase::insertLinkRows(QSqlQuery &insert, Element *element,
									 const QSet<QUuid> &waiting)
{
	int ord = 0;
	for (const Element::SavedLink &link : element->linksToSave(waiting))
	{
		if (link.built && !m_placed_elements.contains(link.uuid, link.built)) {
			continue;
		}
		insert.bindValue(QStringLiteral(":element_uuid"), element->uuid().toString());
		insert.bindValue(QStringLiteral(":linked_uuid"), link.uuid.toString());
		insert.bindValue(QStringLiteral(":group_index"), link.group_index >= 0 ? QVariant(link.group_index) : QVariant());
		insert.bindValue(QStringLiteral(":ord"), ord++);
		if (!insert.exec()) {
			qDebug() << "projectDataBase::insertLinkRows insert error : " << insert.lastError();
		}
	}
}

/**
	@brief projectDataBase::flushLinks
	Write the link rows of every element queued by linksChanged()
*/
void projectDataBase::flushLinks()
{
	if (m_dirty_link_elements.isEmpty()) {
		return;
	}

	const auto dirty = m_dirty_link_elements;
	m_dirty_link_elements.clear();
	const QSet<QUuid> waiting = waitingSymbols();
	const bool own_transaction = m_data_base.transaction();
	QSqlQuery remove(m_data_base);
	remove.prepare(QStringLiteral("DELETE FROM link WHERE element_uuid = :uuid"));
	QSqlQuery insert(m_data_base);
	insert.prepare(QStringLiteral("INSERT INTO link (element_uuid, linked_uuid, group_index, ord) "
								  "VALUES (:element_uuid, :linked_uuid, :group_index, :ord)"));
	for (const QPointer<Element> &element : dirty)
	{
		if (!element) {
			continue;
		}
		remove.bindValue(QStringLiteral(":uuid"), element->uuid().toString());
		if (!remove.exec()) {
			qDebug() << "projectDataBase::flushLinks remove error : " << remove.lastError();
		}
			//An element taken off its folio is unlinked first (Diagram::
			//removeItem()), so it writes no rows here; one on a removed
			//folio is not, and writes none either.
		if (!m_placed_elements.contains(element->uuid(), element)) {
			continue;
		}
		insertLinkRows(insert, element, waiting);
	}
	if (own_transaction) {
		m_data_base.commit();
	}
}

void projectDataBase::populateDiagramInfoTable()
{
	QSqlQuery query(m_data_base);
	query.exec("DELETE FROM diagram_info");

		//The folios' own data: folios() builds nothing
	for (auto *diagram : m_project->folios())
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

	for (auto *diagram : m_project->folios())
	{
		if (m_kept_folios.contains(diagram)) {
			restoreKeptRows(QStringLiteral("terminal"), diagram,
							QStringLiteral("element_uuid IN (SELECT uuid FROM kept_element "
										   "WHERE diagram_uuid = :folio)"));
			restoreKeptRows(QStringLiteral("conductor"), diagram, QStringLiteral("diagram_uuid = :folio"));
			continue;
		}
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

		//The symbol information store and an element's link rows
	m_store_remove_query = QSqlQuery(m_data_base);
	m_store_remove_query.prepare(QStringLiteral("DELETE FROM element_information WHERE element_uuid = :uuid"));
	m_store_insert_query = QSqlQuery(m_data_base);
	m_store_insert_query.prepare(QStringLiteral("INSERT INTO element_information (element_uuid, ord, name, value, show) "
												"VALUES (:uuid, :ord, :name, :value, :show)"));
	m_conductor_properties_insert_query = QSqlQuery(m_data_base);
	m_conductor_properties_insert_query.prepare(QStringLiteral("INSERT OR REPLACE INTO conductor_properties (conductor_uuid, name, value) "
															   "VALUES (:uuid, :name, :value)"));
	m_conductor_property_remove_query = QSqlQuery(m_data_base);
	m_conductor_property_remove_query.prepare(QStringLiteral("DELETE FROM conductor_properties WHERE conductor_uuid = :uuid AND name = :name"));
	m_remove_links_query = QSqlQuery(m_data_base);
	m_remove_links_query.prepare(QStringLiteral("DELETE FROM link WHERE element_uuid = :uuid OR linked_uuid = :uuid"));

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
											", text, rotation, text_width) VALUES (" + drawing_values +
											", :text, :rotation, :text_width)");
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
	QString insert_element_query("INSERT INTO element (uuid, diagram_uuid, pos, type, sub_type, group_uuid, definition, sequence) "
								 "VALUES (:uuid, :diagram_uuid, :pos, :type, :sub_type, :group_uuid, :definition, :sequence)");
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
	query.bindValue(QStringLiteral(":definition"), element->location().path());
	query.bindValue(QStringLiteral(":sequence"), sequenceValue(element->sequenceStruct()));
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
		caption_ = tr("Export the internal project database");
	}

	auto dir_ = dir;
	if(dir_.isEmpty()) {
		dir_ = db->project()->filePath();
		if (dir_.isEmpty()) {
			dir_ = QETApp::documentDir() % "/" % tr("untitled") % ".sqlite";
		} else {
			const QFileInfo project_file(dir_);
			dir_ = project_file.path() % "/" % project_file.completeBaseName() % ".sqlite";
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
	if (db->m_project) {
		db->m_project->loadFolios(); //the drawing tables need built folios
	}
	db->flushDrawingItems();
	db->flushLinks();
	db->flushElementPositions();
	db->flushConductorProperties();
	QSqlQuery query(db->m_data_base);
	if (!query.exec("VACUUM INTO '" % escaped_path % "'")) {
		qWarning() << "Unable to export project database:" << query.lastError().text();
	}
}
#endif
