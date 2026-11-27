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
#ifndef EPLANPROJECTIMPORTDIALOG_H
#define EPLANPROJECTIMPORTDIALOG_H

#include <QDialog>
#include <QProcess>

#include <memory>

class QCheckBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTemporaryDir;

/**
	@brief Dialog of "File > Import EPLAN project": turns an EPLAN export
	archive (.epdz) into a QElectroTech project.

	The dialog unpacks the archive itself (bundled LZMA SDK). The
	translation into an editable project -- parts with their terminals,
	conductors where EPLAN drew them, linked interruption points -- is the
	Python program tools/eplan_import/eplan_zw1/epdz_nach_qet.py, which
	drives a headless copy of this QElectroTech through the scripting API
	(misc/qet-mcp/qet_mcp.py); the dialog finds it and a Python that can
	run it, starts it as a QProcess, shows its output and hands back the
	written .qet, which the editor then opens. Without the program, a
	Python or scripting, the pages come in as pictures (EpdzImport).
*/
class EplanProjectImportDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit EplanProjectImportDialog(QWidget *parent = nullptr);
		~EplanProjectImportDialog() override;

		/// asks for the archive; false when the user cancels
		bool chooseArchive();
		/// takes @p path as the archive to import
		void setArchive(const QString &path);
		/// the .qet the last import wrote, empty while there is none
		QString resultPath() const { return m_result; }

	protected:
		void reject() override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dropEvent(QDropEvent *event) override;

	private:
		void chooseTarget();
		void chooseScript();
		void choosePython();
		void start();
		void importAsPictures(const QString &archive, const QString &target);
		void readOutput();
		void processFinished(int exit_code, QProcess::ExitStatus status);
		void updateButtons();
		void appendLog(const QString &text);
		QString usablePython(const QString &script);
		bool pythonRuns(const QString &python, const QString &script, QString *error);

		QLineEdit *m_archive = nullptr;
		QLineEdit *m_target = nullptr;
		QLineEdit *m_script = nullptr;
		QLineEdit *m_python = nullptr;
		QCheckBox *m_pdf = nullptr;
		QCheckBox *m_keep_parts = nullptr;
		QLabel *m_status = nullptr;
		QPlainTextEdit *m_log = nullptr;
		QProgressBar *m_progress = nullptr;
		QPushButton *m_start = nullptr;
		QPushButton *m_close = nullptr;
		QProcess *m_process = nullptr;
		std::unique_ptr<QTemporaryDir> m_unpacked;	///< the archive, unpacked for the program
		QString m_output;
		QString m_pending;	///< output after the last line break
		QString m_result;
		bool m_target_edited = false;	///< the user chose the target themself
};

#endif // EPLANPROJECTIMPORTDIALOG_H
