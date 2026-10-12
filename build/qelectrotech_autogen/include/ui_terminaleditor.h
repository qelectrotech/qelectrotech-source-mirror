/********************************************************************************
** Form generated from reading UI file 'terminaleditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TERMINALEDITOR_H
#define UI_TERMINALEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TerminalEditor
{
public:
    QGridLayout *gridLayout;
    QLabel *label_2;
    QDoubleSpinBox *m_x_dsb;
    QDoubleSpinBox *m_y_dsb;
    QLabel *label_3;
    QLineEdit *m_name_le;
    QComboBox *m_orientation_cb;
    QLabel *label;
    QLabel *label_4;
    QLabel *label_5;
    QComboBox *m_type_cb;
    QGroupBox *m_label_gb;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *m_show_name_cb;
    QGroupBox *m_text_props_gb;
    QGridLayout *gridLayout_2;
    QPushButton *m_font_pb;
    QSpinBox *m_label_size_sb;
    QLabel *label_7;
    QDoubleSpinBox *m_label_x_dsb;
    QLabel *label_8;
    QDoubleSpinBox *m_label_y_dsb;
    QLabel *label_10;
    QDoubleSpinBox *m_label_rotation_sb;
    QPushButton *m_align_pb;
    QLabel *label_13;
    QWidget *m_color_widget;
    QCheckBox *m_label_frame_cb;
    QGroupBox *m_master_label_gb;
    QGridLayout *gridLayout_3;
    QCheckBox *m_use_master_label_cb;
    QComboBox *m_master_label_cb;

    void setupUi(QWidget *TerminalEditor)
    {
        if (TerminalEditor->objectName().isEmpty())
            TerminalEditor->setObjectName("TerminalEditor");
        TerminalEditor->resize(511, 500);
        gridLayout = new QGridLayout(TerminalEditor);
        gridLayout->setObjectName("gridLayout");
        label_2 = new QLabel(TerminalEditor);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        m_x_dsb = new QDoubleSpinBox(TerminalEditor);
        m_x_dsb->setObjectName("m_x_dsb");
        m_x_dsb->setMinimum(-5000.000000000000000);
        m_x_dsb->setMaximum(5000.000000000000000);

        gridLayout->addWidget(m_x_dsb, 0, 1, 1, 1);

        m_y_dsb = new QDoubleSpinBox(TerminalEditor);
        m_y_dsb->setObjectName("m_y_dsb");
        m_y_dsb->setMinimum(-5000.000000000000000);
        m_y_dsb->setMaximum(5000.000000000000000);

        gridLayout->addWidget(m_y_dsb, 1, 1, 1, 1);

        label_3 = new QLabel(TerminalEditor);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        m_name_le = new QLineEdit(TerminalEditor);
        m_name_le->setObjectName("m_name_le");

        gridLayout->addWidget(m_name_le, 4, 1, 1, 1);

        m_orientation_cb = new QComboBox(TerminalEditor);
        m_orientation_cb->setObjectName("m_orientation_cb");

        gridLayout->addWidget(m_orientation_cb, 2, 1, 1, 1);

        label = new QLabel(TerminalEditor);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_4 = new QLabel(TerminalEditor);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 4, 0, 1, 1);

        label_5 = new QLabel(TerminalEditor);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 3, 0, 1, 1);

        m_type_cb = new QComboBox(TerminalEditor);
        m_type_cb->setObjectName("m_type_cb");

        gridLayout->addWidget(m_type_cb, 3, 1, 1, 1);

        m_label_gb = new QGroupBox(TerminalEditor);
        m_label_gb->setObjectName("m_label_gb");
        verticalLayout_2 = new QVBoxLayout(m_label_gb);
        verticalLayout_2->setObjectName("verticalLayout_2");
        m_show_name_cb = new QCheckBox(m_label_gb);
        m_show_name_cb->setObjectName("m_show_name_cb");

        verticalLayout_2->addWidget(m_show_name_cb);

        m_text_props_gb = new QGroupBox(m_label_gb);
        m_text_props_gb->setObjectName("m_text_props_gb");
        gridLayout_2 = new QGridLayout(m_text_props_gb);
        gridLayout_2->setObjectName("gridLayout_2");
        m_font_pb = new QPushButton(m_text_props_gb);
        m_font_pb->setObjectName("m_font_pb");

        gridLayout_2->addWidget(m_font_pb, 0, 0, 1, 1);

        m_label_size_sb = new QSpinBox(m_text_props_gb);
        m_label_size_sb->setObjectName("m_label_size_sb");
        m_label_size_sb->setMinimum(4);
        m_label_size_sb->setMaximum(50);
        m_label_size_sb->setValue(9);

        gridLayout_2->addWidget(m_label_size_sb, 0, 1, 1, 1);

        label_7 = new QLabel(m_text_props_gb);
        label_7->setObjectName("label_7");

        gridLayout_2->addWidget(label_7, 1, 0, 1, 1);

        m_label_x_dsb = new QDoubleSpinBox(m_text_props_gb);
        m_label_x_dsb->setObjectName("m_label_x_dsb");
        m_label_x_dsb->setMinimum(-5000.000000000000000);
        m_label_x_dsb->setMaximum(5000.000000000000000);

        gridLayout_2->addWidget(m_label_x_dsb, 1, 1, 1, 1);

        label_8 = new QLabel(m_text_props_gb);
        label_8->setObjectName("label_8");

        gridLayout_2->addWidget(label_8, 2, 0, 1, 1);

        m_label_y_dsb = new QDoubleSpinBox(m_text_props_gb);
        m_label_y_dsb->setObjectName("m_label_y_dsb");
        m_label_y_dsb->setMinimum(-5000.000000000000000);
        m_label_y_dsb->setMaximum(5000.000000000000000);

        gridLayout_2->addWidget(m_label_y_dsb, 2, 1, 1, 1);

        label_10 = new QLabel(m_text_props_gb);
        label_10->setObjectName("label_10");

        gridLayout_2->addWidget(label_10, 3, 0, 1, 1);

        m_label_rotation_sb = new QDoubleSpinBox(m_text_props_gb);
        m_label_rotation_sb->setObjectName("m_label_rotation_sb");
        m_label_rotation_sb->setWrapping(true);
        m_label_rotation_sb->setDecimals(0);
        m_label_rotation_sb->setMinimum(0.000000000000000);
        m_label_rotation_sb->setMaximum(359.000000000000000);

        gridLayout_2->addWidget(m_label_rotation_sb, 3, 1, 1, 1);

        m_align_pb = new QPushButton(m_text_props_gb);
        m_align_pb->setObjectName("m_align_pb");

        gridLayout_2->addWidget(m_align_pb, 4, 0, 1, 2);

        label_13 = new QLabel(m_text_props_gb);
        label_13->setObjectName("label_13");

        gridLayout_2->addWidget(label_13, 5, 0, 1, 1);

        m_color_widget = new QWidget(m_text_props_gb);
        m_color_widget->setObjectName("m_color_widget");
        m_color_widget->setMinimumSize(QSize(40, 24));

        gridLayout_2->addWidget(m_color_widget, 5, 1, 1, 1);

        m_label_frame_cb = new QCheckBox(m_text_props_gb);
        m_label_frame_cb->setObjectName("m_label_frame_cb");

        gridLayout_2->addWidget(m_label_frame_cb, 6, 0, 1, 2);


        verticalLayout_2->addWidget(m_text_props_gb);


        gridLayout->addWidget(m_label_gb, 5, 0, 1, 2);

        m_master_label_gb = new QGroupBox(TerminalEditor);
        m_master_label_gb->setObjectName("m_master_label_gb");
        m_master_label_gb->setVisible(false);
        gridLayout_3 = new QGridLayout(m_master_label_gb);
        gridLayout_3->setObjectName("gridLayout_3");
        m_use_master_label_cb = new QCheckBox(m_master_label_gb);
        m_use_master_label_cb->setObjectName("m_use_master_label_cb");

        gridLayout_3->addWidget(m_use_master_label_cb, 0, 0, 1, 1);

        m_master_label_cb = new QComboBox(m_master_label_gb);
        m_master_label_cb->setObjectName("m_master_label_cb");
        m_master_label_cb->setEnabled(false);

        gridLayout_3->addWidget(m_master_label_cb, 0, 1, 1, 1);


        gridLayout->addWidget(m_master_label_gb, 6, 0, 1, 2);


        retranslateUi(TerminalEditor);

        QMetaObject::connectSlotsByName(TerminalEditor);
    } // setupUi

    void retranslateUi(QWidget *TerminalEditor)
    {
        TerminalEditor->setWindowTitle(QCoreApplication::translate("TerminalEditor", "Form", nullptr));
        label_2->setText(QCoreApplication::translate("TerminalEditor", "y :", nullptr));
        label_3->setText(QCoreApplication::translate("TerminalEditor", "Orientation :", nullptr));
        label->setText(QCoreApplication::translate("TerminalEditor", "x :", nullptr));
        label_4->setText(QCoreApplication::translate("TerminalEditor", "Nom :", nullptr));
        label_5->setText(QCoreApplication::translate("TerminalEditor", "Type :", nullptr));
        m_label_gb->setTitle(QCoreApplication::translate("TerminalEditor", "Nom de la borne", nullptr));
        m_show_name_cb->setText(QCoreApplication::translate("TerminalEditor", "Afficher le nom", nullptr));
        m_text_props_gb->setTitle(QCoreApplication::translate("TerminalEditor", "Propri\303\251t\303\251s du texte", nullptr));
        m_font_pb->setText(QCoreApplication::translate("TerminalEditor", "Police", nullptr));
        label_7->setText(QCoreApplication::translate("TerminalEditor", "X :", nullptr));
        label_8->setText(QCoreApplication::translate("TerminalEditor", "Y :", nullptr));
        label_10->setText(QCoreApplication::translate("TerminalEditor", "Rotation :", nullptr));
        m_label_rotation_sb->setSuffix(QCoreApplication::translate("TerminalEditor", "\302\260", nullptr));
        m_align_pb->setText(QCoreApplication::translate("TerminalEditor", "Alignement", nullptr));
        label_13->setText(QCoreApplication::translate("TerminalEditor", "Couleur :", nullptr));
        m_label_frame_cb->setText(QCoreApplication::translate("TerminalEditor", "Encadrer le texte", nullptr));
        m_master_label_gb->setTitle(QCoreApplication::translate("TerminalEditor", "\303\211tiquette du ma\303\256tre", nullptr));
        m_use_master_label_cb->setText(QCoreApplication::translate("TerminalEditor", "Reprendre du ma\303\256tre", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TerminalEditor: public Ui_TerminalEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TERMINALEDITOR_H
