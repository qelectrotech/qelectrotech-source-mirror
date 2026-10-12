/********************************************************************************
** Form generated from reading UI file 'formulaassistantdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FORMULAASSISTANTDIALOG_H
#define UI_FORMULAASSISTANTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_FormulaAssistantDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *m_label;
    QLineEdit *m_line_edit;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *m_button_box;

    void setupUi(QDialog *FormulaAssistantDialog)
    {
        if (FormulaAssistantDialog->objectName().isEmpty())
            FormulaAssistantDialog->setObjectName("FormulaAssistantDialog");
        FormulaAssistantDialog->resize(184, 94);
        verticalLayout = new QVBoxLayout(FormulaAssistantDialog);
        verticalLayout->setObjectName("verticalLayout");
        m_label = new QLabel(FormulaAssistantDialog);
        m_label->setObjectName("m_label");

        verticalLayout->addWidget(m_label);

        m_line_edit = new QLineEdit(FormulaAssistantDialog);
        m_line_edit->setObjectName("m_line_edit");

        verticalLayout->addWidget(m_line_edit);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        m_button_box = new QDialogButtonBox(FormulaAssistantDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setOrientation(Qt::Horizontal);
        m_button_box->setStandardButtons(QDialogButtonBox::Ok);

        verticalLayout->addWidget(m_button_box);


        retranslateUi(FormulaAssistantDialog);
        QObject::connect(m_button_box, &QDialogButtonBox::accepted, FormulaAssistantDialog, qOverload<>(&QDialog::accept));
        QObject::connect(m_button_box, &QDialogButtonBox::rejected, FormulaAssistantDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(FormulaAssistantDialog);
    } // setupUi

    void retranslateUi(QDialog *FormulaAssistantDialog)
    {
        FormulaAssistantDialog->setWindowTitle(QCoreApplication::translate("FormulaAssistantDialog", "Assistant de formule", nullptr));
        m_label->setText(QCoreApplication::translate("FormulaAssistantDialog", "TextLabel", nullptr));
        m_line_edit->setPlaceholderText(QCoreApplication::translate("FormulaAssistantDialog", "Formule", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FormulaAssistantDialog: public Ui_FormulaAssistantDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FORMULAASSISTANTDIALOG_H
