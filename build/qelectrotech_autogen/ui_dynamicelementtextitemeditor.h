/********************************************************************************
** Form generated from reading UI file 'dynamicelementtextitemeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DYNAMICELEMENTTEXTITEMEDITOR_H
#define UI_DYNAMICELEMENTTEXTITEMEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DynamicElementTextItemEditor
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QToolButton *m_export_pb;
    QToolButton *m_import_pb;
    QSpacerItem *horizontalSpacer_2;
    QToolButton *m_add_text;
    QToolButton *m_add_group;
    QToolButton *m_remove_selection;
    QTreeView *m_tree_view;
    QHBoxLayout *m_im_exp_layout;

    void setupUi(QWidget *DynamicElementTextItemEditor)
    {
        if (DynamicElementTextItemEditor->objectName().isEmpty())
            DynamicElementTextItemEditor->setObjectName("DynamicElementTextItemEditor");
        DynamicElementTextItemEditor->resize(226, 234);
        verticalLayout = new QVBoxLayout(DynamicElementTextItemEditor);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        m_export_pb = new QToolButton(DynamicElementTextItemEditor);
        m_export_pb->setObjectName("m_export_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/document-save.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_export_pb->setIcon(icon);

        horizontalLayout->addWidget(m_export_pb);

        m_import_pb = new QToolButton(DynamicElementTextItemEditor);
        m_import_pb->setObjectName("m_import_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/folder-open.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_import_pb->setIcon(icon1);

        horizontalLayout->addWidget(m_import_pb);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        m_add_text = new QToolButton(DynamicElementTextItemEditor);
        m_add_text->setObjectName("m_add_text");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/22x22/textfield.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_add_text->setIcon(icon2);

        horizontalLayout->addWidget(m_add_text);

        m_add_group = new QToolButton(DynamicElementTextItemEditor);
        m_add_group->setObjectName("m_add_group");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/object-group.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_add_group->setIcon(icon3);

        horizontalLayout->addWidget(m_add_group);

        m_remove_selection = new QToolButton(DynamicElementTextItemEditor);
        m_remove_selection->setObjectName("m_remove_selection");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/ico/16x16/list-remove.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_remove_selection->setIcon(icon4);

        horizontalLayout->addWidget(m_remove_selection);


        verticalLayout->addLayout(horizontalLayout);

        m_tree_view = new QTreeView(DynamicElementTextItemEditor);
        m_tree_view->setObjectName("m_tree_view");
        m_tree_view->setEditTriggers(QAbstractItemView::CurrentChanged|QAbstractItemView::SelectedClicked);
        m_tree_view->setDragDropMode(QAbstractItemView::DragDrop);
        m_tree_view->setAlternatingRowColors(true);
        m_tree_view->setAnimated(true);
        m_tree_view->header()->setDefaultSectionSize(150);

        verticalLayout->addWidget(m_tree_view);

        m_im_exp_layout = new QHBoxLayout();
        m_im_exp_layout->setObjectName("m_im_exp_layout");

        verticalLayout->addLayout(m_im_exp_layout);


        retranslateUi(DynamicElementTextItemEditor);

        QMetaObject::connectSlotsByName(DynamicElementTextItemEditor);
    } // setupUi

    void retranslateUi(QWidget *DynamicElementTextItemEditor)
    {
        DynamicElementTextItemEditor->setWindowTitle(QCoreApplication::translate("DynamicElementTextItemEditor", "Form", nullptr));
#if QT_CONFIG(tooltip)
        m_export_pb->setToolTip(QCoreApplication::translate("DynamicElementTextItemEditor", "Exporter l'actuelle configuration des textes", nullptr));
#endif // QT_CONFIG(tooltip)
        m_export_pb->setText(QCoreApplication::translate("DynamicElementTextItemEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        m_import_pb->setToolTip(QCoreApplication::translate("DynamicElementTextItemEditor", "Importer une configuration de texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_import_pb->setText(QCoreApplication::translate("DynamicElementTextItemEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        m_add_text->setToolTip(QCoreApplication::translate("DynamicElementTextItemEditor", "Ajouter un texte", nullptr));
#endif // QT_CONFIG(tooltip)
        m_add_text->setText(QCoreApplication::translate("DynamicElementTextItemEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        m_add_group->setToolTip(QCoreApplication::translate("DynamicElementTextItemEditor", "Ajouter un groupe de textes", nullptr));
#endif // QT_CONFIG(tooltip)
        m_add_group->setText(QCoreApplication::translate("DynamicElementTextItemEditor", "...", nullptr));
#if QT_CONFIG(tooltip)
        m_remove_selection->setToolTip(QCoreApplication::translate("DynamicElementTextItemEditor", "Supprimer la s\303\251lection", nullptr));
#endif // QT_CONFIG(tooltip)
        m_remove_selection->setText(QCoreApplication::translate("DynamicElementTextItemEditor", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DynamicElementTextItemEditor: public Ui_DynamicElementTextItemEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DYNAMICELEMENTTEXTITEMEDITOR_H
