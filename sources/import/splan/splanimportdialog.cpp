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
#include "splanimportdialog.h"

#include "../../qetapp.h"

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
#include <QVBoxLayout>

namespace {
	const char DIR_KEY[]     = "splanimport/dir";
	const char PYTHON_KEY[]  = "splanimport/python";
	const char SCRIPT_KEY[]  = "splanimport/script";
	const char PDF_KEY[]     = "splanimport/pdf";
	const char SCRIPT_NAME[] = "splan_to_qet.py";
	const char FOLDER[]      = "splan_import";

	/**
		What the program needs besides its arguments: this QElectroTech
		for its own runs, the element collections of this QElectroTech,
		UTF-8 output.
	*/
	QProcessEnvironment importEnvironment()
	{
		QProcessEnvironment env =
				QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QET_BINARY"),
			   QDir::toNativeSeparators(
				   QCoreApplication::applicationFilePath()));
		const QList<QPair<QString, QString>> collections{
			{QStringLiteral("QET_COMMON_ELEMENTS_DIR"),
			 QETApp::commonElementsDir()},
			{QStringLiteral("QET_CUSTOM_ELEMENTS_DIR"),
			 QETApp::customElementsDir()},
			{QStringLiteral("QET_COMPANY_ELEMENTS_DIR"),
			 QETApp::companyElementsDir()}};
		for (const auto &c : collections) {
			if (!c.second.isEmpty())
				env.insert(c.first,
					   QDir::toNativeSeparators(c.second));
		}
		env.insert(QStringLiteral("PYTHONIOENCODING"),
			   QStringLiteral("utf-8"));
		env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
		env.insert(QStringLiteral("PYTHONUNBUFFERED"),
			   QStringLiteral("1"));
		return env;
	}

	QLineEdit *pathRow(QWidget *parent, QFormLayout *form,
			   const QString &label, QPushButton **button)
	{
		auto *row = new QHBoxLayout;
		auto *edit = new QLineEdit(parent);
		*button = new QPushButton(
				QCoreApplication::translate("SplanImportDialog",
							    "Change…"),
				parent);
		row->addWidget(edit, 1);
		row->addWidget(*button);
		form->addRow(label, row);
		return edit;
	}
}

SplanImportDialog::SplanImportDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Import an sPlan schematic"));
	setAcceptDrops(true);
	resize(760, 560);

	auto *intro = new QLabel(
			tr("Reads an sPlan 8 schematic (.spl8) and draws it "
			   "anew as a QElectroTech project: the same devices "
			   "and connections on the grid, with cross references "
			   "between coils and contacts. What the schematic "
			   "leaves open is listed in a file "
			   "<name>_open_questions.txt next to the project."),
			this);
	intro->setTextFormat(Qt::PlainText);	// "<name>" is no tag
	intro->setWordWrap(true);

	auto *form = new QFormLayout;
	QPushButton *source_button = nullptr, *target_button = nullptr,
		*script_button = nullptr, *python_button = nullptr;
	m_source = pathRow(this, form, tr("sPlan schematic (.spl8):"),
			   &source_button);
	m_target = pathRow(this, form, tr("Save as:"), &target_button);

	QSettings settings;
	m_pdf = new QCheckBox(tr("Also write a PDF and a comparison with "
				 "the original"), this);
	m_pdf->setChecked(settings.value(PDF_KEY, false).toBool());
	form->addRow(QString(), m_pdf);

	m_script = pathRow(this, form, tr("Import program:"), &script_button);
	m_script->setText(QDir::toNativeSeparators(
				  findScript(QCoreApplication::applicationDirPath())));
	m_python = pathRow(this, form, tr("Python:"), &python_button);
	m_python->setText(settings.value(PYTHON_KEY).toString());
	m_python->setPlaceholderText(tr("found when the import starts"));

	m_status = new QLabel(this);
	m_status->setWordWrap(true);
	m_progress = new QProgressBar(this);
	m_progress->setRange(0, 100);
	m_progress->setValue(0);
	m_progress->setTextVisible(true);
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

	connect(source_button, &QPushButton::clicked,
		this, &SplanImportDialog::chooseSource);
	connect(target_button, &QPushButton::clicked,
		this, &SplanImportDialog::chooseTarget);
	connect(script_button, &QPushButton::clicked,
		this, &SplanImportDialog::chooseScript);
	connect(python_button, &QPushButton::clicked,
		this, &SplanImportDialog::choosePython);
	connect(m_source, &QLineEdit::textChanged, this, [this]() {
		if (!m_target_edited)
			m_target->setText(QDir::toNativeSeparators(
				defaultTarget(m_source->text().trimmed())));
		updateButtons();
	});
	connect(m_target, &QLineEdit::textEdited,
		this, [this]() { m_target_edited = true; });
	connect(m_target, &QLineEdit::textChanged,
		this, &SplanImportDialog::updateButtons);
	connect(m_script, &QLineEdit::textChanged,
		this, &SplanImportDialog::updateButtons);
		// "Import" must not close the dialog
	connect(m_start, &QPushButton::clicked,
		this, &SplanImportDialog::start);
	connect(m_close, &QPushButton::clicked,
		this, &SplanImportDialog::reject);
	updateButtons();
}

