/********************************************************************************
** Form generated from reading UI file 'potentialselectordialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_POTENTIALSELECTORDIALOG_H
#define UI_POTENTIALSELECTORDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_PotentialSelectorDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *PotentialSelectorDialog)
    {
        if (PotentialSelectorDialog->objectName().isEmpty())
            PotentialSelectorDialog->setObjectName("PotentialSelectorDialog");
        PotentialSelectorDialog->resize(425, 74);
        PotentialSelectorDialog->setSizeGripEnabled(false);
        PotentialSelectorDialog->setModal(true);
        verticalLayout = new QVBoxLayout(PotentialSelectorDialog);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        verticalLayout->setContentsMargins(-1, 9, -1, -1);
        label = new QLabel(PotentialSelectorDialog);
        label->setObjectName("label");
        label->setFrameShape(QFrame::NoFrame);
        label->setFrameShadow(QFrame::Plain);
        label->setTextFormat(Qt::PlainText);
        label->setScaledContents(false);
        label->setWordWrap(false);

        verticalLayout->addWidget(label);

        buttonBox = new QDialogButtonBox(PotentialSelectorDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(PotentialSelectorDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, PotentialSelectorDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, PotentialSelectorDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(PotentialSelectorDialog);
    } // setupUi

    void retranslateUi(QDialog *PotentialSelectorDialog)
    {
        PotentialSelectorDialog->setWindowTitle(QCoreApplication::translate("PotentialSelectorDialog", "S\303\251lectionner le potentiel \303\251l\303\251ctrique", nullptr));
        label->setText(QCoreApplication::translate("PotentialSelectorDialog", "Vous tentez de lier deux potentiels diff\303\251rents ensemble.\n"
"Veuillez choisir les propri\303\251t\303\251es \303\240 appliquer au nouveau potentiel.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PotentialSelectorDialog: public Ui_PotentialSelectorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_POTENTIALSELECTORDIALOG_H
