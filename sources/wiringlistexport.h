#ifndef WIRINGLISTEXPORT_H
#define WIRINGLISTEXPORT_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QDomDocument>
	//The roles of a junction, and their reader, shared with the drawing:
	//the two must always answer alike
#include <qetgraphicsitem/terminal.h>

class QETProject;
class QWidget;
class QDomElement;
class QDomNode;

struct ElementInfo {
    QString folio;
    QStringList links;
    QString label;
    QString name;
    bool isPlaceholder = false;
        /// A junction: a connection point on a wire, never a part of its own
    bool isJunction = false;
        /// The connection the wire comes in at, as the symbol stores it
    QString junctionSource;
        /// Where the wire goes on to, in the order the wiring list follows
    QStringList junctionTargets;
};

struct ConductorData {
    int index = 0;
    QString el1_uuid;
    QString el2_uuid;
        /// The connection each end sits on: which one a junction's role is
        /// stored against, so a row can be resolved through a junction
    QString terminal1_uuid;
    QString terminal2_uuid;
    QString element1_label;
    QString element2_label;
    QString terminalname1;
    QString terminalname2;
    QString tension_protocol;
    QString conductor_color;
    QString conductor_section;
    QString function;
    QString cable;
    QString folio;
    int folio_index = 0; // position of the folio in the project, from 1
        /// The row as it stands: through a junction, both ends are the
        /// connections the junction leads to rather than the junction itself
    bool resolved = false;
};

/**
 * @brief The WiringListExport class
 * Exports the wiring diagram from QElectroTech as a CSV file.
 */
class WiringListExport : public QObject
{
    Q_OBJECT
public:
    explicit WiringListExport(QETProject *project, QWidget *parent = nullptr);
    void toCsv();
    /**
        Build the wiring-list CSV and return it as a string (no GUI).
        Used by toCsv() and by the headless command-line export.
        @param unassigned_junctions, when given, is set to how many junctions
        have no source connection set and so stayed in the list unresolved
    */
    QString toCsvString(int *unassigned_junctions = nullptr) const;

    /**
        Write every run through a junction as the one run it is, and count
        the junctions which have a wire on them and no source connection
        set. Shared by the CSV export and by the database list, which are
        meant to agree (#1500).
        @param conductors the rows, changed in place
        @param elementsInfo the symbols the rows name, holding the roles
        read from the definitions of the junctions among them
        @param unassigned_junctions, when given, is set to how many
        junctions stayed in the list unresolved
    */
    static void resolveJunctionRuns(QList<ConductorData> &conductors,
                                    const QMap<QString, ElementInfo> &elementsInfo,
                                    int *unassigned_junctions);

private:
    QETProject *m_project;
    QWidget *m_parent;

    QString normalizeUuid(const QString &u) const;
    QString findDiagramFolio(const QDomElement &diagramElem) const;
    QDomElement climbToDiagram(QDomNode node) const;
    QDomDocument projectDocument() const;
    QHash<QString, JunctionRoles> junctionRoles(const QDomElement &root) const;

    QMap<QString, ElementInfo> collectElementsInfo(const QDomElement &root) const;
    QList<ConductorData> collectConductors(const QDomElement &root) const;
};

#endif // WIRINGLISTEXPORT_H