SplanImportDialog::~SplanImportDialog()
{
	if (m_process && m_process->state() != QProcess::NotRunning) {
		m_process->disconnect(this);
		m_process->kill();
		m_process->waitForFinished(3000);
	}
}

/**
	@brief SplanImportDialog::findScript
	The program in this order: the environment variable QET_SPLAN_IMPORT,
	the path chosen earlier, an installed QElectroTech
	(<data>/import/splan_import/, see CMakeLists.txt), and the source tree
	a build folder lies in (tools/splan_import/).
*/
QString SplanImportDialog::findScript(const QString &application_dir)
{
	QStringList candidates;
	const QString env = qEnvironmentVariable("QET_SPLAN_IMPORT").trimmed();
	if (!env.isEmpty()) candidates << env;
	const QString chosen = QSettings().value(SCRIPT_KEY).toString();
	if (!chosen.isEmpty()) candidates << chosen;
	const QDir app(application_dir);
	const QString tail = QString::fromLatin1(FOLDER)
			+ QLatin1Char('/') + QString::fromLatin1(SCRIPT_NAME);
	const QStringList folders{
		QStringLiteral("../share/qelectrotech/import/"),
		QStringLiteral("../import/"),
		QStringLiteral("import/"),
		QStringLiteral("../tools/"),
		QStringLiteral("../../tools/")};
	for (const QString &folder : folders)
		candidates << app.filePath(folder + tail);
	for (const QString &c : candidates) {
		const QFileInfo fi(c);
		if (fi.isFile()) return QDir::cleanPath(fi.absoluteFilePath());
	}
	return QString();
}

QString SplanImportDialog::defaultTarget(const QString &source)
{
	if (source.isEmpty()) return QString();
	const QFileInfo fi(source);
	QString name = fi.completeBaseName();
	static const QRegularExpression unsafe(
				QStringLiteral("[^\\w.+-]+"),
				QRegularExpression::UseUnicodePropertiesOption);
	name.replace(unsafe, QStringLiteral("_"));
	return QDir(fi.absolutePath()).filePath(
				QStringLiteral("QET/") + name + QStringLiteral(".qet"));
}

QStringList SplanImportDialog::arguments(const QString &script,
					 const QString &source,
					 const QString &target,
					 bool pdf)
{
	QStringList args{QStringLiteral("-u"), QDir::toNativeSeparators(script),
			 QDir::toNativeSeparators(source),
			 QDir::toNativeSeparators(target)};
	if (pdf) {
		args << QStringLiteral("--pdf");
			// a PDF printed from sPlan next to the schematic as
			// <name>.pdf: compare with it
		const QFileInfo fi(source);
		const QString original = QDir(fi.absolutePath()).filePath(
					fi.completeBaseName() + QStringLiteral(".pdf"));
		if (QFileInfo(original).isFile())
			args << QStringLiteral("--compare")
			     << QDir::toNativeSeparators(original);
	}
	return args;
}

QString SplanImportDialog::resultFromOutput(const QString &output)
{
	static const QRegularExpression done(
				QStringLiteral("^done: (.+?)\\s*$"),
				QRegularExpression::MultilineOption);
	QString result;
	auto it = done.globalMatch(output);
	while (it.hasNext()) result = it.next().captured(1);
	return result;
}

