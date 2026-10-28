// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "qetdbfile.h"

#include "projectdatabase.h"
#include "../qetversion.h"
#include "../utils/syncfolder.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>

#ifdef Q_OS_WIN
#	include <windows.h>
#	include <io.h>
#else
#	include <cerrno>
#	include <cstdio>
#	include <cstring>
#	include <unistd.h>
#endif

namespace {

	//The views a .qetdb may hold: the project database's own
const QStringList our_views{QStringLiteral("drawing_item_view"), QStringLiteral("element_nomenclature_view"),
							QStringLiteral("project_summary_view"), QStringLiteral("wiring_list_view")};

	//A hash over every value of every table but qet_format, in a fixed
	//order: SQLite's checks see broken pages, not a changed value inside
	//one (QET-SQL-FORMAT-SCOPE.md 5.4). Empty if it cannot be read.
QString contentDigest(QSqlQuery &query)
{
	QStringList tables;
	if (!query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' "
								   "AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' AND name <> 'qet_format' ORDER BY name")))
		return {};
	while (query.next()) tables << query.value(0).toString();
	QCryptographicHash hash(QCryptographicHash::Sha256);
	const auto add = [&hash](char tag, const QByteArray &bytes) {
		const quint64 size = quint64(bytes.size());
		hash.addData(QByteArrayView(&tag, 1));
		hash.addData(QByteArrayView(reinterpret_cast<const char *>(&size), sizeof size));
		hash.addData(bytes);
	};
	for (const QString &table : std::as_const(tables)) {
		add('T', table.toUtf8());
		if (!query.exec(QStringLiteral("SELECT * FROM \"%1\" ORDER BY rowid").arg(table)))
			return {};
		const int columns = query.record().count();
		while (query.next()) {
			add('R', {});
			for (int i = 0 ; i < columns ; ++i) {
				const QVariant value = query.value(i);
				if (value.isNull()) add('N', {});
				else if (value.typeId() == QMetaType::QByteArray) add('B', value.toByteArray());
				else if (value.typeId() == QMetaType::Double) add('F', QByteArray::number(value.toDouble(), 'g', 17));
				else if (value.typeId() == QMetaType::LongLong || value.typeId() == QMetaType::Int)
					add('I', QByteArray::number(value.toLongLong()));
				else add('S', value.toString().toUtf8());
			}
		}
	}
	return QString::fromLatin1(hash.result().toHex());
}

	//Runs @p work on a connection of its own to the file @p path
template <typename Work>
bool withConnection(const QString &path, QString *error, Work work)
{
	const QString name = QStringLiteral("qetdb_file_") + QUuid::createUuid().toString();
	bool ok = false;
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
		db.setDatabaseName(path);
		if (!db.open()) {
			if (error) *error = db.lastError().text();
		} else {
			QSqlQuery query(db);
			ok = work(query);
			if (!ok && error && error->isEmpty()) *error = query.lastError().text();
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(name);
	return ok;
}

	//What a .qetdb is beside the project database: who wrote it, its
	//format, at rest with nothing beside it; the database's own indexes
	//and the working copy's row left out (QETDB-S4-SCOPE.md 3.1, E7)
bool finish(const QString &path, QString *error)
{
	return withConnection(path, error, [](QSqlQuery &query) {
		if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS working_copy"))) return false;
		QStringList indexes;
		if (!query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'index' "
									   "AND name LIKE 'idx\\_%' ESCAPE '\\'"))) return false;
		while (query.next()) indexes << query.value(0).toString();
		for (const QString &index : std::as_const(indexes))
			if (!query.exec(QStringLiteral("DROP INDEX \"%1\"").arg(index))) return false;
		if (!query.exec(QStringLiteral("CREATE TABLE qet_format (key TEXT PRIMARY KEY, value TEXT)"))) return false;
		const QList<std::pair<QString, QString>> rows{
			{QStringLiteral("format"), QString::number(QetDbFile::format)},
			{QStringLiteral("min_reader"), QStringLiteral("1")},
			{QStringLiteral("writer"), QetVersion::displayedVersion()},
			{QStringLiteral("saved"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
		query.prepare(QStringLiteral("INSERT INTO qet_format (key, value) VALUES (:key, :value)"));
		for (const auto &[key, value] : rows) {
			query.bindValue(QStringLiteral(":key"), key);
			query.bindValue(QStringLiteral(":value"), value);
			if (!query.exec()) return false;
		}
		if (!query.exec(QStringLiteral("PRAGMA application_id = %1").arg(QetDbFile::applicationId))
				|| !query.exec(QStringLiteral("PRAGMA user_version = %1").arg(QetDbFile::format))
				|| !query.exec(QStringLiteral("PRAGMA journal_mode = DELETE"))
				|| !query.exec(QStringLiteral("VACUUM")))
			return false;
			//After the VACUUM, which can renumber rows
		const QString digest = contentDigest(query);
		if (digest.isEmpty()) return false;
		query.prepare(QStringLiteral("INSERT INTO qet_format (key, value) VALUES ('content_digest', :digest)"));
		query.bindValue(QStringLiteral(":digest"), digest);
		return query.exec();
	});
}

	//The checks a file passes before it replaces the project, and before
	//it is opened: SQLite's own, ours, a format this QElectroTech reads
bool check(const QString &path, QString *error)
{
	return withConnection(path, error, [error](QSqlQuery &query) {
		if (!query.exec(QStringLiteral("PRAGMA quick_check")) || !query.next()
				|| query.value(0).toString() != QLatin1String("ok")) {
			if (error) *error = QObject::tr("the file is damaged");
			return false;
		}
		if (!query.exec(QStringLiteral("PRAGMA application_id")) || !query.next()
				|| query.value(0).toInt() != QetDbFile::applicationId) {
			if (error) *error = QObject::tr("the file is not a QElectroTech project");
			return false;
		}
		if (!query.exec(QStringLiteral("SELECT value FROM qet_format WHERE key = 'min_reader'"))
				|| !query.next()) {
			if (error) *error = QObject::tr("the file is not a QElectroTech project");
			return false;
		}
		const int min_reader = query.value(0).toInt();
		if (min_reader > QetDbFile::format) {
			QString writer;
			if (query.exec(QStringLiteral("SELECT value FROM qet_format WHERE key = 'writer'")) && query.next())
				writer = query.value(0).toString();
			if (error) *error = QObject::tr("it was saved by a newer QElectroTech (%1)").arg(writer);
			return false;
		}
			//Nothing that runs SQL of its own: no trigger, no view but ours,
			//every table a table (QET-SQL-FORMAT-SCOPE.md 5.6)
		if (!query.exec(QStringLiteral("SELECT type, name FROM sqlite_master WHERE type IN ('trigger', 'view')")))
			return false;
		while (query.next()) {
			if (query.value(0).toString() == QLatin1String("trigger")
					|| !our_views.contains(query.value(1).toString())) {
				if (error) *error = QObject::tr("the file holds SQL that QElectroTech does not expect (%1)")
						.arg(query.value(1).toString());
				return false;
			}
		}
		if (!query.exec(QStringLiteral("PRAGMA foreign_key_check")) || query.next()) {
			if (error) *error = QObject::tr("the file is damaged: an item refers to one that is not there");
			return false;
		}
			//Written by every save since the first files (format 1 without it)
		if (query.exec(QStringLiteral("SELECT value FROM qet_format WHERE key = 'content_digest'"))
				&& query.next()) {
			const QString saved = query.value(0).toString();
			if (contentDigest(query) != saved) {
				if (error) *error = QObject::tr("its content is not what QElectroTech saved: it is damaged, "
												"or another program changed it");
				return false;
			}
		}
		return true;
	});
}

	//To disk, before it replaces anything
bool flushFile(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadWrite)) return false;
#ifdef Q_OS_WIN
	return FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(file.handle())));
