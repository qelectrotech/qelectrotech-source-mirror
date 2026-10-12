/********************************************************************************
** Form generated from reading UI file 'replaceconductordialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPLACECONDUCTORDIALOG_H
#define UI_REPLACECONDUCTORDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ReplaceConductorDialog
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *m_multi_gb;
    QGridLayout *gridLayout;
    QLineEdit *m_function_le;
    QLabel *label;
    QComboBox *m_vertical_align_cb;
    QLabel *label_9;
    QLabel *label_15;
    QLabel *label_3;
    QCheckBox *m_erase_formula_cb;
    QSpinBox *m_vertical_angle_sb;
    QCheckBox *m_erase_function_cb;
    QComboBox *m_horizontal_align_cb;
    QCheckBox *m_erase_text_cb;
    QLineEdit *m_formula_le;
    QSpinBox *m_horizontal_angle_sb;
    QLabel *label_2;
    QCheckBox *m_erase_tension_protocol_cb;
    QLineEdit *m_text_le;
    QLabel *label_6;
    QSpinBox *m_text_size_sb;
    QLabel *label_4;
    QLabel *label_7;
    QCheckBox *m_show_text;
    QLineEdit *m_tension_protocol_le;
    QLabel *label_8;
    QLabel *label_5;
    QLabel *label_16;
    QCheckBox *m_erase_wire_color_cb;
    QCheckBox *m_erase_wire_section_cb;
    QLineEdit *m_wire_color_le;
    QLineEdit *m_wire_section_le;
    QGroupBox *m_singlewire_gb;
    QGridLayout *gridLayout_5;
    QCheckBox *m_pen_cb;
    QCheckBox *m_phase_cb;
    QSpinBox *m_phase_sb;
    QCheckBox *m_neutral_cb;
    QSlider *m_phase_slider;
    QCheckBox *m_earth_cb;
    QLabel *m_preview_label;
    QPushButton *m_update_preview_pb;
    QWidget *widget;
    QGridLayout *gridLayout_2;
    QLabel *label_13;
    QLabel *label_10;
    QPushButton *m_color_pb;
    QSpacerItem *verticalSpacer;
    QLabel *label_11;
    QCheckBox *m_mod_color_cb;
    QGroupBox *m_second_color_gb;
    QGridLayout *gridLayout_3;
    QPushButton *m_color_2_pb;
    QLabel *label_14;
    QLabel *label_12;
    QCheckBox *m_mod_color_2_cb;
    QSpinBox *m_color_2_dash_size_sb;
    QDoubleSpinBox *m_cond_size_sb;
    QComboBox *m_line_style_cb;
    QDialogButtonBox *m_button_box;

    void setupUi(QDialog *ReplaceConductorDialog)
    {
        if (ReplaceConductorDialog->objectName().isEmpty())
            ReplaceConductorDialog->setObjectName("ReplaceConductorDialog");
        ReplaceConductorDialog->resize(568, 523);
        verticalLayout = new QVBoxLayout(ReplaceConductorDialog);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(ReplaceConductorDialog);
        tabWidget->setObjectName("tabWidget");
        tab = new QWidget();
        tab->setObjectName("tab");
        verticalLayout_2 = new QVBoxLayout(tab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        m_multi_gb = new QGroupBox(tab);
        m_multi_gb->setObjectName("m_multi_gb");
        m_multi_gb->setEnabled(true);
        m_multi_gb->setCheckable(true);
        m_multi_gb->setChecked(false);
        gridLayout = new QGridLayout(m_multi_gb);
        gridLayout->setObjectName("gridLayout");
        m_function_le = new QLineEdit(m_multi_gb);
        m_function_le->setObjectName("m_function_le");
        m_function_le->setClearButtonEnabled(true);

        gridLayout->addWidget(m_function_le, 4, 1, 1, 4);

        label = new QLabel(m_multi_gb);
        label->setObjectName("label");

        gridLayout->addWidget(label, 1, 0, 1, 1);

        m_vertical_align_cb = new QComboBox(m_multi_gb);
        m_vertical_align_cb->addItem(QString());
        m_vertical_align_cb->addItem(QString());
        m_vertical_align_cb->addItem(QString());
        m_vertical_align_cb->setObjectName("m_vertical_align_cb");

        gridLayout->addWidget(m_vertical_align_cb, 8, 1, 1, 1);

        label_9 = new QLabel(m_multi_gb);
        label_9->setObjectName("label_9");
        label_9->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_9, 9, 2, 1, 1);

        label_15 = new QLabel(m_multi_gb);
        label_15->setObjectName("label_15");

        gridLayout->addWidget(label_15, 6, 0, 1, 1);

        label_3 = new QLabel(m_multi_gb);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 3, 0, 1, 1);

        m_erase_formula_cb = new QCheckBox(m_multi_gb);
        m_erase_formula_cb->setObjectName("m_erase_formula_cb");

        gridLayout->addWidget(m_erase_formula_cb, 2, 5, 1, 1);

        m_vertical_angle_sb = new QSpinBox(m_multi_gb);
        m_vertical_angle_sb->setObjectName("m_vertical_angle_sb");
        m_vertical_angle_sb->setWrapping(true);
        m_vertical_angle_sb->setMinimum(-1);
        m_vertical_angle_sb->setMaximum(359);
        m_vertical_angle_sb->setValue(-1);

        gridLayout->addWidget(m_vertical_angle_sb, 8, 3, 1, 2);

        m_erase_function_cb = new QCheckBox(m_multi_gb);
        m_erase_function_cb->setObjectName("m_erase_function_cb");

        gridLayout->addWidget(m_erase_function_cb, 4, 5, 1, 1);

        m_horizontal_align_cb = new QComboBox(m_multi_gb);
        m_horizontal_align_cb->addItem(QString());
        m_horizontal_align_cb->addItem(QString());
        m_horizontal_align_cb->addItem(QString());
        m_horizontal_align_cb->setObjectName("m_horizontal_align_cb");

        gridLayout->addWidget(m_horizontal_align_cb, 9, 1, 1, 1);

        m_erase_text_cb = new QCheckBox(m_multi_gb);
        m_erase_text_cb->setObjectName("m_erase_text_cb");

        gridLayout->addWidget(m_erase_text_cb, 3, 5, 1, 1);

        m_formula_le = new QLineEdit(m_multi_gb);
        m_formula_le->setObjectName("m_formula_le");
        m_formula_le->setClearButtonEnabled(true);

        gridLayout->addWidget(m_formula_le, 2, 1, 1, 4);

        m_horizontal_angle_sb = new QSpinBox(m_multi_gb);
        m_horizontal_angle_sb->setObjectName("m_horizontal_angle_sb");
        m_horizontal_angle_sb->setWrapping(true);
        m_horizontal_angle_sb->setMinimum(-1);
        m_horizontal_angle_sb->setMaximum(359);
        m_horizontal_angle_sb->setValue(-1);

        gridLayout->addWidget(m_horizontal_angle_sb, 9, 3, 1, 2);

        label_2 = new QLabel(m_multi_gb);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        m_erase_tension_protocol_cb = new QCheckBox(m_multi_gb);
        m_erase_tension_protocol_cb->setObjectName("m_erase_tension_protocol_cb");

        gridLayout->addWidget(m_erase_tension_protocol_cb, 5, 5, 1, 1);

        m_text_le = new QLineEdit(m_multi_gb);
        m_text_le->setObjectName("m_text_le");
        m_text_le->setClearButtonEnabled(true);

        gridLayout->addWidget(m_text_le, 3, 1, 1, 3);

        label_6 = new QLabel(m_multi_gb);
        label_6->setObjectName("label_6");

        gridLayout->addWidget(label_6, 8, 0, 1, 1);

        m_text_size_sb = new QSpinBox(m_multi_gb);
        m_text_size_sb->setObjectName("m_text_size_sb");
        m_text_size_sb->setAccelerated(false);
        m_text_size_sb->setMinimum(2);
        m_text_size_sb->setValue(2);

        gridLayout->addWidget(m_text_size_sb, 1, 1, 1, 4);

        label_4 = new QLabel(m_multi_gb);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 4, 0, 1, 1);

        label_7 = new QLabel(m_multi_gb);
        label_7->setObjectName("label_7");
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_7, 8, 2, 1, 1);

        m_show_text = new QCheckBox(m_multi_gb);
        m_show_text->setObjectName("m_show_text");

        gridLayout->addWidget(m_show_text, 3, 4, 1, 1);

        m_tension_protocol_le = new QLineEdit(m_multi_gb);
        m_tension_protocol_le->setObjectName("m_tension_protocol_le");
        m_tension_protocol_le->setClearButtonEnabled(true);

        gridLayout->addWidget(m_tension_protocol_le, 5, 1, 1, 4);

        label_8 = new QLabel(m_multi_gb);
        label_8->setObjectName("label_8");

        gridLayout->addWidget(label_8, 9, 0, 1, 1);

        label_5 = new QLabel(m_multi_gb);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 5, 0, 1, 1);

        label_16 = new QLabel(m_multi_gb);
        label_16->setObjectName("label_16");

        gridLayout->addWidget(label_16, 7, 0, 1, 1);

        m_erase_wire_color_cb = new QCheckBox(m_multi_gb);
        m_erase_wire_color_cb->setObjectName("m_erase_wire_color_cb");

        gridLayout->addWidget(m_erase_wire_color_cb, 6, 5, 1, 1);

        m_erase_wire_section_cb = new QCheckBox(m_multi_gb);
        m_erase_wire_section_cb->setObjectName("m_erase_wire_section_cb");

        gridLayout->addWidget(m_erase_wire_section_cb, 7, 5, 1, 1);

        m_wire_color_le = new QLineEdit(m_multi_gb);
        m_wire_color_le->setObjectName("m_wire_color_le");

        gridLayout->addWidget(m_wire_color_le, 6, 1, 1, 4);

        m_wire_section_le = new QLineEdit(m_multi_gb);
        m_wire_section_le->setObjectName("m_wire_section_le");

        gridLayout->addWidget(m_wire_section_le, 7, 1, 1, 4);

        gridLayout->setColumnStretch(1, 1);

        verticalLayout_2->addWidget(m_multi_gb);

        m_singlewire_gb = new QGroupBox(tab);
        m_singlewire_gb->setObjectName("m_singlewire_gb");
        m_singlewire_gb->setFlat(false);
        m_singlewire_gb->setCheckable(true);
        m_singlewire_gb->setChecked(false);
        gridLayout_5 = new QGridLayout(m_singlewire_gb);
        gridLayout_5->setObjectName("gridLayout_5");
        m_pen_cb = new QCheckBox(m_singlewire_gb);
        m_pen_cb->setObjectName("m_pen_cb");

        gridLayout_5->addWidget(m_pen_cb, 1, 2, 1, 1);

        m_phase_cb = new QCheckBox(m_singlewire_gb);
        m_phase_cb->setObjectName("m_phase_cb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/phase.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_phase_cb->setIcon(icon);

        gridLayout_5->addWidget(m_phase_cb, 2, 1, 1, 1);

        m_phase_sb = new QSpinBox(m_singlewire_gb);
        m_phase_sb->setObjectName("m_phase_sb");
        m_phase_sb->setMinimum(1);
        m_phase_sb->setMaximum(3);

        gridLayout_5->addWidget(m_phase_sb, 2, 3, 1, 1);

        m_neutral_cb = new QCheckBox(m_singlewire_gb);
        m_neutral_cb->setObjectName("m_neutral_cb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/neutral.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_neutral_cb->setIcon(icon1);

        gridLayout_5->addWidget(m_neutral_cb, 1, 1, 1, 1);

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

        gridLayout_5->addWidget(m_phase_slider, 2, 2, 1, 1);

        m_earth_cb = new QCheckBox(m_singlewire_gb);
        m_earth_cb->setObjectName("m_earth_cb");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/ground.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_earth_cb->setIcon(icon2);

        gridLayout_5->addWidget(m_earth_cb, 0, 1, 1, 1);

        m_preview_label = new QLabel(m_singlewire_gb);
        m_preview_label->setObjectName("m_preview_label");
        m_preview_label->setAlignment(Qt::AlignCenter);

        gridLayout_5->addWidget(m_preview_label, 0, 0, 3, 1);

        m_update_preview_pb = new QPushButton(m_singlewire_gb);
        m_update_preview_pb->setObjectName("m_update_preview_pb");

        gridLayout_5->addWidget(m_update_preview_pb, 0, 2, 1, 1);


        verticalLayout_2->addWidget(m_singlewire_gb);

        tabWidget->addTab(tab, QString());
        widget = new QWidget();
        widget->setObjectName("widget");
        gridLayout_2 = new QGridLayout(widget);
        gridLayout_2->setObjectName("gridLayout_2");
        label_13 = new QLabel(widget);
        label_13->setObjectName("label_13");

        gridLayout_2->addWidget(label_13, 2, 1, 1, 1);

        label_10 = new QLabel(widget);
        label_10->setObjectName("label_10");

        gridLayout_2->addWidget(label_10, 0, 1, 1, 1);

        m_color_pb = new QPushButton(widget);
        m_color_pb->setObjectName("m_color_pb");
        m_color_pb->setEnabled(false);

        gridLayout_2->addWidget(m_color_pb, 0, 2, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 4, 1, 1, 1);

        label_11 = new QLabel(widget);
        label_11->setObjectName("label_11");

        gridLayout_2->addWidget(label_11, 1, 1, 1, 1);

        m_mod_color_cb = new QCheckBox(widget);
        m_mod_color_cb->setObjectName("m_mod_color_cb");

        gridLayout_2->addWidget(m_mod_color_cb, 0, 3, 1, 1);

        m_second_color_gb = new QGroupBox(widget);
        m_second_color_gb->setObjectName("m_second_color_gb");
        m_second_color_gb->setCheckable(true);
        m_second_color_gb->setChecked(false);
        gridLayout_3 = new QGridLayout(m_second_color_gb);
        gridLayout_3->setObjectName("gridLayout_3");
        m_color_2_pb = new QPushButton(m_second_color_gb);
        m_color_2_pb->setObjectName("m_color_2_pb");
        m_color_2_pb->setEnabled(false);

        gridLayout_3->addWidget(m_color_2_pb, 0, 1, 1, 1);

        label_14 = new QLabel(m_second_color_gb);
        label_14->setObjectName("label_14");

        gridLayout_3->addWidget(label_14, 1, 0, 1, 1);

        label_12 = new QLabel(m_second_color_gb);
        label_12->setObjectName("label_12");

        gridLayout_3->addWidget(label_12, 0, 0, 1, 1);

        m_mod_color_2_cb = new QCheckBox(m_second_color_gb);
        m_mod_color_2_cb->setObjectName("m_mod_color_2_cb");

        gridLayout_3->addWidget(m_mod_color_2_cb, 0, 2, 1, 1);

        m_color_2_dash_size_sb = new QSpinBox(m_second_color_gb);
        m_color_2_dash_size_sb->setObjectName("m_color_2_dash_size_sb");
        m_color_2_dash_size_sb->setMinimum(1);
        m_color_2_dash_size_sb->setValue(1);

        gridLayout_3->addWidget(m_color_2_dash_size_sb, 1, 1, 1, 2);

        gridLayout_3->setColumnStretch(1, 1);

        gridLayout_2->addWidget(m_second_color_gb, 3, 1, 1, 3);

        m_cond_size_sb = new QDoubleSpinBox(widget);
        m_cond_size_sb->setObjectName("m_cond_size_sb");
        m_cond_size_sb->setMinimum(0.200000000000000);
        m_cond_size_sb->setSingleStep(0.200000000000000);
        m_cond_size_sb->setValue(0.200000000000000);

        gridLayout_2->addWidget(m_cond_size_sb, 2, 2, 1, 2);

        m_line_style_cb = new QComboBox(widget);
        m_line_style_cb->setObjectName("m_line_style_cb");

        gridLayout_2->addWidget(m_line_style_cb, 1, 2, 1, 2);

        gridLayout_2->setColumnStretch(2, 1);
        tabWidget->addTab(widget, QString());

        verticalLayout->addWidget(tabWidget);

        m_button_box = new QDialogButtonBox(ReplaceConductorDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setOrientation(Qt::Horizontal);
        m_button_box->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::Reset);

        verticalLayout->addWidget(m_button_box);


        retranslateUi(ReplaceConductorDialog);
        QObject::connect(m_mod_color_cb, &QCheckBox::toggled, m_color_pb, &QPushButton::setEnabled);
        QObject::connect(m_earth_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_neutral_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_phase_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_pen_cb, &QCheckBox::clicked, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_phase_slider, &QSlider::valueChanged, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_phase_sb, &QSpinBox::valueChanged, m_update_preview_pb, qOverload<>(&QPushButton::click));
        QObject::connect(m_phase_slider, &QSlider::sliderMoved, m_phase_sb, &QSpinBox::setValue);
        QObject::connect(m_phase_sb, &QSpinBox::valueChanged, m_phase_slider, &QSlider::setValue);
        QObject::connect(m_mod_color_2_cb, &QCheckBox::toggled, m_color_2_pb, &QPushButton::setEnabled);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ReplaceConductorDialog);
    } // setupUi

    void retranslateUi(QDialog *ReplaceConductorDialog)
    {
        m_multi_gb->setTitle(QCoreApplication::translate("ReplaceConductorDialog", "&Multifilaire", nullptr));
        m_function_le->setPlaceholderText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        label->setText(QCoreApplication::translate("ReplaceConductorDialog", "Taille du texte :", nullptr));
        m_vertical_align_cb->setItemText(0, QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        m_vertical_align_cb->setItemText(1, QCoreApplication::translate("ReplaceConductorDialog", "\303\200 gauche", nullptr));
        m_vertical_align_cb->setItemText(2, QCoreApplication::translate("ReplaceConductorDialog", "\303\200 droite", nullptr));

        m_vertical_align_cb->setCurrentText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        label_9->setText(QCoreApplication::translate("ReplaceConductorDialog", "Angle :", nullptr));
        label_15->setText(QCoreApplication::translate("ReplaceConductorDialog", "Couleur du conducteur", nullptr));
        label_3->setText(QCoreApplication::translate("ReplaceConductorDialog", "Texte :", nullptr));
#if QT_CONFIG(tooltip)
        m_erase_formula_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_erase_formula_cb->setText(QString());
        m_vertical_angle_sb->setSpecialValueText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        m_vertical_angle_sb->setSuffix(QCoreApplication::translate("ReplaceConductorDialog", "\302\260", nullptr));
        m_vertical_angle_sb->setPrefix(QString());
#if QT_CONFIG(tooltip)
        m_erase_function_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_erase_function_cb->setText(QString());
        m_horizontal_align_cb->setItemText(0, QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        m_horizontal_align_cb->setItemText(1, QCoreApplication::translate("ReplaceConductorDialog", "En haut", nullptr));
        m_horizontal_align_cb->setItemText(2, QCoreApplication::translate("ReplaceConductorDialog", "En bas", nullptr));

#if QT_CONFIG(tooltip)
        m_erase_text_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_erase_text_cb->setText(QString());
        m_formula_le->setPlaceholderText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        m_horizontal_angle_sb->setSpecialValueText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        m_horizontal_angle_sb->setSuffix(QCoreApplication::translate("ReplaceConductorDialog", "\302\260", nullptr));
        label_2->setText(QCoreApplication::translate("ReplaceConductorDialog", "Formule du texte :", nullptr));
#if QT_CONFIG(tooltip)
        m_erase_tension_protocol_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_erase_tension_protocol_cb->setText(QString());
        m_text_le->setPlaceholderText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        label_6->setText(QCoreApplication::translate("ReplaceConductorDialog", "Texte sur conducteur vertical :", nullptr));
        m_text_size_sb->setSpecialValueText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        label_4->setText(QCoreApplication::translate("ReplaceConductorDialog", "Fonction :", nullptr));
        label_7->setText(QCoreApplication::translate("ReplaceConductorDialog", "Angle :", nullptr));
#if QT_CONFIG(tooltip)
        m_show_text->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Texte visible", nullptr));
#endif // QT_CONFIG(tooltip)
        m_show_text->setText(QString());
        m_tension_protocol_le->setPlaceholderText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        label_8->setText(QCoreApplication::translate("ReplaceConductorDialog", "Texte sur conducteur horizontal :", nullptr));
        label_5->setText(QCoreApplication::translate("ReplaceConductorDialog", "Tension / protocol :", nullptr));
        label_16->setText(QCoreApplication::translate("ReplaceConductorDialog", "Section du conducteur", nullptr));
        m_erase_wire_color_cb->setText(QString());
        m_erase_wire_section_cb->setText(QString());
        m_singlewire_gb->setTitle(QCoreApplication::translate("ReplaceConductorDialog", "Unifilaire", nullptr));
#if QT_CONFIG(tooltip)
        m_pen_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Protective Earth Neutral", nullptr));
#endif // QT_CONFIG(tooltip)
        m_pen_cb->setText(QCoreApplication::translate("ReplaceConductorDialog", "PEN", nullptr));
#if QT_CONFIG(tooltip)
        m_phase_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Phase", nullptr));
#endif // QT_CONFIG(tooltip)
        m_phase_cb->setText(QCoreApplication::translate("ReplaceConductorDialog", "phase", nullptr));
#if QT_CONFIG(tooltip)
        m_phase_sb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Nombre de phase", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_neutral_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Neutre", nullptr));
#endif // QT_CONFIG(tooltip)
        m_neutral_cb->setText(QCoreApplication::translate("ReplaceConductorDialog", "neutre", nullptr));
#if QT_CONFIG(tooltip)
        m_phase_slider->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Nombre de phase", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_earth_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Terre", nullptr));
#endif // QT_CONFIG(tooltip)
        m_earth_cb->setText(QCoreApplication::translate("ReplaceConductorDialog", "terre", nullptr));
        m_preview_label->setText(QCoreApplication::translate("ReplaceConductorDialog", "TextLabel", nullptr));
        m_update_preview_pb->setText(QCoreApplication::translate("ReplaceConductorDialog", "PushButton", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("ReplaceConductorDialog", "Type", nullptr));
        label_13->setText(QCoreApplication::translate("ReplaceConductorDialog", "Taille :", nullptr));
        label_10->setText(QCoreApplication::translate("ReplaceConductorDialog", "Couleur :", nullptr));
        m_color_pb->setText(QString());
        label_11->setText(QCoreApplication::translate("ReplaceConductorDialog", "Style :", nullptr));
#if QT_CONFIG(tooltip)
        m_mod_color_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Modifier", nullptr));
#endif // QT_CONFIG(tooltip)
        m_mod_color_cb->setText(QString());
        m_second_color_gb->setTitle(QCoreApplication::translate("ReplaceConductorDialog", "Couleur secondaire :", nullptr));
        m_color_2_pb->setText(QString());
        label_14->setText(QCoreApplication::translate("ReplaceConductorDialog", "Taille de trait :", nullptr));
        label_12->setText(QCoreApplication::translate("ReplaceConductorDialog", "Couleur :", nullptr));
#if QT_CONFIG(tooltip)
        m_mod_color_2_cb->setToolTip(QCoreApplication::translate("ReplaceConductorDialog", "Modifier", nullptr));
#endif // QT_CONFIG(tooltip)
        m_mod_color_2_cb->setText(QString());
        m_color_2_dash_size_sb->setSpecialValueText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        m_color_2_dash_size_sb->setSuffix(QCoreApplication::translate("ReplaceConductorDialog", "px", nullptr));
        m_cond_size_sb->setSpecialValueText(QCoreApplication::translate("ReplaceConductorDialog", "Ne pas modifier", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(widget), QCoreApplication::translate("ReplaceConductorDialog", "Apparence", nullptr));
        (void)ReplaceConductorDialog;
    } // retranslateUi

};

namespace Ui {
    class ReplaceConductorDialog: public Ui_ReplaceConductorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPLACECONDUCTORDIALOG_H
