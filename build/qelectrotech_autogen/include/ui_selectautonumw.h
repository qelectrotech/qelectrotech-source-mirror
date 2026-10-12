/********************************************************************************
** Form generated from reading UI file 'selectautonumw.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SELECTAUTONUMW_H
#define UI_SELECTAUTONUMW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SelectAutonumW
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QSpacerItem *horizontalSpacer_3;
    QComboBox *m_comboBox;
    QPushButton *m_remove_pb;
    QWidget *m_widget;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *m_definition_groupe;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *remove_button;
    QPushButton *add_button;
    QPushButton *m_previous_pb;
    QPushButton *m_next_pb;
    QSpacerItem *horizontalSpacer_2;
    QVBoxLayout *editor_layout;
    QHBoxLayout *label_layout;
    QLabel *type_label;
    QLabel *value_label;
    QLabel *increase_label;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *SelectAutonumW)
    {
        if (SelectAutonumW->objectName().isEmpty())
            SelectAutonumW->setObjectName("SelectAutonumW");
        SelectAutonumW->resize(473, 175);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(SelectAutonumW->sizePolicy().hasHeightForWidth());
        SelectAutonumW->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(SelectAutonumW);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label = new QLabel(SelectAutonumW);
        label->setObjectName("label");

        horizontalLayout_2->addWidget(label);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);

        m_comboBox = new QComboBox(SelectAutonumW);
        m_comboBox->addItem(QString());
        m_comboBox->setObjectName("m_comboBox");
        m_comboBox->setEditable(true);

        horizontalLayout_2->addWidget(m_comboBox);

        m_remove_pb = new QPushButton(SelectAutonumW);
        m_remove_pb->setObjectName("m_remove_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/edit-delete.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_remove_pb->setIcon(icon);

        horizontalLayout_2->addWidget(m_remove_pb);


        verticalLayout->addLayout(horizontalLayout_2);

        m_widget = new QWidget(SelectAutonumW);
        m_widget->setObjectName("m_widget");
        verticalLayout_2 = new QVBoxLayout(m_widget);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        m_definition_groupe = new QGroupBox(m_widget);
        m_definition_groupe->setObjectName("m_definition_groupe");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_definition_groupe->sizePolicy().hasHeightForWidth());
        m_definition_groupe->setSizePolicy(sizePolicy1);
        m_definition_groupe->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        verticalLayout_3 = new QVBoxLayout(m_definition_groupe);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        remove_button = new QPushButton(m_definition_groupe);
        remove_button->setObjectName("remove_button");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/22x22/list-remove.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        remove_button->setIcon(icon1);
        remove_button->setFlat(false);

        horizontalLayout->addWidget(remove_button);

        add_button = new QPushButton(m_definition_groupe);
        add_button->setObjectName("add_button");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/22x22/list-add.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        add_button->setIcon(icon2);
        add_button->setFlat(false);

        horizontalLayout->addWidget(add_button);

        m_previous_pb = new QPushButton(m_definition_groupe);
        m_previous_pb->setObjectName("m_previous_pb");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/arrow-left.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_previous_pb->setIcon(icon3);

        horizontalLayout->addWidget(m_previous_pb);

        m_next_pb = new QPushButton(m_definition_groupe);
        m_next_pb->setObjectName("m_next_pb");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/ico/16x16/arrow-right.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_next_pb->setIcon(icon4);

        horizontalLayout->addWidget(m_next_pb);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout_3->addLayout(horizontalLayout);

        editor_layout = new QVBoxLayout();
        editor_layout->setSpacing(0);
        editor_layout->setObjectName("editor_layout");
        editor_layout->setSizeConstraint(QLayout::SetDefaultConstraint);
        label_layout = new QHBoxLayout();
        label_layout->setSpacing(0);
        label_layout->setObjectName("label_layout");
        label_layout->setSizeConstraint(QLayout::SetDefaultConstraint);
        type_label = new QLabel(m_definition_groupe);
        type_label->setObjectName("type_label");
        type_label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        label_layout->addWidget(type_label);

        value_label = new QLabel(m_definition_groupe);
        value_label->setObjectName("value_label");
        value_label->setAlignment(Qt::AlignCenter);

        label_layout->addWidget(value_label);

        increase_label = new QLabel(m_definition_groupe);
        increase_label->setObjectName("increase_label");
        sizePolicy.setHeightForWidth(increase_label->sizePolicy().hasHeightForWidth());
        increase_label->setSizePolicy(sizePolicy);
        increase_label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        label_layout->addWidget(increase_label);


        editor_layout->addLayout(label_layout);


        verticalLayout_3->addLayout(editor_layout);


        verticalLayout_2->addWidget(m_definition_groupe);


        verticalLayout->addWidget(m_widget);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(SelectAutonumW);
        buttonBox->setObjectName("buttonBox");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy2);
        buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Help|QDialogButtonBox::Reset);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(SelectAutonumW);

        QMetaObject::connectSlotsByName(SelectAutonumW);
    } // setupUi

    void retranslateUi(QWidget *SelectAutonumW)
    {
        SelectAutonumW->setWindowTitle(QCoreApplication::translate("SelectAutonumW", "Form", nullptr));
        label->setText(QCoreApplication::translate("SelectAutonumW", "Num\303\251rotations disponibles :", nullptr));
        m_comboBox->setItemText(0, QCoreApplication::translate("SelectAutonumW", "Nom de la nouvelle num\303\251rotation", nullptr));

#if QT_CONFIG(tooltip)
        m_remove_pb->setToolTip(QCoreApplication::translate("SelectAutonumW", "Supprimer la num\303\251rotation", nullptr));
#endif // QT_CONFIG(tooltip)
        m_remove_pb->setText(QString());
        m_definition_groupe->setTitle(QCoreApplication::translate("SelectAutonumW", "D\303\251finition", nullptr));
#if QT_CONFIG(tooltip)
        remove_button->setToolTip(QCoreApplication::translate("SelectAutonumW", "<html><head/><body><p>Supprimer une variable de num\303\251rotation</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        remove_button->setText(QString());
#if QT_CONFIG(tooltip)
        add_button->setToolTip(QCoreApplication::translate("SelectAutonumW", "<html><head/><body><p>Ajouter une variable de num\303\251rotation</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        add_button->setText(QString());
#if QT_CONFIG(tooltip)
        m_previous_pb->setToolTip(QCoreApplication::translate("SelectAutonumW", "Pr\303\251c\303\251dent", nullptr));
#endif // QT_CONFIG(tooltip)
        m_previous_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_next_pb->setToolTip(QCoreApplication::translate("SelectAutonumW", "Suivant", nullptr));
#endif // QT_CONFIG(tooltip)
        m_next_pb->setText(QString());
        type_label->setText(QCoreApplication::translate("SelectAutonumW", "Type", nullptr));
        value_label->setText(QCoreApplication::translate("SelectAutonumW", "Valeur", nullptr));
        increase_label->setText(QCoreApplication::translate("SelectAutonumW", "Incr\303\251mentation", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SelectAutonumW: public Ui_SelectAutonumW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SELECTAUTONUMW_H
