/*
	Copyright 2006-2017 The QElectroTech Team
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
#if QT_VERSION >= QT_VERSION_CHECK (5, 6, 0)
#if defined Q_OS_MAC
    QApplication::setAttribute (Qt::AA_EnableHighDpiScaling);
#elif !defined(Q_OS_MAC)
    QApplication::setAttribute (Qt::AA_DisableHighDpiScaling);
#else
    qputenv("QT_DEVICE_PIXEL_RATIO", QByteArray("1"));
#endif
#endif
	return(QETApp(argc, argv).exec());
}
