/********************************************************************************
** Form generated from reading UI file 'dynamictextfieldeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DYNAMICTEXTFIELDEDITOR_H
#define UI_DYNAMICTEXTFIELDEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DynamicTextFieldEditor
{
public:
    QGridLayout *m_main_grid_layout;
    QLabel *label_3;
    QSpacerItem *verticalSpacer;
    QLabel *label_2;
    QComboBox *m_text_from_cb;
    QCheckBox *m_keep_visual_rotation_cb;
    QCheckBox *m_rotation_point_center_cb;
    QComboBox *m_elmt_info_cb;
    QSpacerItem *verticalSpacer_2;
    QSpinBox *m_width_sb;
    QSpinBox *m_rotation_sb;
    QLabel *label_6;
    QLabel *label_4;
    QLineEdit *m_user_text_le;
    QLabel *label_7;
    QPushButton *m_composite_text_pb;
    QPushButton *m_alignment_pb;
    QCheckBox *m_frame_cb;
    QPushButton *m_font_pb;
    QDoubleSpinBox *m_y_sb;
    QDoubleSpinBox *m_x_sb;
    QSpinBox *m_size_sb;
    QLabel *label_5;
    QLabel *label;
    QSpacerItem *verticalSpacer_3;

    void setupUi(QWidget *DynamicTextFieldEditor)
    {
        if (DynamicTextFieldEditor->objectName().isEmpty())
            DynamicTextFieldEditor->setObjectName("DynamicTextFieldEditor");
        DynamicTextFieldEditor->resize(354, 391);
        m_main_grid_layout = new QGridLayout(DynamicTextFieldEditor);
        m_main_grid_layout->setObjectName("m_main_grid_layout");
        label_3 = new QLabel(DynamicTextFieldEditor);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_main_grid_layout->addWidget(label_3, 6, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        m_main_grid_layout->addItem(verticalSpacer, 13, 1, 1, 1);

        label_2 = new QLabel(DynamicTextFieldEditor);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_main_grid_layout->addWidget(label_2, 4, 0, 1, 1);

        m_text_from_cb = new QComboBox(DynamicTextFieldEditor);
        m_text_from_cb->addItem(QString());
        m_text_from_cb->addItem(QString());
        m_text_from_cb->addItem(QString());
        m_text_from_cb->setObjectName("m_text_from_cb");

        m_main_grid_layout->addWidget(m_text_from_cb, 9, 1, 1, 2);

        m_keep_visual_rotation_cb = new QCheckBox(DynamicTextFieldEditor);
        m_keep_visual_rotation_cb->setObjectName("m_keep_visual_rotation_cb");

        m_main_grid_layout->addWidget(m_keep_visual_rotation_cb, 2, 2, 1, 1);

        m_rotation_point_center_cb = new QCheckBox(DynamicTextFieldEditor);
        m_rotation_point_center_cb->setObjectName("m_rotation_point_center_cb");

        m_main_grid_layout->addWidget(m_rotation_point_center_cb, 3, 2, 1, 1);

        m_elmt_info_cb = new QComboBox(DynamicTextFieldEditor);
        m_elmt_info_cb->setObjectName("m_elmt_info_cb");

        m_main_grid_layout->addWidget(m_elmt_info_cb, 11, 1, 1, 2);

        verticalSpacer_2 = new QSpacerItem(20, 18, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        m_main_grid_layout->addItem(verticalSpacer_2, 4, 1, 1, 1);

        m_width_sb = new QSpinBox(DynamicTextFieldEditor);
        m_width_sb->setObjectName("m_width_sb");
        m_width_sb->setMinimum(-1);
        m_width_sb->setMaximum(500);

        m_main_grid_layout->addWidget(m_width_sb, 4, 1, 1, 1);

        m_rotation_sb = new QSpinBox(DynamicTextFieldEditor);
        m_rotation_sb->setObjectName("m_rotation_sb");
        m_rotation_sb->setMaximum(359);

        m_main_grid_layout->addWidget(m_rotation_sb, 2, 1, 1, 1);

        label_6 = new QLabel(DynamicTextFieldEditor);
        label_6->setObjectName("label_6");
        label_6->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_main_grid_layout->addWidget(label_6, 1, 0, 1, 1);

        label_4 = new QLabel(DynamicTextFieldEditor);
        label_4->setObjectName("label_4");
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_main_grid_layout->addWidget(label_4, 9, 0, 1, 1);

        m_user_text_le = new QLineEdit(DynamicTextFieldEditor);
        m_user_text_le->setObjectName("m_user_text_le");

        m_main_grid_layout->addWidget(m_user_text_le, 10, 1, 1, 2);

        label_7 = new QLabel(DynamicTextFieldEditor);
        label_7->setObjectName("label_7");
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_main_grid_layout->addWidget(label_7, 2, 0, 1, 1);

        m_composite_text_pb = new QPushButton(DynamicTextFieldEditor);
        m_composite_text_pb->setObjectName("m_composite_text_pb");

        m_main_grid_layout->addWidget(m_composite_text_pb, 12, 1, 1, 2);

        m_alignment_pb = new QPushButton(DynamicTextFieldEditor);
        m_alignment_pb->setObjectName("m_alignment_pb");

        m_main_grid_layout->addWidget(m_alignment_pb, 4, 2, 1, 1);

        m_frame_cb = new QCheckBox(DynamicTextFieldEditor);
        m_frame_cb->setObjectName("m_frame_cb");

        m_main_grid_layout->addWidget(m_frame_cb, 7, 1, 1, 2);

        m_font_pb = new QPushButton(DynamicTextFieldEditor);
        m_font_pb->setObjectName("m_font_pb");

        m_main_grid_layout->addWidget(m_font_pb, 5, 2, 1, 1);

        m_y_sb = new QDoubleSpinBox(DynamicTextFieldEditor);
        m_y_sb->setObjectName("m_y_sb");
        m_y_sb->setMinimum(-5000.000000000000000);
        m_y_sb->setMaximum(5000.000000000000000);

        m_main_grid_layout->addWidget(m_y_sb, 1, 1, 1, 2);

        m_x_sb = new QDoubleSpinBox(DynamicTextFieldEditor);
        m_x_sb->setObjectName("m_x_sb");
        m_x_sb->setMinimum(-5000.000000000000000);
        m_x_sb->setMaximum(5000.000000000000000);

        m_main_grid_layout->addWidget(m_x_sb, 0, 1, 1, 2);

        m_size_sb = new QSpinBox(DynamicTextFieldEditor);
        m_size_sb->setObjectName("m_size_sb");

        m_main_grid_layout->addWidget(m_size_sb, 5, 1, 1, 1);

        label_5 = new QLabel(DynamicTextFieldEditor);
        label_5->setObjectName("label_5");
        label_5->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_main_grid_layout->addWidget(label_5, 0, 0, 1, 1);

        label = new QLabel(DynamicTextFieldEditor);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_main_grid_layout->addWidget(label, 5, 0, 1, 1);

        verticalSpacer_3 = new QSpacerItem(20, 18, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        m_main_grid_layout->addItem(verticalSpacer_3, 8, 1, 1, 1);

        QWidget::setTabOrder(m_x_sb, m_y_sb);
        QWidget::setTabOrder(m_y_sb, m_rotation_sb);
        QWidget::setTabOrder(m_rotation_sb, m_keep_visual_rotation_cb);
        QWidget::setTabOrder(m_keep_visual_rotation_cb, m_rotation_point_center_cb);
        QWidget::setTabOrder(m_rotation_point_center_cb, m_width_sb);
        QWidget::setTabOrder(m_width_sb, m_alignment_pb);
        QWidget::setTabOrder(m_alignment_pb, m_size_sb);
        QWidget::setTabOrder(m_size_sb, m_font_pb);
        QWidget::setTabOrder(m_font_pb, m_frame_cb);
        QWidget::setTabOrder(m_frame_cb, m_text_from_cb);
        QWidget::setTabOrder(m_text_from_cb, m_user_text_le);
        QWidget::setTabOrder(m_user_text_le, m_elmt_info_cb);
        QWidget::setTabOrder(m_elmt_info_cb, m_composite_text_pb);

        retranslateUi(DynamicTextFieldEditor);

        QMetaObject::connectSlotsByName(DynamicTextFieldEditor);
    } // setupUi

    void retranslateUi(QWidget *DynamicTextFieldEditor)
    {
        DynamicTextFieldEditor->setWindowTitle(QCoreApplication::translate("DynamicTextFieldEditor", "Form", nullptr));
        label_3->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Couleur", nullptr));
        label_2->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Largeur", nullptr));
        m_text_from_cb->setItemText(0, QCoreApplication::translate("DynamicTextFieldEditor", "Texte utilisateur", nullptr));
        m_text_from_cb->setItemText(1, QCoreApplication::translate("DynamicTextFieldEditor", "Information de l'\303\251l\303\251ment", nullptr));
        m_text_from_cb->setItemText(2, QCoreApplication::translate("DynamicTextFieldEditor", "Texte compos\303\251", nullptr));

        m_keep_visual_rotation_cb->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Conserver la rotation visuel", nullptr));
        m_rotation_point_center_cb->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Tourner autour de son propre centre", nullptr));
        label_6->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Y", nullptr));
        label_4->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Source du texte", nullptr));
        label_7->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Rotation", nullptr));
        m_composite_text_pb->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Texte compos\303\251", nullptr));
        m_alignment_pb->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Alignement", nullptr));
        m_frame_cb->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Encadrer le texte", nullptr));
        m_font_pb->setText(QString());
        label_5->setText(QCoreApplication::translate("DynamicTextFieldEditor", "X", nullptr));
        label->setText(QCoreApplication::translate("DynamicTextFieldEditor", "Police", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DynamicTextFieldEditor: public Ui_DynamicTextFieldEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DYNAMICTEXTFIELDEDITOR_H
