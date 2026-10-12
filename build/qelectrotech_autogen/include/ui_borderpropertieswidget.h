/********************************************************************************
** Form generated from reading UI file 'borderpropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BORDERPROPERTIESWIDGET_H
#define UI_BORDERPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_BorderPropertiesWidget
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *border_gb;
    QGridLayout *gridLayout;
    QSpinBox *m_rows_height_sp;
    QSpinBox *m_colums_count_sp;
    QSpinBox *m_columns_width_sp;
    QLabel *label_3;
    QCheckBox *m_display_rows_cb;
    QSpinBox *m_rows_count_sp;
    QLabel *label_2;
    QCheckBox *m_display_columns_cb;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *BorderPropertiesWidget)
    {
        if (BorderPropertiesWidget->objectName().isEmpty())
            BorderPropertiesWidget->setObjectName("BorderPropertiesWidget");
        BorderPropertiesWidget->resize(480, 300);
        verticalLayout = new QVBoxLayout(BorderPropertiesWidget);
        verticalLayout->setObjectName("verticalLayout");
        border_gb = new QGroupBox(BorderPropertiesWidget);
        border_gb->setObjectName("border_gb");
        gridLayout = new QGridLayout(border_gb);
        gridLayout->setObjectName("gridLayout");
        m_rows_height_sp = new QSpinBox(border_gb);
        m_rows_height_sp->setObjectName("m_rows_height_sp");
        m_rows_height_sp->setMaximum(1000);

        gridLayout->addWidget(m_rows_height_sp, 1, 2, 1, 1);

        m_colums_count_sp = new QSpinBox(border_gb);
        m_colums_count_sp->setObjectName("m_colums_count_sp");

        gridLayout->addWidget(m_colums_count_sp, 0, 1, 1, 1);

        m_columns_width_sp = new QSpinBox(border_gb);
        m_columns_width_sp->setObjectName("m_columns_width_sp");
        m_columns_width_sp->setMaximum(1000);

        gridLayout->addWidget(m_columns_width_sp, 0, 2, 1, 1);

        label_3 = new QLabel(border_gb);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 1, 0, 1, 1);

        m_display_rows_cb = new QCheckBox(border_gb);
        m_display_rows_cb->setObjectName("m_display_rows_cb");

        gridLayout->addWidget(m_display_rows_cb, 1, 3, 1, 1);

        m_rows_count_sp = new QSpinBox(border_gb);
        m_rows_count_sp->setObjectName("m_rows_count_sp");

        gridLayout->addWidget(m_rows_count_sp, 1, 1, 1, 1);

        label_2 = new QLabel(border_gb);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        m_display_columns_cb = new QCheckBox(border_gb);
        m_display_columns_cb->setObjectName("m_display_columns_cb");

        gridLayout->addWidget(m_display_columns_cb, 0, 3, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 2, 1, 1, 1);


        verticalLayout->addWidget(border_gb);

        QWidget::setTabOrder(m_colums_count_sp, m_columns_width_sp);
        QWidget::setTabOrder(m_columns_width_sp, m_rows_count_sp);
        QWidget::setTabOrder(m_rows_count_sp, m_rows_height_sp);
        QWidget::setTabOrder(m_rows_height_sp, m_display_columns_cb);
        QWidget::setTabOrder(m_display_columns_cb, m_display_rows_cb);

        retranslateUi(BorderPropertiesWidget);

        QMetaObject::connectSlotsByName(BorderPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *BorderPropertiesWidget)
    {
        BorderPropertiesWidget->setWindowTitle(QCoreApplication::translate("BorderPropertiesWidget", "Form", nullptr));
        border_gb->setTitle(QCoreApplication::translate("BorderPropertiesWidget", "Dimensions du folio", nullptr));
        m_rows_height_sp->setSuffix(QCoreApplication::translate("BorderPropertiesWidget", "px", "pixel"));
        m_rows_height_sp->setPrefix(QCoreApplication::translate("BorderPropertiesWidget", "x", nullptr));
        m_columns_width_sp->setSuffix(QCoreApplication::translate("BorderPropertiesWidget", "px", "pixel"));
        m_columns_width_sp->setPrefix(QCoreApplication::translate("BorderPropertiesWidget", "x", nullptr));
        label_3->setText(QCoreApplication::translate("BorderPropertiesWidget", "Lignes :", nullptr));
        m_display_rows_cb->setText(QCoreApplication::translate("BorderPropertiesWidget", "Afficher les en-t\303\252tes", nullptr));
        label_2->setText(QCoreApplication::translate("BorderPropertiesWidget", "Colonnes :", nullptr));
        m_display_columns_cb->setText(QCoreApplication::translate("BorderPropertiesWidget", "Afficher les en-t\303\252tes", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BorderPropertiesWidget: public Ui_BorderPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BORDERPROPERTIESWIDGET_H
