/********************************************************************************
** Form generated from reading UI file 'titleblockpropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TITLEBLOCKPROPERTIESWIDGET_H
#define UI_TITLEBLOCKPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TitleBlockPropertiesWidget
{
public:
    QVBoxLayout *verticalLayout_4;
    QGroupBox *m_tbt_gb;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *m_tbt_hlayout;
    QLabel *m_tbt_label;
    QComboBox *m_tbt_cb;
    QComboBox *m_display_at_cb;
    QPushButton *m_tbt_pb;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QGridLayout *gridLayout_4;
    QLabel *label_4;
    QGridLayout *gridLayout;
    QComboBox *auto_page_cb;
    QPushButton *m_edit_autofolionum_pb;
    QLabel *label_3;
    QLabel *label_12;
    QLineEdit *m_loc;
    QLineEdit *m_plant;
    QLineEdit *m_title_le;
    QLabel *label_6;
    QLabel *label_5;
    QLineEdit *m_author_le;
    QLineEdit *m_file_le;
    QGridLayout *gridLayout_2;
    QRadioButton *m_no_date_rb;
    QRadioButton *m_current_date_rb;
    QPushButton *m_date_now_pb;
    QDateEdit *m_date_edit;
    QRadioButton *m_fixed_date_rb;
    QLineEdit *m_folio_le;
    QLabel *label_2;
    QLabel *label_9;
    QLineEdit *m_indice;
    QLabel *label_10;
    QLabel *label_11;
    QSpacerItem *verticalSpacer;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout;
    QVBoxLayout *m_tab2_vlayout;
    QLabel *label_8;

    void setupUi(QWidget *TitleBlockPropertiesWidget)
    {
        if (TitleBlockPropertiesWidget->objectName().isEmpty())
            TitleBlockPropertiesWidget->setObjectName("TitleBlockPropertiesWidget");
        TitleBlockPropertiesWidget->resize(753, 901);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(TitleBlockPropertiesWidget->sizePolicy().hasHeightForWidth());
        TitleBlockPropertiesWidget->setSizePolicy(sizePolicy);
        TitleBlockPropertiesWidget->setMinimumSize(QSize(0, 0));
        verticalLayout_4 = new QVBoxLayout(TitleBlockPropertiesWidget);
        verticalLayout_4->setObjectName("verticalLayout_4");
        m_tbt_gb = new QGroupBox(TitleBlockPropertiesWidget);
        m_tbt_gb->setObjectName("m_tbt_gb");
        m_tbt_gb->setCheckable(false);
        m_tbt_gb->setChecked(false);
        verticalLayout_3 = new QVBoxLayout(m_tbt_gb);
        verticalLayout_3->setObjectName("verticalLayout_3");
        m_tbt_hlayout = new QHBoxLayout();
        m_tbt_hlayout->setObjectName("m_tbt_hlayout");
        m_tbt_hlayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        m_tbt_label = new QLabel(m_tbt_gb);
        m_tbt_label->setObjectName("m_tbt_label");

        m_tbt_hlayout->addWidget(m_tbt_label);

        m_tbt_cb = new QComboBox(m_tbt_gb);
        m_tbt_cb->setObjectName("m_tbt_cb");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_tbt_cb->sizePolicy().hasHeightForWidth());
        m_tbt_cb->setSizePolicy(sizePolicy1);

        m_tbt_hlayout->addWidget(m_tbt_cb);

        m_display_at_cb = new QComboBox(m_tbt_gb);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/22x22/titleblock-bottom.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_display_at_cb->addItem(icon, QString::fromUtf8(""));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/22x22/titleblock-right.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_display_at_cb->addItem(icon1, QString::fromUtf8(""));
        m_display_at_cb->setObjectName("m_display_at_cb");
        sizePolicy1.setHeightForWidth(m_display_at_cb->sizePolicy().hasHeightForWidth());
        m_display_at_cb->setSizePolicy(sizePolicy1);
        m_display_at_cb->setMinimumSize(QSize(0, 0));
        m_display_at_cb->setMaximumSize(QSize(16777215, 16777215));
        m_display_at_cb->setIconSize(QSize(22, 22));

        m_tbt_hlayout->addWidget(m_display_at_cb);

        m_tbt_pb = new QPushButton(m_tbt_gb);
        m_tbt_pb->setObjectName("m_tbt_pb");
        sizePolicy1.setHeightForWidth(m_tbt_pb->sizePolicy().hasHeightForWidth());
        m_tbt_pb->setSizePolicy(sizePolicy1);
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/22x22/edit-rename.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_tbt_pb->setIcon(icon2);
        m_tbt_pb->setIconSize(QSize(22, 22));
        m_tbt_pb->setFlat(false);

        m_tbt_hlayout->addWidget(m_tbt_pb);

        m_tbt_hlayout->setStretch(1, 1);

        verticalLayout_3->addLayout(m_tbt_hlayout);

        tabWidget = new QTabWidget(m_tbt_gb);
        tabWidget->setObjectName("tabWidget");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy2);
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
        label_4 = new QLabel(tab);
        label_4->setObjectName("label_4");
        label_4->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout_4->addWidget(label_4, 3, 1, 1, 1);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        auto_page_cb = new QComboBox(tab);
        auto_page_cb->setObjectName("auto_page_cb");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(auto_page_cb->sizePolicy().hasHeightForWidth());
        auto_page_cb->setSizePolicy(sizePolicy3);

        gridLayout->addWidget(auto_page_cb, 0, 0, 1, 1);

        m_edit_autofolionum_pb = new QPushButton(tab);
        m_edit_autofolionum_pb->setObjectName("m_edit_autofolionum_pb");
        sizePolicy.setHeightForWidth(m_edit_autofolionum_pb->sizePolicy().hasHeightForWidth());
        m_edit_autofolionum_pb->setSizePolicy(sizePolicy);
        m_edit_autofolionum_pb->setMinimumSize(QSize(16, 16));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/configure.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_edit_autofolionum_pb->setIcon(icon3);

        gridLayout->addWidget(m_edit_autofolionum_pb, 0, 1, 1, 1);


        gridLayout_4->addLayout(gridLayout, 10, 3, 1, 1);

        label_3 = new QLabel(tab);
        label_3->setObjectName("label_3");

        gridLayout_4->addWidget(label_3, 2, 1, 1, 1);

        label_12 = new QLabel(tab);
        label_12->setObjectName("label_12");

        gridLayout_4->addWidget(label_12, 9, 1, 1, 1);

        m_loc = new QLineEdit(tab);
        m_loc->setObjectName("m_loc");

        gridLayout_4->addWidget(m_loc, 8, 3, 1, 1);

        m_plant = new QLineEdit(tab);
        m_plant->setObjectName("m_plant");

        gridLayout_4->addWidget(m_plant, 6, 3, 1, 1);

        m_title_le = new QLineEdit(tab);
        m_title_le->setObjectName("m_title_le");

        gridLayout_4->addWidget(m_title_le, 1, 3, 1, 1);

        label_6 = new QLabel(tab);
        label_6->setObjectName("label_6");

        gridLayout_4->addWidget(label_6, 5, 1, 1, 1);

        label_5 = new QLabel(tab);
        label_5->setObjectName("label_5");

        gridLayout_4->addWidget(label_5, 4, 1, 1, 1);

        m_author_le = new QLineEdit(tab);
        m_author_le->setObjectName("m_author_le");

        gridLayout_4->addWidget(m_author_le, 2, 3, 1, 1);

        m_file_le = new QLineEdit(tab);
        m_file_le->setObjectName("m_file_le");

        gridLayout_4->addWidget(m_file_le, 4, 3, 1, 1);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName("gridLayout_2");
        m_no_date_rb = new QRadioButton(tab);
        m_no_date_rb->setObjectName("m_no_date_rb");

        gridLayout_2->addWidget(m_no_date_rb, 0, 0, 1, 1);

        m_current_date_rb = new QRadioButton(tab);
        m_current_date_rb->setObjectName("m_current_date_rb");

        gridLayout_2->addWidget(m_current_date_rb, 1, 0, 1, 1);

        m_date_now_pb = new QPushButton(tab);
        m_date_now_pb->setObjectName("m_date_now_pb");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/ico/22x22/start.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_date_now_pb->setIcon(icon4);

        gridLayout_2->addWidget(m_date_now_pb, 2, 2, 1, 1);

        m_date_edit = new QDateEdit(tab);
        m_date_edit->setObjectName("m_date_edit");
        m_date_edit->setFrame(true);
        m_date_edit->setCalendarPopup(true);

        gridLayout_2->addWidget(m_date_edit, 2, 1, 1, 1);

        m_fixed_date_rb = new QRadioButton(tab);
        m_fixed_date_rb->setObjectName("m_fixed_date_rb");

        gridLayout_2->addWidget(m_fixed_date_rb, 2, 0, 1, 1);

        gridLayout_2->setColumnStretch(1, 1);

        gridLayout_4->addLayout(gridLayout_2, 3, 3, 1, 1);

        m_folio_le = new QLineEdit(tab);
        m_folio_le->setObjectName("m_folio_le");

        gridLayout_4->addWidget(m_folio_le, 5, 3, 1, 1);

        label_2 = new QLabel(tab);
        label_2->setObjectName("label_2");

        gridLayout_4->addWidget(label_2, 1, 1, 1, 1);

        label_9 = new QLabel(tab);
        label_9->setObjectName("label_9");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(label_9->sizePolicy().hasHeightForWidth());
        label_9->setSizePolicy(sizePolicy4);

        gridLayout_4->addWidget(label_9, 10, 1, 1, 1);

        m_indice = new QLineEdit(tab);
        m_indice->setObjectName("m_indice");

        gridLayout_4->addWidget(m_indice, 9, 3, 1, 1);

        label_10 = new QLabel(tab);
        label_10->setObjectName("label_10");

        gridLayout_4->addWidget(label_10, 6, 1, 1, 1);

        label_11 = new QLabel(tab);
        label_11->setObjectName("label_11");

        gridLayout_4->addWidget(label_11, 8, 1, 1, 1);


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


        verticalLayout_4->addWidget(m_tbt_gb);

        QWidget::setTabOrder(m_title_le, m_author_le);
        QWidget::setTabOrder(m_author_le, m_no_date_rb);
        QWidget::setTabOrder(m_no_date_rb, m_current_date_rb);
        QWidget::setTabOrder(m_current_date_rb, m_fixed_date_rb);
        QWidget::setTabOrder(m_fixed_date_rb, m_date_edit);
        QWidget::setTabOrder(m_date_edit, m_date_now_pb);
        QWidget::setTabOrder(m_date_now_pb, m_file_le);
        QWidget::setTabOrder(m_file_le, m_folio_le);
        QWidget::setTabOrder(m_folio_le, m_plant);
        QWidget::setTabOrder(m_plant, m_loc);
        QWidget::setTabOrder(m_loc, m_indice);
        QWidget::setTabOrder(m_indice, auto_page_cb);
        QWidget::setTabOrder(auto_page_cb, m_edit_autofolionum_pb);
        QWidget::setTabOrder(m_edit_autofolionum_pb, m_tbt_cb);
        QWidget::setTabOrder(m_tbt_cb, m_display_at_cb);
        QWidget::setTabOrder(m_display_at_cb, m_tbt_pb);
        QWidget::setTabOrder(m_tbt_pb, tabWidget);

        retranslateUi(TitleBlockPropertiesWidget);
        QObject::connect(m_fixed_date_rb, &QRadioButton::toggled, m_date_edit, &QDateEdit::setEnabled);
        QObject::connect(m_fixed_date_rb, &QRadioButton::toggled, m_date_now_pb, &QPushButton::setEnabled);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(TitleBlockPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *TitleBlockPropertiesWidget)
    {
        TitleBlockPropertiesWidget->setWindowTitle(QCoreApplication::translate("TitleBlockPropertiesWidget", "Form", nullptr));
        m_tbt_gb->setTitle(QCoreApplication::translate("TitleBlockPropertiesWidget", "Informations des cartouches", nullptr));
        m_tbt_label->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Mod\303\250le :", nullptr));

#if QT_CONFIG(tooltip)
        m_display_at_cb->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "<html><head/><body><p>Affiche le cartouche en bas (horizontalement) ou \303\240 droite (verticalement) du folio.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_tbt_pb->setText(QString());
        label_4->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Date :", nullptr));
        m_edit_autofolionum_pb->setText(QString());
        label_3->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Auteur :", nullptr));
        label_12->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Indice Rev:", nullptr));
#if QT_CONFIG(tooltip)
        m_loc->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Disponible en tant que %locmach pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_plant->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "<html><head/><body><p>Disponible en tant que %plant pour les mod\303\250les de cartouches</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_title_le->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Disponible en tant que %title pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        label_6->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Folio :", nullptr));
        label_5->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Fichier :", nullptr));
#if QT_CONFIG(tooltip)
        m_author_le->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Disponible en tant que %author pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_file_le->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Disponible en tant que %filename pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_no_date_rb->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Pas de date", nullptr));
        m_current_date_rb->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Date courante", nullptr));
#if QT_CONFIG(tooltip)
        m_date_now_pb->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Appliquer la date actuelle", nullptr));
#endif // QT_CONFIG(tooltip)
        m_date_now_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_date_edit->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Disponible en tant que %date pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        m_fixed_date_rb->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Date fixe :", nullptr));
#if QT_CONFIG(tooltip)
        m_folio_le->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Disponible en tant que %folio pour les mod\303\250les de cartouches\n"
"Les variables suivantes sont utilisables :\n"
"- %id : num\303\251ro du folio courant dans le projet \n"
"- %total : nombre total de folios dans le projet \n"
"- %autonum : Folio Auto Numeration", nullptr));
#endif // QT_CONFIG(tooltip)
        label_2->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Titre :", nullptr));
        label_9->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Page Num:", nullptr));
#if QT_CONFIG(tooltip)
        m_indice->setToolTip(QCoreApplication::translate("TitleBlockPropertiesWidget", "Disponible en tant que %indexrev pour les mod\303\250les de cartouches", nullptr));
#endif // QT_CONFIG(tooltip)
        label_10->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Installation :", nullptr));
        label_11->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Localisation:", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("TitleBlockPropertiesWidget", "Principales", nullptr));
        label_8->setText(QCoreApplication::translate("TitleBlockPropertiesWidget", "Vous pouvez d\303\251finir ici vos propres associations noms/valeurs pour que le cartouche en tienne compte. Exemple :\n"
"associer le nom \"volta\" et la valeur \"1745\" remplacera %{volta} par 1745 dans le cartouche.", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("TitleBlockPropertiesWidget", "Personnalis\303\251es", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TitleBlockPropertiesWidget: public Ui_TitleBlockPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TITLEBLOCKPROPERTIESWIDGET_H
