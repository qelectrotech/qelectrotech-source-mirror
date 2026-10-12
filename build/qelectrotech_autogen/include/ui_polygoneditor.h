/********************************************************************************
** Form generated from reading UI file 'polygoneditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_POLYGONEDITOR_H
#define UI_POLYGONEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PolygonEditor
{
public:
    QAction *m_add_point_action;
    QAction *m_remove_point_action;
    QVBoxLayout *m_main_layout;
    QLabel *label;
    QTreeWidget *m_points_list_tree;
    QCheckBox *m_close_polygon_cb;

    void setupUi(QWidget *PolygonEditor)
    {
        if (PolygonEditor->objectName().isEmpty())
            PolygonEditor->setObjectName("PolygonEditor");
        PolygonEditor->resize(400, 300);
        m_add_point_action = new QAction(PolygonEditor);
        m_add_point_action->setObjectName("m_add_point_action");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/list-add.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_add_point_action->setIcon(icon);
        m_remove_point_action = new QAction(PolygonEditor);
        m_remove_point_action->setObjectName("m_remove_point_action");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/list-remove.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_remove_point_action->setIcon(icon1);
        m_main_layout = new QVBoxLayout(PolygonEditor);
        m_main_layout->setObjectName("m_main_layout");
        label = new QLabel(PolygonEditor);
        label->setObjectName("label");

        m_main_layout->addWidget(label);

        m_points_list_tree = new QTreeWidget(PolygonEditor);
        m_points_list_tree->setObjectName("m_points_list_tree");
        m_points_list_tree->setContextMenuPolicy(Qt::ActionsContextMenu);

        m_main_layout->addWidget(m_points_list_tree);

        m_close_polygon_cb = new QCheckBox(PolygonEditor);
        m_close_polygon_cb->setObjectName("m_close_polygon_cb");

        m_main_layout->addWidget(m_close_polygon_cb);


        retranslateUi(PolygonEditor);

        QMetaObject::connectSlotsByName(PolygonEditor);
    } // setupUi

    void retranslateUi(QWidget *PolygonEditor)
    {
        PolygonEditor->setWindowTitle(QCoreApplication::translate("PolygonEditor", "Form", nullptr));
        m_add_point_action->setText(QCoreApplication::translate("PolygonEditor", "Ajouter un point", nullptr));
        m_remove_point_action->setText(QCoreApplication::translate("PolygonEditor", "Supprimer ce point", nullptr));
        label->setText(QCoreApplication::translate("PolygonEditor", "Points du polygone :", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = m_points_list_tree->headerItem();
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("PolygonEditor", "Y", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("PolygonEditor", "X", nullptr));
        m_close_polygon_cb->setText(QCoreApplication::translate("PolygonEditor", "Polygone ferm\303\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PolygonEditor: public Ui_PolygonEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_POLYGONEDITOR_H
