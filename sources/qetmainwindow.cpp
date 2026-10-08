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
#include "ui/aiassistantdialog.h"
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

	configure_action_ = new QAction(QET::Icons::Configure, tr("&Configure QElectroTech"), this);
	ShortcutManager::instance().registerAction(configure_action_, "mainwindow.configure", tr("General"), QKeySequence());
	configure_action_ -> setStatusTip(tr("Allows to specify various parameters for QElectroTech", "status bar tip"));
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

	customize_action_ = new QAction(QET::Icons::ConfigureToolbars, tr("&Customize..."), this);
	ShortcutManager::instance().registerAction(customize_action_, "mainwindow.customize", tr("General"), QKeySequence());
	customize_action_ -> setStatusTip(tr("Toolbars, shortcut bar, keyboard and mouse gestures, in one window", "status bar tip"));
	connect(customize_action_, &QAction::triggered, qet_app, [qet_app]() { qet_app->customizeQET(); });

	export_config_action_ = new QAction(QET::Icons::DocumentExport, tr("Save settings as..."), this);
	export_config_action_ -> setStatusTip(tr("Saves QElectroTech's settings to a file", "status bar tip"));
	connect(export_config_action_, &QAction::triggered, qet_app, &QETApp::exportConfiguration);
	ShortcutManager::instance().registerAction(export_config_action_, "mainwindow.export_configuration", tr("General"), QKeySequence());

	import_config_action_ = new QAction(QET::Icons::DocumentImport, tr("Load settings..."), this);
	import_config_action_ -> setStatusTip(tr("Replaces QElectroTech's settings with those of a file, then closes QElectroTech", "status bar tip"));
	connect(import_config_action_, &QAction::triggered, qet_app, &QETApp::importConfiguration);
	ShortcutManager::instance().registerAction(import_config_action_, "mainwindow.import_configuration", tr("General"), QKeySequence());

	fullscreen_action_ = new QAction(this);
	updateFullScreenAction();
	connect(fullscreen_action_, &QAction::triggered, this, &QETMainWindow::toggleFullScreen);

	whatsthis_action_ = QWhatsThis::createAction(this);
	ShortcutManager::instance().registerAction(whatsthis_action_, "mainwindow.whats_this", tr("General"), Qt::SHIFT | Qt::Key_F1);

	about_qet_ = new QAction(QET::Icons::QETLogo, tr("A&bout QElectroTech"), this);
	ShortcutManager::instance().registerAction(about_qet_, "mainwindow.about_qet", tr("General"), QKeySequence());
	about_qet_ -> setStatusTip(tr("Displays information about QElectroTech", "status bar tip"));
	connect(about_qet_, &QAction::triggered, qet_app, &QETApp::aboutQET);

	manual_online_ = new QAction(QET::Icons::QETManual, tr("Online manual"), this);
	manual_online_ -> setStatusTip(tr("Launches the default browser to the online manual QElectroTech", "status bar tip"));

	connect(manual_online_, &QAction::triggered, [](bool) {
	QString link = "https://download.qelectrotech.org/qet/manual_0.7/build/index.html";
	QDesktopServices::openUrl(QUrl(link));
	});

	ShortcutManager::instance().registerAction(manual_online_, "mainwindow.manual_online", tr("General"), Qt::Key_F1);

	connect_ai_ = new QAction(tr("Connect an AI assistant..."), this);
	ShortcutManager::instance().registerAction(connect_ai_, "mainwindow.connect_ai", tr("General"), QKeySequence());
	connect_ai_ -> setStatusTip(tr("Shows the setup that lets an AI assistant use QElectroTech", "status bar tip"));
	connect(connect_ai_, &QAction::triggered, this, [this]() {
		AiAssistantDialog dialog(this);
		dialog.exec();
	});

	youtube_ = new QAction(QET::Icons::QETVideo, tr("Youtube channel"), this);
	ShortcutManager::instance().registerAction(youtube_, "mainwindow.youtube", tr("General"), QKeySequence());
	youtube_ -> setStatusTip(tr("Launches the default browser on the Youtube channel of QElectroTech", "status bar tip"));

	connect(youtube_, &QAction::triggered, [](bool) {
	QString link = "https://www.youtube.com/user/scorpio8101/videos";
	QDesktopServices::openUrl(QUrl(link));
	});

	upgrade_ = new QAction(QET::Icons::QETDownload, tr("Download a new version (dev)"), this);
	ShortcutManager::instance().registerAction(upgrade_, "mainwindow.download_windows", tr("General"), QKeySequence());
	upgrade_ -> setStatusTip(tr("Launches the default browser to the online repository Nightly QElectroTech", "status bar tip"));

	upgrade_M = new QAction(QET::Icons::QETDownload, tr("Download a new version (dev)"), this);
	ShortcutManager::instance().registerAction(upgrade_M, "mainwindow.download_mac", tr("General"), QKeySequence());
	upgrade_M -> setStatusTip(tr("Launches the default browser to the online repository Nightly QElectroTech", "status bar tip"));

	connect(upgrade_, &QAction::triggered, [](bool) {
	QString link = "https://qelectrotech.org/download_windows.php";
	QDesktopServices::openUrl(QUrl(link));
	});

	connect(upgrade_M, &QAction::triggered, [](bool) {
	QString link = "https://qelectrotech.org/download_mac.php";
	QDesktopServices::openUrl(QUrl(link));
	});

	donate_ = new QAction(QET::Icons::QETDonate, tr("Support the project with a donation"), this);
	ShortcutManager::instance().registerAction(donate_, "mainwindow.donate", tr("General"), QKeySequence());
	donate_ -> setStatusTip(tr("Support the QElectroTech project with a donation", "status bar tip"));

	connect(donate_, &QAction::triggered, [](bool) {
	QString link = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZZHC9D7C3MDPC";
	QDesktopServices::openUrl(QUrl(link));
	});

	about_qt_ = new QAction(QET::Icons::QtLogo,  tr("About &Qt"), this);
	ShortcutManager::instance().registerAction(about_qt_, "mainwindow.about_qt", tr("General"), QKeySequence());
	about_qt_ -> setStatusTip(tr("Displays information about Qt library", "status bar tip"));
	connect(about_qt_, &QAction::triggered, qApp, &QApplication::aboutQt);

	diagnostics_action_ = new QAction(QET::Icons::DialogInformation, tr("Save a diagnostic report..."), this);
	ShortcutManager::instance().registerAction(diagnostics_action_, "mainwindow.diagnostics_report", tr("General"), QKeySequence());
	diagnostics_action_ -> setStatusTip(tr("Generates a report containing the latest log entries, to be included in a bug report", "status bar tip"));
	connect(diagnostics_action_, &QAction::triggered, this, []() {
		QETApp::instance()->showDiagnosticsReport();
	});
}

