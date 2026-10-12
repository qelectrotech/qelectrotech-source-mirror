/********************************************************************************
** Form generated from reading UI file 'freeterminaleditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FREETERMINALEDITOR_H
#define UI_FREETERMINALEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FreeTerminalEditor
{
public:
    QGridLayout *gridLayout;
    QLabel *m_move_label;
    QComboBox *m_move_in_cb;
    QSpacerItem *verticalSpacer;
    QTableView *m_table_view;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QPushButton *m_move_pb;
    QFrame *line;
    QComboBox *m_type_cb;
    QComboBox *m_function_cb;
    QComboBox *m_led_cb;

    void setupUi(QWidget *FreeTerminalEditor)
    {
        if (FreeTerminalEditor->objectName().isEmpty())
            FreeTerminalEditor->setObjectName("FreeTerminalEditor");
        FreeTerminalEditor->resize(727, 279);
        gridLayout = new QGridLayout(FreeTerminalEditor);
        gridLayout->setObjectName("gridLayout");
        m_move_label = new QLabel(FreeTerminalEditor);
        m_move_label->setObjectName("m_move_label");

        gridLayout->addWidget(m_move_label, 0, 1, 1, 1);

        m_move_in_cb = new QComboBox(FreeTerminalEditor);
        m_move_in_cb->setObjectName("m_move_in_cb");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_move_in_cb->sizePolicy().hasHeightForWidth());
        m_move_in_cb->setSizePolicy(sizePolicy);
        m_move_in_cb->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(m_move_in_cb, 0, 2, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 5, 2, 1, 1);

        m_table_view = new QTableView(FreeTerminalEditor);
        m_table_view->setObjectName("m_table_view");
        m_table_view->horizontalHeader()->setStretchLastSection(true);

        gridLayout->addWidget(m_table_view, 0, 0, 6, 1);

        label = new QLabel(FreeTerminalEditor);
        label->setObjectName("label");

        gridLayout->addWidget(label, 2, 1, 1, 1);

        label_2 = new QLabel(FreeTerminalEditor);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 3, 1, 1, 1);

        label_3 = new QLabel(FreeTerminalEditor);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 4, 1, 1, 1);

        m_move_pb = new QPushButton(FreeTerminalEditor);
        m_move_pb->setObjectName("m_move_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/22x22/dialog-ok.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_move_pb->setIcon(icon);

        gridLayout->addWidget(m_move_pb, 0, 3, 1, 1);

        line = new QFrame(FreeTerminalEditor);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout->addWidget(line, 1, 1, 1, 3);

        m_type_cb = new QComboBox(FreeTerminalEditor);
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->addItem(QString());
        m_type_cb->setObjectName("m_type_cb");

        gridLayout->addWidget(m_type_cb, 2, 2, 1, 2);

        m_function_cb = new QComboBox(FreeTerminalEditor);
        m_function_cb->addItem(QString());
        m_function_cb->addItem(QString());
        m_function_cb->addItem(QString());
        m_function_cb->setObjectName("m_function_cb");

        gridLayout->addWidget(m_function_cb, 3, 2, 1, 2);

        m_led_cb = new QComboBox(FreeTerminalEditor);
        m_led_cb->addItem(QString());
        m_led_cb->addItem(QString());
        m_led_cb->setObjectName("m_led_cb");

        gridLayout->addWidget(m_led_cb, 4, 2, 1, 2);


        retranslateUi(FreeTerminalEditor);

        QMetaObject::connectSlotsByName(FreeTerminalEditor);
    } // setupUi

    void retranslateUi(QWidget *FreeTerminalEditor)
    {
        FreeTerminalEditor->setWindowTitle(QCoreApplication::translate("FreeTerminalEditor", "Form", nullptr));
        m_move_label->setText(QCoreApplication::translate("FreeTerminalEditor", "D\303\251placer dans :", nullptr));
        label->setText(QCoreApplication::translate("FreeTerminalEditor", "Type :", nullptr));
        label_2->setText(QCoreApplication::translate("FreeTerminalEditor", "Fonction :", nullptr));
        label_3->setText(QCoreApplication::translate("FreeTerminalEditor", "LED :", nullptr));
#if QT_CONFIG(tooltip)
        m_move_pb->setToolTip(QCoreApplication::translate("FreeTerminalEditor", "Appliquer le d\303\251placement", nullptr));
#endif // QT_CONFIG(tooltip)
        m_move_pb->setText(QString());
        m_type_cb->setItemText(0, QCoreApplication::translate("FreeTerminalEditor", "G\303\251n\303\251rique", nullptr));
        m_type_cb->setItemText(1, QCoreApplication::translate("FreeTerminalEditor", "Fusible", nullptr));
        m_type_cb->setItemText(2, QCoreApplication::translate("FreeTerminalEditor", "Sectionnable", nullptr));
        m_type_cb->setItemText(3, QCoreApplication::translate("FreeTerminalEditor", "Diode", nullptr));
        m_type_cb->setItemText(4, QCoreApplication::translate("FreeTerminalEditor", "Terre", nullptr));

        m_function_cb->setItemText(0, QCoreApplication::translate("FreeTerminalEditor", "G\303\251n\303\251rique", nullptr));
        m_function_cb->setItemText(1, QCoreApplication::translate("FreeTerminalEditor", "Phase", nullptr));
        m_function_cb->setItemText(2, QCoreApplication::translate("FreeTerminalEditor", "Neutre", nullptr));

        m_led_cb->setItemText(0, QCoreApplication::translate("FreeTerminalEditor", "Sans", nullptr));
        m_led_cb->setItemText(1, QCoreApplication::translate("FreeTerminalEditor", "Avec", nullptr));

    } // retranslateUi

};

namespace Ui {
    class FreeTerminalEditor: public Ui_FreeTerminalEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FREETERMINALEDITOR_H
