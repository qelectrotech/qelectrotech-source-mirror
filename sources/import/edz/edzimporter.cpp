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
#include "edzimporter.h"

#include "edzarchive.h"
#include "edzpart.h"
#include "edzelementbuilder.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QRegularExpression>

/**
	Import the .edz at @a edz_path, writing the generated .elmt into the
	filesystem folder @a dest_dir. @return true on success; writtenPath() then
	holds the created file. On failure errorString() explains why.
*/
bool EdzImporter::importToDirectory(const QString &edz_path,
				    const QString &dest_dir)
{
	m_written.clear();
	m_error.clear();

	EdzArchive archive;
	if (!archive.extract(edz_path)) {
		m_error = archive.errorString();
		return false;
	}

	EdzPart part;
	if (!part.parse(archive.partXmlPath())) {
		m_error = part.errorString();
		return false;
	}

	const QDomDocument doc = EdzElementBuilder::build(part);

	// Name the file from the order number (fall back to part number / source).
	QString base = part.orderNumber();
	if (base.isEmpty()) {
		base = part.partNumber();
	}
	if (base.isEmpty()) {
		base = QFileInfo(edz_path).completeBaseName();
	}
	base.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")),
		     QStringLiteral("_"));

	const QDir dir(dest_dir);
	if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
		m_error = QStringLiteral("Destination folder does not exist: %1")
				  .arg(dest_dir);
		return false;
	}
	const QString out_path = dir.filePath(base + QStringLiteral(".elmt"));

	QFile file(out_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		m_error = QStringLiteral("Cannot write %1").arg(out_path);
		return false;
	}
	file.write(EdzElementBuilder::toElmtString(doc).toUtf8());
	file.close();

	m_written = out_path;
	return true;
}
