/********************************************************************************
** Form generated from reading UI file 'addtabledialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDTABLEDIALOG_H
#define UI_ADDTABLEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "configsaveloaderwidget.h"

QT_BEGIN_NAMESPACE

class Ui_AddTableDialog
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *m_tab;
    QWidget *m_display_tab;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *m_adjust_table_size_cb;
    QCheckBox *m_add_table_and_folio;
    QFrame *line_3;
    QLineEdit *m_table_name_le;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QComboBox *m_header_alignment_cb;
    QLabel *label_3;
    QPushButton *m_edit_header_margins_pb;
    QPushButton *m_header_font_pb;
    QLabel *label;
    QLabel *label_2;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *label_4;
    QPushButton *m_table_margins_pb;
    QLabel *label_5;
    QComboBox *m_table_alignment_cb;
    QLabel *label_6;
    QPushButton *m_table_font_pb;
    ConfigSaveLoaderWidget *m_config_gb;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *m_button_box;

    void setupUi(QDialog *AddTableDialog)
    {
        if (AddTableDialog->objectName().isEmpty())
            AddTableDialog->setObjectName("AddTableDialog");
        AddTableDialog->resize(692, 623);
        verticalLayout = new QVBoxLayout(AddTableDialog);
        verticalLayout->setObjectName("verticalLayout");
        m_tab = new QTabWidget(AddTableDialog);
        m_tab->setObjectName("m_tab");
        m_display_tab = new QWidget();
        m_display_tab->setObjectName("m_display_tab");
        verticalLayout_2 = new QVBoxLayout(m_display_tab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        m_adjust_table_size_cb = new QCheckBox(m_display_tab);
        m_adjust_table_size_cb->setObjectName("m_adjust_table_size_cb");
        m_adjust_table_size_cb->setEnabled(true);
        m_adjust_table_size_cb->setChecked(true);

        verticalLayout_2->addWidget(m_adjust_table_size_cb);

        m_add_table_and_folio = new QCheckBox(m_display_tab);
        m_add_table_and_folio->setObjectName("m_add_table_and_folio");
        m_add_table_and_folio->setEnabled(true);
        m_add_table_and_folio->setChecked(true);

        verticalLayout_2->addWidget(m_add_table_and_folio);

        line_3 = new QFrame(m_display_tab);
        line_3->setObjectName("line_3");
        line_3->setFrameShape(QFrame::Shape::HLine);
        line_3->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout_2->addWidget(line_3);

        m_table_name_le = new QLineEdit(m_display_tab);
        m_table_name_le->setObjectName("m_table_name_le");

        verticalLayout_2->addWidget(m_table_name_le);

        groupBox = new QGroupBox(m_display_tab);
        groupBox->setObjectName("groupBox");
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        m_header_alignment_cb = new QComboBox(groupBox);
        m_header_alignment_cb->addItem(QString());
        m_header_alignment_cb->addItem(QString());
        m_header_alignment_cb->addItem(QString());
        m_header_alignment_cb->setObjectName("m_header_alignment_cb");

        gridLayout->addWidget(m_header_alignment_cb, 1, 1, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        m_edit_header_margins_pb = new QPushButton(groupBox);
        m_edit_header_margins_pb->setObjectName("m_edit_header_margins_pb");

        gridLayout->addWidget(m_edit_header_margins_pb, 0, 1, 1, 1);

        m_header_font_pb = new QPushButton(groupBox);
        m_header_font_pb->setObjectName("m_header_font_pb");

        gridLayout->addWidget(m_header_font_pb, 2, 1, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);


        verticalLayout_2->addWidget(groupBox);

        groupBox_2 = new QGroupBox(m_display_tab);
        groupBox_2->setObjectName("groupBox_2");
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName("label_4");

        gridLayout_2->addWidget(label_4, 0, 0, 1, 1);

        m_table_margins_pb = new QPushButton(groupBox_2);
        m_table_margins_pb->setObjectName("m_table_margins_pb");

        gridLayout_2->addWidget(m_table_margins_pb, 0, 1, 1, 1);

        label_5 = new QLabel(groupBox_2);
        label_5->setObjectName("label_5");

        gridLayout_2->addWidget(label_5, 1, 0, 1, 1);

        m_table_alignment_cb = new QComboBox(groupBox_2);
        m_table_alignment_cb->addItem(QString());
        m_table_alignment_cb->addItem(QString());
        m_table_alignment_cb->addItem(QString());
        m_table_alignment_cb->setObjectName("m_table_alignment_cb");

        gridLayout_2->addWidget(m_table_alignment_cb, 1, 1, 1, 1);

        label_6 = new QLabel(groupBox_2);
        label_6->setObjectName("label_6");

        gridLayout_2->addWidget(label_6, 2, 0, 1, 1);

        m_table_font_pb = new QPushButton(groupBox_2);
        m_table_font_pb->setObjectName("m_table_font_pb");

        gridLayout_2->addWidget(m_table_font_pb, 2, 1, 1, 1);


        verticalLayout_2->addWidget(groupBox_2);

        m_config_gb = new ConfigSaveLoaderWidget(m_display_tab);
        m_config_gb->setObjectName("m_config_gb");
        m_config_gb->setEnabled(true);

        verticalLayout_2->addWidget(m_config_gb);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        m_tab->addTab(m_display_tab, QString());

        verticalLayout->addWidget(m_tab);

        m_button_box = new QDialogButtonBox(AddTableDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(m_button_box);

        QWidget::setTabOrder(m_adjust_table_size_cb, m_add_table_and_folio);
        QWidget::setTabOrder(m_add_table_and_folio, m_table_name_le);
        QWidget::setTabOrder(m_table_name_le, m_edit_header_margins_pb);
        QWidget::setTabOrder(m_edit_header_margins_pb, m_header_alignment_cb);
        QWidget::setTabOrder(m_header_alignment_cb, m_header_font_pb);
        QWidget::setTabOrder(m_header_font_pb, m_table_margins_pb);
        QWidget::setTabOrder(m_table_margins_pb, m_table_alignment_cb);
        QWidget::setTabOrder(m_table_alignment_cb, m_table_font_pb);
        QWidget::setTabOrder(m_table_font_pb, m_tab);

        retranslateUi(AddTableDialog);
        QObject::connect(m_button_box, &QDialogButtonBox::accepted, AddTableDialog, qOverload<>(&QDialog::accept));
        QObject::connect(m_button_box, &QDialogButtonBox::rejected, AddTableDialog, qOverload<>(&QDialog::reject));

        m_tab->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(AddTableDialog);
    } // setupUi

    void retranslateUi(QDialog *AddTableDialog)
    {
        AddTableDialog->setWindowTitle(QCoreApplication::translate("AddTableDialog", "Ajouter un tableau", nullptr));
        m_adjust_table_size_cb->setText(QCoreApplication::translate("AddTableDialog", "Ajuster la taille du tableau au folio", nullptr));
        m_add_table_and_folio->setText(QCoreApplication::translate("AddTableDialog", "Ajouter de nouveau folio et tableau si n\303\251cessaire.", nullptr));
        m_table_name_le->setText(QString());
        m_table_name_le->setPlaceholderText(QCoreApplication::translate("AddTableDialog", "Nom du tableau", nullptr));
        groupBox->setTitle(QCoreApplication::translate("AddTableDialog", "Texte des en-t\303\252tes", nullptr));
        m_header_alignment_cb->setItemText(0, QCoreApplication::translate("AddTableDialog", "Gauche", nullptr));
        m_header_alignment_cb->setItemText(1, QCoreApplication::translate("AddTableDialog", "Centre", nullptr));
        m_header_alignment_cb->setItemText(2, QCoreApplication::translate("AddTableDialog", "Droite", nullptr));

        label_3->setText(QCoreApplication::translate("AddTableDialog", "Police :", nullptr));
        m_edit_header_margins_pb->setText(QCoreApplication::translate("AddTableDialog", "\303\211diter", nullptr));
        m_header_font_pb->setText(QString());
        label->setText(QCoreApplication::translate("AddTableDialog", "Marges :", nullptr));
        label_2->setText(QCoreApplication::translate("AddTableDialog", "Alignement :", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("AddTableDialog", "Texte du tableau", nullptr));
        label_4->setText(QCoreApplication::translate("AddTableDialog", "Marges :", nullptr));
        m_table_margins_pb->setText(QCoreApplication::translate("AddTableDialog", "\303\211diter", nullptr));
        label_5->setText(QCoreApplication::translate("AddTableDialog", "Alignement :", nullptr));
        m_table_alignment_cb->setItemText(0, QCoreApplication::translate("AddTableDialog", "Gauche", nullptr));
        m_table_alignment_cb->setItemText(1, QCoreApplication::translate("AddTableDialog", "Centre", nullptr));
        m_table_alignment_cb->setItemText(2, QCoreApplication::translate("AddTableDialog", "Droite", nullptr));

        label_6->setText(QCoreApplication::translate("AddTableDialog", "Police :", nullptr));
        m_table_font_pb->setText(QString());
        m_config_gb->setTitle(QCoreApplication::translate("AddTableDialog", "Configuration", nullptr));
        m_tab->setTabText(m_tab->indexOf(m_display_tab), QCoreApplication::translate("AddTableDialog", "Affichage", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AddTableDialog: public Ui_AddTableDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDTABLEDIALOG_H
