/********************************************************************************
** Form generated from reading UI file 'conductorpropertiesdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONDUCTORPROPERTIESDIALOG_H
#define UI_CONDUCTORPROPERTIESDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ConductorPropertiesDialog
{
public:
    QVBoxLayout *main_layout;
    QCheckBox *m_apply_all_cb;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ConductorPropertiesDialog)
    {
        if (ConductorPropertiesDialog->objectName().isEmpty())
            ConductorPropertiesDialog->setObjectName("ConductorPropertiesDialog");
        ConductorPropertiesDialog->resize(485, 66);
        main_layout = new QVBoxLayout(ConductorPropertiesDialog);
        main_layout->setObjectName("main_layout");
        m_apply_all_cb = new QCheckBox(ConductorPropertiesDialog);
        m_apply_all_cb->setObjectName("m_apply_all_cb");
        m_apply_all_cb->setChecked(true);

        main_layout->addWidget(m_apply_all_cb);

        buttonBox = new QDialogButtonBox(ConductorPropertiesDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        main_layout->addWidget(buttonBox);


        retranslateUi(ConductorPropertiesDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, ConductorPropertiesDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, ConductorPropertiesDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(ConductorPropertiesDialog);
    } // setupUi

    void retranslateUi(QDialog *ConductorPropertiesDialog)
    {
        ConductorPropertiesDialog->setWindowTitle(QCoreApplication::translate("ConductorPropertiesDialog", "\303\211diter les propri\303\251t\303\251s d'un conducteur", nullptr));
        m_apply_all_cb->setText(QCoreApplication::translate("ConductorPropertiesDialog", "Appliquer les propri\303\251t\303\251s \303\240 l'ensemble des conducteurs de ce potentiel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ConductorPropertiesDialog: public Ui_ConductorPropertiesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONDUCTORPROPERTIESDIALOG_H
