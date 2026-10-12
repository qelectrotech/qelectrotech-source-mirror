/********************************************************************************
** Form generated from reading UI file 'importelementdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMPORTELEMENTDIALOG_H
#define UI_IMPORTELEMENTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ImportElementDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QRadioButton *m_use_actual_rd;
    QRadioButton *m_use_drop_rb;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *m_erase_actual_rb;
    QRadioButton *m_use_both_rb;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ImportElementDialog)
    {
        if (ImportElementDialog->objectName().isEmpty())
            ImportElementDialog->setObjectName("ImportElementDialog");
        ImportElementDialog->resize(754, 176);
        verticalLayout = new QVBoxLayout(ImportElementDialog);
        verticalLayout->setObjectName("verticalLayout");
        label = new QLabel(ImportElementDialog);
        label->setObjectName("label");

        verticalLayout->addWidget(label);

        m_use_actual_rd = new QRadioButton(ImportElementDialog);
        m_use_actual_rd->setObjectName("m_use_actual_rd");

        verticalLayout->addWidget(m_use_actual_rd);

        m_use_drop_rb = new QRadioButton(ImportElementDialog);
        m_use_drop_rb->setObjectName("m_use_drop_rb");
        m_use_drop_rb->setCheckable(true);
        m_use_drop_rb->setChecked(false);

        verticalLayout->addWidget(m_use_drop_rb);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(15, -1, -1, -1);
        m_erase_actual_rb = new QRadioButton(ImportElementDialog);
        m_erase_actual_rb->setObjectName("m_erase_actual_rb");

        verticalLayout_2->addWidget(m_erase_actual_rb);

        m_use_both_rb = new QRadioButton(ImportElementDialog);
        m_use_both_rb->setObjectName("m_use_both_rb");
        m_use_both_rb->setChecked(false);

        verticalLayout_2->addWidget(m_use_both_rb);


        verticalLayout->addLayout(verticalLayout_2);

        buttonBox = new QDialogButtonBox(ImportElementDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(ImportElementDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, ImportElementDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, ImportElementDialog, qOverload<>(&QDialog::reject));
        QObject::connect(m_use_drop_rb, &QRadioButton::toggled, m_erase_actual_rb, &QRadioButton::setEnabled);
        QObject::connect(m_use_drop_rb, &QRadioButton::toggled, m_use_both_rb, &QRadioButton::setEnabled);

        QMetaObject::connectSlotsByName(ImportElementDialog);
    } // setupUi

    void retranslateUi(QDialog *ImportElementDialog)
    {
        ImportElementDialog->setWindowTitle(QCoreApplication::translate("ImportElementDialog", "Int\303\251gration d'un \303\251l\303\251ment", nullptr));
        label->setText(QCoreApplication::translate("ImportElementDialog", "L'\303\251l\303\251ment a d\303\251j\303\240 \303\251t\303\251 int\303\251gr\303\251 dans le projet. Toutefois, la version que vous tentez de poser semble diff\303\251rente. Que souhaitez-vous faire ?", nullptr));
        m_use_actual_rd->setText(QCoreApplication::translate("ImportElementDialog", "Utiliser l'\303\251l\303\251ment d\303\251j\303\240 integr\303\251", nullptr));
        m_use_drop_rb->setText(QCoreApplication::translate("ImportElementDialog", "Int\303\251grer l'\303\251l\303\251ment d\303\251pos\303\251", nullptr));
        m_erase_actual_rb->setText(QCoreApplication::translate("ImportElementDialog", "\303\211craser l'\303\251l\303\251ment d\303\251j\303\240 int\303\251g\303\251", nullptr));
        m_use_both_rb->setText(QCoreApplication::translate("ImportElementDialog", "Faire cohabiter les deux \303\251l\303\251ments", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImportElementDialog: public Ui_ImportElementDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMPORTELEMENTDIALOG_H
