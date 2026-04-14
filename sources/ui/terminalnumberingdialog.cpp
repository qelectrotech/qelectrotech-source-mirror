#include "terminalnumberingdialog.h"
#include "ui_terminalnumberingdialog.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../undocommand/changeelementinformationcommand.h"
#include <QUndoCommand>
#include <algorithm>

/**
 * @brief TerminalNumberingDialog::TerminalNumberingDialog
 * Constructor
 * @param parent
 */
TerminalNumberingDialog::TerminalNumberingDialog(QWidget *parent) :
QDialog(parent),
ui(new Ui::TerminalNumberingDialog)
{
    ui->setupUi(this);
}

/**
 * @brief TerminalNumberingDialog::~TerminalNumberingDialog
 * Destructor
 */
TerminalNumberingDialog::~TerminalNumberingDialog()
{
    delete ui;
}

/**
 * @brief TerminalNumberingDialog::isXAxisPriority
 * @return true if X axis has priority, false if Y axis has priority
 */
bool TerminalNumberingDialog::isXAxisPriority() const
{
    return ui->rb_priority_x->isChecked();
}

/**
 * @brief TerminalNumberingDialog::isAlphanumeric
 * @return true if alphanumeric sorting is enabled, false if numeric only
 */
bool TerminalNumberingDialog::isAlphanumeric() const
{
    return ui->rb_type_alpha->isChecked();
}

/**
 * @brief TerminalNumberingDialog::getUndoCommand
 * Scans the given project for terminals, sorts them according to user preferences
 * (X/Y axis, alphanumeric rules), and generates an undo command containing all label changes.
 * * @param project Pointer to the current QETProject
 * @return QUndoCommand* containing the modifications, or nullptr if no changes are needed.
 */
QUndoCommand* TerminalNumberingDialog::getUndoCommand(QETProject *project) const {
    if (!project) return nullptr;

    bool axisX = isXAxisPriority();
    bool alpha = isAlphanumeric();

    // 1. Helper structure to store and sort terminal data
    struct TermInfo {
        Element *elmt;
        QString prefix;
        QString suffix;
        int folioIndex;
        qreal x;
        qreal y;
    };
    QList<TermInfo> termList;

    // 2. Collect all terminals from all folios in the project
    foreach (Diagram *diagram, project->diagrams()) {
        int fIndex = diagram->folioIndex();
        foreach (QGraphicsItem *qgi, diagram->items()) {
            if (Element *elmt = qgraphicsitem_cast<Element *>(qgi)) {

                // Check if the element is actually a terminal
                if (elmt->elementData().m_type == ElementData::Terminal) {
                    DiagramContext info = elmt->elementInformations();

                    // Ignore locked terminals (if the user checked a 'lock' property)
                    if (info.value(QStringLiteral("auto_num_locked")).toString() == QLatin1String("true")) {
                        continue;
                    }

                    QString label = elmt->actualLabel();
                    if (label.isEmpty()) continue;

                    // Split prefix (e.g., "-X1") and suffix (e.g., "1" or "A")
                    QString prefix = label;
                    QString suffix = "";
                    int colonIndex = label.lastIndexOf(':');
                    if (colonIndex != -1) {
                        prefix = label.left(colonIndex);
                        suffix = label.mid(colonIndex + 1);
                    }

                    // If user chose purely numeric, skip terminals with alphabetical suffixes
                    if (!alpha && !suffix.isEmpty()) {
                        bool isNum;
                        suffix.toInt(&isNum);
                        if (!isNum) continue;
                    }

                    TermInfo ti;
                    ti.elmt = elmt;
                    ti.prefix = prefix;
                    ti.suffix = suffix;
                    ti.folioIndex = fIndex;
                    ti.x = elmt->pos().x();
                    ti.y = elmt->pos().y();
                    termList.append(ti);
                }
            }
        }
    }

    // 3. Sort terminals based on user selection (X or Y axis priority)
    std::sort(termList.begin(), termList.end(), [axisX](const TermInfo &a, const TermInfo &b) {
        // First sort by BMK Prefix alphabetically (case insensitive)
        int prefixCmp = a.prefix.compare(b.prefix, Qt::CaseInsensitive);
        if (prefixCmp != 0) return prefixCmp < 0;

        // Then sort by folio (page) index
        if (a.folioIndex != b.folioIndex) return a.folioIndex < b.folioIndex;

        // Finally sort by coordinates (with a 1.0px tolerance to handle slight misalignments)
        if (axisX) {
            if (qAbs(a.x - b.x) > 1.0) return a.x < b.x;
            return a.y < b.y;
        } else {
            if (qAbs(a.y - b.y) > 1.0) return a.y < b.y;
            return a.x < b.x;
        }
    });

    // 4. Generate new numbering and create the undo command macro
    QUndoCommand *macro = new QUndoCommand(QObject::tr("Automatic terminal numbering"));
    QMap<QString, int> counters;

    foreach (const TermInfo &ti, termList) {
        // Increment the counter for this terminal block (e.g., "-X3")
        counters[ti.prefix]++;
        int newNum = counters[ti.prefix];

        // Determine if the original suffix was a pure number
        QString newLabel;
        bool isNum;
        ti.suffix.toInt(&isNum);

        if (isNum || ti.suffix.isEmpty()) {
            // If it was a number (e.g., "1") or empty, update it with the new counter
            newLabel = ti.prefix + ":" + QString::number(newNum);
        } else {
            // If it was alphabetical (e.g., "N", "PE"), keep the original text but consume the count!
            newLabel = ti.prefix + ":" + ti.suffix;
        }

        DiagramContext oldInfo = ti.elmt->elementInformations();
        DiagramContext newInfo = oldInfo;
        newInfo.addValue(QStringLiteral("label"), newLabel);

        // Create an undo command only if the label actually changes
        if (oldInfo != newInfo) {
            new ChangeElementInformationCommand(ti.elmt, oldInfo, newInfo, macro);
        }
    }

    // 5. Return the macro if it contains changes, otherwise delete and return null
    if (macro->childCount() > 0) {
        return macro;
    } else {
        delete macro;
        return nullptr;
    }
}
