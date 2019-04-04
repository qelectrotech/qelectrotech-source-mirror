/*
	Copyright 2006-2019 The QElectroTech Team
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
#include <QFileOpenEvent>

class MacOSXOpenEvent : public QObject
{
		Q_OBJECT

	public:
		MacOSXOpenEvent(QObject *parent = nullptr) :
			QObject(parent)
		{}

		~MacOSXOpenEvent(){}

		bool eventFilter(QObject *obj, QEvent *event)
		{
			if (event->type() == QEvent::FileOpen)
			{
				SingleApplication *app = dynamic_cast<SingleApplication *>(obj);
				QFileOpenEvent *open_event = static_cast<QFileOpenEvent*>(event);
				QString message = "launched-with-args: " + open_event->file();
				app->sendMessage(message.toUtf8());
				return true;
			}
			return false;
		}
};

/**
 * @brief main
 * Main function of QElectroTech
 * @param argc : number of paramètres
 * @param argv : paramètres
 * @return
 */
int main(int argc, char **argv)
{
		//Some setup, notably to use with QSetting.
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setOrganizationDomain("qelectrotech.org");
	QCoreApplication::setApplicationName("QElectroTech");
		//Creation and execution of the application
		//HighDPI
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#else
    qputenv("QT_DEVICE_PIXEL_RATIO", QByteArray("auto"));
#endif
	
	SingleApplication app(argc, argv, true);
#ifdef Q_OS_MACOS
		//Handle the opening of QET when user double click on a .qet .elmt .tbt file
		//or drop these same files to the QET icon of the dock
	MacOSXOpenEvent open_event;
	app.installEventFilter(open_event);
#endif
	
	if (app.isSecondary())
	{
		QStringList arg_list = app.arguments();
			//Remove the first argument, it's the binary file
		arg_list.takeFirst();
		QETArguments qetarg(arg_list);
		QString message = "launched-with-args: " + QET::joinWithSpaces(QStringList(qetarg.arguments()));
		app.sendMessage(message.toUtf8());
		return 0;
	}
	
	QETApp qetapp;
	QETApp::instance()->installEventFilter(&qetapp);
	QObject::connect(&app, &SingleApplication::receivedMessage, &qetapp, &QETApp::receiveMessage);
	
	return app.exec();
}