QString SplanImportDialog::progressText(const QString &line, int *percent)
{
	static const QRegularExpression progress(
				QStringLiteral("^PROGRESS (\\d+) (\\w+)\\s*$"));
	const auto m = progress.match(line);
	if (!m.hasMatch()) return QString();
	if (percent) *percent = qBound(0, m.captured(1).toInt(), 100);
	const QString step = m.captured(2);
	if (step == QLatin1String("read"))
		return tr("Reading the sPlan schematic");
	if (step == QLatin1String("sheet"))
		return tr("Recognising devices and connections, laying out "
			  "the sheets");
	if (step == QLatin1String("write"))
		return tr("Writing the QElectroTech project");
	if (step == QLatin1String("check"))
		return tr("Checking and tidying the layout");
	if (step == QLatin1String("pdf"))
		return tr("Writing the PDF");
	if (step == QLatin1String("done"))
		return tr("Done");
	return step;
}

bool SplanImportDialog::chooseSource()
{
	QString dir = QSettings().value(DIR_KEY).toString();
	if (!m_source->text().trimmed().isEmpty())
		dir = QFileInfo(m_source->text().trimmed()).absolutePath();
	const QString path = QFileDialog::getOpenFileName(
				this, tr("Import an sPlan schematic"), dir,
				tr("sPlan 8 schematics (*.spl8);;All files (*)"));
	if (path.isEmpty()) return false;
	setSource(path);
	return true;
}

void SplanImportDialog::setSource(const QString &path)
{
	QSettings().setValue(DIR_KEY, QFileInfo(path).absolutePath());
	m_source->setText(QDir::toNativeSeparators(path));
}

void SplanImportDialog::chooseTarget()
{
	QString start = m_target->text().trimmed();
	if (start.isEmpty()) start = defaultTarget(m_source->text().trimmed());
	const QString path = QFileDialog::getSaveFileName(
				this, tr("Save the imported project as"), start,
				tr("QElectroTech projects (*.qet)"));
	if (path.isEmpty()) return;
	m_target_edited = true;
	m_target->setText(QDir::toNativeSeparators(
				  path.endsWith(QStringLiteral(".qet"),
						Qt::CaseInsensitive)
				  ? path : path + QStringLiteral(".qet")));
}

void SplanImportDialog::chooseScript()
{
	const QString path = QFileDialog::getOpenFileName(
				this, tr("Where is the import program?"),
				m_script->text().trimmed(),
				tr("Import program (%1)")
				.arg(QString::fromLatin1(SCRIPT_NAME)));
	if (path.isEmpty()) return;
	QSettings().setValue(SCRIPT_KEY, path);
	m_script->setText(QDir::toNativeSeparators(path));
}

void SplanImportDialog::choosePython()
{
#ifdef Q_OS_WIN
	const QString filter = tr("Programs (*.exe)");
#else
	const QString filter;
#endif
	const QString path = QFileDialog::getOpenFileName(
				this, tr("Which Python should run the import?"),
				m_python->text().trimmed(), filter);
	if (path.isEmpty()) return;
	m_python->setText(QDir::toNativeSeparators(path));
}

void SplanImportDialog::updateButtons()
{
	const bool running = m_process
			&& m_process->state() != QProcess::NotRunning;
	const QString source = m_source->text().trimmed();
	QString missing;
	if (source.isEmpty())
		missing = tr("Choose the sPlan schematic (.spl8).");
	else if (!QFileInfo(source).isFile())
		missing = tr("The sPlan schematic %1 does not exist.").arg(source);
	else if (m_target->text().trimmed().isEmpty())
		missing = tr("Choose where the QElectroTech project is to be "
			     "saved.");
	else if (!QFileInfo(m_script->text().trimmed()).isFile())
		missing = tr("The import program (%1) was not found. Choose it "
			     "with \"Change…\".")
			  .arg(QString::fromLatin1(SCRIPT_NAME));
	if (!running && !missing.isEmpty()) m_status->setText(missing);
	m_start->setEnabled(!running && missing.isEmpty());
	m_close->setText(running ? tr("Cancel") : tr("Close"));
	const QList<QWidget *> inputs{m_source, m_target, m_script, m_python,
				      m_pdf};
	for (QWidget *w : inputs) w->setEnabled(!running);
}

