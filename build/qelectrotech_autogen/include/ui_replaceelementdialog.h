/********************************************************************************
** Form generated from reading UI file 'replaceelementdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPLACEELEMENTDIALOG_H
#define UI_REPLACEELEMENTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ReplaceElementDialog
{
public:
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *m_scroll_layout;
    QDialogButtonBox *m_button_box;

    void setupUi(QDialog *ReplaceElementDialog)
    {
        if (ReplaceElementDialog->objectName().isEmpty())
            ReplaceElementDialog->setObjectName("ReplaceElementDialog");
        ReplaceElementDialog->resize(300, 400);
        ReplaceElementDialog->setMinimumSize(QSize(300, 400));
        verticalLayout = new QVBoxLayout(ReplaceElementDialog);
        verticalLayout->setObjectName("verticalLayout");
        scrollArea = new QScrollArea(ReplaceElementDialog);
        scrollArea->setObjectName("scrollArea");
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 284, 346));
        m_scroll_layout = new QVBoxLayout(scrollAreaWidgetContents);
        m_scroll_layout->setObjectName("m_scroll_layout");
        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);

        m_button_box = new QDialogButtonBox(ReplaceElementDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setOrientation(Qt::Horizontal);
        m_button_box->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::Reset);

        verticalLayout->addWidget(m_button_box);


        retranslateUi(ReplaceElementDialog);

        QMetaObject::connectSlotsByName(ReplaceElementDialog);
    } // setupUi

    void retranslateUi(QDialog *ReplaceElementDialog)
    {
        (void)ReplaceElementDialog;
    } // retranslateUi

};

namespace Ui {
    class ReplaceElementDialog: public Ui_ReplaceElementDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPLACEELEMENTDIALOG_H
