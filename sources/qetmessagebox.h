/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef QET_MESSAGE_BOX_H
#define QET_MESSAGE_BOX_H
#include <QMessageBox>
namespace QET {
	/**
		This namespace defines static methods behaving in a very similar way to some
		static methods from the QMessageBox Qt class. The behavior is expected to be
		the same under X11 and Windows. However, under MacOS, when a parent widget is
		provided, these methodes ensure the dialog box is "window modal" and has the
		Qt:Sheet flag, thus enabling a better MacOS integration.
	*/
	namespace QetMessageBox {
		QMessageBox::StandardButton critical   (QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton information(QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton question   (QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton warning    (QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
	};
};
#endif
