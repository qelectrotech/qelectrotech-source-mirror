/********************************************************************************
** Form generated from reading UI file 'terminalstriptreedockwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TERMINALSTRIPTREEDOCKWIDGET_H
#define UI_TERMINALSTRIPTREEDOCKWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TerminalStripTreeDockWidget
{
public:
    QWidget *dockWidgetContents;
    QVBoxLayout *verticalLayout;
    QTreeWidget *m_tree_view;

    void setupUi(QDockWidget *TerminalStripTreeDockWidget)
    {
        if (TerminalStripTreeDockWidget->objectName().isEmpty())
            TerminalStripTreeDockWidget->setObjectName("TerminalStripTreeDockWidget");
        TerminalStripTreeDockWidget->resize(397, 542);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName("dockWidgetContents");
        verticalLayout = new QVBoxLayout(dockWidgetContents);
        verticalLayout->setObjectName("verticalLayout");
        m_tree_view = new QTreeWidget(dockWidgetContents);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        m_tree_view->setHeaderItem(__qtreewidgetitem);
        m_tree_view->setObjectName("m_tree_view");
        m_tree_view->setAnimated(true);
        m_tree_view->header()->setVisible(false);

        verticalLayout->addWidget(m_tree_view);

        TerminalStripTreeDockWidget->setWidget(dockWidgetContents);

        retranslateUi(TerminalStripTreeDockWidget);

        QMetaObject::connectSlotsByName(TerminalStripTreeDockWidget);
    } // setupUi

    void retranslateUi(QDockWidget *TerminalStripTreeDockWidget)
    {
        TerminalStripTreeDockWidget->setWindowTitle(QCoreApplication::translate("TerminalStripTreeDockWidget", "Explorateur de bornier", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TerminalStripTreeDockWidget: public Ui_TerminalStripTreeDockWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TERMINALSTRIPTREEDOCKWIDGET_H
