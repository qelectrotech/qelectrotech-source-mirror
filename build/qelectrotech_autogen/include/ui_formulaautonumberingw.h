/********************************************************************************
** Form generated from reading UI file 'formulaautonumberingw.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FORMULAAUTONUMBERINGW_H
#define UI_FORMULAAUTONUMBERINGW_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FormulaAutonumberingW
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout;
    QLabel *label_5;
    QLineEdit *m_formula_le;
    QSpacerItem *horizontalSpacer;
    QLabel *label;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *FormulaAutonumberingW)
    {
        if (FormulaAutonumberingW->objectName().isEmpty())
            FormulaAutonumberingW->setObjectName("FormulaAutonumberingW");
        FormulaAutonumberingW->resize(716, 531);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(FormulaAutonumberingW->sizePolicy().hasHeightForWidth());
        FormulaAutonumberingW->setSizePolicy(sizePolicy);
        FormulaAutonumberingW->setMinimumSize(QSize(0, 0));
        verticalLayout = new QVBoxLayout(FormulaAutonumberingW);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        groupBox = new QGroupBox(FormulaAutonumberingW);
        groupBox->setObjectName("groupBox");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy1);
        horizontalLayout = new QHBoxLayout(groupBox);
        horizontalLayout->setObjectName("horizontalLayout");
        label_5 = new QLabel(groupBox);
        label_5->setObjectName("label_5");
        sizePolicy1.setHeightForWidth(label_5->sizePolicy().hasHeightForWidth());
        label_5->setSizePolicy(sizePolicy1);

        horizontalLayout->addWidget(label_5);

        m_formula_le = new QLineEdit(groupBox);
        m_formula_le->setObjectName("m_formula_le");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(m_formula_le->sizePolicy().hasHeightForWidth());
        m_formula_le->setSizePolicy(sizePolicy2);
        m_formula_le->setInputMask(QString::fromUtf8(""));
        m_formula_le->setText(QString::fromUtf8(""));
        m_formula_le->setClearButtonEnabled(true);

        horizontalLayout->addWidget(m_formula_le);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addWidget(groupBox);

        label = new QLabel(FormulaAutonumberingW);
        label->setObjectName("label");
        label->setLocale(QLocale(QLocale::French, QLocale::France));

        verticalLayout->addWidget(label);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(FormulaAutonumberingW);

        QMetaObject::connectSlotsByName(FormulaAutonumberingW);
    } // setupUi

    void retranslateUi(QWidget *FormulaAutonumberingW)
    {
        FormulaAutonumberingW->setWindowTitle(QCoreApplication::translate("FormulaAutonumberingW", "Form", nullptr));
        groupBox->setTitle(QCoreApplication::translate("FormulaAutonumberingW", "D\303\251nomination automatique :", nullptr));
        label_5->setText(QCoreApplication::translate("FormulaAutonumberingW", "Formule", nullptr));
        m_formula_le->setPlaceholderText(QCoreApplication::translate("FormulaAutonumberingW", "Insert Formula Here e.g.: %prefix%l%c", nullptr));
        label->setText(QCoreApplication::translate("FormulaAutonumberingW", "You can use the following variables to your formula:\n"
" -%prefix: Default Element Prefix\n"
" -%l: Element Line\n"
" -%c: Element Column\n"
" -%F: Folio Name\n"
" -%f or %id: Folio ID\n"
" -%total: Total of folios\n"
"You can also assign any other titleblock variable \n"
"that you create. Text and number inputs are\n"
" also available.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FormulaAutonumberingW: public Ui_FormulaAutonumberingW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FORMULAAUTONUMBERINGW_H
