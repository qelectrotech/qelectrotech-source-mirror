/********************************************************************************
** Form generated from reading UI file 'alignmenttextdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ALIGNMENTTEXTDIALOG_H
#define UI_ALIGNMENTTEXTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AlignmentTextDialog
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QRadioButton *right;
    QSpacerItem *horizontalSpacer;
    QRadioButton *top;
    QRadioButton *bottom;
    QRadioButton *top_right;
    QRadioButton *left;
    QRadioButton *bottom_right;
    QRadioButton *bottom_left;
    QRadioButton *top_left;
    QRadioButton *center;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AlignmentTextDialog)
    {
        if (AlignmentTextDialog->objectName().isEmpty())
            AlignmentTextDialog->setObjectName("AlignmentTextDialog");
        AlignmentTextDialog->resize(160, 158);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(200);
        sizePolicy.setVerticalStretch(150);
        sizePolicy.setHeightForWidth(AlignmentTextDialog->sizePolicy().hasHeightForWidth());
        AlignmentTextDialog->setSizePolicy(sizePolicy);
        AlignmentTextDialog->setMaximumSize(QSize(160, 158));
        AlignmentTextDialog->setModal(true);
        verticalLayout = new QVBoxLayout(AlignmentTextDialog);
        verticalLayout->setObjectName("verticalLayout");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        right = new QRadioButton(AlignmentTextDialog);
        right->setObjectName("right");

        gridLayout->addWidget(right, 1, 3, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 1, 4, 1, 1);

        top = new QRadioButton(AlignmentTextDialog);
        top->setObjectName("top");

        gridLayout->addWidget(top, 0, 2, 1, 1);

        bottom = new QRadioButton(AlignmentTextDialog);
        bottom->setObjectName("bottom");

        gridLayout->addWidget(bottom, 2, 2, 1, 1);

        top_right = new QRadioButton(AlignmentTextDialog);
        top_right->setObjectName("top_right");

        gridLayout->addWidget(top_right, 0, 3, 1, 1);

        left = new QRadioButton(AlignmentTextDialog);
        left->setObjectName("left");

        gridLayout->addWidget(left, 1, 1, 1, 1);

        bottom_right = new QRadioButton(AlignmentTextDialog);
        bottom_right->setObjectName("bottom_right");

        gridLayout->addWidget(bottom_right, 2, 3, 1, 1);

        bottom_left = new QRadioButton(AlignmentTextDialog);
        bottom_left->setObjectName("bottom_left");

        gridLayout->addWidget(bottom_left, 2, 1, 1, 1);

        top_left = new QRadioButton(AlignmentTextDialog);
        top_left->setObjectName("top_left");

        gridLayout->addWidget(top_left, 0, 1, 1, 1);

        center = new QRadioButton(AlignmentTextDialog);
        center->setObjectName("center");

        gridLayout->addWidget(center, 1, 2, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 1, 0, 1, 1);


        verticalLayout->addLayout(gridLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(AlignmentTextDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(AlignmentTextDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, AlignmentTextDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, AlignmentTextDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(AlignmentTextDialog);
    } // setupUi

    void retranslateUi(QDialog *AlignmentTextDialog)
    {
        AlignmentTextDialog->setWindowTitle(QCoreApplication::translate("AlignmentTextDialog", "Alignement du texte", nullptr));
        right->setText(QString());
        top->setText(QString());
        bottom->setText(QString());
        top_right->setText(QString());
        left->setText(QString());
        bottom_right->setText(QString());
        bottom_left->setText(QString());
        top_left->setText(QString());
        center->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class AlignmentTextDialog: public Ui_AlignmentTextDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ALIGNMENTTEXTDIALOG_H
