/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "qetapp.h"
#include "singleapplication.h"
#include "qet.h"
#include "macosxopenevent.h"
#include <QStyleFactory>

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

	QByteArray dbs =
		QTime::currentTime().toString("hh:mm:ss.zzz").toLocal8Bit();
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
		break;
	case QtInfoMsg:
		fprintf(stderr,
			"%s Info: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
		break;
	case QtWarningMsg:
		fprintf(stderr,
			"%s Warning: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file, context.line,
			function);
		break;
	case QtCriticalMsg:
		fprintf(stderr,
			"%s Critical: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
		break;
	case QtFatalMsg:
		fprintf(stderr,
			"%s Fatal: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
		break;
	default:
		fprintf(stderr,
			"%s Unknown: %s (%s:%u, %s)\n",
			dbs.constData(),
			localMsg.constData(),
			file,
			context.line,
			function);
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
	qInstallMessageHandler(myMessageOutput);
	//Some setup, notably to use with QSetting.
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setOrganizationDomain("qelectrotech.org");
	QCoreApplication::setApplicationName("QElectroTech");
	qDebug()<<"test message generated";
	qInfo()<<"OS:"
		  + QString(QSysInfo::kernelType())
		  + "-"
		  + QString(QSysInfo::currentCpuArchitecture())
		  + " Version:"
		  + QString(QSysInfo::prettyProductName())
		  + " Kernel:"
		  + QString(QSysInfo::kernelVersion());
	//Creation and execution of the application
	//HighDPI
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
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
	
	return app.exec();
}

