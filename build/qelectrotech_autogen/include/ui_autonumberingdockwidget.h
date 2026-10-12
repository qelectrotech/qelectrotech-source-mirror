/********************************************************************************
** Form generated from reading UI file 'autonumberingdockwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUTONUMBERINGDOCKWIDGET_H
#define UI_AUTONUMBERINGDOCKWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AutoNumberingDockWidget
{
public:
    QWidget *dockWidgetContents;
    QGridLayout *gridLayout;
    QLabel *value_header_label;
    QLabel *increase_header_label;
    QLabel *next_header_label;
    QComboBox *m_element_cb;
    QLabel *label_3;
    QComboBox *m_conductor_cb;
    QPushButton *m_conductor_reset_start_pb;
    QLineEdit *m_conductor_value_le;
    QSpinBox *m_conductor_increase_sb;
    QLineEdit *m_conductor_next_le;
    QLabel *label;
    QLabel *label_2;
    QPushButton *m_element_reset_start_pb;
    QLineEdit *m_element_value_le;
    QSpinBox *m_element_increase_sb;
    QLineEdit *m_element_next_le;
    QComboBox *m_folio_cb;
    QPushButton *m_folio_reset_start_pb;
    QLineEdit *m_folio_value_le;
    QSpinBox *m_folio_increase_sb;
    QLineEdit *m_folio_next_le;
    QSpacerItem *verticalSpacer;
    QPushButton *m_configure_pb;

    void setupUi(QDockWidget *AutoNumberingDockWidget)
    {
        if (AutoNumberingDockWidget->objectName().isEmpty())
            AutoNumberingDockWidget->setObjectName("AutoNumberingDockWidget");
        AutoNumberingDockWidget->resize(388, 335);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName("dockWidgetContents");
        gridLayout = new QGridLayout(dockWidgetContents);
        gridLayout->setObjectName("gridLayout");
        value_header_label = new QLabel(dockWidgetContents);
        value_header_label->setObjectName("value_header_label");
        value_header_label->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(value_header_label, 0, 3, 1, 1);

        increase_header_label = new QLabel(dockWidgetContents);
        increase_header_label->setObjectName("increase_header_label");
        increase_header_label->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(increase_header_label, 0, 4, 1, 1);

        next_header_label = new QLabel(dockWidgetContents);
        next_header_label->setObjectName("next_header_label");
        next_header_label->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(next_header_label, 0, 5, 1, 1);

        m_element_cb = new QComboBox(dockWidgetContents);
        m_element_cb->setObjectName("m_element_cb");

        gridLayout->addWidget(m_element_cb, 3, 1, 1, 1);

        label_3 = new QLabel(dockWidgetContents);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 4, 0, 1, 1);

        m_conductor_cb = new QComboBox(dockWidgetContents);
        m_conductor_cb->setObjectName("m_conductor_cb");

        gridLayout->addWidget(m_conductor_cb, 2, 1, 1, 1);

        m_conductor_reset_start_pb = new QPushButton(dockWidgetContents);
        m_conductor_reset_start_pb->setObjectName("m_conductor_reset_start_pb");
        m_conductor_reset_start_pb->setMaximumSize(QSize(24, 16777215));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/view-refresh.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_conductor_reset_start_pb->setIcon(icon);

        gridLayout->addWidget(m_conductor_reset_start_pb, 2, 2, 1, 1);

        m_conductor_value_le = new QLineEdit(dockWidgetContents);
        m_conductor_value_le->setObjectName("m_conductor_value_le");
        m_conductor_value_le->setMaximumSize(QSize(70, 16777215));

        gridLayout->addWidget(m_conductor_value_le, 2, 3, 1, 1);

        m_conductor_increase_sb = new QSpinBox(dockWidgetContents);
        m_conductor_increase_sb->setObjectName("m_conductor_increase_sb");
        m_conductor_increase_sb->setMaximumSize(QSize(55, 16777215));
        m_conductor_increase_sb->setAlignment(Qt::AlignCenter);
        m_conductor_increase_sb->setAccelerated(true);
        m_conductor_increase_sb->setMinimum(0);

        gridLayout->addWidget(m_conductor_increase_sb, 2, 4, 1, 1);

        m_conductor_next_le = new QLineEdit(dockWidgetContents);
        m_conductor_next_le->setObjectName("m_conductor_next_le");
        m_conductor_next_le->setMaximumSize(QSize(70, 16777215));
        m_conductor_next_le->setReadOnly(true);
        m_conductor_next_le->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(m_conductor_next_le, 2, 5, 1, 1);

        label = new QLabel(dockWidgetContents);
        label->setObjectName("label");

        gridLayout->addWidget(label, 3, 0, 1, 1);

        label_2 = new QLabel(dockWidgetContents);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        m_element_reset_start_pb = new QPushButton(dockWidgetContents);
        m_element_reset_start_pb->setObjectName("m_element_reset_start_pb");
        m_element_reset_start_pb->setMaximumSize(QSize(24, 16777215));
        m_element_reset_start_pb->setIcon(icon);

        gridLayout->addWidget(m_element_reset_start_pb, 3, 2, 1, 1);

        m_element_value_le = new QLineEdit(dockWidgetContents);
        m_element_value_le->setObjectName("m_element_value_le");
        m_element_value_le->setMaximumSize(QSize(70, 16777215));

        gridLayout->addWidget(m_element_value_le, 3, 3, 1, 1);

        m_element_increase_sb = new QSpinBox(dockWidgetContents);
        m_element_increase_sb->setObjectName("m_element_increase_sb");
        m_element_increase_sb->setMaximumSize(QSize(55, 16777215));
        m_element_increase_sb->setAlignment(Qt::AlignCenter);
        m_element_increase_sb->setAccelerated(true);
        m_element_increase_sb->setMinimum(0);

        gridLayout->addWidget(m_element_increase_sb, 3, 4, 1, 1);

        m_element_next_le = new QLineEdit(dockWidgetContents);
        m_element_next_le->setObjectName("m_element_next_le");
        m_element_next_le->setMaximumSize(QSize(70, 16777215));
        m_element_next_le->setReadOnly(true);
        m_element_next_le->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(m_element_next_le, 3, 5, 1, 1);

        m_folio_cb = new QComboBox(dockWidgetContents);
        m_folio_cb->setObjectName("m_folio_cb");

        gridLayout->addWidget(m_folio_cb, 4, 1, 1, 1);

        m_folio_reset_start_pb = new QPushButton(dockWidgetContents);
        m_folio_reset_start_pb->setObjectName("m_folio_reset_start_pb");
        m_folio_reset_start_pb->setMaximumSize(QSize(24, 16777215));
        m_folio_reset_start_pb->setIcon(icon);

        gridLayout->addWidget(m_folio_reset_start_pb, 4, 2, 1, 1);

        m_folio_value_le = new QLineEdit(dockWidgetContents);
        m_folio_value_le->setObjectName("m_folio_value_le");
        m_folio_value_le->setMaximumSize(QSize(70, 16777215));

        gridLayout->addWidget(m_folio_value_le, 4, 3, 1, 1);

        m_folio_increase_sb = new QSpinBox(dockWidgetContents);
        m_folio_increase_sb->setObjectName("m_folio_increase_sb");
        m_folio_increase_sb->setMaximumSize(QSize(55, 16777215));
        m_folio_increase_sb->setAlignment(Qt::AlignCenter);
        m_folio_increase_sb->setAccelerated(true);
        m_folio_increase_sb->setMinimum(0);

        gridLayout->addWidget(m_folio_increase_sb, 4, 4, 1, 1);

        m_folio_next_le = new QLineEdit(dockWidgetContents);
        m_folio_next_le->setObjectName("m_folio_next_le");
        m_folio_next_le->setMaximumSize(QSize(70, 16777215));
        m_folio_next_le->setReadOnly(true);
        m_folio_next_le->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(m_folio_next_le, 4, 5, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 6, 0, 1, 1);

        m_configure_pb = new QPushButton(dockWidgetContents);
        m_configure_pb->setObjectName("m_configure_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/configure.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_configure_pb->setIcon(icon1);

        gridLayout->addWidget(m_configure_pb, 5, 1, 1, 1);

        AutoNumberingDockWidget->setWidget(dockWidgetContents);

        retranslateUi(AutoNumberingDockWidget);

        QMetaObject::connectSlotsByName(AutoNumberingDockWidget);
    } // setupUi

    void retranslateUi(QDockWidget *AutoNumberingDockWidget)
    {
        AutoNumberingDockWidget->setWindowTitle(QCoreApplication::translate("AutoNumberingDockWidget", "S\303\251lection num\303\251rotation auto", nullptr));
        value_header_label->setText(QCoreApplication::translate("AutoNumberingDockWidget", "Valeur", nullptr));
        increase_header_label->setText(QCoreApplication::translate("AutoNumberingDockWidget", "Incr\303\251ment", nullptr));
        next_header_label->setText(QCoreApplication::translate("AutoNumberingDockWidget", "Suivant", nullptr));
        label_3->setText(QCoreApplication::translate("AutoNumberingDockWidget", "Folio", nullptr));
#if QT_CONFIG(tooltip)
        m_conductor_reset_start_pb->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "R\303\251initialiser \303\240 la valeur de d\303\251part", nullptr));
#endif // QT_CONFIG(tooltip)
        m_conductor_reset_start_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_conductor_value_le->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Valeur actuelle du compteur. Saisir une nouvelle valeur et valider pour la modifier.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_conductor_increase_sb->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Incr\303\251ment : valeur ajout\303\251e au compteur \303\240 chaque nouvelle num\303\251rotation", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_conductor_next_le->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Prochaine valeur qui sera appliqu\303\251e avec cet incr\303\251ment", nullptr));
#endif // QT_CONFIG(tooltip)
        label->setText(QCoreApplication::translate("AutoNumberingDockWidget", "Element", nullptr));
        label_2->setText(QCoreApplication::translate("AutoNumberingDockWidget", "Conducteur", nullptr));
#if QT_CONFIG(tooltip)
        m_element_reset_start_pb->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "R\303\251initialiser \303\240 la valeur de d\303\251part", nullptr));
#endif // QT_CONFIG(tooltip)
        m_element_reset_start_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_element_value_le->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Valeur actuelle du compteur. Saisir une nouvelle valeur et valider pour la modifier.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_element_increase_sb->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Incr\303\251ment : valeur ajout\303\251e au compteur \303\240 chaque nouvelle num\303\251rotation", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_element_next_le->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Prochaine valeur qui sera appliqu\303\251e avec cet incr\303\251ment", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_folio_reset_start_pb->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "R\303\251initialiser \303\240 la valeur de d\303\251part", nullptr));
#endif // QT_CONFIG(tooltip)
        m_folio_reset_start_pb->setText(QString());
#if QT_CONFIG(tooltip)
        m_folio_value_le->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Valeur actuelle du compteur. Saisir une nouvelle valeur et valider pour la modifier.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_folio_increase_sb->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Incr\303\251ment : valeur ajout\303\251e au compteur \303\240 chaque nouvelle num\303\251rotation", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_folio_next_le->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Prochaine valeur qui sera appliqu\303\251e avec cet incr\303\251ment", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        m_configure_pb->setToolTip(QCoreApplication::translate("AutoNumberingDockWidget", "Configurer les r\303\250gles d'auto num\303\251rotation", nullptr));
#endif // QT_CONFIG(tooltip)
        m_configure_pb->setText(QCoreApplication::translate("AutoNumberingDockWidget", "Configurer", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AutoNumberingDockWidget: public Ui_AutoNumberingDockWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUTONUMBERINGDOCKWIDGET_H
