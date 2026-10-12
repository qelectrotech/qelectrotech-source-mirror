/********************************************************************************
** Form generated from reading UI file 'multipastedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MULTIPASTEDIALOG_H
#define UI_MULTIPASTEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_MultiPasteDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpinBox *m_x_sb;
    QSpinBox *m_y_sb;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QSpinBox *m_copy_count;
    QCheckBox *m_auto_connection_cb;
    QCheckBox *m_auto_num_cb;
    QCheckBox *m_auto_num_cond_cb;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *m_button_box;

    void setupUi(QDialog *MultiPasteDialog)
    {
        if (MultiPasteDialog->objectName().isEmpty())
            MultiPasteDialog->setObjectName("MultiPasteDialog");
        MultiPasteDialog->resize(304, 162);
        verticalLayout = new QVBoxLayout(MultiPasteDialog);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(MultiPasteDialog);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        m_x_sb = new QSpinBox(MultiPasteDialog);
        m_x_sb->setObjectName("m_x_sb");
        m_x_sb->setMinimum(-1000);
        m_x_sb->setMaximum(1000);
        m_x_sb->setSingleStep(10);
        m_x_sb->setValue(100);

        horizontalLayout->addWidget(m_x_sb);

        m_y_sb = new QSpinBox(MultiPasteDialog);
        m_y_sb->setObjectName("m_y_sb");
        m_y_sb->setMinimum(-1000);
        m_y_sb->setMaximum(1000);
        m_y_sb->setSingleStep(10);
        m_y_sb->setValue(0);

        horizontalLayout->addWidget(m_y_sb);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_2 = new QLabel(MultiPasteDialog);
        label_2->setObjectName("label_2");

        horizontalLayout_2->addWidget(label_2);

        m_copy_count = new QSpinBox(MultiPasteDialog);
        m_copy_count->setObjectName("m_copy_count");
        m_copy_count->setMinimum(1);

        horizontalLayout_2->addWidget(m_copy_count);


        verticalLayout->addLayout(horizontalLayout_2);

        m_auto_connection_cb = new QCheckBox(MultiPasteDialog);
        m_auto_connection_cb->setObjectName("m_auto_connection_cb");

        verticalLayout->addWidget(m_auto_connection_cb);

        m_auto_num_cb = new QCheckBox(MultiPasteDialog);
        m_auto_num_cb->setObjectName("m_auto_num_cb");

        verticalLayout->addWidget(m_auto_num_cb);

        m_auto_num_cond_cb = new QCheckBox(MultiPasteDialog);
        m_auto_num_cond_cb->setObjectName("m_auto_num_cond_cb");

        verticalLayout->addWidget(m_auto_num_cond_cb);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        m_button_box = new QDialogButtonBox(MultiPasteDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setOrientation(Qt::Horizontal);
        m_button_box->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(m_button_box);


        retranslateUi(MultiPasteDialog);
        QObject::connect(m_button_box, &QDialogButtonBox::accepted, MultiPasteDialog, qOverload<>(&QDialog::accept));
        QObject::connect(m_button_box, &QDialogButtonBox::rejected, MultiPasteDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(MultiPasteDialog);
    } // setupUi

    void retranslateUi(QDialog *MultiPasteDialog)
    {
        MultiPasteDialog->setWindowTitle(QCoreApplication::translate("MultiPasteDialog", "Collage multiple", nullptr));
        label->setText(QCoreApplication::translate("MultiPasteDialog", "D\303\251calage", nullptr));
        m_x_sb->setSuffix(QCoreApplication::translate("MultiPasteDialog", "px", nullptr));
        m_x_sb->setPrefix(QCoreApplication::translate("MultiPasteDialog", "x:  ", nullptr));
        m_y_sb->setSuffix(QCoreApplication::translate("MultiPasteDialog", "px", nullptr));
        m_y_sb->setPrefix(QCoreApplication::translate("MultiPasteDialog", "y:  ", nullptr));
        label_2->setText(QCoreApplication::translate("MultiPasteDialog", "Nombre de copie", nullptr));
        m_auto_connection_cb->setText(QCoreApplication::translate("MultiPasteDialog", "Auto-connexion", nullptr));
        m_auto_num_cb->setText(QCoreApplication::translate("MultiPasteDialog", "Auto-num\303\251rotation des \303\251l\303\251ments", nullptr));
        m_auto_num_cond_cb->setText(QCoreApplication::translate("MultiPasteDialog", "Auto-num\303\251rotation des conducteurs", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MultiPasteDialog: public Ui_MultiPasteDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MULTIPASTEDIALOG_H
