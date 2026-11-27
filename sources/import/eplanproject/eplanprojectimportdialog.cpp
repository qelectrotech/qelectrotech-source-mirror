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
#include "eplanprojectimportdialog.h"
#include "eplanprojectimport.h"
#include "epdzimport.h"

#include "../../qetapp.h"
#include "../edz/edzsevenzip.h"

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>

namespace {
	const char DIR_KEY[]    = "eplanimport/project_dir";
	const char PYTHON_KEY[] = "eplanimport/python";
	const char SCRIPT_KEY[] = "eplanimport/project_script";
	const char PDF_KEY[]    = "eplanimport/project_pdf";
	const char KEEP_KEY[]   = "eplanimport/project_keep_parts";

	/**
		What the program needs besides the arguments: this QElectroTech,
		which it runs headless with scripting switched on for that run
		only, the collections of this QElectroTech and UTF-8 output.
	*/
	QProcessEnvironment importEnvironment()
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QET_BINARY"),
				   QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("QET_CUSTOM_ELEMENTS_DIR"),
				   QDir::toNativeSeparators(QETApp::customElementsDir()));
		const QString common = QETApp::commonElementsDir();
		if (!common.isEmpty())
			env.insert(QStringLiteral("QET_COMMON_ELEMENTS_DIR"), QDir::toNativeSeparators(common));
		env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
		env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
		env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
		return env;
	}

	QLineEdit *pathRow(QWidget *parent, QFormLayout *form, const QString &label,
					   QPushButton **button)
	{
		auto *row = new QHBoxLayout;
		auto *edit = new QLineEdit(parent);
		*button = new QPushButton(QCoreApplication::translate("EplanProjectImportDialog", "Change…"), parent);
		row->addWidget(edit, 1);
		row->addWidget(*button);
		form->addRow(label, row);
		return edit;
	}

	bool isEpdz(const QString &path)
	{
		return path.endsWith(QLatin1String(".epdz"), Qt::CaseInsensitive);
	}
}

EplanProjectImportDialog::EplanProjectImportDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Import an EPLAN project"));
	setAcceptDrops(true);
	resize(760, 560);

	auto *intro = new QLabel(tr("Reads an EPLAN export archive (.epdz) and writes a QElectroTech project "
								"with its pages, parts, conductors and interruption points. The import "
								"opens in QElectroTech when it is done. Without Python 3 the pages come "
								"in as pictures."), this);
	intro->setWordWrap(true);

	auto *form = new QFormLayout;
	QPushButton *archive_button = nullptr, *target_button = nullptr,
		*script_button = nullptr, *python_button = nullptr;
	m_archive = pathRow(this, form, tr("EPLAN export archive (.epdz):"), &archive_button);
	m_target = pathRow(this, form, tr("Save as:"), &target_button);

	QSettings settings;
	m_pdf = new QCheckBox(tr("Also write a PDF (takes a while on big projects)"), this);
	m_pdf->setChecked(settings.value(PDF_KEY, false).toBool());
	m_keep_parts = new QCheckBox(tr("Keep the parts made from the EPLAN symbols in the user collection"), this);
	m_keep_parts->setChecked(settings.value(KEEP_KEY, false).toBool());
	form->addRow(QString(), m_pdf);
	form->addRow(QString(), m_keep_parts);

	m_script = pathRow(this, form, tr("Import program:"), &script_button);
	m_script->setText(QDir::toNativeSeparators(EplanProjectImport::findScript(
		QCoreApplication::applicationDirPath(), settings.value(SCRIPT_KEY).toString())));
	m_python = pathRow(this, form, tr("Python:"), &python_button);
	m_python->setText(settings.value(PYTHON_KEY).toString());
	m_python->setPlaceholderText(tr("found when the import starts"));

	m_status = new QLabel(this);
	m_status->setWordWrap(true);
	m_progress = new QProgressBar(this);
	m_progress->setRange(0, 100);
	m_progress->setValue(0);
	m_progress->setVisible(false);
	m_log = new QPlainTextEdit(this);
	m_log->setReadOnly(true);
	m_log->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	m_log->setPlaceholderText(tr("The output of the import appears here."));

	auto *buttons = new QDialogButtonBox(this);
	m_start = buttons->addButton(tr("Import"), QDialogButtonBox::AcceptRole);
	m_close = buttons->addButton(QDialogButtonBox::Close);
	m_start->setDefault(true);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(intro);
	layout->addLayout(form);
	layout->addWidget(m_status);
	layout->addWidget(m_progress);
	layout->addWidget(m_log, 1);
	layout->addWidget(buttons);

	connect(archive_button, &QPushButton::clicked, this, &EplanProjectImportDialog::chooseArchive);
	connect(target_button, &QPushButton::clicked, this, &EplanProjectImportDialog::chooseTarget);
	connect(script_button, &QPushButton::clicked, this, &EplanProjectImportDialog::chooseScript);
	connect(python_button, &QPushButton::clicked, this, &EplanProjectImportDialog::choosePython);
	connect(m_archive, &QLineEdit::textChanged, this, [this]() {
		if (!m_target_edited)
			m_target->setText(QDir::toNativeSeparators(
				EplanProjectImport::defaultTarget(m_archive->text().trimmed())));
		updateButtons();
	});
	connect(m_target, &QLineEdit::textEdited, this, [this]() { m_target_edited = true; });
	connect(m_target, &QLineEdit::textChanged, this, &EplanProjectImportDialog::updateButtons);
	connect(m_script, &QLineEdit::textChanged, this, &EplanProjectImportDialog::updateButtons);
		// the buttons do not go through the box's accepted/rejected:
		// "Import" must not close the dialog
	connect(m_start, &QPushButton::clicked, this, &EplanProjectImportDialog::start);
	connect(m_close, &QPushButton::clicked, this, &EplanProjectImportDialog::reject);
	updateButtons();
}

