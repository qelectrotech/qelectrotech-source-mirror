/********************************************************************************
** Form generated from reading UI file 'addterminalstripitemdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDTERMINALSTRIPITEMDIALOG_H
#define UI_ADDTERMINALSTRIPITEMDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AddTerminalStripItemDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QComboBox *m_terminal_strip_cb;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AddTerminalStripItemDialog)
    {
        if (AddTerminalStripItemDialog->objectName().isEmpty())
            AddTerminalStripItemDialog->setObjectName("AddTerminalStripItemDialog");
        AddTerminalStripItemDialog->resize(326, 100);
        verticalLayout = new QVBoxLayout(AddTerminalStripItemDialog);
        verticalLayout->setObjectName("verticalLayout");
        label = new QLabel(AddTerminalStripItemDialog);
        label->setObjectName("label");

        verticalLayout->addWidget(label);

        m_terminal_strip_cb = new QComboBox(AddTerminalStripItemDialog);
        m_terminal_strip_cb->setObjectName("m_terminal_strip_cb");

        verticalLayout->addWidget(m_terminal_strip_cb);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(AddTerminalStripItemDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(AddTerminalStripItemDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, AddTerminalStripItemDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, AddTerminalStripItemDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(AddTerminalStripItemDialog);
    } // setupUi

    void retranslateUi(QDialog *AddTerminalStripItemDialog)
    {
        AddTerminalStripItemDialog->setWindowTitle(QCoreApplication::translate("AddTerminalStripItemDialog", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("AddTerminalStripItemDialog", "Ajouter le plan de bornes suivant :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AddTerminalStripItemDialog: public Ui_AddTerminalStripItemDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDTERMINALSTRIPITEMDIALOG_H
