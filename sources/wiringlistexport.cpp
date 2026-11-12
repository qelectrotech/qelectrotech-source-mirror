#include "wiringlistexport.h"
#include "qetproject.h"
#include "diagram.h"
#include "dataBase/projectdatabase.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "qetgraphicsitem/element.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDomDocument>
#include <QFile>
#include <QSet>
#include <algorithm>
#include <functional>

WiringListExport::WiringListExport(QETProject *project, QWidget *parent) :
QObject(parent),
m_project(project),
m_parent(parent)
{
}

QString WiringListExport::normalizeUuid(const QString &u) const
{
    QString res = u;
    res.remove('{').remove('}');
    return res.trimmed().toLower();
}

QString WiringListExport::findDiagramFolio(const QDomElement &diagramElem) const
{
    if (diagramElem.isNull()) return "";
    // The folio number as the folio shows it ("3/12"). The saved "folio"
    // attribute is its template ("%id/%total").
    const int index = diagramElem.attribute("order").toInt();
    if (m_project && index >= 1 && index <= m_project->folios().size()) {
        return m_project->folios().at(index - 1)->border_and_titleblock
                .titleblockInformation().value("folio").toString();
    }
    if (diagramElem.hasAttribute("folio")) return diagramElem.attribute("folio");
    if (diagramElem.hasAttribute("title")) return diagramElem.attribute("title");
    return "";
}

QDomElement WiringListExport::climbToDiagram(QDomNode node) const
{
    while (!node.isNull()) {
        if (node.isElement() && node.toElement().tagName().toLower() == "diagram") {
            return node.toElement();
        }
        node = node.parentNode();
    }
    return QDomElement();
}

/**
    The project as the export reads it: its symbol collection and its folios
    in order, each with its "order". A folio not built (lazy folios) is
    not built for it: its XML is copied, and what QElectroTech computes for
    its wires' ends -- the labels of the symbols they end on and of their
    partners, the terminals' names -- is filled in from the symbols built
    (on built folios or alone, as partners) and from the project database.
*/
QDomDocument WiringListExport::projectDocument() const
{
    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("project"));
    document.appendChild(root);
    if (XmlElementCollection *collection = m_project->embeddedElementCollection())
        root.appendChild(document.importNode(collection->root(), true));

        //A folio whose partners built alone were linked or unlinked since
        //is not what its XML says: built, as saving builds it
    for (Diagram *diagram : m_project->folios())
        if (!diagram->isLoaded() && diagram->partialChanged())
            m_project->buildFolios({diagram});

    QHash<QUuid, Element *> built;
    for (Diagram *diagram : m_project->folios()) {
        if (diagram->isLoaded()) {
            for (Element *element : diagram->elements()) built.insert(element->uuid(), element);
        } else {
            const QHash<QUuid, Element *> partial = diagram->partialElements();
            for (auto it = partial.cbegin() ; it != partial.cend() ; ++it) built.insert(it.key(), it.value());
        }
    }
    projectDataBase *database = m_project->dataBase();
    auto label = [&built, database](const QUuid &uuid) {
        Element *element = built.value(uuid);
        return element ? element->actualLabel() : database->elementLabel(uuid);
    };
        //Conductor::toXml(): the label of the last of the symbol's partners.
        //A symbol still waiting for a partner not built has its links as
        //saved, in the database
    const QSet<QUuid> waiting = m_project->unloadedFolioCount() ? m_project->symbolsOnUnbuiltFolios()
                                                                : QSet<QUuid>();
    auto linked = [&built, database, &label, &waiting](const QUuid &uuid) -> std::optional<QString> {
        Element *element = built.value(uuid);
        if (element && database->waitingPartners(uuid, waiting).isEmpty()) {
            if (element->linkedElements().isEmpty()) return std::nullopt;
            return element->linkedElements().last()->actualLabel();
        }
        const QList<QUuid> links = database->linkedSymbols(uuid);
        if (links.isEmpty()) return std::nullopt;
        return label(links.last());
    };

    const QList<Diagram *> folios = m_project->folios();
    for (int i = 0 ; i < folios.size() ; ++i) {
        Diagram *diagram = folios.at(i);
        QDomElement folio;
        const bool is_built = diagram->isLoaded();
        folio = document.importNode(is_built ? m_project->folioXml(diagram).documentElement()
                                             : diagram->deferredXml(), true).toElement();
            //A built folio's wires describe their ends but a partner not
            //built yet; a folio not built has none of it in its part (.qetz)
            //or as read, before any change (.qet)
        QDomNodeList wires = folio.elementsByTagName(QStringLiteral("conductor"));
        for (int w = 0 ; w < wires.size() ; ++w) {
            QDomElement wire = wires.at(w).toElement();
            for (const QString end : {QStringLiteral("1"), QStringLiteral("2")}) {
                const QUuid element(wire.attribute(QStringLiteral("element") + end));
                if (element.isNull()) continue;   //an end saved the older way
                if (const auto partner = linked(element))
                    wire.setAttribute(QStringLiteral("element") + end + QStringLiteral("_linked"), *partner);
                else
                    wire.removeAttribute(QStringLiteral("element") + end + QStringLiteral("_linked"));
                if (is_built) continue;
                wire.setAttribute(QStringLiteral("element") + end + QStringLiteral("_label"), label(element));
                wire.setAttribute(QStringLiteral("terminalname") + end,
                                  database->terminalName(QUuid(wire.attribute(QStringLiteral("terminal") + end)),
                                                         element));
            }
        }
        folio.setAttribute(QStringLiteral("order"), i + 1);
        root.appendChild(folio);
    }
    return document;
}

/**
    @brief mergeFields
    Two places one row spans, written the way every list writes them: the
    first one, then the second -- or just the one there is.
    @param a first place
    @param b second place
    @return both places as one string
*/
static QString mergeFields(const QString &a, const QString &b)
{
    QString at = a.trimmed();
    QString bt = b.trimmed();
    if (at.isEmpty()) return bt;
    if (bt.isEmpty()) return at;
    if (at == bt) return at;
    return at + ", " + bt;
}

/**
	@brief WiringListExport::junctionRoles
	@return the roles of every junction definition the project's embedded
	collection holds, by the path a placed symbol names its definition with.

	The roles are a property of the symbol: they sit in the definition, so
	every copy of the symbol answers the same way and no database has to
	hold them. Symbols written before the roles became kind information keep
	them among their element information, which is read as a fallback so an
	older symbol opens as it was saved (#1500).
	@param root the document holding the project's embedded collection
*/
QHash<QString, JunctionRoles> WiringListExport::junctionRoles(const QDomElement &root) const
{
	QHash<QString, JunctionRoles> roles;
	const QDomElement collection = root.firstChildElement(QStringLiteral("collection"));
	if (collection.isNull()) {
		return roles;
	}

	QList<QPair<QDomElement, QString>> pending;
	pending.append(qMakePair(collection, QStringLiteral("embed://")));
	while (!pending.isEmpty())
	{
		const auto step = pending.takeLast();
		for (QDomElement child = step.first.firstChildElement() ;
		     !child.isNull() ; child = child.nextSiblingElement())
		{
			const QString path = step.second + child.attribute(QStringLiteral("name"));
			if (child.tagName() == QLatin1String("category")) {
				pending.append(qMakePair(child, path + QLatin1Char('/')));
			} else if (child.tagName() == QLatin1String("element")) {
				const QDomElement definition =
					child.firstChildElement(QStringLiteral("definition"));
				if (definition.attribute(QStringLiteral("link_type"))
				    != QLatin1String("junction")) {
					continue;
				}
					//Read by the same reader the drawing uses, so the
					//two always answer alike
				roles.insert(path, junctionRolesOf(definition));
			}
		}
	}
	return roles;
}

QMap<QString, ElementInfo> WiringListExport::collectElementsInfo(const QDomElement &root) const
{
    QMap<QString, ElementInfo> infoMap;

    const QHash<QString, JunctionRoles> junctions = junctionRoles(root);
    QSet<QString> placeholderTypes;
    QDomElement collection = root.firstChildElement("collection");
    if (!collection.isNull()) {
        QDomNodeList defs = collection.elementsByTagName("definition");
        for (int i = 0; i < defs.size(); ++i) {
            QDomElement def = defs.at(i).toElement();
            QString ltype = def.attribute("link_type");
            if (ltype == "next_report" || ltype == "previous_report") {
                QDomElement parentEl = def.parentNode().toElement();
                if (parentEl.tagName().toLower() == "element") {
                    QString name = parentEl.attribute("name");
                    if (!name.isEmpty()) {
                        placeholderTypes.insert(name);
                    }
                }
            }
        }
    }

    QDomNodeList elements = root.elementsByTagName("element");
    for (int i = 0; i < elements.size(); ++i) {
        QDomElement el = elements.at(i).toElement();
        QString uuid = normalizeUuid(el.attribute("uuid", el.attribute("id", "")));
        if (uuid.isEmpty()) continue;

        ElementInfo info;
        info.folio = findDiagramFolio(climbToDiagram(el));

        QDomElement linksNode = el.firstChildElement("links_uuids");
        if (!linksNode.isNull()) {
            QDomNodeList linkUuids = linksNode.elementsByTagName("link_uuid");
            for (int j = 0; j < linkUuids.size(); ++j) {
                QString luuid = normalizeUuid(linkUuids.at(j).toElement().attribute("uuid"));
                if (!luuid.isEmpty()) info.links.append(luuid);
            }
        }

        QDomElement elInfoNode = el.firstChildElement("elementInformations");
        if (!elInfoNode.isNull()) {
            QDomNodeList eics = elInfoNode.elementsByTagName("elementInformation");
            for (int j = 0; j < eics.size(); ++j) {
                QDomElement eic = eics.at(j).toElement();
                QString nameAttr = eic.attribute("name").toLower();
                if (nameAttr == "label") info.label = eic.text().trimmed();
                if (nameAttr == "name") info.name = eic.text().trimmed();
            }
        }

        QString typeVal = el.attribute("type");
            //Where the wire comes in at and where it goes on to are the
            //symbol's own information, read from its definition -- the one
            //every copy is drawn from -- and not from this copy's own block.
            //A definition stores them the way an uuid is written there, with
            //the braces; the wires store theirs normalized, so they are
            //normalized here too and the two meet again.
        const JunctionRoles roles = junctions.value(typeVal);
        info.isJunction = junctions.contains(typeVal);
        info.junctionSource = normalizeUuid(roles.source);
        for (const QString &target : roles.targets) {
            info.junctionTargets << normalizeUuid(target);
        }
        info.isPlaceholder = false;
        for (const QString &ptype : placeholderTypes) {
            if (typeVal.endsWith(ptype)) {
                info.isPlaceholder = true;
                break;
            }
        }

        infoMap.insert(uuid, info);
    }
    return infoMap;
}

QList<ConductorData> WiringListExport::collectConductors(const QDomElement &root) const
{
    QList<ConductorData> conductors;
    QDomNodeList conductorNodes = root.elementsByTagName("conductor");

    for (int i = 0; i < conductorNodes.size(); ++i) {
        QDomElement cond = conductorNodes.at(i).toElement();

        if (cond.attribute("num") == "Brücke") continue;

        ConductorData data;
        data.index = i;
        data.el1_uuid = normalizeUuid(cond.attribute("element1", cond.attribute("element1id", "")));
        data.el2_uuid = normalizeUuid(cond.attribute("element2", cond.attribute("element2id", "")));
            //The connection each end sits on: a junction's roles are stored
            //against them, so a row can be resolved through a junction
        data.terminal1_uuid = normalizeUuid(cond.attribute("terminal1"));
        data.terminal2_uuid = normalizeUuid(cond.attribute("terminal2"));

        data.element1_label = cond.attribute("element1_label");
        if (data.element1_label.isEmpty()) {
            data.element1_label = cond.attribute("element1_linked");
        }

        data.element2_label = cond.attribute("element2_label");
        if (data.element2_label.isEmpty()) {
            data.element2_label = cond.attribute("element2_linked");
        }

        data.terminalname1 = cond.attribute("terminalname1");
        data.terminalname2 = cond.attribute("terminalname2");
        data.tension_protocol = cond.attribute("tension_protocol");
        data.conductor_color = cond.attribute("conductor_color");
        data.conductor_section = cond.attribute("conductor_section");
        data.function = cond.attribute("function");
        data.cable = cond.attribute("cable");

        QDomElement diag = climbToDiagram(cond);
        data.folio = findDiagramFolio(diag);
        data.folio_index = diag.attribute("order").toInt();
        if (data.folio.isEmpty()) data.folio = cond.attribute("folio", cond.attribute("page", ""));

        conductors.append(data);
    }
    return conductors;
}

void WiringListExport::toCsv()
{
    if (!m_project) return;

    int unassigned_junctions = 0;
    const QString csv = toCsvString(&unassigned_junctions);
    if (csv.isEmpty()) {
        QMessageBox::warning(m_parent, tr("Error"), tr("Unable to read the project's in-memory structure."));
        return;
    }

    QFileDialog dialog(m_parent);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(tr("Export the wiring diagram"));
    dialog.setDefaultSuffix("csv");
    dialog.setNameFilter(tr("CSV files (*.csv)"));

    if (dialog.exec() != QDialog::Accepted) return;
    QString fileName = dialog.selectedFiles().first();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(m_parent, tr("Error"), tr("Unable to open the file for writing."));
        return;
    }
    QTextStream out(&file);
    out << csv;
    file.close();

    QString message = tr("The wiring diagram has been successfully exported !");
    if (unassigned_junctions > 0) {
            //Said rather than left out: those rows end at the junction,
            //which is what the export would otherwise not tell anyone about
        message += QLatin1Char('\n') + tr(
            "Note: %n junction(s) have no source connection set, so their "
            "rows still end at the junction.", nullptr, unassigned_junctions);
    }
    QMessageBox::information(m_parent, tr("Export successful"), message);
}

QString WiringListExport::toCsvString(int *unassigned_junctions) const
{
    if (!m_project) return QString();
    if (unassigned_junctions) *unassigned_junctions = 0;

    QDomDocument doc = projectDocument();
    if (doc.isNull()) return QString();

    QSet<QString> conductorDefinitionTypes;
    QDomElement rootElem = doc.documentElement();
    QDomElement collection = rootElem.firstChildElement("collection");
    if (!collection.isNull()) {
        QDomNodeList defs = collection.elementsByTagName("definition");
        for (int i = 0; i < defs.size(); ++i) {
            QDomElement def = defs.at(i).toElement();
            if (def.attribute("link_type") == "conductor_definition") {
                QDomElement parentEl = def.parentNode().toElement();
                if (parentEl.tagName().toLower() == "element") {
                    QString name = parentEl.attribute("name");
                    if (!name.isEmpty()) {
                        conductorDefinitionTypes.insert(name);
                    }
                }
            }
        }
    }

    QSet<QString> conductorDefinitionUuids;
    QDomNodeList projectElements = rootElem.elementsByTagName("element");
    for (int i = 0; i < projectElements.size(); ++i) {
        QDomElement el = projectElements.at(i).toElement();
        QString typeVal = el.attribute("type");
        bool isCondDef = false;
        for (const QString &cType : conductorDefinitionTypes) {
            if (typeVal.endsWith(cType)) {
                isCondDef = true;
                break;
            }
        }
        if (isCondDef) {
            QString uuid = normalizeUuid(el.attribute("uuid", el.attribute("id", "")));
            if (!uuid.isEmpty()) {
                conductorDefinitionUuids.insert(uuid);
            }
        }
    }

    QMap<QString, ElementInfo> elementsInfo = collectElementsInfo(doc.documentElement());
    QList<ConductorData> conductors = collectConductors(doc.documentElement());

    QList<ConductorData> uniqueConductors;
    // Conductors with one end on a folio report, by that report. All of them
    // are collected before any is merged, so the result does not depend on
    // the order the folios are saved in.
    QMap<QString, QList<ConductorData>> partialWires;

    auto normalizePartial = [](ConductorData c, const QString &ph_uuid) {
        if (c.el1_uuid == ph_uuid) {
            std::swap(c.el1_uuid, c.el2_uuid);
            std::swap(c.element1_label, c.element2_label);
            std::swap(c.terminalname1, c.terminalname2);
        }
        return c;
    };

    resolveJunctionRuns(conductors, elementsInfo, unassigned_junctions);

    for (int i = 0; i < conductors.size(); ++i) {
        ConductorData c = conductors[i];

        if (conductorDefinitionUuids.contains(c.el1_uuid) || conductorDefinitionUuids.contains(c.el2_uuid)) {
            continue;
        }

        if (c.element1_label.isEmpty() && elementsInfo.contains(c.el1_uuid)) {
            c.element1_label = elementsInfo[c.el1_uuid].label;
            if (c.element1_label.isEmpty()) c.element1_label = elementsInfo[c.el1_uuid].name;
        }
        if (c.element2_label.isEmpty() && elementsInfo.contains(c.el2_uuid)) {
            c.element2_label = elementsInfo[c.el2_uuid].label;
            if (c.element2_label.isEmpty()) c.element2_label = elementsInfo[c.el2_uuid].name;
        }

        bool el1_ph = elementsInfo.value(c.el1_uuid).isPlaceholder;
        bool el2_ph = elementsInfo.value(c.el2_uuid).isPlaceholder;

        if (!el1_ph && !el2_ph) {
            uniqueConductors.append(c);
            continue;
        }

        if (el1_ph && el2_ph) {
            uniqueConductors.append(c);
            continue;
        }

        QString ph_uuid = el1_ph ? c.el1_uuid : c.el2_uuid;
        partialWires[ph_uuid].append(normalizePartial(c, ph_uuid));
    }

    // Two halves are one wire only when each report of a linked pair carries
    // exactly one conductor. With several on a side, the diagram does not say
    // which terminal is wired to which, so each conductor gets its own row,
    // ending at the report.
    QSet<QString> written;
    for (auto it = partialWires.cbegin(); it != partialWires.cend(); ++it) {
        const QString &ph_uuid = it.key();
        if (written.contains(ph_uuid)) continue;
        written.insert(ph_uuid);

        const QStringList links = elementsInfo.value(ph_uuid).links;
        const QString matching_ph_uuid = links.isEmpty() ? QString() : links.first();
        const QList<ConductorData> others = partialWires.value(matching_ph_uuid);

        if (it.value().size() != 1 || others.size() != 1 || written.contains(matching_ph_uuid)) {
            uniqueConductors.append(it.value());
            continue;
        }
        written.insert(matching_ph_uuid);

        const ConductorData &otherHalf = it.value().first();
        const ConductorData &normC = others.first();

        ConductorData merged;
        merged.folio = mergeFields(otherHalf.folio, normC.folio);
        merged.folio_index = std::min(otherHalf.folio_index, normC.folio_index);

        merged.el1_uuid = otherHalf.el1_uuid;
        merged.element1_label = otherHalf.element1_label;
        merged.terminalname1 = otherHalf.terminalname1;

        merged.el2_uuid = normC.el1_uuid;
        merged.element2_label = normC.element1_label;
        merged.terminalname2 = normC.terminalname1;

        merged.tension_protocol = mergeFields(otherHalf.tension_protocol, normC.tension_protocol);
        merged.conductor_color = mergeFields(otherHalf.conductor_color, normC.conductor_color);
        merged.conductor_section = mergeFields(otherHalf.conductor_section, normC.conductor_section);
        merged.function = mergeFields(otherHalf.function, normC.function);
        merged.cable = mergeFields(otherHalf.cable, normC.cable);

        uniqueConductors.append(merged);
    }

    for (ConductorData &c : uniqueConductors) {
        if (!c.element2_label.isEmpty() && (c.element1_label.isEmpty() || c.element2_label.toLower() < c.element1_label.toLower())) {
            std::swap(c.element1_label, c.element2_label);
            std::swap(c.terminalname1, c.terminalname2);
            std::swap(c.el1_uuid, c.el2_uuid);
        }
    }

    std::sort(uniqueConductors.begin(), uniqueConductors.end(), [](const ConductorData &a, const ConductorData &b) {
        // By the folio's position in the project: its number is free text
        // ("3/12", "A-2") and does not sort.
        if (a.folio_index != b.folio_index) return a.folio_index < b.folio_index;

        int el1Cmp = a.element1_label.toLower().compare(b.element1_label.toLower());
        if (el1Cmp != 0) return el1Cmp < 0;

        int el2Cmp = a.element2_label.toLower().compare(b.element2_label.toLower());
        if (el2Cmp != 0) return el2Cmp < 0;

        int term1Cmp = a.terminalname1.compare(b.terminalname1);
        if (term1Cmp != 0) return term1Cmp < 0;

        return a.terminalname2 < b.terminalname2;
    });

    QString csv;
    QTextStream out(&csv);
    out << tr("Page", "Wiring list CSV header") << ";"
    << tr("Component 1", "Wiring list CSV header") << ";"
    << tr("Terminal 1", "Wiring list CSV header") << ";"
    << tr("Component 2", "Wiring list CSV header") << ";"
    << tr("Terminal 2", "Wiring list CSV header") << ";"
    << tr("Voltage / Protocol", "Wiring list CSV header") << ";"
    << tr("Wire color", "Wiring list CSV header") << ";"
    << tr("Wire section", "Wiring list CSV header") << ";"
    << tr("Function", "Wiring list CSV header") << ";"
    << tr("Cable", "Wiring list CSV header") << "\n";

    for (const ConductorData &c : uniqueConductors) {
        out << c.folio << ";"
        << c.element1_label << ";"
        << c.terminalname1 << ";"
        << c.element2_label << ";"
        << c.terminalname2 << ";"
        << c.tension_protocol << ";"
        << c.conductor_color << ";"
        << c.conductor_section << ";"
        << c.function << ";"
        << c.cable << "\n";
    }

    return csv;
}

