/*
	Copyright 2006-2026 The QElectroTech Team
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
#include <QAction>
#include <QApplication>
#include <QWhatsThis>
#include <QMenu>
#include <QMenuBar>
#include <QShortcut>
#include <QDragEnterEvent>
#include <QDesktopServices>

#include "qetmainwindow.h"
#include "qeticons.h"
#include "shortcutmanager.h"
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "projectview.h"

/**
	Constructor
*/
QETMainWindow::QETMainWindow(QWidget *widget, Qt::WindowFlags flags) :
	QMainWindow(widget, flags),
	display_toolbars_(nullptr),
	first_activation_(true)
{
	initCommonActions();
	initCommonMenus();

	setAcceptDrops(true);
		//A shortcut rather than a key handler: a key press goes to the
		//focused child widget, so a keyPressEvent() here would never see F10
		//while the canvas or a panel holds focus.
	QShortcut *menu_bar_shortcut = new QShortcut(QKeySequence(Qt::Key_F10), this);
	menu_bar_shortcut -> setContext(Qt::WindowShortcut);
	connect(menu_bar_shortcut, &QShortcut::activated,
		this, &QETMainWindow::activateMenuBar);

}

/**
	Destructor
*/
QETMainWindow::~QETMainWindow()
{
}

/**
	Initialize common actions.
*/
void QETMainWindow::initCommonActions()
{
	QETApp *qet_app = QETApp::instance();

	configure_action_ = new QAction(QET::Icons::Configure, tr("&Configurer QElectroTech"), this);
	configure_action_ -> setStatusTip(tr("Permet de régler différents paramètres de QElectroTech", "status bar tip"));
	connect(configure_action_, &QAction::triggered, [qet_app]()
	{
		qet_app->configureQET();
#if TODO_LIST
#	pragma message("@TODO we use reloadOldElementPanel only to keep up to ")
#	pragma message("datethe string of the folio in the old element panel.")
#	pragma message("then,if user change the option")
#	pragma message(" 'Use labels of folio instead of their ID' the string")
#	pragma message(" of folio in the old element panel is up to date")
#endif
			//TODO we use reloadOldElementPanel only to keep up to date the string of the folio in the old element panel.
			//then, if user change the option "Use labels of folio instead of their ID" the string of folio in the old element panel is up to date
		for (QETDiagramEditor *qde : qet_app->diagramEditors())
		{
			qde->reloadOldElementPanel();
			for (ProjectView *pv : qde->openedProjects())
			{
				pv->updateAllTabsTitle();
			}
		}
	});

	fullscreen_action_ = new QAction(this);
	updateFullScreenAction();
	connect(fullscreen_action_, &QAction::triggered, this, &QETMainWindow::toggleFullScreen);

	whatsthis_action_ = QWhatsThis::createAction(this);

	about_qet_ = new QAction(QET::Icons::QETLogo, tr("À &propos de QElectroTech"), this);
	about_qet_ -> setStatusTip(tr("Affiche des informations sur QElectroTech", "status bar tip"));
	connect(about_qet_, &QAction::triggered, qet_app, &QETApp::aboutQET);

	manual_online_ = new QAction(QET::Icons::QETManual, tr("Manuel en ligne"), this);
	manual_online_ -> setStatusTip(tr("Lance le navigateur par défaut vers le manuel en ligne de QElectroTech", "status bar tip"));

	connect(manual_online_, &QAction::triggered, [](bool) {
	QString link = "https://download.qelectrotech.org/qet/manual_0.7/build/index.html";
	QDesktopServices::openUrl(QUrl(link));
	});

	ShortcutManager::instance().registerAction(manual_online_, "mainwindow.manual_online", tr("Général"), Qt::Key_F1);

	youtube_ = new QAction(QET::Icons::QETVideo, tr("Chaine Youtube"), this);
	youtube_ -> setStatusTip(tr("Lance le navigateur par défaut vers la chaine Youtube de QElectroTech", "status bar tip"));

	connect(youtube_, &QAction::triggered, [](bool) {
	QString link = "https://www.youtube.com/user/scorpio8101/videos";
	QDesktopServices::openUrl(QUrl(link));
	});

	upgrade_ = new QAction(QET::Icons::QETDownload, tr("Télécharger une nouvelle version (dev)"), this);
	upgrade_ -> setStatusTip(tr("Lance le navigateur par défaut vers le dépot Nightly en ligne de QElectroTech", "status bar tip"));

	upgrade_M = new QAction(QET::Icons::QETDownload, tr("Télécharger une nouvelle version (dev)"), this);
	upgrade_M -> setStatusTip(tr("Lance le navigateur par défaut vers le dépot Nightly en ligne de QElectroTech", "status bar tip"));

	connect(upgrade_, &QAction::triggered, [](bool) {
	QString link = "https://qelectrotech.org/download_windows.php";
	QDesktopServices::openUrl(QUrl(link));
	});

	connect(upgrade_M, &QAction::triggered, [](bool) {
	QString link = "https://qelectrotech.org/download_mac.php";
	QDesktopServices::openUrl(QUrl(link));
	});

	donate_ = new QAction(QET::Icons::QETDonate, tr("Soutenir le projet par un don"), this);
	donate_ -> setStatusTip(tr("Soutenir le projet QElectroTech par un don", "status bar tip"));

	connect(donate_, &QAction::triggered, [](bool) {
	QString link = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZZHC9D7C3MDPC";
	QDesktopServices::openUrl(QUrl(link));
	});

	about_qt_ = new QAction(QET::Icons::QtLogo,  tr("À propos de &Qt"), this);
	about_qt_ -> setStatusTip(tr("Affiche des informations sur la bibliothèque Qt", "status bar tip"));
	connect(about_qt_, &QAction::triggered, qApp, &QApplication::aboutQt);

	diagnostics_action_ = new QAction(QET::Icons::DialogInformation, tr("Enregistrer un rapport de diagnostic..."), this);
	diagnostics_action_ -> setStatusTip(tr("Génère un rapport avec les derniers messages de journalisation, pour l'inclure dans un rapport de bug", "status bar tip"));
	connect(diagnostics_action_, &QAction::triggered, this, []() {
		QETApp::instance()->showDiagnosticsReport();
	});
}

