/********************************************************************************
** Form generated from reading UI file 'diagramcontextwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIAGRAMCONTEXTWIDGET_H
#define UI_DIAGRAMCONTEXTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DiagramContextWidget
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *m_label;
    QTableWidget *m_table;

    void setupUi(QWidget *DiagramContextWidget)
    {
        if (DiagramContextWidget->objectName().isEmpty())
            DiagramContextWidget->setObjectName("DiagramContextWidget");
        DiagramContextWidget->resize(400, 300);
        verticalLayout = new QVBoxLayout(DiagramContextWidget);
        verticalLayout->setObjectName("verticalLayout");
        m_label = new QLabel(DiagramContextWidget);
        m_label->setObjectName("m_label");
        m_label->setWordWrap(true);

        verticalLayout->addWidget(m_label);

        m_table = new QTableWidget(DiagramContextWidget);
        if (m_table->columnCount() < 2)
            m_table->setColumnCount(2);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        m_table->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        m_table->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        if (m_table->rowCount() < 1)
            m_table->setRowCount(1);
        m_table->setObjectName("m_table");
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setRowCount(1);
        m_table->setColumnCount(2);
        m_table->horizontalHeader()->setVisible(true);
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->verticalHeader()->setVisible(false);

        verticalLayout->addWidget(m_table);


        retranslateUi(DiagramContextWidget);

        QMetaObject::connectSlotsByName(DiagramContextWidget);
    } // setupUi

    void retranslateUi(QWidget *DiagramContextWidget)
    {
        DiagramContextWidget->setWindowTitle(QCoreApplication::translate("DiagramContextWidget", "Form", nullptr));
        m_label->setText(QCoreApplication::translate("DiagramContextWidget", "Les noms ne peuvent contenir que des lettres minuscules, des chiffres et des tirets.", nullptr));
        QTableWidgetItem *___qtablewidgetitem = m_table->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("DiagramContextWidget", "Nom", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = m_table->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("DiagramContextWidget", "Valeur", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DiagramContextWidget: public Ui_DiagramContextWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIAGRAMCONTEXTWIDGET_H
