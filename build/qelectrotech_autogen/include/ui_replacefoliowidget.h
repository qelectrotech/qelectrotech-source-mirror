/********************************************************************************
** Form generated from reading UI file 'replacefoliowidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPLACEFOLIOWIDGET_H
#define UI_REPLACEFOLIOWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ReplaceFolioWidget
{
public:
    QVBoxLayout *verticalLayout_3;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QGridLayout *gridLayout_4;
    QLabel *label_12;
    QLabel *label_11;
    QLabel *label_5;
    QLineEdit *m_title_le;
    QLineEdit *m_author_le;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_10;
    QLineEdit *m_indice;
    QLineEdit *m_file_le;
    QGridLayout *gridLayout;
    QLabel *label_6;
    QGridLayout *gridLayout_2;
    QRadioButton *m_no_date_rb;
    QDateEdit *m_date_edit;
    QRadioButton *m_fixed_date_rb;
    QPushButton *m_date_now_pb;
    QRadioButton *m_unchanged_date;
    QLineEdit *m_folio_le;
    QLineEdit *m_loc;
    QLineEdit *m_plant;
    QLabel *label_2;
    QCheckBox *m_title_cb;
    QCheckBox *m_author_cb;
    QCheckBox *m_file_cb;
    QCheckBox *m_folio_cb;
    QCheckBox *m_plant_cb;
    QCheckBox *m_loc_cb;
    QCheckBox *m_indice_cb;
    QSpacerItem *verticalSpacer;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout;
    QVBoxLayout *m_tab2_vlayout;
    QLabel *label_8;

    void setupUi(QWidget *ReplaceFolioWidget)
    {
        if (ReplaceFolioWidget->objectName().isEmpty())
            ReplaceFolioWidget->setObjectName("ReplaceFolioWidget");
        ReplaceFolioWidget->resize(744, 705);
        verticalLayout_3 = new QVBoxLayout(ReplaceFolioWidget);
        verticalLayout_3->setObjectName("verticalLayout_3");
        tabWidget = new QTabWidget(ReplaceFolioWidget);
        tabWidget->setObjectName("tabWidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy);
        tabWidget->setTabPosition(QTabWidget::South);
        tabWidget->setTabShape(QTabWidget::Rounded);
        tabWidget->setElideMode(Qt::ElideNone);
        tabWidget->setUsesScrollButtons(true);
        tabWidget->setTabsClosable(false);
        tab = new QWidget();
        tab->setObjectName("tab");
        verticalLayout_2 = new QVBoxLayout(tab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        gridLayout_4 = new QGridLayout();
        gridLayout_4->setObjectName("gridLayout_4");
        label_12 = new QLabel(tab);
        label_12->setObjectName("label_12");

        gridLayout_4->addWidget(label_12, 9, 1, 1, 1);

        label_11 = new QLabel(tab);
        label_11->setObjectName("label_11");

        gridLayout_4->addWidget(label_11, 8, 1, 1, 1);

        label_5 = new QLabel(tab);
        label_5->setObjectName("label_5");

        gridLayout_4->addWidget(label_5, 4, 1, 1, 1);

        m_title_le = new QLineEdit(tab);
        m_title_le->setObjectName("m_title_le");

        gridLayout_4->addWidget(m_title_le, 1, 3, 1, 1);

        m_author_le = new QLineEdit(tab);
        m_author_le->setObjectName("m_author_le");

        gridLayout_4->addWidget(m_author_le, 2, 3, 1, 1);

        label_3 = new QLabel(tab);
        label_3->setObjectName("label_3");

        gridLayout_4->addWidget(label_3, 2, 1, 1, 1);

        label_4 = new QLabel(tab);
        label_4->setObjectName("label_4");
        label_4->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout_4->addWidget(label_4, 3, 1, 1, 1);

        label_10 = new QLabel(tab);
        label_10->setObjectName("label_10");

        gridLayout_4->addWidget(label_10, 6, 1, 1, 1);

        m_indice = new QLineEdit(tab);
        m_indice->setObjectName("m_indice");

        gridLayout_4->addWidget(m_indice, 9, 3, 1, 1);

        m_file_le = new QLineEdit(tab);
        m_file_le->setObjectName("m_file_le");

        gridLayout_4->addWidget(m_file_le, 4, 3, 1, 1);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");

        gridLayout_4->addLayout(gridLayout, 10, 3, 1, 1);

        label_6 = new QLabel(tab);
        label_6->setObjectName("label_6");

        gridLayout_4->addWidget(label_6, 5, 1, 1, 1);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName("gridLayout_2");
        m_no_date_rb = new QRadioButton(tab);
        m_no_date_rb->setObjectName("m_no_date_rb");

        gridLayout_2->addWidget(m_no_date_rb, 1, 0, 1, 1);

        m_date_edit = new QDateEdit(tab);
        m_date_edit->setObjectName("m_date_edit");
        m_date_edit->setFrame(true);
        m_date_edit->setCalendarPopup(true);

        gridLayout_2->addWidget(m_date_edit, 2, 1, 1, 1);

        m_fixed_date_rb = new QRadioButton(tab);
        m_fixed_date_rb->setObjectName("m_fixed_date_rb");

        gridLayout_2->addWidget(m_fixed_date_rb, 2, 0, 1, 1);

        m_date_now_pb = new QPushButton(tab);
        m_date_now_pb->setObjectName("m_date_now_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/22x22/start.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_date_now_pb->setIcon(icon);

        gridLayout_2->addWidget(m_date_now_pb, 2, 2, 1, 1);

        m_unchanged_date = new QRadioButton(tab);
        m_unchanged_date->setObjectName("m_unchanged_date");

        gridLayout_2->addWidget(m_unchanged_date, 0, 0, 1, 1);


        gridLayout_4->addLayout(gridLayout_2, 3, 3, 1, 1);

        m_folio_le = new QLineEdit(tab);
        m_folio_le->setObjectName("m_folio_le");

        gridLayout_4->addWidget(m_folio_le, 5, 3, 1, 1);

        m_loc = new QLineEdit(tab);
        m_loc->setObjectName("m_loc");

        gridLayout_4->addWidget(m_loc, 8, 3, 1, 1);

        m_plant = new QLineEdit(tab);
        m_plant->setObjectName("m_plant");

        gridLayout_4->addWidget(m_plant, 6, 3, 1, 1);

        label_2 = new QLabel(tab);
        label_2->setObjectName("label_2");

        gridLayout_4->addWidget(label_2, 1, 1, 1, 1);

        m_title_cb = new QCheckBox(tab);
        m_title_cb->setObjectName("m_title_cb");

        gridLayout_4->addWidget(m_title_cb, 1, 4, 1, 1);

        m_author_cb = new QCheckBox(tab);
        m_author_cb->setObjectName("m_author_cb");

        gridLayout_4->addWidget(m_author_cb, 2, 4, 1, 1);

        m_file_cb = new QCheckBox(tab);
        m_file_cb->setObjectName("m_file_cb");

        gridLayout_4->addWidget(m_file_cb, 4, 4, 1, 1);

        m_folio_cb = new QCheckBox(tab);
        m_folio_cb->setObjectName("m_folio_cb");

        gridLayout_4->addWidget(m_folio_cb, 5, 4, 1, 1);

        m_plant_cb = new QCheckBox(tab);
        m_plant_cb->setObjectName("m_plant_cb");

        gridLayout_4->addWidget(m_plant_cb, 6, 4, 1, 1);

        m_loc_cb = new QCheckBox(tab);
        m_loc_cb->setObjectName("m_loc_cb");

        gridLayout_4->addWidget(m_loc_cb, 8, 4, 1, 1);

        m_indice_cb = new QCheckBox(tab);
        m_indice_cb->setObjectName("m_indice_cb");

        gridLayout_4->addWidget(m_indice_cb, 9, 4, 1, 1);


        verticalLayout_2->addLayout(gridLayout_4);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        verticalLayout = new QVBoxLayout(tab_2);
        verticalLayout->setObjectName("verticalLayout");
        m_tab2_vlayout = new QVBoxLayout();
        m_tab2_vlayout->setObjectName("m_tab2_vlayout");
        label_8 = new QLabel(tab_2);
        label_8->setObjectName("label_8");
        label_8->setWordWrap(true);

        m_tab2_vlayout->addWidget(label_8);


        verticalLayout->addLayout(m_tab2_vlayout);

        tabWidget->addTab(tab_2, QString());

        verticalLayout_3->addWidget(tabWidget);


        retranslateUi(ReplaceFolioWidget);
        QObject::connect(m_fixed_date_rb, &QRadioButton::toggled, m_date_edit, &QDateEdit::setEnabled);
        QObject::connect(m_fixed_date_rb, &QRadioButton::toggled, m_date_now_pb, &QPushButton::setEnabled);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ReplaceFolioWidget);
    } // setupUi

    void retranslateUi(QWidget *ReplaceFolioWidget)
    {
        ReplaceFolioWidget->setWindowTitle(QCoreApplication::translate("ReplaceFolioWidget", "Form", nullptr));
        label_12->setText(QCoreApplication::translate("ReplaceFolioWidget", "Indice Rev", nullptr));
        label_11->setText(QCoreApplication::translate("ReplaceFolioWidget", "Localisation", nullptr));
        label_5->setText(QCoreApplication::translate("ReplaceFolioWidget", "Fichier :", nullptr));
#if QT_CONFIG(tooltip)
        m_title_le->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Disponible en tant que %title pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_title_le->setPlaceholderText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
#if QT_CONFIG(tooltip)
        m_author_le->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Disponible en tant que %author pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_author_le->setPlaceholderText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
        label_3->setText(QCoreApplication::translate("ReplaceFolioWidget", "Auteur :", nullptr));
        label_4->setText(QCoreApplication::translate("ReplaceFolioWidget", "Date :", nullptr));
        label_10->setText(QCoreApplication::translate("ReplaceFolioWidget", "Installation :", nullptr));
#if QT_CONFIG(tooltip)
        m_indice->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Disponible en tant que %indexrev pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_indice->setPlaceholderText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
#if QT_CONFIG(tooltip)
        m_file_le->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Disponible en tant que %filename pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_file_le->setPlaceholderText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
        label_6->setText(QCoreApplication::translate("ReplaceFolioWidget", "Folio :", nullptr));
        m_no_date_rb->setText(QCoreApplication::translate("ReplaceFolioWidget", "Pas de date", nullptr));
#if QT_CONFIG(tooltip)
        m_date_edit->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Disponible en tant que %date pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_fixed_date_rb->setText(QCoreApplication::translate("ReplaceFolioWidget", "Date fixe :", nullptr));
#if QT_CONFIG(tooltip)
        m_date_now_pb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Appliquer la date actuelle", nullptr));
#endif // QT_CONFIG(tooltip)
        m_date_now_pb->setText(QString());
        m_unchanged_date->setText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
#if QT_CONFIG(tooltip)
        m_folio_le->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Disponible en tant que %folio pour les mod\303\250les de cartouches\n"
"Les variables suivantes sont utilisables :\n"
"- %id : num\303\251ro du folio courant dans le projet \n"
"- %total : nombre total de folios dans le projet \n"
"- %autonum : Folio Auto Numeration", nullptr));
#endif // QT_CONFIG(tooltip)
        m_folio_le->setPlaceholderText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
#if QT_CONFIG(tooltip)
        m_loc->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Disponible en tant que %locmach pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_loc->setPlaceholderText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
#if QT_CONFIG(tooltip)
        m_plant->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "<html><head/><body><p>Disponible en tant que %plant pour les mod\303\250les de cartouches</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_plant->setPlaceholderText(QCoreApplication::translate("ReplaceFolioWidget", "Ne pas modifier", nullptr));
        label_2->setText(QCoreApplication::translate("ReplaceFolioWidget", "Titre :", nullptr));
#if QT_CONFIG(tooltip)
        m_title_cb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_title_cb->setText(QString());
#if QT_CONFIG(tooltip)
        m_author_cb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_author_cb->setText(QString());
#if QT_CONFIG(tooltip)
        m_file_cb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_file_cb->setText(QString());
#if QT_CONFIG(tooltip)
        m_folio_cb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_folio_cb->setText(QString());
#if QT_CONFIG(tooltip)
        m_plant_cb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_plant_cb->setText(QString());
#if QT_CONFIG(tooltip)
        m_loc_cb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_loc_cb->setText(QString());
#if QT_CONFIG(tooltip)
        m_indice_cb->setToolTip(QCoreApplication::translate("ReplaceFolioWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_indice_cb->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("ReplaceFolioWidget", "Principales", nullptr));
        label_8->setText(QCoreApplication::translate("ReplaceFolioWidget", "Vous pouvez d\303\251finir ici vos propres associations noms/valeurs pour que le cartouche en tienne compte. Exemple :\n"
"associer le nom \"volta\" et la valeur \"1745\" remplacera %{volta} par 1745 dans le cartouche.", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("ReplaceFolioWidget", "Personnalis\303\251es", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ReplaceFolioWidget: public Ui_ReplaceFolioWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPLACEFOLIOWIDGET_H
