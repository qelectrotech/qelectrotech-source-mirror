/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef SPLANIMPORTDIALOG_H
#define SPLANIMPORTDIALOG_H

#include <QDialog>
#include <QProcess>

class QCheckBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;

/**
	@brief Dialog of "File > Import sPlan schematic": turns an sPlan 8
	schematic (.spl8, Abacom) into a QElectroTech project that is drawn
	anew: the same devices and connections, laid out on the grid.

	The translation is the Python program
	tools/splan_import/splan_to_qet.py; the dialog finds it and a Python
	that can run it, starts it as a QProcess, moves the progress bar with
	its PROGRESS lines and hands back the written .qet, which the editor
	then opens.
*/
class SplanImportDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit SplanImportDialog(QWidget *parent = nullptr);
		~SplanImportDialog() override;

		/// asks for the schematic; false when the user cancels
		bool chooseSource();
		/// takes @p path as the schematic to import
		void setSource(const QString &path);
		/// the .qet the last import wrote, empty while there is none
		QString resultPath() const { return m_result; }

		/// splan_to_qet.py, or empty when it is nowhere to be found
		static QString findScript(const QString &application_dir);
		/// <folder of the schematic>/QET/<name>.qet, as the program names it
		static QString defaultTarget(const QString &source);
		/// the command line for the program
		static QStringList arguments(const QString &script,
					     const QString &source,
					     const QString &target,
					     bool pdf);
		/// the path after "done: " in the program's output, or empty
		static QString resultFromOutput(const QString &output);
		/// the text for a progress line "PROGRESS <percent> <step>" of the
		/// program, in the language of QElectroTech; @p percent gets the
		/// percentage. Empty when @p line is no progress line.
		static QString progressText(const QString &line, int *percent);

	protected:
		void reject() override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dropEvent(QDropEvent *event) override;

	private:
		void chooseTarget();
		void chooseScript();
		void choosePython();
		void start();
		void readOutput();
		void processFinished(int exit_code, QProcess::ExitStatus status);
		void updateButtons();
		QString usablePython(const QString &script);
		bool pythonRuns(const QString &python,
				const QString &script,
				QString *error);

		QLineEdit *m_source = nullptr;
		QLineEdit *m_target = nullptr;
		QLineEdit *m_script = nullptr;
		QLineEdit *m_python = nullptr;
		QCheckBox *m_pdf = nullptr;
		QLabel *m_status = nullptr;
		QPlainTextEdit *m_log = nullptr;
		QProgressBar *m_progress = nullptr;
		QPushButton *m_start = nullptr;
		QPushButton *m_close = nullptr;
		QProcess *m_process = nullptr;
		QString m_output;
		QString m_pending;	///< output after the last line break
		QString m_result;
		bool m_target_edited = false;	///< the user chose the target
};

#endif // SPLANIMPORTDIALOG_H
