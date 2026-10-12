/********************************************************************************
** Form generated from reading UI file 'propertieseditordockwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROPERTIESEDITORDOCKWIDGET_H
#define UI_PROPERTIESEDITORDOCKWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PropertiesEditorDockWidget
{
public:
    QWidget *dockWidgetContents;
    QVBoxLayout *verticalLayout;
    QVBoxLayout *m_main_vlayout;

    void setupUi(QDockWidget *PropertiesEditorDockWidget)
    {
        if (PropertiesEditorDockWidget->objectName().isEmpty())
            PropertiesEditorDockWidget->setObjectName("PropertiesEditorDockWidget");
        PropertiesEditorDockWidget->resize(400, 300);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName("dockWidgetContents");
        verticalLayout = new QVBoxLayout(dockWidgetContents);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setSizeConstraint(QLayout::SetMaximumSize);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        m_main_vlayout = new QVBoxLayout();
        m_main_vlayout->setObjectName("m_main_vlayout");

        verticalLayout->addLayout(m_main_vlayout);

        PropertiesEditorDockWidget->setWidget(dockWidgetContents);

        retranslateUi(PropertiesEditorDockWidget);

        QMetaObject::connectSlotsByName(PropertiesEditorDockWidget);
    } // setupUi

    void retranslateUi(QDockWidget *PropertiesEditorDockWidget)
    {
        PropertiesEditorDockWidget->setWindowTitle(QCoreApplication::translate("PropertiesEditorDockWidget", "Propri\303\251t\303\251s de la s\303\251lection", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PropertiesEditorDockWidget: public Ui_PropertiesEditorDockWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROPERTIESEDITORDOCKWIDGET_H