EplanProjectImportDialog::~EplanProjectImportDialog()
{
	if (m_process && m_process->state() != QProcess::NotRunning) {
		m_process->disconnect(this);
		m_process->kill();
		m_process->waitForFinished(3000);
	}
}

bool EplanProjectImportDialog::chooseArchive()
{
	QString dir = QSettings().value(DIR_KEY).toString();
	if (!m_archive->text().trimmed().isEmpty())
		dir = QFileInfo(m_archive->text().trimmed()).absolutePath();
	const QString path = QFileDialog::getOpenFileName(this, tr("Import an EPLAN project"), dir,
		tr("EPLAN export archives (*.epdz);;All files (*)"));
	if (path.isEmpty()) return false;
	setArchive(path);
	return true;
}

void EplanProjectImportDialog::setArchive(const QString &path)
{
	QSettings().setValue(DIR_KEY, QFileInfo(path).absolutePath());
	m_archive->setText(QDir::toNativeSeparators(path));
}

void EplanProjectImportDialog::chooseTarget()
{
	QString start = m_target->text().trimmed();
	if (start.isEmpty()) start = EplanProjectImport::defaultTarget(m_archive->text().trimmed());
	const QString path = QFileDialog::getSaveFileName(this, tr("Save the imported project as"), start,
		tr("QElectroTech projects (*.qet)"));
	if (path.isEmpty()) return;
	m_target_edited = true;
	m_target->setText(QDir::toNativeSeparators(path.endsWith(QStringLiteral(".qet"), Qt::CaseInsensitive)
											   ? path : path + QStringLiteral(".qet")));
}

void EplanProjectImportDialog::chooseScript()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Where is the import program?"),
		m_script->text().trimmed(), tr("Import program (%1)").arg(EplanProjectImport::scriptName()));
	if (path.isEmpty()) return;
	QSettings().setValue(SCRIPT_KEY, path);
	m_script->setText(QDir::toNativeSeparators(path));
}

void EplanProjectImportDialog::choosePython()
{
#ifdef Q_OS_WIN
	const QString filter = tr("Programs (*.exe)");
#else
	const QString filter;
#endif
	const QString path = QFileDialog::getOpenFileName(this, tr("Which Python should run the import?"),
		m_python->text().trimmed(), filter);
	if (path.isEmpty()) return;
	m_python->setText(QDir::toNativeSeparators(path));
}

