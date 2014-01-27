/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "qetmessagebox.h"

/**
	@see Documentation Qt pour QMessageBox::critical
*/
QMessageBox::StandardButton QET::MessageBox::critical   (QWidget *parent, const QString &title, const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
#ifdef Q_WS_MAC
	QMessageBox message_box(QMessageBox::Critical, title, text, buttons, parent, Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(QMessageBox::Critical, title, text, buttons, parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}

/**
	@see Documentation Qt pour QMessageBox::information
*/
QMessageBox::StandardButton QET::MessageBox::information(QWidget *parent, const QString &title, const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
#ifdef Q_WS_MAC
	QMessageBox message_box(QMessageBox::Information, title, text, buttons, parent, Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(QMessageBox::Information, title, text, buttons, parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}

/**
	@see Documentation Qt pour QMessageBox::question
*/
QMessageBox::StandardButton QET::MessageBox::question   (QWidget *parent, const QString &title, const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
#ifdef Q_WS_MAC
	QMessageBox message_box(QMessageBox::Question, title, text, buttons, parent, Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(QMessageBox::Question, title, text, buttons, parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}

/**
	@see Documentation Qt pour QMessageBox::warning
*/
QMessageBox::StandardButton QET::MessageBox::warning    (QWidget *parent, const QString &title, const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
#ifdef Q_WS_MAC
	QMessageBox message_box(QMessageBox::Warning, title, text, buttons, parent, Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(QMessageBox::Warning, title, text, buttons, parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}
