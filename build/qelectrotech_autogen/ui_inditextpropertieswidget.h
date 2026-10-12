/********************************************************************************
** Form generated from reading UI file 'inditextpropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INDITEXTPROPERTIESWIDGET_H
#define UI_INDITEXTPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IndiTextPropertiesWidget
{
public:
    QGridLayout *gridLayout;
    QLabel *label_2;
    QPushButton *m_advanced_editor_pb;
    QLabel *label_4;
    QLabel *label_3;
    QDoubleSpinBox *m_x_sb;
    QSpacerItem *verticalSpacer;
    QSpinBox *m_size_sb;
    QDoubleSpinBox *m_y_sb;
    QDoubleSpinBox *m_angle_sb;
    QLabel *label;
    QLabel *m_label;
    QPushButton *m_break_html_pb;
    QLineEdit *m_line_edit;
    QPushButton *m_font_pb;

    void setupUi(QWidget *IndiTextPropertiesWidget)
    {
        if (IndiTextPropertiesWidget->objectName().isEmpty())
            IndiTextPropertiesWidget->setObjectName("IndiTextPropertiesWidget");
        IndiTextPropertiesWidget->resize(340, 296);
        gridLayout = new QGridLayout(IndiTextPropertiesWidget);
        gridLayout->setObjectName("gridLayout");
        label_2 = new QLabel(IndiTextPropertiesWidget);
        label_2->setObjectName("label_2");
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        m_advanced_editor_pb = new QPushButton(IndiTextPropertiesWidget);
        m_advanced_editor_pb->setObjectName("m_advanced_editor_pb");

        gridLayout->addWidget(m_advanced_editor_pb, 1, 0, 1, 4);

        label_4 = new QLabel(IndiTextPropertiesWidget);
        label_4->setObjectName("label_4");
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_4, 3, 2, 1, 1);

        label_3 = new QLabel(IndiTextPropertiesWidget);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_3, 3, 0, 1, 1);

        m_x_sb = new QDoubleSpinBox(IndiTextPropertiesWidget);
        m_x_sb->setObjectName("m_x_sb");
        m_x_sb->setDecimals(0);
        m_x_sb->setMaximum(10000.000000000000000);

        gridLayout->addWidget(m_x_sb, 2, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 6, 0, 1, 1);

        m_size_sb = new QSpinBox(IndiTextPropertiesWidget);
        m_size_sb->setObjectName("m_size_sb");
        m_size_sb->setMinimum(4);
        m_size_sb->setMaximum(50);

        gridLayout->addWidget(m_size_sb, 3, 3, 1, 1);

        m_y_sb = new QDoubleSpinBox(IndiTextPropertiesWidget);
        m_y_sb->setObjectName("m_y_sb");
        m_y_sb->setDecimals(0);
        m_y_sb->setMaximum(10000.000000000000000);

        gridLayout->addWidget(m_y_sb, 2, 3, 1, 1);

        m_angle_sb = new QDoubleSpinBox(IndiTextPropertiesWidget);
        m_angle_sb->setObjectName("m_angle_sb");
        m_angle_sb->setWrapping(true);
        m_angle_sb->setDecimals(0);
        m_angle_sb->setMaximum(359.000000000000000);

        gridLayout->addWidget(m_angle_sb, 3, 1, 1, 1);

        label = new QLabel(IndiTextPropertiesWidget);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label, 2, 2, 1, 1);

        m_label = new QLabel(IndiTextPropertiesWidget);
        m_label->setObjectName("m_label");
        m_label->setScaledContents(false);
        m_label->setWordWrap(true);

        gridLayout->addWidget(m_label, 4, 0, 1, 4);

        m_break_html_pb = new QPushButton(IndiTextPropertiesWidget);
        m_break_html_pb->setObjectName("m_break_html_pb");

        gridLayout->addWidget(m_break_html_pb, 5, 0, 1, 4);

        m_line_edit = new QLineEdit(IndiTextPropertiesWidget);
        m_line_edit->setObjectName("m_line_edit");

        gridLayout->addWidget(m_line_edit, 0, 0, 1, 3);

        m_font_pb = new QPushButton(IndiTextPropertiesWidget);
        m_font_pb->setObjectName("m_font_pb");

        gridLayout->addWidget(m_font_pb, 0, 3, 1, 1);

        QWidget::setTabOrder(m_line_edit, m_font_pb);
        QWidget::setTabOrder(m_font_pb, m_advanced_editor_pb);
        QWidget::setTabOrder(m_advanced_editor_pb, m_x_sb);
        QWidget::setTabOrder(m_x_sb, m_y_sb);
        QWidget::setTabOrder(m_y_sb, m_angle_sb);
        QWidget::setTabOrder(m_angle_sb, m_size_sb);
        QWidget::setTabOrder(m_size_sb, m_break_html_pb);

        retranslateUi(IndiTextPropertiesWidget);

        QMetaObject::connectSlotsByName(IndiTextPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *IndiTextPropertiesWidget)
    {
        IndiTextPropertiesWidget->setWindowTitle(QCoreApplication::translate("IndiTextPropertiesWidget", "Form", nullptr));
        label_2->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "X :", nullptr));
        m_advanced_editor_pb->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "\303\211diteur avanc\303\251", nullptr));
        label_4->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "Taille :", nullptr));
        label_3->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "Angle :", nullptr));
        m_x_sb->setSuffix(QCoreApplication::translate("IndiTextPropertiesWidget", "px", nullptr));
        m_y_sb->setSuffix(QCoreApplication::translate("IndiTextPropertiesWidget", "px", nullptr));
        m_angle_sb->setSuffix(QCoreApplication::translate("IndiTextPropertiesWidget", "\302\260", nullptr));
        label->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "Y :", nullptr));
        m_label->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "Le contenu, la taille et la police du texte ne peuvent \303\252tre modifi\303\251 car format\303\251 en html.\n"
"Veuillez utiliser l'\303\251diteur avanc\303\251 pour cela.", nullptr));
        m_break_html_pb->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "Cliquez ici pour annuler le formatage html", nullptr));
        m_line_edit->setPlaceholderText(QCoreApplication::translate("IndiTextPropertiesWidget", "Texte", nullptr));
        m_font_pb->setText(QCoreApplication::translate("IndiTextPropertiesWidget", "Police", nullptr));
    } // retranslateUi

};

namespace Ui {
    class IndiTextPropertiesWidget: public Ui_IndiTextPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INDITEXTPROPERTIESWIDGET_H