void EplanProjectImportDialog::updateButtons()
{
	const bool running = m_process && m_process->state() != QProcess::NotRunning;
	const QString archive = m_archive->text().trimmed();
	QString missing;
	if (archive.isEmpty())
		missing = tr("Choose the EPLAN export archive (.epdz).");
	else if (!QFileInfo(archive).isFile())
		missing = tr("The file %1 does not exist.").arg(archive);
	else if (!isEpdz(archive))
		missing = tr("%1 is no EPLAN export archive (.epdz).").arg(QFileInfo(archive).fileName());
	else if (m_target->text().trimmed().isEmpty())
		missing = tr("Choose where the QElectroTech project is to be saved.");
	if (!running) m_status->setText(missing);
	m_start->setEnabled(!running && missing.isEmpty());
	m_close->setText(running ? tr("Cancel") : tr("Close"));
	const QList<QWidget *> inputs{m_archive, m_target, m_script, m_python, m_pdf, m_keep_parts};
	for (QWidget *w : inputs) w->setEnabled(!running);
}

/**
	Whether @p python runs the program: "--check" makes it import
	everything it needs and find the MCP module it drives QElectroTech
	with; the reason it gives otherwise goes to @p error.
*/
bool EplanProjectImportDialog::pythonRuns(const QString &python, const QString &script, QString *error)
{
	QProcess p;
	p.setProcessEnvironment(importEnvironment());
	p.setWorkingDirectory(QFileInfo(script).absolutePath());
	p.setProcessChannelMode(QProcess::MergedChannels);
	p.start(python, {QDir::toNativeSeparators(script), QStringLiteral("--check")});
	if (!p.waitForStarted(10000)) {
		if (error) *error = tr("%1 does not start.").arg(python);
		return false;
	}
	if (!p.waitForFinished(60000)) {
		p.kill();
		p.waitForFinished(3000);
		if (error) *error = tr("%1 did not answer.").arg(python);
		return false;
	}
	if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) return true;
	if (error) {
		const QStringList lines = QString::fromUtf8(p.readAll()).trimmed().split(QLatin1Char('\n'));
		*error = QStringLiteral("%1: %2").arg(python, lines.isEmpty() ? QString() : lines.last().trimmed());
	}
	return false;
}

/**
	The Python in the field when it runs the program; otherwise the first
	that does among the ones the "py" launcher knows (Windows) and the one
	on PATH. Empty when none does; the reasons are in the log then.
*/
QString EplanProjectImportDialog::usablePython(const QString &script)
{
	QStringList candidates;
	const QString chosen = m_python->text().trimmed();
	if (!chosen.isEmpty()) candidates << chosen;
#ifdef Q_OS_WIN
	const QString py = QStandardPaths::findExecutable(QStringLiteral("py"));
	if (!py.isEmpty()) {
		QProcess p;
		p.start(py, {QStringLiteral("-0p")});
		if (p.waitForFinished(10000)) {
			static const QRegularExpression path(QStringLiteral("([A-Za-z]:\\\\.*python\\.exe)\\s*$"),
												 QRegularExpression::CaseInsensitiveOption);
			const QStringList lines = QString::fromLocal8Bit(p.readAllStandardOutput()).split(QLatin1Char('\n'));
			for (const QString &line : lines) {
				const auto m = path.match(line.trimmed());
				if (m.hasMatch()) candidates << m.captured(1);
			}
		}
	}
	const QStringList names{QStringLiteral("python"), QStringLiteral("python3")};
#else
	const QStringList names{QStringLiteral("python3"), QStringLiteral("python")};
#endif
	for (const QString &name : names) {
		const QString found = QStandardPaths::findExecutable(name);
			// Windows' WindowsApps shortcut only opens the Store
		if (!found.isEmpty() && !found.contains(QStringLiteral("WindowsApps"), Qt::CaseInsensitive))
			candidates << found;
	}
	candidates.removeDuplicates();

	QStringList reasons;
	for (const QString &c : std::as_const(candidates)) {
		m_status->setText(tr("Checking %1 …").arg(QDir::toNativeSeparators(c)));
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		QString why;
		if (pythonRuns(c, script, &why)) return c;
		reasons << why;
	}
	if (candidates.isEmpty())
		reasons << tr("No Python 3 was found.");
	appendLog(tr("No Python runs the import program:") + QLatin1Char('\n')
			  + reasons.join(QLatin1Char('\n')));
	return QString();
}

