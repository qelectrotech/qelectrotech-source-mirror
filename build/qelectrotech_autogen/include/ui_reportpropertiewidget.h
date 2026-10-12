/********************************************************************************
** Form generated from reading UI file 'reportpropertiewidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPORTPROPERTIEWIDGET_H
#define UI_REPORTPROPERTIEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ReportPropertieWidget
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *report_label_gb;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;
    QLineEdit *line_edit;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *ReportPropertieWidget)
    {
        if (ReportPropertieWidget->objectName().isEmpty())
            ReportPropertieWidget->setObjectName("ReportPropertieWidget");
        ReportPropertieWidget->resize(783, 1175);
        verticalLayout = new QVBoxLayout(ReportPropertieWidget);
        verticalLayout->setObjectName("verticalLayout");
        report_label_gb = new QGroupBox(ReportPropertieWidget);
        report_label_gb->setObjectName("report_label_gb");
        verticalLayout_2 = new QVBoxLayout(report_label_gb);
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        line_edit = new QLineEdit(report_label_gb);
        line_edit->setObjectName("line_edit");

        horizontalLayout->addWidget(line_edit);


        verticalLayout_2->addLayout(horizontalLayout);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName("verticalLayout_3");
        label_ = new QLabel(report_label_gb);
        label_->setObjectName("label_");
        label_->setScaledContents(false);
        label_->setWordWrap(true);

        verticalLayout_3->addWidget(label_);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);


        verticalLayout_2->addLayout(verticalLayout_3);


        verticalLayout->addWidget(report_label_gb);


        retranslateUi(ReportPropertieWidget);

        QMetaObject::connectSlotsByName(ReportPropertieWidget);
    } // setupUi

    void retranslateUi(QWidget *ReportPropertieWidget)
    {
        ReportPropertieWidget->setWindowTitle(QCoreApplication::translate("ReportPropertieWidget", "Form", nullptr));
        report_label_gb->setTitle(QCoreApplication::translate("ReportPropertieWidget", "Label de report de folio", nullptr));
        label_->setText(QCoreApplication::translate("ReportPropertieWidget", "Vous pouvez d\303\251finir un label personnalis\303\251 pour les reports de folio.\n"
"Cr\303\251er votre propre texte en vous aidant des variables suivantes :\n"
"%f : la position du folio dans le projet \n"
"%F : le num\303\251ro de folio\n"
"%M :  l'installation\n"
"%LM : la localisation\n"
"%l : le num\303\251ro de ligne\n"
"%c : le num\303\251ro de colonne", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ReportPropertieWidget: public Ui_ReportPropertieWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPORTPROPERTIEWIDGET_H
