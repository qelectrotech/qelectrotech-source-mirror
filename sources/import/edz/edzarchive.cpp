/*
	Copyright 2006 The QElectroTech Team
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
#include "edzarchive.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

EdzArchive::EdzArchive() = default;
EdzArchive::~EdzArchive() = default;

/**
	Extract the .edz at @a edz_path into a fresh temporary directory.
	@return true on success; extractedDir()/partXmlPath() are then valid.
*/
bool EdzArchive::extract(const QString &edz_path)
{
	m_error.clear();

	const QFileInfo fi(edz_path);
	if (!fi.exists() || !fi.isFile()) {
		m_error = QStringLiteral("File not found: %1").arg(edz_path);
		return false;
	}

	m_dir.reset(new QTemporaryDir);
	if (!m_dir->isValid()) {
		m_error = QStringLiteral("Could not create a temporary directory: %1")
				  .arg(m_dir->errorString());
		return false;
	}

	if (!extractWithSevenZipCli(fi.absoluteFilePath(), m_dir->path())) {
		return false;
	}

	if (partXmlPath().isEmpty()) {
		m_error = QStringLiteral("No *.part.xml found inside the .edz");
		return false;
	}
	return true;
}

/** @return the temporary folder holding the extracted tree (empty if none). */
QString EdzArchive::extractedDir() const
{
	return m_dir ? m_dir->path() : QString();
}

/** @return absolute path to the first *.part.xml in the extracted tree. */
QString EdzArchive::partXmlPath() const
{
	const QString root = extractedDir();
	if (root.isEmpty()) {
		return QString();
	}
	QDirIterator it(root, QStringList{QStringLiteral("*.part.xml")},
			QDir::Files, QDirIterator::Subdirectories);
	return it.hasNext() ? it.next() : QString();
}

QString EdzArchive::errorString() const
{
	return m_error;
}

/**
	M0 backend: run a 7-Zip command-line tool to extract the archive.
	Placeholder for a bundled decompressor in a later milestone.
*/
bool EdzArchive::extractWithSevenZipCli(const QString &edz_path,
					const QString &dest)
{
	const QString exe = findSevenZip();
	if (exe.isEmpty()) {
		m_error = QStringLiteral(
			"7-Zip not found. A .edz is a 7z archive; install 7-Zip or "
			"put 7z on PATH.");
		return false;
	}

	QProcess proc;
	proc.start(exe, QStringList{QStringLiteral("x"), edz_path,
				    QStringLiteral("-o%1").arg(dest),
				    QStringLiteral("-y")});
	if (!proc.waitForStarted()) {
		m_error = QStringLiteral("Could not start 7-Zip: %1").arg(exe);
		return false;
	}
	proc.waitForFinished(-1);

	if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
		QString detail;
		const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
		for (const QString &line : out.split('\n')) {
			const QString t = line.trimmed();
			if (t.startsWith(QStringLiteral("ERROR"))
			    || t.startsWith(QStringLiteral("Cannot"))
			    || t.startsWith(QStringLiteral("Can't"))) {
				detail += (detail.isEmpty() ? QString() : QStringLiteral(" ")) + t;
			}
		}
		if (detail.isEmpty()) {
			detail = QStringLiteral("7-Zip exit code %1").arg(proc.exitCode());
		}
		m_error = QStringLiteral("7-Zip could not extract the archive: %1")
				  .arg(detail);
		return false;
	}
	return true;
}

/** Locate a 7-Zip executable: PATH first, then the usual install locations. */
QString EdzArchive::findSevenZip()
{
	for (const QString &name : {QStringLiteral("7z"), QStringLiteral("7za"),
				    QStringLiteral("7zr")}) {
		const QString found = QStandardPaths::findExecutable(name);
		if (!found.isEmpty()) {
			return found;
		}
	}
	const QStringList candidates {
		QStringLiteral("C:/Program Files/7-Zip/7z.exe"),
		QStringLiteral("C:/Program Files (x86)/7-Zip/7z.exe"),
		QStringLiteral("/usr/bin/7z"),
		QStringLiteral("/usr/local/bin/7z"),
		QStringLiteral("/opt/homebrew/bin/7z"),
	};
	for (const QString &c : candidates) {
		if (QFileInfo::exists(c)) {
			return c;
		}
	}
	return QString();
}
