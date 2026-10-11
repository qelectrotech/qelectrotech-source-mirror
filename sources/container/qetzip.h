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
#ifndef QETZIP_H
#define QETZIP_H

#include <QByteArray>
#include <QList>
#include <QString>

/**
	@brief The QetZip namespace
	Reads and writes the zip file a .qetz project is stored in: plain ZIP,
	Deflate, as ODF and OOXML documents are, so any unzip tool opens it.
	A whole archive is read into, or written from, memory -- projects are
	megabytes, not gigabytes -- and written through QSaveFile, so a failed
	save leaves the previous file as it was.

	Entries carry no timestamp: saving the same project twice gives the
	same bytes.
*/
namespace QetZip
{
	struct Entry
	{
		QString name;      ///< path inside the archive, '/'-separated
		QByteArray data;
		bool compress = true; ///< false: stored (pictures, the mimetype entry)
	};

		/// Largest archive read(): uncompressed total and entry count
	constexpr qint64 MaxUncompressedSize = qint64(2) * 1024 * 1024 * 1024;
	constexpr int MaxEntries = 100000;

	bool write(const QString &path, const QList<Entry> &entries, QString *error = nullptr);
	QByteArray toBytes(const QList<Entry> &entries, QString *error = nullptr);
	bool read(const QString &path, QList<Entry> *entries, QString *error = nullptr);
	bool fromBytes(const QByteArray &zip, QList<Entry> *entries, QString *error = nullptr);
}

#endif // QETZIP_H
