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
#include "eplanprojectimport.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

QString EplanProjectImport::scriptName()
{
	return QStringLiteral("epdz_nach_qet.py");
}

QString EplanProjectImport::findScript(const QString &application_dir, const QString &chosen)
{
	QStringList candidates;
	if (!chosen.isEmpty()) candidates << chosen;
	const QString env = qEnvironmentVariable("QET_EPLAN_IMPORT").trimmed();
	if (!env.isEmpty()) candidates << env;
	const QDir app(application_dir);
	const QString tail = QStringLiteral("eplan_import/eplan_zw1/") + scriptName();
	candidates << app.filePath(tail)
			   << app.filePath(QStringLiteral("../") + tail)						// Windows install (bin/)
			   << app.filePath(QStringLiteral("../share/qelectrotech/") + tail)	// Linux, BSD
			   << app.filePath(QStringLiteral("../Resources/") + tail)			// macOS bundle
			   << app.filePath(QStringLiteral("../tools/") + tail)				// build folder in the source tree
			   << app.filePath(QStringLiteral("../../tools/") + tail);
	for (const QString &c : std::as_const(candidates)) {
		const QFileInfo fi(c);
		if (fi.isFile()) return QDir::cleanPath(fi.absoluteFilePath());
	}
	return QString();
}

QString EplanProjectImport::defaultTarget(const QString &archive)
{
	if (archive.isEmpty()) return QString();
	const QFileInfo fi(archive);
	return QDir(fi.absolutePath()).filePath(fi.completeBaseName() + QStringLiteral(".qet"));
}

QStringList EplanProjectImport::arguments(const QString &script, const QString &archive,
										  const QString &target, const QString &unpacked,
										  bool pdf, bool keep_parts)
{
	QStringList args{QStringLiteral("-u"), QDir::toNativeSeparators(script),
					 QDir::toNativeSeparators(archive), QDir::toNativeSeparators(target)};
	if (!unpacked.isEmpty())
		args << QStringLiteral("--unpacked") << QDir::toNativeSeparators(unpacked);
	if (pdf) args << QStringLiteral("--pdf");
	if (!keep_parts) args << QStringLiteral("--no-collection");
	return args;
}

QString EplanProjectImport::resultFromOutput(const QString &output)
{
	static const QRegularExpression done(QStringLiteral("^done: (.+?)\\s*$"),
										 QRegularExpression::MultilineOption);
	QString result;
	auto it = done.globalMatch(output);
	while (it.hasNext()) result = it.next().captured(1);
	return result;
}

QString EplanProjectImport::progressText(const QString &line, int *percent)
{
	static const QRegularExpression progress(
		QStringLiteral("^PROGRESS (\\d+) (\\w+)\\s*$"));
	const auto m = progress.match(line);
	if (!m.hasMatch()) return QString();
	if (percent) *percent = qBound(0, m.captured(1).toInt(), 100);
	const QString step = m.captured(2);
	if (step == QLatin1String("read"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Unpacking and reading the EPLAN archive");
	if (step == QLatin1String("pages"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Reading the pages, symbols and connections");
	if (step == QLatin1String("interruptions"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Interruption points");
	if (step == QLatin1String("connections"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Drawing connections");
	if (step == QLatin1String("writing"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Writing the QElectroTech project");
	if (step == QLatin1String("lists"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Device list, connection list and 3D installation spaces");
	if (step == QLatin1String("pdf"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Writing the PDF");
	if (step == QLatin1String("done"))
		return QCoreApplication::translate("EplanProjectImportDialog", "Done");
	return step;
}
