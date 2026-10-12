/********************************************************************************
** Form generated from reading UI file 'replaceadvanceddialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPLACEADVANCEDDIALOG_H
#define UI_REPLACEADVANCEDDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_replaceAdvancedDialog
{
public:
    QGridLayout *gridLayout;
    QLineEdit *m_replace_le;
    QLabel *label_4;
    QLabel *label_2;
    QDialogButtonBox *m_button_box;
    QLabel *label;
    QLineEdit *m_search_le;
    QComboBox *m_what_cb;
    QComboBox *m_who_cb;
    QLabel *label_3;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *replaceAdvancedDialog)
    {
        if (replaceAdvancedDialog->objectName().isEmpty())
            replaceAdvancedDialog->setObjectName("replaceAdvancedDialog");
        replaceAdvancedDialog->resize(508, 178);
        gridLayout = new QGridLayout(replaceAdvancedDialog);
        gridLayout->setObjectName("gridLayout");
        m_replace_le = new QLineEdit(replaceAdvancedDialog);
        m_replace_le->setObjectName("m_replace_le");
        m_replace_le->setClearButtonEnabled(false);

        gridLayout->addWidget(m_replace_le, 1, 3, 1, 1);

        label_4 = new QLabel(replaceAdvancedDialog);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 1, 2, 1, 1);

        label_2 = new QLabel(replaceAdvancedDialog);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        m_button_box = new QDialogButtonBox(replaceAdvancedDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setOrientation(Qt::Horizontal);
        m_button_box->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::Reset);

        gridLayout->addWidget(m_button_box, 3, 0, 1, 4);

        label = new QLabel(replaceAdvancedDialog);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        m_search_le = new QLineEdit(replaceAdvancedDialog);
        m_search_le->setObjectName("m_search_le");

        gridLayout->addWidget(m_search_le, 1, 1, 1, 1);

        m_what_cb = new QComboBox(replaceAdvancedDialog);
        m_what_cb->setObjectName("m_what_cb");

        gridLayout->addWidget(m_what_cb, 0, 3, 1, 1);

        m_who_cb = new QComboBox(replaceAdvancedDialog);
        m_who_cb->addItem(QString());
        m_who_cb->addItem(QString());
        m_who_cb->addItem(QString());
        m_who_cb->addItem(QString());
        m_who_cb->setObjectName("m_who_cb");

        gridLayout->addWidget(m_who_cb, 0, 1, 1, 1);

        label_3 = new QLabel(replaceAdvancedDialog);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 0, 2, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 2, 1, 1, 1);

        QWidget::setTabOrder(m_search_le, m_replace_le);
        QWidget::setTabOrder(m_replace_le, m_who_cb);
        QWidget::setTabOrder(m_who_cb, m_what_cb);

        retranslateUi(replaceAdvancedDialog);

        QMetaObject::connectSlotsByName(replaceAdvancedDialog);
    } // setupUi

    void retranslateUi(QDialog *replaceAdvancedDialog)
    {
        replaceAdvancedDialog->setWindowTitle(QCoreApplication::translate("replaceAdvancedDialog", "Rechercher/Remplacer avanc\303\251", nullptr));
        label_4->setText(QCoreApplication::translate("replaceAdvancedDialog", "par :", nullptr));
        label_2->setText(QCoreApplication::translate("replaceAdvancedDialog", "Remplacer :", nullptr));
        label->setText(QCoreApplication::translate("replaceAdvancedDialog", "Qui :", nullptr));
        m_search_le->setPlaceholderText(QCoreApplication::translate("replaceAdvancedDialog", "Texte ou expression r\303\251guli\303\250re", nullptr));
        m_who_cb->setItemText(0, QCoreApplication::translate("replaceAdvancedDialog", "Folio", nullptr));
        m_who_cb->setItemText(1, QCoreApplication::translate("replaceAdvancedDialog", "\303\211l\303\251ment", nullptr));
        m_who_cb->setItemText(2, QCoreApplication::translate("replaceAdvancedDialog", "Conducteur", nullptr));
        m_who_cb->setItemText(3, QCoreApplication::translate("replaceAdvancedDialog", "Texte ind\303\251pendant", nullptr));

        label_3->setText(QCoreApplication::translate("replaceAdvancedDialog", "Quoi :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class replaceAdvancedDialog: public Ui_replaceAdvancedDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPLACEADVANCEDDIALOG_H
