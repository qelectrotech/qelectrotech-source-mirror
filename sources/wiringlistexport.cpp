#include "wiringlistexport.h"
#include "qetproject.h"
#include "diagram.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDomDocument>
#include <QFile>
#include <algorithm>

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
    if (m_project && index >= 1 && index <= m_project->diagrams().size()) {
        return m_project->diagrams().at(index - 1)->border_and_titleblock
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

QMap<QString, ElementInfo> WiringListExport::collectElementsInfo(const QDomElement &root) const
{
    QMap<QString, ElementInfo> infoMap;

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

    const QString csv = toCsvString();
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
    QMessageBox::information(m_parent, tr("Export successful"), tr("The wiring diagram has been successfully exported !"));
}

QString WiringListExport::toCsvString() const
{
    if (!m_project) return QString();

    QDomDocument doc = m_project->toXml();
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

    auto mergeField = [](const QString &a, const QString &b) {
        QString at = a.trimmed();
        QString bt = b.trimmed();
        if (at.isEmpty()) return bt;
        if (bt.isEmpty()) return at;
        if (at == bt) return at;
        return at + ", " + bt;
    };

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
        merged.folio = mergeField(otherHalf.folio, normC.folio);
        merged.folio_index = std::min(otherHalf.folio_index, normC.folio_index);

        merged.el1_uuid = otherHalf.el1_uuid;
        merged.element1_label = otherHalf.element1_label;
        merged.terminalname1 = otherHalf.terminalname1;

        merged.el2_uuid = normC.el1_uuid;
        merged.element2_label = normC.element1_label;
        merged.terminalname2 = normC.terminalname1;

        merged.tension_protocol = mergeField(otherHalf.tension_protocol, normC.tension_protocol);
        merged.conductor_color = mergeField(otherHalf.conductor_color, normC.conductor_color);
        merged.conductor_section = mergeField(otherHalf.conductor_section, normC.conductor_section);
        merged.function = mergeField(otherHalf.function, normC.function);
        merged.cable = mergeField(otherHalf.cable, normC.cable);

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
