/*
	Copyright 2006-2013 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetmainwindow.h"
#include "qeticons.h"
#include "qetapp.h"

/**
	Constructor
*/
QETMainWindow::QETMainWindow(QWidget *widget, Qt::WindowFlags flags) :
	QMainWindow(widget, flags),
	display_toolbars_(0),
	first_activation_(true)
{
	initCommonActions();
	initCommonMenus();
	
	setAcceptDrops(true);
}

/**
	Destructor
*/
QETMainWindow::~QETMainWindow() {
}

/**
	Initialize common actions.
*/
void QETMainWindow::initCommonActions() {
	QETApp *qet_app = QETApp::instance();
	
	configure_action_ = new QAction(QET::Icons::Configure, tr("&Configurer QElectroTech"), this);
	configure_action_ -> setStatusTip(tr("Permet de r\351gler diff\351rents param\350tres de QElectroTech", "status bar tip"));
	connect(configure_action_,  SIGNAL(triggered()), qet_app, SLOT(configureQET()));
	
	fullscreen_action_ = new QAction(this);
	updateFullScreenAction();
	connect(fullscreen_action_, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	
	whatsthis_action_ = QWhatsThis::createAction(this);
	
	about_qet_ = new QAction(QET::Icons::QETLogo, tr("\300 &propos de QElectroTech"), this);
	about_qet_ -> setStatusTip(tr("Affiche des informations sur QElectroTech", "status bar tip"));
	connect(about_qet_,  SIGNAL(triggered()), qet_app, SLOT(aboutQET()));
	
	about_qt_ = new QAction(QET::Icons::QtLogo,  tr("\300 propos de &Qt"), this);
	about_qt_ -> setStatusTip(tr("Affiche des informations sur la biblioth\350que Qt", "status bar tip"));
	connect(about_qt_, SIGNAL(triggered()), qet_app, SLOT(aboutQt()));
}

/**
	Initialize common menus.
*/
void QETMainWindow::initCommonMenus() {
	settings_menu_ = new QMenu(tr("&Configuration", "window menu"));
	settings_menu_ -> addAction(fullscreen_action_);
	settings_menu_ -> addAction(configure_action_);
	connect(settings_menu_, SIGNAL(aboutToShow()), this, SLOT(checkToolbarsmenu()));
	
	
	help_menu_ = new QMenu(tr("&Aide", "window menu"));
	help_menu_ -> addAction(whatsthis_action_);
	help_menu_ -> addSeparator();
	help_menu_ -> addAction(about_qet_);
	help_menu_ -> addAction(about_qt_);
	
	insertMenu(0, settings_menu_);
	insertMenu(0, help_menu_);
}

/**
	Add \a menu before \a before. Unless \a customize is false, this method also
	enables some common settings on the inserted menu.
*/
void QETMainWindow::insertMenu(QMenu *before, QMenu *menu, bool customize) {
	if (!menu) return;
	
	QAction *before_action = actionForMenu(before);
	QAction *menu_action = menuBar() -> insertMenu(before_action, menu);
	menu_actions_.insert(menu, menu_action);
	
	if (customize) {
		menu -> setTearOffEnabled(true);
	}
}

/**
	@return the action returned when inserting \a menu
*/
QAction *QETMainWindow::actionForMenu(QMenu *menu) {
	return(menu_actions_.value(menu, 0));
}

/**
	Toggle the window from/to full screen.
*/
void QETMainWindow::toggleFullScreen() {
	setWindowState(windowState() ^ Qt::WindowFullScreen);
}

/**
	Update the look of the full screen action according to the current state of
	the window.
*/
void QETMainWindow::updateFullScreenAction() {
	if (windowState() & Qt::WindowFullScreen) {
		fullscreen_action_ -> setText(tr("Sortir du &mode plein \351cran"));
		fullscreen_action_ -> setIcon(QET::Icons::FullScreenExit);
		fullscreen_action_ -> setStatusTip(tr("Affiche QElectroTech en mode fen\352tr\351", "status bar tip"));
	} else {
		fullscreen_action_ -> setText(tr("Passer en &mode plein \351cran"));
		fullscreen_action_ -> setIcon(QET::Icons::FullScreenEnter);
		fullscreen_action_ -> setStatusTip(tr("Affiche QElectroTech en mode plein \351cran", "status bar tip"));
	}
	fullscreen_action_ -> setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
}

/**
	Check whether a sub menu dedicated to docks and toolbars can be inserted on
	top of the settings menu.
*/
void QETMainWindow::checkToolbarsmenu() {
	if (display_toolbars_) return;
	display_toolbars_ = createPopupMenu();
	if (display_toolbars_) {
		display_toolbars_ -> setTearOffEnabled(true);
		display_toolbars_ -> setTitle(tr("Afficher", "menu entry"));
		display_toolbars_ -> setIcon(QET::Icons::ConfigureToolbars);
		settings_menu_ -> insertMenu(fullscreen_action_, display_toolbars_);
	}
}

/**
	Handle the \a e event.
*/
bool QETMainWindow::event(QEvent *e) {
	if (e -> type() == QEvent::WindowStateChange) {
		updateFullScreenAction();
	} else if (first_activation_ && e -> type() == QEvent::WindowActivate) {
		firstActivation(e);
		first_activation_ = false;
	}
	return(QMainWindow::event(e));
}

/**
	Base implementation of firstActivation (does nothing).
*/
void QETMainWindow::firstActivation(QEvent *) {
}


/**
	Accept or refuse drag'n drop events depending on the dropped mime type;
	especially, accepts only URLs to local files that we could open.
	@param e le QDragEnterEvent correspondant au drag'n drop tente
*/
void QETMainWindow::dragEnterEvent(QDragEnterEvent *e) {
	if (e -> mimeData() -> hasUrls()) {
		if (QETApp::handledFiles(e -> mimeData() -> urls()).count()) {
			e -> acceptProposedAction();
		}
	}
}

/**
	Handle drops accepted on main windows; more specifically, open dropped files
	as long as they are handled by QElectrotech.
	@param e the QDropEvent describing the current drag'n drop
*/
void QETMainWindow::dropEvent(QDropEvent *e) {
	if (e -> mimeData() -> hasUrls()) {
		QStringList filepaths = QETApp::handledFiles(e -> mimeData() -> urls());
		if (filepaths.count()) {
			QETApp::instance() -> openFiles(QETArguments(filepaths));
		}
	}
}
