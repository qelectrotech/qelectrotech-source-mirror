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
#include "qetzip.h"

#include "miniz/miniz.h"
#include "../utils/syncfolder.h"

#include <QFile>
#include <QSaveFile>
#include <QSet>
#include <QtEndian>

namespace {
void setError(QString *error, const QString &text)
{
	if (error) *error = text;
}

	//A name read from an archive is only ever a key, but refuse the ones a
	//careless caller could turn into a path outside a folder: absolute,
	//with a drive, with "..", or not valid UTF-8.
bool safeName(const QString &name)
{
	if (name.isEmpty() || name.startsWith(QLatin1Char('/'))
		|| name.contains(QLatin1Char('\\')) || name.contains(QLatin1Char(':'))
		|| name.contains(QChar::ReplacementCharacter)) {
		return false;
	}
	for (const QString &part : name.split(QLatin1Char('/'))) {
		if (part == QLatin1String("..")) return false;
	}
	return true;
}

	//Built with MINIZ_NO_TIME, miniz leaves each entry's DOS date at 0,
	//day 0 of month 0, which is no date: unzip shows 30 Nov 1979, other
	//tools 13 Dec 1901. Give every entry 1 Jan 1980 00:00 instead, the
	//first valid DOS date, so the bytes still do not depend on the time.
constexpr quint16 EntryDosDate = (1 << 5) | 1;

bool setEntryDates(QByteArray *bytes)
{
	char *data = bytes->data();
	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	if (!mz_zip_reader_init_mem(&zip, data, size_t(bytes->size()), 0)) return false;

	bool ok = true;
	const mz_uint count = mz_zip_reader_get_num_files(&zip);
	for (mz_uint i = 0 ; i < count && ok ; ++i)
	{
		mz_zip_archive_file_stat stat;
		if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
			ok = false;
			break;
		}
			//date at offset 12 of the local header, 14 of the central one
		const quint64 local = stat.m_local_header_ofs + 12;
		const quint64 central = zip.m_central_directory_file_ofs + stat.m_central_dir_ofs + 14;
		if (local + 2 > quint64(bytes->size()) || central + 2 > quint64(bytes->size())) {
			ok = false;
			break;
		}
		qToLittleEndian<quint16>(EntryDosDate, data + local);
		qToLittleEndian<quint16>(EntryDosDate, data + central);
	}
	mz_zip_reader_end(&zip);
	return ok;
}
}

/**
	@brief QetZip::toBytes
	@return the zip holding @p entries in their order, or an empty array
	with @p error set
*/
QByteArray QetZip::toBytes(const QList<Entry> &entries, QString *error)
{
	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	if (!mz_zip_writer_init_heap(&zip, 0, 0)) {
		setError(error, QStringLiteral("cannot start a zip archive"));
		return {};
	}

	QSet<QString> names;
	for (const Entry &entry : entries)
	{
		if (!safeName(entry.name) || names.contains(entry.name)) {
			setError(error, QStringLiteral("invalid or repeated entry name \"%1\"").arg(entry.name));
			mz_zip_writer_end(&zip);
			return {};
		}
		names.insert(entry.name);
		const QByteArray name = entry.name.toUtf8();
		const mz_uint level = entry.compress ? MZ_DEFAULT_LEVEL : MZ_NO_COMPRESSION;
		if (!mz_zip_writer_add_mem(&zip, name.constData(), entry.data.constData(),
								   size_t(entry.data.size()), level)) {
			setError(error, QStringLiteral("cannot add \"%1\": %2")
					 .arg(entry.name, QString::fromLatin1(mz_zip_get_error_string(mz_zip_get_last_error(&zip)))));
			mz_zip_writer_end(&zip);
			return {};
		}
	}

	void *buffer = nullptr;
	size_t size = 0;
	if (!mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size)) {
		setError(error, QStringLiteral("cannot finish the zip archive"));
		mz_zip_writer_end(&zip);
		return {};
	}
	QByteArray bytes(static_cast<const char *>(buffer), qsizetype(size));
	mz_zip_writer_end(&zip);   //frees buffer
	if (!setEntryDates(&bytes)) {
		setError(error, QStringLiteral("cannot date the zip entries"));
		return {};
	}
	return bytes;
}