/**
	Initialize common menus.
*/
void QETMainWindow::initCommonMenus()
{
	settings_menu_ = new QMenu(tr("&Configuration", "window menu"), this);
	settings_menu_ -> addAction(fullscreen_action_);
	settings_menu_ -> addAction(configure_action_);
	connect(settings_menu_, &QMenu::aboutToShow, this, &QETMainWindow::checkToolbarsmenu);

	help_menu_ = new QMenu(tr("&Aide", "window menu"), this);
	help_menu_ -> addAction(diagnostics_action_);
	help_menu_ -> addSeparator();
	help_menu_ -> addAction(whatsthis_action_);
	help_menu_ -> addSeparator();
	help_menu_ -> addAction(manual_online_);
	help_menu_ -> addAction(youtube_);
	help_menu_ -> addAction(upgrade_);
	help_menu_ -> addAction(upgrade_M);
	help_menu_ -> addAction(donate_);
	help_menu_ -> addAction(about_qt_);
	help_menu_ -> addAction(about_qet_);

#ifdef Q_OS_WIN32
upgrade_ -> setVisible(true);
#else
upgrade_ -> setVisible(false);
#endif

#ifdef Q_OS_MACOS
upgrade_M -> setVisible(true);
#else
upgrade_M -> setVisible(false);
#endif

	insertMenu(nullptr, settings_menu_);
	insertMenu(nullptr, help_menu_);
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
	return(menu_actions_.value(menu, nullptr));
}

/**
	Toggle the window from/to full screen.
*/
void QETMainWindow::toggleFullScreen()
{
	setWindowState(windowState() ^ Qt::WindowFullScreen);
}

