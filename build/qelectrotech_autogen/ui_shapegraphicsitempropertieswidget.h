/********************************************************************************
** Form generated from reading UI file 'shapegraphicsitempropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SHAPEGRAPHICSITEMPROPERTIESWIDGET_H
#define UI_SHAPEGRAPHICSITEMPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "kcolorbutton.h"

QT_BEGIN_NAMESPACE

class Ui_ShapeGraphicsItemPropertiesWidget
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *m_geometry_gb;
    QGridLayout *gridLayout_3;
    QLabel *m_geom_dim1_label;
    QDoubleSpinBox *m_geom_dim1_dsb;
    QLabel *m_geom_dim2_label;
    QDoubleSpinBox *m_geom_dim2_dsb;
    QLabel *m_geom_angle_label;
    QDoubleSpinBox *m_geom_angle_dsb;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    KColorButton *m_color_kpb;
    QDoubleSpinBox *m_size_dsb;
    QComboBox *m_style_cb;
    QGroupBox *m_filling_gb;
    QGridLayout *gridLayout;
    QComboBox *m_brush_style_cb;
    QLabel *label_4;
    QLabel *label_5;
    KColorButton *m_brush_color_kpb;
    QCheckBox *m_lock_pos_cb;
    QCheckBox *m_close_polygon;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *ShapeGraphicsItemPropertiesWidget)
    {
        if (ShapeGraphicsItemPropertiesWidget->objectName().isEmpty())
            ShapeGraphicsItemPropertiesWidget->setObjectName("ShapeGraphicsItemPropertiesWidget");
        ShapeGraphicsItemPropertiesWidget->resize(245, 311);
        verticalLayout = new QVBoxLayout(ShapeGraphicsItemPropertiesWidget);
        verticalLayout->setObjectName("verticalLayout");
        m_geometry_gb = new QGroupBox(ShapeGraphicsItemPropertiesWidget);
        m_geometry_gb->setObjectName("m_geometry_gb");
        gridLayout_3 = new QGridLayout(m_geometry_gb);
        gridLayout_3->setObjectName("gridLayout_3");
        m_geom_dim1_label = new QLabel(m_geometry_gb);
        m_geom_dim1_label->setObjectName("m_geom_dim1_label");

        gridLayout_3->addWidget(m_geom_dim1_label, 0, 0, 1, 1);

        m_geom_dim1_dsb = new QDoubleSpinBox(m_geometry_gb);
        m_geom_dim1_dsb->setObjectName("m_geom_dim1_dsb");
        m_geom_dim1_dsb->setDecimals(2);
        m_geom_dim1_dsb->setMinimum(0.010000000000000);
        m_geom_dim1_dsb->setMaximum(100000.000000000000000);

        gridLayout_3->addWidget(m_geom_dim1_dsb, 0, 1, 1, 1);

        m_geom_dim2_label = new QLabel(m_geometry_gb);
        m_geom_dim2_label->setObjectName("m_geom_dim2_label");

        gridLayout_3->addWidget(m_geom_dim2_label, 1, 0, 1, 1);

        m_geom_dim2_dsb = new QDoubleSpinBox(m_geometry_gb);
        m_geom_dim2_dsb->setObjectName("m_geom_dim2_dsb");
        m_geom_dim2_dsb->setDecimals(2);
        m_geom_dim2_dsb->setMinimum(0.010000000000000);
        m_geom_dim2_dsb->setMaximum(100000.000000000000000);

        gridLayout_3->addWidget(m_geom_dim2_dsb, 1, 1, 1, 1);

        m_geom_angle_label = new QLabel(m_geometry_gb);
        m_geom_angle_label->setObjectName("m_geom_angle_label");

        gridLayout_3->addWidget(m_geom_angle_label, 2, 0, 1, 1);

        m_geom_angle_dsb = new QDoubleSpinBox(m_geometry_gb);
        m_geom_angle_dsb->setObjectName("m_geom_angle_dsb");
        m_geom_angle_dsb->setDecimals(1);
        m_geom_angle_dsb->setMinimum(-360.000000000000000);
        m_geom_angle_dsb->setMaximum(360.000000000000000);

        gridLayout_3->addWidget(m_geom_angle_dsb, 2, 1, 1, 1);


        verticalLayout->addWidget(m_geometry_gb);

        groupBox = new QGroupBox(ShapeGraphicsItemPropertiesWidget);
        groupBox->setObjectName("groupBox");
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        label = new QLabel(groupBox);
        label->setObjectName("label");

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");

        gridLayout_2->addWidget(label_2, 1, 0, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName("label_3");

        gridLayout_2->addWidget(label_3, 2, 0, 1, 1);

        m_color_kpb = new KColorButton(groupBox);
        m_color_kpb->setObjectName("m_color_kpb");

        gridLayout_2->addWidget(m_color_kpb, 2, 1, 1, 1);

        m_size_dsb = new QDoubleSpinBox(groupBox);
        m_size_dsb->setObjectName("m_size_dsb");
        m_size_dsb->setMaximumSize(QSize(16777212, 16777215));
        m_size_dsb->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        m_size_dsb->setAccelerated(true);
        m_size_dsb->setCorrectionMode(QAbstractSpinBox::CorrectToNearestValue);
        m_size_dsb->setDecimals(1);
        m_size_dsb->setMinimum(0.200000000000000);
        m_size_dsb->setMaximum(50.000000000000000);
        m_size_dsb->setSingleStep(0.200000000000000);
        m_size_dsb->setValue(1.000000000000000);

        gridLayout_2->addWidget(m_size_dsb, 1, 1, 1, 1);

        m_style_cb = new QComboBox(groupBox);
        m_style_cb->addItem(QString());
        m_style_cb->addItem(QString());
        m_style_cb->addItem(QString());
        m_style_cb->addItem(QString());
        m_style_cb->addItem(QString());
        m_style_cb->addItem(QString());
        m_style_cb->setObjectName("m_style_cb");

        gridLayout_2->addWidget(m_style_cb, 0, 1, 1, 1);


        verticalLayout->addWidget(groupBox);

        m_filling_gb = new QGroupBox(ShapeGraphicsItemPropertiesWidget);
        m_filling_gb->setObjectName("m_filling_gb");
        gridLayout = new QGridLayout(m_filling_gb);
        gridLayout->setObjectName("gridLayout");
        m_brush_style_cb = new QComboBox(m_filling_gb);
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->addItem(QString());
        m_brush_style_cb->setObjectName("m_brush_style_cb");

        gridLayout->addWidget(m_brush_style_cb, 0, 1, 1, 1);

        label_4 = new QLabel(m_filling_gb);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 0, 0, 1, 1);

        label_5 = new QLabel(m_filling_gb);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 1, 0, 1, 1);

        m_brush_color_kpb = new KColorButton(m_filling_gb);
        m_brush_color_kpb->setObjectName("m_brush_color_kpb");

        gridLayout->addWidget(m_brush_color_kpb, 1, 1, 1, 1);


        verticalLayout->addWidget(m_filling_gb);

        m_lock_pos_cb = new QCheckBox(ShapeGraphicsItemPropertiesWidget);
        m_lock_pos_cb->setObjectName("m_lock_pos_cb");

        verticalLayout->addWidget(m_lock_pos_cb);

        m_close_polygon = new QCheckBox(ShapeGraphicsItemPropertiesWidget);
        m_close_polygon->setObjectName("m_close_polygon");

        verticalLayout->addWidget(m_close_polygon);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        QWidget::setTabOrder(m_geom_dim1_dsb, m_geom_dim2_dsb);
        QWidget::setTabOrder(m_geom_dim2_dsb, m_geom_angle_dsb);
        QWidget::setTabOrder(m_geom_angle_dsb, m_style_cb);
        QWidget::setTabOrder(m_style_cb, m_size_dsb);
        QWidget::setTabOrder(m_size_dsb, m_color_kpb);
        QWidget::setTabOrder(m_color_kpb, m_brush_style_cb);
        QWidget::setTabOrder(m_brush_style_cb, m_brush_color_kpb);
        QWidget::setTabOrder(m_brush_color_kpb, m_lock_pos_cb);
        QWidget::setTabOrder(m_lock_pos_cb, m_close_polygon);

        retranslateUi(ShapeGraphicsItemPropertiesWidget);

        QMetaObject::connectSlotsByName(ShapeGraphicsItemPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *ShapeGraphicsItemPropertiesWidget)
    {
        ShapeGraphicsItemPropertiesWidget->setWindowTitle(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Form", nullptr));
        m_geometry_gb->setTitle(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "G\303\251om\303\251trie", nullptr));
        m_geom_dim1_label->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Largeur", nullptr));
        m_geom_dim2_label->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Hauteur", nullptr));
        m_geom_angle_label->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Angle", nullptr));
        m_geom_angle_dsb->setSuffix(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", " \302\260", nullptr));
        groupBox->setTitle(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Trait", nullptr));
        label->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Type", nullptr));
        label_2->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "\303\211paisseur", nullptr));
        label_3->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Couleur", nullptr));
        m_style_cb->setItemText(0, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Normal", nullptr));
        m_style_cb->setItemText(1, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Tiret", nullptr));
        m_style_cb->setItemText(2, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Pointill\303\251", nullptr));
        m_style_cb->setItemText(3, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Traits et points", nullptr));
        m_style_cb->setItemText(4, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Traits points points", nullptr));
        m_style_cb->setItemText(5, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Tiret custom", nullptr));

        m_filling_gb->setTitle(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Remplissage", nullptr));
        m_brush_style_cb->setItemText(0, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Aucun", nullptr));
        m_brush_style_cb->setItemText(1, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Plein", nullptr));
        m_brush_style_cb->setItemText(2, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Densit\303\251 1", nullptr));
        m_brush_style_cb->setItemText(3, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Densit\303\251 2", nullptr));
        m_brush_style_cb->setItemText(4, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Densit\303\251 3", nullptr));
        m_brush_style_cb->setItemText(5, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Densit\303\251 4", nullptr));
        m_brush_style_cb->setItemText(6, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Densit\303\251 5", nullptr));
        m_brush_style_cb->setItemText(7, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Densit\303\251 6", nullptr));
        m_brush_style_cb->setItemText(8, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Densit\303\251 7", nullptr));
        m_brush_style_cb->setItemText(9, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Horizontal", nullptr));
        m_brush_style_cb->setItemText(10, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Vertical", nullptr));
        m_brush_style_cb->setItemText(11, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Croix", nullptr));
        m_brush_style_cb->setItemText(12, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Diagonal arri\303\250re", nullptr));
        m_brush_style_cb->setItemText(13, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Diagonal avant", nullptr));
        m_brush_style_cb->setItemText(14, QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Diagonal en croix", nullptr));

        label_4->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Style", nullptr));
        label_5->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Couleur", nullptr));
        m_lock_pos_cb->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Verrouiller la position", nullptr));
        m_close_polygon->setText(QCoreApplication::translate("ShapeGraphicsItemPropertiesWidget", "Forme ferm\303\251e", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ShapeGraphicsItemPropertiesWidget: public Ui_ShapeGraphicsItemPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SHAPEGRAPHICSITEMPROPERTIESWIDGET_H
