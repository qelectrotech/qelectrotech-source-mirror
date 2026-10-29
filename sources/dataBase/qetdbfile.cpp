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
#include <QFuture>
#include <QtConcurrent>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QThread>
#include <QThreadPool>
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

	//The views a .qetdb may hold: the project database's own, and the
	//public ones other tools read it through (QETDB-S10)
const QStringList our_views{QStringLiteral("drawing_item_view"), QStringLiteral("element_nomenclature_view"),
							QStringLiteral("project_summary_view"), QStringLiteral("wiring_list_view"),
							QStringLiteral("qet_folios"), QStringLiteral("qet_devices"),
							QStringLiteral("qet_wires"), QStringLiteral("qet_links")};

template <typename Work>
bool withConnection(const QString &path, QString *error, Work work);

	//A hash over every value of every table but qet_format, in a fixed
	//order: SQLite's checks see broken pages, not a changed value inside
	//one (QET-SQL-FORMAT-SCOPE.md 5.4). Each table on a connection of its
	//own in parallel, BLAKE2b: 1 s of f2099's save and open as one SHA-256
	//read row by row. Empty if it cannot be read.
QString contentDigest(const QString &path)
{
	QStringList tables;
	withConnection(path, nullptr, [&tables](QSqlQuery &query) {
		if (!query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' "
									   "AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' AND name <> 'qet_format' ORDER BY name")))
			return false;
		while (query.next()) tables << query.value(0).toString();
		return true;
	});
	if (tables.isEmpty()) return {};
	const auto table_digest = [&path](const QString &table) {
		QCryptographicHash hash(QCryptographicHash::Blake2b_256);
		const auto add = [&hash](char tag, const QByteArray &bytes) {
			const quint64 size = quint64(bytes.size());
			hash.addData(QByteArrayView(&tag, 1));
			hash.addData(QByteArrayView(reinterpret_cast<const char *>(&size), sizeof size));
			hash.addData(bytes);
		};
		bool ok = withConnection(path, nullptr, [&](QSqlQuery &query) {
			query.setForwardOnly(true);
			if (!query.exec(QStringLiteral("SELECT * FROM \"%1\" ORDER BY rowid").arg(table)))
				return false;
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
			return true;
		});
		return ok ? hash.result() : QByteArray();
	};
	QThreadPool pool;
	pool.setMaxThreadCount(std::max(1, std::min(QThread::idealThreadCount(), int(tables.size()))));
	const QList<QByteArray> digests = QtConcurrent::blockingMapped(&pool, tables, std::function<QByteArray(const QString &)>(table_digest));
	QCryptographicHash all(QCryptographicHash::Blake2b_256);
	for (int i = 0 ; i < tables.size() ; ++i) {
		if (digests.at(i).isEmpty()) return {};
		all.addData(tables.at(i).toUtf8());
		all.addData(digests.at(i));
	}
	return QStringLiteral("blake2b:") + QString::fromLatin1(all.result().toHex());
}

	//What a project read back from the file gives (its document drops a
	//symbol's empty information, DiagramContext::toXml(), and reads an
	//empty number as ""): the file holds that, not what the session that
	//saved it held, so opening it from its tables gives the same database
	//as from its document (S4x X2)
