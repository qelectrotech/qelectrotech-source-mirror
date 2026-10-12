/********************************************************************************
** Form generated from reading UI file 'conductorpropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONDUCTORPROPERTIESWIDGET_H
#define UI_CONDUCTORPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "kcolorbutton.h"

QT_BEGIN_NAMESPACE

class Ui_ConductorPropertiesWidget
{
public:
    QGridLayout *gridLayout_9;
    QTabWidget *tabWidget;
    QWidget *type;
    QGridLayout *gridLayout_8;
    QTabWidget *tabWidget_2;
    QWidget *Multifilaire;
    QGridLayout *gridLayout_7;
    QGroupBox *m_multiwires_gb;
    QGridLayout *gridLayout_3;
    QLabel *label_14;
    QLabel *label_4;
    QComboBox *m_available_autonum_cb;
    QLineEdit *m_text_le;
    QLabel *label_9;
    QLabel *m_autonum_label;
    KColorButton *m_text_color_kpb;
    QSpinBox *m_text_size_sb;
    QLineEdit *m_tension_protocol_le;
    QLabel *label_5;
    QCheckBox *m_show_text_cb;
    QCheckBox *m_one_text_per_folio_cb;
    QLabel *label_8;
    QGridLayout *m_text_angle_gl;
    QComboBox *m_verti_cb;
    QLabel *label_3;
    QComboBox *m_horiz_cb;
    QVBoxLayout *m_autonum_layout;
    QLabel *label;
    QLineEdit *m_wire_color_le;
    QLabel *label_2;
    QPushButton *m_edit_autonum_pb;
    QLineEdit *m_function_le;
    QLineEdit *m_wire_section_le;
    QLabel *label_11;
    QLineEdit *m_formula_le;
    QLabel *label_15;
    QLineEdit *m_cable_le;
    QLabel *label_16;
    QLineEdit *m_bus_le;
    QSpacerItem *verticalSpacer_2;
    QWidget *Unifilaire;
    QGridLayout *gridLayout_5;
    QGroupBox *m_singlewire_gb;
    QHBoxLayout *horizontalLayout;
    QGridLayout *gridLayout;
    QLabel *m_preview_label;
    QSpinBox *m_phase_sb;
    QPushButton *m_update_preview_pb;
    QCheckBox *m_neutral_cb;
    QCheckBox *m_earth_cb;
    QCheckBox *m_phase_cb;
    QSlider *m_phase_slider;
    QCheckBox *m_pen_cb;
    QSpacerItem *verticalSpacer_3;
    QWidget *appearance;
    QGridLayout *gridLayout_4;
    KColorButton *m_color_kpb;
    QLabel *label_10;
    QSpacerItem *verticalSpacer;
    QLabel *label_7;
    QGroupBox *m_color_2_gb;
    QGridLayout *gridLayout_2;
    QLabel *label_13;
    QSpinBox *m_dash_size_sb;
    QLabel *label_12;
    KColorButton *m_color_2_kpb;
    QDoubleSpinBox *m_cond_size_sb;
    QComboBox *m_line_style_cb;
    QLabel *label_6;

    void setupUi(QWidget *ConductorPropertiesWidget)
    {
        if (ConductorPropertiesWidget->objectName().isEmpty())
            ConductorPropertiesWidget->setObjectName("ConductorPropertiesWidget");
        ConductorPropertiesWidget->resize(666, 418);
        gridLayout_9 = new QGridLayout(ConductorPropertiesWidget);
        gridLayout_9->setObjectName("gridLayout_9");
        tabWidget = new QTabWidget(ConductorPropertiesWidget);
        tabWidget->setObjectName("tabWidget");
        type = new QWidget();
        type->setObjectName("type");
        gridLayout_8 = new QGridLayout(type);
        gridLayout_8->setObjectName("gridLayout_8");
        tabWidget_2 = new QTabWidget(type);
        tabWidget_2->setObjectName("tabWidget_2");
        Multifilaire = new QWidget();
        Multifilaire->setObjectName("Multifilaire");
        gridLayout_7 = new QGridLayout(Multifilaire);
        gridLayout_7->setObjectName("gridLayout_7");
        m_multiwires_gb = new QGroupBox(Multifilaire);
        m_multiwires_gb->setObjectName("m_multiwires_gb");
        m_multiwires_gb->setMinimumSize(QSize(600, 0));
        m_multiwires_gb->setCheckable(true);
        m_multiwires_gb->setChecked(true);
        gridLayout_3 = new QGridLayout(m_multiwires_gb);
        gridLayout_3->setObjectName("gridLayout_3");
        label_14 = new QLabel(m_multiwires_gb);
        label_14->setObjectName("label_14");

        gridLayout_3->addWidget(label_14, 4, 0, 1, 1);

        label_4 = new QLabel(m_multiwires_gb);
        label_4->setObjectName("label_4");

        gridLayout_3->addWidget(label_4, 3, 3, 1, 1);

        m_available_autonum_cb = new QComboBox(m_multiwires_gb);
        m_available_autonum_cb->setObjectName("m_available_autonum_cb");

        gridLayout_3->addWidget(m_available_autonum_cb, 6, 1, 1, 1);

        m_text_le = new QLineEdit(m_multiwires_gb);
        m_text_le->setObjectName("m_text_le");
        m_text_le->setClearButtonEnabled(true);

        gridLayout_3->addWidget(m_text_le, 3, 1, 1, 1);

        label_9 = new QLabel(m_multiwires_gb);
        label_9->setObjectName("label_9");

        gridLayout_3->addWidget(label_9, 2, 3, 1, 1);

        m_autonum_label = new QLabel(m_multiwires_gb);
        m_autonum_label->setObjectName("m_autonum_label");

        gridLayout_3->addWidget(m_autonum_label, 6, 0, 1, 1);

        m_text_color_kpb = new KColorButton(m_multiwires_gb);
        m_text_color_kpb->setObjectName("m_text_color_kpb");

        gridLayout_3->addWidget(m_text_color_kpb, 4, 1, 1, 1);

        m_text_size_sb = new QSpinBox(m_multiwires_gb);
        m_text_size_sb->setObjectName("m_text_size_sb");
        m_text_size_sb->setWrapping(false);
        m_text_size_sb->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        m_text_size_sb->setMinimum(3);
        m_text_size_sb->setMaximum(99);

        gridLayout_3->addWidget(m_text_size_sb, 1, 1, 1, 1);

        m_tension_protocol_le = new QLineEdit(m_multiwires_gb);
        m_tension_protocol_le->setObjectName("m_tension_protocol_le");
        m_tension_protocol_le->setClearButtonEnabled(true);

        gridLayout_3->addWidget(m_tension_protocol_le, 2, 4, 1, 1);

        label_5 = new QLabel(m_multiwires_gb);
        label_5->setObjectName("label_5");

        gridLayout_3->addWidget(label_5, 4, 3, 1, 1);

        m_show_text_cb = new QCheckBox(m_multiwires_gb);
        m_show_text_cb->setObjectName("m_show_text_cb");
        m_show_text_cb->setChecked(true);

        gridLayout_3->addWidget(m_show_text_cb, 3, 2, 1, 1);

        m_one_text_per_folio_cb = new QCheckBox(m_multiwires_gb);
        m_one_text_per_folio_cb->setObjectName("m_one_text_per_folio_cb");

        gridLayout_3->addWidget(m_one_text_per_folio_cb, 0, 0, 1, 3);

        label_8 = new QLabel(m_multiwires_gb);
        label_8->setObjectName("label_8");

        gridLayout_3->addWidget(label_8, 1, 3, 1, 1);

        m_text_angle_gl = new QGridLayout();
        m_text_angle_gl->setObjectName("m_text_angle_gl");
        m_verti_cb = new QComboBox(m_multiwires_gb);
        m_verti_cb->addItem(QString());
        m_verti_cb->addItem(QString());
        m_verti_cb->setObjectName("m_verti_cb");

        m_text_angle_gl->addWidget(m_verti_cb, 1, 0, 1, 1);

        label_3 = new QLabel(m_multiwires_gb);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        m_text_angle_gl->addWidget(label_3, 0, 0, 1, 2);

        m_horiz_cb = new QComboBox(m_multiwires_gb);
        m_horiz_cb->addItem(QString());
        m_horiz_cb->addItem(QString());
        m_horiz_cb->setObjectName("m_horiz_cb");

        m_text_angle_gl->addWidget(m_horiz_cb, 1, 1, 1, 1);


        gridLayout_3->addLayout(m_text_angle_gl, 8, 0, 1, 5);

        m_autonum_layout = new QVBoxLayout();
        m_autonum_layout->setObjectName("m_autonum_layout");

        gridLayout_3->addLayout(m_autonum_layout, 7, 0, 1, 2);

        label = new QLabel(m_multiwires_gb);
        label->setObjectName("label");

        gridLayout_3->addWidget(label, 3, 0, 1, 1);

        m_wire_color_le = new QLineEdit(m_multiwires_gb);
        m_wire_color_le->setObjectName("m_wire_color_le");

        gridLayout_3->addWidget(m_wire_color_le, 3, 4, 1, 1);

        label_2 = new QLabel(m_multiwires_gb);
        label_2->setObjectName("label_2");

        gridLayout_3->addWidget(label_2, 1, 0, 1, 1);

        m_edit_autonum_pb = new QPushButton(m_multiwires_gb);
        m_edit_autonum_pb->setObjectName("m_edit_autonum_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/configure.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_edit_autonum_pb->setIcon(icon);

        gridLayout_3->addWidget(m_edit_autonum_pb, 6, 2, 1, 1);

        m_function_le = new QLineEdit(m_multiwires_gb);
        m_function_le->setObjectName("m_function_le");
        m_function_le->setClearButtonEnabled(true);

        gridLayout_3->addWidget(m_function_le, 1, 4, 1, 1);

        m_wire_section_le = new QLineEdit(m_multiwires_gb);
        m_wire_section_le->setObjectName("m_wire_section_le");

        gridLayout_3->addWidget(m_wire_section_le, 4, 4, 1, 1);

        label_11 = new QLabel(m_multiwires_gb);
        label_11->setObjectName("label_11");

        gridLayout_3->addWidget(label_11, 2, 0, 1, 1);

        m_formula_le = new QLineEdit(m_multiwires_gb);
        m_formula_le->setObjectName("m_formula_le");

        gridLayout_3->addWidget(m_formula_le, 2, 1, 1, 1);

        label_15 = new QLabel(m_multiwires_gb);
        label_15->setObjectName("label_15");

        gridLayout_3->addWidget(label_15, 6, 3, 1, 1);

        m_cable_le = new QLineEdit(m_multiwires_gb);
        m_cable_le->setObjectName("m_cable_le");

        gridLayout_3->addWidget(m_cable_le, 6, 4, 1, 1);

        label_16 = new QLabel(m_multiwires_gb);
        label_16->setObjectName("label_16");

        gridLayout_3->addWidget(label_16, 7, 3, 1, 1);

        m_bus_le = new QLineEdit(m_multiwires_gb);
        m_bus_le->setObjectName("m_bus_le");

        gridLayout_3->addWidget(m_bus_le, 7, 4, 1, 1);


        gridLayout_7->addWidget(m_multiwires_gb, 0, 0, 1, 1);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_7->addItem(verticalSpacer_2, 1, 0, 1, 1);

        tabWidget_2->addTab(Multifilaire, QString());
        Unifilaire = new QWidget();
        Unifilaire->setObjectName("Unifilaire");
        gridLayout_5 = new QGridLayout(Unifilaire);
        gridLayout_5->setObjectName("gridLayout_5");
        m_singlewire_gb = new QGroupBox(Unifilaire);
        m_singlewire_gb->setObjectName("m_singlewire_gb");
        m_singlewire_gb->setMaximumSize(QSize(450, 16777215));
        m_singlewire_gb->setFlat(false);
        m_singlewire_gb->setCheckable(true);
        m_singlewire_gb->setChecked(false);
        horizontalLayout = new QHBoxLayout(m_singlewire_gb);
        horizontalLayout->setObjectName("horizontalLayout");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        m_preview_label = new QLabel(m_singlewire_gb);
        m_preview_label->setObjectName("m_preview_label");
        m_preview_label->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(m_preview_label, 0, 0, 3, 1);

        m_phase_sb = new QSpinBox(m_singlewire_gb);
        m_phase_sb->setObjectName("m_phase_sb");
        m_phase_sb->setMinimum(1);
        m_phase_sb->setMaximum(3);

        gridLayout->addWidget(m_phase_sb, 2, 3, 1, 1);

        m_update_preview_pb = new QPushButton(m_singlewire_gb);
        m_update_preview_pb->setObjectName("m_update_preview_pb");

        gridLayout->addWidget(m_update_preview_pb, 0, 2, 1, 1);

        m_neutral_cb = new QCheckBox(m_singlewire_gb);
        m_neutral_cb->setObjectName("m_neutral_cb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/neutral.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_neutral_cb->setIcon(icon1);

        gridLayout->addWidget(m_neutral_cb, 1, 1, 1, 1);

        m_earth_cb = new QCheckBox(m_singlewire_gb);
        m_earth_cb->setObjectName("m_earth_cb");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/ground.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_earth_cb->setIcon(icon2);

        gridLayout->addWidget(m_earth_cb, 0, 1, 1, 1);

        m_phase_cb = new QCheckBox(m_singlewire_gb);
        m_phase_cb->setObjectName("m_phase_cb");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/phase.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_phase_cb->setIcon(icon3);

        gridLayout->addWidget(m_phase_cb, 2, 1, 1, 1);

        m_phase_slider = new QSlider(m_singlewire_gb);
        m_phase_slider->setObjectName("m_phase_slider");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_phase_slider->sizePolicy().hasHeightForWidth());
        m_phase_slider->setSizePolicy(sizePolicy);
        m_phase_slider->setMinimum(1);
        m_phase_slider->setMaximum(3);
        m_phase_slider->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(m_phase_slider, 2, 2, 1, 1);

        m_pen_cb = new QCheckBox(m_singlewire_gb);
        m_pen_cb->setObjectName("m_pen_cb");

        gridLayout->addWidget(m_pen_cb, 1, 2, 1, 1);


        horizontalLayout->addLayout(gridLayout);


        gridLayout_5->addWidget(m_singlewire_gb, 0, 0, 1, 1);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_5->addItem(verticalSpacer_3, 1, 0, 1, 1);

        tabWidget_2->addTab(Unifilaire, QString());

        gridLayout_8->addWidget(tabWidget_2, 0, 0, 1, 1);

        tabWidget->addTab(type, QString());
        appearance = new QWidget();
        appearance->setObjectName("appearance");
        gridLayout_4 = new QGridLayout(appearance);
        gridLayout_4->setObjectName("gridLayout_4");
        m_color_kpb = new KColorButton(appearance);
        m_color_kpb->setObjectName("m_color_kpb");

        gridLayout_4->addWidget(m_color_kpb, 0, 1, 1, 1);

        label_10 = new QLabel(appearance);
        label_10->setObjectName("label_10");

        gridLayout_4->addWidget(label_10, 2, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_4->addItem(verticalSpacer, 4, 1, 1, 1);

        label_7 = new QLabel(appearance);
        label_7->setObjectName("label_7");

        gridLayout_4->addWidget(label_7, 1, 0, 1, 1);

        m_color_2_gb = new QGroupBox(appearance);
        m_color_2_gb->setObjectName("m_color_2_gb");
        m_color_2_gb->setFlat(true);
        m_color_2_gb->setCheckable(true);
        m_color_2_gb->setChecked(false);
        gridLayout_2 = new QGridLayout(m_color_2_gb);
        gridLayout_2->setObjectName("gridLayout_2");
        label_13 = new QLabel(m_color_2_gb);
        label_13->setObjectName("label_13");

        gridLayout_2->addWidget(label_13, 1, 0, 1, 1);

        m_dash_size_sb = new QSpinBox(m_color_2_gb);
        m_dash_size_sb->setObjectName("m_dash_size_sb");
        m_dash_size_sb->setMinimum(2);

        gridLayout_2->addWidget(m_dash_size_sb, 1, 1, 1, 1);

        label_12 = new QLabel(m_color_2_gb);
        label_12->setObjectName("label_12");

        gridLayout_2->addWidget(label_12, 0, 0, 1, 1);

        m_color_2_kpb = new KColorButton(m_color_2_gb);
        m_color_2_kpb->setObjectName("m_color_2_kpb");

        gridLayout_2->addWidget(m_color_2_kpb, 0, 1, 1, 1);


        gridLayout_4->addWidget(m_color_2_gb, 3, 0, 1, 2);

        m_cond_size_sb = new QDoubleSpinBox(appearance);
        m_cond_size_sb->setObjectName("m_cond_size_sb");
        m_cond_size_sb->setMinimum(0.400000000000000);
        m_cond_size_sb->setMaximum(20.000000000000000);
        m_cond_size_sb->setSingleStep(0.200000000000000);
        m_cond_size_sb->setValue(1.000000000000000);

        gridLayout_4->addWidget(m_cond_size_sb, 2, 1, 1, 1);

        m_line_style_cb = new QComboBox(appearance);
        m_line_style_cb->setObjectName("m_line_style_cb");

        gridLayout_4->addWidget(m_line_style_cb, 1, 1, 1, 1);

        label_6 = new QLabel(appearance);
        label_6->setObjectName("label_6");

        gridLayout_4->addWidget(label_6, 0, 0, 1, 1);

        tabWidget->addTab(appearance, QString());

        gridLayout_9->addWidget(tabWidget, 0, 0, 1, 1);

        QWidget::setTabOrder(tabWidget, tabWidget_2);
        QWidget::setTabOrder(tabWidget_2, m_multiwires_gb);
        QWidget::setTabOrder(m_multiwires_gb, m_one_text_per_folio_cb);
        QWidget::setTabOrder(m_one_text_per_folio_cb, m_text_size_sb);
        QWidget::setTabOrder(m_text_size_sb, m_formula_le);
        QWidget::setTabOrder(m_formula_le, m_text_le);
        QWidget::setTabOrder(m_text_le, m_show_text_cb);
        QWidget::setTabOrder(m_show_text_cb, m_text_color_kpb);
        QWidget::setTabOrder(m_text_color_kpb, m_available_autonum_cb);
        QWidget::setTabOrder(m_available_autonum_cb, m_edit_autonum_pb);
        QWidget::setTabOrder(m_edit_autonum_pb, m_function_le);
        QWidget::setTabOrder(m_function_le, m_tension_protocol_le);
        QWidget::setTabOrder(m_tension_protocol_le, m_wire_color_le);
        QWidget::setTabOrder(m_wire_color_le, m_wire_section_le);
        QWidget::setTabOrder(m_wire_section_le, m_cable_le);
        QWidget::setTabOrder(m_cable_le, m_bus_le);
        QWidget::setTabOrder(m_bus_le, m_verti_cb);
        QWidget::setTabOrder(m_verti_cb, m_horiz_cb);
        QWidget::setTabOrder(m_horiz_cb, m_singlewire_gb);
        QWidget::setTabOrder(m_singlewire_gb, m_phase_sb);
        QWidget::setTabOrder(m_phase_sb, m_update_preview_pb);
        QWidget::setTabOrder(m_update_preview_pb, m_neutral_cb);
        QWidget::setTabOrder(m_neutral_cb, m_earth_cb);
        QWidget::setTabOrder(m_earth_cb, m_phase_cb);
        QWidget::setTabOrder(m_phase_cb, m_phase_slider);
        QWidget::setTabOrder(m_phase_slider, m_pen_cb);
        QWidget::setTabOrder(m_pen_cb, m_color_kpb);
        QWidget::setTabOrder(m_color_kpb, m_color_2_gb);
        QWidget::setTabOrder(m_color_2_gb, m_dash_size_sb);
        QWidget::setTabOrder(m_dash_size_sb, m_color_2_kpb);
        QWidget::setTabOrder(m_color_2_kpb, m_cond_size_sb);
        QWidget::setTabOrder(m_cond_size_sb, m_line_style_cb);

        retranslateUi(ConductorPropertiesWidget);
        QObject::connect(m_phase_slider, &QSlider::valueChanged, m_phase_sb, &QSpinBox::setValue);
        QObject::connect(m_phase_sb, &QSpinBox::valueChanged, m_phase_slider, &QSlider::setValue);
        QObject::connect(m_phase_cb, &QCheckBox::toggled, m_phase_slider, &QSlider::setEnabled);
        QObject::connect(m_phase_cb, &QCheckBox::toggled, m_phase_sb, &QSpinBox::setEnabled);
        QObject::connect(m_earth_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_neutral_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_phase_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_pen_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_phase_sb, &QSpinBox::valueChanged, m_update_preview_pb, qOverload<>(&QPushButton::click));

        tabWidget->setCurrentIndex(0);
        tabWidget_2->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ConductorPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *ConductorPropertiesWidget)
    {
        ConductorPropertiesWidget->setWindowTitle(QCoreApplication::translate("ConductorPropertiesWidget", "Form", nullptr));
        m_multiwires_gb->setTitle(QCoreApplication::translate("ConductorPropertiesWidget", "&Multifilaire", nullptr));
        label_14->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Couleur du texte:", nullptr));
        label_4->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Couleur du conducteur", nullptr));
#if QT_CONFIG(tooltip)
        m_text_le->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Texte", nullptr));
#endif // QT_CONFIG(tooltip)
        label_9->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Tension / Protocole :", nullptr));
        m_autonum_label->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Autonum\303\251rotation", nullptr));
#if QT_CONFIG(tooltip)
        m_text_size_sb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Taille du texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_text_size_sb->setPrefix(QString());
        label_5->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Section du conducteur", nullptr));
#if QT_CONFIG(tooltip)
        m_show_text_cb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Texte visible", nullptr));
#endif // QT_CONFIG(tooltip)
        m_show_text_cb->setText(QString());
#if QT_CONFIG(tooltip)
        m_one_text_per_folio_cb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "activer l'option un texte par potentiel", nullptr));
#endif // QT_CONFIG(tooltip)
        m_one_text_per_folio_cb->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Afficher un texte de potentiel par folio. ", nullptr));
        label_8->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Fonction :", nullptr));
        m_verti_cb->setItemText(0, QCoreApplication::translate("ConductorPropertiesWidget", "Vertical \303\240 gauche", nullptr));
        m_verti_cb->setItemText(1, QCoreApplication::translate("ConductorPropertiesWidget", "Vertical \303\240 droite", nullptr));

        label_3->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Position et rotation du texte de conducteur :", nullptr));
        m_horiz_cb->setItemText(0, QCoreApplication::translate("ConductorPropertiesWidget", "Horizontal en haut", nullptr));
        m_horiz_cb->setItemText(1, QCoreApplication::translate("ConductorPropertiesWidget", "Horizontal en bas", nullptr));

        label->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Texte :", nullptr));
        label_2->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Taille du texte :", nullptr));
#if QT_CONFIG(tooltip)
        m_edit_autonum_pb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "\303\251diter les num\303\251rotations", nullptr));
#endif // QT_CONFIG(tooltip)
        m_edit_autonum_pb->setText(QString());
        label_11->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Formule du texte :", nullptr));
        label_15->setText(QCoreApplication::translate("ConductorPropertiesWidget", "cable", nullptr));
        label_16->setText(QCoreApplication::translate("ConductorPropertiesWidget", "bus", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(Multifilaire), QCoreApplication::translate("ConductorPropertiesWidget", "&Multifilaire", nullptr));
        m_singlewire_gb->setTitle(QCoreApplication::translate("ConductorPropertiesWidget", "Unifilaire", nullptr));
        m_preview_label->setText(QCoreApplication::translate("ConductorPropertiesWidget", "TextLabel", nullptr));
#if QT_CONFIG(tooltip)
        m_phase_sb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Nombre de phase", nullptr));
#endif // QT_CONFIG(tooltip)
        m_update_preview_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_neutral_cb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Neutre", nullptr));
#endif // QT_CONFIG(tooltip)
        m_neutral_cb->setText(QCoreApplication::translate("ConductorPropertiesWidget", "neutre", nullptr));
#if QT_CONFIG(tooltip)
        m_earth_cb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Terre", nullptr));
#endif // QT_CONFIG(tooltip)
        m_earth_cb->setText(QCoreApplication::translate("ConductorPropertiesWidget", "terre", nullptr));
#if QT_CONFIG(tooltip)
        m_phase_cb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Phase", nullptr));
#endif // QT_CONFIG(tooltip)
        m_phase_cb->setText(QCoreApplication::translate("ConductorPropertiesWidget", "phase", nullptr));
#if QT_CONFIG(tooltip)
        m_phase_slider->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Nombre de phase", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_pen_cb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Protective Earth Neutral", nullptr));
#endif // QT_CONFIG(tooltip)
        m_pen_cb->setText(QCoreApplication::translate("ConductorPropertiesWidget", "PEN", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(Unifilaire), QCoreApplication::translate("ConductorPropertiesWidget", "Unifilaire", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(type), QCoreApplication::translate("ConductorPropertiesWidget", "Type", nullptr));
        label_10->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Taille :", nullptr));
#if QT_CONFIG(tooltip)
        label_7->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Style du conducteur", nullptr));
#endif // QT_CONFIG(tooltip)
        label_7->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Style :", nullptr));
        m_color_2_gb->setTitle(QCoreApplication::translate("ConductorPropertiesWidget", "Couleur secondaire :", nullptr));
        label_13->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Taille de trait :", nullptr));
        m_dash_size_sb->setSuffix(QCoreApplication::translate("ConductorPropertiesWidget", "px", nullptr));
        label_12->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Couleur :", nullptr));
#if QT_CONFIG(tooltip)
        m_line_style_cb->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Style du conducteur", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        label_6->setToolTip(QCoreApplication::translate("ConductorPropertiesWidget", "Couleur du conducteur", nullptr));
#endif // QT_CONFIG(tooltip)
        label_6->setText(QCoreApplication::translate("ConductorPropertiesWidget", "Couleur :", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(appearance), QCoreApplication::translate("ConductorPropertiesWidget", "Apparence", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ConductorPropertiesWidget: public Ui_ConductorPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONDUCTORPROPERTIESWIDGET_H