/**
	@brief QetZip::write
	Write @p entries to @p path, replacing it only once the whole archive
	is written.
*/
bool QetZip::write(const QString &path, const QList<Entry> &entries, QString *error)
{
	const QByteArray bytes = toBytes(entries, error);
	if (bytes.isEmpty()) return false;

	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		setError(error, file.errorString());
		return false;
	}
	if (file.write(bytes) != bytes.size() || !file.commit()) {
		setError(error, file.errorString());
		return false;
	}
	QET::syncFolderOf(path);
	return true;
}

/**
	@brief QetZip::fromBytes
	Read every file entry of the zip @p zip into @p entries, in archive
	order. Folders are skipped. An encrypted entry, an unsafe or repeated
	name, a method miniz cannot read, or more than MaxEntries entries or
	MaxUncompressedSize bytes refuse the whole archive. With @p skipped (a
	damaged project recovered), an entry that cannot be read is named
	there and left out instead; the size limits still refuse it all.
*/
bool QetZip::fromBytes(const QByteArray &zip_bytes, QList<Entry> *entries, QString *error,
					   QStringList *skipped)
{
	if (!entries) return false;
	entries->clear();

	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	if (!mz_zip_reader_init_mem(&zip, zip_bytes.constData(), size_t(zip_bytes.size()), 0)) {
		setError(error, QStringLiteral("not a zip archive: %1")
				 .arg(QString::fromLatin1(mz_zip_get_error_string(mz_zip_get_last_error(&zip)))));
		return false;
	}

	const mz_uint count = mz_zip_reader_get_num_files(&zip);
	if (count > mz_uint(MaxEntries)) {
		setError(error, QStringLiteral("too many entries (%1)").arg(count));
		mz_zip_reader_end(&zip);
		return false;
	}

	qint64 total = 0;
	QSet<QString> names;
	for (mz_uint i = 0 ; i < count ; ++i)
	{
		mz_zip_archive_file_stat stat;
		if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
			if (skipped) {
				skipped->append(QStringLiteral("unreadable entry %1").arg(i));
				continue;
			}
			setError(error, QStringLiteral("unreadable entry %1").arg(i));
			mz_zip_reader_end(&zip);
			return false;
		}
		if (stat.m_is_directory) continue;

		const QString name = QString::fromUtf8(stat.m_filename);
		if (stat.m_is_encrypted || !stat.m_is_supported || !safeName(name)
			|| names.contains(name)) {
			if (skipped) {
				skipped->append(QStringLiteral("entry \"%1\" cannot be read").arg(name));
				continue;
			}
			setError(error, QStringLiteral("entry \"%1\" cannot be read").arg(name));
			mz_zip_reader_end(&zip);
			return false;
		}
		total += qint64(stat.m_uncomp_size);
		if (total > MaxUncompressedSize) {
			setError(error, QStringLiteral("archive too large once unpacked"));
			mz_zip_reader_end(&zip);
			return false;
		}

		size_t size = 0;
		void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
		if (!data) {
			const QString problem = QStringLiteral("cannot unpack \"%1\": %2")
					.arg(name, QString::fromLatin1(mz_zip_get_error_string(mz_zip_get_last_error(&zip))));
			if (skipped) {
				skipped->append(problem);
				continue;
			}
			setError(error, problem);
			mz_zip_reader_end(&zip);
			return false;
		}
		Entry entry;
		entry.name = name;
		entry.data = QByteArray(static_cast<const char *>(data), qsizetype(size));
		entry.compress = stat.m_method != 0;
		mz_free(data);
		names.insert(name);
		entries->append(entry);
	}
	mz_zip_reader_end(&zip);
	return true;
}

/**
	@brief QetZip::read
	fromBytes() on the file at @p path
*/
bool QetZip::read(const QString &path, QList<Entry> *entries, QString *error)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		setError(error, file.errorString());
		return false;
	}
	return fromBytes(file.readAll(), entries, error);
}
