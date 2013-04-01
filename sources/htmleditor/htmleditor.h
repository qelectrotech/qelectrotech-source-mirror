/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Graphics Dojo project on Qt Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#ifndef HTML_EDITOR_H
#define HTML_EDITOR_H

#include <QMainWindow>
#include "highlighter.h"

#if QT_VERSION < 0x040500
#error You must use Qt >= 4.5.0!
#endif

class Ui_MainWindow;
class Ui_Dialog;

class QLabel;
class QSlider;
class QUrl;

class HtmlEditor : public QMainWindow
{
    Q_OBJECT

public:
    HtmlEditor(QWidget *parent = 0);
    ~HtmlEditor();

	void loadHtml(const QString &f);
	void setSimpleDisplay(bool state);
	
private:
    void setupActions();
    void setupToolBar();
    void setupMenu();
    bool load(const QString &f);
    bool maybeSave();
    void setCurrentFileName(const QString &fileName);
    void execCommand(const QString&);
    void execCommand(const QString &cmd, const QString &arg);
    bool queryCommandState(const QString&);

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();
    void editSelectAll();
    void styleParagraph();
    void styleHeading1();
    void styleHeading2();
    void styleHeading3();
    void styleHeading4();
    void styleHeading5();
    void styleHeading6();
    void stylePreformatted();
    void styleAddress();
    void formatStrikeThrough();
    void formatAlignLeft();
    void formatAlignCenter();
    void formatAlignRight();
    void formatAlignJustify();
    void formatIncreaseIndent();
    void formatDecreaseIndent();
    void formatNumberedList();
    void formatBulletedList();
    void formatFontName();
    void formatFontSize();
    void formatTextColor();
    void formatBackgroundColor();
    void insertImage();
    void createLink();
    void insertHtml();
    void zoomOut();
    void zoomIn();
    void adjustActions();
    void adjustSource();
    void changeTab(int);
    void openLink(const QUrl&);
    void changeZoom(int);
	
signals:
	void applyEditText(const QString &);
			
private:
    Ui_MainWindow *ui;
    QString fileName;
    bool sourceDirty;
    QLabel *zoomLabel;
    QSlider *zoomSlider;
    Highlighter *highlighter;
    Ui_Dialog *ui_dialog;
    QDialog *insertHtmlDialog;
	
	private slots:
		void on_buttonBox_accepted();
		void on_buttonBox_rejected();
};

#endif // HTML_EDITOR_H
