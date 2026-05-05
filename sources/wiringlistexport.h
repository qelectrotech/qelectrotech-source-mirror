#ifndef WIRINGLISTEXPORT_H
#define WIRINGLISTEXPORT_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QStringList>

class QETProject;
class QWidget;
class QDomElement;
class QDomNode;

// Internal data structures for parsing the XML graph
struct ElementInfo {
    QString folio;
    QStringList links;
    QString label;
    QString name;
    bool isPlaceholder = false;
};

struct ConductorData {
    int index = 0;
    QString el1_uuid;
    QString el2_uuid;
    QString element1_label;
    QString element2_label;
    QString terminalname1;
    QString terminalname2;
    QString tension_protocol;
    QString conductor_color;
    QString conductor_section;
    QString function;
    QString folio;

    // Resolved endpoints
    QString chosen_a_uuid;
    QString chosen_a_label;
    QString chosen_b_uuid;
    QString chosen_b_label;
};

/**
 * @brief The WiringListExport class
 * Handles the export of the wiring list (Verdrahtungsplan) to a CSV file.
 * Automatically resolves links and placeholders to find physical endpoints.
 */
class WiringListExport : public QObject
{
    Q_OBJECT
public:
    explicit WiringListExport(QETProject *project, QWidget *parent = nullptr);
    void toCsv();

private:
    QETProject *m_project;
    QWidget *m_parent;

    QString normalizeUuid(const QString &u) const;
    QString findDiagramFolio(const QDomElement &diagramElem) const;
    QDomElement climbToDiagram(QDomNode node) const;

    QMap<QString, ElementInfo> collectElementsInfo(const QDomElement &root) const;
    QList<ConductorData> collectConductors(const QDomElement &root) const;

    void resolveEndpoints(QList<ConductorData> &conductors, const QMap<QString, ElementInfo> &elementsInfo) const;
};

#endif // WIRINGLISTEXPORT_H
