/********************************************************************************
** Form generated from reading UI file 'renamedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RENAMEDIALOG_H
#define UI_RENAMEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_RenameDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *label_2;
    QPushButton *m_erase_pb;
    QPushButton *m_rename_pb;
    QPushButton *m_cancel_pb;
    QLineEdit *lineEdit;
    QLabel *m_label;

    void setupUi(QDialog *RenameDialog)
    {
        if (RenameDialog->objectName().isEmpty())
            RenameDialog->setObjectName("RenameDialog");
        RenameDialog->resize(284, 88);
        gridLayout = new QGridLayout(RenameDialog);
        gridLayout->setObjectName("gridLayout");
        label_2 = new QLabel(RenameDialog);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        m_erase_pb = new QPushButton(RenameDialog);
        m_erase_pb->setObjectName("m_erase_pb");

        gridLayout->addWidget(m_erase_pb, 2, 0, 1, 1);

        m_rename_pb = new QPushButton(RenameDialog);
        m_rename_pb->setObjectName("m_rename_pb");

        gridLayout->addWidget(m_rename_pb, 2, 1, 1, 1);

        m_cancel_pb = new QPushButton(RenameDialog);
        m_cancel_pb->setObjectName("m_cancel_pb");

        gridLayout->addWidget(m_cancel_pb, 2, 2, 1, 1);

        lineEdit = new QLineEdit(RenameDialog);
        lineEdit->setObjectName("lineEdit");

        gridLayout->addWidget(lineEdit, 1, 1, 1, 2);

        m_label = new QLabel(RenameDialog);
        m_label->setObjectName("m_label");

        gridLayout->addWidget(m_label, 0, 0, 1, 3);


        retranslateUi(RenameDialog);

        QMetaObject::connectSlotsByName(RenameDialog);
    } // setupUi

    void retranslateUi(QDialog *RenameDialog)
    {
        RenameDialog->setWindowTitle(QCoreApplication::translate("RenameDialog", "Dialog", nullptr));
        label_2->setText(QCoreApplication::translate("RenameDialog", "Nouveau nom :", nullptr));
        m_erase_pb->setText(QCoreApplication::translate("RenameDialog", "\303\211craser", nullptr));
        m_rename_pb->setText(QCoreApplication::translate("RenameDialog", "Renommer", nullptr));
        m_cancel_pb->setText(QCoreApplication::translate("RenameDialog", "Annuler", nullptr));
        m_label->setText(QCoreApplication::translate("RenameDialog", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class RenameDialog: public Ui_RenameDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RENAMEDIALOG_H
