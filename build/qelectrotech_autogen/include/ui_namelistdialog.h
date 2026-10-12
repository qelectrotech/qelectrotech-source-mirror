/********************************************************************************
** Form generated from reading UI file 'namelistdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NAMELISTDIALOG_H
#define UI_NAMELISTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_NameListDialog
{
public:
    QVBoxLayout *m_main_layout;
    QLabel *m_top_label;
    QDialogButtonBox *m_button_box;

    void setupUi(QDialog *NameListDialog)
    {
        if (NameListDialog->objectName().isEmpty())
            NameListDialog->setObjectName("NameListDialog");
        NameListDialog->resize(507, 370);
        m_main_layout = new QVBoxLayout(NameListDialog);
        m_main_layout->setObjectName("m_main_layout");
        m_top_label = new QLabel(NameListDialog);
        m_top_label->setObjectName("m_top_label");
        m_top_label->setTextFormat(Qt::RichText);
        m_top_label->setWordWrap(true);

        m_main_layout->addWidget(m_top_label);

        m_button_box = new QDialogButtonBox(NameListDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setOrientation(Qt::Horizontal);
        m_button_box->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        m_main_layout->addWidget(m_button_box);


        retranslateUi(NameListDialog);
        QObject::connect(m_button_box, &QDialogButtonBox::accepted, NameListDialog, qOverload<>(&QDialog::accept));
        QObject::connect(m_button_box, &QDialogButtonBox::rejected, NameListDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(NameListDialog);
    } // setupUi

    void retranslateUi(QDialog *NameListDialog)
    {
        NameListDialog->setWindowTitle(QCoreApplication::translate("NameListDialog", "Dialog", nullptr));
        m_top_label->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class NameListDialog: public Ui_NameListDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NAMELISTDIALOG_H