/**
	@brief SplanImportDialog::pythonRuns
	Whether @p python imports the program. The optional packages (Pillow
	for pictures, pymupdf for the comparison PDF) are not required: the
	program says in its output what it left out without them.
*/
bool SplanImportDialog::pythonRuns(const QString &python,
				   const QString &script,
				   QString *error)
{
	const QString dir = QFileInfo(script).absolutePath();
	const QString module = QFileInfo(script).completeBaseName();
	const QString code = QStringLiteral(
				"import sys; assert sys.version_info >= (3, 10), "
				"'Python 3.10 or newer is needed'; "
				"sys.path[:0] = [r'%1']; import %2")
			.arg(QDir::toNativeSeparators(dir), module);
	QProcess p;
	p.setProcessEnvironment(importEnvironment());
	p.setWorkingDirectory(dir);
	p.setProcessChannelMode(QProcess::MergedChannels);
	p.start(python, {QStringLiteral("-c"), code});
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
	if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0)
		return true;
	if (error) {
		const QStringList lines = QString::fromUtf8(p.readAll()).trimmed()
				.split(QLatin1Char('\n'));
		*error = QStringLiteral("%1: %2").arg(
					python,
					lines.isEmpty() ? QString()
							: lines.last().trimmed());
	}
	return false;
}

/**
	@brief SplanImportDialog::usablePython
	The Python in the field when it runs the program; otherwise the first
	that does among the ones the "py" launcher knows and the one on PATH.
*/
QString SplanImportDialog::usablePython(const QString &script)
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
			static const QRegularExpression path(
						QStringLiteral("([A-Za-z]:\\\\.*python\\.exe)\\s*$"),
						QRegularExpression::CaseInsensitiveOption);
			const QStringList lines = QString::fromLocal8Bit(
						p.readAllStandardOutput())
					.split(QLatin1Char('\n'));
			for (const QString &line : lines) {
				const auto m = path.match(line.trimmed());
				if (m.hasMatch()) candidates << m.captured(1);
			}
		}
	}
	const QStringList names{QStringLiteral("python"),
				QStringLiteral("python3")};
#else
	const QStringList names{QStringLiteral("python3"),
				QStringLiteral("python")};
#endif
	for (const QString &name : names) {
		const QString found = QStandardPaths::findExecutable(name);
			// the Microsoft Store stub only opens the store
		if (!found.isEmpty()
			&& !found.contains(QStringLiteral("WindowsApps"),
					   Qt::CaseInsensitive))
			candidates << found;
	}
	candidates.removeDuplicates();

	QStringList reasons;
	for (const QString &c : candidates) {
		m_status->setText(tr("Checking %1 …")
				  .arg(QDir::toNativeSeparators(c)));
		QCoreApplication::processEvents(
					QEventLoop::ExcludeUserInputEvents);
		QString why;
		if (pythonRuns(c, script, &why)) return c;
		reasons << why;
	}
	m_log->appendPlainText(tr("No Python runs the import program:")
			       + QLatin1Char('\n')
			       + reasons.join(QLatin1Char('\n'))
			       + QLatin1Char('\n')
			       + tr("Install Python 3.10 or newer, or choose "
				    "another Python."));
	return QString();
}

void SplanImportDialog::start()
{
	const QString source = m_source->text().trimmed();
	const QString target = QDir::cleanPath(
				QDir::fromNativeSeparators(m_target->text().trimmed()));
	const QString script = m_script->text().trimmed();

	if (QETApp::diagramEditorForFile(target)) {
		QMessageBox::warning(
					this, windowTitle(),
					tr("%1 is open in QElectroTech. Close it first "
					   "or choose another file.")
					.arg(QDir::toNativeSeparators(target)));
		return;
	}
	if (QFileInfo::exists(target)
		&& QMessageBox::question(
			this, windowTitle(),
			tr("%1 already exists. Replace it?")
			.arg(QDir::toNativeSeparators(target)))
		!= QMessageBox::Yes)
		return;
	if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
		QMessageBox::warning(
					this, windowTitle(),
					tr("The folder %1 cannot be created.")
					.arg(QDir::toNativeSeparators(
						     QFileInfo(target).absolutePath())));
		return;
	}

	m_log->clear();
	m_output.clear();
	m_pending.clear();
	m_result.clear();
	m_progress->setValue(0);
	m_progress->setVisible(true);
	m_start->setEnabled(false);
	QApplication::setOverrideCursor(Qt::BusyCursor);
	const QString python = usablePython(script);
	QApplication::restoreOverrideCursor();
	if (python.isEmpty()) {
		updateButtons();
		m_status->setText(tr("No usable Python found."));
		return;
	}
	m_python->setText(QDir::toNativeSeparators(python));

	QSettings settings;
	settings.setValue(PYTHON_KEY, python);
	settings.setValue(PDF_KEY, m_pdf->isChecked());

	m_process = new QProcess(this);
	m_process->setProcessEnvironment(importEnvironment());
	m_process->setWorkingDirectory(QFileInfo(script).absolutePath());
	m_process->setProcessChannelMode(QProcess::MergedChannels);
	connect(m_process, &QProcess::readyRead,
		this, &SplanImportDialog::readOutput);
	connect(m_process, &QProcess::finished,
		this, &SplanImportDialog::processFinished);
	connect(m_process, &QProcess::errorOccurred,
		this, [this](QProcess::ProcessError e) {
		if (e != QProcess::FailedToStart || !m_process) return;
		m_log->appendPlainText(tr("The import does not start: %1")
				       .arg(m_process->errorString()));
		m_process->deleteLater();
		m_process = nullptr;
		updateButtons();
		m_status->setText(tr("The import did not start."));
	});
	const QStringList args = arguments(script, source, target,
					   m_pdf->isChecked());
	m_log->appendPlainText(QStringLiteral("> %1 %2").arg(
				       QDir::toNativeSeparators(python),
				       args.join(QLatin1Char(' '))));
	m_process->start(python, args);
	updateButtons();
	m_status->setText(tr("Importing %1 … this can take a few minutes.")
			  .arg(QFileInfo(source).fileName()));
}