/**
	Update the look of the full screen action according to the current state of
	the window.
*/
void QETMainWindow::updateFullScreenAction()
{
	if (windowState() & Qt::WindowFullScreen) {
		fullscreen_action_ -> setText(tr("Sortir du &mode plein écran"));
		fullscreen_action_ -> setIcon(QET::Icons::FullScreenExit);
		fullscreen_action_ -> setStatusTip(tr("Affiche QElectroTech en mode fenêtré", "status bar tip"));
	} else {
		fullscreen_action_ -> setText(tr("Passer en &mode plein écran"));
		fullscreen_action_ -> setIcon(QET::Icons::FullScreenEnter);
		fullscreen_action_ -> setStatusTip(tr("Affiche QElectroTech en mode plein écran", "status bar tip"));
	}
	ShortcutManager::instance().registerAction(fullscreen_action_, "mainwindow.fullscreen", tr("Général"), Qt::CTRL | Qt::SHIFT | Qt::Key_F);
}

/**
	Check whether a sub menu dedicated to docks and toolbars can be inserted on
	top of the settings menu.
*/
void QETMainWindow::checkToolbarsmenu()
{
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
/**
	@brief QETMainWindow::activateMenuBar
	Open the first usable menu, as pressing Alt and a menu's letter would.

	F10 is what most applications use for this, and QMenuBar does not handle
	it: given the key directly it leaves it unaccepted, and sent to the window
	it never reaches the menu bar at all, because a key press goes to the
	focused child widget. So the press fell through to whichever widget had
	focus and looked like nothing happening.

	This is convenience, not access. Qt already provides two keyboard routes
	into the menus and both work: a bare Alt tap focuses the bar, and Alt with
	a menu's letter opens it. This adds the key people reach for out of habit.

	A shortcut rather than a keyPressEvent() override, for the reason above --
	the window never sees the key while a child holds focus.
*/
void QETMainWindow::activateMenuBar() {
	QMenuBar *bar = menuBar();
	if (!bar) return;

	for (QAction *action : bar -> actions()) {
		if (action -> isVisible() && action -> isEnabled() && action -> menu()) {
			bar -> setActiveAction(action);
			return;
		}
	}
}

bool QETMainWindow::event(QEvent *e) {
	if (e -> type() == QEvent::Close && refuseCloseWhileModal(e)) {
		return(true);
	}
	if (e -> type() == QEvent::WindowStateChange) {
		updateFullScreenAction();
	} else if (first_activation_ && e -> type() == QEvent::WindowActivate) {
		firstActivation(e);
		first_activation_ = false;
	}
	return(QMainWindow::event(e));
}

/**
	@brief QETMainWindow::refuseCloseWhileModal
	Refuse to close an editor window while any modal dialog is running.

	A modal dialog's exec() runs a nested event loop. If a window is closed
	during it, the window's WA_DeleteOnClose turns into a deleteLater() that
	the *nested* loop processes: the window is destroyed while code that
	belongs to it -- often the very function that opened the dialog -- is
	still on the stack. Most of QET's dialogs are stack objects parented to
	the window (BackupDialog, and every QET::QetMessageBox), so ~QWidget()
	then deletes a stack object and the process aborts (issue #904). Even a
	dialog without a parent would only trade that abort for a silent
	use-after-free in the caller.

	Qt already ignores window-manager close requests for a window blocked by
	a modal, so this is only reachable through close() called directly: the
	File > Quit action, which macOS moves into the application menu where it
	stays usable during a modal, and QETApp::quitQET() from the system tray.

	Handled in event(), before closeEvent() runs, because the editors'
	closeEvent() starts closing projects before it decides whether to accept.
	The dialog is raised so a refused quit is not silent.

	@param e : the QEvent::Close being delivered
	@return true if the close was refused and must not be processed further
*/
bool QETMainWindow::refuseCloseWhileModal(QEvent *e)
{
	QWidget *modal = QApplication::activeModalWidget();
	if (!modal) {
		return(false);
	}
	modal -> raise();
	modal -> activateWindow();
	e -> ignore();
	return(true);
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
