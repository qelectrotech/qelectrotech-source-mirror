/********************************************************************************
** Form generated from reading UI file 'rectangleeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RECTANGLEEDITOR_H
#define UI_RECTANGLEEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_RectangleEditor
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *m_grid_layout;
    QDoubleSpinBox *m_width_sb;
    QLabel *label_6;
    QLabel *label_4;
    QLabel *label_3;
    QDoubleSpinBox *m_height_sb;
    QDoubleSpinBox *m_y_sb;
    QDoubleSpinBox *m_rx_sb;
    QLabel *label;
    QLabel *label_5;
    QLabel *label_7;
    QLabel *label_2;
    QDoubleSpinBox *m_x_sb;
    QLabel *label_8;
    QDoubleSpinBox *m_ry_sb;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *RectangleEditor)
    {
        if (RectangleEditor->objectName().isEmpty())
            RectangleEditor->setObjectName("RectangleEditor");
        RectangleEditor->resize(293, 147);
        verticalLayout = new QVBoxLayout(RectangleEditor);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_grid_layout = new QGridLayout();
        m_grid_layout->setObjectName("m_grid_layout");
        m_width_sb = new QDoubleSpinBox(RectangleEditor);
        m_width_sb->setObjectName("m_width_sb");
        m_width_sb->setMinimum(-5000.000000000000000);
        m_width_sb->setMaximum(5000.000000000000000);

        m_grid_layout->addWidget(m_width_sb, 3, 1, 1, 1);

        label_6 = new QLabel(RectangleEditor);
        label_6->setObjectName("label_6");
        label_6->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_grid_layout->addWidget(label_6, 4, 0, 1, 1);

        label_4 = new QLabel(RectangleEditor);
        label_4->setObjectName("label_4");

        m_grid_layout->addWidget(label_4, 2, 0, 1, 4);

        label_3 = new QLabel(RectangleEditor);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_grid_layout->addWidget(label_3, 1, 2, 1, 1);

        m_height_sb = new QDoubleSpinBox(RectangleEditor);
        m_height_sb->setObjectName("m_height_sb");
        m_height_sb->setMinimum(-5000.000000000000000);
        m_height_sb->setMaximum(5000.000000000000000);

        m_grid_layout->addWidget(m_height_sb, 4, 1, 1, 1);

        m_y_sb = new QDoubleSpinBox(RectangleEditor);
        m_y_sb->setObjectName("m_y_sb");
        m_y_sb->setMinimum(-5000.000000000000000);
        m_y_sb->setMaximum(5000.000000000000000);

        m_grid_layout->addWidget(m_y_sb, 1, 3, 1, 1);

        m_rx_sb = new QDoubleSpinBox(RectangleEditor);
        m_rx_sb->setObjectName("m_rx_sb");
        m_rx_sb->setMaximum(2500.000000000000000);

        m_grid_layout->addWidget(m_rx_sb, 3, 3, 1, 1);

        label = new QLabel(RectangleEditor);
        label->setObjectName("label");

        m_grid_layout->addWidget(label, 0, 0, 1, 4);

        label_5 = new QLabel(RectangleEditor);
        label_5->setObjectName("label_5");
        label_5->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_grid_layout->addWidget(label_5, 3, 0, 1, 1);

        label_7 = new QLabel(RectangleEditor);
        label_7->setObjectName("label_7");
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_grid_layout->addWidget(label_7, 3, 2, 1, 1);

        label_2 = new QLabel(RectangleEditor);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_grid_layout->addWidget(label_2, 1, 0, 1, 1);

        m_x_sb = new QDoubleSpinBox(RectangleEditor);
        m_x_sb->setObjectName("m_x_sb");
        m_x_sb->setMinimum(-5000.000000000000000);
        m_x_sb->setMaximum(5000.000000000000000);

        m_grid_layout->addWidget(m_x_sb, 1, 1, 1, 1);

        label_8 = new QLabel(RectangleEditor);
        label_8->setObjectName("label_8");
        label_8->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        m_grid_layout->addWidget(label_8, 4, 2, 1, 1);

        m_ry_sb = new QDoubleSpinBox(RectangleEditor);
        m_ry_sb->setObjectName("m_ry_sb");
        m_ry_sb->setMaximum(2500.000000000000000);

        m_grid_layout->addWidget(m_ry_sb, 4, 3, 1, 1);


        verticalLayout->addLayout(m_grid_layout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(RectangleEditor);

        QMetaObject::connectSlotsByName(RectangleEditor);
    } // setupUi

    void retranslateUi(QWidget *RectangleEditor)
    {
        label_6->setText(QCoreApplication::translate("RectangleEditor", "Hauteur :", nullptr));
        label_4->setText(QCoreApplication::translate("RectangleEditor", "Dimensions :", nullptr));
        label_3->setText(QCoreApplication::translate("RectangleEditor", "y", nullptr));
        label->setText(QCoreApplication::translate("RectangleEditor", "Coin sup\303\251rieur gauche :", nullptr));
        label_5->setText(QCoreApplication::translate("RectangleEditor", "Largeur :", nullptr));
        label_7->setText(QCoreApplication::translate("RectangleEditor", "Arrondi :", nullptr));
        label_2->setText(QCoreApplication::translate("RectangleEditor", "x", nullptr));
        label_8->setText(QCoreApplication::translate("RectangleEditor", "Arrondi :", nullptr));
        (void)RectangleEditor;
    } // retranslateUi

};

namespace Ui {
    class RectangleEditor: public Ui_RectangleEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RECTANGLEEDITOR_H
