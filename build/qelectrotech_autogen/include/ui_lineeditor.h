/********************************************************************************
** Form generated from reading UI file 'lineeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LINEEDITOR_H
#define UI_LINEEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LineEditor
{
public:
    QVBoxLayout *m_main_layout;
    QGridLayout *gridLayout;
    QDoubleSpinBox *m_y2_sb;
    QDoubleSpinBox *m_y1_sb;
    QDoubleSpinBox *m_x1_sb;
    QLabel *label;
    QLabel *label_3;
    QLabel *label_2;
    QDoubleSpinBox *m_x2_sb;
    QLabel *label_5;
    QLabel *label_4;
    QLabel *label_6;
    QComboBox *m_end1_cb;
    QComboBox *m_end2_cb;
    QLabel *label_7;
    QLabel *label_8;
    QDoubleSpinBox *m_lenght1_sb;
    QDoubleSpinBox *m_lenght2_sb;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *LineEditor)
    {
        if (LineEditor->objectName().isEmpty())
            LineEditor->setObjectName("LineEditor");
        LineEditor->resize(347, 144);
        m_main_layout = new QVBoxLayout(LineEditor);
        m_main_layout->setObjectName("m_main_layout");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        m_y2_sb = new QDoubleSpinBox(LineEditor);
        m_y2_sb->setObjectName("m_y2_sb");
        m_y2_sb->setMinimum(-1000000.000000000000000);
        m_y2_sb->setMaximum(1000000.000000000000000);

        gridLayout->addWidget(m_y2_sb, 1, 3, 1, 1);

        m_y1_sb = new QDoubleSpinBox(LineEditor);
        m_y1_sb->setObjectName("m_y1_sb");
        m_y1_sb->setMinimum(-1000000.000000000000000);
        m_y1_sb->setMaximum(1000000.000000000000000);

        gridLayout->addWidget(m_y1_sb, 0, 3, 1, 1);

        m_x1_sb = new QDoubleSpinBox(LineEditor);
        m_x1_sb->setObjectName("m_x1_sb");
        m_x1_sb->setMinimum(-1000000.000000000000000);
        m_x1_sb->setMaximum(1000000.000000000000000);

        gridLayout->addWidget(m_x1_sb, 0, 1, 1, 1);

        label = new QLabel(LineEditor);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_3 = new QLabel(LineEditor);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 0, 2, 1, 1);

        label_2 = new QLabel(LineEditor);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        m_x2_sb = new QDoubleSpinBox(LineEditor);
        m_x2_sb->setObjectName("m_x2_sb");
        m_x2_sb->setMinimum(-1000000.000000000000000);
        m_x2_sb->setMaximum(1000000.000000000000000);

        gridLayout->addWidget(m_x2_sb, 1, 1, 1, 1);

        label_5 = new QLabel(LineEditor);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 2, 0, 1, 1);

        label_4 = new QLabel(LineEditor);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 1, 2, 1, 1);

        label_6 = new QLabel(LineEditor);
        label_6->setObjectName("label_6");

        gridLayout->addWidget(label_6, 3, 0, 1, 1);

        m_end1_cb = new QComboBox(LineEditor);
        m_end1_cb->setObjectName("m_end1_cb");

        gridLayout->addWidget(m_end1_cb, 2, 1, 1, 1);

        m_end2_cb = new QComboBox(LineEditor);
        m_end2_cb->setObjectName("m_end2_cb");

        gridLayout->addWidget(m_end2_cb, 3, 1, 1, 1);

        label_7 = new QLabel(LineEditor);
        label_7->setObjectName("label_7");

        gridLayout->addWidget(label_7, 2, 2, 1, 1);

        label_8 = new QLabel(LineEditor);
        label_8->setObjectName("label_8");

        gridLayout->addWidget(label_8, 3, 2, 1, 1);

        m_lenght1_sb = new QDoubleSpinBox(LineEditor);
        m_lenght1_sb->setObjectName("m_lenght1_sb");
        m_lenght1_sb->setSingleStep(0.100000000000000);
        m_lenght1_sb->setValue(1.500000000000000);

        gridLayout->addWidget(m_lenght1_sb, 2, 3, 1, 1);

        m_lenght2_sb = new QDoubleSpinBox(LineEditor);
        m_lenght2_sb->setObjectName("m_lenght2_sb");
        m_lenght2_sb->setSingleStep(0.100000000000000);
        m_lenght2_sb->setValue(1.500000000000000);

        gridLayout->addWidget(m_lenght2_sb, 3, 3, 1, 1);


        m_main_layout->addLayout(gridLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        m_main_layout->addItem(verticalSpacer);


        retranslateUi(LineEditor);

        QMetaObject::connectSlotsByName(LineEditor);
    } // setupUi

    void retranslateUi(QWidget *LineEditor)
    {
        LineEditor->setWindowTitle(QCoreApplication::translate("LineEditor", "Form", nullptr));
        label->setText(QCoreApplication::translate("LineEditor", "X1 :", nullptr));
        label_3->setText(QCoreApplication::translate("LineEditor", "Y1 :", nullptr));
        label_2->setText(QCoreApplication::translate("LineEditor", "X2 :", nullptr));
        label_5->setText(QCoreApplication::translate("LineEditor", "Fin 1 :", nullptr));
        label_4->setText(QCoreApplication::translate("LineEditor", "Y2 :", nullptr));
        label_6->setText(QCoreApplication::translate("LineEditor", "Fin 2 :", nullptr));
        label_7->setText(QCoreApplication::translate("LineEditor", "Longueur :", nullptr));
        label_8->setText(QCoreApplication::translate("LineEditor", "Longueur :", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LineEditor: public Ui_LineEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LINEEDITOR_H
