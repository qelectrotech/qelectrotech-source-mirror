/********************************************************************************
** Form generated from reading UI file 'terminalstriplayouteditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TERMINALSTRIPLAYOUTEDITOR_H
#define UI_TERMINALSTRIPLAYOUTEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TerminalStripLayoutEditor
{
public:
    QGridLayout *gridLayout;
    QSpinBox *m_width_terminal_0_sb;
    QSpinBox *m_height_terminal_2_sb;
    QFrame *line;
    QLabel *label_3;
    QSpinBox *m_bridge_point_3_sb;
    QSpinBox *m_height_terminal_1_sb;
    QLabel *label_7;
    QSpinBox *m_width_terminal_3_sb;
    QSpinBox *m_bridge_point_2_sb;
    QSpinBox *m_width_terminal_1_sb;
    QSpinBox *m_height_terminal_3_sb;
    QGraphicsView *m_graphics_view;
    QSpinBox *m_height_spacer_sb;
    QLabel *label_10;
    QLabel *label_9;
    QSpinBox *m_width_spacer_sb;
    QSpinBox *m_height_terminal_0_sb;
    QSpinBox *m_y_spacer_sb;
    QCheckBox *m_display_preview_help;
    QLabel *label_4;
    QSpinBox *m_bridge_point_1_sb;
    QLabel *label_6;
    QFrame *line_2;
    QSpinBox *m_bridge_point_0_sb;
    QSpinBox *m_y_header_sb;
    QLabel *label_8;
    QWidget *widget;
    QGridLayout *gridLayout_2;
    QComboBox *m_header_text_orientation_cb;
    QComboBox *m_header_text_alignment_cb;
    QLabel *label_14;
    QLabel *label_15;
    QWidget *widget_2;
    QHBoxLayout *horizontalLayout;
    QLabel *label_11;
    QFontComboBox *m_font_cb;
    QLabel *label_12;
    QSpinBox *m_font_size_sb;
    QSpacerItem *horizontalSpacer;
    QLabel *label_20;
    QLabel *label_13;
    QLabel *label_19;
    QSpinBox *m_terminal_text_y_sb;
    QSpinBox *m_terminal_text_height_sb;
    QComboBox *m_terminal_text_alignment_cb;
    QComboBox *m_terminal_text_orientation_cb;
    QLabel *label_21;
    QLabel *label_22;
    QComboBox *m_xref_orientation_cb;
    QComboBox *m_xref_alignment_cb;
    QSpinBox *m_xref_y_sb;
    QSpinBox *m_xref_height_sb;
    QSpinBox *m_width_terminal_2_sb;
    QSpinBox *m_width_header_sb;
    QLabel *label;
    QSpinBox *m_height_header_sb;
    QSpinBox *m_y_terminal_2_sb;
    QFrame *line_5;
    QSpinBox *m_y_terminal_3_sb;
    QSpinBox *m_y_terminal_1_sb;
    QLabel *label_5;
    QLabel *label_18;
    QSpinBox *m_y_terminal_0_sb;
    QLabel *label_2;

    void setupUi(QWidget *TerminalStripLayoutEditor)
    {
        if (TerminalStripLayoutEditor->objectName().isEmpty())
            TerminalStripLayoutEditor->setObjectName("TerminalStripLayoutEditor");
        TerminalStripLayoutEditor->resize(961, 624);
        gridLayout = new QGridLayout(TerminalStripLayoutEditor);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setSizeConstraint(QLayout::SetMaximumSize);
        m_width_terminal_0_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_width_terminal_0_sb->setObjectName("m_width_terminal_0_sb");
        m_width_terminal_0_sb->setMaximum(1000);

        gridLayout->addWidget(m_width_terminal_0_sb, 4, 2, 1, 1);

        m_height_terminal_2_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_height_terminal_2_sb->setObjectName("m_height_terminal_2_sb");
        m_height_terminal_2_sb->setMaximum(1000);

        gridLayout->addWidget(m_height_terminal_2_sb, 6, 3, 1, 1);

        line = new QFrame(TerminalStripLayoutEditor);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout->addWidget(line, 3, 0, 1, 4);

        label_3 = new QLabel(TerminalStripLayoutEditor);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 0, 1, 1, 1);

        m_bridge_point_3_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_bridge_point_3_sb->setObjectName("m_bridge_point_3_sb");
        m_bridge_point_3_sb->setMaximum(1000);

        gridLayout->addWidget(m_bridge_point_3_sb, 7, 4, 1, 1);

        m_height_terminal_1_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_height_terminal_1_sb->setObjectName("m_height_terminal_1_sb");
        m_height_terminal_1_sb->setMaximum(1000);

        gridLayout->addWidget(m_height_terminal_1_sb, 5, 3, 1, 1);

        label_7 = new QLabel(TerminalStripLayoutEditor);
        label_7->setObjectName("label_7");

        gridLayout->addWidget(label_7, 4, 0, 1, 1);

        m_width_terminal_3_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_width_terminal_3_sb->setObjectName("m_width_terminal_3_sb");
        m_width_terminal_3_sb->setMaximum(1000);

        gridLayout->addWidget(m_width_terminal_3_sb, 7, 2, 1, 1);

        m_bridge_point_2_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_bridge_point_2_sb->setObjectName("m_bridge_point_2_sb");
        m_bridge_point_2_sb->setMaximum(1000);

        gridLayout->addWidget(m_bridge_point_2_sb, 6, 4, 1, 1);

        m_width_terminal_1_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_width_terminal_1_sb->setObjectName("m_width_terminal_1_sb");
        m_width_terminal_1_sb->setMaximum(1000);

        gridLayout->addWidget(m_width_terminal_1_sb, 5, 2, 1, 1);

        m_height_terminal_3_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_height_terminal_3_sb->setObjectName("m_height_terminal_3_sb");
        m_height_terminal_3_sb->setMaximum(1000);

        gridLayout->addWidget(m_height_terminal_3_sb, 7, 3, 1, 1);

        m_graphics_view = new QGraphicsView(TerminalStripLayoutEditor);
        m_graphics_view->setObjectName("m_graphics_view");

        gridLayout->addWidget(m_graphics_view, 14, 0, 1, 5);

        m_height_spacer_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_height_spacer_sb->setObjectName("m_height_spacer_sb");
        m_height_spacer_sb->setMaximum(1000);

        gridLayout->addWidget(m_height_spacer_sb, 2, 3, 1, 1);

        label_10 = new QLabel(TerminalStripLayoutEditor);
        label_10->setObjectName("label_10");

        gridLayout->addWidget(label_10, 7, 0, 1, 1);

        label_9 = new QLabel(TerminalStripLayoutEditor);
        label_9->setObjectName("label_9");

        gridLayout->addWidget(label_9, 6, 0, 1, 1);

        m_width_spacer_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_width_spacer_sb->setObjectName("m_width_spacer_sb");
        m_width_spacer_sb->setMaximum(1000);

        gridLayout->addWidget(m_width_spacer_sb, 2, 2, 1, 1);

        m_height_terminal_0_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_height_terminal_0_sb->setObjectName("m_height_terminal_0_sb");
        m_height_terminal_0_sb->setMaximum(1000);

        gridLayout->addWidget(m_height_terminal_0_sb, 4, 3, 1, 1);

        m_y_spacer_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_y_spacer_sb->setObjectName("m_y_spacer_sb");
        m_y_spacer_sb->setMaximum(1000);

        gridLayout->addWidget(m_y_spacer_sb, 2, 1, 1, 1);

        m_display_preview_help = new QCheckBox(TerminalStripLayoutEditor);
        m_display_preview_help->setObjectName("m_display_preview_help");
        m_display_preview_help->setChecked(false);

        gridLayout->addWidget(m_display_preview_help, 13, 4, 1, 1);

        label_4 = new QLabel(TerminalStripLayoutEditor);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 0, 2, 1, 1);

        m_bridge_point_1_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_bridge_point_1_sb->setObjectName("m_bridge_point_1_sb");
        m_bridge_point_1_sb->setMaximum(1000);

        gridLayout->addWidget(m_bridge_point_1_sb, 5, 4, 1, 1);

        label_6 = new QLabel(TerminalStripLayoutEditor);
        label_6->setObjectName("label_6");

        gridLayout->addWidget(label_6, 2, 0, 1, 1);

        line_2 = new QFrame(TerminalStripLayoutEditor);
        line_2->setObjectName("line_2");
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout->addWidget(line_2, 11, 0, 1, 5);

        m_bridge_point_0_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_bridge_point_0_sb->setObjectName("m_bridge_point_0_sb");
        m_bridge_point_0_sb->setMaximum(1000);

        gridLayout->addWidget(m_bridge_point_0_sb, 4, 4, 1, 1);

        m_y_header_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_y_header_sb->setObjectName("m_y_header_sb");
        m_y_header_sb->setMaximum(1000);

        gridLayout->addWidget(m_y_header_sb, 1, 1, 1, 1);

        label_8 = new QLabel(TerminalStripLayoutEditor);
        label_8->setObjectName("label_8");

        gridLayout->addWidget(label_8, 5, 0, 1, 1);

        widget = new QWidget(TerminalStripLayoutEditor);
        widget->setObjectName("widget");
        gridLayout_2 = new QGridLayout(widget);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        m_header_text_orientation_cb = new QComboBox(widget);
        m_header_text_orientation_cb->addItem(QString());
        m_header_text_orientation_cb->addItem(QString());
        m_header_text_orientation_cb->setObjectName("m_header_text_orientation_cb");

        gridLayout_2->addWidget(m_header_text_orientation_cb, 5, 3, 1, 1);

        m_header_text_alignment_cb = new QComboBox(widget);
        m_header_text_alignment_cb->addItem(QString());
        m_header_text_alignment_cb->addItem(QString());
        m_header_text_alignment_cb->addItem(QString());
        m_header_text_alignment_cb->setObjectName("m_header_text_alignment_cb");

        gridLayout_2->addWidget(m_header_text_alignment_cb, 6, 3, 1, 1);

        label_14 = new QLabel(widget);
        label_14->setObjectName("label_14");

        gridLayout_2->addWidget(label_14, 5, 2, 1, 1);

        label_15 = new QLabel(widget);
        label_15->setObjectName("label_15");

        gridLayout_2->addWidget(label_15, 6, 2, 1, 1);

        widget_2 = new QWidget(widget);
        widget_2->setObjectName("widget_2");
        horizontalLayout = new QHBoxLayout(widget_2);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_11 = new QLabel(widget_2);
        label_11->setObjectName("label_11");

        horizontalLayout->addWidget(label_11);

        m_font_cb = new QFontComboBox(widget_2);
        m_font_cb->setObjectName("m_font_cb");

        horizontalLayout->addWidget(m_font_cb);

        label_12 = new QLabel(widget_2);
        label_12->setObjectName("label_12");

        horizontalLayout->addWidget(label_12);

        m_font_size_sb = new QSpinBox(widget_2);
        m_font_size_sb->setObjectName("m_font_size_sb");
        m_font_size_sb->setMinimum(1);

        horizontalLayout->addWidget(m_font_size_sb);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        gridLayout_2->addWidget(widget_2, 1, 2, 1, 4);

        label_20 = new QLabel(widget);
        label_20->setObjectName("label_20");
        label_20->setAlignment(Qt::AlignCenter);

        gridLayout_2->addWidget(label_20, 2, 3, 1, 1);

        label_13 = new QLabel(widget);
        label_13->setObjectName("label_13");

        gridLayout_2->addWidget(label_13, 7, 2, 1, 1);

        label_19 = new QLabel(widget);
        label_19->setObjectName("label_19");

        gridLayout_2->addWidget(label_19, 8, 2, 1, 1);

        m_terminal_text_y_sb = new QSpinBox(widget);
        m_terminal_text_y_sb->setObjectName("m_terminal_text_y_sb");
        m_terminal_text_y_sb->setMaximum(1000);

        gridLayout_2->addWidget(m_terminal_text_y_sb, 7, 4, 1, 1);

        m_terminal_text_height_sb = new QSpinBox(widget);
        m_terminal_text_height_sb->setObjectName("m_terminal_text_height_sb");
        m_terminal_text_height_sb->setMinimum(30);
        m_terminal_text_height_sb->setMaximum(1000);

        gridLayout_2->addWidget(m_terminal_text_height_sb, 8, 4, 1, 1);

        m_terminal_text_alignment_cb = new QComboBox(widget);
        m_terminal_text_alignment_cb->addItem(QString());
        m_terminal_text_alignment_cb->addItem(QString());
        m_terminal_text_alignment_cb->addItem(QString());
        m_terminal_text_alignment_cb->setObjectName("m_terminal_text_alignment_cb");

        gridLayout_2->addWidget(m_terminal_text_alignment_cb, 6, 4, 1, 1);

        m_terminal_text_orientation_cb = new QComboBox(widget);
        m_terminal_text_orientation_cb->addItem(QString());
        m_terminal_text_orientation_cb->addItem(QString());
        m_terminal_text_orientation_cb->setObjectName("m_terminal_text_orientation_cb");

        gridLayout_2->addWidget(m_terminal_text_orientation_cb, 5, 4, 1, 1);

        label_21 = new QLabel(widget);
        label_21->setObjectName("label_21");
        label_21->setAlignment(Qt::AlignCenter);

        gridLayout_2->addWidget(label_21, 2, 4, 1, 1);

        label_22 = new QLabel(widget);
        label_22->setObjectName("label_22");
        label_22->setAlignment(Qt::AlignCenter);

        gridLayout_2->addWidget(label_22, 2, 5, 1, 1);

        m_xref_orientation_cb = new QComboBox(widget);
        m_xref_orientation_cb->addItem(QString());
        m_xref_orientation_cb->addItem(QString());
        m_xref_orientation_cb->setObjectName("m_xref_orientation_cb");

        gridLayout_2->addWidget(m_xref_orientation_cb, 5, 5, 1, 1);

        m_xref_alignment_cb = new QComboBox(widget);
        m_xref_alignment_cb->addItem(QString());
        m_xref_alignment_cb->addItem(QString());
        m_xref_alignment_cb->addItem(QString());
        m_xref_alignment_cb->setObjectName("m_xref_alignment_cb");

        gridLayout_2->addWidget(m_xref_alignment_cb, 6, 5, 1, 1);

        m_xref_y_sb = new QSpinBox(widget);
        m_xref_y_sb->setObjectName("m_xref_y_sb");
        m_xref_y_sb->setMaximum(1000);

        gridLayout_2->addWidget(m_xref_y_sb, 7, 5, 1, 1);

        m_xref_height_sb = new QSpinBox(widget);
        m_xref_height_sb->setObjectName("m_xref_height_sb");
        m_xref_height_sb->setMinimum(30);
        m_xref_height_sb->setMaximum(1000);

        gridLayout_2->addWidget(m_xref_height_sb, 8, 5, 1, 1);


        gridLayout->addWidget(widget, 10, 0, 1, 5);

        m_width_terminal_2_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_width_terminal_2_sb->setObjectName("m_width_terminal_2_sb");
        m_width_terminal_2_sb->setMaximum(1000);

        gridLayout->addWidget(m_width_terminal_2_sb, 6, 2, 1, 1);

        m_width_header_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_width_header_sb->setObjectName("m_width_header_sb");
        m_width_header_sb->setMaximum(1000);

        gridLayout->addWidget(m_width_header_sb, 1, 2, 1, 1);

        label = new QLabel(TerminalStripLayoutEditor);
        label->setObjectName("label");

        gridLayout->addWidget(label, 1, 0, 1, 1);

        m_height_header_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_height_header_sb->setObjectName("m_height_header_sb");
        m_height_header_sb->setMaximum(1000);

        gridLayout->addWidget(m_height_header_sb, 1, 3, 1, 1);

        m_y_terminal_2_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_y_terminal_2_sb->setObjectName("m_y_terminal_2_sb");
        m_y_terminal_2_sb->setMaximum(1000);

        gridLayout->addWidget(m_y_terminal_2_sb, 6, 1, 1, 1);

        line_5 = new QFrame(TerminalStripLayoutEditor);
        line_5->setObjectName("line_5");
        line_5->setFrameShape(QFrame::Shape::HLine);
        line_5->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout->addWidget(line_5, 9, 0, 1, 5);

        m_y_terminal_3_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_y_terminal_3_sb->setObjectName("m_y_terminal_3_sb");
        m_y_terminal_3_sb->setMaximum(1000);

        gridLayout->addWidget(m_y_terminal_3_sb, 7, 1, 1, 1);

        m_y_terminal_1_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_y_terminal_1_sb->setObjectName("m_y_terminal_1_sb");
        m_y_terminal_1_sb->setMaximum(1000);

        gridLayout->addWidget(m_y_terminal_1_sb, 5, 1, 1, 1);

        label_5 = new QLabel(TerminalStripLayoutEditor);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 0, 3, 1, 1);

        label_18 = new QLabel(TerminalStripLayoutEditor);
        label_18->setObjectName("label_18");

        gridLayout->addWidget(label_18, 3, 4, 1, 1);

        m_y_terminal_0_sb = new QSpinBox(TerminalStripLayoutEditor);
        m_y_terminal_0_sb->setObjectName("m_y_terminal_0_sb");
        m_y_terminal_0_sb->setMaximum(1000);

        gridLayout->addWidget(m_y_terminal_0_sb, 4, 1, 1, 1);

        label_2 = new QLabel(TerminalStripLayoutEditor);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 13, 0, 1, 1);


        retranslateUi(TerminalStripLayoutEditor);
        QObject::connect(m_y_header_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_y_terminal_1_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_bridge_point_0_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_height_terminal_0_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_width_terminal_0_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_y_terminal_0_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_height_spacer_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_height_header_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_y_spacer_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_width_spacer_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_width_header_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_y_terminal_2_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_y_terminal_3_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_width_terminal_1_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_width_terminal_2_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_width_terminal_3_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_height_terminal_1_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_height_terminal_2_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_height_terminal_3_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_bridge_point_1_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_bridge_point_3_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_bridge_point_2_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_header_text_alignment_cb, SIGNAL(currentIndexChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_header_text_orientation_cb, SIGNAL(currentIndexChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_terminal_text_alignment_cb, SIGNAL(currentIndexChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_terminal_text_orientation_cb, SIGNAL(currentIndexChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_font_cb, SIGNAL(currentFontChanged(QFont)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_font_size_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_terminal_text_y_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_terminal_text_height_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_xref_alignment_cb, SIGNAL(currentIndexChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_xref_height_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_xref_orientation_cb, SIGNAL(currentIndexChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));
        QObject::connect(m_xref_y_sb, SIGNAL(valueChanged(int)), TerminalStripLayoutEditor, SLOT(valueEdited()));

        QMetaObject::connectSlotsByName(TerminalStripLayoutEditor);
    } // setupUi

    void retranslateUi(QWidget *TerminalStripLayoutEditor)
    {
        TerminalStripLayoutEditor->setWindowTitle(QCoreApplication::translate("TerminalStripLayoutEditor", "Form", nullptr));
        label_3->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "D\303\251calage vertical", nullptr));
        label_7->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Borne niveau 0 :", nullptr));
        label_10->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Borne niveau 3 :", nullptr));
        label_9->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Borne niveau 2 :", nullptr));
        m_display_preview_help->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Afficher l'aide", nullptr));
        label_4->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Largeur", nullptr));
        label_6->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Espace :", nullptr));
        label_8->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Borne niveau 1 :", nullptr));
        m_header_text_orientation_cb->setItemText(0, QCoreApplication::translate("TerminalStripLayoutEditor", "Horizontal", nullptr));
        m_header_text_orientation_cb->setItemText(1, QCoreApplication::translate("TerminalStripLayoutEditor", "Vertical", nullptr));

        m_header_text_alignment_cb->setItemText(0, QCoreApplication::translate("TerminalStripLayoutEditor", "Gauche", nullptr));
        m_header_text_alignment_cb->setItemText(1, QCoreApplication::translate("TerminalStripLayoutEditor", "Centre", nullptr));
        m_header_text_alignment_cb->setItemText(2, QCoreApplication::translate("TerminalStripLayoutEditor", "Droite", nullptr));

        label_14->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Orientation", nullptr));
        label_15->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Alignement", nullptr));
        label_11->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Police :", nullptr));
        label_12->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Taille :", nullptr));
        label_20->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Texte d'en t\303\252te", nullptr));
        label_13->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Origine vertical", nullptr));
        label_19->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Longueur maximal", nullptr));
        m_terminal_text_alignment_cb->setItemText(0, QCoreApplication::translate("TerminalStripLayoutEditor", "Gauche", nullptr));
        m_terminal_text_alignment_cb->setItemText(1, QCoreApplication::translate("TerminalStripLayoutEditor", "Centre", nullptr));
        m_terminal_text_alignment_cb->setItemText(2, QCoreApplication::translate("TerminalStripLayoutEditor", "Droite", nullptr));

        m_terminal_text_orientation_cb->setItemText(0, QCoreApplication::translate("TerminalStripLayoutEditor", "Horizontal", nullptr));
        m_terminal_text_orientation_cb->setItemText(1, QCoreApplication::translate("TerminalStripLayoutEditor", "Vertical", nullptr));

        label_21->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Texte borne", nullptr));
        label_22->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "R\303\251f\303\251rence crois\303\251e", nullptr));
        m_xref_orientation_cb->setItemText(0, QCoreApplication::translate("TerminalStripLayoutEditor", "Horizontal", nullptr));
        m_xref_orientation_cb->setItemText(1, QCoreApplication::translate("TerminalStripLayoutEditor", "Vertical", nullptr));

        m_xref_alignment_cb->setItemText(0, QCoreApplication::translate("TerminalStripLayoutEditor", "Gauche", nullptr));
        m_xref_alignment_cb->setItemText(1, QCoreApplication::translate("TerminalStripLayoutEditor", "Centre", nullptr));
        m_xref_alignment_cb->setItemText(2, QCoreApplication::translate("TerminalStripLayoutEditor", "Droite", nullptr));

        label->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "En t\303\252te :", nullptr));
        label_5->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Hauteur", nullptr));
        label_18->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Point de pont", nullptr));
        label_2->setText(QCoreApplication::translate("TerminalStripLayoutEditor", "Pr\303\251visualisation :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TerminalStripLayoutEditor: public Ui_TerminalStripLayoutEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TERMINALSTRIPLAYOUTEDITOR_H
