#include "wiringlistexport.h"
#include "qetproject.h"
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

        QDomElement diag = climbToDiagram(cond);
        data.folio = findDiagramFolio(diag);
        if (data.folio.isEmpty()) data.folio = cond.attribute("folio", cond.attribute("page", ""));

        conductors.append(data);
    }
    return conductors;
}

void WiringListExport::toCsv()
{
    if (!m_project) return;

    QDomDocument doc = m_project->toXml();

    if (doc.isNull()) {
        QMessageBox::warning(m_parent, tr("Erreur"), tr("Impossible de lire la structure en mémoire du projet."));
        return;
    }

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

    QFileDialog dialog(m_parent);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(tr("Exporter le plan de câblage"));
    dialog.setDefaultSuffix("csv");
    dialog.setNameFilter(tr("Fichiers CSV (*.csv)"));

    if (dialog.exec() != QDialog::Accepted) return;
    QString fileName = dialog.selectedFiles().first();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(m_parent, tr("Erreur"), tr("Impossible d'ouvrir le fichier pour l'écriture."));
        return;
    }

    QMap<QString, ElementInfo> elementsInfo = collectElementsInfo(doc.documentElement());
    QList<ConductorData> conductors = collectConductors(doc.documentElement());

    QList<ConductorData> uniqueConductors;
    QMap<QString, ConductorData> partialWires;

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
        ConductorData normC = normalizePartial(c, ph_uuid);

        QString matching_ph_uuid;
        if (!elementsInfo[ph_uuid].links.isEmpty()) {
            matching_ph_uuid = elementsInfo[ph_uuid].links.first();
        }

        if (!matching_ph_uuid.isEmpty() && partialWires.contains(matching_ph_uuid)) {
            ConductorData otherHalf = partialWires.take(matching_ph_uuid);

            ConductorData merged;
            merged.folio = mergeField(otherHalf.folio, normC.folio);

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

            uniqueConductors.append(merged);
        } else {
            partialWires.insert(ph_uuid, normC);
        }
    }

    for (const ConductorData &leftover : partialWires.values()) {
        uniqueConductors.append(leftover);
    }

    for (ConductorData &c : uniqueConductors) {
        if (!c.element2_label.isEmpty() && (c.element1_label.isEmpty() || c.element2_label.toLower() < c.element1_label.toLower())) {
            std::swap(c.element1_label, c.element2_label);
            std::swap(c.terminalname1, c.terminalname2);
            std::swap(c.el1_uuid, c.el2_uuid);
        }
    }

    std::sort(uniqueConductors.begin(), uniqueConductors.end(), [](const ConductorData &a, const ConductorData &b) {
        QStringList partsA = a.folio.split(',');
        QStringList partsB = b.folio.split(',');
        int minLen = std::min(partsA.size(), partsB.size());
        int folioCmp = 0;

        for (int i = 0; i < minLen; ++i) {
            bool okA, okB;
            int numA = partsA[i].trimmed().toInt(&okA);
            int numB = partsB[i].trimmed().toInt(&okB);

            if (okA && okB) {
                if (numA != numB) {
                    folioCmp = (numA < numB) ? -1 : 1;
                    break;
                }
            } else {
                int strCmp = partsA[i].trimmed().compare(partsB[i].trimmed(), Qt::CaseInsensitive);
                if (strCmp != 0) {
                    folioCmp = strCmp;
                    break;
                }
            }
        }

        if (folioCmp == 0 && partsA.size() != partsB.size()) {
            folioCmp = (partsA.size() < partsB.size()) ? -1 : 1;
        }

        if (folioCmp != 0) return folioCmp < 0;

        int el1Cmp = a.element1_label.toLower().compare(b.element1_label.toLower());
        if (el1Cmp != 0) return el1Cmp < 0;

        int el2Cmp = a.element2_label.toLower().compare(b.element2_label.toLower());
        if (el2Cmp != 0) return el2Cmp < 0;

        int term1Cmp = a.terminalname1.compare(b.terminalname1);
        if (term1Cmp != 0) return term1Cmp < 0;

        return a.terminalname2 < b.terminalname2;
    });

    QTextStream out(&file);
    out << tr("Page", "Wiring list CSV header") << ";"
    << tr("Composant 1", "Wiring list CSV header") << ";"
    << tr("Borne 1", "Wiring list CSV header") << ";"
    << tr("Composant 2", "Wiring list CSV header") << ";"
    << tr("Borne 2", "Wiring list CSV header") << ";"
    << tr("Tension / Protocole", "Wiring list CSV header") << ";"
    << tr("Couleur du fil", "Wiring list CSV header") << ";"
    << tr("Section du fil", "Wiring list CSV header") << ";"
    << tr("Fonction", "Wiring list CSV header") << "\n";

    for (const ConductorData &c : uniqueConductors) {
        out << c.folio << ";"
        << c.element1_label << ";"
        << c.terminalname1 << ";"
        << c.element2_label << ";"
        << c.terminalname2 << ";"
        << c.tension_protocol << ";"
        << c.conductor_color << ";"
        << c.conductor_section << ";"
        << c.function << "\n";
    }

    file.close();
    QMessageBox::information(m_parent, tr("Export réussi"), tr("Le plan de câblage a été exporté avec succès !"));
}