void EplanProjectImportDialog::start()
{
	const QString archive = m_archive->text().trimmed();
	const QString target = QDir::cleanPath(QDir::fromNativeSeparators(m_target->text().trimmed()));
	const QString script = m_script->text().trimmed();

	if (QETApp::diagramEditorForFile(target)) {
		QMessageBox::warning(this, windowTitle(),
			tr("%1 is open in QElectroTech. Close it first or choose another file.")
				.arg(QDir::toNativeSeparators(target)));
		return;
	}
	if (QFileInfo::exists(target)
		&& QMessageBox::question(this, windowTitle(),
			   tr("%1 already exists. Replace it?").arg(QDir::toNativeSeparators(target)))
			   != QMessageBox::Yes)
		return;
	if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
		QMessageBox::warning(this, windowTitle(),
			tr("The folder %1 cannot be created.")
				.arg(QDir::toNativeSeparators(QFileInfo(target).absolutePath())));
		return;
	}

	m_log->clear();
	m_output.clear();
	m_pending.clear();
	m_result.clear();

	QString python;
#ifdef QET_HAS_SCRIPTING
	if (QFileInfo(script).isFile()) {
		QApplication::setOverrideCursor(Qt::BusyCursor);
		python = usablePython(script);
		QApplication::restoreOverrideCursor();
	} else {
		appendLog(tr("The import program (%1) was not found.").arg(EplanProjectImport::scriptName()));
	}
#else
	Q_UNUSED(script)
	appendLog(tr("This QElectroTech is built without scripting, which the import program needs."));
#endif
	if (python.isEmpty()) {
			// the program is there but no Python runs it: the reasons are
			// in the log, which closing the dialog would take away
		if (QFileInfo(script).isFile()
			&& QMessageBox::question(this, windowTitle(),
				   tr("No Python 3 runs the import program; the reasons are in the output. "
					  "Import the pages as pictures instead?")) != QMessageBox::Yes) {
			m_status->setText(tr("No usable Python found."));
			return;
		}
		importAsPictures(archive, target);
		return;
	}
	m_python->setText(QDir::toNativeSeparators(python));

		// unpacked here, with the 7-Zip reader QElectroTech brings along,
		// so that the program needs no 7-Zip of its own
	m_status->setText(tr("Unpacking %1 …").arg(QFileInfo(archive).fileName()));
	QApplication::setOverrideCursor(Qt::BusyCursor);
	m_unpacked = std::make_unique<QTemporaryDir>();
	QString error;
	const bool unpacked = m_unpacked->isValid() && sevenZipExtract(archive, m_unpacked->path(), error);
	QApplication::restoreOverrideCursor();
	if (!unpacked) {
		m_unpacked.reset();
		appendLog(tr("%1 cannot be unpacked: %2").arg(QFileInfo(archive).fileName(), error));
		m_status->setText(tr("The import failed."));
		return;
	}

	QSettings settings;
	settings.setValue(PYTHON_KEY, python);
	settings.setValue(PDF_KEY, m_pdf->isChecked());
	settings.setValue(KEEP_KEY, m_keep_parts->isChecked());

	m_progress->setValue(0);
	m_progress->setVisible(true);
	m_process = new QProcess(this);
	m_process->setProcessEnvironment(importEnvironment());
	m_process->setWorkingDirectory(QFileInfo(script).absolutePath());
	m_process->setProcessChannelMode(QProcess::MergedChannels);
	connect(m_process, &QProcess::readyRead, this, &EplanProjectImportDialog::readOutput);
	connect(m_process, &QProcess::finished, this, &EplanProjectImportDialog::processFinished);
	connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
		if (e != QProcess::FailedToStart || !m_process) return;
		appendLog(tr("The import does not start: %1").arg(m_process->errorString()));
		m_process->deleteLater();
		m_process = nullptr;
		m_unpacked.reset();
		updateButtons();
		m_status->setText(tr("The import did not start."));
	});
	const QStringList args = EplanProjectImport::arguments(script, archive, target, m_unpacked->path(),
														   m_pdf->isChecked(), m_keep_parts->isChecked());
	appendLog(QStringLiteral("> %1 %2").arg(QDir::toNativeSeparators(python), args.join(QLatin1Char(' '))));
	m_process->start(python, args);
	updateButtons();
	m_status->setText(tr("Importing %1 … this can take several minutes.").arg(QFileInfo(archive).fileName()));
}

