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

#include <QTextStream>

namespace {
	bool g_non_interactive = false;

	/**
		@brief autoAnswer
		Report a message box on stderr and pick an answer, for use when there
		is no user to click anything. @see QET::QetMessageBox::setNonInteractive
		@param severity : short word naming the kind of box, for the log line
		@param title
		@param text
		@param buttons : the buttons the caller offered
		@param defaultButton : the caller's preferred answer, may be NoButton
		@return the button to report as pressed
	*/
	QMessageBox::StandardButton autoAnswer(
			const char *severity,
			const QString &title,
			const QString &text,
			QMessageBox::StandardButtons buttons,
			QMessageBox::StandardButton defaultButton)
	{
			//Honour the caller's own default when it named one.
		if (defaultButton != QMessageBox::NoButton
			&& (buttons & defaultButton)) {
			QTextStream(stderr) << severity << ": " << title << " -- " << text
								<< "\n(no display: answered with the caller's default button)\n";
			return defaultButton;
		}

			//Otherwise prefer a "carry on" answer over one that cancels, so a
			//batch run completes rather than silently doing nothing.
		static const QMessageBox::StandardButton preference[] = {
			QMessageBox::Ok,     QMessageBox::Open,   QMessageBox::Yes,
			QMessageBox::Save,   QMessageBox::Apply,  QMessageBox::YesToAll,
			QMessageBox::Retry,  QMessageBox::Ignore, QMessageBox::Close
		};
		for (auto candidate : preference) {
			if (buttons & candidate) {
				QTextStream(stderr) << severity << ": " << title << " -- " << text
									<< "\n(no display: continuing)\n";
				return candidate;
			}
		}

			//Nothing affirmative on offer -- fall back to whatever is set.
		for (int bit = QMessageBox::Ok; bit <= QMessageBox::RestoreDefaults; bit <<= 1) {
			auto candidate = static_cast<QMessageBox::StandardButton>(bit);
			if (buttons & candidate) {
				QTextStream(stderr) << severity << ": " << title << " -- " << text
									<< "\n(no display: answered automatically)\n";
				return candidate;
			}
		}

		QTextStream(stderr) << severity << ": " << title << " -- " << text
							<< "\n(no display: no button offered)\n";
		return QMessageBox::NoButton;
	}
}

/**
	@brief QET::QetMessageBox::setNonInteractive
	@param non_interactive
*/
void QET::QetMessageBox::setNonInteractive(bool non_interactive) {
	g_non_interactive = non_interactive;
}

/**
	@brief QET::QetMessageBox::isNonInteractive
	@return true when message boxes are answered without a user
*/
bool QET::QetMessageBox::isNonInteractive() {
	return g_non_interactive;
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
	if (g_non_interactive) {
		return autoAnswer("Critical", title, text, buttons, defaultButton);
	}
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
	if (g_non_interactive) {
		return autoAnswer("Information", title, text, buttons, defaultButton);
	}
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
	if (g_non_interactive) {
		return autoAnswer("Question", title, text, buttons, defaultButton);
	}
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
	if (g_non_interactive) {
		return autoAnswer("Warning", title, text, buttons, defaultButton);
	}
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
