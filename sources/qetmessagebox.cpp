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
#include "qetmessagebox.h"

#include <QDebug>

#include "qet.h"

namespace {
	/**
		Answer a message box without a user.

		In a headless run (QET::isInteractive() == false) there is nobody to
		click a button, so opening the dialog would block the process forever.
		Report the question on stderr instead and hand back @p defaultButton.

		A call site that has not specified a default gets QMessageBox::NoButton,
		which is deliberate: almost every caller is written as "if the user
		chose the abort option, abort", so NoButton means the operation carries
		on unchanged. Pass an explicit default wherever a specific unattended
		answer is wanted.
	*/
	QMessageBox::StandardButton answerWithoutUser(
			const char *level,
			const QString &title,
			const QString &text,
			QMessageBox::StandardButton defaultButton)
	{
		qWarning().noquote() << QStringLiteral("%1 (headless, no user to ask): %2 -- %3")
								.arg(QString::fromLatin1(level), title, text);
		return defaultButton;
	}
}

/**
	@see Documentation Qt pour QMessageBox::critical
*/
QMessageBox::StandardButton QET::QetMessageBox::critical (
		QWidget *parent,
		const QString &title,
		const QString &text,
		QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton)
{
	if (!QET::isInteractive())
		return answerWithoutUser("critical", title, text, defaultButton);

#ifdef Q_OS_MACOS
	QMessageBox message_box(
				QMessageBox::Critical,
				title,
				text,
				buttons,
				parent,
				Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(
				QMessageBox::Critical,
				title,
				text,
				buttons,
				parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}

/**
	@see Documentation Qt pour QMessageBox::information
*/
QMessageBox::StandardButton QET::QetMessageBox::information(
		QWidget *parent,
		const QString &title,
		const QString &text,
		QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton)
{
	if (!QET::isInteractive())
		return answerWithoutUser("information", title, text, defaultButton);

#ifdef Q_OS_MACOS
	QMessageBox message_box(
				QMessageBox::Information,
				title,
				text,
				buttons,
				parent,
				Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(
				QMessageBox::Information,
				title,
				text,
				buttons,
				parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}

/**
	@see Documentation Qt pour QMessageBox::question
*/
QMessageBox::StandardButton QET::QetMessageBox::question (
		QWidget *parent,
		const QString &title,
		const QString &text,
		QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton)
{
	if (!QET::isInteractive())
		return answerWithoutUser("question", title, text, defaultButton);

#ifdef Q_OS_MACOS
	QMessageBox message_box(
				QMessageBox::Question,
				title,
				text,
				buttons,
				parent,
				Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(
				QMessageBox::Question,
				title,
				text,
				buttons,
				parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}

/**
	@see Documentation Qt pour QMessageBox::warning
*/
QMessageBox::StandardButton QET::QetMessageBox::warning (
		QWidget *parent,
		const QString &title,
		const QString &text,
		QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton)
{
	if (!QET::isInteractive())
		return answerWithoutUser("warning", title, text, defaultButton);

#ifdef Q_OS_MACOS
	QMessageBox message_box(
				QMessageBox::Warning,
				title,
				text,
				buttons,
				parent,
				Qt::Sheet);
	message_box.setWindowModality(Qt::WindowModal);
#else
	QMessageBox message_box(
				QMessageBox::Warning,
				title,
				text,
				buttons,
				parent);
#endif
	message_box.setDefaultButton(defaultButton);
	
	return(static_cast<QMessageBox::StandardButton>(message_box.exec()));
}
