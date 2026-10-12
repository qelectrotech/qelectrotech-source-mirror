/********************************************************************************
** Form generated from reading UI file 'elementinfowidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ELEMENTINFOWIDGET_H
#define UI_ELEMENTINFOWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ElementInfoWidget
{
public:
    QVBoxLayout *verticalLayout;
    QCheckBox *m_auto_num_locked_cb;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *scroll_vlayout;

    void setupUi(QWidget *ElementInfoWidget)
    {
        if (ElementInfoWidget->objectName().isEmpty())
            ElementInfoWidget->setObjectName("ElementInfoWidget");
        ElementInfoWidget->resize(300, 400);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ElementInfoWidget->sizePolicy().hasHeightForWidth());
        ElementInfoWidget->setSizePolicy(sizePolicy);
        ElementInfoWidget->setMinimumSize(QSize(300, 400));
        verticalLayout = new QVBoxLayout(ElementInfoWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        m_auto_num_locked_cb = new QCheckBox(ElementInfoWidget);
        m_auto_num_locked_cb->setObjectName("m_auto_num_locked_cb");
        m_auto_num_locked_cb->setLayoutDirection(Qt::LeftToRight);
        m_auto_num_locked_cb->setStyleSheet(QString::fromUtf8("margin: 5px; font-weight: bold;"));

        verticalLayout->addWidget(m_auto_num_locked_cb);

        scrollArea = new QScrollArea(ElementInfoWidget);
        scrollArea->setObjectName("scrollArea");
        sizePolicy.setHeightForWidth(scrollArea->sizePolicy().hasHeightForWidth());
        scrollArea->setSizePolicy(sizePolicy);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 280, 380));
        scrollAreaWidgetContents->setMinimumSize(QSize(0, 280));
        scroll_vlayout = new QVBoxLayout(scrollAreaWidgetContents);
        scroll_vlayout->setSpacing(2);
        scroll_vlayout->setObjectName("scroll_vlayout");
        scroll_vlayout->setSizeConstraint(QLayout::SetMinimumSize);
        scroll_vlayout->setContentsMargins(0, 2, 0, 4);
        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);


        retranslateUi(ElementInfoWidget);

        QMetaObject::connectSlotsByName(ElementInfoWidget);
    } // setupUi

    void retranslateUi(QWidget *ElementInfoWidget)
    {
        ElementInfoWidget->setWindowTitle(QCoreApplication::translate("ElementInfoWidget", "Form", nullptr));
        m_auto_num_locked_cb->setText(QCoreApplication::translate("ElementInfoWidget", "Exclure de la num\303\251rotation auto", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ElementInfoWidget: public Ui_ElementInfoWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ELEMENTINFOWIDGET_H
