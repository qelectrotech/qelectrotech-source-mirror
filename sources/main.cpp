/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "machine_info.h"
#include "qet.h"
#include "qetapp.h"
#include "singleapplication.h"
#include "utils/macosxopenevent.h"
#include "utils/qetsettings.h"

#include <QStyleFactory>
#include <QThreadPool>

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
	const QString path = QETApp::dataDir() + "/";

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
			qDebug() << "File " + filepath + " is deleted!";
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


	SingleApplication app(argc, argv, true);
#ifdef Q_OS_MACOS
	//Handle the opening of QET when user double click on a .qet .elmt .tbt file
	//or drop these same files to the QET icon of the dock
	MacOSXOpenEvent open_event;
	app.installEventFilter(&open_event);
	app.setStyle(QStyleFactory::create("Fusion"));
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
	QObject::connect(&app, &SingleApplication::receivedMessage,
			 &qetapp, &QETApp::receiveMessage);

	QThreadPool::globalInstance()->start([=]()
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