/**
	The pages as pictures, read here: what is left without the program, a
	Python that runs it or scripting.
*/
void EplanProjectImportDialog::importAsPictures(const QString &archive, const QString &target)
{
	appendLog(tr("Without the import program and a Python 3 that runs it, the pages of %1 come in as "
				 "pictures; parts and conductors need them.").arg(QFileInfo(archive).fileName()));
	m_status->setText(tr("Importing %1 …").arg(QFileInfo(archive).fileName()));
	QApplication::setOverrideCursor(Qt::BusyCursor);
	const EpdzImport::Result result = EpdzImport::run(archive, target);
	QApplication::restoreOverrideCursor();
	for (const QString &n : result.notes) appendLog(n);
	if (!result.ok) {
		appendLog(result.error);
		m_status->setText(tr("The import failed: %1").arg(result.error));
		updateButtons();
		return;
	}
	m_result = target;
	accept();
}

void EplanProjectImportDialog::readOutput()
{
	if (!m_process) return;
	m_pending += QString::fromUtf8(m_process->readAll());
		// whole lines only: progress lines move the bar, all others go to the log
	const int end = m_pending.lastIndexOf(QLatin1Char('\n'));
	if (end < 0) return;
	const QStringList lines = m_pending.left(end).split(QLatin1Char('\n'));
	m_pending.remove(0, end + 1);
	for (QString line : lines) {
		line.remove(QLatin1Char('\r'));
		int percent = 0;
		const QString step = EplanProjectImport::progressText(line, &percent);
		if (!step.isEmpty()) {
			m_progress->setValue(percent);
			m_status->setText(step);
			continue;
		}
		m_output += line + QLatin1Char('\n');
		m_log->appendPlainText(line);
	}
}

void EplanProjectImportDialog::processFinished(int exit_code, QProcess::ExitStatus status)
{
	readOutput();
	if (!m_pending.isEmpty()) {		// a last line without line break
		const QString rest = m_pending;
		m_pending.clear();
		for (const QString &line : rest.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
			int percent = 0;
			if (EplanProjectImport::progressText(line.trimmed(), &percent).isEmpty()) {
				m_output += line + QLatin1Char('\n');
				m_log->appendPlainText(line);
			} else {
				m_progress->setValue(percent);
			}
		}
	}
	m_process->deleteLater();
	m_process = nullptr;
	m_unpacked.reset();
	const QString result = EplanProjectImport::resultFromOutput(m_output);
	updateButtons();
	if (status == QProcess::NormalExit && exit_code == 0 && QFileInfo(result).isFile()) {
		m_progress->setValue(100);
		m_result = QDir::cleanPath(QDir::fromNativeSeparators(result));
		accept();
		return;
	}
	m_status->setText(status == QProcess::CrashExit
					  ? tr("The import was stopped.")
					  : tr("The import failed (exit code %1). The reason is in the output below.").arg(exit_code));
}

void EplanProjectImportDialog::appendLog(const QString &text)
{
	m_log->appendPlainText(text);
}

void EplanProjectImportDialog::reject()
{
	if (m_process && m_process->state() != QProcess::NotRunning) {
		if (QMessageBox::question(this, windowTitle(), tr("Stop the running import?"))
			!= QMessageBox::Yes)
			return;
		m_process->kill();	// processFinished() tidies up
		return;
	}
	QDialog::reject();
}

void EplanProjectImportDialog::dragEnterEvent(QDragEnterEvent *event)
{
	const auto urls = event->mimeData()->urls();
	if (!m_process && urls.size() == 1 && urls.first().isLocalFile()
		&& isEpdz(urls.first().toLocalFile()))
		event->acceptProposedAction();
}

void EplanProjectImportDialog::dropEvent(QDropEvent *event)
{
	setArchive(event->mimeData()->urls().first().toLocalFile());
	event->acceptProposedAction();
}
