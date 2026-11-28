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
#include "fluidsimimport.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

namespace {
	const char DIR_KEY[] = "fluidsimimport/dir";
	const char SCRIPT_NAME[] = "fluidsim_import.py";
}

/**
	@brief FluidSimImport::pythons
	The "py" launcher on Windows, then python3 and python on the PATH. The
	Windows Store shortcut (WindowsApps) is left out: it opens the Store
	instead of running a program.
*/
QStringList FluidSimImport::pythons()
{
	QStringList out;
#ifdef Q_OS_WIN
	const QStringList names{QStringLiteral("py"), QStringLiteral("python3"), QStringLiteral("python")};
#else
	const QStringList names{QStringLiteral("python3"), QStringLiteral("python")};
#endif
	for (const QString &n : names) {
		const QString found = QStandardPaths::findExecutable(n);
		if (!found.isEmpty() && !found.contains(QStringLiteral("WindowsApps"), Qt::CaseInsensitive))
			out << found;
	}
	out.removeDuplicates();
	return out;
}

QString FluidSimImport::findScript(const QString &application_dir)
{
	QStringList candidates;
	const QString env = qEnvironmentVariable("QET_FLUIDSIM_IMPORT").trimmed();
	if (!env.isEmpty()) candidates << env;
	const QDir app(application_dir);
	const QString tail = QStringLiteral("fluidsim_import/") + QString::fromLatin1(SCRIPT_NAME);
	candidates << app.filePath(tail)
		   << app.filePath(QStringLiteral("../") + tail)
		   << app.filePath(QStringLiteral("../Resources/") + tail)	// macOS bundle
		   << app.filePath(QStringLiteral("../share/qelectrotech/") + tail)
		   << app.filePath(QStringLiteral("../../tools/") + tail)
		   << app.filePath(QStringLiteral("../tools/") + tail);
	for (const QString &c : candidates) {
		const QFileInfo fi(c);
		if (fi.isFile()) return QDir::cleanPath(fi.absoluteFilePath());
	}
	return QString();
}

QStringList FluidSimImport::arguments(const QString &script, const QStringList &circuits,
				      const QString &target, const QString &qet_binary)
{
	QStringList args{QStringLiteral("-u"), QDir::toNativeSeparators(script)};
	for (const QString &c : circuits) args << QDir::toNativeSeparators(c);
	args << QDir::toNativeSeparators(target)
	     << QStringLiteral("--qet") << QDir::toNativeSeparators(qet_binary)
	     << QStringLiteral("--stop-on-eof");
	return args;
}

QString FluidSimImport::resultFromOutput(const QString &output)
{
	static const QRegularExpression done(QStringLiteral("^done: (.+?)\\s*$"),
					     QRegularExpression::MultilineOption);
	QString result;
	auto it = done.globalMatch(output);
	while (it.hasNext()) result = it.next().captured(1);
	return result;
}

QString FluidSimImport::run(QWidget *parent)
{
	const QString script = findScript(QCoreApplication::applicationDirPath());
	if (script.isEmpty()) {
		QMessageBox::warning(parent, QObject::tr("Import FluidSIM circuit"),
			QObject::tr("The import program (%1) was not found next to QElectroTech. "
				    "Set QET_FLUIDSIM_IMPORT to its path.").arg(QString::fromLatin1(SCRIPT_NAME)));
		return QString();
	}
	QSettings settings;
	const QStringList circuits = QFileDialog::getOpenFileNames(parent, QObject::tr("Import FluidSIM circuit"),
		settings.value(DIR_KEY).toString(), QObject::tr("FluidSIM circuits (*.circ);;All files (*)"));
	if (circuits.isEmpty()) return QString();
	const QFileInfo first(circuits.first());
	settings.setValue(DIR_KEY, first.absolutePath());
	QString name = circuits.size() == 1 ? first.completeBaseName() : first.absoluteDir().dirName();
	name.replace(QLatin1Char(' '), QLatin1Char('_'));
		// the dialog adds ".qet" itself, so its overwrite question sees the
		// file that is really written
	QFileDialog save(parent, QObject::tr("Save the imported project as"),
			 first.absoluteDir().filePath(name + QStringLiteral("_FluidSIM.qet")),
			 QObject::tr("QElectroTech projects (*.qet)"));
	save.setAcceptMode(QFileDialog::AcceptSave);
	save.setFileMode(QFileDialog::AnyFile);
	save.setDefaultSuffix(QStringLiteral("qet"));
	if (save.exec() != QDialog::Accepted || save.selectedFiles().isEmpty()) return QString();
	QString target = save.selectedFiles().first();
	if (!target.endsWith(QStringLiteral(".qet"), Qt::CaseInsensitive)) {
			// another suffix typed ("foo.bak"): the project is foo.bak.qet,
			// which the dialog did not ask about
		target += QStringLiteral(".qet");
		if (QFileInfo::exists(target)
		    && QMessageBox::question(parent, QObject::tr("Import FluidSIM circuit"),
					     QObject::tr("%1 already exists. Replace it?")
					     .arg(QDir::toNativeSeparators(target)))
		       != QMessageBox::Yes)
			return QString();
	}

	QProgressDialog progress(QObject::tr("Importing %n FluidSIM circuit(s) …", nullptr,
					     static_cast<int>(circuits.size())),
				 QObject::tr("Cancel"), 0, 0, parent);
	progress.setWindowTitle(QObject::tr("Import FluidSIM circuit"));
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(0);
	progress.show();

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
	env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
	QString output, tried;
	for (const QString &python : pythons()) {
		QProcess p;
		p.setProcessEnvironment(env);
		p.setProcessChannelMode(QProcess::MergedChannels);
		QStringList args = arguments(script, circuits, target, QCoreApplication::applicationFilePath());
		if (QFileInfo(python).baseName().compare(QStringLiteral("py"), Qt::CaseInsensitive) == 0)
			args.prepend(QStringLiteral("-3"));
		p.start(python, args);
		if (!p.waitForStarted(10000)) { tried += python + QLatin1Char('\n'); continue; }
		while (!p.waitForFinished(100)) {
			QApplication::processEvents();
			if (progress.wasCanceled()) {
					// --stop-on-eof: closing its input makes the program
					// stop the QElectroTech runs it started and remove its
					// temporary folders; kill it only when it does not
				p.closeWriteChannel();
				if (!p.waitForFinished(15000)) {
					p.kill();
					p.waitForFinished(3000);
				}
				return QString();
			}
		}
		output = QString::fromUtf8(p.readAll());
		const QString result = resultFromOutput(output);
		if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0 && !result.isEmpty())
			return QDir::fromNativeSeparators(result);
			// a Python 2, or one without what the program needs: try the next one
		if (output.contains(QStringLiteral("SyntaxError"))
		    || output.contains(QStringLiteral("ModuleNotFoundError"))) {
			tried += python + QLatin1Char('\n');
			output.clear();
			continue;
		}
		break;
	}
	progress.close();
	const QStringList lines = output.trimmed().split(QLatin1Char('\n'));
	QMessageBox::warning(parent, QObject::tr("Import FluidSIM circuit"),
		output.isEmpty() ? QObject::tr("No Python 3 could run the import program:\n%1").arg(tried)
				 : QObject::tr("The import did not finish:\n%1")
				   .arg(lines.mid(qMax(0, static_cast<int>(lines.size()) - 12)).join(QLatin1Char('\n'))));
	return QString();
}
