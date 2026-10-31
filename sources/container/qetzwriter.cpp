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
#include "qetzwriter.h"

#include "../qetversion.h"
#include "qetcontainer.h"
#include "qetzip.h"

/**
	@brief QetzWriter::write
	Write the project document @p project to @p path as a .qetz: the same
	document, in parts, zipped (#1440). The file is replaced only once
	the whole archive is written. @p kept_from: the entries of the .qetz
	read, whose parts the folios not built are copied from.
*/
bool QetzWriter::write(const QDomDocument &project, const QString &path, QString *error,
					   const QHash<QString, QByteArray> &kept_from)
{
	QList<QetZip::Entry> entries = QetContainer::split(
				project,
				QStringLiteral("QElectroTech %1").arg(QetVersion::currentVersion().toString()),
				error, kept_from);
	return !entries.isEmpty() && QetZip::write(path, entries, error);
}