#else
	return ::fsync(file.handle()) == 0;
#endif
}

	//@p from in place of @p to, at once; on Windows, retried while a virus
	//scanner or an indexer holds a file for a moment
bool replace(const QString &from, const QString &to, QString *error)
{
#ifdef Q_OS_WIN
	const std::wstring source = QDir::toNativeSeparators(from).toStdWString();
	const std::wstring target = QDir::toNativeSeparators(to).toStdWString();
	QElapsedTimer timer;
	timer.start();
	for (;;) {
		if (MoveFileExW(source.c_str(), target.c_str(),
						MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) break;
		const DWORD code = GetLastError();
		if ((code != ERROR_ACCESS_DENIED && code != ERROR_SHARING_VIOLATION) || timer.elapsed() > 2000) {
			if (error) *error = QObject::tr("the file could not be replaced (error %1)").arg(code);
			return false;
		}
		QThread::msleep(100);
	}
		//Best effort: MOVEFILE_WRITE_THROUGH does not flush the folder
	const std::wstring folder = QDir::toNativeSeparators(QFileInfo(to).absolutePath()).toStdWString();
	const HANDLE handle = CreateFileW(folder.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
									  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (handle != INVALID_HANDLE_VALUE) {
		FlushFileBuffers(handle);
		CloseHandle(handle);
	}
#else
	if (std::rename(QFile::encodeName(from).constData(), QFile::encodeName(to).constData()) != 0) {
		if (error) *error = QString::fromLocal8Bit(strerror(errno));
		return false;
	}
	QET::syncFolderOf(to);
#endif
	return true;
}

} // namespace

/**
	@return true if @p path names a .qetdb file
*/
bool QetDbFile::isDatabasePath(const QString &path)
{
	return path.endsWith(QLatin1String(".qetdb"), Qt::CaseInsensitive);
}

/**
	@return true if @p head, a file's first bytes, are those of a SQLite
	database
*/
bool QetDbFile::isDatabase(const QByteArray &head)
{
	return head.startsWith(QByteArrayLiteral("SQLite format 3\0"));
}

/**
	@brief QetDbFile::save
	Save the project @p database holds as the .qetdb @p path: written whole
	beside it (<path>.saving), checked, flushed to disk, then put in its
	place at once. On any failure @p path is as it was.
*/
bool QetDbFile::save(projectDataBase &database, const QString &path, QString *error)
{
	const QString saving = path + QStringLiteral(".saving");
	const auto give_up = [&saving]() {
		for (const char *suffix : {"", "-journal", "-wal", "-shm"})
			QFile::remove(saving + QLatin1String(suffix));
		return false;
	};
	give_up();
	if (!database.copyTo(saving, error) || !finish(saving, error) || !check(saving, error))
		return give_up();
	if (!flushFile(saving)) {
		if (error) *error = QObject::tr("the file could not be written to disk");
		return give_up();
	}
	if (!replace(saving, path, error))
		return give_up();
	return true;
}

/**
	@brief QetDbFile::readAsQet
	The project the .qetdb @p path holds, as the text of a .qet: read from a
	copy, so that the file itself is never opened for writing, once it
	passes the checks of a save.
	@return the text, or an empty array with @p error saying why
*/
QByteArray QetDbFile::readAsQet(const QString &path, QString *error)
{
	QTemporaryDir dir;
	const QString copy = dir.filePath(QStringLiteral("project.qetdb"));
	const QString written = dir.filePath(QStringLiteral("project.qet"));
	if (!dir.isValid() || !QFile::copy(path, copy)) {
		if (error) *error = QObject::tr("the file could not be read");
		return {};
	}
	if (!check(copy, error) || !projectDataBase::writeDocument(copy, written, error))
		return {};
	QFile file(written);
	if (!file.open(QIODevice::ReadOnly)) {
		if (error) *error = file.errorString();
		return {};
	}
	return file.readAll();
}
