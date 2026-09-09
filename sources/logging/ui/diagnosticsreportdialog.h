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
#ifndef DIAGNOSTICSREPORTDIALOG_H
#define DIAGNOSTICSREPORTDIALOG_H

#include <QDialog>

/**
	@brief The DiagnosticsReportDialog class
	Discussion #644, step 5: "Show what's in it before saving -- the user
	is about to attach this to a public tracker." Used for both the
	after-a-crash offer (QETApp::checkBackupFiles()) and the manual
	"Help > Diagnostics > Save report" action -- the only difference
	between the two is the intro text and where the content comes from
	(QetLogger::pendingCrashDumpContents() vs. buildDiagnosticsReport()).

	The content passed in is expected to already be redacted
	(QetLogger::redact()) -- this dialog just displays and optionally
	saves whatever it's given.
*/
class DiagnosticsReportDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit DiagnosticsReportDialog(
				const QString &title,
				const QString &intro,
				const QByteArray &content,
				QWidget *parent = nullptr);

	private slots:
		void saveToFile();
};

#endif // DIAGNOSTICSREPORTDIALOG_H
