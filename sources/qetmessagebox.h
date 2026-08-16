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
		/**
			Enable non-interactive mode.

			In non-interactive mode the functions below never construct a
			dialog. They write the message to stderr and return an answer
			immediately, so a headless run cannot block on a modal box that
			nobody is there to dismiss.

			This is needed because these are reachable from the command-line
			tools: opening a project written by an older QElectroTech raises
			a warning from QETProject::readProjectXml(), and with no display
			to click it, QDialog::exec() spins its event loop forever.

			The answer is chosen as: the caller's defaultButton when it gave
			one, otherwise the first "carry on" button among those offered
			(Ok, Open, Yes, Save, Apply...), otherwise the first button set.
			So the two warnings above resolve to Open and the project loads,
			which is what a batch invocation wants.
		*/
		void setNonInteractive(bool non_interactive);
		bool isNonInteractive();

		QMessageBox::StandardButton critical (
				QWidget *,
				const QString &,
				const QString &,
				QMessageBox::StandardButtons = QMessageBox::Ok,
				QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton information(
				QWidget *,
				const QString &,
				const QString &,
				QMessageBox::StandardButtons = QMessageBox::Ok,
				QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton question (
				QWidget *,
				const QString &,
				const QString &,
				QMessageBox::StandardButtons = QMessageBox::Ok,
				QMessageBox::StandardButton = QMessageBox::NoButton);
		QMessageBox::StandardButton warning (
				QWidget *,
				const QString &,
				const QString &,
				QMessageBox::StandardButtons = QMessageBox::Ok,
				QMessageBox::StandardButton = QMessageBox::NoButton);
	};
};
#endif
