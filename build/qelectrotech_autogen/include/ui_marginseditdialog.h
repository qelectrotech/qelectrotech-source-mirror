/********************************************************************************
** Form generated from reading UI file 'marginseditdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MARGINSEDITDIALOG_H
#define UI_MARGINSEDITDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_MarginsEditDialog
{
public:
    QGridLayout *gridLayout;
    QSpinBox *m_bottom_sb;
    QSpinBox *m_left_sb;
    QDialogButtonBox *buttonBox;
    QSpinBox *m_top_sb;
    QSpinBox *m_right_sb;
    QSpacerItem *verticalSpacer;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;

    void setupUi(QDialog *MarginsEditDialog)
    {
        if (MarginsEditDialog->objectName().isEmpty())
            MarginsEditDialog->setObjectName("MarginsEditDialog");
        MarginsEditDialog->resize(266, 172);
        gridLayout = new QGridLayout(MarginsEditDialog);
        gridLayout->setObjectName("gridLayout");
        m_bottom_sb = new QSpinBox(MarginsEditDialog);
        m_bottom_sb->setObjectName("m_bottom_sb");

        gridLayout->addWidget(m_bottom_sb, 3, 1, 1, 1);

        m_left_sb = new QSpinBox(MarginsEditDialog);
        m_left_sb->setObjectName("m_left_sb");

        gridLayout->addWidget(m_left_sb, 1, 1, 1, 1);

        buttonBox = new QDialogButtonBox(MarginsEditDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 5, 1, 1, 1);

        m_top_sb = new QSpinBox(MarginsEditDialog);
        m_top_sb->setObjectName("m_top_sb");

        gridLayout->addWidget(m_top_sb, 0, 1, 1, 1);

        m_right_sb = new QSpinBox(MarginsEditDialog);
        m_right_sb->setObjectName("m_right_sb");

        gridLayout->addWidget(m_right_sb, 2, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 4, 1, 1, 1);

        label = new QLabel(MarginsEditDialog);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_2 = new QLabel(MarginsEditDialog);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        label_3 = new QLabel(MarginsEditDialog);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        label_4 = new QLabel(MarginsEditDialog);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 3, 0, 1, 1);

        QWidget::setTabOrder(m_top_sb, m_left_sb);
        QWidget::setTabOrder(m_left_sb, m_right_sb);
        QWidget::setTabOrder(m_right_sb, m_bottom_sb);

        retranslateUi(MarginsEditDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, MarginsEditDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, MarginsEditDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(MarginsEditDialog);
    } // setupUi

    void retranslateUi(QDialog *MarginsEditDialog)
    {
        MarginsEditDialog->setWindowTitle(QCoreApplication::translate("MarginsEditDialog", "Editer les marges", nullptr));
        m_bottom_sb->setPrefix(QString());
        m_left_sb->setPrefix(QString());
        m_top_sb->setPrefix(QString());
        m_right_sb->setPrefix(QString());
        label->setText(QCoreApplication::translate("MarginsEditDialog", "Haut :", nullptr));
        label_2->setText(QCoreApplication::translate("MarginsEditDialog", "Gauche :", nullptr));
        label_3->setText(QCoreApplication::translate("MarginsEditDialog", "Droit :", nullptr));
        label_4->setText(QCoreApplication::translate("MarginsEditDialog", "Bas :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MarginsEditDialog: public Ui_MarginsEditDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MARGINSEDITDIALOG_H
