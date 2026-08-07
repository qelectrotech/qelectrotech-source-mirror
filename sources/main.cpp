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
#include "cli_export.h"
#include "logging/eventloopwatchdog.h"
#include "logging/qetlogger.h"
#include "machine_info.h"
#include "qet.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "singleapplication.h"
#include "utils/qetsettings.h"

#include <QApplication>
#include <QDomImplementation>

#include <QStyleFactory>
#include <QtConcurrentRun>

#ifdef Q_OS_MACOS
#include <QFileOpenEvent>

/**
	@brief EarlyFileOpenCatcher
	On macOS, a cold launch via Finder double-click can deliver the
	QFileOpenEvent to QApplication before QETApp exists and before its
	real eventFilter is installed (the event loop can start servicing
	native/Cocoa events before our own code in main() reaches that
	point). This tiny filter is installed immediately on `app` so no
	QFileOpenEvent can slip through unseen; it just buffers the path.
	Once QETApp is constructed, main() drains the buffer and installs
	the real QETApp::eventFilter for any subsequent event.
*/
class EarlyFileOpenCatcher : public QObject
{
	public:
		using QObject::QObject;
		QStringList bufferedFiles;

	protected:
		bool eventFilter(QObject *object, QEvent *e) override
		{
			if (e->type() == QEvent::FileOpen) {
				bufferedFiles << static_cast<QFileOpenEvent *>(e)->file();
				return true;
			}
			return QObject::eventFilter(object, e);
		}
};
#endif

/**
	@brief qetLogMessageHandler
	Installed via qInstallMessageHandler(); forwards to QetLogger, which
	holds all the actual formatting/ring/rotation state. See
	logging/qetlogger.h for the rationale (discussion #644).
*/
void qetLogMessageHandler(QtMsgType type,
			 const QMessageLogContext &context,
			 const QString &msg)
{
	QetLogger::instance().handleMessage(type, context, msg);
}

/**
	@brief main
	Main function of QElectroTech
	@param argc : number of parameters
	\~French number of paramètres
	\~ @param argv : parameters
	\~French paramètres
	\~ @return exit code
*/
int main(int argc, char **argv)
{
	// before creating Application:
	// export environment-variable "QT_HASH_SEED" with value "0" to
	// disable radomisation for hashes in order to obtain "clean" XML-diffs:
	qputenv("QT_HASH_SEED", "0");
	//Some setup, notably to use with QSetting.
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setOrganizationDomain("qelectrotech.org");
	QCoreApplication::setApplicationName("QElectroTech");

	// Refuse invalid data when building QDom documents instead of
	// serializing malformed XML (CVE-2026-15037). This is the default
	// from Qt 6.12 on; opt in explicitly for older Qt 5/6.
	QDomImplementation::setInvalidDataPolicy(
		QDomImplementation::ReturnNullNode);
	//Creation and execution of the application
	//HighDPI
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)	// ### Qt 6: remove
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
#endif


#if QT_VERSION > QT_VERSION_CHECK(5, 7, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0) // ### Qt 6: remove
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif


#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
QGuiApplication::setHighDpiScaleFactorRoundingPolicy(QetSettings::hdpiScaleFactorRoundingPolicy());
#endif


	// Headless command-line export: render a project to PDF/PNG/SVG without
	// opening the GUI, then exit.  Must be handled before SingleApplication
	// (which would forward the args to an already-running instance).
	{
		QStringList raw_args;
		for (int i = 0; i < argc; ++i)
			raw_args << QString::fromLocal8Bit(argv[i]);
		if (CLIExport::isExportRequest(raw_args)) {
			QApplication export_app(argc, argv);
			// No crash-recovery backups in one-shot CLI mode: the backup write
			// runs on a background thread referencing the project and races the
			// process exit (intermittent segfault in QET::writeToFile).
			QETProject::setBackupEnabled(false);
			// Answer message boxes instead of showing them: opening a project
			// saved by an older QElectroTech raises a warning from
			// QETProject::readProjectXml(), and with nobody able to dismiss it
			// QDialog::exec() would spin its event loop forever.
			QET::QetMessageBox::setNonInteractive(true);
			return CLIExport::run(export_app.arguments());
		}
	}

	// Resolve the logger's state (log directory, session filename, open
	// file handle) explicitly here, immediately before installing the
	// handler -- not implicitly on whichever thread happens to log
	// first. See QetLogger::init().
	//
	// Install the log-file message handler BEFORE the application starts:
	// QETApp's constructor does the whole startup (collections, editor,
	// opening the projects given on the command line), so installing the
	// handler afterwards - as was done in the startup worker below - meant
	// exactly the interesting lines (collection and project load timers)
	// went to stderr, which is invisible in a Windows GUI session.
	QetLogger::instance().init();
	qInstallMessageHandler(qetLogMessageHandler);
	// Step 4 (discussion #644): flush the ring to a crash-dump file if
	// the process dies from here on. Installed right after the ring
	// exists (init() just constructed it) and as early as reasonably
	// possible, so it also covers whatever runs between here and
	// QETApp's own construction below.
	QetLogger::instance().installCrashHandler();

	SingleApplication app(argc, argv, true);
#ifdef Q_OS_MACOS
	app.setStyle(QStyleFactory::create("Fusion"));
	// Installed as early as possible, before anything else can run an
	// event loop, to catch a QFileOpenEvent that might be delivered
	// during a cold launch before QETApp exists.
	EarlyFileOpenCatcher early_catcher;
	app.installEventFilter(&early_catcher);
#endif

	if (app.isSecondary())
	{
		QStringList arg_list = app.arguments();
		//Remove the first argument, it's the binary file
		arg_list.takeFirst();
		QETArguments qetarg(arg_list);
		QString message = "launched-with-args: " + QET::joinWithSpaces(
					QStringList(qetarg.arguments()));
		app.sendMessage(message.toUtf8());
		return 0;
	}

	QETApp qetapp;
	QETApp::instance()->installEventFilter(&qetapp);
#ifdef Q_OS_MACOS
	//Handle the opening of QET when user double click on a .qet .elmt .tbt file
	//or drop these same files to the QET icon of the dock.
	//Swap the early catcher (installed right after `app` was constructed,
	//see above) for the real filter, then drain anything it buffered
	//during the cold-launch window before QETApp existed.
	app.removeEventFilter(&early_catcher);
	app.installEventFilter(&qetapp);
	if (!early_catcher.bufferedFiles.isEmpty())
		qetapp.openFiles(QETArguments(early_catcher.bufferedFiles));
#endif
	QObject::connect(&app, &SingleApplication::receivedMessage,
			 &qetapp, &QETApp::receiveMessage);

	// Pre-initialise on the main (GUI) thread: the constructor calls
	// qApp->screens() which is not thread-safe in Qt5 — calling instance()
	// here guarantees the singleton is fully built before the worker runs.
	MachineInfo::instance();

	[[maybe_unused]] auto startup_future = QtConcurrent::run([=]()
	{
		qInfo("Start-up");
		// delete old log files of max 7 days old.
		QetLogger::instance().pruneOldLogFiles(7);
		MachineInfo::instance()->send_info_to_debug();
	});

	// Constructed here rather than earlier: start() measures ticks against
	// the event loop app.exec() is about to run, so there is no point
	// (and no accurate baseline) before this line.
	EventLoopWatchdog watchdog;
	watchdog.start();

	return app.exec();
}

