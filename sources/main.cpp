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
#include "machine_info.h"
#include "qet.h"
#include "qetapp.h"
#include "qetproject.h"
#include "singleapplication.h"
#include "utils/qetsettings.h"

#include <QApplication>

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
	@brief myMessageOutput
	for debugging
	@param type : the messages that can be sent to a message handler
	@param context : were? wat?
	@param msg : Message
*/
void myMessageOutput(QtMsgType type,
			 const QMessageLogContext &context,
			 const QString &msg)
{

	QString txt=QTime::currentTime().toString("hh:mm:ss.zzz");
	QByteArray dbs =txt.toLocal8Bit();
	QByteArray localMsg = msg.toLocal8Bit();
	const char *file = context.file ? context.file : "";
	const char *function = context.function ? context.function : "";

	switch (type) {
	case QtDebugMsg:
		fprintf(stderr,
			"%s Debug: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
		txt+=" Debug: ";
		break;
	case QtInfoMsg:
		fprintf(stderr,
			"%s Info: %s \n",
			dbs.constData(),
			localMsg.constData());
		txt+=" Info: ";
		break;
	case QtWarningMsg:
		fprintf(stderr,
			"%s Warning: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file, context.line,
			function);
		txt+=" Warning: ";
		break;
	case QtCriticalMsg:
		fprintf(stderr,
			"%s Critical: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
		txt+=" Critical: ";
		break;
	case QtFatalMsg:
		fprintf(stderr,
			"%s Fatal: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
		txt+=" Fatal: ";
		break;
	default:
		fprintf(stderr,
			"%s Unknown: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
		txt+=" Unknown: ";
	}
	txt+= msg;
	if(type==QtInfoMsg){
		txt+=" \n";
	} else {
		txt+= " (";
		txt+= context.file ? context.file : "";
		txt+= ":";
		txt+=QString::number(context.line ? context.line :0);
		txt+= ", ";
		txt+= context.function ? context.function : "";
		txt+=")\n";
	}
	QFile outFile(QETApp::dataDir()
			  +"/"
			  +QDate::currentDate().toString("yyyyMMdd")
			  +".log");
	if(outFile.open(QIODevice::WriteOnly | QIODevice::Append))
	{
		QTextStream ts(&outFile);
		ts << txt;
	}
	outFile.close();
}

/**
	@brief delete_old_log_files
	delete old log files
	@param days : max days old
*/
void delete_old_log_files(int days)
{
	const QDate today = QDate::currentDate();
	const QString path = QETApp::dataDir() % "/";

	QString filter("%1%1%1%1%1%1%1%1.log"); // pattern
	filter = filter.arg("[0123456789]"); // valid characters

	Q_FOREACH (auto fileInfo,
		   QDir(path).entryInfoList(
			   QStringList(filter),
			   QDir::Files))
	{
		if (fileInfo.lastRead().date().daysTo(today) > days)
		{
			QString filepath = fileInfo.absoluteFilePath();
			QDir deletefile;
			deletefile.setPath(filepath);
			deletefile.remove(filepath);
			qDebug() << "File " % filepath % " is deleted!";
		}
	}
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
			return CLIExport::run(export_app.arguments());
		}
	}

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

	QtConcurrent::run([=]()
	{
		// for debugging
		qInstallMessageHandler(myMessageOutput);
		qInfo("Start-up");
		// delete old log files of max 7 days old.
		delete_old_log_files(7);
		MachineInfo::instance()->send_info_to_debug();
	});
	return app.exec();
}

