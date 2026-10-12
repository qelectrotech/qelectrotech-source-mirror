/********************************************************************************
** Form generated from reading UI file 'masterpropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MASTERPROPERTIESWIDGET_H
#define UI_MASTERPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MasterPropertiesWidget
{
public:
    QGridLayout *gridLayout;
    QLabel *label_2;
    QTreeWidget *m_link_tree_widget;
    QTreeWidget *m_free_tree_widget;
    QLabel *label;
    QPushButton *unlink_button;
    QPushButton *link_button;

    void setupUi(QWidget *MasterPropertiesWidget)
    {
        if (MasterPropertiesWidget->objectName().isEmpty())
            MasterPropertiesWidget->setObjectName("MasterPropertiesWidget");
        MasterPropertiesWidget->resize(642, 666);
        MasterPropertiesWidget->setMinimumSize(QSize(300, 400));
        gridLayout = new QGridLayout(MasterPropertiesWidget);
        gridLayout->setObjectName("gridLayout");
        label_2 = new QLabel(MasterPropertiesWidget);
        label_2->setObjectName("label_2");
        label_2->setFrameShape(QFrame::NoFrame);
        label_2->setFrameShadow(QFrame::Plain);
        label_2->setScaledContents(false);
        label_2->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label_2, 2, 2, 1, 1);

        m_link_tree_widget = new QTreeWidget(MasterPropertiesWidget);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        m_link_tree_widget->setHeaderItem(__qtreewidgetitem);
        m_link_tree_widget->setObjectName("m_link_tree_widget");
        m_link_tree_widget->setFrameShape(QFrame::StyledPanel);
        m_link_tree_widget->setProperty("showDropIndicator", QVariant(false));
        m_link_tree_widget->setIconSize(QSize(32, 32));
        m_link_tree_widget->setIndentation(5);
        m_link_tree_widget->setUniformRowHeights(true);
        m_link_tree_widget->setSortingEnabled(true);
        m_link_tree_widget->header()->setDefaultSectionSize(80);
        m_link_tree_widget->header()->setStretchLastSection(true);

        gridLayout->addWidget(m_link_tree_widget, 3, 2, 1, 3);

        m_free_tree_widget = new QTreeWidget(MasterPropertiesWidget);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem();
        __qtreewidgetitem1->setText(0, QString::fromUtf8("1"));
        m_free_tree_widget->setHeaderItem(__qtreewidgetitem1);
        m_free_tree_widget->setObjectName("m_free_tree_widget");
        m_free_tree_widget->setProperty("showDropIndicator", QVariant(false));
        m_free_tree_widget->setIconSize(QSize(32, 32));
        m_free_tree_widget->setIndentation(5);
        m_free_tree_widget->setUniformRowHeights(true);
        m_free_tree_widget->setSortingEnabled(true);
        m_free_tree_widget->setWordWrap(false);
        m_free_tree_widget->header()->setVisible(true);
        m_free_tree_widget->header()->setCascadingSectionResizes(false);
        m_free_tree_widget->header()->setDefaultSectionSize(80);
        m_free_tree_widget->header()->setStretchLastSection(true);

        gridLayout->addWidget(m_free_tree_widget, 1, 2, 1, 3);

        label = new QLabel(MasterPropertiesWidget);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label, 0, 2, 1, 1);

        unlink_button = new QPushButton(MasterPropertiesWidget);
        unlink_button->setObjectName("unlink_button");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/go-up.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        unlink_button->setIcon(icon);

        gridLayout->addWidget(unlink_button, 2, 3, 1, 1);

        link_button = new QPushButton(MasterPropertiesWidget);
        link_button->setObjectName("link_button");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/go-down.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        link_button->setIcon(icon1);

        gridLayout->addWidget(link_button, 2, 4, 1, 1);


        retranslateUi(MasterPropertiesWidget);

        QMetaObject::connectSlotsByName(MasterPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *MasterPropertiesWidget)
    {
        MasterPropertiesWidget->setWindowTitle(QCoreApplication::translate("MasterPropertiesWidget", "Form", nullptr));
        label_2->setText(QCoreApplication::translate("MasterPropertiesWidget", "\303\211l\303\251ments li\303\251s", nullptr));
        label->setText(QCoreApplication::translate("MasterPropertiesWidget", "\303\211l\303\251ments disponibles", nullptr));
#if QT_CONFIG(tooltip)
        unlink_button->setToolTip(QCoreApplication::translate("MasterPropertiesWidget", "<html><head/><body><p>D\303\251lier l'\303\251l\303\251ment s\303\251lectionn\303\251</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        unlink_button->setText(QString());
#if QT_CONFIG(tooltip)
        link_button->setToolTip(QCoreApplication::translate("MasterPropertiesWidget", "<html><head/><body><p>Lier l'\303\251l\303\251ment s\303\251lectionn\303\251</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        link_button->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MasterPropertiesWidget: public Ui_MasterPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MASTERPROPERTIESWIDGET_H
