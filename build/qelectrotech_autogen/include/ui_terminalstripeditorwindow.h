/********************************************************************************
** Form generated from reading UI file 'terminalstripeditorwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TERMINALSTRIPEDITORWINDOW_H
#define UI_TERMINALSTRIPEDITORWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TerminalStripEditorWindow
{
public:
    QAction *m_add_terminal_strip;
    QAction *m_remove_terminal;
    QAction *m_reload;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QStackedWidget *m_stacked_widget;
    QDialogButtonBox *m_button_box;
    QMenuBar *menubar;
    QStatusBar *statusbar;
    QToolBar *m_tool_bar;

    void setupUi(QMainWindow *TerminalStripEditorWindow)
    {
        if (TerminalStripEditorWindow->objectName().isEmpty())
            TerminalStripEditorWindow->setObjectName("TerminalStripEditorWindow");
        TerminalStripEditorWindow->resize(1364, 868);
        m_add_terminal_strip = new QAction(TerminalStripEditorWindow);
        m_add_terminal_strip->setObjectName("m_add_terminal_strip");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/list-add.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_add_terminal_strip->setIcon(icon);
        m_remove_terminal = new QAction(TerminalStripEditorWindow);
        m_remove_terminal->setObjectName("m_remove_terminal");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/list-remove.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_remove_terminal->setIcon(icon1);
        m_reload = new QAction(TerminalStripEditorWindow);
        m_reload->setObjectName("m_reload");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/view-refresh.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_reload->setIcon(icon2);
        centralwidget = new QWidget(TerminalStripEditorWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        m_stacked_widget = new QStackedWidget(centralwidget);
        m_stacked_widget->setObjectName("m_stacked_widget");

        verticalLayout->addWidget(m_stacked_widget);

        m_button_box = new QDialogButtonBox(centralwidget);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Reset);

        verticalLayout->addWidget(m_button_box);

        TerminalStripEditorWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(TerminalStripEditorWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1364, 23));
        TerminalStripEditorWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(TerminalStripEditorWindow);
        statusbar->setObjectName("statusbar");
        TerminalStripEditorWindow->setStatusBar(statusbar);
        m_tool_bar = new QToolBar(TerminalStripEditorWindow);
        m_tool_bar->setObjectName("m_tool_bar");
        TerminalStripEditorWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, m_tool_bar);

        m_tool_bar->addAction(m_add_terminal_strip);
        m_tool_bar->addAction(m_remove_terminal);
        m_tool_bar->addAction(m_reload);

        retranslateUi(TerminalStripEditorWindow);

        QMetaObject::connectSlotsByName(TerminalStripEditorWindow);
    } // setupUi

    void retranslateUi(QMainWindow *TerminalStripEditorWindow)
    {
        TerminalStripEditorWindow->setWindowTitle(QCoreApplication::translate("TerminalStripEditorWindow", "Gestionnaire de borniers", nullptr));
        m_add_terminal_strip->setText(QCoreApplication::translate("TerminalStripEditorWindow", "Ajouter un bornier", nullptr));
#if QT_CONFIG(tooltip)
        m_add_terminal_strip->setToolTip(QCoreApplication::translate("TerminalStripEditorWindow", "Ajouter un bornier au projet", nullptr));
#endif // QT_CONFIG(tooltip)
        m_remove_terminal->setText(QCoreApplication::translate("TerminalStripEditorWindow", "Supprimer le bornier", nullptr));
#if QT_CONFIG(tooltip)
        m_remove_terminal->setToolTip(QCoreApplication::translate("TerminalStripEditorWindow", "Supprimer le bornier du projet", nullptr));
#endif // QT_CONFIG(tooltip)
        m_reload->setText(QCoreApplication::translate("TerminalStripEditorWindow", "Recharger", nullptr));
#if QT_CONFIG(tooltip)
        m_reload->setToolTip(QCoreApplication::translate("TerminalStripEditorWindow", "Recharger les borniers", nullptr));
#endif // QT_CONFIG(tooltip)
        m_tool_bar->setWindowTitle(QCoreApplication::translate("TerminalStripEditorWindow", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TerminalStripEditorWindow: public Ui_TerminalStripEditorWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TERMINALSTRIPEDITORWINDOW_H
