/********************************************************************************
** Form generated from reading UI file 'htmleditor.ui'
**
** Created: Mon 1. Apr 15:44:25 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HTMLEDITOR_H
#define UI_HTMLEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTabWidget>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include <QtWebKit/QWebView>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionFileNew;
    QAction *actionFileOpen;
    QAction *actionFileSave;
    QAction *actionFileSaveAs;
    QAction *actionEditUndo;
    QAction *actionEditRedo;
    QAction *actionEditCut;
    QAction *actionEditCopy;
    QAction *actionEditPaste;
    QAction *actionEditSelectAll;
    QAction *actionFormatBold;
    QAction *actionFormatItalic;
    QAction *actionFormatUnderline;
    QAction *actionFormatStrikethrough;
    QAction *actionFormatAlignLeft;
    QAction *actionFormatAlignCenter;
    QAction *actionFormatAlignRight;
    QAction *actionFormatAlignJustify;
    QAction *actionFormatIncreaseIndent;
    QAction *actionFormatDecreaseIndent;
    QAction *actionFormatBulletedList;
    QAction *actionFormatNumberedList;
    QAction *actionInsertImage;
    QAction *actionCreateLink;
    QAction *actionZoomOut;
    QAction *actionZoomIn;
    QAction *actionExit;
    QAction *actionStyleParagraph;
    QAction *actionStyleHeading1;
    QAction *actionStyleHeading2;
    QAction *actionStyleHeading3;
    QAction *actionStyleHeading4;
    QAction *actionStyleHeading5;
    QAction *actionStyleHeading6;
    QAction *actionStylePreformatted;
    QAction *actionStyleAddress;
    QAction *actionFormatFontName;
    QAction *actionFormatTextColor;
    QAction *actionFormatBackgroundColor;
    QAction *actionFormatFontSize;
    QAction *actionInsertHtml;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QWebView *webView;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_3;
    QPlainTextEdit *plainTextEdit;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_Edit;
    QMenu *menuF_ormat;
    QMenu *menuSt_yle;
    QMenu *menu_Align;
    QToolBar *standardToolBar;
    QToolBar *formatToolBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(659, 398);
        actionFileNew = new QAction(MainWindow);
        actionFileNew->setObjectName(QString::fromUtf8("actionFileNew"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/document-new.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFileNew->setIcon(icon);
        actionFileOpen = new QAction(MainWindow);
        actionFileOpen->setObjectName(QString::fromUtf8("actionFileOpen"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/images/document-open.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFileOpen->setIcon(icon1);
        actionFileSave = new QAction(MainWindow);
        actionFileSave->setObjectName(QString::fromUtf8("actionFileSave"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/images/document-save.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFileSave->setIcon(icon2);
        actionFileSaveAs = new QAction(MainWindow);
        actionFileSaveAs->setObjectName(QString::fromUtf8("actionFileSaveAs"));
        actionEditUndo = new QAction(MainWindow);
        actionEditUndo->setObjectName(QString::fromUtf8("actionEditUndo"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/images/edit-undo.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditUndo->setIcon(icon3);
        actionEditRedo = new QAction(MainWindow);
        actionEditRedo->setObjectName(QString::fromUtf8("actionEditRedo"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/images/edit-redo.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditRedo->setIcon(icon4);
        actionEditCut = new QAction(MainWindow);
        actionEditCut->setObjectName(QString::fromUtf8("actionEditCut"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/images/edit-cut.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditCut->setIcon(icon5);
        actionEditCopy = new QAction(MainWindow);
        actionEditCopy->setObjectName(QString::fromUtf8("actionEditCopy"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/images/edit-copy.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditCopy->setIcon(icon6);
        actionEditPaste = new QAction(MainWindow);
        actionEditPaste->setObjectName(QString::fromUtf8("actionEditPaste"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/images/edit-paste.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditPaste->setIcon(icon7);
        actionEditSelectAll = new QAction(MainWindow);
        actionEditSelectAll->setObjectName(QString::fromUtf8("actionEditSelectAll"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/images/edit-select-all.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditSelectAll->setIcon(icon8);
        actionFormatBold = new QAction(MainWindow);
        actionFormatBold->setObjectName(QString::fromUtf8("actionFormatBold"));
        actionFormatBold->setCheckable(true);
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/images/format-text-bold.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatBold->setIcon(icon9);
        actionFormatItalic = new QAction(MainWindow);
        actionFormatItalic->setObjectName(QString::fromUtf8("actionFormatItalic"));
        actionFormatItalic->setCheckable(true);
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/images/format-text-italic.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatItalic->setIcon(icon10);
        actionFormatUnderline = new QAction(MainWindow);
        actionFormatUnderline->setObjectName(QString::fromUtf8("actionFormatUnderline"));
        actionFormatUnderline->setCheckable(true);
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/images/format-text-underline.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatUnderline->setIcon(icon11);
        actionFormatStrikethrough = new QAction(MainWindow);
        actionFormatStrikethrough->setObjectName(QString::fromUtf8("actionFormatStrikethrough"));
        actionFormatStrikethrough->setCheckable(true);
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/images/format-text-strikethrough.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatStrikethrough->setIcon(icon12);
        actionFormatAlignLeft = new QAction(MainWindow);
        actionFormatAlignLeft->setObjectName(QString::fromUtf8("actionFormatAlignLeft"));
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/images/format-justify-left.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatAlignLeft->setIcon(icon13);
        actionFormatAlignCenter = new QAction(MainWindow);
        actionFormatAlignCenter->setObjectName(QString::fromUtf8("actionFormatAlignCenter"));
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/images/format-justify-center.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatAlignCenter->setIcon(icon14);
        actionFormatAlignRight = new QAction(MainWindow);
        actionFormatAlignRight->setObjectName(QString::fromUtf8("actionFormatAlignRight"));
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/images/format-justify-right.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatAlignRight->setIcon(icon15);
        actionFormatAlignJustify = new QAction(MainWindow);
        actionFormatAlignJustify->setObjectName(QString::fromUtf8("actionFormatAlignJustify"));
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/images/format-justify-fill.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatAlignJustify->setIcon(icon16);
        actionFormatIncreaseIndent = new QAction(MainWindow);
        actionFormatIncreaseIndent->setObjectName(QString::fromUtf8("actionFormatIncreaseIndent"));
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/images/format-indent-more.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatIncreaseIndent->setIcon(icon17);
        actionFormatDecreaseIndent = new QAction(MainWindow);
        actionFormatDecreaseIndent->setObjectName(QString::fromUtf8("actionFormatDecreaseIndent"));
        QIcon icon18;
        icon18.addFile(QString::fromUtf8(":/images/format-indent-less.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatDecreaseIndent->setIcon(icon18);
        actionFormatBulletedList = new QAction(MainWindow);
        actionFormatBulletedList->setObjectName(QString::fromUtf8("actionFormatBulletedList"));
        actionFormatBulletedList->setCheckable(true);
        QIcon icon19;
        icon19.addFile(QString::fromUtf8(":/images/bulleted-list.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatBulletedList->setIcon(icon19);
        actionFormatNumberedList = new QAction(MainWindow);
        actionFormatNumberedList->setObjectName(QString::fromUtf8("actionFormatNumberedList"));
        actionFormatNumberedList->setCheckable(true);
        QIcon icon20;
        icon20.addFile(QString::fromUtf8(":/images/numbered-list.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatNumberedList->setIcon(icon20);
        actionInsertImage = new QAction(MainWindow);
        actionInsertImage->setObjectName(QString::fromUtf8("actionInsertImage"));
        QIcon icon21;
        icon21.addFile(QString::fromUtf8(":/images/image-x-generic.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionInsertImage->setIcon(icon21);
        actionInsertImage->setVisible(false);
        actionCreateLink = new QAction(MainWindow);
        actionCreateLink->setObjectName(QString::fromUtf8("actionCreateLink"));
        QIcon icon22;
        icon22.addFile(QString::fromUtf8(":/images/text-html.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionCreateLink->setIcon(icon22);
        actionZoomOut = new QAction(MainWindow);
        actionZoomOut->setObjectName(QString::fromUtf8("actionZoomOut"));
        QIcon icon23;
        icon23.addFile(QString::fromUtf8(":/images/list-remove.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionZoomOut->setIcon(icon23);
        actionZoomIn = new QAction(MainWindow);
        actionZoomIn->setObjectName(QString::fromUtf8("actionZoomIn"));
        QIcon icon24;
        icon24.addFile(QString::fromUtf8(":/images/list-add.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionZoomIn->setIcon(icon24);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionStyleParagraph = new QAction(MainWindow);
        actionStyleParagraph->setObjectName(QString::fromUtf8("actionStyleParagraph"));
        actionStyleHeading1 = new QAction(MainWindow);
        actionStyleHeading1->setObjectName(QString::fromUtf8("actionStyleHeading1"));
        actionStyleHeading2 = new QAction(MainWindow);
        actionStyleHeading2->setObjectName(QString::fromUtf8("actionStyleHeading2"));
        actionStyleHeading3 = new QAction(MainWindow);
        actionStyleHeading3->setObjectName(QString::fromUtf8("actionStyleHeading3"));
        actionStyleHeading4 = new QAction(MainWindow);
        actionStyleHeading4->setObjectName(QString::fromUtf8("actionStyleHeading4"));
        actionStyleHeading5 = new QAction(MainWindow);
        actionStyleHeading5->setObjectName(QString::fromUtf8("actionStyleHeading5"));
        actionStyleHeading6 = new QAction(MainWindow);
        actionStyleHeading6->setObjectName(QString::fromUtf8("actionStyleHeading6"));
        actionStylePreformatted = new QAction(MainWindow);
        actionStylePreformatted->setObjectName(QString::fromUtf8("actionStylePreformatted"));
        actionStyleAddress = new QAction(MainWindow);
        actionStyleAddress->setObjectName(QString::fromUtf8("actionStyleAddress"));
        actionFormatFontName = new QAction(MainWindow);
        actionFormatFontName->setObjectName(QString::fromUtf8("actionFormatFontName"));
        actionFormatTextColor = new QAction(MainWindow);
        actionFormatTextColor->setObjectName(QString::fromUtf8("actionFormatTextColor"));
        QIcon icon25;
        icon25.addFile(QString::fromUtf8(":/images/text_color.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatTextColor->setIcon(icon25);
        actionFormatBackgroundColor = new QAction(MainWindow);
        actionFormatBackgroundColor->setObjectName(QString::fromUtf8("actionFormatBackgroundColor"));
        actionFormatFontSize = new QAction(MainWindow);
        actionFormatFontSize->setObjectName(QString::fromUtf8("actionFormatFontSize"));
        QIcon icon26;
        icon26.addFile(QString::fromUtf8(":/images/text.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFormatFontSize->setIcon(icon26);
        actionInsertHtml = new QAction(MainWindow);
        actionInsertHtml->setObjectName(QString::fromUtf8("actionInsertHtml"));
        QIcon icon27;
        icon27.addFile(QString::fromUtf8(":/images/insert-html.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionInsertHtml->setIcon(icon27);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setTabPosition(QTabWidget::South);
        tabWidget->setTabShape(QTabWidget::Rounded);
        tabWidget->setDocumentMode(true);
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        verticalLayout_2 = new QVBoxLayout(tab);
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        webView = new QWebView(tab);
        webView->setObjectName(QString::fromUtf8("webView"));
        webView->setUrl(QUrl(QString::fromUtf8("about:blank")));

        verticalLayout_2->addWidget(webView);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        verticalLayout_3 = new QVBoxLayout(tab_2);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        plainTextEdit = new QPlainTextEdit(tab_2);
        plainTextEdit->setObjectName(QString::fromUtf8("plainTextEdit"));
        plainTextEdit->setFrameShape(QFrame::NoFrame);
        plainTextEdit->setReadOnly(true);

        verticalLayout_3->addWidget(plainTextEdit);

        tabWidget->addTab(tab_2, QString());

        verticalLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 659, 31));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menu_Edit = new QMenu(menubar);
        menu_Edit->setObjectName(QString::fromUtf8("menu_Edit"));
        menuF_ormat = new QMenu(menubar);
        menuF_ormat->setObjectName(QString::fromUtf8("menuF_ormat"));
        menuSt_yle = new QMenu(menuF_ormat);
        menuSt_yle->setObjectName(QString::fromUtf8("menuSt_yle"));
        menu_Align = new QMenu(menuF_ormat);
        menu_Align->setObjectName(QString::fromUtf8("menu_Align"));
        MainWindow->setMenuBar(menubar);
        standardToolBar = new QToolBar(MainWindow);
        standardToolBar->setObjectName(QString::fromUtf8("standardToolBar"));
        standardToolBar->setEnabled(false);
        standardToolBar->setIconSize(QSize(24, 24));
        MainWindow->addToolBar(Qt::TopToolBarArea, standardToolBar);
        formatToolBar = new QToolBar(MainWindow);
        formatToolBar->setObjectName(QString::fromUtf8("formatToolBar"));
        formatToolBar->setIconSize(QSize(24, 24));
        MainWindow->addToolBar(Qt::TopToolBarArea, formatToolBar);
        MainWindow->insertToolBarBreak(formatToolBar);

        menubar->addAction(menu_File->menuAction());
        menubar->addAction(menu_Edit->menuAction());
        menubar->addAction(menuF_ormat->menuAction());
        menu_File->addAction(actionFileNew);
        menu_File->addAction(actionFileOpen);
        menu_File->addAction(actionFileSave);
        menu_File->addAction(actionFileSaveAs);
        menu_File->addSeparator();
        menu_File->addAction(actionExit);
        menu_Edit->addAction(actionEditUndo);
        menu_Edit->addAction(actionEditRedo);
        menu_Edit->addSeparator();
        menu_Edit->addAction(actionEditCut);
        menu_Edit->addAction(actionEditCopy);
        menu_Edit->addAction(actionEditPaste);
        menu_Edit->addSeparator();
        menu_Edit->addAction(actionEditSelectAll);
        menu_Edit->addSeparator();
        menu_Edit->addAction(actionInsertImage);
        menu_Edit->addAction(actionCreateLink);
        menu_Edit->addAction(actionInsertHtml);
        menuF_ormat->addAction(menuSt_yle->menuAction());
        menuF_ormat->addAction(menu_Align->menuAction());
        menuF_ormat->addSeparator();
        menuF_ormat->addAction(actionFormatBold);
        menuF_ormat->addAction(actionFormatItalic);
        menuF_ormat->addAction(actionFormatUnderline);
        menuF_ormat->addAction(actionFormatStrikethrough);
        menuF_ormat->addSeparator();
        menuF_ormat->addAction(actionFormatIncreaseIndent);
        menuF_ormat->addAction(actionFormatDecreaseIndent);
        menuF_ormat->addSeparator();
        menuF_ormat->addAction(actionFormatNumberedList);
        menuF_ormat->addAction(actionFormatBulletedList);
        menuF_ormat->addSeparator();
        menuF_ormat->addAction(actionFormatFontName);
        menuF_ormat->addAction(actionFormatFontSize);
        menuF_ormat->addSeparator();
        menuF_ormat->addAction(actionFormatTextColor);
        menuF_ormat->addAction(actionFormatBackgroundColor);
        menuSt_yle->addAction(actionStyleParagraph);
        menuSt_yle->addAction(actionStyleHeading1);
        menuSt_yle->addAction(actionStyleHeading2);
        menuSt_yle->addAction(actionStyleHeading3);
        menuSt_yle->addAction(actionStyleHeading4);
        menuSt_yle->addAction(actionStyleHeading5);
        menuSt_yle->addAction(actionStyleHeading6);
        menuSt_yle->addAction(actionStylePreformatted);
        menuSt_yle->addAction(actionStyleAddress);
        menu_Align->addAction(actionFormatAlignLeft);
        menu_Align->addAction(actionFormatAlignCenter);
        menu_Align->addAction(actionFormatAlignRight);
        menu_Align->addAction(actionFormatAlignJustify);
        standardToolBar->addAction(actionFileNew);
        standardToolBar->addAction(actionFileOpen);
        standardToolBar->addAction(actionFileSave);
        standardToolBar->addSeparator();
        standardToolBar->addAction(actionEditUndo);
        standardToolBar->addAction(actionEditRedo);
        standardToolBar->addSeparator();
        standardToolBar->addAction(actionEditCut);
        standardToolBar->addAction(actionEditCopy);
        standardToolBar->addAction(actionEditPaste);
        standardToolBar->addSeparator();
        standardToolBar->addAction(actionZoomOut);
        standardToolBar->addAction(actionZoomIn);
        formatToolBar->addAction(actionFormatBold);
        formatToolBar->addAction(actionFormatItalic);
        formatToolBar->addAction(actionFormatUnderline);
        formatToolBar->addAction(actionFormatStrikethrough);
        formatToolBar->addAction(actionFormatFontSize);
        formatToolBar->addAction(actionFormatTextColor);
        formatToolBar->addSeparator();
        formatToolBar->addAction(actionFormatAlignLeft);
        formatToolBar->addAction(actionFormatAlignCenter);
        formatToolBar->addAction(actionFormatAlignRight);
        formatToolBar->addAction(actionFormatAlignJustify);
        formatToolBar->addSeparator();
        formatToolBar->addAction(actionFormatDecreaseIndent);
        formatToolBar->addAction(actionFormatIncreaseIndent);
        formatToolBar->addSeparator();
        formatToolBar->addAction(actionFormatNumberedList);
        formatToolBar->addAction(actionFormatBulletedList);
        formatToolBar->addSeparator();
        formatToolBar->addAction(actionInsertImage);
        formatToolBar->addAction(actionCreateLink);
        formatToolBar->addAction(actionInsertHtml);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "HTML Editor", 0, QApplication::UnicodeUTF8));
        actionFileNew->setText(QApplication::translate("MainWindow", "&New", 0, QApplication::UnicodeUTF8));
        actionFileNew->setShortcut(QApplication::translate("MainWindow", "Ctrl+N", 0, QApplication::UnicodeUTF8));
        actionFileOpen->setText(QApplication::translate("MainWindow", "&Open...", 0, QApplication::UnicodeUTF8));
        actionFileOpen->setShortcut(QApplication::translate("MainWindow", "Ctrl+O", 0, QApplication::UnicodeUTF8));
        actionFileSave->setText(QApplication::translate("MainWindow", "&Save", 0, QApplication::UnicodeUTF8));
        actionFileSave->setShortcut(QApplication::translate("MainWindow", "Ctrl+S", 0, QApplication::UnicodeUTF8));
        actionFileSaveAs->setText(QApplication::translate("MainWindow", "Save &As...", 0, QApplication::UnicodeUTF8));
        actionEditUndo->setText(QApplication::translate("MainWindow", "&Undo", 0, QApplication::UnicodeUTF8));
        actionEditUndo->setShortcut(QApplication::translate("MainWindow", "Ctrl+Z", 0, QApplication::UnicodeUTF8));
        actionEditRedo->setText(QApplication::translate("MainWindow", "&Redo", 0, QApplication::UnicodeUTF8));
        actionEditRedo->setShortcut(QApplication::translate("MainWindow", "Ctrl+Y", 0, QApplication::UnicodeUTF8));
        actionEditCut->setText(QApplication::translate("MainWindow", "Cu&t", 0, QApplication::UnicodeUTF8));
        actionEditCut->setShortcut(QApplication::translate("MainWindow", "Ctrl+X", 0, QApplication::UnicodeUTF8));
        actionEditCopy->setText(QApplication::translate("MainWindow", "&Copy", 0, QApplication::UnicodeUTF8));
        actionEditCopy->setShortcut(QApplication::translate("MainWindow", "Ctrl+C", 0, QApplication::UnicodeUTF8));
        actionEditPaste->setText(QApplication::translate("MainWindow", "&Paste", 0, QApplication::UnicodeUTF8));
        actionEditPaste->setShortcut(QApplication::translate("MainWindow", "Ctrl+V", 0, QApplication::UnicodeUTF8));
        actionEditSelectAll->setText(QApplication::translate("MainWindow", "Select A&ll", 0, QApplication::UnicodeUTF8));
        actionEditSelectAll->setShortcut(QApplication::translate("MainWindow", "Ctrl+A", 0, QApplication::UnicodeUTF8));
        actionFormatBold->setText(QApplication::translate("MainWindow", "&Bold", 0, QApplication::UnicodeUTF8));
        actionFormatBold->setShortcut(QApplication::translate("MainWindow", "Ctrl+B", 0, QApplication::UnicodeUTF8));
        actionFormatItalic->setText(QApplication::translate("MainWindow", "&Italic", 0, QApplication::UnicodeUTF8));
        actionFormatItalic->setShortcut(QApplication::translate("MainWindow", "Ctrl+I", 0, QApplication::UnicodeUTF8));
        actionFormatUnderline->setText(QApplication::translate("MainWindow", "&Underline", 0, QApplication::UnicodeUTF8));
        actionFormatUnderline->setShortcut(QApplication::translate("MainWindow", "Ctrl+U", 0, QApplication::UnicodeUTF8));
        actionFormatStrikethrough->setText(QApplication::translate("MainWindow", "&Strikethrough", 0, QApplication::UnicodeUTF8));
        actionFormatAlignLeft->setText(QApplication::translate("MainWindow", "Align &Left", 0, QApplication::UnicodeUTF8));
        actionFormatAlignCenter->setText(QApplication::translate("MainWindow", "Align &Center", 0, QApplication::UnicodeUTF8));
        actionFormatAlignRight->setText(QApplication::translate("MainWindow", "Align &Right", 0, QApplication::UnicodeUTF8));
        actionFormatAlignJustify->setText(QApplication::translate("MainWindow", "Align &Justify", 0, QApplication::UnicodeUTF8));
        actionFormatIncreaseIndent->setText(QApplication::translate("MainWindow", "I&ncrease Indent", 0, QApplication::UnicodeUTF8));
        actionFormatDecreaseIndent->setText(QApplication::translate("MainWindow", "&Decrease Indent", 0, QApplication::UnicodeUTF8));
        actionFormatBulletedList->setText(QApplication::translate("MainWindow", "Bulle&ted List", 0, QApplication::UnicodeUTF8));
        actionFormatNumberedList->setText(QApplication::translate("MainWindow", "&Numbered List", 0, QApplication::UnicodeUTF8));
        actionInsertImage->setText(QApplication::translate("MainWindow", "Insert &Image...", 0, QApplication::UnicodeUTF8));
        actionCreateLink->setText(QApplication::translate("MainWindow", "Create Link...", 0, QApplication::UnicodeUTF8));
        actionZoomOut->setText(QApplication::translate("MainWindow", "Zoom Out", 0, QApplication::UnicodeUTF8));
        actionZoomIn->setText(QApplication::translate("MainWindow", "Zoom In", 0, QApplication::UnicodeUTF8));
        actionExit->setText(QApplication::translate("MainWindow", "E&xit", 0, QApplication::UnicodeUTF8));
        actionExit->setShortcut(QApplication::translate("MainWindow", "Ctrl+Q", 0, QApplication::UnicodeUTF8));
        actionStyleParagraph->setText(QApplication::translate("MainWindow", "&Paragraph", 0, QApplication::UnicodeUTF8));
        actionStyleHeading1->setText(QApplication::translate("MainWindow", "Heading &1", 0, QApplication::UnicodeUTF8));
        actionStyleHeading2->setText(QApplication::translate("MainWindow", "Heading &2", 0, QApplication::UnicodeUTF8));
        actionStyleHeading3->setText(QApplication::translate("MainWindow", "Heading &3", 0, QApplication::UnicodeUTF8));
        actionStyleHeading4->setText(QApplication::translate("MainWindow", "Heading &4", 0, QApplication::UnicodeUTF8));
        actionStyleHeading5->setText(QApplication::translate("MainWindow", "Heading &5", 0, QApplication::UnicodeUTF8));
        actionStyleHeading6->setText(QApplication::translate("MainWindow", "Heading &6", 0, QApplication::UnicodeUTF8));
        actionStylePreformatted->setText(QApplication::translate("MainWindow", "Pre&formatted", 0, QApplication::UnicodeUTF8));
        actionStyleAddress->setText(QApplication::translate("MainWindow", "&Address", 0, QApplication::UnicodeUTF8));
        actionFormatFontName->setText(QApplication::translate("MainWindow", "&Font Name...", 0, QApplication::UnicodeUTF8));
        actionFormatTextColor->setText(QApplication::translate("MainWindow", "Text &Color...", 0, QApplication::UnicodeUTF8));
        actionFormatBackgroundColor->setText(QApplication::translate("MainWindow", "Bac&kground Color...", 0, QApplication::UnicodeUTF8));
        actionFormatFontSize->setText(QApplication::translate("MainWindow", "Font Si&ze...", 0, QApplication::UnicodeUTF8));
        actionInsertHtml->setText(QApplication::translate("MainWindow", "Insert HTML...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionInsertHtml->setToolTip(QApplication::translate("MainWindow", "Insert HTML", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("MainWindow", "Tab 1", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("MainWindow", "Tab 2", 0, QApplication::UnicodeUTF8));
        menu_File->setTitle(QApplication::translate("MainWindow", "&File", 0, QApplication::UnicodeUTF8));
        menu_Edit->setTitle(QApplication::translate("MainWindow", "&Edit", 0, QApplication::UnicodeUTF8));
        menuF_ormat->setTitle(QApplication::translate("MainWindow", "F&ormat", 0, QApplication::UnicodeUTF8));
        menuSt_yle->setTitle(QApplication::translate("MainWindow", "St&yle", 0, QApplication::UnicodeUTF8));
        menu_Align->setTitle(QApplication::translate("MainWindow", "&Align", 0, QApplication::UnicodeUTF8));
        standardToolBar->setWindowTitle(QApplication::translate("MainWindow", "Standard", 0, QApplication::UnicodeUTF8));
        formatToolBar->setWindowTitle(QApplication::translate("MainWindow", "Formatting", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HTMLEDITOR_H