void SplanImportDialog::readOutput()
{
	if (!m_process) return;
	m_pending += QString::fromUtf8(m_process->readAll());
	const int end = m_pending.lastIndexOf(QLatin1Char('\n'));
	if (end < 0) return;
	const QStringList lines = m_pending.left(end).split(QLatin1Char('\n'));
	m_pending.remove(0, end + 1);
	for (QString line : lines) {
		line.remove(QLatin1Char('\r'));
		int percent = 0;
		const QString step = progressText(line, &percent);
		if (!step.isEmpty()) {
			m_progress->setValue(percent);
			m_status->setText(step);
			continue;
		}
		m_output += line + QLatin1Char('\n');
		m_log->appendPlainText(line);
	}
}

void SplanImportDialog::processFinished(int exit_code,
					QProcess::ExitStatus status)
{
	readOutput();
	if (!m_pending.isEmpty()) {
		const QString rest = m_pending;
		m_pending.clear();
		const QStringList lines = rest.split(QLatin1Char('\n'),
						     Qt::SkipEmptyParts);
		for (const QString &line : lines) {
			int percent = 0;
			if (progressText(line.trimmed(), &percent).isEmpty()) {
				m_output += line + QLatin1Char('\n');
				m_log->appendPlainText(line);
			} else {
				m_progress->setValue(percent);
			}
		}
	}
	m_process->deleteLater();
	m_process = nullptr;
	const QString result = resultFromOutput(m_output);
	updateButtons();
	if (status == QProcess::NormalExit && exit_code == 0
		&& QFileInfo(result).isFile()) {
		m_progress->setValue(100);
		m_result = QDir::cleanPath(QDir::fromNativeSeparators(result));
		m_status->setText(tr("Done: %1")
				  .arg(QDir::toNativeSeparators(m_result)));
		accept();
		return;
	}
	m_status->setText(status == QProcess::CrashExit
			  ? tr("The import was stopped.")
			  : tr("The import failed (exit code %1). The reason is "
			       "in the output below.").arg(exit_code));
}

void SplanImportDialog::reject()
{
	if (m_process && m_process->state() != QProcess::NotRunning) {
		if (QMessageBox::question(this, windowTitle(),
					  tr("Stop the running import?"))
			!= QMessageBox::Yes)
			return;
		m_process->kill();
		return;
	}
	QDialog::reject();
}

void SplanImportDialog::dragEnterEvent(QDragEnterEvent *event)
{
	const auto urls = event->mimeData()->urls();
	if (!m_process && urls.size() == 1 && urls.first().isLocalFile()
		&& urls.first().toLocalFile().endsWith(QStringLiteral(".spl8"),
						       Qt::CaseInsensitive))
		event->acceptProposedAction();
}

void SplanImportDialog::dropEvent(QDropEvent *event)
{
	setSource(event->mimeData()->urls().first().toLocalFile());
	event->acceptProposedAction();
}
