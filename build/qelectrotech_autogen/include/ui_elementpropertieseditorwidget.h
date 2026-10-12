/********************************************************************************
** Form generated from reading UI file 'elementpropertieseditorwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ELEMENTPROPERTIESEDITORWIDGET_H
#define UI_ELEMENTPROPERTIESEDITORWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ElementPropertiesEditorWidget
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *Type;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QComboBox *m_base_type_cb;
    QSpacerItem *verticalSpacer_2;
    QGroupBox *m_slave_gb;
    QVBoxLayout *verticalLayout_2;
    QGridLayout *gridLayout;
    QSpinBox *m_number_ctc;
    QLabel *label_4;
    QLabel *label_3;
    QLabel *label_2;
    QComboBox *m_state_cb;
    QComboBox *m_type_cb;
    QGroupBox *m_master_gb;
    QGridLayout *gridLayout_3;
    QHBoxLayout *type_concret_layout;
    QLabel *label_5;
    QSpacerItem *type_concret_spacer;
    QComboBox *m_master_type_cb;
    QCheckBox *max_slaves_checkbox;
    QSpinBox *max_slaves_spinbox;
    QCheckBox *m_slave_groups_checkbox;
    QTableWidget *m_slave_groups_table;
    QGroupBox *m_terminal_gb;
    QGridLayout *gridLayout_2;
    QLabel *label_6;
    QLabel *label_7;
    QComboBox *m_terminal_type_cb;
    QComboBox *m_terminal_func_cb;
    QWidget *Informations;
    QVBoxLayout *verticalLayout_4;
    QTreeWidget *m_tree;
    QDialogButtonBox *m_buttonBox;

    void setupUi(QDialog *ElementPropertiesEditorWidget)
    {
        if (ElementPropertiesEditorWidget->objectName().isEmpty())
            ElementPropertiesEditorWidget->setObjectName("ElementPropertiesEditorWidget");
        ElementPropertiesEditorWidget->resize(527, 492);
        ElementPropertiesEditorWidget->setSizeGripEnabled(false);
        verticalLayout = new QVBoxLayout(ElementPropertiesEditorWidget);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(ElementPropertiesEditorWidget);
        tabWidget->setObjectName("tabWidget");
        Type = new QWidget();
        Type->setObjectName("Type");
        verticalLayout_3 = new QVBoxLayout(Type);
        verticalLayout_3->setObjectName("verticalLayout_3");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(Type);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        m_base_type_cb = new QComboBox(Type);
        m_base_type_cb->setObjectName("m_base_type_cb");

        horizontalLayout->addWidget(m_base_type_cb);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        horizontalLayout->addItem(verticalSpacer_2);


        verticalLayout_3->addLayout(horizontalLayout);

        m_slave_gb = new QGroupBox(Type);
        m_slave_gb->setObjectName("m_slave_gb");
        verticalLayout_2 = new QVBoxLayout(m_slave_gb);
        verticalLayout_2->setObjectName("verticalLayout_2");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        m_number_ctc = new QSpinBox(m_slave_gb);
        m_number_ctc->setObjectName("m_number_ctc");
        m_number_ctc->setMinimum(1);

        gridLayout->addWidget(m_number_ctc, 3, 1, 1, 1);

        label_4 = new QLabel(m_slave_gb);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 3, 0, 1, 1);

        label_3 = new QLabel(m_slave_gb);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        label_2 = new QLabel(m_slave_gb);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        m_state_cb = new QComboBox(m_slave_gb);
        m_state_cb->setObjectName("m_state_cb");

        gridLayout->addWidget(m_state_cb, 1, 1, 1, 1);

        m_type_cb = new QComboBox(m_slave_gb);
        m_type_cb->setObjectName("m_type_cb");

        gridLayout->addWidget(m_type_cb, 2, 1, 1, 1);


        verticalLayout_2->addLayout(gridLayout);


        verticalLayout_3->addWidget(m_slave_gb);

        m_master_gb = new QGroupBox(Type);
        m_master_gb->setObjectName("m_master_gb");
        gridLayout_3 = new QGridLayout(m_master_gb);
        gridLayout_3->setObjectName("gridLayout_3");
        type_concret_layout = new QHBoxLayout();
        type_concret_layout->setObjectName("type_concret_layout");
        label_5 = new QLabel(m_master_gb);
        label_5->setObjectName("label_5");

        type_concret_layout->addWidget(label_5);

        type_concret_spacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        type_concret_layout->addItem(type_concret_spacer);

        m_master_type_cb = new QComboBox(m_master_gb);
        m_master_type_cb->setObjectName("m_master_type_cb");

        type_concret_layout->addWidget(m_master_type_cb);


        gridLayout_3->addLayout(type_concret_layout, 0, 0, 1, 2);

        max_slaves_checkbox = new QCheckBox(m_master_gb);
        max_slaves_checkbox->setObjectName("max_slaves_checkbox");

        gridLayout_3->addWidget(max_slaves_checkbox, 1, 0, 1, 1);

        max_slaves_spinbox = new QSpinBox(m_master_gb);
        max_slaves_spinbox->setObjectName("max_slaves_spinbox");
        max_slaves_spinbox->setEnabled(false);
        max_slaves_spinbox->setMinimum(1);

        gridLayout_3->addWidget(max_slaves_spinbox, 1, 1, 1, 1);

        m_slave_groups_checkbox = new QCheckBox(m_master_gb);
        m_slave_groups_checkbox->setObjectName("m_slave_groups_checkbox");

        gridLayout_3->addWidget(m_slave_groups_checkbox, 2, 0, 1, 1);

        m_slave_groups_table = new QTableWidget(m_master_gb);
        if (m_slave_groups_table->columnCount() < 4)
            m_slave_groups_table->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        m_slave_groups_table->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        m_slave_groups_table->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        m_slave_groups_table->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        m_slave_groups_table->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        m_slave_groups_table->setObjectName("m_slave_groups_table");
        m_slave_groups_table->setEnabled(false);
        m_slave_groups_table->setMinimumSize(QSize(0, 150));
        m_slave_groups_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_slave_groups_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_slave_groups_table->setColumnCount(4);

        gridLayout_3->addWidget(m_slave_groups_table, 3, 0, 1, 2);


        verticalLayout_3->addWidget(m_master_gb);

        m_terminal_gb = new QGroupBox(Type);
        m_terminal_gb->setObjectName("m_terminal_gb");
        gridLayout_2 = new QGridLayout(m_terminal_gb);
        gridLayout_2->setObjectName("gridLayout_2");
        label_6 = new QLabel(m_terminal_gb);
        label_6->setObjectName("label_6");

        gridLayout_2->addWidget(label_6, 0, 0, 1, 1);

        label_7 = new QLabel(m_terminal_gb);
        label_7->setObjectName("label_7");

        gridLayout_2->addWidget(label_7, 1, 0, 1, 1);

        m_terminal_type_cb = new QComboBox(m_terminal_gb);
        m_terminal_type_cb->setObjectName("m_terminal_type_cb");

        gridLayout_2->addWidget(m_terminal_type_cb, 0, 1, 1, 1);

        m_terminal_func_cb = new QComboBox(m_terminal_gb);
        m_terminal_func_cb->setObjectName("m_terminal_func_cb");

        gridLayout_2->addWidget(m_terminal_func_cb, 1, 1, 1, 1);


        verticalLayout_3->addWidget(m_terminal_gb);

        tabWidget->addTab(Type, QString());
        Informations = new QWidget();
        Informations->setObjectName("Informations");
        verticalLayout_4 = new QVBoxLayout(Informations);
        verticalLayout_4->setObjectName("verticalLayout_4");
        m_tree = new QTreeWidget(Informations);
        m_tree->setObjectName("m_tree");
        m_tree->header()->setDefaultSectionSize(150);

        verticalLayout_4->addWidget(m_tree);

        tabWidget->addTab(Informations, QString());

        verticalLayout->addWidget(tabWidget);

        m_buttonBox = new QDialogButtonBox(ElementPropertiesEditorWidget);
        m_buttonBox->setObjectName("m_buttonBox");
        m_buttonBox->setOrientation(Qt::Horizontal);
        m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(m_buttonBox);


        retranslateUi(ElementPropertiesEditorWidget);
        QObject::connect(m_buttonBox, &QDialogButtonBox::accepted, ElementPropertiesEditorWidget, qOverload<>(&QDialog::accept));
        QObject::connect(m_buttonBox, &QDialogButtonBox::rejected, ElementPropertiesEditorWidget, qOverload<>(&QDialog::reject));

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ElementPropertiesEditorWidget);
    } // setupUi

    void retranslateUi(QDialog *ElementPropertiesEditorWidget)
    {
        ElementPropertiesEditorWidget->setWindowTitle(QCoreApplication::translate("ElementPropertiesEditorWidget", "Propri\303\251t\303\251 de l'\303\251l\303\251ment", nullptr));
        label->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Type de base :", nullptr));
        m_slave_gb->setTitle(QCoreApplication::translate("ElementPropertiesEditorWidget", "\303\211l\303\251ment esclave", nullptr));
        label_4->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Nombre de contact repr\303\251sent\303\251", nullptr));
        label_3->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Type de contact", nullptr));
        label_2->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "\303\211tat du contact", nullptr));
        m_master_gb->setTitle(QCoreApplication::translate("ElementPropertiesEditorWidget", "\303\211l\303\251ment ma\303\256tre", nullptr));
        label_5->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Type concret", nullptr));
        max_slaves_checkbox->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "D\303\251finir le nombre maximal d'esclaves", nullptr));
        m_slave_groups_checkbox->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "D\303\251finir les \303\251l\303\251ments esclave", nullptr));
        QTableWidgetItem *___qtablewidgetitem = m_slave_groups_table->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Type", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = m_slave_groups_table->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Contact", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = m_slave_groups_table->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Nb. contacts", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = m_slave_groups_table->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Nb. bornes", nullptr));
        m_terminal_gb->setTitle(QCoreApplication::translate("ElementPropertiesEditorWidget", "\303\211l\303\251ment bornier", nullptr));
        label_6->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Type", nullptr));
        label_7->setText(QCoreApplication::translate("ElementPropertiesEditorWidget", "Fonction", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(Type), QCoreApplication::translate("ElementPropertiesEditorWidget", "Type", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = m_tree->headerItem();
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("ElementPropertiesEditorWidget", "Valeurs", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("ElementPropertiesEditorWidget", "Nom", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(Informations), QCoreApplication::translate("ElementPropertiesEditorWidget", "Informations", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ElementPropertiesEditorWidget: public Ui_ElementPropertiesEditorWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ELEMENTPROPERTIESEDITORWIDGET_H
