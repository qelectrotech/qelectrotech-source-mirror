/********************************************************************************
** Form generated from reading UI file 'terminalnumberingdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TERMINALNUMBERINGDIALOG_H
#define UI_TERMINALNUMBERINGDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TerminalNumberingDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label_description;
    QHBoxLayout *horizontalLayout_main;
    QVBoxLayout *verticalLayout_left;
    QGroupBox *groupBox_axis;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *rb_priority_x;
    QRadioButton *rb_priority_y;
    QGroupBox *groupBox_type;
    QVBoxLayout *verticalLayout_3;
    QRadioButton *rb_type_num;
    QRadioButton *rb_type_alpha;
    QHBoxLayout *horizontalLayout_also_alpha;
    QSpacerItem *spacer_also_alpha;
    QCheckBox *cb_also_alpha;
    QGroupBox *groupBox_strips;
    QVBoxLayout *verticalLayout_strips;
    QLabel *label_strips_description;
    QScrollArea *scrollArea_strips;
    QWidget *scrollAreaWidgetContents_strips;
    QVBoxLayout *verticalLayout_strips_content;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *TerminalNumberingDialog)
    {
        if (TerminalNumberingDialog->objectName().isEmpty())
            TerminalNumberingDialog->setObjectName("TerminalNumberingDialog");
        TerminalNumberingDialog->resize(500, 450);
        verticalLayout = new QVBoxLayout(TerminalNumberingDialog);
        verticalLayout->setObjectName("verticalLayout");
        label_description = new QLabel(TerminalNumberingDialog);
        label_description->setObjectName("label_description");
        label_description->setWordWrap(true);

        verticalLayout->addWidget(label_description);

        horizontalLayout_main = new QHBoxLayout();
        horizontalLayout_main->setObjectName("horizontalLayout_main");
        verticalLayout_left = new QVBoxLayout();
        verticalLayout_left->setObjectName("verticalLayout_left");
        groupBox_axis = new QGroupBox(TerminalNumberingDialog);
        groupBox_axis->setObjectName("groupBox_axis");
        verticalLayout_2 = new QVBoxLayout(groupBox_axis);
        verticalLayout_2->setObjectName("verticalLayout_2");
        rb_priority_x = new QRadioButton(groupBox_axis);
        rb_priority_x->setObjectName("rb_priority_x");
        rb_priority_x->setChecked(true);

        verticalLayout_2->addWidget(rb_priority_x);

        rb_priority_y = new QRadioButton(groupBox_axis);
        rb_priority_y->setObjectName("rb_priority_y");

        verticalLayout_2->addWidget(rb_priority_y);


        verticalLayout_left->addWidget(groupBox_axis);

        groupBox_type = new QGroupBox(TerminalNumberingDialog);
        groupBox_type->setObjectName("groupBox_type");
        verticalLayout_3 = new QVBoxLayout(groupBox_type);
        verticalLayout_3->setObjectName("verticalLayout_3");
        rb_type_num = new QRadioButton(groupBox_type);
        rb_type_num->setObjectName("rb_type_num");
        rb_type_num->setChecked(true);

        verticalLayout_3->addWidget(rb_type_num);

        rb_type_alpha = new QRadioButton(groupBox_type);
        rb_type_alpha->setObjectName("rb_type_alpha");

        verticalLayout_3->addWidget(rb_type_alpha);

        horizontalLayout_also_alpha = new QHBoxLayout();
        horizontalLayout_also_alpha->setObjectName("horizontalLayout_also_alpha");
        spacer_also_alpha = new QSpacerItem(20, 0, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_also_alpha->addItem(spacer_also_alpha);

        cb_also_alpha = new QCheckBox(groupBox_type);
        cb_also_alpha->setObjectName("cb_also_alpha");
        cb_also_alpha->setEnabled(false);

        horizontalLayout_also_alpha->addWidget(cb_also_alpha);


        verticalLayout_3->addLayout(horizontalLayout_also_alpha);


        verticalLayout_left->addWidget(groupBox_type);


        horizontalLayout_main->addLayout(verticalLayout_left);

        groupBox_strips = new QGroupBox(TerminalNumberingDialog);
        groupBox_strips->setObjectName("groupBox_strips");
        verticalLayout_strips = new QVBoxLayout(groupBox_strips);
        verticalLayout_strips->setObjectName("verticalLayout_strips");
        label_strips_description = new QLabel(groupBox_strips);
        label_strips_description->setObjectName("label_strips_description");
        label_strips_description->setWordWrap(true);

        verticalLayout_strips->addWidget(label_strips_description);

        scrollArea_strips = new QScrollArea(groupBox_strips);
        scrollArea_strips->setObjectName("scrollArea_strips");
        scrollArea_strips->setWidgetResizable(true);
        scrollAreaWidgetContents_strips = new QWidget();
        scrollAreaWidgetContents_strips->setObjectName("scrollAreaWidgetContents_strips");
        verticalLayout_strips_content = new QVBoxLayout(scrollAreaWidgetContents_strips);
        verticalLayout_strips_content->setObjectName("verticalLayout_strips_content");
        scrollArea_strips->setWidget(scrollAreaWidgetContents_strips);

        verticalLayout_strips->addWidget(scrollArea_strips);


        horizontalLayout_main->addWidget(groupBox_strips);


        verticalLayout->addLayout(horizontalLayout_main);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(TerminalNumberingDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(TerminalNumberingDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, TerminalNumberingDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, TerminalNumberingDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(TerminalNumberingDialog);
    } // setupUi

    void retranslateUi(QDialog *TerminalNumberingDialog)
    {
        TerminalNumberingDialog->setWindowTitle(QCoreApplication::translate("TerminalNumberingDialog", "Num\303\251rotation automatique des bornes", nullptr));
        label_description->setText(QCoreApplication::translate("TerminalNumberingDialog", "Cette fonction num\303\251rote les bornes du projet selon leur position. Les bornes vides ou verrouill\303\251es sont ignor\303\251es.Le marquage des bornes doit \303\252tre configur\303\251 au pr\303\251alable comme suit : '-X:AB'. La partie avant les deux-points (le bornier) peut \303\252tre nomm\303\251e au choix. 'AB' peut \303\252tre compos\303\251 de chiffres ou de lettres.\"", nullptr));
        groupBox_axis->setTitle(QCoreApplication::translate("TerminalNumberingDialog", "Priorit\303\251 des axes", nullptr));
        rb_priority_x->setText(QCoreApplication::translate("TerminalNumberingDialog", "Priorit\303\251 \303\240 l'axe X (horizontal)", nullptr));
        rb_priority_y->setText(QCoreApplication::translate("TerminalNumberingDialog", "Priorit\303\251 \303\240 l'axe Y (vertical)", nullptr));
        groupBox_type->setTitle(QCoreApplication::translate("TerminalNumberingDialog", "Type de num\303\251rotation", nullptr));
        rb_type_num->setText(QCoreApplication::translate("TerminalNumberingDialog", "Num\303\251rique uniquement (1, 2, 3...)", nullptr));
        rb_type_alpha->setText(QCoreApplication::translate("TerminalNumberingDialog", "Alphanum\303\251rique (A, B, C... 1, 2...)", nullptr));
        cb_also_alpha->setText(QCoreApplication::translate("TerminalNumberingDialog", "Num\303\251roter \303\251galement les lettres", nullptr));
        groupBox_strips->setTitle(QCoreApplication::translate("TerminalNumberingDialog", "Borniers", nullptr));
        label_strips_description->setText(QCoreApplication::translate("TerminalNumberingDialog", "D\303\251cochez les borniers dont la num\303\251rotation doit \303\252tre exclue", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TerminalNumberingDialog: public Ui_TerminalNumberingDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TERMINALNUMBERINGDIALOG_H
