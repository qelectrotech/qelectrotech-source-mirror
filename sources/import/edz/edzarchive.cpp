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

#include "edzsevenzip.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
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

	// A .edz is a 7-Zip archive (magic "7z\xBC\xAF\x27\x1C"). Some EPLAN exports
	// are instead zip-format ("PK\x03\x04"); the bundled reader only does 7z, so
	// detect that up front and say so clearly rather than failing opaquely.
	QFile probe(fi.absoluteFilePath());
	if (!probe.open(QIODevice::ReadOnly)) {
		m_error = QStringLiteral("Cannot read %1").arg(edz_path);
		return false;
	}
	const QByteArray magic = probe.read(6);
	probe.close();
	static const QByteArray k7z("7z\xBC\xAF\x27\x1C", 6);
	if (!magic.startsWith(k7z)) {
		if (magic.startsWith(QByteArray("PK\x03\x04", 4))) {
			m_error = QStringLiteral(
				"This .edz is a zip-format package, which is not yet "
				"supported (only 7-Zip .edz files can be imported).");
		} else {
			m_error = QStringLiteral(
				"Not a valid .edz package (unrecognised archive format).");
		}
		return false;
	}

	m_dir.reset(new QTemporaryDir);
	if (!m_dir->isValid()) {
		m_error = QStringLiteral("Could not create a temporary directory: %1")
				  .arg(m_dir->errorString());
		return false;
	}

	if (!sevenZipExtract(fi.absoluteFilePath(), m_dir->path(), m_error)) {
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
