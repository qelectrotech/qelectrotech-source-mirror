/********************************************************************************
** Form generated from reading UI file 'namelistwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NAMELISTWIDGET_H
#define UI_NAMELISTWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NameListWidget
{
public:
    QVBoxLayout *verticalLayout;
    QTreeWidget *m_tree;
    QHBoxLayout *horizontalLayout;
    QPushButton *m_add_line_pb;
    QComboBox *m_clipboard_cb;

    void setupUi(QWidget *NameListWidget)
    {
        if (NameListWidget->objectName().isEmpty())
            NameListWidget->setObjectName("NameListWidget");
        NameListWidget->resize(400, 300);
        verticalLayout = new QVBoxLayout(NameListWidget);
        verticalLayout->setObjectName("verticalLayout");
        m_tree = new QTreeWidget(NameListWidget);
        m_tree->setObjectName("m_tree");

        verticalLayout->addWidget(m_tree);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        m_add_line_pb = new QPushButton(NameListWidget);
        m_add_line_pb->setObjectName("m_add_line_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/list-add.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_add_line_pb->setIcon(icon);

        horizontalLayout->addWidget(m_add_line_pb);

        m_clipboard_cb = new QComboBox(NameListWidget);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/edit-paste.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_clipboard_cb->addItem(icon1, QString());
        m_clipboard_cb->setObjectName("m_clipboard_cb");

        horizontalLayout->addWidget(m_clipboard_cb);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(NameListWidget);

        QMetaObject::connectSlotsByName(NameListWidget);
    } // setupUi

    void retranslateUi(QWidget *NameListWidget)
    {
        NameListWidget->setWindowTitle(QCoreApplication::translate("NameListWidget", "Form", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = m_tree->headerItem();
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("NameListWidget", "Texte", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("NameListWidget", "Langue", nullptr));
        m_add_line_pb->setText(QCoreApplication::translate("NameListWidget", "Ajouter une ligne", nullptr));
        m_clipboard_cb->setItemText(0, QCoreApplication::translate("NameListWidget", "Copier dans le presse papier", nullptr));

    } // retranslateUi

};

namespace Ui {
    class NameListWidget: public Ui_NameListWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NAMELISTWIDGET_H
