/********************************************************************************
** Form generated from reading UI file 'graphicstablepropertieseditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GRAPHICSTABLEPROPERTIESEDITOR_H
#define UI_GRAPHICSTABLEPROPERTIESEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GraphicsTablePropertiesEditor
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *m_tab;
    QWidget *m_display_tab;
    QVBoxLayout *verticalLayout_5;
    QLineEdit *m_table_name_le;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QSpinBox *m_display_n_row_sb;
    QPushButton *m_next_pb;
    QComboBox *m_previous_table_cb;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_auto_geometry_pb;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_7;
    QLabel *label_8;
    QSpinBox *m_x_pos;
    QWidget *widget_3;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *m_previous_pb;
    QSpinBox *m_y_pos;
    QPushButton *m_apply_geometry_to_linked_table_pb;
    QLabel *label_3;
    QLabel *label;
    QLabel *m_info_label;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QGridLayout *gridLayout;
    QSpinBox *m_header_bottom_margin;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_2;
    QSpinBox *m_header_right_margin;
    QSpacerItem *horizontalSpacer_3;
    QSpinBox *m_header_left_margin;
    QSpinBox *m_header_top_margin;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_7;
    QLabel *label_5;
    QRadioButton *m_header_align_left_rb;
    QRadioButton *m_header_align_center_rb;
    QRadioButton *m_header_align_right_rb;
    QSpacerItem *horizontalSpacer_8;
    QPushButton *m_header_font_pb;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_3;
    QGridLayout *gridLayout_3;
    QSpinBox *m_table_bottom_margin;
    QSpinBox *m_table_left_margin;
    QSpinBox *m_table_top_margin;
    QSpacerItem *horizontalSpacer_6;
    QSpinBox *m_table_right_margin;
    QLabel *label_4;
    QSpacerItem *horizontalSpacer_5;
    QWidget *widget_2;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_9;
    QLabel *label_6;
    QRadioButton *m_table_align_left_rb;
    QRadioButton *m_table_align_center_rb;
    QRadioButton *m_table_align_right_rb;
    QSpacerItem *horizontalSpacer_10;
    QPushButton *m_table_font_pb;
    QSpacerItem *verticalSpacer;
    QWidget *m_content_tab;
    QVBoxLayout *m_content_layout;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QWidget *GraphicsTablePropertiesEditor)
    {
        if (GraphicsTablePropertiesEditor->objectName().isEmpty())
            GraphicsTablePropertiesEditor->setObjectName("GraphicsTablePropertiesEditor");
        GraphicsTablePropertiesEditor->resize(524, 623);
        verticalLayout = new QVBoxLayout(GraphicsTablePropertiesEditor);
        verticalLayout->setObjectName("verticalLayout");
        m_tab = new QTabWidget(GraphicsTablePropertiesEditor);
        m_tab->setObjectName("m_tab");
        m_display_tab = new QWidget();
        m_display_tab->setObjectName("m_display_tab");
        verticalLayout_5 = new QVBoxLayout(m_display_tab);
        verticalLayout_5->setObjectName("verticalLayout_5");
        m_table_name_le = new QLineEdit(m_display_tab);
        m_table_name_le->setObjectName("m_table_name_le");

        verticalLayout_5->addWidget(m_table_name_le);

        groupBox = new QGroupBox(m_display_tab);
        groupBox->setObjectName("groupBox");
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        m_display_n_row_sb = new QSpinBox(groupBox);
        m_display_n_row_sb->setObjectName("m_display_n_row_sb");
        m_display_n_row_sb->setMaximum(999);

        gridLayout_2->addWidget(m_display_n_row_sb, 1, 4, 1, 1);

        m_next_pb = new QPushButton(groupBox);
        m_next_pb->setObjectName("m_next_pb");
        m_next_pb->setEnabled(true);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/arrow-right.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_next_pb->setIcon(icon);
        m_next_pb->setFlat(true);

        gridLayout_2->addWidget(m_next_pb, 2, 6, 1, 1);

        m_previous_table_cb = new QComboBox(groupBox);
        m_previous_table_cb->addItem(QString());
        m_previous_table_cb->setObjectName("m_previous_table_cb");
        m_previous_table_cb->setInsertPolicy(QComboBox::InsertAtBottom);

        gridLayout_2->addWidget(m_previous_table_cb, 2, 4, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer, 1, 0, 1, 1);

        m_auto_geometry_pb = new QPushButton(groupBox);
        m_auto_geometry_pb->setObjectName("m_auto_geometry_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/22x22/zoom-fit-best.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_auto_geometry_pb->setIcon(icon1);

        gridLayout_2->addWidget(m_auto_geometry_pb, 1, 5, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_2, 2, 7, 1, 1);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName("label_7");
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_7, 2, 1, 1, 1);

        label_8 = new QLabel(groupBox);
        label_8->setObjectName("label_8");
        label_8->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_8, 1, 1, 1, 1);

        m_x_pos = new QSpinBox(groupBox);
        m_x_pos->setObjectName("m_x_pos");
        m_x_pos->setMaximum(10000);

        gridLayout_2->addWidget(m_x_pos, 1, 2, 1, 1);

        widget_3 = new QWidget(groupBox);
        widget_3->setObjectName("widget_3");
        horizontalLayout_3 = new QHBoxLayout(widget_3);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);

        gridLayout_2->addWidget(widget_3, 4, 1, 1, 6);

        m_previous_pb = new QPushButton(groupBox);
        m_previous_pb->setObjectName("m_previous_pb");
        m_previous_pb->setEnabled(true);
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/arrow-left.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_previous_pb->setIcon(icon2);
        m_previous_pb->setFlat(true);

        gridLayout_2->addWidget(m_previous_pb, 2, 5, 1, 1);

        m_y_pos = new QSpinBox(groupBox);
        m_y_pos->setObjectName("m_y_pos");
        m_y_pos->setMaximum(10000);

        gridLayout_2->addWidget(m_y_pos, 2, 2, 1, 1);

        m_apply_geometry_to_linked_table_pb = new QPushButton(groupBox);
        m_apply_geometry_to_linked_table_pb->setObjectName("m_apply_geometry_to_linked_table_pb");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/22x22/all_pages.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_apply_geometry_to_linked_table_pb->setIcon(icon3);

        gridLayout_2->addWidget(m_apply_geometry_to_linked_table_pb, 1, 6, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_3, 2, 3, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label, 1, 3, 1, 1);

        m_info_label = new QLabel(groupBox);
        m_info_label->setObjectName("m_info_label");
        m_info_label->setTextFormat(Qt::RichText);
        m_info_label->setScaledContents(false);

        gridLayout_2->addWidget(m_info_label, 3, 1, 1, 6);


        verticalLayout_5->addWidget(groupBox);

        groupBox_2 = new QGroupBox(m_display_tab);
        groupBox_2->setObjectName("groupBox_2");
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setObjectName("verticalLayout_2");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        m_header_bottom_margin = new QSpinBox(groupBox_2);
        m_header_bottom_margin->setObjectName("m_header_bottom_margin");

        gridLayout->addWidget(m_header_bottom_margin, 3, 2, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_4, 2, 4, 1, 1);

        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label_2, 2, 2, 1, 1);

        m_header_right_margin = new QSpinBox(groupBox_2);
        m_header_right_margin->setObjectName("m_header_right_margin");

        gridLayout->addWidget(m_header_right_margin, 2, 3, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 2, 0, 1, 1);

        m_header_left_margin = new QSpinBox(groupBox_2);
        m_header_left_margin->setObjectName("m_header_left_margin");

        gridLayout->addWidget(m_header_left_margin, 2, 1, 1, 1);

        m_header_top_margin = new QSpinBox(groupBox_2);
        m_header_top_margin->setObjectName("m_header_top_margin");

        gridLayout->addWidget(m_header_top_margin, 1, 2, 1, 1);


        verticalLayout_2->addLayout(gridLayout);

        widget = new QWidget(groupBox_2);
        widget->setObjectName("widget");
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_7);

        label_5 = new QLabel(widget);
        label_5->setObjectName("label_5");

        horizontalLayout->addWidget(label_5);

        m_header_align_left_rb = new QRadioButton(widget);
        m_header_align_left_rb->setObjectName("m_header_align_left_rb");

        horizontalLayout->addWidget(m_header_align_left_rb);

        m_header_align_center_rb = new QRadioButton(widget);
        m_header_align_center_rb->setObjectName("m_header_align_center_rb");

        horizontalLayout->addWidget(m_header_align_center_rb);

        m_header_align_right_rb = new QRadioButton(widget);
        m_header_align_right_rb->setObjectName("m_header_align_right_rb");

        horizontalLayout->addWidget(m_header_align_right_rb);

        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);


        verticalLayout_2->addWidget(widget);

        m_header_font_pb = new QPushButton(groupBox_2);
        m_header_font_pb->setObjectName("m_header_font_pb");
        m_header_font_pb->setFlat(false);

        verticalLayout_2->addWidget(m_header_font_pb);


        verticalLayout_5->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(m_display_tab);
        groupBox_3->setObjectName("groupBox_3");
        verticalLayout_3 = new QVBoxLayout(groupBox_3);
        verticalLayout_3->setObjectName("verticalLayout_3");
        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName("gridLayout_3");
        m_table_bottom_margin = new QSpinBox(groupBox_3);
        m_table_bottom_margin->setObjectName("m_table_bottom_margin");

        gridLayout_3->addWidget(m_table_bottom_margin, 2, 2, 1, 1);

        m_table_left_margin = new QSpinBox(groupBox_3);
        m_table_left_margin->setObjectName("m_table_left_margin");

        gridLayout_3->addWidget(m_table_left_margin, 1, 1, 1, 1);

        m_table_top_margin = new QSpinBox(groupBox_3);
        m_table_top_margin->setObjectName("m_table_top_margin");

        gridLayout_3->addWidget(m_table_top_margin, 0, 2, 1, 1);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_6, 1, 4, 1, 1);

        m_table_right_margin = new QSpinBox(groupBox_3);
        m_table_right_margin->setObjectName("m_table_right_margin");

        gridLayout_3->addWidget(m_table_right_margin, 1, 3, 1, 1);

        label_4 = new QLabel(groupBox_3);
        label_4->setObjectName("label_4");
        label_4->setAlignment(Qt::AlignCenter);

        gridLayout_3->addWidget(label_4, 1, 2, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_5, 1, 0, 1, 1);


        verticalLayout_3->addLayout(gridLayout_3);

        widget_2 = new QWidget(groupBox_3);
        widget_2->setObjectName("widget_2");
        horizontalLayout_2 = new QHBoxLayout(widget_2);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_9);

        label_6 = new QLabel(widget_2);
        label_6->setObjectName("label_6");

        horizontalLayout_2->addWidget(label_6);

        m_table_align_left_rb = new QRadioButton(widget_2);
        m_table_align_left_rb->setObjectName("m_table_align_left_rb");

        horizontalLayout_2->addWidget(m_table_align_left_rb);

        m_table_align_center_rb = new QRadioButton(widget_2);
        m_table_align_center_rb->setObjectName("m_table_align_center_rb");

        horizontalLayout_2->addWidget(m_table_align_center_rb);

        m_table_align_right_rb = new QRadioButton(widget_2);
        m_table_align_right_rb->setObjectName("m_table_align_right_rb");

        horizontalLayout_2->addWidget(m_table_align_right_rb);

        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_10);


        verticalLayout_3->addWidget(widget_2);

        m_table_font_pb = new QPushButton(groupBox_3);
        m_table_font_pb->setObjectName("m_table_font_pb");
        m_table_font_pb->setFlat(false);

        verticalLayout_3->addWidget(m_table_font_pb);


        verticalLayout_5->addWidget(groupBox_3);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);

        m_tab->addTab(m_display_tab, QString());
        m_content_tab = new QWidget();
        m_content_tab->setObjectName("m_content_tab");
        m_content_layout = new QVBoxLayout(m_content_tab);
        m_content_layout->setObjectName("m_content_layout");
        verticalSpacer_2 = new QSpacerItem(20, 534, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        m_content_layout->addItem(verticalSpacer_2);

        m_tab->addTab(m_content_tab, QString());

        verticalLayout->addWidget(m_tab);


        retranslateUi(GraphicsTablePropertiesEditor);

        m_tab->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(GraphicsTablePropertiesEditor);
    } // setupUi

    void retranslateUi(QWidget *GraphicsTablePropertiesEditor)
    {
        GraphicsTablePropertiesEditor->setWindowTitle(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Form", nullptr));
        m_table_name_le->setText(QString());
        m_table_name_le->setPlaceholderText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Nom du tableau", nullptr));
        groupBox->setTitle(QCoreApplication::translate("GraphicsTablePropertiesEditor", "G\303\251ometrie et lignes", nullptr));
        m_display_n_row_sb->setSpecialValueText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Toutes", nullptr));
#if QT_CONFIG(tooltip)
        m_next_pb->setToolTip(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Tableau suivant", nullptr));
#endif // QT_CONFIG(tooltip)
        m_next_pb->setText(QString());
        m_previous_table_cb->setItemText(0, QCoreApplication::translate("GraphicsTablePropertiesEditor", "Aucun", nullptr));

#if QT_CONFIG(tooltip)
        m_auto_geometry_pb->setToolTip(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Ajuster le tableau au folio", nullptr));
#endif // QT_CONFIG(tooltip)
        m_auto_geometry_pb->setText(QString());
        label_7->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Y :", nullptr));
        label_8->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "X :", nullptr));
#if QT_CONFIG(tooltip)
        m_previous_pb->setToolTip(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Tableau pr\303\251c\303\251dent", nullptr));
#endif // QT_CONFIG(tooltip)
        m_previous_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_apply_geometry_to_linked_table_pb->setToolTip(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Appliquer la g\303\251ometrie \303\240 tous les tableaux li\303\251e \303\240 celui-ci", nullptr));
#endif // QT_CONFIG(tooltip)
        m_apply_geometry_to_linked_table_pb->setText(QString());
        label_3->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Tableau pr\303\251c\303\251dent :", nullptr));
        label->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Lignes \303\240 afficher :", nullptr));
        m_info_label->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "TextLabel", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("GraphicsTablePropertiesEditor", "En t\303\252te", nullptr));
        label_2->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Marge", nullptr));
        m_header_left_margin->setSuffix(QString());
        m_header_left_margin->setPrefix(QString());
        label_5->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Aligement :", nullptr));
        m_header_align_left_rb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Gauche", nullptr));
        m_header_align_center_rb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Centr\303\251", nullptr));
        m_header_align_right_rb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Droite", nullptr));
        m_header_font_pb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Police", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Tableau", nullptr));
        label_4->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Marge", nullptr));
        label_6->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Alignement :", nullptr));
        m_table_align_left_rb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Gauche", nullptr));
        m_table_align_center_rb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Centr\303\251", nullptr));
        m_table_align_right_rb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Droite", nullptr));
        m_table_font_pb->setText(QCoreApplication::translate("GraphicsTablePropertiesEditor", "Police", nullptr));
        m_tab->setTabText(m_tab->indexOf(m_display_tab), QCoreApplication::translate("GraphicsTablePropertiesEditor", "Affichage", nullptr));
        m_tab->setTabText(m_tab->indexOf(m_content_tab), QCoreApplication::translate("GraphicsTablePropertiesEditor", "Contenu", nullptr));
    } // retranslateUi

};

namespace Ui {
    class GraphicsTablePropertiesEditor: public Ui_GraphicsTablePropertiesEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GRAPHICSTABLEPROPERTIESEDITOR_H