/**
	Initialize common menus.
*/
void QETMainWindow::initCommonMenus()
{
	settings_menu_ = new QMenu(tr("&Settings", "window menu"), this);
	settings_menu_ -> addAction(fullscreen_action_);
	settings_menu_ -> addAction(configure_action_);
	settings_menu_ -> addAction(customize_action_);
	settings_menu_ -> addSeparator();
	settings_menu_ -> addAction(export_config_action_);
	settings_menu_ -> addAction(import_config_action_);
	connect(settings_menu_, &QMenu::aboutToShow, this, &QETMainWindow::checkToolbarsmenu);

	help_menu_ = new QMenu(tr("&Help", "window menu"), this);
	help_menu_ -> addAction(diagnostics_action_);
	help_menu_ -> addSeparator();
	help_menu_ -> addAction(whatsthis_action_);
	help_menu_ -> addSeparator();
	help_menu_ -> addAction(manual_online_);
	help_menu_ -> addAction(connect_ai_);
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
		fullscreen_action_ -> setText(tr("Leave F&ullScreen Mode"));
		fullscreen_action_ -> setIcon(QET::Icons::FullScreenExit);
		fullscreen_action_ -> setStatusTip(tr("Displays QElectroTech in windowed mode", "status bar tip"));
	} else {
		fullscreen_action_ -> setText(tr("F&ullScreen Mode"));
		fullscreen_action_ -> setIcon(QET::Icons::FullScreenEnter);
		fullscreen_action_ -> setStatusTip(tr("Displays QElectroTech in full screen mode", "status bar tip"));
	}
	ShortcutManager::instance().registerAction(fullscreen_action_, "mainwindow.fullscreen", tr("General"), Qt::CTRL | Qt::SHIFT | Qt::Key_F);
}

/**
	@brief QETMainWindow::createPopupMenu
	The menu shown on a right-click on a toolbar or a dock title, and the
	Configuration > Afficher submenu: Qt's list of toolbars and docks,
	then Personnaliser..., as in most applications with toolbars.
*/
QMenu *QETMainWindow::createPopupMenu()
{
	QMenu *menu = QMainWindow::createPopupMenu();
	if (menu) {
		menu -> addSeparator();
		menu -> addAction(customize_action_);
	}
	return menu;
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
		display_toolbars_ -> setTitle(tr("Display", "menu entry"));
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
