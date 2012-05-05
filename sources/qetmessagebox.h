/*
	Copyright 2006-2012 Xavier Guerrin
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
		Ce namespace definit certaines methodes statiques visant a recreer le
		comportement de certaines methodes statiques de la classe Qt
		QMessageBox.
		Le comportement est sense etre le meme sur les plateformes X11 et
		Windows. En revanche, sous MacOS, si un widget parent est fourni, ces
		methodes s'assurent que la boite de dialogue sera "window modal" mais
		aussi qu'elle sera dotee du flag "Qt:Sheet", qui permet une meilleure
		integration sous MacOS.
	*/
	namespace MessageBox {
		QMessageBox::StandardButton critical   (QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton information(QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton question   (QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton warning    (QWidget *, const QString &, const QString &, QMessageBox::StandardButtons = QMessageBox::Ok, QMessageBox::StandardButton = QMessageBox::NoButton);
	};
};
#endif
