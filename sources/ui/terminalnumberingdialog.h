#ifndef TERMINALNUMBERINGDIALOG_H
#define TERMINALNUMBERINGDIALOG_H

#include <QDialog>

class QETProject;
class QUndoCommand;

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
    explicit TerminalNumberingDialog(QWidget *parent = nullptr);
    ~TerminalNumberingDialog();

    // Getters for the user's choices
    bool isXAxisPriority() const;
    bool isAlphanumeric() const;

    QUndoCommand* getUndoCommand(QETProject *project) const;

private:
    Ui::TerminalNumberingDialog *ui;
};

#endif // TERMINALNUMBERINGDIALOG_H
