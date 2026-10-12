/********************************************************************************
** Form generated from reading UI file 'linksingleelementwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LINKSINGLEELEMENTWIDGET_H
#define UI_LINKSINGLEELEMENTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LinkSingleElementWidget
{
public:
    QGridLayout *gridLayout;
    QPushButton *m_show_this_pb;
    QTreeWidget *m_tree_widget;
    QPushButton *m_unlink_pb;
    QPushButton *m_show_linked_pb;
    QLabel *m_label;
    QLineEdit *m_search_field;
    QLabel *m_hidden_masters_label;

    void setupUi(QWidget *LinkSingleElementWidget)
    {
        if (LinkSingleElementWidget->objectName().isEmpty())
            LinkSingleElementWidget->setObjectName("LinkSingleElementWidget");
        LinkSingleElementWidget->resize(389, 460);
        gridLayout = new QGridLayout(LinkSingleElementWidget);
        gridLayout->setObjectName("gridLayout");
        m_show_this_pb = new QPushButton(LinkSingleElementWidget);
        m_show_this_pb->setObjectName("m_show_this_pb");

        gridLayout->addWidget(m_show_this_pb, 1, 2, 1, 1);

        m_tree_widget = new QTreeWidget(LinkSingleElementWidget);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        m_tree_widget->setHeaderItem(__qtreewidgetitem);
        m_tree_widget->setObjectName("m_tree_widget");
        m_tree_widget->setIndentation(5);
        m_tree_widget->setSortingEnabled(true);

        gridLayout->addWidget(m_tree_widget, 3, 0, 1, 3);

        m_unlink_pb = new QPushButton(LinkSingleElementWidget);
        m_unlink_pb->setObjectName("m_unlink_pb");

        gridLayout->addWidget(m_unlink_pb, 1, 0, 1, 1);

        m_show_linked_pb = new QPushButton(LinkSingleElementWidget);
        m_show_linked_pb->setObjectName("m_show_linked_pb");

        gridLayout->addWidget(m_show_linked_pb, 0, 2, 1, 1);

        m_label = new QLabel(LinkSingleElementWidget);
        m_label->setObjectName("m_label");

        gridLayout->addWidget(m_label, 0, 0, 1, 1);

        m_search_field = new QLineEdit(LinkSingleElementWidget);
        m_search_field->setObjectName("m_search_field");

        gridLayout->addWidget(m_search_field, 2, 0, 1, 3);

        m_hidden_masters_label = new QLabel(LinkSingleElementWidget);
        m_hidden_masters_label->setObjectName("m_hidden_masters_label");
        m_hidden_masters_label->setWordWrap(true);
        QFont font;
        font.setItalic(true);
        m_hidden_masters_label->setFont(font);

        gridLayout->addWidget(m_hidden_masters_label, 4, 0, 1, 3);


        retranslateUi(LinkSingleElementWidget);

        QMetaObject::connectSlotsByName(LinkSingleElementWidget);
    } // setupUi

    void retranslateUi(QWidget *LinkSingleElementWidget)
    {
        LinkSingleElementWidget->setWindowTitle(QCoreApplication::translate("LinkSingleElementWidget", "Form", nullptr));
        m_show_this_pb->setText(QCoreApplication::translate("LinkSingleElementWidget", "Voir cet \303\251l\303\251ment", nullptr));
        m_unlink_pb->setText(QCoreApplication::translate("LinkSingleElementWidget", "D\303\251lier", nullptr));
        m_show_linked_pb->setText(QCoreApplication::translate("LinkSingleElementWidget", "Voir l'\303\251l\303\251ment li\303\251", nullptr));
        m_label->setText(QCoreApplication::translate("LinkSingleElementWidget", "Cet \303\251l\303\251ment est d\303\251j\303\240 li\303\251", nullptr));
        m_search_field->setPlaceholderText(QCoreApplication::translate("LinkSingleElementWidget", "Recherche", nullptr));
        m_hidden_masters_label->setText(QCoreApplication::translate("LinkSingleElementWidget", "Remarque : les \303\251l\303\251ments ma\303\256tres ayant atteint leur nombre maximal d'esclaves sont masqu\303\251s.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LinkSingleElementWidget: public Ui_LinkSingleElementWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LINKSINGLEELEMENTWIDGET_H
