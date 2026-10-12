/********************************************************************************
** Form generated from reading UI file 'elementquerywidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ELEMENTQUERYWIDGET_H
#define UI_ELEMENTQUERYWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ElementQueryWidget
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *m_info_widget;
    QGridLayout *gridLayout;
    QLabel *label_2;
    QLabel *label;
    QListWidget *m_var_list;
    QListWidget *m_choosen_list;
    QVBoxLayout *verticalLayout_2;
    QSpacerItem *verticalSpacer;
    QPushButton *m_up_pb;
    QPushButton *m_add_pb;
    QPushButton *m_remove_pb;
    QPushButton *m_down_pb;
    QSpacerItem *verticalSpacer_2;
    QFrame *line;
    QWidget *widget;
    QGridLayout *gridLayout_3;
    QComboBox *m_filter_type_cb;
    QLineEdit *m_filter_le;
    QLabel *label_4;
    QWidget *m_parametre_widget;
    QHBoxLayout *horizontalLayout_3;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_4;
    QCheckBox *m_all_cb;
    QCheckBox *m_simple_cb;
    QCheckBox *m_terminal_cb;
    QCheckBox *m_protection_cb;
    QCheckBox *m_coil_cb;
    QCheckBox *m_thumbnail_cb;
    QCheckBox *m_button_cb;
    QCheckBox *m_plc_cb;
    QFrame *line_4;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QPushButton *m_load_pb;
    QComboBox *m_conf_cb;
    QLineEdit *m_save_name_le;
    QPushButton *m_save_current_conf_pb;
    QFrame *line_3;
    QHBoxLayout *horizontalLayout;
    QCheckBox *m_edit_sql_query_cb;
    QLabel *label_3;
    QLineEdit *m_sql_query;

    void setupUi(QWidget *ElementQueryWidget)
    {
        if (ElementQueryWidget->objectName().isEmpty())
            ElementQueryWidget->setObjectName("ElementQueryWidget");
        ElementQueryWidget->resize(738, 534);
        verticalLayout = new QVBoxLayout(ElementQueryWidget);
        verticalLayout->setObjectName("verticalLayout");
        m_info_widget = new QWidget(ElementQueryWidget);
        m_info_widget->setObjectName("m_info_widget");
        gridLayout = new QGridLayout(m_info_widget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(0, 0, 0, 0);
        label_2 = new QLabel(m_info_widget);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        label = new QLabel(m_info_widget);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignCenter);
        label->setMargin(0);

        gridLayout->addWidget(label, 0, 2, 1, 1);

        m_var_list = new QListWidget(m_info_widget);
        m_var_list->setObjectName("m_var_list");

        gridLayout->addWidget(m_var_list, 1, 0, 1, 1);

        m_choosen_list = new QListWidget(m_info_widget);
        m_choosen_list->setObjectName("m_choosen_list");

        gridLayout->addWidget(m_choosen_list, 1, 2, 1, 1);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setSizeConstraint(QLayout::SetDefaultConstraint);
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        m_up_pb = new QPushButton(m_info_widget);
        m_up_pb->setObjectName("m_up_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/go-up.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_up_pb->setIcon(icon);
        m_up_pb->setCheckable(false);

        verticalLayout_2->addWidget(m_up_pb);

        m_add_pb = new QPushButton(m_info_widget);
        m_add_pb->setObjectName("m_add_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/list-add.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_add_pb->setIcon(icon1);

        verticalLayout_2->addWidget(m_add_pb);

        m_remove_pb = new QPushButton(m_info_widget);
        m_remove_pb->setObjectName("m_remove_pb");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/list-remove.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_remove_pb->setIcon(icon2);

        verticalLayout_2->addWidget(m_remove_pb);

        m_down_pb = new QPushButton(m_info_widget);
        m_down_pb->setObjectName("m_down_pb");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/go-down.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_down_pb->setIcon(icon3);

        verticalLayout_2->addWidget(m_down_pb);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);


        gridLayout->addLayout(verticalLayout_2, 1, 1, 1, 1);


        verticalLayout->addWidget(m_info_widget);

        line = new QFrame(ElementQueryWidget);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout->addWidget(line);

        widget = new QWidget(ElementQueryWidget);
        widget->setObjectName("widget");
        gridLayout_3 = new QGridLayout(widget);
        gridLayout_3->setObjectName("gridLayout_3");
        m_filter_type_cb = new QComboBox(widget);
        m_filter_type_cb->addItem(QString());
        m_filter_type_cb->addItem(QString());
        m_filter_type_cb->addItem(QString());
        m_filter_type_cb->addItem(QString());
        m_filter_type_cb->addItem(QString());
        m_filter_type_cb->addItem(QString());
        m_filter_type_cb->addItem(QString());
        m_filter_type_cb->setObjectName("m_filter_type_cb");

        gridLayout_3->addWidget(m_filter_type_cb, 1, 1, 1, 1);

        m_filter_le = new QLineEdit(widget);
        m_filter_le->setObjectName("m_filter_le");

        gridLayout_3->addWidget(m_filter_le, 1, 2, 1, 1);

        label_4 = new QLabel(widget);
        label_4->setObjectName("label_4");

        gridLayout_3->addWidget(label_4, 1, 0, 1, 1);


        verticalLayout->addWidget(widget);

        m_parametre_widget = new QWidget(ElementQueryWidget);
        m_parametre_widget->setObjectName("m_parametre_widget");
        horizontalLayout_3 = new QHBoxLayout(m_parametre_widget);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        groupBox_3 = new QGroupBox(m_parametre_widget);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        gridLayout_4 = new QGridLayout(groupBox_3);
        gridLayout_4->setObjectName("gridLayout_4");
        m_all_cb = new QCheckBox(groupBox_3);
        m_all_cb->setObjectName("m_all_cb");
        m_all_cb->setChecked(true);

        gridLayout_4->addWidget(m_all_cb, 0, 0, 1, 1);

        m_simple_cb = new QCheckBox(groupBox_3);
        m_simple_cb->setObjectName("m_simple_cb");
        m_simple_cb->setChecked(true);

        gridLayout_4->addWidget(m_simple_cb, 0, 1, 1, 1);

        m_terminal_cb = new QCheckBox(groupBox_3);
        m_terminal_cb->setObjectName("m_terminal_cb");
        m_terminal_cb->setChecked(true);

        gridLayout_4->addWidget(m_terminal_cb, 1, 0, 1, 1);

        m_protection_cb = new QCheckBox(groupBox_3);
        m_protection_cb->setObjectName("m_protection_cb");
        m_protection_cb->setChecked(true);

        gridLayout_4->addWidget(m_protection_cb, 1, 1, 1, 1);

        m_coil_cb = new QCheckBox(groupBox_3);
        m_coil_cb->setObjectName("m_coil_cb");
        m_coil_cb->setChecked(true);

        gridLayout_4->addWidget(m_coil_cb, 2, 0, 1, 1);

        m_thumbnail_cb = new QCheckBox(groupBox_3);
        m_thumbnail_cb->setObjectName("m_thumbnail_cb");
        m_thumbnail_cb->setChecked(true);

        gridLayout_4->addWidget(m_thumbnail_cb, 2, 1, 1, 1);

        m_button_cb = new QCheckBox(groupBox_3);
        m_button_cb->setObjectName("m_button_cb");
        m_button_cb->setChecked(true);

        gridLayout_4->addWidget(m_button_cb, 3, 0, 1, 1);

        m_plc_cb = new QCheckBox(groupBox_3);
        m_plc_cb->setObjectName("m_plc_cb");
        m_plc_cb->setChecked(true);

        gridLayout_4->addWidget(m_plc_cb, 3, 1, 1, 1);


        horizontalLayout_3->addWidget(groupBox_3);


        verticalLayout->addWidget(m_parametre_widget);

        line_4 = new QFrame(ElementQueryWidget);
        line_4->setObjectName("line_4");
        line_4->setFrameShape(QFrame::Shape::HLine);
        line_4->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout->addWidget(line_4);

        groupBox = new QGroupBox(ElementQueryWidget);
        groupBox->setObjectName("groupBox");
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        m_load_pb = new QPushButton(groupBox);
        m_load_pb->setObjectName("m_load_pb");
        m_load_pb->setEnabled(true);
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/ico/16x16/folder-open.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_load_pb->setIcon(icon4);

        gridLayout_2->addWidget(m_load_pb, 0, 1, 1, 1);

        m_conf_cb = new QComboBox(groupBox);
        m_conf_cb->setObjectName("m_conf_cb");
        m_conf_cb->setEditable(false);

        gridLayout_2->addWidget(m_conf_cb, 0, 0, 1, 1);

        m_save_name_le = new QLineEdit(groupBox);
        m_save_name_le->setObjectName("m_save_name_le");

        gridLayout_2->addWidget(m_save_name_le, 1, 0, 1, 1);

        m_save_current_conf_pb = new QPushButton(groupBox);
        m_save_current_conf_pb->setObjectName("m_save_current_conf_pb");
        m_save_current_conf_pb->setEnabled(false);
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/ico/16x16/document-save.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_save_current_conf_pb->setIcon(icon5);
        m_save_current_conf_pb->setFlat(false);

        gridLayout_2->addWidget(m_save_current_conf_pb, 1, 1, 1, 1);


        verticalLayout->addWidget(groupBox);

        line_3 = new QFrame(ElementQueryWidget);
        line_3->setObjectName("line_3");
        line_3->setFrameShadow(QFrame::Sunken);
        line_3->setFrameShape(QFrame::Shape::HLine);

        verticalLayout->addWidget(line_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        m_edit_sql_query_cb = new QCheckBox(ElementQueryWidget);
        m_edit_sql_query_cb->setObjectName("m_edit_sql_query_cb");

        horizontalLayout->addWidget(m_edit_sql_query_cb);

        label_3 = new QLabel(ElementQueryWidget);
        label_3->setObjectName("label_3");

        horizontalLayout->addWidget(label_3);

        m_sql_query = new QLineEdit(ElementQueryWidget);
        m_sql_query->setObjectName("m_sql_query");
        m_sql_query->setEnabled(false);

        horizontalLayout->addWidget(m_sql_query);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(ElementQueryWidget);

        QMetaObject::connectSlotsByName(ElementQueryWidget);
    } // setupUi

    void retranslateUi(QWidget *ElementQueryWidget)
    {
        ElementQueryWidget->setWindowTitle(QCoreApplication::translate("ElementQueryWidget", "Form", nullptr));
        label_2->setText(QCoreApplication::translate("ElementQueryWidget", "Informations disponibles", nullptr));
        label->setText(QCoreApplication::translate("ElementQueryWidget", "Informations \303\240 exporter", nullptr));
#if QT_CONFIG(tooltip)
        m_up_pb->setToolTip(QCoreApplication::translate("ElementQueryWidget", "Monter la s\303\251lection", nullptr));
#endif // QT_CONFIG(tooltip)
        m_up_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_add_pb->setToolTip(QCoreApplication::translate("ElementQueryWidget", "Ajouter la s\303\251lection", nullptr));
#endif // QT_CONFIG(tooltip)
        m_add_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_remove_pb->setToolTip(QCoreApplication::translate("ElementQueryWidget", "Supprimer la s\303\251lection", nullptr));
#endif // QT_CONFIG(tooltip)
        m_remove_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_down_pb->setToolTip(QCoreApplication::translate("ElementQueryWidget", "Descendre la s\303\251lection", nullptr));
#endif // QT_CONFIG(tooltip)
        m_down_pb->setText(QString());
        m_filter_type_cb->setItemText(0, QCoreApplication::translate("ElementQueryWidget", "Pas de filtre", nullptr));
        m_filter_type_cb->setItemText(1, QCoreApplication::translate("ElementQueryWidget", "N'est pas vide", nullptr));
        m_filter_type_cb->setItemText(2, QCoreApplication::translate("ElementQueryWidget", "Est vide", nullptr));
        m_filter_type_cb->setItemText(3, QCoreApplication::translate("ElementQueryWidget", "Contient", nullptr));
        m_filter_type_cb->setItemText(4, QCoreApplication::translate("ElementQueryWidget", "Ne contient pas", nullptr));
        m_filter_type_cb->setItemText(5, QCoreApplication::translate("ElementQueryWidget", "Est \303\251gal \303\240", nullptr));
        m_filter_type_cb->setItemText(6, QCoreApplication::translate("ElementQueryWidget", "N'est pas \303\251gale \303\240", nullptr));

        label_4->setText(QCoreApplication::translate("ElementQueryWidget", "Filtre :", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("ElementQueryWidget", "Type d'\303\251l\303\251ments", nullptr));
        m_all_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Tous", nullptr));
        m_simple_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Simples", nullptr));
        m_terminal_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Borniers", nullptr));
        m_protection_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Organes de protection", nullptr));
        m_coil_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Contacteurs et relais", nullptr));
        m_thumbnail_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Vignettes", nullptr));
        m_button_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Boutons et commutateurs", nullptr));
        m_plc_cb->setText(QCoreApplication::translate("ElementQueryWidget", "Automates (MAE/SPS)", nullptr));
        groupBox->setTitle(QCoreApplication::translate("ElementQueryWidget", "Configuration", nullptr));
#if QT_CONFIG(tooltip)
        m_load_pb->setToolTip(QCoreApplication::translate("ElementQueryWidget", "Ouvrir la configuration s\303\251lectionn\303\251", nullptr));
#endif // QT_CONFIG(tooltip)
        m_load_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_save_current_conf_pb->setToolTip(QCoreApplication::translate("ElementQueryWidget", "Sauvegarder la configuration actuelle", nullptr));
#endif // QT_CONFIG(tooltip)
        m_save_current_conf_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_edit_sql_query_cb->setToolTip(QCoreApplication::translate("ElementQueryWidget", "Requ\303\252te SQL personnalis\303\251e", nullptr));
#endif // QT_CONFIG(tooltip)
        m_edit_sql_query_cb->setText(QString());
        label_3->setText(QCoreApplication::translate("ElementQueryWidget", "Requ\303\252te SQL :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ElementQueryWidget: public Ui_ElementQueryWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ELEMENTQUERYWIDGET_H
