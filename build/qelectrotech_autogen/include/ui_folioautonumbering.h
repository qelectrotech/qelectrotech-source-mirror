/********************************************************************************
** Form generated from reading UI file 'folioautonumbering.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FOLIOAUTONUMBERING_H
#define UI_FOLIOAUTONUMBERING_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FolioAutonumberingW
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout_3;
    QLabel *label;
    QComboBox *m_autonums_cb;
    QGroupBox *Select;
    QHBoxLayout *horizontalLayout;
    QRadioButton *m_create_new_tabs_rb;
    QRadioButton *m_autonumber_tabs_rb;
    QGroupBox *Options;
    QGridLayout *gridLayout_2;
    QComboBox *m_to_cb;
    QLabel *label_2;
    QComboBox *m_from_cb;
    QSpinBox *m_new_tabs_sb;
    QLabel *label_4;
    QLabel *label_3;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *FolioAutonumberingW)
    {
        if (FolioAutonumberingW->objectName().isEmpty())
            FolioAutonumberingW->setObjectName("FolioAutonumberingW");
        FolioAutonumberingW->resize(1082, 515);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(FolioAutonumberingW->sizePolicy().hasHeightForWidth());
        FolioAutonumberingW->setSizePolicy(sizePolicy);
        FolioAutonumberingW->setMinimumSize(QSize(460, 0));
        verticalLayout = new QVBoxLayout(FolioAutonumberingW);
        verticalLayout->setObjectName("verticalLayout");
        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName("gridLayout_3");
        label = new QLabel(FolioAutonumberingW);
        label->setObjectName("label");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy1);

        gridLayout_3->addWidget(label, 0, 0, 1, 1);

        m_autonums_cb = new QComboBox(FolioAutonumberingW);
        m_autonums_cb->setObjectName("m_autonums_cb");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(m_autonums_cb->sizePolicy().hasHeightForWidth());
        m_autonums_cb->setSizePolicy(sizePolicy2);
        m_autonums_cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_3->addWidget(m_autonums_cb, 0, 1, 1, 1);

        Select = new QGroupBox(FolioAutonumberingW);
        Select->setObjectName("Select");
        sizePolicy1.setHeightForWidth(Select->sizePolicy().hasHeightForWidth());
        Select->setSizePolicy(sizePolicy1);
        horizontalLayout = new QHBoxLayout(Select);
        horizontalLayout->setObjectName("horizontalLayout");
        m_create_new_tabs_rb = new QRadioButton(Select);
        m_create_new_tabs_rb->setObjectName("m_create_new_tabs_rb");
        sizePolicy2.setHeightForWidth(m_create_new_tabs_rb->sizePolicy().hasHeightForWidth());
        m_create_new_tabs_rb->setSizePolicy(sizePolicy2);

        horizontalLayout->addWidget(m_create_new_tabs_rb);

        m_autonumber_tabs_rb = new QRadioButton(Select);
        m_autonumber_tabs_rb->setObjectName("m_autonumber_tabs_rb");
        sizePolicy2.setHeightForWidth(m_autonumber_tabs_rb->sizePolicy().hasHeightForWidth());
        m_autonumber_tabs_rb->setSizePolicy(sizePolicy2);

        horizontalLayout->addWidget(m_autonumber_tabs_rb);


        gridLayout_3->addWidget(Select, 1, 0, 1, 2);

        Options = new QGroupBox(FolioAutonumberingW);
        Options->setObjectName("Options");
        sizePolicy1.setHeightForWidth(Options->sizePolicy().hasHeightForWidth());
        Options->setSizePolicy(sizePolicy1);
        gridLayout_2 = new QGridLayout(Options);
        gridLayout_2->setObjectName("gridLayout_2");
        m_to_cb = new QComboBox(Options);
        m_to_cb->setObjectName("m_to_cb");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(m_to_cb->sizePolicy().hasHeightForWidth());
        m_to_cb->setSizePolicy(sizePolicy3);
        m_to_cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_2->addWidget(m_to_cb, 2, 1, 1, 1);

        label_2 = new QLabel(Options);
        label_2->setObjectName("label_2");
        sizePolicy1.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy1);

        gridLayout_2->addWidget(label_2, 0, 0, 1, 1);

        m_from_cb = new QComboBox(Options);
        m_from_cb->setObjectName("m_from_cb");
        sizePolicy3.setHeightForWidth(m_from_cb->sizePolicy().hasHeightForWidth());
        m_from_cb->setSizePolicy(sizePolicy3);
        m_from_cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_2->addWidget(m_from_cb, 1, 1, 1, 1);

        m_new_tabs_sb = new QSpinBox(Options);
        m_new_tabs_sb->setObjectName("m_new_tabs_sb");
        sizePolicy3.setHeightForWidth(m_new_tabs_sb->sizePolicy().hasHeightForWidth());
        m_new_tabs_sb->setSizePolicy(sizePolicy3);
        m_new_tabs_sb->setMaximum(999);
        m_new_tabs_sb->setDisplayIntegerBase(10);

        gridLayout_2->addWidget(m_new_tabs_sb, 0, 1, 1, 1);

        label_4 = new QLabel(Options);
        label_4->setObjectName("label_4");
        sizePolicy1.setHeightForWidth(label_4->sizePolicy().hasHeightForWidth());
        label_4->setSizePolicy(sizePolicy1);

        gridLayout_2->addWidget(label_4, 2, 0, 1, 1);

        label_3 = new QLabel(Options);
        label_3->setObjectName("label_3");
        sizePolicy1.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy1);

        gridLayout_2->addWidget(label_3, 1, 0, 1, 1);


        gridLayout_3->addWidget(Options, 2, 0, 1, 2);


        verticalLayout->addLayout(gridLayout_3);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(FolioAutonumberingW);
        buttonBox->setObjectName("buttonBox");
        sizePolicy2.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy2);
        buttonBox->setStandardButtons(QDialogButtonBox::Apply);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(FolioAutonumberingW);

        QMetaObject::connectSlotsByName(FolioAutonumberingW);
    } // setupUi

    void retranslateUi(QWidget *FolioAutonumberingW)
    {
        FolioAutonumberingW->setWindowTitle(QCoreApplication::translate("FolioAutonumberingW", "Form", nullptr));
        label->setText(QCoreApplication::translate("FolioAutonumberingW", "Num\303\251rotation automatique de Folio :", nullptr));
        Select->setTitle(QCoreApplication::translate("FolioAutonumberingW", "S\303\251lection:", nullptr));
        m_create_new_tabs_rb->setText(QCoreApplication::translate("FolioAutonumberingW", "C&r\303\251er de nouveaux folios", nullptr));
        m_autonumber_tabs_rb->setText(QCoreApplication::translate("FolioAutonumberingW", "Num\303\251rotation automatique des folios s\303\251lectionn\303\251s", nullptr));
        Options->setTitle(QCoreApplication::translate("FolioAutonumberingW", " Options de num\303\251rotation", nullptr));
        label_2->setText(QCoreApplication::translate("FolioAutonumberingW", "Nouveaux folios", nullptr));
        label_4->setText(QCoreApplication::translate("FolioAutonumberingW", "\303\200", nullptr));
        label_3->setText(QCoreApplication::translate("FolioAutonumberingW", "De", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FolioAutonumberingW: public Ui_FolioAutonumberingW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FOLIOAUTONUMBERING_H
