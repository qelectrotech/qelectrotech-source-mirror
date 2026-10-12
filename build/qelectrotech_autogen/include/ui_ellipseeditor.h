/********************************************************************************
** Form generated from reading UI file 'ellipseeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ELLIPSEEDITOR_H
#define UI_ELLIPSEEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EllipseEditor
{
public:
    QVBoxLayout *m_main_layout;
    QWidget *widget;
    QGridLayout *m_geometry_widget;
    QLabel *label;
    QLabel *label_3;
    QDoubleSpinBox *m_vertical_diameter_sb;
    QDoubleSpinBox *m_y_sb;
    QLabel *label_2;
    QLabel *label_6;
    QDoubleSpinBox *m_x_sb;
    QLabel *label_5;
    QDoubleSpinBox *m_horizontal_diameter_sb;
    QLabel *label_4;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *EllipseEditor)
    {
        if (EllipseEditor->objectName().isEmpty())
            EllipseEditor->setObjectName("EllipseEditor");
        EllipseEditor->resize(314, 173);
        m_main_layout = new QVBoxLayout(EllipseEditor);
        m_main_layout->setObjectName("m_main_layout");
        widget = new QWidget(EllipseEditor);
        widget->setObjectName("widget");
        m_geometry_widget = new QGridLayout(widget);
        m_geometry_widget->setObjectName("m_geometry_widget");
        label = new QLabel(widget);
        label->setObjectName("label");

        m_geometry_widget->addWidget(label, 0, 0, 1, 5);

        label_3 = new QLabel(widget);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_geometry_widget->addWidget(label_3, 1, 2, 1, 1);

        m_vertical_diameter_sb = new QDoubleSpinBox(widget);
        m_vertical_diameter_sb->setObjectName("m_vertical_diameter_sb");
        m_vertical_diameter_sb->setMinimum(-10000.000000000000000);
        m_vertical_diameter_sb->setMaximum(10000.000000000000000);

        m_geometry_widget->addWidget(m_vertical_diameter_sb, 5, 1, 1, 1);

        m_y_sb = new QDoubleSpinBox(widget);
        m_y_sb->setObjectName("m_y_sb");
        m_y_sb->setMinimum(-10000.000000000000000);
        m_y_sb->setMaximum(10000.000000000000000);

        m_geometry_widget->addWidget(m_y_sb, 1, 3, 1, 1);

        label_2 = new QLabel(widget);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_geometry_widget->addWidget(label_2, 1, 0, 1, 1);

        label_6 = new QLabel(widget);
        label_6->setObjectName("label_6");
        label_6->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        m_geometry_widget->addWidget(label_6, 5, 0, 1, 1);

        m_x_sb = new QDoubleSpinBox(widget);
        m_x_sb->setObjectName("m_x_sb");
        m_x_sb->setMinimum(-10000.000000000000000);
        m_x_sb->setMaximum(10000.000000000000000);

        m_geometry_widget->addWidget(m_x_sb, 1, 1, 1, 1);

        label_5 = new QLabel(widget);
        label_5->setObjectName("label_5");
        label_5->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        m_geometry_widget->addWidget(label_5, 4, 0, 1, 1);

        m_horizontal_diameter_sb = new QDoubleSpinBox(widget);
        m_horizontal_diameter_sb->setObjectName("m_horizontal_diameter_sb");
        m_horizontal_diameter_sb->setMinimum(-10000.000000000000000);
        m_horizontal_diameter_sb->setMaximum(10000.000000000000000);

        m_geometry_widget->addWidget(m_horizontal_diameter_sb, 4, 1, 1, 1);

        label_4 = new QLabel(widget);
        label_4->setObjectName("label_4");

        m_geometry_widget->addWidget(label_4, 3, 0, 1, 4);


        m_main_layout->addWidget(widget);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        m_main_layout->addItem(verticalSpacer);


        retranslateUi(EllipseEditor);

        QMetaObject::connectSlotsByName(EllipseEditor);
    } // setupUi

    void retranslateUi(QWidget *EllipseEditor)
    {
        EllipseEditor->setWindowTitle(QCoreApplication::translate("EllipseEditor", "Form", nullptr));
        label->setText(QCoreApplication::translate("EllipseEditor", "Centre :", nullptr));
        label_3->setText(QCoreApplication::translate("EllipseEditor", "Y", nullptr));
        label_2->setText(QCoreApplication::translate("EllipseEditor", "X", nullptr));
        label_6->setText(QCoreApplication::translate("EllipseEditor", "Vertical :", nullptr));
        label_5->setText(QCoreApplication::translate("EllipseEditor", "Horizontal :", nullptr));
        label_4->setText(QCoreApplication::translate("EllipseEditor", "Diam\303\250tres :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EllipseEditor: public Ui_EllipseEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ELLIPSEEDITOR_H
