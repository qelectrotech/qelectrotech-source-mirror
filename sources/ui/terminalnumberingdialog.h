#ifndef TERMINALNUMBERINGDIALOG_H
#define TERMINALNUMBERINGDIALOG_H

#include <QDialog>
#include <QMap>

class QETProject;
class QUndoCommand;
class QCheckBox;

namespace Ui {
    class TerminalNumberingDialog;
}

/**
 * @brief The TerminalNumberingDialog class
 * Dialog to configure the automatic numbering of terminals.
 */
class TerminalNumberingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TerminalNumberingDialog(QWidget *parent = nullptr, QETProject *project = nullptr);
    ~TerminalNumberingDialog();

    // Getters for the user's choices
    bool isXAxisPriority() const;
    bool isAlphanumeric() const;
    bool alsoNumberLetters() const;
    QStringList excludedStrips() const;

    QUndoCommand* getUndoCommand(QETProject *project) const;

private:
    Ui::TerminalNumberingDialog *ui;
    QMap<QString, QCheckBox*> m_stripCheckboxes;
};

#endif // TERMINALNUMBERINGDIALOG_H
