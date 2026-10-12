/********************************************************************************
** Form generated from reading UI file 'terminalstripcreatordialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TERMINALSTRIPCREATORDIALOG_H
#define UI_TERMINALSTRIPCREATORDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TerminalStripCreatorDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label;
    QLineEdit *m_name_le;
    QLineEdit *m_location_le;
    QLineEdit *m_installation_le;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QLabel *label_4;
    QFrame *line;
    QPlainTextEdit *m_description_te;
    QDialogButtonBox *buttonBox;
    QLabel *label_5;
    QLineEdit *m_comment_le;

    void setupUi(QDialog *TerminalStripCreatorDialog)
    {
        if (TerminalStripCreatorDialog->objectName().isEmpty())
            TerminalStripCreatorDialog->setObjectName("TerminalStripCreatorDialog");
        TerminalStripCreatorDialog->resize(744, 321);
        gridLayout = new QGridLayout(TerminalStripCreatorDialog);
        gridLayout->setObjectName("gridLayout");
        label_2 = new QLabel(TerminalStripCreatorDialog);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 3, 0, 1, 1);

        label_3 = new QLabel(TerminalStripCreatorDialog);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 4, 0, 1, 1);

        label = new QLabel(TerminalStripCreatorDialog);
        label->setObjectName("label");

        gridLayout->addWidget(label, 2, 0, 1, 1);

        m_name_le = new QLineEdit(TerminalStripCreatorDialog);
        m_name_le->setObjectName("m_name_le");

        gridLayout->addWidget(m_name_le, 4, 1, 1, 1);

        m_location_le = new QLineEdit(TerminalStripCreatorDialog);
        m_location_le->setObjectName("m_location_le");

        gridLayout->addWidget(m_location_le, 3, 1, 1, 1);

        m_installation_le = new QLineEdit(TerminalStripCreatorDialog);
        m_installation_le->setObjectName("m_installation_le");

        gridLayout->addWidget(m_installation_le, 2, 1, 1, 1);

        widget = new QWidget(TerminalStripCreatorDialog);
        widget->setObjectName("widget");
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_4 = new QLabel(widget);
        label_4->setObjectName("label_4");

        horizontalLayout->addWidget(label_4);

        line = new QFrame(widget);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        horizontalLayout->addWidget(line);

        horizontalLayout->setStretch(1, 1);

        gridLayout->addWidget(widget, 7, 0, 1, 2);

        m_description_te = new QPlainTextEdit(TerminalStripCreatorDialog);
        m_description_te->setObjectName("m_description_te");

        gridLayout->addWidget(m_description_te, 8, 0, 1, 2);

        buttonBox = new QDialogButtonBox(TerminalStripCreatorDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 10, 0, 1, 2);

        label_5 = new QLabel(TerminalStripCreatorDialog);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 5, 0, 1, 1);

        m_comment_le = new QLineEdit(TerminalStripCreatorDialog);
        m_comment_le->setObjectName("m_comment_le");

        gridLayout->addWidget(m_comment_le, 5, 1, 1, 1);

        QWidget::setTabOrder(m_installation_le, m_location_le);
        QWidget::setTabOrder(m_location_le, m_name_le);
        QWidget::setTabOrder(m_name_le, m_comment_le);
        QWidget::setTabOrder(m_comment_le, m_description_te);

        retranslateUi(TerminalStripCreatorDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, TerminalStripCreatorDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, TerminalStripCreatorDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(TerminalStripCreatorDialog);
    } // setupUi

    void retranslateUi(QDialog *TerminalStripCreatorDialog)
    {
        TerminalStripCreatorDialog->setWindowTitle(QCoreApplication::translate("TerminalStripCreatorDialog", "Cr\303\251ation groupe de bornes", nullptr));
        label_2->setText(QCoreApplication::translate("TerminalStripCreatorDialog", "Localisation :", nullptr));
        label_3->setText(QCoreApplication::translate("TerminalStripCreatorDialog", "Nom :", nullptr));
        label->setText(QCoreApplication::translate("TerminalStripCreatorDialog", "Installation :", nullptr));
        label_4->setText(QCoreApplication::translate("TerminalStripCreatorDialog", "Description :", nullptr));
        label_5->setText(QCoreApplication::translate("TerminalStripCreatorDialog", "Commentaire :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TerminalStripCreatorDialog: public Ui_TerminalStripCreatorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TERMINALSTRIPCREATORDIALOG_H
