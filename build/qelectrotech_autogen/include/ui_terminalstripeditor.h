/********************************************************************************
** Form generated from reading UI file 'terminalstripeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TERMINALSTRIPEDITOR_H
#define UI_TERMINALSTRIPEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
#include "kcolorcombo.h"

QT_BEGIN_NAMESPACE

class Ui_TerminalStripEditor
{
public:
    QHBoxLayout *horizontalLayout;
    QTabWidget *m_tab_widget;
    QWidget *m_layout_tab;
    QHBoxLayout *horizontalLayout_2;
    QTableView *m_table_widget;
    QWidget *widget;
    QGridLayout *gridLayout_2;
    QComboBox *m_led_cb;
    QFrame *line_3;
    QPushButton *m_move_to_pb;
    KColorCombo *m_bridge_color_cb;
    QLabel *label_6;
    QLabel *label_7;
    QLabel *label_10;
    QComboBox *m_type_cb;
    QLabel *label_9;
    QFrame *line_2;
    QSpacerItem *verticalSpacer;
    QLabel *label_8;
    QLabel *m_move_to_label;
    QComboBox *m_function_cb;
    QComboBox *m_move_to_cb;
    QSpinBox *m_level_sb;
    QPushButton *m_auto_ordering_pb;
    QPushButton *m_group_terminals_pb;
    QPushButton *m_ungroup_pb;
    QPushButton *m_bridge_terminals_pb;
    QPushButton *m_unbridge_terminals_pb;
    QWidget *m_data_tab;
    QGridLayout *gridLayout;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label;
    QLineEdit *m_location_le;
    QLineEdit *m_installation_le;
    QLineEdit *m_comment_le;
    QLabel *label_2;
    QLabel *label_5;
    QFrame *line;
    QLineEdit *m_name_le;
    QPlainTextEdit *m_description_te;

    void setupUi(QWidget *TerminalStripEditor)
    {
        if (TerminalStripEditor->objectName().isEmpty())
            TerminalStripEditor->setObjectName("TerminalStripEditor");
        TerminalStripEditor->resize(873, 483);
        horizontalLayout = new QHBoxLayout(TerminalStripEditor);
        horizontalLayout->setObjectName("horizontalLayout");
        m_tab_widget = new QTabWidget(TerminalStripEditor);
        m_tab_widget->setObjectName("m_tab_widget");
        m_layout_tab = new QWidget();
        m_layout_tab->setObjectName("m_layout_tab");
        horizontalLayout_2 = new QHBoxLayout(m_layout_tab);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        m_table_widget = new QTableView(m_layout_tab);
        m_table_widget->setObjectName("m_table_widget");

        horizontalLayout_2->addWidget(m_table_widget);

        widget = new QWidget(m_layout_tab);
        widget->setObjectName("widget");
        gridLayout_2 = new QGridLayout(widget);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setContentsMargins(-1, 0, -1, -1);
        m_led_cb = new QComboBox(widget);
        m_led_cb->addItem(QString());
        m_led_cb->addItem(QString());
        m_led_cb->setObjectName("m_led_cb");

        gridLayout_2->addWidget(m_led_cb, 8, 1, 1, 2);

        line_3 = new QFrame(widget);
        line_3->setObjectName("line_3");
        line_3->setFrameShape(QFrame::Shape::HLine);
        line_3->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout_2->addWidget(line_3, 1, 0, 1, 3);

        m_move_to_pb = new QPushButton(widget);
        m_move_to_pb->setObjectName("m_move_to_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/dialog-ok.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_move_to_pb->setIcon(icon);

        gridLayout_2->addWidget(m_move_to_pb, 0, 2, 1, 1);

        m_bridge_color_cb = new KColorCombo(widget);
        m_bridge_color_cb->setObjectName("m_bridge_color_cb");

        gridLayout_2->addWidget(m_bridge_color_cb, 12, 1, 1, 2);

        label_6 = new QLabel(widget);
        label_6->setObjectName("label_6");
        label_6->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_6, 5, 0, 1, 1);

        label_7 = new QLabel(widget);
        label_7->setObjectName("label_7");
        label_7->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_7, 6, 0, 1, 1);

        label_10 = new QLabel(widget);
        label_10->setObjectName("label_10");
        label_10->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_10, 12, 0, 1, 1);

        m_type_cb = new QComboBox(widget);
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->setObjectName("m_type_cb");

        gridLayout_2->addWidget(m_type_cb, 6, 1, 1, 2);

        label_9 = new QLabel(widget);
        label_9->setObjectName("label_9");
        label_9->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_9, 8, 0, 1, 1);

        line_2 = new QFrame(widget);
        line_2->setObjectName("line_2");
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout_2->addWidget(line_2, 9, 0, 1, 3);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 13, 0, 1, 1);

        label_8 = new QLabel(widget);
        label_8->setObjectName("label_8");
        label_8->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_8, 7, 0, 1, 1);

        m_move_to_label = new QLabel(widget);
        m_move_to_label->setObjectName("m_move_to_label");
        m_move_to_label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(m_move_to_label, 0, 0, 1, 1);

        m_function_cb = new QComboBox(widget);
        m_function_cb->addItem(QString());
        m_function_cb->addItem(QString());
        m_function_cb->addItem(QString());
        m_function_cb->setObjectName("m_function_cb");

        gridLayout_2->addWidget(m_function_cb, 7, 1, 1, 2);

        m_move_to_cb = new QComboBox(widget);
        m_move_to_cb->setObjectName("m_move_to_cb");
        m_move_to_cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_2->addWidget(m_move_to_cb, 0, 1, 1, 1);

        m_level_sb = new QSpinBox(widget);
        m_level_sb->setObjectName("m_level_sb");
        m_level_sb->setMaximum(6);

        gridLayout_2->addWidget(m_level_sb, 5, 1, 1, 2);

        m_auto_ordering_pb = new QPushButton(widget);
        m_auto_ordering_pb->setObjectName("m_auto_ordering_pb");

        gridLayout_2->addWidget(m_auto_ordering_pb, 2, 1, 1, 2);

        m_group_terminals_pb = new QPushButton(widget);
        m_group_terminals_pb->setObjectName("m_group_terminals_pb");

        gridLayout_2->addWidget(m_group_terminals_pb, 3, 1, 1, 2);

        m_ungroup_pb = new QPushButton(widget);
        m_ungroup_pb->setObjectName("m_ungroup_pb");

        gridLayout_2->addWidget(m_ungroup_pb, 4, 1, 1, 2);

        m_bridge_terminals_pb = new QPushButton(widget);
        m_bridge_terminals_pb->setObjectName("m_bridge_terminals_pb");

        gridLayout_2->addWidget(m_bridge_terminals_pb, 10, 1, 1, 2);

        m_unbridge_terminals_pb = new QPushButton(widget);
        m_unbridge_terminals_pb->setObjectName("m_unbridge_terminals_pb");

        gridLayout_2->addWidget(m_unbridge_terminals_pb, 11, 1, 1, 2);


        horizontalLayout_2->addWidget(widget);

        m_tab_widget->addTab(m_layout_tab, QString());
        m_data_tab = new QWidget();
        m_data_tab->setObjectName("m_data_tab");
        gridLayout = new QGridLayout(m_data_tab);
        gridLayout->setObjectName("gridLayout");
        label_3 = new QLabel(m_data_tab);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        label_4 = new QLabel(m_data_tab);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 3, 0, 1, 1);

        label = new QLabel(m_data_tab);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        m_location_le = new QLineEdit(m_data_tab);
        m_location_le->setObjectName("m_location_le");

        gridLayout->addWidget(m_location_le, 1, 1, 1, 1);

        m_installation_le = new QLineEdit(m_data_tab);
        m_installation_le->setObjectName("m_installation_le");

        gridLayout->addWidget(m_installation_le, 0, 1, 1, 1);

        m_comment_le = new QLineEdit(m_data_tab);
        m_comment_le->setObjectName("m_comment_le");

        gridLayout->addWidget(m_comment_le, 3, 1, 1, 1);

        label_2 = new QLabel(m_data_tab);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        label_5 = new QLabel(m_data_tab);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 4, 0, 1, 1);

        line = new QFrame(m_data_tab);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout->addWidget(line, 4, 1, 1, 1);

        m_name_le = new QLineEdit(m_data_tab);
        m_name_le->setObjectName("m_name_le");

        gridLayout->addWidget(m_name_le, 2, 1, 1, 1);

        m_description_te = new QPlainTextEdit(m_data_tab);
        m_description_te->setObjectName("m_description_te");

        gridLayout->addWidget(m_description_te, 5, 0, 1, 2);

        m_tab_widget->addTab(m_data_tab, QString());

        horizontalLayout->addWidget(m_tab_widget);


        retranslateUi(TerminalStripEditor);

        m_tab_widget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(TerminalStripEditor);
    } // setupUi

    void retranslateUi(QWidget *TerminalStripEditor)
    {
        TerminalStripEditor->setWindowTitle(QCoreApplication::translate("TerminalStripEditor", "Form", nullptr));
        m_led_cb->setItemText(0, QCoreApplication::translate("TerminalStripEditor", "Sans", nullptr));
        m_led_cb->setItemText(1, QCoreApplication::translate("TerminalStripEditor", "Avec", nullptr));

#if QT_CONFIG(tooltip)
        m_move_to_pb->setToolTip(QCoreApplication::translate("TerminalStripEditor", "Effectuer le d\303\251placement", nullptr));
#endif // QT_CONFIG(tooltip)
        m_move_to_pb->setText(QString());
        label_6->setText(QCoreApplication::translate("TerminalStripEditor", "\303\211tage :", nullptr));
        label_7->setText(QCoreApplication::translate("TerminalStripEditor", "Type :", nullptr));
        label_10->setText(QCoreApplication::translate("TerminalStripEditor", "Couleur pont :", nullptr));
        m_type_cb->setItemText(0, QCoreApplication::translate("TerminalStripEditor", "G\303\251n\303\251rique", nullptr));
        m_type_cb->setItemText(1, QCoreApplication::translate("TerminalStripEditor", "Fusible", nullptr));
        m_type_cb->setItemText(2, QCoreApplication::translate("TerminalStripEditor", "Sectionnable", nullptr));
        m_type_cb->setItemText(3, QCoreApplication::translate("TerminalStripEditor", "Diode", nullptr));
        m_type_cb->setItemText(4, QCoreApplication::translate("TerminalStripEditor", "Terre", nullptr));

        label_9->setText(QCoreApplication::translate("TerminalStripEditor", "LED :", nullptr));
        label_8->setText(QCoreApplication::translate("TerminalStripEditor", "Fonction :", nullptr));
        m_move_to_label->setText(QCoreApplication::translate("TerminalStripEditor", "D\303\251placer dans :", nullptr));
        m_function_cb->setItemText(0, QCoreApplication::translate("TerminalStripEditor", "G\303\251n\303\251rique", nullptr));
        m_function_cb->setItemText(1, QCoreApplication::translate("TerminalStripEditor", "Phase", nullptr));
        m_function_cb->setItemText(2, QCoreApplication::translate("TerminalStripEditor", "Neutre", nullptr));

        m_auto_ordering_pb->setText(QCoreApplication::translate("TerminalStripEditor", "Position automatique", nullptr));
        m_group_terminals_pb->setText(QCoreApplication::translate("TerminalStripEditor", "Grouper les bornes", nullptr));
        m_ungroup_pb->setText(QCoreApplication::translate("TerminalStripEditor", "Degrouper les bornes", nullptr));
        m_bridge_terminals_pb->setText(QCoreApplication::translate("TerminalStripEditor", "Ponter les bornes", nullptr));
        m_unbridge_terminals_pb->setText(QCoreApplication::translate("TerminalStripEditor", "D\303\251ponter les bornes", nullptr));
        m_tab_widget->setTabText(m_tab_widget->indexOf(m_layout_tab), QCoreApplication::translate("TerminalStripEditor", "Disposition", nullptr));
        label_3->setText(QCoreApplication::translate("TerminalStripEditor", "Nom :", nullptr));
        label_4->setText(QCoreApplication::translate("TerminalStripEditor", "Commentaire :", nullptr));
        label->setText(QCoreApplication::translate("TerminalStripEditor", "Installation :", nullptr));
        label_2->setText(QCoreApplication::translate("TerminalStripEditor", "Localisation :", nullptr));
        label_5->setText(QCoreApplication::translate("TerminalStripEditor", "Description", nullptr));
        m_tab_widget->setTabText(m_tab_widget->indexOf(m_data_tab), QCoreApplication::translate("TerminalStripEditor", "Propri\303\251t\303\251s", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TerminalStripEditor: public Ui_TerminalStripEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TERMINALSTRIPEDITOR_H
