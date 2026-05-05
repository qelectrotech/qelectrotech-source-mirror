#include "wiringlistexport.h"
#include "qetproject.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDomDocument>
#include <QFile>
#include <QRegularExpression>
#include <QQueue>
#include <QSet>

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

        QString typeVal = el.attribute("type").toLower();
        if (typeVal.contains("naechste") || typeVal.contains("vorherige") ||
            typeVal.contains("next") || typeVal.contains("previous")) {
            info.isPlaceholder = true;
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
        data.element2_label = cond.attribute("element2_label");
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

void WiringListExport::resolveEndpoints(QList<ConductorData> &conductors, const QMap<QString, ElementInfo> &elementsInfo) const
{
    QRegularExpression numericLabelRe("^\\d+(\\.\\d+)?$");

    QMap<QString, QList<ConductorData>> el_to_cons;
    for (const ConductorData &c : conductors) {
        if (!c.el1_uuid.isEmpty()) el_to_cons[c.el1_uuid].append(c);
        if (!c.el2_uuid.isEmpty()) el_to_cons[c.el2_uuid].append(c);
    }

    for (int i = 0; i < conductors.size(); ++i) {
        ConductorData &c = conductors[i];

        auto resolveSide = [&](const QString &startUuid, QString &outLabel, QString &outTerminal) {
            if (startUuid.isEmpty() || !elementsInfo.contains(startUuid)) return;

            const ElementInfo &startInfo = elementsInfo[startUuid];
            if (!startInfo.links.isEmpty() || startInfo.isPlaceholder) {
                QQueue<QString> q;
                QSet<QString> visited;
                q.enqueue(startUuid);
                visited.insert(startUuid);

                int depth = 0;
                while (!q.isEmpty() && depth < 3) {
                    int levelSize = q.size();
                    for (int k = 0; k < levelSize; ++k) {
                        QString curr = q.dequeue();

                        if (elementsInfo.contains(curr)) {
                            const ElementInfo &currInfo = elementsInfo[curr];

                            if (!currInfo.isPlaceholder && !currInfo.label.isEmpty() && !numericLabelRe.match(currInfo.label).hasMatch()) {
                                outLabel = currInfo.label;
                                return;
                            }

                            for (const QString &lnk : currInfo.links) {
                                if (!visited.contains(lnk)) {
                                    visited.insert(lnk);
                                    q.enqueue(lnk);
                                }
                            }
                        }

                        for (const ConductorData &cond : el_to_cons.value(curr)) {
                            if (cond.index == c.index) continue;

                            QString other;
                            QString terminalHint;
                            if (cond.el1_uuid == curr) {
                                other = cond.el2_uuid;
                                terminalHint = cond.terminalname2;
                            } else {
                                other = cond.el1_uuid;
                                terminalHint = cond.terminalname1;
                            }

                            if (!other.isEmpty() && !visited.contains(other)) {
                                if (elementsInfo.contains(other)) {
                                    const ElementInfo &oInfo = elementsInfo[other];
                                    if (!oInfo.isPlaceholder && !oInfo.label.isEmpty() && !numericLabelRe.match(oInfo.label).hasMatch()) {
                                        outLabel = oInfo.label;
                                        if (outTerminal.isEmpty()) outTerminal = terminalHint;
                                        return;
                                    }
                                }
                                visited.insert(other);
                                q.enqueue(other);
                            }
                        }
                    }
                    depth++;
                }
            } else {
                if (outLabel.isEmpty()) {
                    outLabel = startInfo.label.isEmpty() ? startInfo.name : startInfo.label;
                }
            }
        };

        bool p1 = elementsInfo.value(c.el1_uuid).isPlaceholder;
        bool p2 = elementsInfo.value(c.el2_uuid).isPlaceholder;

        if (c.element1_label.isEmpty() || p1) {
            if (p1) c.element1_label = "";
            resolveSide(c.el1_uuid, c.element1_label, c.terminalname1);
        }
        if (c.element2_label.isEmpty() || p2) {
            if (p2) c.element2_label = "";
            resolveSide(c.el2_uuid, c.element2_label, c.terminalname2);
        }
    }
}

void WiringListExport::toCsv()
{
    if (!m_project) return;

    QDomDocument doc = m_project->toXml();

    if (doc.isNull()) {
        QMessageBox::warning(m_parent, tr("Erreur"), tr("Impossible de lire la structure en mémoire du projet."));
        return;
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

    resolveEndpoints(conductors, elementsInfo);

    QList<ConductorData> uniqueConductors;
    QSet<QString> seenConnections;

    for (const ConductorData &c : conductors) {
        if (c.element1_label.isEmpty() && c.element2_label.isEmpty()) continue;

        QString sideA = c.element1_label + ":" + c.terminalname1;
        QString sideB = c.element2_label + ":" + c.terminalname2;

        QString key = (sideA < sideB) ? (sideA + "||" + sideB) : (sideB + "||" + sideA);

        if (!seenConnections.contains(key)) {
            seenConnections.insert(key);
            uniqueConductors.append(c);
        }
    }

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
