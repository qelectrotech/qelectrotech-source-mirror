/********************************************************************************
** Form generated from reading UI file 'imagepropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMAGEPROPERTIESWIDGET_H
#define UI_IMAGEPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ImagePropertiesWidget
{
public:
    QGridLayout *gridLayout;
    QLabel *label_width;
    QDoubleSpinBox *m_width_sb;
    QToolButton *m_lock_ratio_tb;
    QLabel *label_height;
    QDoubleSpinBox *m_height_sb;
    QPushButton *m_restore_ratio_pb;
    QLabel *label_angle;
    QDoubleSpinBox *m_angle_sb;
    QLabel *label_skew_x;
    QDoubleSpinBox *m_skew_x_sb;
    QLabel *label_skew_y;
    QDoubleSpinBox *m_skew_y_sb;
    QCheckBox *m_lock_pos_cb;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *ImagePropertiesWidget)
    {
        if (ImagePropertiesWidget->objectName().isEmpty())
            ImagePropertiesWidget->setObjectName("ImagePropertiesWidget");
        ImagePropertiesWidget->resize(280, 230);
        gridLayout = new QGridLayout(ImagePropertiesWidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setSizeConstraint(QLayout::SetMinimumSize);
        label_width = new QLabel(ImagePropertiesWidget);
        label_width->setObjectName("label_width");

        gridLayout->addWidget(label_width, 0, 0, 1, 1);

        m_width_sb = new QDoubleSpinBox(ImagePropertiesWidget);
        m_width_sb->setObjectName("m_width_sb");
        m_width_sb->setDecimals(1);
        m_width_sb->setMinimum(1.000000000000000);
        m_width_sb->setMaximum(1000.000000000000000);

        gridLayout->addWidget(m_width_sb, 0, 1, 1, 1);

        m_lock_ratio_tb = new QToolButton(ImagePropertiesWidget);
        m_lock_ratio_tb->setObjectName("m_lock_ratio_tb");
        m_lock_ratio_tb->setCheckable(true);
        m_lock_ratio_tb->setChecked(true);

        gridLayout->addWidget(m_lock_ratio_tb, 0, 2, 2, 1);

        label_height = new QLabel(ImagePropertiesWidget);
        label_height->setObjectName("label_height");

        gridLayout->addWidget(label_height, 1, 0, 1, 1);

        m_height_sb = new QDoubleSpinBox(ImagePropertiesWidget);
        m_height_sb->setObjectName("m_height_sb");
        m_height_sb->setDecimals(1);
        m_height_sb->setMinimum(1.000000000000000);
        m_height_sb->setMaximum(1000.000000000000000);

        gridLayout->addWidget(m_height_sb, 1, 1, 1, 1);

        m_restore_ratio_pb = new QPushButton(ImagePropertiesWidget);
        m_restore_ratio_pb->setObjectName("m_restore_ratio_pb");

        gridLayout->addWidget(m_restore_ratio_pb, 2, 0, 1, 3);

        label_angle = new QLabel(ImagePropertiesWidget);
        label_angle->setObjectName("label_angle");

        gridLayout->addWidget(label_angle, 3, 0, 1, 1);

        m_angle_sb = new QDoubleSpinBox(ImagePropertiesWidget);
        m_angle_sb->setObjectName("m_angle_sb");
        m_angle_sb->setDecimals(1);
        m_angle_sb->setMinimum(-360.000000000000000);
        m_angle_sb->setMaximum(360.000000000000000);

        gridLayout->addWidget(m_angle_sb, 3, 1, 1, 1);

        label_skew_x = new QLabel(ImagePropertiesWidget);
        label_skew_x->setObjectName("label_skew_x");

        gridLayout->addWidget(label_skew_x, 4, 0, 1, 1);

        m_skew_x_sb = new QDoubleSpinBox(ImagePropertiesWidget);
        m_skew_x_sb->setObjectName("m_skew_x_sb");
        m_skew_x_sb->setDecimals(1);
        m_skew_x_sb->setMinimum(-85.000000000000000);
        m_skew_x_sb->setMaximum(85.000000000000000);

        gridLayout->addWidget(m_skew_x_sb, 4, 1, 1, 1);

        label_skew_y = new QLabel(ImagePropertiesWidget);
        label_skew_y->setObjectName("label_skew_y");

        gridLayout->addWidget(label_skew_y, 5, 0, 1, 1);

        m_skew_y_sb = new QDoubleSpinBox(ImagePropertiesWidget);
        m_skew_y_sb->setObjectName("m_skew_y_sb");
        m_skew_y_sb->setDecimals(1);
        m_skew_y_sb->setMinimum(-85.000000000000000);
        m_skew_y_sb->setMaximum(85.000000000000000);

        gridLayout->addWidget(m_skew_y_sb, 5, 1, 1, 1);

        m_lock_pos_cb = new QCheckBox(ImagePropertiesWidget);
        m_lock_pos_cb->setObjectName("m_lock_pos_cb");

        gridLayout->addWidget(m_lock_pos_cb, 6, 0, 1, 2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 7, 0, 1, 1);


        retranslateUi(ImagePropertiesWidget);

        QMetaObject::connectSlotsByName(ImagePropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *ImagePropertiesWidget)
    {
        ImagePropertiesWidget->setWindowTitle(QCoreApplication::translate("ImagePropertiesWidget", "Form", nullptr));
        label_width->setText(QCoreApplication::translate("ImagePropertiesWidget", "Largeur", nullptr));
        m_width_sb->setSuffix(QCoreApplication::translate("ImagePropertiesWidget", " %", nullptr));
#if QT_CONFIG(tooltip)
        m_lock_ratio_tb->setToolTip(QCoreApplication::translate("ImagePropertiesWidget", "Verrouill\303\251 : modifier la largeur ou la hauteur ajuste l'autre pour conserver les proportions. Cliquer pour d\303\251verrouiller.", nullptr));
#endif // QT_CONFIG(tooltip)
        label_height->setText(QCoreApplication::translate("ImagePropertiesWidget", "Hauteur", nullptr));
        m_height_sb->setSuffix(QCoreApplication::translate("ImagePropertiesWidget", " %", nullptr));
        m_restore_ratio_pb->setText(QCoreApplication::translate("ImagePropertiesWidget", "Restaurer les proportions", nullptr));
        label_angle->setText(QCoreApplication::translate("ImagePropertiesWidget", "Angle", nullptr));
        m_angle_sb->setSuffix(QCoreApplication::translate("ImagePropertiesWidget", " \302\260", nullptr));
        label_skew_x->setText(QCoreApplication::translate("ImagePropertiesWidget", "Inclinaison X", nullptr));
        m_skew_x_sb->setSuffix(QCoreApplication::translate("ImagePropertiesWidget", " \302\260", nullptr));
        label_skew_y->setText(QCoreApplication::translate("ImagePropertiesWidget", "Inclinaison Y", nullptr));
        m_skew_y_sb->setSuffix(QCoreApplication::translate("ImagePropertiesWidget", " \302\260", nullptr));
        m_lock_pos_cb->setText(QCoreApplication::translate("ImagePropertiesWidget", "Verrouiller la position", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImagePropertiesWidget: public Ui_ImagePropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMAGEPROPERTIESWIDGET_H
