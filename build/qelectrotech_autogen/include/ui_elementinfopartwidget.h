/********************************************************************************
** Form generated from reading UI file 'elementinfopartwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ELEMENTINFOPARTWIDGET_H
#define UI_ELEMENTINFOPARTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ElementInfoPartWidget
{
public:
    QGridLayout *gridLayout_2;
    QLineEdit *line_edit;
    QLabel *label_;
    QCheckBox *m_erase_text;

    void setupUi(QWidget *ElementInfoPartWidget)
    {
        if (ElementInfoPartWidget->objectName().isEmpty())
            ElementInfoPartWidget->setObjectName("ElementInfoPartWidget");
        ElementInfoPartWidget->resize(65, 44);
        ElementInfoPartWidget->setMinimumSize(QSize(0, 0));
        gridLayout_2 = new QGridLayout(ElementInfoPartWidget);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setSizeConstraint(QLayout::SetMinimumSize);
        gridLayout_2->setHorizontalSpacing(0);
        gridLayout_2->setVerticalSpacing(2);
        gridLayout_2->setContentsMargins(0, 2, 0, 2);
        line_edit = new QLineEdit(ElementInfoPartWidget);
        line_edit->setObjectName("line_edit");
        line_edit->setClearButtonEnabled(true);

        gridLayout_2->addWidget(line_edit, 2, 0, 1, 1);

        label_ = new QLabel(ElementInfoPartWidget);
        label_->setObjectName("label_");

        gridLayout_2->addWidget(label_, 1, 0, 1, 1);

        m_erase_text = new QCheckBox(ElementInfoPartWidget);
        m_erase_text->setObjectName("m_erase_text");

        gridLayout_2->addWidget(m_erase_text, 2, 1, 1, 1);


        retranslateUi(ElementInfoPartWidget);

        QMetaObject::connectSlotsByName(ElementInfoPartWidget);
    } // setupUi

    void retranslateUi(QWidget *ElementInfoPartWidget)
    {
        ElementInfoPartWidget->setWindowTitle(QCoreApplication::translate("ElementInfoPartWidget", "Form", nullptr));
        label_->setText(QCoreApplication::translate("ElementInfoPartWidget", "TextLabel", nullptr));
#if QT_CONFIG(tooltip)
        m_erase_text->setToolTip(QCoreApplication::translate("ElementInfoPartWidget", "Supprimer ce texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_erase_text->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ElementInfoPartWidget: public Ui_ElementInfoPartWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ELEMENTINFOPARTWIDGET_H