bool normalize(QSqlQuery &query)
{
	if (!query.exec(QStringLiteral("DELETE FROM element_information WHERE trim(coalesce(value, '')) = ''")))
		return false;
		//Each symbol's rows numbered from 0, only the rows whose number changes rewritten
	if (!query.exec(QStringLiteral("UPDATE element_information SET ord = n.ord FROM "
										  "(SELECT rowid AS id, rank() OVER (PARTITION BY element_uuid ORDER BY ord) - 1 AS ord "
										  "FROM element_information) AS n "
										  "WHERE n.id = element_information.rowid AND n.ord <> element_information.ord")))
		return false;
	if (!query.exec(QStringLiteral("UPDATE sequential_number SET value = '' WHERE value IS NULL")))
		return false;
	QStringList columns;
	if (!query.exec(QStringLiteral("PRAGMA table_info(element_info)")))
		return false;
	while (query.next())
		if (query.value(1).toString() != QLatin1String("element_uuid")) columns << query.value(1).toString();
	if (columns.isEmpty())
		return true;
		//One pass over the table, rewriting only the rows with an empty value
	QStringList sets, empties;
	for (const QString &column : std::as_const(columns)) {
		sets << QStringLiteral("\"%1\" = NULLIF(\"%1\", '')").arg(column);
		empties << QStringLiteral("\"%1\" = ''").arg(column);
	}
	return query.exec(QStringLiteral("UPDATE element_info SET %1 WHERE %2")
					  .arg(sets.join(QStringLiteral(", ")), empties.join(QStringLiteral(" OR "))));
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
		//QET_QETDB_SAVE_TIMES=1: its own steps, in ms
	QElapsedTimer timer;
	timer.start();
	QStringList times;
	const auto lap = [&timer, &times](const char *step) {
		times << QStringLiteral("%1 %2").arg(QLatin1String(step)).arg(timer.restart());
		return true;
	};
	const bool ok = withConnection(path, error, [&path, &lap](QSqlQuery &query) {
			//The file is not the project's until it is flushed whole and
			//put in its place (QetDbFile::save()): a crash before leaves
			//only it, thrown away by the next save. So no journal on disk
			//and no flush per statement, and its changes in one transaction
		if (!query.exec(QStringLiteral("PRAGMA synchronous = OFF"))
				|| !query.exec(QStringLiteral("PRAGMA journal_mode = MEMORY"))
				|| !query.exec(QStringLiteral("BEGIN")))
			return false;
		if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS working_copy"))) return false;
			//The indexes stay: speed matters more than size (the user,
			//2026-10-28); VACUUM INTO wrote the file compact already
		if (!normalize(query) || !lap("normalize")) return false;
		if (!query.exec(QStringLiteral("CREATE TABLE qet_format (key TEXT PRIMARY KEY, value TEXT)"))) return false;
		const QList<std::pair<QString, QString>> rows{
			{QStringLiteral("format"), QString::number(QetDbFile::format)},
			{QStringLiteral("min_reader"), QStringLiteral("1")},
				//The qet_ views' version (projectDataBase::publicViews())
			{QStringLiteral("views"), QString::number(projectDataBase::PublicViewsVersion)},
			{QStringLiteral("writer"), QetVersion::displayedVersion()},
			{QStringLiteral("saved"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
		query.prepare(QStringLiteral("INSERT INTO qet_format (key, value) VALUES (:key, :value)"));
		for (const auto &[key, value] : rows) {
			query.bindValue(QStringLiteral(":key"), key);
			query.bindValue(QStringLiteral(":value"), value);
			if (!query.exec()) return false;
		}
			//Read by contentDigest() through connections of its own
		if (!query.exec(QStringLiteral("COMMIT")) || !lap("format and commit")) return false;
		if (!query.exec(QStringLiteral("PRAGMA application_id = %1").arg(QetDbFile::applicationId))
				|| !query.exec(QStringLiteral("PRAGMA user_version = %1").arg(QetDbFile::format))
				|| !query.exec(QStringLiteral("PRAGMA journal_mode = DELETE"))
				|| !lap("pragmas"))
			return false;
		const QString digest = contentDigest(path);
		if (digest.isEmpty() || !lap("digest")) return false;
		query.prepare(QStringLiteral("INSERT INTO qet_format (key, value) VALUES ('content_digest', :digest)"));
		query.bindValue(QStringLiteral(":digest"), digest);
		return query.exec() && lap("digest row");
	});
	if (ok && qEnvironmentVariableIntValue("QET_QETDB_SAVE_TIMES") == 1)
		qInfo().noquote() << "finish (ms):" << times.join(QStringLiteral(", "));
	return ok;
}

	//The checks a file passes before it replaces the project, and before
	//it is opened: SQLite's own, ours, a format this QElectroTech reads.
	//The content digest is not among them: the save writes it, and the
	//open compares it beside reading the file (QetDbFile::read())
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
bool QetDbFile::save(projectDataBase &database, const QString &path, QString *error, QByteArray *sha1)
{
	const QString saving = path + QStringLiteral(".saving");
	QFuture<QByteArray> hashing;
	const auto give_up = [&saving, &hashing]() {
			//Not removed while it is read (Windows refuses)
		if (hashing.isValid()) hashing.waitForFinished();
		for (const char *suffix : {"", "-journal", "-wal", "-shm"})
			QFile::remove(saving + QLatin1String(suffix));
		return false;
	};
	give_up();
		//QET_QETDB_SAVE_TIMES=1: how long each step took, in ms
	QElapsedTimer timer;
	timer.start();
	QStringList times;
	const auto lap = [&timer, &times](const char *step) {
		times << QStringLiteral("%1 %2").arg(QLatin1String(step)).arg(timer.restart());
		return true;
	};
	if (!database.copyTo(saving, error) || !lap("copy") || !finish(saving, error) || !lap("finish"))
		return give_up();
		//The file is complete and no one else's yet: hashed beside its
		//check and flush, not read again once it is the project
	if (sha1)
		hashing = QtConcurrent::run([saving]() {
			QFile file(saving);
			QCryptographicHash hash(QCryptographicHash::Sha1);
			if (!file.open(QIODevice::ReadOnly) || !hash.addData(&file))
				return QByteArray();
			return hash.result();
		});
		//Flushed while it is checked: neither changes it, and it replaces
		//nothing before both are done
	QFuture<bool> flushing = QtConcurrent::run([saving]() { return flushFile(saving); });
	const bool checked = check(saving, error);
	lap("check");
	const bool flushed = flushing.result();
	lap("flush");
	if (!checked)
		return give_up();
	if (!flushed) {
		if (error) *error = QObject::tr("the file could not be written to disk");
		return give_up();
	}
	if (sha1) {
		*sha1 = hashing.result();
		lap("hash");
	}
	if (!replace(saving, path, error))
		return give_up();
	lap("replace");
	if (qEnvironmentVariableIntValue("QET_QETDB_SAVE_TIMES") == 1)
		qInfo().noquote() << "QetDbFile::save (ms):" << times.join(QStringLiteral(", "));
	return true;
}

/**
	@brief QetDbFile::read
	@p document becomes the project the .qetdb @p path holds, as a .qet's
	document: read from a copy, so that the file itself is never opened for
	writing, once it passes the checks of a save. Handed over as it is, not
	written as text and parsed again (S4x X1). @p with_copy, if given, is
	called with the checked copy, indexes made again, while it exists; if
	it returns true, the folios' items are left out of @p document, marked
	for projectDataBase::writeFolioContent().
	@return false, with @p error saying why, if it cannot be read
*/
bool QetDbFile::read(const QString &path, QDomDocument *document, QString *error,
					 const std::function<bool(const QString &copy)> &with_copy)
{
	QTemporaryDir dir;
	const QString copy = dir.filePath(QStringLiteral("project.qetdb"));
	if (!dir.isValid() || !QFile::copy(path, copy)) {
		if (error) *error = QObject::tr("the file could not be read");
		return false;
	}
	if (!check(copy, error))
		return false;
	QString saved_digest;
	withConnection(copy, nullptr, [&saved_digest](QSqlQuery &query) {
			//Written by every save but #1642's, which neither normalized
		if (query.exec(QStringLiteral("SELECT value FROM qet_format WHERE key = 'content_digest'"))
				&& query.next())
			saved_digest = query.value(0).toString();
		else
			normalize(query);
		return true;
	});
		//The content digest, worked out on a connection of its own beside
		//what follows, which only reads the copy too: a second off f2099's
		//open
	QFuture<QString> digest;
	if (!saved_digest.isEmpty())
		digest = QtConcurrent::run([copy]() {
			return contentDigest(copy);
		});
		//The checked copy, for the project database to start from (S4x X2):
		//if it did, the folios' items are left in it until each is built
		//(S4x X3)
	const bool deferred = with_copy && with_copy(copy);
	const bool read = projectDataBase::readDocument(copy, document, error, deferred);
	if (!saved_digest.isEmpty() && digest.result() != saved_digest) {
		if (error) *error = QObject::tr("its content is not what QElectroTech saved: it is damaged, "
										"or another program changed it");
		return false;
	}
	return read;
}

/**
	@brief QetDbFile::isReadable
	@return true if @p path passes the checks a .qetdb passes before it is
	opened, with @p error saying why not otherwise
*/
bool QetDbFile::isReadable(const QString &path, QString *error)
{
	return check(path, error);
}
