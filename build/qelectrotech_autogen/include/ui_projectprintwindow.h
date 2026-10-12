/********************************************************************************
** Form generated from reading UI file 'projectprintwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROJECTPRINTWINDOW_H
#define UI_PROJECTPRINTWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ProjectPrintWindow
{
public:
    QAction *m_adjust_width_action;
    QAction *m_adjust_page_action;
    QAction *m_zoom_out_action;
    QAction *m_zoom_in_action;
    QAction *m_landscape_action;
    QAction *m_portrait_action;
    QAction *m_first_page_action;
    QAction *m_previous_page_action;
    QAction *m_next_page_action;
    QAction *m_last_page_action;
    QAction *m_display_single_page_action;
    QAction *m_display_two_page_action;
    QAction *m_display_all_page_action;
    QAction *m_page_setup;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *m_vertical_layout;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_3;
    QScrollArea *m_diagram_list;
    QWidget *scrollAreaWidgetContents;
    QPushButton *m_check_all_pb;
    QPushButton *m_uncheck_all;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QComboBox *m_date_from_cb;
    QDateEdit *m_date_cb;
    QPushButton *m_apply_date_pb;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QCheckBox *m_draw_border_cb;
    QCheckBox *m_draw_titleblock_cb;
    QCheckBox *m_keep_conductor_color_cb;
    QCheckBox *m_draw_terminal_cb;
    QCheckBox *m_draw_terminal_names_cb;
    QCheckBox *m_component_info_cb;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QCheckBox *m_fit_in_page_cb;
    QCheckBox *m_use_full_page_cb;
    QLabel *label_2;
    QLabel *label;
    QFrame *line;
    QDialogButtonBox *m_button_box;
    QToolBar *toolBar;

    void setupUi(QMainWindow *ProjectPrintWindow)
    {
        if (ProjectPrintWindow->objectName().isEmpty())
            ProjectPrintWindow->setObjectName("ProjectPrintWindow");
        ProjectPrintWindow->resize(956, 537);
        QSizePolicy sizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ProjectPrintWindow->sizePolicy().hasHeightForWidth());
        ProjectPrintWindow->setSizePolicy(sizePolicy);
        m_adjust_width_action = new QAction(ProjectPrintWindow);
        m_adjust_width_action->setObjectName("m_adjust_width_action");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/22x22/view_fit_width.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_adjust_width_action->setIcon(icon);
        m_adjust_page_action = new QAction(ProjectPrintWindow);
        m_adjust_page_action->setObjectName("m_adjust_page_action");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/22x22/view-fit-window.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_adjust_page_action->setIcon(icon1);
        m_zoom_out_action = new QAction(ProjectPrintWindow);
        m_zoom_out_action->setObjectName("m_zoom_out_action");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/ico/16x16/zoom-out.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_zoom_out_action->setIcon(icon2);
        m_zoom_in_action = new QAction(ProjectPrintWindow);
        m_zoom_in_action->setObjectName("m_zoom_in_action");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/ico/16x16/zoom-in.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_zoom_in_action->setIcon(icon3);
        m_landscape_action = new QAction(ProjectPrintWindow);
        m_landscape_action->setObjectName("m_landscape_action");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/ico/22x22/landscape.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_landscape_action->setIcon(icon4);
        m_portrait_action = new QAction(ProjectPrintWindow);
        m_portrait_action->setObjectName("m_portrait_action");
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/ico/22x22/portrait.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_portrait_action->setIcon(icon5);
        m_first_page_action = new QAction(ProjectPrintWindow);
        m_first_page_action->setObjectName("m_first_page_action");
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/ico/16x16/arrow-left-double.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_first_page_action->setIcon(icon6);
        m_previous_page_action = new QAction(ProjectPrintWindow);
        m_previous_page_action->setObjectName("m_previous_page_action");
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/ico/16x16/arrow-left.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_previous_page_action->setIcon(icon7);
        m_next_page_action = new QAction(ProjectPrintWindow);
        m_next_page_action->setObjectName("m_next_page_action");
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/ico/16x16/arrow-right.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_next_page_action->setIcon(icon8);
        m_last_page_action = new QAction(ProjectPrintWindow);
        m_last_page_action->setObjectName("m_last_page_action");
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/ico/22x22/arrow-right-double.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_last_page_action->setIcon(icon9);
        m_display_single_page_action = new QAction(ProjectPrintWindow);
        m_display_single_page_action->setObjectName("m_display_single_page_action");
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/ico/22x22/single_page.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_display_single_page_action->setIcon(icon10);
        m_display_two_page_action = new QAction(ProjectPrintWindow);
        m_display_two_page_action->setObjectName("m_display_two_page_action");
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/ico/22x22/two_pages.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_display_two_page_action->setIcon(icon11);
        m_display_all_page_action = new QAction(ProjectPrintWindow);
        m_display_all_page_action->setObjectName("m_display_all_page_action");
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/ico/22x22/all_pages.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_display_all_page_action->setIcon(icon12);
        m_page_setup = new QAction(ProjectPrintWindow);
        m_page_setup->setObjectName("m_page_setup");
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/ico/48x48/document-print-frame.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_page_setup->setIcon(icon13);
        centralwidget = new QWidget(ProjectPrintWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout_3 = new QVBoxLayout(centralwidget);
        verticalLayout_3->setObjectName("verticalLayout_3");
        m_vertical_layout = new QHBoxLayout();
        m_vertical_layout->setObjectName("m_vertical_layout");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");

        verticalLayout_2->addWidget(label_3);

        m_diagram_list = new QScrollArea(centralwidget);
        m_diagram_list->setObjectName("m_diagram_list");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_diagram_list->sizePolicy().hasHeightForWidth());
        m_diagram_list->setSizePolicy(sizePolicy1);
        m_diagram_list->setLineWidth(3);
        m_diagram_list->setMidLineWidth(3);
        m_diagram_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_diagram_list->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 932, 58));
        m_diagram_list->setWidget(scrollAreaWidgetContents);

        verticalLayout_2->addWidget(m_diagram_list);

        m_check_all_pb = new QPushButton(centralwidget);
        m_check_all_pb->setObjectName("m_check_all_pb");

        verticalLayout_2->addWidget(m_check_all_pb);

        m_uncheck_all = new QPushButton(centralwidget);
        m_uncheck_all->setObjectName("m_uncheck_all");

        verticalLayout_2->addWidget(m_uncheck_all);

        widget = new QWidget(centralwidget);
        widget->setObjectName("widget");
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setSpacing(9);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        m_date_from_cb = new QComboBox(widget);
        m_date_from_cb->addItem(QString());
        m_date_from_cb->addItem(QString());
        m_date_from_cb->addItem(QString());
        m_date_from_cb->setObjectName("m_date_from_cb");

        horizontalLayout->addWidget(m_date_from_cb);

        m_date_cb = new QDateEdit(widget);
        m_date_cb->setObjectName("m_date_cb");
        m_date_cb->setEnabled(false);
        m_date_cb->setCalendarPopup(true);

        horizontalLayout->addWidget(m_date_cb);

        m_apply_date_pb = new QPushButton(widget);
        m_apply_date_pb->setObjectName("m_apply_date_pb");
        m_apply_date_pb->setEnabled(false);
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/ico/16x16/dialog-ok.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_apply_date_pb->setIcon(icon14);
        m_apply_date_pb->setFlat(true);

        horizontalLayout->addWidget(m_apply_date_pb);


        verticalLayout_2->addWidget(widget);


        m_vertical_layout->addLayout(verticalLayout_2);


        verticalLayout_3->addLayout(m_vertical_layout);

        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName("verticalLayout");
        m_draw_border_cb = new QCheckBox(groupBox);
        m_draw_border_cb->setObjectName("m_draw_border_cb");

        verticalLayout->addWidget(m_draw_border_cb);

        m_draw_titleblock_cb = new QCheckBox(groupBox);
        m_draw_titleblock_cb->setObjectName("m_draw_titleblock_cb");

        verticalLayout->addWidget(m_draw_titleblock_cb);

        m_keep_conductor_color_cb = new QCheckBox(groupBox);
        m_keep_conductor_color_cb->setObjectName("m_keep_conductor_color_cb");

        verticalLayout->addWidget(m_keep_conductor_color_cb);

        m_draw_terminal_cb = new QCheckBox(groupBox);
        m_draw_terminal_cb->setObjectName("m_draw_terminal_cb");

        verticalLayout->addWidget(m_draw_terminal_cb);

        m_draw_terminal_names_cb = new QCheckBox(groupBox);
        m_draw_terminal_names_cb->setObjectName("m_draw_terminal_names_cb");
        m_draw_terminal_names_cb->setChecked(false);

        verticalLayout->addWidget(m_draw_terminal_names_cb);

        m_component_info_cb = new QCheckBox(groupBox);
        m_component_info_cb->setObjectName("m_component_info_cb");
        m_component_info_cb->setChecked(false);

        verticalLayout->addWidget(m_component_info_cb);


        verticalLayout_3->addWidget(groupBox);

        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        m_fit_in_page_cb = new QCheckBox(groupBox_2);
        m_fit_in_page_cb->setObjectName("m_fit_in_page_cb");
        m_fit_in_page_cb->setChecked(true);

        gridLayout_2->addWidget(m_fit_in_page_cb, 3, 0, 1, 1);

        m_use_full_page_cb = new QCheckBox(groupBox_2);
        m_use_full_page_cb->setObjectName("m_use_full_page_cb");

        gridLayout_2->addWidget(m_use_full_page_cb, 0, 0, 1, 1);

        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");
        label_2->setMargin(0);

        gridLayout_2->addWidget(label_2, 3, 1, 1, 1);

        label = new QLabel(groupBox_2);
        label->setObjectName("label");
        label->setWordWrap(true);
        label->setMargin(0);
        label->setOpenExternalLinks(false);

        gridLayout_2->addWidget(label, 0, 1, 1, 1);

        line = new QFrame(groupBox_2);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout_2->addWidget(line, 2, 0, 1, 2);


        verticalLayout_3->addWidget(groupBox_2);

        m_button_box = new QDialogButtonBox(centralwidget);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setStandardButtons(QDialogButtonBox::Cancel);

        verticalLayout_3->addWidget(m_button_box);

        ProjectPrintWindow->setCentralWidget(centralwidget);
        toolBar = new QToolBar(ProjectPrintWindow);
        toolBar->setObjectName("toolBar");
        ProjectPrintWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar);

        toolBar->addAction(m_adjust_width_action);
        toolBar->addAction(m_adjust_page_action);
        toolBar->addAction(m_zoom_out_action);
        toolBar->addAction(m_zoom_in_action);
        toolBar->addSeparator();
        toolBar->addAction(m_landscape_action);
        toolBar->addAction(m_portrait_action);
        toolBar->addSeparator();
        toolBar->addAction(m_first_page_action);
        toolBar->addAction(m_previous_page_action);
        toolBar->addAction(m_next_page_action);
        toolBar->addAction(m_last_page_action);
        toolBar->addSeparator();
        toolBar->addAction(m_display_single_page_action);
        toolBar->addAction(m_display_two_page_action);
        toolBar->addAction(m_display_all_page_action);
        toolBar->addSeparator();
        toolBar->addAction(m_page_setup);

        retranslateUi(ProjectPrintWindow);
        QObject::connect(m_button_box, &QDialogButtonBox::rejected, ProjectPrintWindow, qOverload<>(&QMainWindow::close));

        QMetaObject::connectSlotsByName(ProjectPrintWindow);
    } // setupUi

    void retranslateUi(QMainWindow *ProjectPrintWindow)
    {
        ProjectPrintWindow->setWindowTitle(QCoreApplication::translate("ProjectPrintWindow", "MainWindow", nullptr));
        m_adjust_width_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Ajuster la largeur", nullptr));
        m_adjust_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Ajuster la page", nullptr));
        m_zoom_out_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Zoom arri\303\250re", nullptr));
        m_zoom_in_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Zoom avant", nullptr));
        m_landscape_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Paysage", nullptr));
        m_portrait_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Portrait", nullptr));
        m_first_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Premi\303\250re page", nullptr));
        m_previous_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Page pr\303\251c\303\251dente", nullptr));
        m_next_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Page suivante", nullptr));
        m_last_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Derni\303\250re page", nullptr));
        m_display_single_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Afficher une seule page", nullptr));
        m_display_two_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Afficher deux pages", nullptr));
        m_display_all_page_action->setText(QCoreApplication::translate("ProjectPrintWindow", "Afficher un aper\303\247u de toutes les pages", nullptr));
        m_page_setup->setText(QCoreApplication::translate("ProjectPrintWindow", "mise en page", nullptr));
        label_3->setText(QCoreApplication::translate("ProjectPrintWindow", "Folios \303\240 imprimer :", nullptr));
        m_check_all_pb->setText(QCoreApplication::translate("ProjectPrintWindow", "Tout cocher", nullptr));
        m_uncheck_all->setText(QCoreApplication::translate("ProjectPrintWindow", "Tout d\303\251cocher", nullptr));
        m_date_from_cb->setItemText(0, QCoreApplication::translate("ProjectPrintWindow", "Toutes les dates", nullptr));
        m_date_from_cb->setItemText(1, QCoreApplication::translate("ProjectPrintWindow", "\303\200 partir de la date du :", nullptr));
        m_date_from_cb->setItemText(2, QCoreApplication::translate("ProjectPrintWindow", "\303\200 la date du  :", nullptr));

        m_apply_date_pb->setText(QString());
        groupBox->setTitle(QCoreApplication::translate("ProjectPrintWindow", "Option de rendu", nullptr));
        m_draw_border_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Dessiner le cadre", nullptr));
        m_draw_titleblock_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Dessiner le cartouche", nullptr));
        m_keep_conductor_color_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Conserver les couleurs des conducteurs", nullptr));
        m_draw_terminal_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Dessiner les bornes", nullptr));
        m_draw_terminal_names_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Dessiner les noms des bornes", nullptr));
        m_component_info_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Inscrire les informations des composants", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("ProjectPrintWindow", "Option d'impression", nullptr));
        m_fit_in_page_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Adapter le folio \303\240 la page", nullptr));
        m_use_full_page_cb->setText(QCoreApplication::translate("ProjectPrintWindow", "Utiliser toute la feuille", nullptr));
        label_2->setText(QCoreApplication::translate("ProjectPrintWindow", "Si cette option est coch\303\251e, le folio sera agrandi ou r\303\251tr\303\251ci de fa\303\247on \303\240 remplir toute la surface imprimable d'une et une seule page.\"", nullptr));
        label->setText(QCoreApplication::translate("ProjectPrintWindow", "Si cette option est coch\303\251e, les marges de la feuille seront  ignor\303\251es et toute sa surface sera utilis\303\251e pour l'impression.  Cela peut ne pas \303\252tre support\303\251 par votre imprimante.", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("ProjectPrintWindow", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ProjectPrintWindow: public Ui_ProjectPrintWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROJECTPRINTWINDOW_H
