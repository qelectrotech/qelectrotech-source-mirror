/********************************************************************************
** Form generated from reading UI file 'searchandreplacewidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SEARCHANDREPLACEWIDGET_H
#define UI_SEARCHANDREPLACEWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SearchAndReplaceWidget
{
public:
    QGridLayout *gridLayout;
    QWidget *m_header_widget;
    QHBoxLayout *horizontalLayout_2;
    QLabel *m_search;
    QLineEdit *m_search_le;
    QWidget *m_advanced_button_widget;
    QHBoxLayout *horizontalLayout;
    QComboBox *m_mode_cb;
    QCheckBox *m_case_sensitive_cb;
    QPushButton *m_next_pb;
    QPushButton *m_previous_pb;
    QPushButton *m_reload_pb;
    QPushButton *m_advanced_pb;
    QWidget *m_advanced_widget;
    QGridLayout *gridLayout_2;
    QPushButton *m_element_pb;
    QPushButton *m_conductor_pb;
    QPushButton *m_replace_all_pb;
    QPushButton *m_advanced_replace_pb;
    QLabel *m_replace;
    QPushButton *m_folio_pb;
    QLineEdit *m_replace_le;
    QPushButton *m_replace_pb;
    QTreeWidget *m_tree_widget;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_quit_button;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *SearchAndReplaceWidget)
    {
        if (SearchAndReplaceWidget->objectName().isEmpty())
            SearchAndReplaceWidget->setObjectName("SearchAndReplaceWidget");
        SearchAndReplaceWidget->resize(989, 661);
        gridLayout = new QGridLayout(SearchAndReplaceWidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(9, -1, -1, -1);
        m_header_widget = new QWidget(SearchAndReplaceWidget);
        m_header_widget->setObjectName("m_header_widget");
        horizontalLayout_2 = new QHBoxLayout(m_header_widget);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        m_search = new QLabel(m_header_widget);
        m_search->setObjectName("m_search");

        horizontalLayout_2->addWidget(m_search);

        m_search_le = new QLineEdit(m_header_widget);
        m_search_le->setObjectName("m_search_le");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_search_le->sizePolicy().hasHeightForWidth());
        m_search_le->setSizePolicy(sizePolicy);
        m_search_le->setClearButtonEnabled(true);

        horizontalLayout_2->addWidget(m_search_le);

        m_advanced_button_widget = new QWidget(m_header_widget);
        m_advanced_button_widget->setObjectName("m_advanced_button_widget");
        horizontalLayout = new QHBoxLayout(m_advanced_button_widget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        m_mode_cb = new QComboBox(m_advanced_button_widget);
        m_mode_cb->addItem(QString());
        m_mode_cb->addItem(QString());
        m_mode_cb->setObjectName("m_mode_cb");
        m_mode_cb->setFrame(true);

        horizontalLayout->addWidget(m_mode_cb);

        m_case_sensitive_cb = new QCheckBox(m_advanced_button_widget);
        m_case_sensitive_cb->setObjectName("m_case_sensitive_cb");

        horizontalLayout->addWidget(m_case_sensitive_cb);


        horizontalLayout_2->addWidget(m_advanced_button_widget);

        m_next_pb = new QPushButton(m_header_widget);
        m_next_pb->setObjectName("m_next_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/go-bottom.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_next_pb->setIcon(icon);
        m_next_pb->setFlat(true);

        horizontalLayout_2->addWidget(m_next_pb);

        m_previous_pb = new QPushButton(m_header_widget);
        m_previous_pb->setObjectName("m_previous_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/go-top.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_previous_pb->setIcon(icon1);
        m_previous_pb->setFlat(true);

        horizontalLayout_2->addWidget(m_previous_pb);

        m_reload_pb = new QPushButton(m_header_widget);
        m_reload_pb->setObjectName("m_reload_pb");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/view-refresh.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_reload_pb->setIcon(icon2);
        m_reload_pb->setFlat(true);

        horizontalLayout_2->addWidget(m_reload_pb);

        m_advanced_pb = new QPushButton(m_header_widget);
        m_advanced_pb->setObjectName("m_advanced_pb");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/configure-toolbars.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_advanced_pb->setIcon(icon3);
        m_advanced_pb->setCheckable(true);
        m_advanced_pb->setChecked(false);
        m_advanced_pb->setFlat(true);

        horizontalLayout_2->addWidget(m_advanced_pb);


        gridLayout->addWidget(m_header_widget, 1, 1, 1, 1);

        m_advanced_widget = new QWidget(SearchAndReplaceWidget);
        m_advanced_widget->setObjectName("m_advanced_widget");
        m_advanced_widget->setMinimumSize(QSize(0, 0));
        m_advanced_widget->setBaseSize(QSize(0, 0));
        gridLayout_2 = new QGridLayout(m_advanced_widget);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setContentsMargins(0, -1, -1, -1);
        m_element_pb = new QPushButton(m_advanced_widget);
        m_element_pb->setObjectName("m_element_pb");
        m_element_pb->setEnabled(true);

        gridLayout_2->addWidget(m_element_pb, 0, 3, 1, 1);

        m_conductor_pb = new QPushButton(m_advanced_widget);
        m_conductor_pb->setObjectName("m_conductor_pb");

        gridLayout_2->addWidget(m_conductor_pb, 0, 4, 1, 1);

        m_replace_all_pb = new QPushButton(m_advanced_widget);
        m_replace_all_pb->setObjectName("m_replace_all_pb");
        m_replace_all_pb->setEnabled(false);

        gridLayout_2->addWidget(m_replace_all_pb, 0, 8, 1, 1);

        m_advanced_replace_pb = new QPushButton(m_advanced_widget);
        m_advanced_replace_pb->setObjectName("m_advanced_replace_pb");

        gridLayout_2->addWidget(m_advanced_replace_pb, 0, 5, 1, 1);

        m_replace = new QLabel(m_advanced_widget);
        m_replace->setObjectName("m_replace");

        gridLayout_2->addWidget(m_replace, 0, 0, 1, 1);

        m_folio_pb = new QPushButton(m_advanced_widget);
        m_folio_pb->setObjectName("m_folio_pb");

        gridLayout_2->addWidget(m_folio_pb, 0, 2, 1, 1);

        m_replace_le = new QLineEdit(m_advanced_widget);
        m_replace_le->setObjectName("m_replace_le");
        m_replace_le->setMinimumSize(QSize(200, 0));
        m_replace_le->setClearButtonEnabled(true);

        gridLayout_2->addWidget(m_replace_le, 0, 1, 1, 1);

        m_replace_pb = new QPushButton(m_advanced_widget);
        m_replace_pb->setObjectName("m_replace_pb");
        m_replace_pb->setEnabled(false);

        gridLayout_2->addWidget(m_replace_pb, 0, 7, 1, 1);

        m_tree_widget = new QTreeWidget(m_advanced_widget);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        m_tree_widget->setHeaderItem(__qtreewidgetitem);
        m_tree_widget->setObjectName("m_tree_widget");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_tree_widget->sizePolicy().hasHeightForWidth());
        m_tree_widget->setSizePolicy(sizePolicy1);
        m_tree_widget->setMinimumSize(QSize(0, 0));
        m_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
        m_tree_widget->setUniformRowHeights(true);
        m_tree_widget->setAnimated(true);
        m_tree_widget->setAllColumnsShowFocus(true);
        m_tree_widget->header()->setVisible(false);

        gridLayout_2->addWidget(m_tree_widget, 1, 0, 1, 9);

        horizontalSpacer = new QSpacerItem(100, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer, 0, 6, 1, 1);


        gridLayout->addWidget(m_advanced_widget, 2, 1, 1, 1);

        m_quit_button = new QPushButton(SearchAndReplaceWidget);
        m_quit_button->setObjectName("m_quit_button");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/ico/16x16/window-close.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_quit_button->setIcon(icon4);
        m_quit_button->setFlat(true);

        gridLayout->addWidget(m_quit_button, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 2, 0, 1, 1);


        retranslateUi(SearchAndReplaceWidget);

        QMetaObject::connectSlotsByName(SearchAndReplaceWidget);
    } // setupUi

    void retranslateUi(QWidget *SearchAndReplaceWidget)
    {
        SearchAndReplaceWidget->setWindowTitle(QCoreApplication::translate("SearchAndReplaceWidget", "Form", nullptr));
        m_search->setText(QCoreApplication::translate("SearchAndReplaceWidget", "Chercher :", nullptr));
        m_mode_cb->setItemText(0, QCoreApplication::translate("SearchAndReplaceWidget", "Texte brut", nullptr));
        m_mode_cb->setItemText(1, QCoreApplication::translate("SearchAndReplaceWidget", "Mots entiers", nullptr));

#if QT_CONFIG(tooltip)
        m_mode_cb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "Mode", nullptr));
#endif // QT_CONFIG(tooltip)
        m_case_sensitive_cb->setText(QCoreApplication::translate("SearchAndReplaceWidget", "Sensible \303\240 la casse", nullptr));
#if QT_CONFIG(tooltip)
        m_next_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "Aller \303\240 la correspondance suivante", nullptr));
#endif // QT_CONFIG(tooltip)
        m_next_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_previous_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "Aller \303\240 la correspondance pr\303\251c\303\251dente", nullptr));
#endif // QT_CONFIG(tooltip)
        m_previous_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_reload_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "Actualiser", nullptr));
#endif // QT_CONFIG(tooltip)
        m_reload_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_advanced_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "<html><head/><body><p>Afficher les options avanc\303\251es</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_advanced_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_element_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "<html><head/><body><p>D\303\251finir les propri\303\251t\303\251s \303\240 remplacer dans les \303\251l\303\251ments</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_element_pb->setText(QCoreApplication::translate("SearchAndReplaceWidget", "\303\211l\303\251ment", nullptr));
#if QT_CONFIG(tooltip)
        m_conductor_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "<html><head/><body><p>D\303\251finir les propri\303\251t\303\251s \303\240 remplacer dans les conducteurs</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_conductor_pb->setText(QCoreApplication::translate("SearchAndReplaceWidget", "Conducteur", nullptr));
#if QT_CONFIG(tooltip)
        m_replace_all_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "<html><head/><body><p>Remplacer les correspondances coch\303\251es</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_replace_all_pb->setText(QCoreApplication::translate("SearchAndReplaceWidget", "Tout remplacer", nullptr));
        m_advanced_replace_pb->setText(QCoreApplication::translate("SearchAndReplaceWidget", "avanc\303\251", nullptr));
        m_replace->setText(QCoreApplication::translate("SearchAndReplaceWidget", "Remplacer :", nullptr));
#if QT_CONFIG(tooltip)
        m_folio_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "<html><head/><body><p>D\303\251finir les propri\303\251t\303\251s \303\240 remplacer dans les folios</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_folio_pb->setText(QCoreApplication::translate("SearchAndReplaceWidget", "Folio", nullptr));
        m_replace_le->setPlaceholderText(QCoreApplication::translate("SearchAndReplaceWidget", "Champ texte de folio", nullptr));
#if QT_CONFIG(tooltip)
        m_replace_pb->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "<html><head/><body><p>Remplacer la correspondance s\303\251lectionn\303\251e</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        m_replace_pb->setText(QCoreApplication::translate("SearchAndReplaceWidget", "Remplacer", nullptr));
#if QT_CONFIG(tooltip)
        m_quit_button->setToolTip(QCoreApplication::translate("SearchAndReplaceWidget", "Quitter", nullptr));
#endif // QT_CONFIG(tooltip)
        m_quit_button->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class SearchAndReplaceWidget: public Ui_SearchAndReplaceWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SEARCHANDREPLACEWIDGET_H