/**
	@brief WiringListExport::resolveJunctionRuns
	Written once for both lists: the CSV export and the database list are
	meant to agree, so they share this one rule (#1500).

	A junction is a connection point on a wire, never a part of its own:
	the rows running through one are listed as the one run they are, from
	the real end the wire comes in at to the real end it goes on to, and
	the halves which stopped at the junction are left out. The order the
	roles give is the chain: source to target 1, target 1 to target 2, and
	so on.

	A junction with no source set stays as it is -- the user has not said
	which way it runs -- and is counted, so both lists can say so rather
	than listing it silently wrong.

	@param conductors the rows, changed in place
	@param elementsInfo the symbols the rows name, holding the roles read
	from the definitions of the junctions among them
	@param unassigned_junctions, when given, is set to how many junctions
	have a wire on them and no source connection set, and so stayed in the
	list unresolved
*/
void WiringListExport::resolveJunctionRuns(QList<ConductorData> &conductors,
                                           const QMap<QString, ElementInfo> &elementsInfo,
                                           int *unassigned_junctions)
{
    if (unassigned_junctions) *unassigned_junctions = 0;
    //The symbols at least one conductor hangs on: what says whether
    //a junction with no source set has rows ending at it
    QSet<QString> wired;
    for (const ConductorData &c : std::as_const(conductors)) {
        wired.insert(c.el1_uuid);
        wired.insert(c.el2_uuid);
    }

    //The conductors sitting on each connection of each symbol
    QHash<QString, QList<int>> on_connection;
    for (int i = 0; i < conductors.size(); ++i) {
        on_connection[conductors[i].el1_uuid + QLatin1Char('|')
                      + conductors[i].terminal1_uuid] << i;
        on_connection[conductors[i].el2_uuid + QLatin1Char('|')
                      + conductors[i].terminal2_uuid] << i;
    }

    auto labelOf = [&elementsInfo](const QString &element_uuid) {
        const ElementInfo info = elementsInfo.value(element_uuid);
        return info.label.isEmpty() ? info.name : info.label;
    };

        //The end a conductor reaches, given the end it hangs on
    auto farEnd = [&conductors](int wire, const QString &element_uuid,
                                const QString &terminal_uuid) {
        const ConductorData &c = conductors.at(wire);
        if (c.el1_uuid == element_uuid && c.terminal1_uuid == terminal_uuid)
            return QPair<QString, QString>(c.el2_uuid, c.terminal2_uuid);
        return QPair<QString, QString>(c.el1_uuid, c.terminal1_uuid);
    };

        //One end of the chain: what it reaches, and the wire it sits on
    struct Stop {
        QString element;
        QString terminal;
        int wire = 0;
    };

        //Where the wires on one connection of a chain really end: their
        //far ends, and on through every junction in the way, because the
        //chain of such a junction says how the run goes on and a junction
        //reached that way is passed in turn. A run through two junctions
        //in a row therefore ends at real pieces rather than at the second
        //one (#1500).
        //
        //Every wire hanging on the connection is followed, the second and
        //third one like the first, and every wire walked comes back in
        //@a walked so that none of them is left with a row ending at a
        //junction. The connections already passed are carried along the
        //path rather than shared between the paths, so a loop is caught
        //without holding up a wire which happens to reach the same place
        //another way (#1500).
    std::function<QList<Stop>(const QString &, const QString &, QSet<QString>,
                              QList<int> &)> walk;
    walk = [&](const QString &element_uuid, const QString &terminal_uuid,
               QSet<QString> passed, QList<int> &walked) {
        QList<Stop> stops;
        if (element_uuid.isEmpty() || terminal_uuid.isEmpty()) {
            return stops;
        }
        const QString here = element_uuid + QLatin1Char('|') + terminal_uuid;
        if (passed.contains(here)) {
                //A loop: two junctions wired to each other, say
            return stops;
        }
        passed.insert(here);

        for (const int wire : on_connection.value(here))
        {
            walked.append(wire);
            const auto far = farEnd(wire, element_uuid, terminal_uuid);
            const ElementInfo &reached = elementsInfo.value(far.first);
            if (!reached.isJunction) {
                    //A real piece: this is where the run ends
                stops.append(Stop{far.first, far.second, wire});
                continue;
            }
                //On through that junction by the rule the drawing follows,
                //so the two cannot disagree (#1500): what leads to the
                //connection we came in at, or the first one it goes on to
                //when that is its source
            const auto links = junctionChainRoles(reached.junctionSource,
                                                  reached.junctionTargets,
                                                  far.second);
            const QString next = links.first.isEmpty() ? links.second : links.first;
            stops += walk(far.first, next, passed, walked);
        }
        return stops;
    };

    QList<ConductorData> added;
    QSet<int> superseded;

    for (auto it = elementsInfo.constBegin(); it != elementsInfo.constEnd(); ++it)
    {
        const ElementInfo &info = it.value();
        if (!info.isJunction) continue;

        if (info.junctionSource.isEmpty()) {
                //A junction a wire hangs on has rows ending at it,
                //which stay as they are: say so rather than listing
                //them as if the way through it were known. One
                //nothing was ever wired to has no row to get wrong,
                //and counts for nothing (#1500)
            if (unassigned_junctions && wired.contains(it.key())) {
                ++(*unassigned_junctions);
            }
            continue;
        }
        const QString on_source = it.key() + QLatin1Char('|') + info.junctionSource;
        if (on_connection.value(on_source).isEmpty()) continue;

        QList<QList<Stop>> chain;   // one list of stops per connection
        QList<QList<int>> walked;   // every wire walked to get there
        auto addToChain = [&](const QString &terminal_uuid) {
            if (terminal_uuid.isEmpty()) return;
            if (on_connection.value(it.key() + QLatin1Char('|') + terminal_uuid).isEmpty()) {
                    //A connection the user has not wired yet simply leaves
                    //the chain shorter; it does not end it somewhere wrong
                return;
            }
            QList<int> wires;
            QSet<QString> passed;
            const QList<Stop> stops = walk(it.key(), terminal_uuid, passed, wires);
            if (stops.isEmpty()) {
                    //The chain does not say where this one goes on to:
                    //its wires stay as they are rather than being written
                    //somewhere wrong (#1500)
                return;
            }
            chain.append(stops);
            walked.append(wires);
        };
        addToChain(info.junctionSource);
        for (const QString &target : info.junctionTargets) {
            addToChain(target);
        }

            //Two junctions in a row share the wire between them, and
            //whichever of them is read first writes the one row for
            //the whole run: the other is left alone rather than
            //writing the same run a second time, with a junction at
            //its end (#1500)
        bool claimed = false;
        for (const QList<int> &wires : walked)
        {
            for (const int wire : wires)
            {
                if (superseded.contains(wire)) {
                    claimed = true;
                    break;
                }
            }
            if (claimed) break;
        }
        if (claimed) continue;

        for (int k = 1; k < chain.size(); ++k)
        {
            const QList<Stop> &from_side = chain.at(k - 1);
            const QList<Stop> &to_side = chain.at(k);
                //Both halves of this link are written as the rows below,
                //so neither is listed on its own any more -- nor is
                //anything walked through the junctions in the way. Every
                //wire on either side gets a row: where there are more on
                //one side than on the other, the extra ones go to the
                //first of the others rather than being dropped (#1500)
            for (const int wire : walked.at(k - 1)) {
                superseded.insert(wire);
            }
            for (const int wire : walked.at(k)) {
                superseded.insert(wire);
            }

            for (int p = 0, sides = std::max(from_side.size(), to_side.size());
                 p < sides; ++p)
            {
                const Stop &from = from_side.at(p % from_side.size());
                const Stop &to = to_side.at(p % to_side.size());
                const ConductorData &from_wire = conductors.at(from.wire);
                const ConductorData &to_wire = conductors.at(to.wire);

                ConductorData row;
                row.index = to_wire.index;
                row.el1_uuid = from.element;
                row.el2_uuid = to.element;
                row.terminal1_uuid = from.terminal;
                row.terminal2_uuid = to.terminal;
                row.element1_label = labelOf(from.element);
                row.element2_label = labelOf(to.element);
                    //The name each end is drawn with, read from the wire
                    //that end sits on: the connection the wire ends at is
                    //the one the row is about, so it is the side holding it
                    //whose name is written
                row.terminalname1 = (from_wire.el1_uuid == from.element
                                     && from_wire.terminal1_uuid == from.terminal)
                        ? from_wire.terminalname1 : from_wire.terminalname2;
                row.terminalname2 = (to_wire.el1_uuid == to.element
                                     && to_wire.terminal1_uuid == to.terminal)
                        ? to_wire.terminalname1 : to_wire.terminalname2;
                    //One potential all the way through, so the run's
                    //properties are the wire's which leads to its far end
                row.tension_protocol = to_wire.tension_protocol;
                row.conductor_color = to_wire.conductor_color;
                row.conductor_section = to_wire.conductor_section;
                row.function = to_wire.function;
                row.cable = to_wire.cable;
                row.folio = mergeFields(from_wire.folio, to_wire.folio);
                row.folio_index = std::min(from_wire.folio_index, to_wire.folio_index);
                added.append(row);
            }
        }
    }

        //The halves which ended at a resolved junction are replaced by
        //the rows above; one ending at a junction with no source set
        //stays, which is what was asked for
    if (!superseded.isEmpty()) {
        QList<ConductorData> kept;
        for (int i = 0; i < conductors.size(); ++i) {
            if (!superseded.contains(i)) kept << conductors.at(i);
        }
        conductors = kept;
    }
    conductors.append(added);
}