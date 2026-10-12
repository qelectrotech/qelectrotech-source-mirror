/********************************************************************************
** Form generated from reading UI file 'xrefpropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_XREFPROPERTIESWIDGET_H
#define UI_XREFPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_XRefPropertiesWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_4;
    QComboBox *m_type_cb;
    QGroupBox *m_display_gb;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout;
    QLabel *label_3;
    QComboBox *m_snap_to_cb;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_9;
    QSpinBox *m_offset_sb;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_10;
    QComboBox *m_xrefpos_cb;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_11;
    QSpinBox *m_slave_offset_sb;
    QHBoxLayout *horizontalLayout_3;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *m_show_terminal_name_cb;
    QRadioButton *m_display_has_contacts_rb;
    QRadioButton *m_display_has_cross_rb;
    QVBoxLayout *verticalLayout_5;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_7;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_6;
    QLineEdit *m_master_le;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_7;
    QLineEdit *m_slave_le;
    QLabel *label_8;
    QGroupBox *m_cross_properties_gb;
    QVBoxLayout *verticalLayout_4;
    QCheckBox *m_show_power_cb;
    QGridLayout *gridLayout;
    QLabel *label;
    QLabel *label_2;
    QLineEdit *m_delay_prefix_le;
    QLineEdit *m_power_prefix_le;
    QLabel *label_5;
    QLineEdit *m_switch_prefix_le;

    void setupUi(QWidget *XRefPropertiesWidget)
    {
        if (XRefPropertiesWidget->objectName().isEmpty())
            XRefPropertiesWidget->setObjectName("XRefPropertiesWidget");
        XRefPropertiesWidget->resize(376, 531);
        verticalLayout = new QVBoxLayout(XRefPropertiesWidget);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_4 = new QLabel(XRefPropertiesWidget);
        label_4->setObjectName("label_4");

        horizontalLayout_2->addWidget(label_4);

        m_type_cb = new QComboBox(XRefPropertiesWidget);
        m_type_cb->setObjectName("m_type_cb");

        horizontalLayout_2->addWidget(m_type_cb);


        verticalLayout->addLayout(horizontalLayout_2);

        m_display_gb = new QGroupBox(XRefPropertiesWidget);
        m_display_gb->setObjectName("m_display_gb");
        verticalLayout_3 = new QVBoxLayout(m_display_gb);
        verticalLayout_3->setObjectName("verticalLayout_3");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label_3 = new QLabel(m_display_gb);
        label_3->setObjectName("label_3");

        horizontalLayout->addWidget(label_3);

        m_snap_to_cb = new QComboBox(m_display_gb);
        m_snap_to_cb->setObjectName("m_snap_to_cb");

        horizontalLayout->addWidget(m_snap_to_cb);


        verticalLayout_3->addLayout(horizontalLayout);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        label_9 = new QLabel(m_display_gb);
        label_9->setObjectName("label_9");

        horizontalLayout_7->addWidget(label_9);

        m_offset_sb = new QSpinBox(m_display_gb);
        m_offset_sb->setObjectName("m_offset_sb");
        m_offset_sb->setCorrectionMode(QAbstractSpinBox::CorrectToNearestValue);
        m_offset_sb->setSuffix(QString::fromUtf8("px"));
        m_offset_sb->setMinimum(40);
        m_offset_sb->setMaximum(300);
        m_offset_sb->setSingleStep(10);
        m_offset_sb->setValue(40);

        horizontalLayout_7->addWidget(m_offset_sb);


        verticalLayout_3->addLayout(horizontalLayout_7);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        label_10 = new QLabel(m_display_gb);
        label_10->setObjectName("label_10");

        horizontalLayout_6->addWidget(label_10);

        m_xrefpos_cb = new QComboBox(m_display_gb);
        m_xrefpos_cb->setObjectName("m_xrefpos_cb");

        horizontalLayout_6->addWidget(m_xrefpos_cb);


        verticalLayout_3->addLayout(horizontalLayout_6);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        label_11 = new QLabel(m_display_gb);
        label_11->setObjectName("label_11");

        horizontalLayout_8->addWidget(label_11);

        m_slave_offset_sb = new QSpinBox(m_display_gb);
        m_slave_offset_sb->setObjectName("m_slave_offset_sb");
        m_slave_offset_sb->setSuffix(QString::fromUtf8("px"));
        m_slave_offset_sb->setCorrectionMode(QAbstractSpinBox::CorrectToNearestValue);
        m_slave_offset_sb->setMinimum(-50);
        m_slave_offset_sb->setMaximum(100);
        m_slave_offset_sb->setSingleStep(1);
        m_slave_offset_sb->setValue(0);

        horizontalLayout_8->addWidget(m_slave_offset_sb);


        verticalLayout_3->addLayout(horizontalLayout_8);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        m_show_terminal_name_cb = new QCheckBox(m_display_gb);
        m_show_terminal_name_cb->setObjectName("m_show_terminal_name_cb");

        verticalLayout_2->addWidget(m_show_terminal_name_cb);

        m_display_has_contacts_rb = new QRadioButton(m_display_gb);
        m_display_has_contacts_rb->setObjectName("m_display_has_contacts_rb");

        verticalLayout_2->addWidget(m_display_has_contacts_rb);

        m_display_has_cross_rb = new QRadioButton(m_display_gb);
        m_display_has_cross_rb->setObjectName("m_display_has_cross_rb");

        verticalLayout_2->addWidget(m_display_has_cross_rb);


        horizontalLayout_3->addLayout(verticalLayout_2);


        verticalLayout_3->addLayout(horizontalLayout_3);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName("verticalLayout_5");
        groupBox = new QGroupBox(m_display_gb);
        groupBox->setObjectName("groupBox");
        verticalLayout_7 = new QVBoxLayout(groupBox);
        verticalLayout_7->setObjectName("verticalLayout_7");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        label_6 = new QLabel(groupBox);
        label_6->setObjectName("label_6");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy);
        label_6->setMinimumSize(QSize(48, 0));

        horizontalLayout_4->addWidget(label_6);

        m_master_le = new QLineEdit(groupBox);
        m_master_le->setObjectName("m_master_le");

        horizontalLayout_4->addWidget(m_master_le);


        verticalLayout_7->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        label_7 = new QLabel(groupBox);
        label_7->setObjectName("label_7");
        sizePolicy.setHeightForWidth(label_7->sizePolicy().hasHeightForWidth());
        label_7->setSizePolicy(sizePolicy);
        label_7->setMinimumSize(QSize(48, 0));
        label_7->setScaledContents(false);

        horizontalLayout_5->addWidget(label_7);

        m_slave_le = new QLineEdit(groupBox);
        m_slave_le->setObjectName("m_slave_le");

        horizontalLayout_5->addWidget(m_slave_le);


        verticalLayout_7->addLayout(horizontalLayout_5);

        label_8 = new QLabel(groupBox);
        label_8->setObjectName("label_8");

        verticalLayout_7->addWidget(label_8);


        verticalLayout_5->addWidget(groupBox);


        verticalLayout_3->addLayout(verticalLayout_5);


        verticalLayout->addWidget(m_display_gb);

        m_cross_properties_gb = new QGroupBox(XRefPropertiesWidget);
        m_cross_properties_gb->setObjectName("m_cross_properties_gb");
        verticalLayout_4 = new QVBoxLayout(m_cross_properties_gb);
        verticalLayout_4->setObjectName("verticalLayout_4");
        m_show_power_cb = new QCheckBox(m_cross_properties_gb);
        m_show_power_cb->setObjectName("m_show_power_cb");

        verticalLayout_4->addWidget(m_show_power_cb);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        label = new QLabel(m_cross_properties_gb);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_2 = new QLabel(m_cross_properties_gb);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        m_delay_prefix_le = new QLineEdit(m_cross_properties_gb);
        m_delay_prefix_le->setObjectName("m_delay_prefix_le");

        gridLayout->addWidget(m_delay_prefix_le, 1, 1, 1, 1);

        m_power_prefix_le = new QLineEdit(m_cross_properties_gb);
        m_power_prefix_le->setObjectName("m_power_prefix_le");

        gridLayout->addWidget(m_power_prefix_le, 0, 1, 1, 1);

        label_5 = new QLabel(m_cross_properties_gb);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 2, 0, 1, 1);

        m_switch_prefix_le = new QLineEdit(m_cross_properties_gb);
        m_switch_prefix_le->setObjectName("m_switch_prefix_le");

        gridLayout->addWidget(m_switch_prefix_le, 2, 1, 1, 1);


        verticalLayout_4->addLayout(gridLayout);


        verticalLayout->addWidget(m_cross_properties_gb);

        QWidget::setTabOrder(m_type_cb, m_snap_to_cb);
        QWidget::setTabOrder(m_snap_to_cb, m_offset_sb);
        QWidget::setTabOrder(m_offset_sb, m_xrefpos_cb);
        QWidget::setTabOrder(m_xrefpos_cb, m_display_has_contacts_rb);
        QWidget::setTabOrder(m_display_has_contacts_rb, m_display_has_cross_rb);
        QWidget::setTabOrder(m_display_has_cross_rb, m_master_le);
        QWidget::setTabOrder(m_master_le, m_slave_le);
        QWidget::setTabOrder(m_slave_le, m_show_power_cb);
        QWidget::setTabOrder(m_show_power_cb, m_show_terminal_name_cb);
        QWidget::setTabOrder(m_show_terminal_name_cb, m_power_prefix_le);
        QWidget::setTabOrder(m_power_prefix_le, m_delay_prefix_le);
        QWidget::setTabOrder(m_delay_prefix_le, m_switch_prefix_le);

        retranslateUi(XRefPropertiesWidget);

        QMetaObject::connectSlotsByName(XRefPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *XRefPropertiesWidget)
    {
        XRefPropertiesWidget->setWindowTitle(QCoreApplication::translate("XRefPropertiesWidget", "Form", nullptr));
        label_4->setText(QCoreApplication::translate("XRefPropertiesWidget", "Type :", nullptr));
        m_display_gb->setTitle(QCoreApplication::translate("XRefPropertiesWidget", "Repr\303\251sentation:", nullptr));
        label_3->setText(QCoreApplication::translate("XRefPropertiesWidget", "Positionner :", nullptr));
        label_9->setText(QCoreApplication::translate("XRefPropertiesWidget", "XRef Vertical Offset:", nullptr));
#if QT_CONFIG(tooltip)
        m_offset_sb->setToolTip(QCoreApplication::translate("XRefPropertiesWidget", "10px corresponds to 1 tile displacement", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        m_offset_sb->setWhatsThis(QCoreApplication::translate("XRefPropertiesWidget", "Set Vertical Offset for the Cross References. 10px corresponds to 1 tile displacement.", nullptr));
#endif // QT_CONFIG(whatsthis)
        m_offset_sb->setSpecialValueText(QCoreApplication::translate("XRefPropertiesWidget", "Default - Fit to XRef height", nullptr));
        label_10->setText(QCoreApplication::translate("XRefPropertiesWidget", "XRef slave position", nullptr));
        label_11->setText(QCoreApplication::translate("XRefPropertiesWidget", "Distance label - slave :", nullptr));
#if QT_CONFIG(tooltip)
        m_slave_offset_sb->setToolTip(QCoreApplication::translate("XRefPropertiesWidget", "Distance in pixels between the label and the slave cross reference", nullptr));
#endif // QT_CONFIG(tooltip)
        m_show_terminal_name_cb->setText(QCoreApplication::translate("XRefPropertiesWidget", "Afficher les num\303\251ros de bornes dans les Xrefs", nullptr));
        m_display_has_contacts_rb->setText(QCoreApplication::translate("XRefPropertiesWidget", "Affiche&r en contacts", nullptr));
        m_display_has_cross_rb->setText(QCoreApplication::translate("XRefPropertiesWidget", "Afficher en croix", nullptr));
        groupBox->setTitle(QCoreApplication::translate("XRefPropertiesWidget", "Label des r\303\251f\303\251rences crois\303\251es", nullptr));
        label_6->setText(QCoreApplication::translate("XRefPropertiesWidget", "Maitre", nullptr));
        m_master_le->setInputMask(QString());
        m_master_le->setText(QCoreApplication::translate("XRefPropertiesWidget", "%f-%l%c", nullptr));
        label_7->setText(QCoreApplication::translate("XRefPropertiesWidget", "Esclave", nullptr));
        m_slave_le->setText(QCoreApplication::translate("XRefPropertiesWidget", "(%f-%l%c)", nullptr));
        label_8->setText(QCoreApplication::translate("XRefPropertiesWidget", "Cr\303\251er votre propre texte en vous aidant des variables suivantes :\n"
"%f : le num\303\251ro de folio \n"
"%F: le label de folio\n"
"%l : le num\303\251ro de ligne\n"
"%c : le num\303\251ro de colonne\n"
"%M: Installation \n"
"%LM: Localisation ", nullptr));
        m_cross_properties_gb->setTitle(QCoreApplication::translate("XRefPropertiesWidget", "Option d'affichage en croix", nullptr));
        m_show_power_cb->setText(QCoreApplication::translate("XRefPropertiesWidget", "Afficher les contacts de puissance dans la croix", nullptr));
        label->setText(QCoreApplication::translate("XRefPropertiesWidget", "Pr\303\251fixe des contacts de puissance :", nullptr));
        label_2->setText(QCoreApplication::translate("XRefPropertiesWidget", "Pr\303\251fixe des contacts temporis\303\251s :", nullptr));
        label_5->setText(QCoreApplication::translate("XRefPropertiesWidget", "Pr\303\251fixe des contacts inverseurs :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class XRefPropertiesWidget: public Ui_XRefPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_XREFPROPERTIESWIDGET_H
