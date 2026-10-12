/********************************************************************************
** Form generated from reading UI file 'bomexportdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BOMEXPORTDIALOG_H
#define UI_BOMEXPORTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_BOMExportDialog
{
public:
    QVBoxLayout *m_main_layout;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *m_include_headers;
    QCheckBox *m_format_as_bom;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *BOMExportDialog)
    {
        if (BOMExportDialog->objectName().isEmpty())
            BOMExportDialog->setObjectName("BOMExportDialog");
        BOMExportDialog->resize(610, 232);
        m_main_layout = new QVBoxLayout(BOMExportDialog);
        m_main_layout->setObjectName("m_main_layout");
        groupBox = new QGroupBox(BOMExportDialog);
        groupBox->setObjectName("groupBox");
        verticalLayout_2 = new QVBoxLayout(groupBox);
        verticalLayout_2->setObjectName("verticalLayout_2");
        m_include_headers = new QCheckBox(groupBox);
        m_include_headers->setObjectName("m_include_headers");
        m_include_headers->setChecked(true);

        verticalLayout_2->addWidget(m_include_headers);

        m_format_as_bom = new QCheckBox(groupBox);
        m_format_as_bom->setObjectName("m_format_as_bom");
        m_format_as_bom->setChecked(true);

        verticalLayout_2->addWidget(m_format_as_bom);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);


        m_main_layout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(BOMExportDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        m_main_layout->addWidget(buttonBox);


        retranslateUi(BOMExportDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, BOMExportDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, BOMExportDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(BOMExportDialog);
    } // setupUi

    void retranslateUi(QDialog *BOMExportDialog)
    {
        BOMExportDialog->setWindowTitle(QCoreApplication::translate("BOMExportDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("BOMExportDialog", "Mise en page", nullptr));
        m_include_headers->setText(QCoreApplication::translate("BOMExportDialog", "inclure les en-t\303\252tes", nullptr));
        m_format_as_bom->setText(QCoreApplication::translate("BOMExportDialog", "Formater en tant que liste de materiel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BOMExportDialog: public Ui_BOMExportDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BOMEXPORTDIALOG_H
