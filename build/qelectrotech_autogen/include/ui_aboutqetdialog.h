/********************************************************************************
** Form generated from reading UI file 'aboutqetdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTQETDIALOG_H
#define UI_ABOUTQETDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AboutQETDialog
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QLabel *m_about_label;
    QWidget *tab_2;
    QHBoxLayout *horizontalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QLabel *m_author_label;
    QSpacerItem *horizontalSpacer_2;
    QWidget *tab_3;
    QVBoxLayout *verticalLayout_4;
    QScrollArea *scrollArea_2;
    QWidget *scrollAreaWidgetContents_2;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_3;
    QLabel *m_translators_label;
    QSpacerItem *horizontalSpacer_4;
    QWidget *tab_4;
    QVBoxLayout *verticalLayout_5;
    QScrollArea *scrollArea_3;
    QWidget *scrollAreaWidgetContents_3;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_5;
    QLabel *m_contrib_label;
    QSpacerItem *horizontalSpacer_6;
    QWidget *tab_5;
    QHBoxLayout *horizontalLayout_6;
    QSpacerItem *horizontalSpacer_9;
    QLabel *m_version_label;
    QSpacerItem *horizontalSpacer_10;
    QWidget *m_annex_project_tab;
    QVBoxLayout *verticalLayout_6;
    QScrollArea *scrollArea_5;
    QWidget *scrollAreaWidgetContents_5;
    QHBoxLayout *horizontalLayout_7;
    QSpacerItem *horizontalSpacer_11;
    QLabel *m_annex_project_label;
    QSpacerItem *horizontalSpacer_12;
    QWidget *tab_7;
    QVBoxLayout *verticalLayout_7;
    QScrollArea *scrollArea_4;
    QWidget *scrollAreaWidgetContents_4;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_7;
    QLabel *m_libraries_label;
    QSpacerItem *horizontalSpacer_8;
    QWidget *tab_9;
    QVBoxLayout *verticalLayout_9;
    QComboBox *m_licenses_comboBox;
    QPlainTextEdit *m_licenses_notice_plainTextEdit;
    QPlainTextEdit *m_licenses_license_plainTextEdit;
    QWidget *tab_8;
    QVBoxLayout *verticalLayout_3;
    QComboBox *m_log_comboBox;
    QTextEdit *m_log_textEdit;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AboutQETDialog)
    {
        if (AboutQETDialog->objectName().isEmpty())
            AboutQETDialog->setObjectName("AboutQETDialog");
        AboutQETDialog->resize(700, 500);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(AboutQETDialog->sizePolicy().hasHeightForWidth());
        AboutQETDialog->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(AboutQETDialog);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(AboutQETDialog);
        tabWidget->setObjectName("tabWidget");
        tab = new QWidget();
        tab->setObjectName("tab");
        verticalLayout_2 = new QVBoxLayout(tab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        m_about_label = new QLabel(tab);
        m_about_label->setObjectName("m_about_label");
        m_about_label->setTextFormat(Qt::RichText);
        m_about_label->setAlignment(Qt::AlignCenter);
        m_about_label->setOpenExternalLinks(true);

        verticalLayout_2->addWidget(m_about_label);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        horizontalLayout = new QHBoxLayout(tab_2);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        scrollArea = new QScrollArea(tab_2);
        scrollArea->setObjectName("scrollArea");
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 676, 420));
        horizontalLayout_2 = new QHBoxLayout(scrollAreaWidgetContents);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        m_author_label = new QLabel(scrollAreaWidgetContents);
        m_author_label->setObjectName("m_author_label");
        m_author_label->setTextFormat(Qt::RichText);
        m_author_label->setOpenExternalLinks(true);

        horizontalLayout_2->addWidget(m_author_label);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        scrollArea->setWidget(scrollAreaWidgetContents);

        horizontalLayout->addWidget(scrollArea);

        tabWidget->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName("tab_3");
        verticalLayout_4 = new QVBoxLayout(tab_3);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        scrollArea_2 = new QScrollArea(tab_3);
        scrollArea_2->setObjectName("scrollArea_2");
        scrollArea_2->setWidgetResizable(true);
        scrollAreaWidgetContents_2 = new QWidget();
        scrollAreaWidgetContents_2->setObjectName("scrollAreaWidgetContents_2");
        scrollAreaWidgetContents_2->setGeometry(QRect(0, 0, 676, 420));
        horizontalLayout_3 = new QHBoxLayout(scrollAreaWidgetContents_2);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);

        m_translators_label = new QLabel(scrollAreaWidgetContents_2);
        m_translators_label->setObjectName("m_translators_label");
        m_translators_label->setTextFormat(Qt::RichText);
        m_translators_label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        m_translators_label->setOpenExternalLinks(true);

        horizontalLayout_3->addWidget(m_translators_label);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_4);

        scrollArea_2->setWidget(scrollAreaWidgetContents_2);

        verticalLayout_4->addWidget(scrollArea_2);

        tabWidget->addTab(tab_3, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName("tab_4");
        verticalLayout_5 = new QVBoxLayout(tab_4);
        verticalLayout_5->setSpacing(0);
        verticalLayout_5->setObjectName("verticalLayout_5");
        verticalLayout_5->setContentsMargins(0, 0, 0, 0);
        scrollArea_3 = new QScrollArea(tab_4);
        scrollArea_3->setObjectName("scrollArea_3");
        scrollArea_3->setWidgetResizable(true);
        scrollAreaWidgetContents_3 = new QWidget();
        scrollAreaWidgetContents_3->setObjectName("scrollAreaWidgetContents_3");
        scrollAreaWidgetContents_3->setGeometry(QRect(0, 0, 676, 420));
        horizontalLayout_4 = new QHBoxLayout(scrollAreaWidgetContents_3);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_5);

        m_contrib_label = new QLabel(scrollAreaWidgetContents_3);
        m_contrib_label->setObjectName("m_contrib_label");
        m_contrib_label->setTextFormat(Qt::RichText);
        m_contrib_label->setOpenExternalLinks(true);

        horizontalLayout_4->addWidget(m_contrib_label);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_6);

        scrollArea_3->setWidget(scrollAreaWidgetContents_3);

        verticalLayout_5->addWidget(scrollArea_3);

        tabWidget->addTab(tab_4, QString());
        tab_5 = new QWidget();
        tab_5->setObjectName("tab_5");
        horizontalLayout_6 = new QHBoxLayout(tab_5);
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_9);

        m_version_label = new QLabel(tab_5);
        m_version_label->setObjectName("m_version_label");
        m_version_label->setTextFormat(Qt::RichText);
        m_version_label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        m_version_label->setOpenExternalLinks(false);
        m_version_label->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        horizontalLayout_6->addWidget(m_version_label);

        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_10);

        tabWidget->addTab(tab_5, QString());
        m_annex_project_tab = new QWidget();
        m_annex_project_tab->setObjectName("m_annex_project_tab");
        verticalLayout_6 = new QVBoxLayout(m_annex_project_tab);
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setObjectName("verticalLayout_6");
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        scrollArea_5 = new QScrollArea(m_annex_project_tab);
        scrollArea_5->setObjectName("scrollArea_5");
        scrollArea_5->setWidgetResizable(true);
        scrollAreaWidgetContents_5 = new QWidget();
        scrollAreaWidgetContents_5->setObjectName("scrollAreaWidgetContents_5");
        scrollAreaWidgetContents_5->setGeometry(QRect(0, 0, 676, 420));
        horizontalLayout_7 = new QHBoxLayout(scrollAreaWidgetContents_5);
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        horizontalSpacer_11 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_11);

        m_annex_project_label = new QLabel(scrollAreaWidgetContents_5);
        m_annex_project_label->setObjectName("m_annex_project_label");
        m_annex_project_label->setTextFormat(Qt::RichText);
        m_annex_project_label->setOpenExternalLinks(true);

        horizontalLayout_7->addWidget(m_annex_project_label);

        horizontalSpacer_12 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_12);

        scrollArea_5->setWidget(scrollAreaWidgetContents_5);

        verticalLayout_6->addWidget(scrollArea_5);

        tabWidget->addTab(m_annex_project_tab, QString());
        tab_7 = new QWidget();
        tab_7->setObjectName("tab_7");
        verticalLayout_7 = new QVBoxLayout(tab_7);
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName("verticalLayout_7");
        verticalLayout_7->setContentsMargins(0, 0, 0, 0);
        scrollArea_4 = new QScrollArea(tab_7);
        scrollArea_4->setObjectName("scrollArea_4");
        scrollArea_4->setWidgetResizable(true);
        scrollAreaWidgetContents_4 = new QWidget();
        scrollAreaWidgetContents_4->setObjectName("scrollAreaWidgetContents_4");
        scrollAreaWidgetContents_4->setGeometry(QRect(0, 0, 676, 420));
        horizontalLayout_5 = new QHBoxLayout(scrollAreaWidgetContents_4);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_7);

        m_libraries_label = new QLabel(scrollAreaWidgetContents_4);
        m_libraries_label->setObjectName("m_libraries_label");
        m_libraries_label->setTextFormat(Qt::RichText);
        m_libraries_label->setOpenExternalLinks(true);

        horizontalLayout_5->addWidget(m_libraries_label);

        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_8);

        scrollArea_4->setWidget(scrollAreaWidgetContents_4);

        verticalLayout_7->addWidget(scrollArea_4);

        tabWidget->addTab(tab_7, QString());
        tab_9 = new QWidget();
        tab_9->setObjectName("tab_9");
        verticalLayout_9 = new QVBoxLayout(tab_9);
        verticalLayout_9->setObjectName("verticalLayout_9");
        m_licenses_comboBox = new QComboBox(tab_9);
        m_licenses_comboBox->setObjectName("m_licenses_comboBox");

        verticalLayout_9->addWidget(m_licenses_comboBox);

        m_licenses_notice_plainTextEdit = new QPlainTextEdit(tab_9);
        m_licenses_notice_plainTextEdit->setObjectName("m_licenses_notice_plainTextEdit");
        m_licenses_notice_plainTextEdit->setReadOnly(true);

        verticalLayout_9->addWidget(m_licenses_notice_plainTextEdit);

        m_licenses_license_plainTextEdit = new QPlainTextEdit(tab_9);
        m_licenses_license_plainTextEdit->setObjectName("m_licenses_license_plainTextEdit");
        m_licenses_license_plainTextEdit->setReadOnly(true);

        verticalLayout_9->addWidget(m_licenses_license_plainTextEdit);

        tabWidget->addTab(tab_9, QString());
        tab_8 = new QWidget();
        tab_8->setObjectName("tab_8");
        verticalLayout_3 = new QVBoxLayout(tab_8);
        verticalLayout_3->setObjectName("verticalLayout_3");
        m_log_comboBox = new QComboBox(tab_8);
        m_log_comboBox->setObjectName("m_log_comboBox");

        verticalLayout_3->addWidget(m_log_comboBox);

        m_log_textEdit = new QTextEdit(tab_8);
        m_log_textEdit->setObjectName("m_log_textEdit");

        verticalLayout_3->addWidget(m_log_textEdit);

        tabWidget->addTab(tab_8, QString());

        verticalLayout->addWidget(tabWidget);

        buttonBox = new QDialogButtonBox(AboutQETDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(AboutQETDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, AboutQETDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, AboutQETDialog, qOverload<>(&QDialog::reject));

        tabWidget->setCurrentIndex(4);


        QMetaObject::connectSlotsByName(AboutQETDialog);
    } // setupUi

    void retranslateUi(QDialog *AboutQETDialog)
    {
        AboutQETDialog->setWindowTitle(QCoreApplication::translate("AboutQETDialog", "\303\200 propos de QElectrotech", nullptr));
        m_about_label->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("AboutQETDialog", "\303\200 propos", nullptr));
        m_author_label->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("AboutQETDialog", "Auteurs", nullptr));
        m_translators_label->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QCoreApplication::translate("AboutQETDialog", "Traducteurs", nullptr));
        m_contrib_label->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tab_4), QCoreApplication::translate("AboutQETDialog", "Contributeurs", nullptr));
        m_version_label->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tab_5), QCoreApplication::translate("AboutQETDialog", "Version", nullptr));
        m_annex_project_label->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(m_annex_project_tab), QCoreApplication::translate("AboutQETDialog", "Projet annexe", nullptr));
        m_libraries_label->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tab_7), QCoreApplication::translate("AboutQETDialog", "Biblioth\303\250ques", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_9), QCoreApplication::translate("AboutQETDialog", "Licenses", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_8), QCoreApplication::translate("AboutQETDialog", "log", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AboutQETDialog: public Ui_AboutQETDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTQETDIALOG_H
