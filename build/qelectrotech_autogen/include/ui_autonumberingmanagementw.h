/********************************************************************************
** Form generated from reading UI file 'autonumberingmanagementw.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUTONUMBERINGMANAGEMENTW_H
#define UI_AUTONUMBERINGMANAGEMENTW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AutoNumberingManagementW
{
public:
    QGridLayout *gridLayout_2;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_3;
    QLineEdit *m_selected_folios_le;
    QRadioButton *m_apply_locations_rb;
    QRadioButton *m_apply_folios_rb;
    QRadioButton *m_apply_project_rb;
    QLineEdit *m_selected_locations_le;
    QWidget *m_selected_folios_widget;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_5;
    QComboBox *m_from_folios_cb;
    QLabel *label_6;
    QComboBox *m_to_folios_cb;
    QDialogButtonBox *buttonBox;
    QLabel *label;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_2;
    QWidget *conductorWidget;
    QHBoxLayout *conductor_hl;
    QLabel *label_2;
    QRadioButton *m_both_conductor_rb;
    QRadioButton *m_new_conductor_rb;
    QRadioButton *m_existent_conductor_rb;
    QRadioButton *m_disable_conductor_rb;
    QSpacerItem *horizontalSpacer;
    QWidget *elementWidget;
    QHBoxLayout *element_hl;
    QLabel *label_3;
    QRadioButton *m_both_element_rb;
    QRadioButton *m_new_element_rb;
    QRadioButton *m_existent_element_rb;
    QRadioButton *m_disable_element_rb;
    QSpacerItem *horizontalSpacer_2;
    QWidget *folioWidget;
    QHBoxLayout *folio_hl;
    QLabel *label_4;
    QRadioButton *m_new_folio_rb;
    QRadioButton *m_existent_folio_rb;
    QRadioButton *m_both_folio_rb;
    QRadioButton *m_disable_folio_rb;
    QSpacerItem *horizontalSpacer_3;
    QComboBox *m_status_cb;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *AutoNumberingManagementW)
    {
        if (AutoNumberingManagementW->objectName().isEmpty())
            AutoNumberingManagementW->setObjectName("AutoNumberingManagementW");
        AutoNumberingManagementW->resize(460, 508);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(AutoNumberingManagementW->sizePolicy().hasHeightForWidth());
        AutoNumberingManagementW->setSizePolicy(sizePolicy);
        AutoNumberingManagementW->setMinimumSize(QSize(460, 0));
        gridLayout_2 = new QGridLayout(AutoNumberingManagementW);
        gridLayout_2->setObjectName("gridLayout_2");
        groupBox_2 = new QGroupBox(AutoNumberingManagementW);
        groupBox_2->setObjectName("groupBox_2");
        gridLayout_3 = new QGridLayout(groupBox_2);
        gridLayout_3->setObjectName("gridLayout_3");
        m_selected_folios_le = new QLineEdit(groupBox_2);
        m_selected_folios_le->setObjectName("m_selected_folios_le");

        gridLayout_3->addWidget(m_selected_folios_le, 2, 0, 1, 1);

        m_apply_locations_rb = new QRadioButton(groupBox_2);
        m_apply_locations_rb->setObjectName("m_apply_locations_rb");

        gridLayout_3->addWidget(m_apply_locations_rb, 3, 0, 1, 1);

        m_apply_folios_rb = new QRadioButton(groupBox_2);
        m_apply_folios_rb->setObjectName("m_apply_folios_rb");

        gridLayout_3->addWidget(m_apply_folios_rb, 1, 0, 1, 1);

        m_apply_project_rb = new QRadioButton(groupBox_2);
        m_apply_project_rb->setObjectName("m_apply_project_rb");

        gridLayout_3->addWidget(m_apply_project_rb, 0, 0, 1, 1);

        m_selected_locations_le = new QLineEdit(groupBox_2);
        m_selected_locations_le->setObjectName("m_selected_locations_le");

        gridLayout_3->addWidget(m_selected_locations_le, 4, 0, 1, 1);

        m_selected_folios_widget = new QWidget(groupBox_2);
        m_selected_folios_widget->setObjectName("m_selected_folios_widget");
        horizontalLayout_5 = new QHBoxLayout(m_selected_folios_widget);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        label_5 = new QLabel(m_selected_folios_widget);
        label_5->setObjectName("label_5");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label_5->sizePolicy().hasHeightForWidth());
        label_5->setSizePolicy(sizePolicy1);

        horizontalLayout_5->addWidget(label_5);

        m_from_folios_cb = new QComboBox(m_selected_folios_widget);
        m_from_folios_cb->setObjectName("m_from_folios_cb");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(m_from_folios_cb->sizePolicy().hasHeightForWidth());
        m_from_folios_cb->setSizePolicy(sizePolicy2);

        horizontalLayout_5->addWidget(m_from_folios_cb);

        label_6 = new QLabel(m_selected_folios_widget);
        label_6->setObjectName("label_6");
        sizePolicy1.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy1);

        horizontalLayout_5->addWidget(label_6);

        m_to_folios_cb = new QComboBox(m_selected_folios_widget);
        m_to_folios_cb->setObjectName("m_to_folios_cb");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(m_to_folios_cb->sizePolicy().hasHeightForWidth());
        m_to_folios_cb->setSizePolicy(sizePolicy3);

        horizontalLayout_5->addWidget(m_to_folios_cb);


        gridLayout_3->addWidget(m_selected_folios_widget, 5, 0, 1, 1);


        gridLayout_2->addWidget(groupBox_2, 2, 0, 1, 2);

        buttonBox = new QDialogButtonBox(AutoNumberingManagementW);
        buttonBox->setObjectName("buttonBox");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy4);
        buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Help);

        gridLayout_2->addWidget(buttonBox, 4, 0, 1, 2);

        label = new QLabel(AutoNumberingManagementW);
        label->setObjectName("label");
        sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy1);
        QFont font;
        font.setBold(true);
        label->setFont(font);

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        groupBox = new QGroupBox(AutoNumberingManagementW);
        groupBox->setObjectName("groupBox");
        verticalLayout_2 = new QVBoxLayout(groupBox);
        verticalLayout_2->setObjectName("verticalLayout_2");
        conductorWidget = new QWidget(groupBox);
        conductorWidget->setObjectName("conductorWidget");
        conductor_hl = new QHBoxLayout(conductorWidget);
        conductor_hl->setObjectName("conductor_hl");
        label_2 = new QLabel(conductorWidget);
        label_2->setObjectName("label_2");
        label_2->setFont(font);

        conductor_hl->addWidget(label_2);

        m_both_conductor_rb = new QRadioButton(conductorWidget);
        m_both_conductor_rb->setObjectName("m_both_conductor_rb");
        m_both_conductor_rb->setAutoExclusive(true);

        conductor_hl->addWidget(m_both_conductor_rb);

        m_new_conductor_rb = new QRadioButton(conductorWidget);
        m_new_conductor_rb->setObjectName("m_new_conductor_rb");
        m_new_conductor_rb->setAutoExclusive(true);

        conductor_hl->addWidget(m_new_conductor_rb);

        m_existent_conductor_rb = new QRadioButton(conductorWidget);
        m_existent_conductor_rb->setObjectName("m_existent_conductor_rb");
        m_existent_conductor_rb->setAutoExclusive(true);

        conductor_hl->addWidget(m_existent_conductor_rb);

        m_disable_conductor_rb = new QRadioButton(conductorWidget);
        m_disable_conductor_rb->setObjectName("m_disable_conductor_rb");

        conductor_hl->addWidget(m_disable_conductor_rb);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        conductor_hl->addItem(horizontalSpacer);


        verticalLayout_2->addWidget(conductorWidget);

        elementWidget = new QWidget(groupBox);
        elementWidget->setObjectName("elementWidget");
        sizePolicy1.setHeightForWidth(elementWidget->sizePolicy().hasHeightForWidth());
        elementWidget->setSizePolicy(sizePolicy1);
        element_hl = new QHBoxLayout(elementWidget);
        element_hl->setObjectName("element_hl");
        label_3 = new QLabel(elementWidget);
        label_3->setObjectName("label_3");
        label_3->setFont(font);

        element_hl->addWidget(label_3);

        m_both_element_rb = new QRadioButton(elementWidget);
        m_both_element_rb->setObjectName("m_both_element_rb");
        QSizePolicy sizePolicy5(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(m_both_element_rb->sizePolicy().hasHeightForWidth());
        m_both_element_rb->setSizePolicy(sizePolicy5);
        m_both_element_rb->setAutoExclusive(true);

        element_hl->addWidget(m_both_element_rb);

        m_new_element_rb = new QRadioButton(elementWidget);
        m_new_element_rb->setObjectName("m_new_element_rb");
        sizePolicy5.setHeightForWidth(m_new_element_rb->sizePolicy().hasHeightForWidth());
        m_new_element_rb->setSizePolicy(sizePolicy5);
        m_new_element_rb->setAutoExclusive(true);

        element_hl->addWidget(m_new_element_rb);

        m_existent_element_rb = new QRadioButton(elementWidget);
        m_existent_element_rb->setObjectName("m_existent_element_rb");
        sizePolicy5.setHeightForWidth(m_existent_element_rb->sizePolicy().hasHeightForWidth());
        m_existent_element_rb->setSizePolicy(sizePolicy5);
        m_existent_element_rb->setAutoExclusive(true);

        element_hl->addWidget(m_existent_element_rb);

        m_disable_element_rb = new QRadioButton(elementWidget);
        m_disable_element_rb->setObjectName("m_disable_element_rb");
        sizePolicy5.setHeightForWidth(m_disable_element_rb->sizePolicy().hasHeightForWidth());
        m_disable_element_rb->setSizePolicy(sizePolicy5);

        element_hl->addWidget(m_disable_element_rb);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        element_hl->addItem(horizontalSpacer_2);


        verticalLayout_2->addWidget(elementWidget);

        folioWidget = new QWidget(groupBox);
        folioWidget->setObjectName("folioWidget");
        QFont font1;
        font1.setBold(false);
        font1.setKerning(true);
        folioWidget->setFont(font1);
        folio_hl = new QHBoxLayout(folioWidget);
        folio_hl->setObjectName("folio_hl");
        label_4 = new QLabel(folioWidget);
        label_4->setObjectName("label_4");
        label_4->setFont(font);

        folio_hl->addWidget(label_4);

        m_new_folio_rb = new QRadioButton(folioWidget);
        m_new_folio_rb->setObjectName("m_new_folio_rb");
        m_new_folio_rb->setAutoExclusive(true);

        folio_hl->addWidget(m_new_folio_rb);

        m_existent_folio_rb = new QRadioButton(folioWidget);
        m_existent_folio_rb->setObjectName("m_existent_folio_rb");
        m_existent_folio_rb->setAutoExclusive(true);

        folio_hl->addWidget(m_existent_folio_rb);

        m_both_folio_rb = new QRadioButton(folioWidget);
        m_both_folio_rb->setObjectName("m_both_folio_rb");
        m_both_folio_rb->setAutoExclusive(true);

        folio_hl->addWidget(m_both_folio_rb);

        m_disable_folio_rb = new QRadioButton(folioWidget);
        m_disable_folio_rb->setObjectName("m_disable_folio_rb");

        folio_hl->addWidget(m_disable_folio_rb);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        folio_hl->addItem(horizontalSpacer_3);


        verticalLayout_2->addWidget(folioWidget);


        gridLayout_2->addWidget(groupBox, 1, 0, 1, 2);

        m_status_cb = new QComboBox(AutoNumberingManagementW);
        m_status_cb->setObjectName("m_status_cb");
        sizePolicy3.setHeightForWidth(m_status_cb->sizePolicy().hasHeightForWidth());
        m_status_cb->setSizePolicy(sizePolicy3);
        m_status_cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_2->addWidget(m_status_cb, 0, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 3, 0, 1, 1);


        retranslateUi(AutoNumberingManagementW);

        QMetaObject::connectSlotsByName(AutoNumberingManagementW);
    } // setupUi

    void retranslateUi(QWidget *AutoNumberingManagementW)
    {
        AutoNumberingManagementW->setWindowTitle(QCoreApplication::translate("AutoNumberingManagementW", "Form", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("AutoNumberingManagementW", "Range", nullptr));
        m_selected_folios_le->setText(QString());
        m_apply_locations_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Apply to Selected Locations", nullptr));
        m_apply_folios_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Apply to Selected Folios", nullptr));
        m_apply_project_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Apply to Entire Project", nullptr));
        label_5->setText(QCoreApplication::translate("AutoNumberingManagementW", "From", nullptr));
        label_6->setText(QCoreApplication::translate("AutoNumberingManagementW", "To", nullptr));
        label->setText(QCoreApplication::translate("AutoNumberingManagementW", "Project Status:", nullptr));
        groupBox->setTitle(QCoreApplication::translate("AutoNumberingManagementW", "Update Policy", nullptr));
        label_2->setText(QCoreApplication::translate("AutoNumberingManagementW", "Conductor", nullptr));
        m_both_conductor_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Both", nullptr));
        m_new_conductor_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Only New", nullptr));
        m_existent_conductor_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Only Existent", nullptr));
        m_disable_conductor_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Disable", nullptr));
        label_3->setText(QCoreApplication::translate("AutoNumberingManagementW", "Element", nullptr));
        m_both_element_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Both", nullptr));
        m_new_element_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Only New", nullptr));
        m_existent_element_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Only Existent", nullptr));
        m_disable_element_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Disable", nullptr));
        label_4->setText(QCoreApplication::translate("AutoNumberingManagementW", "Folio", nullptr));
        m_new_folio_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Only New", nullptr));
        m_existent_folio_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Existent", nullptr));
        m_both_folio_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Both", nullptr));
        m_disable_folio_rb->setText(QCoreApplication::translate("AutoNumberingManagementW", "Disable", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AutoNumberingManagementW: public Ui_AutoNumberingManagementW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUTONUMBERINGMANAGEMENTW_H
