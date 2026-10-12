/********************************************************************************
** Form generated from reading UI file 'summaryquerywidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SUMMARYQUERYWIDGET_H
#define UI_SUMMARYQUERYWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "configsaveloaderwidget.h"

QT_BEGIN_NAMESPACE

class Ui_SummaryQueryWidget
{
public:
    QVBoxLayout *verticalLayout_2;
    QWidget *m_info_widget;
    QGridLayout *gridLayout_2;
    QListWidget *m_choosen_list;
    QLabel *label;
    QListWidget *m_available_list;
    QLabel *label_2;
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer;
    QPushButton *m_up_pb;
    QPushButton *m_add_pb;
    QPushButton *m_remove_pb;
    QPushButton *m_down_pb;
    QSpacerItem *verticalSpacer_2;
    QFrame *line;
    ConfigSaveLoaderWidget *m_config_gb;
    QFrame *line_2;
    QHBoxLayout *horizontalLayout;
    QCheckBox *m_edit_sql_query_cb;
    QLineEdit *m_user_query_le;

    void setupUi(QWidget *SummaryQueryWidget)
    {
        if (SummaryQueryWidget->objectName().isEmpty())
            SummaryQueryWidget->setObjectName("SummaryQueryWidget");
        SummaryQueryWidget->resize(347, 277);
        verticalLayout_2 = new QVBoxLayout(SummaryQueryWidget);
        verticalLayout_2->setObjectName("verticalLayout_2");
        m_info_widget = new QWidget(SummaryQueryWidget);
        m_info_widget->setObjectName("m_info_widget");
        gridLayout_2 = new QGridLayout(m_info_widget);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        m_choosen_list = new QListWidget(m_info_widget);
        m_choosen_list->setObjectName("m_choosen_list");

        gridLayout_2->addWidget(m_choosen_list, 1, 2, 1, 1);

        label = new QLabel(m_info_widget);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignCenter);

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        m_available_list = new QListWidget(m_info_widget);
        m_available_list->setObjectName("m_available_list");

        gridLayout_2->addWidget(m_available_list, 1, 0, 1, 1);

        label_2 = new QLabel(m_info_widget);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignCenter);

        gridLayout_2->addWidget(label_2, 0, 2, 1, 1);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        m_up_pb = new QPushButton(m_info_widget);
        m_up_pb->setObjectName("m_up_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/go-up.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_up_pb->setIcon(icon);

        verticalLayout->addWidget(m_up_pb);

        m_add_pb = new QPushButton(m_info_widget);
        m_add_pb->setObjectName("m_add_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/list-add.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_add_pb->setIcon(icon1);

        verticalLayout->addWidget(m_add_pb);

        m_remove_pb = new QPushButton(m_info_widget);
        m_remove_pb->setObjectName("m_remove_pb");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/list-remove.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_remove_pb->setIcon(icon2);

        verticalLayout->addWidget(m_remove_pb);

        m_down_pb = new QPushButton(m_info_widget);
        m_down_pb->setObjectName("m_down_pb");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/go-down.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_down_pb->setIcon(icon3);

        verticalLayout->addWidget(m_down_pb);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);


        gridLayout_2->addLayout(verticalLayout, 1, 1, 1, 1);


        verticalLayout_2->addWidget(m_info_widget);

        line = new QFrame(SummaryQueryWidget);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout_2->addWidget(line);

        m_config_gb = new ConfigSaveLoaderWidget(SummaryQueryWidget);
        m_config_gb->setObjectName("m_config_gb");

        verticalLayout_2->addWidget(m_config_gb);

        line_2 = new QFrame(SummaryQueryWidget);
        line_2->setObjectName("line_2");
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout_2->addWidget(line_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        m_edit_sql_query_cb = new QCheckBox(SummaryQueryWidget);
        m_edit_sql_query_cb->setObjectName("m_edit_sql_query_cb");

        horizontalLayout->addWidget(m_edit_sql_query_cb);

        m_user_query_le = new QLineEdit(SummaryQueryWidget);
        m_user_query_le->setObjectName("m_user_query_le");
        m_user_query_le->setEnabled(false);

        horizontalLayout->addWidget(m_user_query_le);


        verticalLayout_2->addLayout(horizontalLayout);


        retranslateUi(SummaryQueryWidget);

        QMetaObject::connectSlotsByName(SummaryQueryWidget);
    } // setupUi

    void retranslateUi(QWidget *SummaryQueryWidget)
    {
        SummaryQueryWidget->setWindowTitle(QCoreApplication::translate("SummaryQueryWidget", "Form", nullptr));
        label->setText(QCoreApplication::translate("SummaryQueryWidget", "Informations disponibles", nullptr));
        label_2->setText(QCoreApplication::translate("SummaryQueryWidget", "Information \303\240 afficher", nullptr));
        m_up_pb->setText(QString());
        m_add_pb->setText(QString());
        m_remove_pb->setText(QString());
        m_down_pb->setText(QString());
        m_config_gb->setTitle(QCoreApplication::translate("SummaryQueryWidget", "Configuration", nullptr));
        m_edit_sql_query_cb->setText(QCoreApplication::translate("SummaryQueryWidget", "Requ\303\252te SQL :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SummaryQueryWidget: public Ui_SummaryQueryWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SUMMARYQUERYWIDGET_H
