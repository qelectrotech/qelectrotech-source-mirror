/********************************************************************************
** Form generated from reading UI file 'qetelementeditor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QETELEMENTEDITOR_H
#define UI_QETELEMENTEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QETElementEditor
{
public:
    QAction *m_select_all_act;
    QAction *m_new_action;
    QAction *m_open_action;
    QAction *m_open_from_file_action;
    QAction *m_save_action;
    QAction *m_save_as_action;
    QAction *m_save_as_file_action;
    QAction *m_export_svg_action;
    QAction *m_reload_action;
    QAction *m_quit_action;
    QAction *m_deselect_all_action;
    QAction *m_cut_action;
    QAction *m_copy_action;
    QAction *m_paste_action;
    QAction *m_paste_in_area_action;
    QAction *m_paste_from_file_action;
    QAction *m_paste_from_element_action;
    QAction *m_revert_selection_action;
    QAction *m_delete_action;
    QAction *m_edit_names_action;
    QAction *m_edit_author_action;
    QAction *m_edit_element_properties_action;
    QAction *m_zoom_in_action;
    QAction *m_zoom_out_action;
    QAction *m_zoom_fit_best_action;
    QAction *m_zoom_original_action;
    QAction *m_about_qet_action;
    QAction *m_online_manual;
    QAction *m_youtube_action;
    QAction *m_donate_action;
    QAction *m_about_qt_action;
    QAction *m_rotate_action;
    QAction *m_rotateFine_action;
    QAction *m_mirror_action;
    QAction *m_flip_action;
    QAction *m_import_dxf;
    QAction *m_import_scaled_element;
    QWidget *centralwidget;
    QStatusBar *statusbar;
    QMenuBar *menubar;
    QMenu *m_file_menu;
    QMenu *m_edit_menu;
    QMenu *m_paste_from_menu;
    QMenu *m_display_menu;
    QMenu *menu_Aide;
    QDockWidget *m_undo_dock;
    QWidget *dockWidgetContents;
    QDockWidget *m_parts_dock;
    QWidget *dockWidgetContents_2;
    QDockWidget *m_tools_dock;
    QWidget *dockWidgetContents_3;
    QToolBar *m_main_toolbar;
    QToolBar *m_view_toolbar;
    QToolBar *m_element_toolbar;
    QToolBar *m_undo_toolbar;

    void setupUi(QMainWindow *QETElementEditor)
    {
        if (QETElementEditor->objectName().isEmpty())
            QETElementEditor->setObjectName("QETElementEditor");
        QETElementEditor->resize(800, 600);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/qet.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        QETElementEditor->setWindowIcon(icon);
        m_select_all_act = new QAction(QETElementEditor);
        m_select_all_act->setObjectName("m_select_all_act");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/edit-select-all.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_select_all_act->setIcon(icon1);
        m_new_action = new QAction(QETElementEditor);
        m_new_action->setObjectName("m_new_action");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/22x22/document-new.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_new_action->setIcon(icon2);
        m_open_action = new QAction(QETElementEditor);
        m_open_action->setObjectName("m_open_action");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/22x22/folder-open.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_open_action->setIcon(icon3);
        m_open_from_file_action = new QAction(QETElementEditor);
        m_open_from_file_action->setObjectName("m_open_from_file_action");
        m_open_from_file_action->setIcon(icon3);
        m_save_action = new QAction(QETElementEditor);
        m_save_action->setObjectName("m_save_action");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/ico/22x22/document-save.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_save_action->setIcon(icon4);
        m_save_as_action = new QAction(QETElementEditor);
        m_save_as_action->setObjectName("m_save_as_action");
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/ico/22x22/document-save-as.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_save_as_action->setIcon(icon5);
        m_save_as_file_action = new QAction(QETElementEditor);
        m_save_as_file_action->setObjectName("m_save_as_file_action");
        m_save_as_file_action->setIcon(icon4);
        m_export_svg_action = new QAction(QETElementEditor);
        m_export_svg_action->setObjectName("m_export_svg_action");
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/ico/22x22/document-export.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_export_svg_action->setIcon(icon6);
        m_reload_action = new QAction(QETElementEditor);
        m_reload_action->setObjectName("m_reload_action");
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/ico/22x22/view-refresh.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_reload_action->setIcon(icon7);
        m_quit_action = new QAction(QETElementEditor);
        m_quit_action->setObjectName("m_quit_action");
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/ico/16x16/application-exit.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_quit_action->setIcon(icon8);
        m_deselect_all_action = new QAction(QETElementEditor);
        m_deselect_all_action->setObjectName("m_deselect_all_action");
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/ico/16x16/edit-select-none.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_deselect_all_action->setIcon(icon9);
        m_cut_action = new QAction(QETElementEditor);
        m_cut_action->setObjectName("m_cut_action");
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/ico/16x16/edit-cut.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_cut_action->setIcon(icon10);
        m_copy_action = new QAction(QETElementEditor);
        m_copy_action->setObjectName("m_copy_action");
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/ico/16x16/edit-copy.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_copy_action->setIcon(icon11);
        m_paste_action = new QAction(QETElementEditor);
        m_paste_action->setObjectName("m_paste_action");
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/ico/16x16/edit-paste.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_paste_action->setIcon(icon12);
        m_paste_in_area_action = new QAction(QETElementEditor);
        m_paste_in_area_action->setObjectName("m_paste_in_area_action");
        m_paste_in_area_action->setIcon(icon12);
        m_paste_from_file_action = new QAction(QETElementEditor);
        m_paste_from_file_action->setObjectName("m_paste_from_file_action");
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/ico/16x16/text-xml.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_paste_from_file_action->setIcon(icon13);
        m_paste_from_element_action = new QAction(QETElementEditor);
        m_paste_from_element_action->setObjectName("m_paste_from_element_action");
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/ico/16x16/element.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_paste_from_element_action->setIcon(icon14);
        m_revert_selection_action = new QAction(QETElementEditor);
        m_revert_selection_action->setObjectName("m_revert_selection_action");
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/ico/16x16/edit-select-invert.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_revert_selection_action->setIcon(icon15);
        m_delete_action = new QAction(QETElementEditor);
        m_delete_action->setObjectName("m_delete_action");
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/ico/22x22/edit-delete.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_delete_action->setIcon(icon16);
        m_edit_names_action = new QAction(QETElementEditor);
        m_edit_names_action->setObjectName("m_edit_names_action");
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/ico/22x22/names.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_edit_names_action->setIcon(icon17);
        m_edit_author_action = new QAction(QETElementEditor);
        m_edit_author_action->setObjectName("m_edit_author_action");
        QIcon icon18;
        icon18.addFile(QString::fromUtf8(":/ico/16x16/preferences-desktop-user.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_edit_author_action->setIcon(icon18);
        m_edit_element_properties_action = new QAction(QETElementEditor);
        m_edit_element_properties_action->setObjectName("m_edit_element_properties_action");
        QIcon icon19;
        icon19.addFile(QString::fromUtf8(":/ico/22x22/element-edit.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_edit_element_properties_action->setIcon(icon19);
        m_zoom_in_action = new QAction(QETElementEditor);
        m_zoom_in_action->setObjectName("m_zoom_in_action");
        QIcon icon20;
        icon20.addFile(QString::fromUtf8(":/ico/22x22/zoom-in.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_zoom_in_action->setIcon(icon20);
        m_zoom_out_action = new QAction(QETElementEditor);
        m_zoom_out_action->setObjectName("m_zoom_out_action");
        QIcon icon21;
        icon21.addFile(QString::fromUtf8(":/ico/22x22/zoom-out.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_zoom_out_action->setIcon(icon21);
        m_zoom_fit_best_action = new QAction(QETElementEditor);
        m_zoom_fit_best_action->setObjectName("m_zoom_fit_best_action");
        QIcon icon22;
        icon22.addFile(QString::fromUtf8(":/ico/22x22/zoom-fit-best.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_zoom_fit_best_action->setIcon(icon22);
        m_zoom_original_action = new QAction(QETElementEditor);
        m_zoom_original_action->setObjectName("m_zoom_original_action");
        QIcon icon23;
        icon23.addFile(QString::fromUtf8(":/ico/22x22/zoom-original.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_zoom_original_action->setIcon(icon23);
        m_about_qet_action = new QAction(QETElementEditor);
        m_about_qet_action->setObjectName("m_about_qet_action");
        m_about_qet_action->setIcon(icon);
        m_online_manual = new QAction(QETElementEditor);
        m_online_manual->setObjectName("m_online_manual");
        QIcon icon24;
        icon24.addFile(QString::fromUtf8(":/ico/16x16/help-contents.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_online_manual->setIcon(icon24);
        m_youtube_action = new QAction(QETElementEditor);
        m_youtube_action->setObjectName("m_youtube_action");
        QIcon icon25;
        icon25.addFile(QString::fromUtf8(":/ico/16x16/kdenlive-show-video.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_youtube_action->setIcon(icon25);
        m_donate_action = new QAction(QETElementEditor);
        m_donate_action->setObjectName("m_donate_action");
        QIcon icon26;
        icon26.addFile(QString::fromUtf8(":/ico/16x16/help-donate.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_donate_action->setIcon(icon26);
        m_about_qt_action = new QAction(QETElementEditor);
        m_about_qt_action->setObjectName("m_about_qt_action");
        QIcon icon27;
        icon27.addFile(QString::fromUtf8(":/ico/16x16/qt.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_about_qt_action->setIcon(icon27);
        m_rotate_action = new QAction(QETElementEditor);
        m_rotate_action->setObjectName("m_rotate_action");
        QIcon icon28;
        icon28.addFile(QString::fromUtf8(":/ico/16x16/orientations.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_rotate_action->setIcon(icon28);
        m_rotateFine_action = new QAction(QETElementEditor);
        m_rotateFine_action->setObjectName("m_rotateFine_action");
        m_rotateFine_action->setIcon(icon28);
        m_mirror_action = new QAction(QETElementEditor);
        m_mirror_action->setObjectName("m_mirror_action");
        QIcon icon29;
        icon29.addFile(QString::fromUtf8(":/ico/16x16/mirror.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_mirror_action->setIcon(icon29);
        m_flip_action = new QAction(QETElementEditor);
        m_flip_action->setObjectName("m_flip_action");
        QIcon icon30;
        icon30.addFile(QString::fromUtf8(":/ico/16x16/flip.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_flip_action->setIcon(icon30);
        m_import_dxf = new QAction(QETElementEditor);
        m_import_dxf->setObjectName("m_import_dxf");
        QIcon icon31;
        icon31.addFile(QString::fromUtf8(":/ico/16x16/run-dxf.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_import_dxf->setIcon(icon31);
        m_import_scaled_element = new QAction(QETElementEditor);
        m_import_scaled_element->setObjectName("m_import_scaled_element");
        m_import_scaled_element->setIcon(icon3);
        centralwidget = new QWidget(QETElementEditor);
        centralwidget->setObjectName("centralwidget");
        QETElementEditor->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(QETElementEditor);
        statusbar->setObjectName("statusbar");
        QETElementEditor->setStatusBar(statusbar);
        menubar = new QMenuBar(QETElementEditor);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 22));
        m_file_menu = new QMenu(menubar);
        m_file_menu->setObjectName("m_file_menu");
        m_edit_menu = new QMenu(menubar);
        m_edit_menu->setObjectName("m_edit_menu");
        m_paste_from_menu = new QMenu(m_edit_menu);
        m_paste_from_menu->setObjectName("m_paste_from_menu");
        m_paste_from_menu->setIcon(icon12);
        m_display_menu = new QMenu(menubar);
        m_display_menu->setObjectName("m_display_menu");
        menu_Aide = new QMenu(menubar);
        menu_Aide->setObjectName("menu_Aide");
        QETElementEditor->setMenuBar(menubar);
        m_undo_dock = new QDockWidget(QETElementEditor);
        m_undo_dock->setObjectName("m_undo_dock");
        m_undo_dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName("dockWidgetContents");
        m_undo_dock->setWidget(dockWidgetContents);
        QETElementEditor->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_undo_dock);
        m_parts_dock = new QDockWidget(QETElementEditor);
        m_parts_dock->setObjectName("m_parts_dock");
        m_parts_dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
        dockWidgetContents_2 = new QWidget();
        dockWidgetContents_2->setObjectName("dockWidgetContents_2");
        m_parts_dock->setWidget(dockWidgetContents_2);
        QETElementEditor->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, m_parts_dock);
        m_tools_dock = new QDockWidget(QETElementEditor);
        m_tools_dock->setObjectName("m_tools_dock");
        m_tools_dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName("dockWidgetContents_3");
        m_tools_dock->setWidget(dockWidgetContents_3);
        QETElementEditor->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_tools_dock);
        m_main_toolbar = new QToolBar(QETElementEditor);
        m_main_toolbar->setObjectName("m_main_toolbar");
        QETElementEditor->addToolBar(Qt::ToolBarArea::TopToolBarArea, m_main_toolbar);
        m_view_toolbar = new QToolBar(QETElementEditor);
        m_view_toolbar->setObjectName("m_view_toolbar");
        QETElementEditor->addToolBar(Qt::ToolBarArea::TopToolBarArea, m_view_toolbar);
        m_element_toolbar = new QToolBar(QETElementEditor);
        m_element_toolbar->setObjectName("m_element_toolbar");
        QETElementEditor->addToolBar(Qt::ToolBarArea::TopToolBarArea, m_element_toolbar);
        m_undo_toolbar = new QToolBar(QETElementEditor);
        m_undo_toolbar->setObjectName("m_undo_toolbar");
        QETElementEditor->addToolBar(Qt::ToolBarArea::TopToolBarArea, m_undo_toolbar);

        menubar->addAction(m_file_menu->menuAction());
        menubar->addAction(m_edit_menu->menuAction());
        menubar->addAction(m_display_menu->menuAction());
        menubar->addAction(menu_Aide->menuAction());
        m_file_menu->addAction(m_new_action);
        m_file_menu->addAction(m_open_action);
        m_file_menu->addAction(m_open_from_file_action);
        m_file_menu->addAction(m_import_scaled_element);
        m_file_menu->addAction(m_import_dxf);
        m_file_menu->addAction(m_save_action);
        m_file_menu->addAction(m_save_as_action);
        m_file_menu->addAction(m_save_as_file_action);
        m_file_menu->addAction(m_export_svg_action);
        m_file_menu->addSeparator();
        m_file_menu->addAction(m_reload_action);
        m_file_menu->addSeparator();
        m_file_menu->addAction(m_quit_action);
        m_edit_menu->addSeparator();
        m_edit_menu->addAction(m_select_all_act);
        m_edit_menu->addAction(m_deselect_all_action);
        m_edit_menu->addAction(m_revert_selection_action);
        m_edit_menu->addSeparator();
        m_edit_menu->addAction(m_cut_action);
        m_edit_menu->addAction(m_copy_action);
        m_edit_menu->addAction(m_paste_action);
        m_edit_menu->addAction(m_paste_in_area_action);
        m_edit_menu->addAction(m_paste_from_menu->menuAction());
        m_edit_menu->addSeparator();
        m_edit_menu->addAction(m_delete_action);
        m_edit_menu->addSeparator();
        m_edit_menu->addAction(m_edit_names_action);
        m_edit_menu->addAction(m_edit_author_action);
        m_edit_menu->addAction(m_edit_element_properties_action);
        m_edit_menu->addAction(m_rotate_action);
        m_edit_menu->addAction(m_rotateFine_action);
        m_edit_menu->addAction(m_mirror_action);
        m_edit_menu->addAction(m_flip_action);
        m_paste_from_menu->addAction(m_paste_from_file_action);
        m_paste_from_menu->addAction(m_paste_from_element_action);
        m_display_menu->addAction(m_zoom_in_action);
        m_display_menu->addAction(m_zoom_out_action);
        m_display_menu->addAction(m_zoom_fit_best_action);
        m_display_menu->addAction(m_zoom_original_action);
        m_display_menu->addSeparator();
        menu_Aide->addSeparator();
        menu_Aide->addAction(m_online_manual);
        menu_Aide->addAction(m_youtube_action);
        menu_Aide->addAction(m_donate_action);
        menu_Aide->addAction(m_about_qt_action);
        menu_Aide->addAction(m_about_qet_action);
        m_main_toolbar->addAction(m_new_action);
        m_main_toolbar->addAction(m_open_action);
        m_main_toolbar->addAction(m_save_action);
        m_main_toolbar->addAction(m_save_as_action);
        m_main_toolbar->addAction(m_reload_action);
        m_main_toolbar->addAction(m_delete_action);
        m_view_toolbar->addAction(m_zoom_in_action);
        m_view_toolbar->addAction(m_zoom_out_action);
        m_view_toolbar->addAction(m_zoom_fit_best_action);
        m_view_toolbar->addAction(m_zoom_original_action);
        m_element_toolbar->addAction(m_edit_names_action);
        m_element_toolbar->addAction(m_edit_element_properties_action);

        retranslateUi(QETElementEditor);

        QMetaObject::connectSlotsByName(QETElementEditor);
    } // setupUi

    void retranslateUi(QMainWindow *QETElementEditor)
    {
        QETElementEditor->setWindowTitle(QCoreApplication::translate("QETElementEditor", "QElectroTech - \303\211diteur d'\303\251l\303\251ment", nullptr));
        m_select_all_act->setText(QCoreApplication::translate("QETElementEditor", "Tout s\303\251lectionner", nullptr));
        m_new_action->setText(QCoreApplication::translate("QETElementEditor", "&Nouveau", nullptr));
        m_open_action->setText(QCoreApplication::translate("QETElementEditor", "&Ouvrir", nullptr));
        m_open_from_file_action->setText(QCoreApplication::translate("QETElementEditor", "&Ouvrir depuis un fichier", nullptr));
        m_save_action->setText(QCoreApplication::translate("QETElementEditor", "&Enregistrer", nullptr));
        m_save_as_action->setText(QCoreApplication::translate("QETElementEditor", "Enregistrer sous", nullptr));
        m_save_as_file_action->setText(QCoreApplication::translate("QETElementEditor", "Enregistrer dans un fichier", nullptr));
        m_export_svg_action->setText(QCoreApplication::translate("QETElementEditor", "Exporter en SVG", nullptr));
        m_reload_action->setText(QCoreApplication::translate("QETElementEditor", "Recharger", nullptr));
        m_quit_action->setText(QCoreApplication::translate("QETElementEditor", "&Fermer cet \303\251diteur", nullptr));
#if QT_CONFIG(tooltip)
        m_quit_action->setToolTip(QCoreApplication::translate("QETElementEditor", "Fermer cet \303\251diteur", nullptr));
#endif // QT_CONFIG(tooltip)
        m_deselect_all_action->setText(QCoreApplication::translate("QETElementEditor", "D\303\251s\303\251lectionner tout", nullptr));
        m_cut_action->setText(QCoreApplication::translate("QETElementEditor", "Co&uper", nullptr));
        m_copy_action->setText(QCoreApplication::translate("QETElementEditor", "Cop&ier", nullptr));
        m_paste_action->setText(QCoreApplication::translate("QETElementEditor", "C&oller", nullptr));
        m_paste_in_area_action->setText(QCoreApplication::translate("QETElementEditor", "C&oller dans la zone", nullptr));
        m_paste_from_file_action->setText(QCoreApplication::translate("QETElementEditor", "Un fichier", nullptr));
        m_paste_from_element_action->setText(QCoreApplication::translate("QETElementEditor", "Un \303\251l\303\251ment", nullptr));
        m_revert_selection_action->setText(QCoreApplication::translate("QETElementEditor", "Inverser la s\303\251lection", nullptr));
        m_delete_action->setText(QCoreApplication::translate("QETElementEditor", "&Supprimer", nullptr));
        m_edit_names_action->setText(QCoreApplication::translate("QETElementEditor", "\303\211diter le nom et les traductions de l'\303\251l\303\251ment", nullptr));
        m_edit_author_action->setText(QCoreApplication::translate("QETElementEditor", "\303\211diter les informations sur l'auteur", nullptr));
        m_edit_element_properties_action->setText(QCoreApplication::translate("QETElementEditor", "\303\211diter les propri\303\251t\303\251s de l'\303\251l\303\251ment", nullptr));
        m_zoom_in_action->setText(QCoreApplication::translate("QETElementEditor", "Zoom avant", nullptr));
        m_zoom_out_action->setText(QCoreApplication::translate("QETElementEditor", "Zoom arri\303\250re", nullptr));
        m_zoom_fit_best_action->setText(QCoreApplication::translate("QETElementEditor", "Zoom adapt\303\251", nullptr));
        m_zoom_original_action->setText(QCoreApplication::translate("QETElementEditor", "Pas de zoom", nullptr));
        m_about_qet_action->setText(QCoreApplication::translate("QETElementEditor", "\303\200 &propos de QElectroTech", nullptr));
#if QT_CONFIG(statustip)
        m_about_qet_action->setStatusTip(QCoreApplication::translate("QETElementEditor", "Affiche des informations sur QElectroTech", nullptr));
#endif // QT_CONFIG(statustip)
        m_online_manual->setText(QCoreApplication::translate("QETElementEditor", "Manuel en ligne", nullptr));
#if QT_CONFIG(statustip)
        m_online_manual->setStatusTip(QCoreApplication::translate("QETElementEditor", "Lance le navigateur par d\303\251faut vers le manuel en ligne de QElectroTech", nullptr));
#endif // QT_CONFIG(statustip)
        m_youtube_action->setText(QCoreApplication::translate("QETElementEditor", "Chaine Youtube", nullptr));
#if QT_CONFIG(statustip)
        m_youtube_action->setStatusTip(QCoreApplication::translate("QETElementEditor", "Lance le navigateur par d\303\251faut vers la chaine Youtube de QElectroTech", nullptr));
#endif // QT_CONFIG(statustip)
        m_donate_action->setText(QCoreApplication::translate("QETElementEditor", "Soutenir le projet par un don", nullptr));
#if QT_CONFIG(statustip)
        m_donate_action->setStatusTip(QCoreApplication::translate("QETElementEditor", "Soutenir le projet QElectroTech par un don", nullptr));
#endif // QT_CONFIG(statustip)
        m_about_qt_action->setText(QCoreApplication::translate("QETElementEditor", "\303\200 propos de &Qt", nullptr));
#if QT_CONFIG(statustip)
        m_about_qt_action->setStatusTip(QCoreApplication::translate("QETElementEditor", "Affiche des informations sur la biblioth\303\250que Qt", nullptr));
#endif // QT_CONFIG(statustip)
        m_rotate_action->setText(QCoreApplication::translate("QETElementEditor", "Rotation", nullptr));
        m_rotateFine_action->setText(QCoreApplication::translate("QETElementEditor", "Fine-Rotation", nullptr));
        m_mirror_action->setText(QCoreApplication::translate("QETElementEditor", "Mirror", nullptr));
        m_flip_action->setText(QCoreApplication::translate("QETElementEditor", "Flip", nullptr));
        m_import_dxf->setText(QCoreApplication::translate("QETElementEditor", "Importer un dxf", nullptr));
        m_import_scaled_element->setText(QCoreApplication::translate("QETElementEditor", "importer un \303\251l\303\251ment \303\240 redimensionner", nullptr));
        m_file_menu->setTitle(QCoreApplication::translate("QETElementEditor", "&Fichier", nullptr));
        m_edit_menu->setTitle(QCoreApplication::translate("QETElementEditor", "&\303\211dition", nullptr));
        m_paste_from_menu->setTitle(QCoreApplication::translate("QETElementEditor", "Coller depuis...", nullptr));
        m_display_menu->setTitle(QCoreApplication::translate("QETElementEditor", "Afficha&ge", nullptr));
        menu_Aide->setTitle(QCoreApplication::translate("QETElementEditor", "&Aide", nullptr));
        m_undo_dock->setWindowTitle(QCoreApplication::translate("QETElementEditor", "Annulations", nullptr));
        m_parts_dock->setWindowTitle(QCoreApplication::translate("QETElementEditor", "Parties", nullptr));
        m_tools_dock->setWindowTitle(QCoreApplication::translate("QETElementEditor", "Informations", nullptr));
        m_main_toolbar->setWindowTitle(QCoreApplication::translate("QETElementEditor", "Outils", nullptr));
        m_view_toolbar->setWindowTitle(QCoreApplication::translate("QETElementEditor", "Affichage", nullptr));
        m_element_toolbar->setWindowTitle(QCoreApplication::translate("QETElementEditor", "\303\211l\303\251ment", nullptr));
        m_undo_toolbar->setWindowTitle(QCoreApplication::translate("QETElementEditor", "Annulation", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QETElementEditor: public Ui_QETElementEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QETELEMENTEDITOR_H
