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
#include "qetparity.h"
#include "../qetversion.h"
#include "../utils/syncfolder.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFuture>
#include <QHash>
#include <QtConcurrent>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QThread>
#include <QSysInfo>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>
#include <QtEndian>

#include <algorithm>
#include <limits>
#include <cstring>

	//The session extension's API is declared only with these
#define SQLITE_ENABLE_SESSION
#define SQLITE_ENABLE_PREUPDATE_HOOK
#include <sqlite3.h>

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

	//The history a project keeps in its .qetdb when told to, one revision
	//per save (QETDB-S6-SCOPE.md): neither the content digest nor a
	//revision covers these tables, and the working copy does not hold
	//them: each save carries them over from the file it replaces
const QString history_schema = QStringLiteral(
	"CREATE TABLE IF NOT EXISTS revision (uuid TEXT PRIMARY KEY NOT NULL, parent_uuid TEXT, "
	"ord INTEGER NOT NULL, author TEXT, host TEXT, time TEXT NOT NULL, message TEXT, "
	"kind TEXT NOT NULL, release_label TEXT, digest TEXT, prev_digest TEXT); "
	"CREATE TABLE IF NOT EXISTS revision_changes (revision TEXT PRIMARY KEY NOT NULL, changeset BLOB)");
	//How many saves the history keeps (S6-6): only in a file told to keep
	//fewer than all, so that a file without a limit is one the history's
	//first version reads too
const QString history_setting_schema = QStringLiteral(
	"CREATE TABLE IF NOT EXISTS revision_setting (key TEXT PRIMARY KEY NOT NULL, value TEXT)");

	//The tables neither the content digest nor a revision covers: what the
	//file says of itself, its history and its repair data (QETDB-S4C-PLAN.md).
	//Each save names them in qet_format's digest_skips, so that a table a
	//later QElectroTech adds there does not make the file unreadable to
	//this one. Only names a project table never has are taken from a file
const QStringList unsigned_tables{QStringLiteral("qet_format"), QStringLiteral("qet_parity"),
								  QStringLiteral("revision"), QStringLiteral("revision_changes"),
								  QStringLiteral("revision_setting")};

bool isUnsigned(const QString &table, const QStringList &listed)
{
	return unsigned_tables.contains(table)
			|| (listed.contains(table) && (table.startsWith(QLatin1String("qet_"))
										   || table.startsWith(QLatin1String("revision"))));
}

template <typename Work>
bool withConnection(const QString &path, QString *error, Work work);

	//A hash over every value of every table but qet_format, in a fixed
	//order: SQLite's checks see broken pages, not a changed value inside
	//one (QET-SQL-FORMAT-SCOPE.md 5.4). Each table on a connection of its
	//own in parallel, BLAKE2b: 1 s of f2099's save and open as one SHA-256
	//read row by row. Empty if it cannot be read. The tables in @p reuse
	//are not read again: their digests are taken from it (a save, of the
	//tables no one wrote to since the file before, QETDB-S6-SCOPE.md §8
	//H5). @p table_digests, if given, gets each table's.
QString contentDigest(const QString &path, const QHash<QString, QByteArray> &reuse = {},
					  QHash<QString, QByteArray> *table_digests = nullptr)
{
	QStringList tables;
	withConnection(path, nullptr, [&tables](QSqlQuery &query) {
		QStringList listed;
		if (query.exec(QStringLiteral("SELECT value FROM qet_format WHERE key = 'digest_skips'")) && query.next())
			listed = query.value(0).toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
		if (!query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' "
									   "AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' ORDER BY name")))
			return false;
		while (query.next())
			if (!isUnsigned(query.value(0).toString(), listed)) tables << query.value(0).toString();
		return true;
	});
	if (tables.isEmpty()) return {};
	const auto table_digest = [&path, &reuse](const QString &table) {
		if (reuse.contains(table)) return reuse.value(table);
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
		if (table_digests) table_digests->insert(tables.at(i), digests.at(i));
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
	//and the working copy's row left out (QETDB-S4-SCOPE.md 3.1, E7).
	//@p reuse and @p table_digests: contentDigest()'s
bool finish(const QString &path, QString *error, const QHash<QString, QByteArray> &reuse = {},
			QHash<QString, QByteArray> *table_digests = nullptr)
{
		//QET_QETDB_SAVE_TIMES=1: its own steps, in ms
	QElapsedTimer timer;
	timer.start();
	QStringList times;
	const auto lap = [&timer, &times](const char *step) {
		times << QStringLiteral("%1 %2").arg(QLatin1String(step)).arg(timer.restart());
		return true;
	};
	const bool ok = withConnection(path, error, [&path, &lap, &reuse, table_digests, error](QSqlQuery &query) {
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
			{QStringLiteral("digest_skips"), unsigned_tables.join(QLatin1Char(','))},
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
		const QString digest = contentDigest(path, reuse, table_digests);
		if (digest.isEmpty() || !lap("digest")) return false;
			//QET_QETDB_CHECK_DIGEST=1 (tests): the reused digests are what
			//reading every table again gives
		if (!reuse.isEmpty() && qEnvironmentVariableIntValue("QET_QETDB_CHECK_DIGEST") == 1) {
			QHash<QString, QByteArray> read_again;
			contentDigest(path, {}, &read_again);
			QStringList wrong;
			for (auto it = reuse.constBegin() ; it != reuse.constEnd() ; ++it)
				if (read_again.value(it.key()) != it.value()) wrong << it.key();
			QStringList read;
			for (auto it = read_again.constBegin() ; it != read_again.constEnd() ; ++it)
				if (!reuse.contains(it.key())) read << it.key();
			read.sort();
			qInfo().noquote() << "QetDbFile: content digest reused for" << reuse.size() << "of"
							  << read_again.size() << "tables; read:" << read.join(QStringLiteral(", "));
			if (!wrong.isEmpty()) {
				const QString why = QStringLiteral("content digest reused for changed tables: ")
						+ wrong.join(QStringLiteral(", "));
				qWarning().noquote() << "QetDbFile:" << why;
				if (error) *error = why;
				return false;
			}
		}
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

	//A libsqlite3 connection of its own, never the QSQLITE driver's: Qt may
	//carry a SQLite of its own in that driver (#1045), and the session
	//extension a revision is made with is libsqlite3's
class Sqlite
{
	public:
		Sqlite(const QString &path, bool write)
		{
			const int flags = (write ? SQLITE_OPEN_READWRITE : SQLITE_OPEN_READONLY) | SQLITE_OPEN_URI;
			if (sqlite3_open_v2(QFile::encodeName(path).constData(), &m_db, flags, nullptr) != SQLITE_OK) {
				sqlite3_close(m_db);
				m_db = nullptr;
			}
		}
		~Sqlite() { if (m_db) sqlite3_close(m_db); }
		Sqlite(const Sqlite &) = delete;
		Sqlite &operator=(const Sqlite &) = delete;
		sqlite3 *db() const { return m_db; }
		QString error() const
		{
			return m_db ? QString::fromUtf8(sqlite3_errmsg(m_db)) : QObject::tr("the file could not be opened");
		}
		bool exec(const QString &sql)
		{
			return m_db && sqlite3_exec(m_db, sql.toUtf8().constData(), nullptr, nullptr, nullptr) == SQLITE_OK;
		}
			//The rows @p sql gives, with @p values bound in order
		QList<QVariantList> select(const QString &sql, const QVariantList &values = {})
		{
			QList<QVariantList> rows;
			sqlite3_stmt *statement = nullptr;
			if (!m_db || sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &statement, nullptr) != SQLITE_OK)
				return rows;
			bind(statement, values);
			while (sqlite3_step(statement) == SQLITE_ROW) {
				QVariantList row;
				for (int i = 0 ; i < sqlite3_column_count(statement) ; ++i) {
					switch (sqlite3_column_type(statement, i)) {
						case SQLITE_NULL: row << QVariant(); break;
						case SQLITE_INTEGER: row << QVariant(qlonglong(sqlite3_column_int64(statement, i))); break;
						case SQLITE_BLOB:
							row << QByteArray(static_cast<const char *>(sqlite3_column_blob(statement, i)),
											  sqlite3_column_bytes(statement, i));
							break;
						default:
							row << QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, i)),
													 sqlite3_column_bytes(statement, i));
					}
				}
				rows << row;
			}
			sqlite3_finalize(statement);
			return rows;
		}
		bool run(const QString &sql, const QVariantList &values)
		{
			sqlite3_stmt *statement = nullptr;
			if (!m_db || sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &statement, nullptr) != SQLITE_OK)
				return false;
			bind(statement, values);
			const bool done = sqlite3_step(statement) == SQLITE_DONE;
			sqlite3_finalize(statement);
			return done;
		}

	private:
		static void bind(sqlite3_stmt *statement, const QVariantList &values)
		{
			for (int i = 0 ; i < values.size() ; ++i) {
				const QVariant &value = values.at(i);
				if (value.isNull())
					sqlite3_bind_null(statement, i + 1);
				else if (value.typeId() == QMetaType::QByteArray) {
					const QByteArray bytes = value.toByteArray();
					sqlite3_bind_blob(statement, i + 1, bytes.constData(), int(bytes.size()), SQLITE_TRANSIENT);
				} else if (value.typeId() == QMetaType::LongLong || value.typeId() == QMetaType::Int)
					sqlite3_bind_int64(statement, i + 1, value.toLongLong());
				else {
					const QByteArray text = value.toString().toUtf8();
					sqlite3_bind_text(statement, i + 1, text.constData(), int(text.size()), SQLITE_TRANSIENT);
				}
			}
		}
		sqlite3 *m_db = nullptr;
};

	//@p path as SQLite reads a file name in ATTACH, read-only
QString readOnlyUri(const QString &path)
{
	return QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()).toString(QUrl::FullyEncoded)
			.replace(QLatin1Char('\''), QStringLiteral("''")) + QStringLiteral("?mode=ro");
}

	//Who and where, as a revision records them: a record, not a signature
	//(QET-SQL-FORMAT-SCOPE.md 6.8)
QString author()
{
	const QString user = qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME"));
	return user.isEmpty() ? QStringLiteral("unknown") : user;
}

QString digestOf(Sqlite &file, const QString &schema)
{
	const auto rows = file.select(QStringLiteral("SELECT value FROM %1.qet_format WHERE key = 'content_digest'").arg(schema));
	return rows.isEmpty() ? QString() : rows.first().value(0).toString();
}

	//What a row of @p table is part of, for a revision's summary: a
	//symbol, a wire, a folio or a drawn item, by the uuid its key starts
	//with; empty for the rest (a terminal's key starts with its own)
QString objectKind(const QString &table)
{
	if (table == QLatin1String("link") || (table.startsWith(QLatin1String("element"))))
		return QStringLiteral("symbol");
	if (table.startsWith(QLatin1String("conductor")))
		return QStringLiteral("wire");
	if (table.startsWith(QLatin1String("diagram")) || table.startsWith(QLatin1String("folio_")))
		return QStringLiteral("folio");
	if (table == QLatin1String("drawing_item"))
		return QStringLiteral("drawn item");
	return {};
}

	//What @p changeset changes: how many symbols, wires, folios and drawn
	//items (each by the uuid its rows start their key with) and how many
	//rows of which tables. @p objects, if given, gets a line for each of
	//them: added or removed when the row of its own table is, else changed
QString changeSummary(const QByteArray &changeset, QStringList *objects = nullptr)
{
	static const QMap<QString, QString> own_table{{QStringLiteral("symbol"), QStringLiteral("element")},
												  {QStringLiteral("wire"), QStringLiteral("conductor")},
												  {QStringLiteral("folio"), QStringLiteral("diagram")},
												  {QStringLiteral("drawn item"), QStringLiteral("drawing_item")}};
	QMap<QString, int> counts;
	QMap<QString, QMap<QString, QString>> changed_objects;
	int rows = 0;
	sqlite3_changeset_iter *iter = nullptr;
	if (sqlite3changeset_start(&iter, int(changeset.size()), const_cast<char *>(changeset.constData())) == SQLITE_OK) {
		while (sqlite3changeset_next(iter) == SQLITE_ROW) {
			const char *table = nullptr;
			int columns = 0, op = 0, indirect = 0;
			sqlite3changeset_op(iter, &table, &columns, &op, &indirect);
			const QString name = QString::fromUtf8(table);
			++counts[name];
			++rows;
			const QString kind = objectKind(name);
			unsigned char *key = nullptr;
			int key_columns = 0;
			if (kind.isEmpty() || sqlite3changeset_pk(iter, &key, &key_columns) != SQLITE_OK) continue;
			for (int i = 0 ; i < key_columns ; ++i) {
				if (!key[i]) continue;
				sqlite3_value *value = nullptr;
				if ((op == SQLITE_INSERT ? sqlite3changeset_new(iter, i, &value)
										 : sqlite3changeset_old(iter, i, &value)) == SQLITE_OK && value) {
					QString &how = changed_objects[kind][QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_value_text(value)))];
					if (name == own_table.value(kind) && op != SQLITE_UPDATE)
						how = op == SQLITE_INSERT ? QStringLiteral("added") : QStringLiteral("removed");
					else if (how.isEmpty())
						how = QStringLiteral("changed");
				}
				break;
			}
		}
		sqlite3changeset_finalize(iter);
	}
	QStringList changed, tables;
	for (const QString &kind : {QStringLiteral("symbol"), QStringLiteral("wire"),
								QStringLiteral("folio"), QStringLiteral("drawn item")}) {
		const QMap<QString, QString> of_kind = changed_objects.value(kind);
		if (const int n = int(of_kind.size()))
			changed << QStringLiteral("%1 %2%3").arg(n).arg(kind, n == 1 ? QString() : QStringLiteral("s"));
		if (objects)
			for (auto it = of_kind.constBegin() ; it != of_kind.constEnd() ; ++it)
				*objects << QStringLiteral("%1 %2: %3").arg(kind, it.key(), it.value());
	}
	for (auto it = counts.constBegin() ; it != counts.constEnd() ; ++it)
		tables << QStringLiteral("%1 %2").arg(it.key()).arg(it.value());
	return QStringLiteral("%1%2 rows changed").arg(changed.isEmpty() ? QString()
					: changed.join(QStringLiteral(", ")) + QStringLiteral("; ")).arg(rows)
			+ (tables.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(tables.join(QStringLiteral(", "))));
}

	//The history @p file keeps cut to the number of saves its
	//revision_setting "keep" says, if it says one (QETDB-S6-SCOPE.md S6-6):
	//the oldest revisions dropped, the oldest kept still one to go back to
bool keepLimit(Sqlite &file)
{
	const auto rows = file.select(QStringLiteral(
		"SELECT CAST(value AS INTEGER) FROM revision_setting WHERE key = 'keep'"));
	const qint64 keep = rows.isEmpty() ? 0 : rows.first().value(0).toLongLong();
	if (keep <= 0) return true;
	return file.run(QStringLiteral("DELETE FROM revision_changes WHERE revision IN (SELECT uuid FROM revision "
								   "WHERE ord <= (SELECT max(ord) FROM revision) - ?)"), {keep})
		&& file.run(QStringLiteral("DELETE FROM revision WHERE ord <= (SELECT max(ord) FROM revision) - ?"), {keep});
}

	//The revision of this save, if the project keeps a history (the file
	//it replaces, @p previous, has one): what changed from @p previous to
	//@p saving as SQLite's session extension finds it, table by table,
	//compressed, after the history before it, all written into @p saving.
	//A table it cannot compare (a file saved before every table had a key)
	//leaves the revision without its changes, not the save undone
bool recordRevision(const QString &saving, const QString &previous, QString *error)
{
	if (!QFileInfo::exists(previous)) return true;
	Sqlite file(saving, true);
	if (!file.db() || !file.exec(QStringLiteral("ATTACH DATABASE '%1' AS prev").arg(readOnlyUri(previous)))) {
		if (error) *error = file.error();
		return false;
	}
	const auto has_table = [&file](const QString &schema, const QString &table) {
		return !file.select(QStringLiteral("SELECT 1 FROM %1.sqlite_master WHERE type = 'table' AND name = ?").arg(schema),
							{table}).isEmpty();
	};
	if (!has_table(QStringLiteral("prev"), QStringLiteral("revision"))) {
		file.exec(QStringLiteral("DETACH DATABASE prev"));
		return true;
	}
	QStringList tables, listed;
	for (const QVariantList &row : file.select(QStringLiteral(
			"SELECT value FROM main.qet_format WHERE key = 'digest_skips'")))
		listed = row.value(0).toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
	for (const QVariantList &row : file.select(QStringLiteral(
			"SELECT name FROM main.sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' "
			"ORDER BY name")))
		if (!isUnsigned(row.value(0).toString(), listed)) tables << row.value(0).toString();

	bool recorded = true;
	QByteArray changeset;
	sqlite3_session *session = nullptr;
	if (sqlite3session_create(file.db(), "main", &session) != SQLITE_OK)
		recorded = false;
	for (const QString &table : std::as_const(tables)) {
		if (!recorded) break;
		const QByteArray name = table.toUtf8();
		char *message = nullptr;
		recorded = has_table(QStringLiteral("prev"), table)
				&& sqlite3session_attach(session, name.constData()) == SQLITE_OK
				&& sqlite3session_diff(session, "prev", name.constData(), &message) == SQLITE_OK;
		sqlite3_free(message);
	}
	if (recorded) {
		int size = 0;
		void *bytes = nullptr;
		recorded = sqlite3session_changeset(session, &size, &bytes) == SQLITE_OK;
		if (recorded) changeset = QByteArray(static_cast<const char *>(bytes), size);
		sqlite3_free(bytes);
	}
	if (session) sqlite3session_delete(session);

	const auto parent = file.select(QStringLiteral("SELECT uuid, ord FROM prev.revision ORDER BY ord DESC LIMIT 1"));
	const QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
	const bool ok = file.exec(QStringLiteral("BEGIN"))
			&& file.exec(history_schema)
			&& file.exec(QStringLiteral("INSERT INTO main.revision SELECT * FROM prev.revision"))
			&& file.exec(QStringLiteral("INSERT INTO main.revision_changes SELECT * FROM prev.revision_changes"))
			&& (!has_table(QStringLiteral("prev"), QStringLiteral("revision_setting"))
				|| (file.exec(history_setting_schema)
					&& file.exec(QStringLiteral("INSERT INTO main.revision_setting SELECT * FROM prev.revision_setting"))))
			&& file.run(QStringLiteral("INSERT INTO main.revision (uuid, parent_uuid, ord, author, host, time, kind, "
									   "digest, prev_digest) VALUES (?, ?, ?, ?, ?, ?, 'save', ?, ?)"),
						{uuid, parent.isEmpty() ? QVariant() : parent.first().value(0),
						 parent.isEmpty() ? 1 : parent.first().value(1).toLongLong() + 1, author(),
						 QSysInfo::machineHostName(),
						 QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
						 digestOf(file, QStringLiteral("main")), digestOf(file, QStringLiteral("prev"))})
			&& file.run(QStringLiteral("INSERT INTO main.revision_changes (revision, changeset) VALUES (?, ?)"),
						{uuid, recorded ? QVariant(qCompress(changeset, 9)) : QVariant()})
			&& keepLimit(file)
			&& file.exec(QStringLiteral("COMMIT"));
	if (!ok) {
		if (error) *error = file.error();
		file.exec(QStringLiteral("ROLLBACK"));
	}
	file.exec(QStringLiteral("DETACH DATABASE prev"));
	return ok;
}

	//The repair data in a .qetdb (QET-SQL-FORMAT-SCOPE.md 5.8, QETDB-S4C-PLAN.md):
	//one row of qet_parity per parity page, marked by parity_magic and
	//carrying the layout, little-endian: k, m, the file's page count,
	//qet_parity's root page, the row's group, the header's change counter
	//once the rows are written, the page size. Four rows of each group also
	//carry the hashes of its pages (carriesHashes()), so that no single row
	//is one the repair depends on. As tools/qet-sqlite/parity.py writes
const QByteArray parity_magic("QETPAR3\0", 8);
constexpr int parity_percent = 10;

	//Whether parity row @p j of @p m carries its group's page hashes: four
	//spread over the rows, which are written j after j, so that one run of
	//bad pages cannot take them all (with the last data pages before them).
	//A reader takes the hashes from any row that has them
bool carriesHashes(int j, int m)
{
	return j == 0 || j == m / 3 || j == 2 * m / 3 || j == m - 1;
}

	//The 8 bytes a page is checked against
QByteArray pageHash(QByteArrayView bytes)
{
	return QCryptographicHash::hash(bytes, QCryptographicHash::Blake2b_256).left(8);
}

	//The pages of the file @p file the repair data covers, one after the
	//other: its first @p count pages (all if -1) but qet_parity's root page
	//@p root, page 1 without the header (its first 100 bytes, which each
	//write changes)
QByteArray coveredPages(const QByteArray &file, int page_size, int root, int count = -1)
{
	QByteArray pages;
	if (count < 0) count = int(file.size() / page_size);
	pages.reserve(qsizetype(count) * page_size);
	for (int p = 1 ; p <= count ; ++p) {
		if (p == root) continue;
		pages.append(file.constData() + qsizetype(p - 1) * page_size, page_size);
	}
	if (!pages.isEmpty()) std::memset(pages.data(), 0, 100);
	return pages;
}

	//pageHash() of each page of @p pages, in parallel
QByteArray pageHashes(const QByteArray &pages, int page_size)
{
	const int count = int(pages.size() / page_size);
	QByteArray hashes(qsizetype(count) * 8, '\0');
	char *out = hashes.data();
	constexpr int chunk = 256;
	QList<int> firsts;
	for (int first = 0 ; first < count ; first += chunk) firsts << first;
	QtConcurrent::blockingMap(firsts, [&pages, page_size, count, out](int first) {
		for (int i = first ; i < std::min(count, first + chunk) ; ++i)
			std::memcpy(out + qsizetype(i) * 8,
						pageHash(QByteArrayView(pages.constData() + qsizetype(i) * page_size, page_size)).constData(), 8);
	});
	return hashes;
}

	//The last write to a new file before it is checked, flushed and put in
	//place, by every way of writing one (a save, rewrite()): the repair data,
	//which covers the file as it ends. If writing it changed a page it
	//covers, the file is kept without it, and says so in the log.
	//QET_QETDB_PARITY=0 leaves it out (tests, measurements)
bool seal(const QString &path, QString *error)
{
	if (qEnvironmentVariable("QET_QETDB_PARITY") == QLatin1String("0")) return true;
	QElapsedTimer timer;
	timer.start();
	QStringList times;
	const auto lap = [&timer, &times](const char *step) {
		times << QStringLiteral("%1 %2").arg(QLatin1String(step)).arg(timer.restart());
	};
	int page_size = 0, root = 0;
		//Rows from the file it was written from are not this one's; the
		//pages they free are compacted away, so that the new rows only
		//add pages after the ones they cover
	if (!withConnection(path, error, [&page_size, &root](QSqlQuery &query) {
			if (!query.exec(QStringLiteral("PRAGMA synchronous = OFF"))
					|| !query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS qet_parity (id INTEGER PRIMARY KEY, "
												  "grp INTEGER, j INTEGER, hash BLOB, info BLOB, data BLOB)"))
					|| !query.exec(QStringLiteral("DELETE FROM qet_parity"))
					|| !query.exec(QStringLiteral("PRAGMA freelist_count")) || !query.next())
				return false;
			if (query.value(0).toInt() > 0 && !query.exec(QStringLiteral("VACUUM")))
				return false;
			if (!query.exec(QStringLiteral("PRAGMA page_size")) || !query.next()) return false;
			page_size = query.value(0).toInt();
			if (!query.exec(QStringLiteral("SELECT rootpage FROM sqlite_master WHERE type = 'table' AND name = 'qet_parity'"))
					|| !query.next())
				return false;
			root = query.value(0).toInt();
			return true;
		}))
		return false;
	lap("table");
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		if (error) *error = file.errorString();
		return false;
	}
	const QByteArray before = file.readAll();
	file.close();
	if (page_size < 512 || before.size() < 100 || before.size() % page_size) {
		if (error) *error = QObject::tr("the file could not be read back");
		return false;
	}
	const int file_pages = int(before.size() / page_size);
	const QByteArray pages = coveredPages(before, page_size, root);
	const int count = int(pages.size() / page_size);
	lap("read");
	const QByteArray hashes = pageHashes(pages, page_size);
	lap("hash");
	const QetParity::Layout layout = QetParity::layout(count, parity_percent);
	const QByteArray parity = QetParity::encode(pages, page_size, layout.k, layout.m);
	lap("encode");
		//Written in one transaction, which raises the change counter by one
	const quint32 counter = qFromBigEndian<quint32>(before.constData() + 24) + 1;
	if (!withConnection(path, error, [&](QSqlQuery &query) {
			if (!query.exec(QStringLiteral("PRAGMA synchronous = OFF")) || !query.exec(QStringLiteral("BEGIN")))
				return false;
			query.prepare(QStringLiteral("INSERT INTO qet_parity (grp, j, hash, info, data) VALUES (?, ?, ?, ?, ?)"));
				//j first: a run of bad pages in the file costs each group one row
			for (int j = 0 ; j < layout.m ; ++j) {
				for (int g = 0 ; g < layout.groups ; ++g) {
					QByteArray info = parity_magic;
					for (quint32 value : {quint32(layout.k), quint32(layout.m), quint32(file_pages), quint32(root),
										  quint32(g), counter, quint32(page_size)}) {
						const quint32 le = qToLittleEndian(value);
						info.append(reinterpret_cast<const char *>(&le), sizeof le);
					}
					if (carriesHashes(j, layout.m))
						for (int i = g ; i < count ; i += layout.groups)
							info.append(hashes.constData() + qsizetype(i) * 8, 8);
					const QByteArray shard = parity.mid((qsizetype(g) * layout.m + j) * page_size, page_size);
					query.bindValue(0, g);
					query.bindValue(1, j);
					query.bindValue(2, pageHash(shard + info));
					query.bindValue(3, info);
					query.bindValue(4, shard);
					if (!query.exec()) return false;
				}
			}
			return query.exec(QStringLiteral("COMMIT"));
		}))
		return false;
	lap("rows");
		//The counter is the one it says. Nothing it covers changed: compared
		//byte for byte only under QET_QETDB_CHECK_DIGEST=1 (tests), 76 ms of
		//f2099's save otherwise (speed first, the user, 2026-10-31)
	const bool compare = qEnvironmentVariableIntValue("QET_QETDB_CHECK_DIGEST") == 1;
	bool same = file.open(QIODevice::ReadOnly);
	if (same) {
		const QByteArray after = file.read(compare ? before.size() : 100);
		file.close();
		same = after.size() >= 100 && file.size() > before.size()
				&& qFromBigEndian<quint32>(after.constData() + 24) == counter
				&& qFromBigEndian<quint32>(after.constData() + 92) == counter
				&& (!compare || (after.size() == before.size() && coveredPages(after, page_size, root) == pages));
	}
	lap("verify");
	if (!same) {
		qWarning().noquote() << "QetDbFile: writing the repair data changed the pages it covers; "
								"the file is saved without it";
		if (!withConnection(path, error, [](QSqlQuery &query) {
				return query.exec(QStringLiteral("DELETE FROM qet_parity"));
			}))
			return false;
	}
	if (qEnvironmentVariableIntValue("QET_QETDB_SAVE_TIMES") == 1)
		qInfo().noquote() << "seal (ms):" << times.join(QStringLiteral(", "))
						  << QStringLiteral("; %1 pages, %2 groups of %3 + %4").arg(count).arg(layout.groups)
							 .arg(layout.k).arg(layout.m);
	return true;
}

	//The header QElectroTech writes, for a file whose own is destroyed:
	//SQLite refuses the whole file without one (as tools/qet-sqlite/rescue.py)
QByteArray qetdbHeader(int page_size, quint32 pages, quint32 counter)
{
	QByteArray h(100, '\0');
	char *d = h.data();
	std::memcpy(d, "SQLite format 3", 16);
	qToBigEndian<quint16>(quint16(page_size == 65536 ? 1 : page_size), d + 16);
	d[18] = 1; d[19] = 1; d[21] = 64; d[22] = 32; d[23] = 32;
	qToBigEndian<quint32>(counter, d + 24);
	qToBigEndian<quint32>(pages, d + 28);
	qToBigEndian<quint32>(1, d + 40);
	qToBigEndian<quint32>(4, d + 44);
	qToBigEndian<quint32>(1, d + 56);
	qToBigEndian<quint32>(quint32(QetDbFile::format), d + 60);
	qToBigEndian<quint32>(quint32(QetDbFile::applicationId), d + 68);
	qToBigEndian<quint32>(counter, d + 92);
	qToBigEndian<quint32>(SQLITE_VERSION_NUMBER, d + 96);
	return h;
}

	//The fields of @p data's header that every .qetdb has the same
bool headerUsable(const QByteArray &data, int page_size)
{
	if (data.size() < 100) return false;
	const QByteArray ref = qetdbHeader(page_size, 0, 0);
	for (const auto &[from, to] : {std::pair{0, 24}, {44, 48}, {56, 60}, {68, 72}})
		if (data.mid(from, to - from) != ref.mid(from, to - from)) return false;
	return true;
}

bool writeWhole(const QString &path, const QByteArray &bytes)
{
	QFile::setPermissions(path, QFile::permissions(path) | QFile::WriteOwner);
	QFile file(path);
	return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size();
}

	//SQLite's variable-length integer at @p at, before @p end
bool readVarint(const uchar *bytes, qsizetype &at, qsizetype end, quint64 &value)
{
	value = 0;
	for (int i = 0 ; i < 9 ; ++i) {
		if (at >= end) return false;
		const uchar b = bytes[at++];
		if (i == 8) {
			value = (value << 8) | b;
			return true;
		}
		value = (value << 7) | (b & 0x7f);
		if (!(b & 0x80)) return true;
	}
	return true;
}

	//The rows of qet_parity, found by reading every page of @p data as
	//SQLite lays out a table's rows (https://sqlite.org/fileformat2.html):
	//leaf pages, their cells, each row's spill into overflow pages, its
	//record. For a file SQLite cannot read them from; what is not one of
	//them is left to @p take, which checks each row's own hash
void scanParityRows(const QByteArray &data, int page_size,
					const std::function<void(int, int, const QByteArray &, const QByteArray &, const QByteArray &)> &take)
{
	const int pages = int(data.size() / page_size);
	const int usable = page_size;  //No reserved bytes in a page of a .qetdb
	const auto *bytes = reinterpret_cast<const uchar *>(data.constData());
	const auto be16 = [bytes](qsizetype at) { return int(qFromBigEndian<quint16>(bytes + at)); };
	const auto be32 = [bytes](qsizetype at) { return qFromBigEndian<quint32>(bytes + at); };
		//The largest row: its layout and every page hash of a group
	const quint64 largest = quint64(page_size) + 36 + 8 * 255 + 64;
	for (int p = 0 ; p < pages ; ++p) {
		const qsizetype base = qsizetype(p) * page_size, end = base + page_size;
		const qsizetype head = base + (p == 0 ? 100 : 0);
		if (bytes[head] != 0x0d) continue;  //A table's leaf page
		const int cells = be16(head + 3);
		if (cells < 1 || head + 8 + 2 * qsizetype(cells) > end) continue;
		for (int c = 0 ; c < cells ; ++c) {
			qsizetype at = base + be16(head + 8 + 2 * c);
			quint64 size = 0, rowid = 0;
			if (at < head + 8 || !readVarint(bytes, at, end, size) || !readVarint(bytes, at, end, rowid)
					|| size <= quint64(page_size) || size > largest)
				continue;
				//The part of the row on this page, as SQLite works it out
			const qsizetype most = usable - 35, least = (qsizetype(usable - 12) * 32 / 255) - 23;
			qsizetype local = least + qsizetype((size - quint64(least)) % quint64(usable - 4));
			if (local > most) local = least;
			if (at + local + 4 > end) continue;
			QByteArray payload(reinterpret_cast<const char *>(bytes + at), local);
			quint32 next = be32(at + local);
				//A page number from a damaged page can be anything: unsigned
				//throughout, so that a huge one is not taken for a small one
			for (int hops = 0 ; quint64(payload.size()) < size && next >= 1 && next <= quint32(pages) && hops < 8 ; ++hops) {
				const qsizetype o = qsizetype(next - 1) * page_size;
				next = be32(o);
				payload.append(reinterpret_cast<const char *>(bytes + o + 4),
							   qsizetype(std::min<quint64>(quint64(usable - 4), size - quint64(payload.size()))));
			}
			if (quint64(payload.size()) != size) continue;
				//The record: id (the rowid, stored as null), grp, j, hash, info, data
			const auto *rec = reinterpret_cast<const uchar *>(payload.constData());
			qsizetype h = 0;
			quint64 header_size = 0;
			if (!readVarint(rec, h, payload.size(), header_size) || header_size > quint64(payload.size())) continue;
			quint64 types[6];
			bool ok = true;
			for (quint64 &type : types) ok = ok && readVarint(rec, h, qsizetype(header_size), type);
			if (!ok || h != qsizetype(header_size) || types[0] != 0) continue;
			qsizetype v = qsizetype(header_size);
			const auto integer = [rec, &v, &payload](quint64 type, int &out) {
				static const int widths[] = {0, 1, 2, 3, 4, 6, 8};
				if (type == 8 || type == 9) {
					out = int(type - 8);
					return true;
				}
				if (type < 1 || type > 6 || v + widths[type] > payload.size()) return false;
				qint64 value = qint8(rec[v]);
				for (int i = 1 ; i < widths[type] ; ++i) value = (value << 8) | rec[v + i];
				v += widths[type];
				out = int(value);
				return true;
			};
			const auto blob = [&payload, &v](quint64 type, QByteArray &out) {
				if (type < 12 || type % 2) return false;
				const qsizetype length = qsizetype((type - 12) / 2);
				if (v + length > payload.size()) return false;
				out = payload.mid(v, length);
				v += length;
				return true;
			};
			int grp = 0, j = 0;
			QByteArray hash, info, shard;
			if (integer(types[1], grp) && integer(types[2], j) && blob(types[3], hash) && blob(types[4], info)
					&& blob(types[5], shard))
				take(grp, j, hash, info, shard);
		}
	}
}

	//The pages of the .qetdb @p copy that are not what its save wrote,
	//rebuilt in place from the repair data it carries (QETDB-S4C-PLAN.md
	//P3), before anything else reads it. A file another program wrote to
	//since (@p path's header or a -wal/-journal beside it says so) is left
	//as it is: its new pages are not damage. Unless @p back_to_save: then
	//its pages are rebuilt as saved too, as the user chose (P5)
	//@p dry: what is wrong found, nothing written or rebuilt (an open's
	//check beside its read)
QetDbFile::Repair repairPages(const QString &copy, const QString &path, bool back_to_save = false, bool dry = false)
{
	QetDbFile::Repair r;
	QFile file(copy);
	if (!file.open(QIODevice::ReadOnly)) return r;
	const QByteArray original = file.readAll();
	file.close();
	int page_size = 4096;
	if (original.size() >= 18) {
		const int field = qFromBigEndian<quint16>(original.constData() + 16);
		const int size = field == 1 ? 65536 : field;
		if (size >= 512 && size <= 65536 && !(size & (size - 1))) page_size = size;
	}
		//A destroyed header first: SQLite reads nothing, the repair data
		//included, without one
	QByteArray data = original;
	const bool header_bad = !headerUsable(data, page_size);
	if (header_bad && dry) {
		r.header = true;
		return r;
	}
	if (header_bad) {
		data = qetdbHeader(page_size, quint32(data.size() / page_size), 1) + data.mid(100);
		if (!writeWhole(copy, data)) return r;
	}

	int k = 0, m = 0, file_pages = 0, root = 0;
	quint32 counter = 0;
	QHash<std::pair<int, int>, QByteArray> shards;
	QHash<int, QByteArray> group_hashes;
		//Each row checks itself: a damaged one is left out
	const auto take = [&](int grp, int j, const QByteArray &hash, const QByteArray &info, const QByteArray &shard) {
		if (info.size() < 36 || !info.startsWith(parity_magic) || pageHash(shard + info) != hash)
			return;
		const auto at = [&info](int i) { return qFromLittleEndian<quint32>(info.constData() + 8 + 4 * i); };
		const int g = int(at(4));
		if (g != grp || int(at(6)) != page_size || shard.size() != page_size || j < 0 || int(at(1)) <= j)
			return;
		if (!k) {
				//A layout that can be: nothing read from a file is trusted
				//to bound what is read or written after
			if (at(0) < 1 || at(1) < 1 || at(0) + at(1) > 255 || at(2) < 1
					|| at(2) > quint32(std::numeric_limits<int>::max() / page_size) || at(3) < 2 || at(3) > at(2))
				return;
			k = int(at(0)); m = int(at(1)); file_pages = int(at(2)); root = int(at(3)); counter = at(5);
		}
		if (int(at(0)) != k || int(at(1)) != m) return;
		if (info.size() > 36 && !group_hashes.contains(g)) group_hashes.insert(g, info.mid(36));
		shards.insert({g, j}, shard);
	};
	const QString name = QStringLiteral("qetdb_repair_") + QUuid::createUuid().toString();
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
		db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
		db.setDatabaseName(copy);
		if (db.open()) {
			QSqlQuery query(db);
			query.setForwardOnly(true);
				//Those after an error part way through the table are left out too
			if (query.exec(QStringLiteral("SELECT grp, j, hash, info, data FROM qet_parity")))
				while (query.next())
					take(query.value(0).toInt(), query.value(1).toInt(), query.value(2).toByteArray(),
						 query.value(3).toByteArray(), query.value(4).toByteArray());
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(name);
		//SQLite cannot read them all (its schema page lost, or one of the
		//table's own pages, which stops the read part way): the rest found
		//by reading every page as SQLite lays rows out (P4)
	const qsizetype expected = k ? qsizetype((file_pages - 1 + k - 1) / k) * m : 0;
	if (!k || shards.size() < expected) {
		const qsizetype before = shards.size();
		scanParityRows(data, page_size, take);
		r.scanned = shards.size() > before;
	}
	if (!k || !m || file_pages < 1) {
		if (header_bad) writeWhole(copy, original);
		return r;
	}
	r.parity = true;

		//Damage, or another program's write? A write through SQLite raises
		//the header's change counter and its copy at byte 92 together;
		//damage moves neither, or only one (QET-SQL-FORMAT-SCOPE.md 5.8)
	for (const char *suffix : {"-wal", "-journal"})
		r.edited = r.edited || QFileInfo::exists(path + QLatin1String(suffix));
	if (!r.edited && original.size() >= 100 && original.startsWith(QByteArray("SQLite format 3\0", 16))) {
		const quint32 c = qFromBigEndian<quint32>(original.constData() + 24);
		r.edited = c == qFromBigEndian<quint32>(original.constData() + 92) && c != counter;
	}
	if (r.edited && !back_to_save) {
		if (header_bad) writeWhole(copy, original);
		return r;
	}
	r.header = header_bad;

		//A file cut short: its lost end is lost pages
	if (data.size() < qsizetype(file_pages) * page_size)
		data.append(QByteArray(qsizetype(file_pages) * page_size - data.size(), '\0'));
	const QByteArray pages = coveredPages(data, page_size, root, file_pages);
	const int count = int(pages.size() / page_size);
	const int groups = (count + k - 1) / k;
	const QByteArray hashes = pageHashes(pages, page_size);
		//A page is checked against its group's hashes; a group whose rows
		//carrying them are all lost cannot be checked
	QList<int> bad;
	for (int i = 0 ; i < count ; ++i) {
		const QByteArray group = group_hashes.value(i % groups);
		const int at = (i / groups) * 8;
		if (group.size() >= at + 8 && QByteArrayView(group).sliced(at, 8) != QByteArrayView(hashes).sliced(qsizetype(i) * 8, 8))
			bad << i;
	}
	r.bad_pages = int(bad.size());
		//Nothing to rebuild, and the repair data whole: the copy is the file
		//(SQLite could not read every row: the table's own pages are damaged,
		//or the file is shorter than its header says)
	const bool parity_damaged = shards.size() < expected || r.scanned
			|| original.size() < qsizetype(qFromBigEndian<quint32>(data.constData() + 28)) * page_size;
	if (dry || (bad.isEmpty() && !header_bad && !parity_damaged && !r.edited)) {
		r.parity_damaged = dry && parity_damaged;
		return r;
	}
	r.parity_damaged = parity_damaged;
	if (!bad.isEmpty()) {
		QList<std::pair<int, int>> lost_parity;
		QByteArray parity(qsizetype(groups) * m * page_size, '\0');
		for (int g = 0 ; g < groups ; ++g)
			for (int j = 0 ; j < m ; ++j) {
				const auto it = shards.constFind({g, j});
				if (it == shards.constEnd()) lost_parity << std::pair{g, j};
				else parity.replace((qsizetype(g) * m + j) * page_size, page_size, *it);
			}
		QByteArray rebuilt = pages;
		r.beyond = QetParity::decode(rebuilt, page_size, parity, k, m, bad, lost_parity);
			//Beyond repair: the copy as it came, not half rebuilt (after a
			//VACUUM by another program, every page moved, that is the
			//saved content still)
		if (r.beyond) {
			if (r.beyond < 0) r.beyond = groups;
			writeWhole(copy, original);
			return r;
		}
			//Back in their places; page 1's header stays as it is now
		for (int i = 0, p = 1 ; i < count ; ++p) {
			if (p == root) continue;
			const int skip = p == 1 ? 100 : 0;
			data.replace(qsizetype(p - 1) * page_size + skip, page_size - skip,
						 rebuilt.constData() + qsizetype(i) * page_size + skip, page_size - skip);
			++i;
		}
	}
		//The file as its save had it just before the repair rows went in:
		//the pages they cover, and qet_parity an empty table. Whatever
		//happened to the rows' own pages, or to the pages another program
		//added (going back to the save), is then gone
	data.truncate(qsizetype(file_pages) * page_size);
	char *d = data.data();
	const qsizetype at = qsizetype(root - 1) * page_size;
	std::memset(d + at, 0, size_t(page_size));
	d[at] = 0x0d;  //A table's leaf page, without rows
	qToBigEndian<quint16>(quint16(page_size == 65536 ? 0 : page_size), d + at + 5);
	qToBigEndian<quint32>(quint32(file_pages), d + 28);
	std::memset(d + 32, 0, 8);  //No free page
	std::memcpy(d + 92, d + 24, 4);  //The page count above holds
	if (!writeWhole(copy, data)) {
		r.beyond = groups;
		return r;
	}
	r.back_to_save = r.edited && !r.beyond;
	return r;
}

	//@p to written as @p from with @p edit made to it, as a save writes a
	//file: a new one in its place, checked and on disk before it replaces
	//anything, so that a previous version sharing @p to's bytes is not
	//changed. @p refinish: its content changed, so it is normalized and its
	//digest written again (finish(); @p edit drops qet_format)
bool rewrite(const QString &from, const QString &to, const std::function<bool(Sqlite &, QString *)> &edit,
			 bool refinish, QString *error)
{
	const QString saving = to + QStringLiteral(".saving");
	QFile::remove(saving);
	if (!QFile::copy(from, saving)) {
		if (error) *error = QObject::tr("the file could not be written");
		return false;
	}
	QFile::setPermissions(saving, QFile::permissions(saving) | QFile::WriteOwner);
	bool ok = check(saving, error);
	if (ok) {
		Sqlite file(saving, true);
		ok = edit(file, error);
	}
	ok = ok && (!refinish || finish(saving, error)) && seal(saving, error) && check(saving, error) && flushFile(saving)
			&& replace(saving, to, error);
	if (!ok) QFile::remove(saving);
	return ok;
}

	//@p file as it was at its revision @p ord: the changes of every save
	//after it undone, newest first, and those revisions dropped. Fails if
	//one of them has no changes recorded or does not undo cleanly
bool windBack(Sqlite &file, qint64 ord, QString *error)
{
	if (file.select(QStringLiteral("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'revision'")).isEmpty()
			|| file.select(QStringLiteral("SELECT 1 FROM revision WHERE ord = ?"), {ord}).isEmpty()) {
		if (error) *error = QObject::tr("it keeps no revision %1").arg(ord);
		return false;
	}
	const auto fail = [&file, error](const QString &why) {
		if (error) *error = why;
		file.exec(QStringLiteral("ROLLBACK"));
		return false;
	};
	if (!file.exec(QStringLiteral("BEGIN"))) return fail(file.error());
	for (const QVariantList &row : file.select(QStringLiteral(
			"SELECT r.ord, c.changeset FROM revision r LEFT JOIN revision_changes c ON c.revision = r.uuid "
			"WHERE r.ord > ? ORDER BY r.ord DESC"), {ord})) {
		if (row.value(1).isNull())
			return fail(QObject::tr("the changes of revision %1 were not recorded").arg(row.value(0).toString()));
		QByteArray changeset = qUncompress(row.value(1).toByteArray());
		int size = 0;
		void *inverse = nullptr;
		if (sqlite3changeset_invert(int(changeset.size()), changeset.data(), &size, &inverse) != SQLITE_OK)
			return fail(QObject::tr("revision %1 cannot be undone").arg(row.value(0).toString()));
		int conflicts = 0;
		const int applied = sqlite3changeset_apply(
				file.db(), size, inverse, nullptr,
				[](void *context, int, sqlite3_changeset_iter *) {
					++*static_cast<int *>(context);
					return SQLITE_CHANGESET_ABORT;
				}, &conflicts);
		sqlite3_free(inverse);
		if (applied != SQLITE_OK || conflicts)
			return fail(QObject::tr("revision %1 does not undo cleanly").arg(row.value(0).toString()));
	}
	if (!file.run(QStringLiteral("DELETE FROM revision_changes WHERE revision IN "
								 "(SELECT uuid FROM revision WHERE ord > ?)"), {ord})
			|| !file.run(QStringLiteral("DELETE FROM revision WHERE ord > ?"), {ord})
			|| !file.exec(QStringLiteral("DROP TABLE qet_format"))
			|| !file.exec(QStringLiteral("COMMIT")))
		return fail(file.error());
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
bool QetDbFile::save(projectDataBase &database, const QString &path, QString *error, QByteArray *sha1,
					 const QString &previous)
{
	const QString saving = path + QStringLiteral(".saving");
	QFuture<QByteArray> hashing;
	QFuture<bool> early_flush;
	const auto give_up = [&saving, &hashing, &early_flush]() {
			//Not removed while it is read (Windows refuses)
		if (hashing.isValid()) hashing.waitForFinished();
		if (early_flush.isValid()) early_flush.waitForFinished();
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
	QHash<QString, QByteArray> digests;
	if (!database.copyTo(saving, error) || !lap("copy"))
		return give_up();
		//Most of its pages, written by the copy, on their way to disk while
		//the rest is done: the flush before it replaces anything then has
		//only the last changes to write (110 ms of f2099's save otherwise)
	early_flush = QtConcurrent::run([saving]() { return flushFile(saving); });
	if (!finish(saving, error, database.unchangedDigests(), &digests) || !lap("finish")
			|| !recordRevision(saving, previous.isEmpty() ? path : previous, error) || !lap("revision")
			|| !seal(saving, error) || !lap("seal"))
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
	QFuture<bool> flushing = QtConcurrent::run([saving, early_flush]() mutable {
		early_flush.waitForFinished();
		return flushFile(saving);
	});
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
	database.savedDigests(digests);
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
static bool readCopy(const QString &path, QDomDocument *document, QString *error,
					 const std::function<bool(const QString &copy)> &with_copy, QetDbFile::Repair *repair,
					 QetDbFile::OutsideEdit outside, bool pages_first, bool *again);

bool QetDbFile::read(const QString &path, QDomDocument *document, QString *error,
					 const std::function<bool(const QString &copy)> &with_copy, Repair *repair,
					 OutsideEdit outside)
{
		//Speed first (the user, 2026-10-31): the pages are checked against
		//the repair data beside the read, not before it (111 ms of f2099's
		//open); a file that does not open as saved, or whose pages are not
		//as saved, is read again with them repaired first. Another
		//program's journal beside it, or going back to its save, first
	bool again = false;
	const bool pages_first = outside == OutsideEdit::BackToSave || QFileInfo::exists(path + QLatin1String("-wal"))
			|| QFileInfo::exists(path + QLatin1String("-journal"));
	const bool ok = readCopy(path, document, error, with_copy, repair, outside, pages_first, &again);
	if (!again && (ok || pages_first)) return ok;
	if (document) *document = QDomDocument();
	if (error) error->clear();
	return readCopy(path, document, error, with_copy, repair, outside, true, nullptr);
}

	//QetDbFile::read(), with the pages checked first (@p pages_first), or
	//@p again set where a file that does not open as saved should be read
	//again with them checked first
static bool readCopy(const QString &path, QDomDocument *document, QString *error,
					 const std::function<bool(const QString &copy)> &with_copy, QetDbFile::Repair *repair,
					 QetDbFile::OutsideEdit outside, bool pages_first, bool *again)
{
	using QetDbFile::Repair;
	using QetDbFile::OutsideEdit;
	QTemporaryDir dir;
	const QString copy = dir.filePath(QStringLiteral("project.qetdb"));
	if (!dir.isValid() || !QFile::copy(path, copy)) {
		if (error) *error = QObject::tr("the file could not be read");
		return false;
	}
		//Bad pages rebuilt in the copy first, from the repair data the file
		//carries; the file itself is written by the next save only
	QElapsedTimer timer;
	timer.start();
	Repair repaired = pages_first ? repairPages(copy, path, outside == OutsideEdit::BackToSave) : Repair();
	if (repair) *repair = repaired;
	if (repaired.back_to_save)
		qInfo().noquote() << "QetDbFile:" << QFileInfo(path).fileName() << "was changed by another program;"
						  << repaired.bad_pages << "pages rebuilt as saved, as the user chose";
	else if (repaired.repaired())
		qInfo().noquote() << "QetDbFile:" << QFileInfo(path).fileName() << "was damaged;"
						  << repaired.bad_pages << "pages repaired" << (repaired.header ? "and its header" : "")
						  << (repaired.parity_damaged ? "(its repair data damaged too, left out until the next save)" : "")
						  << (repaired.scanned ? "(its repair data read from its pages)" : "");
	else if (repaired.beyond)
		qWarning().noquote() << "QetDbFile:" << QFileInfo(path).fileName() << "has" << repaired.bad_pages
							 << "damaged pages, more in" << repaired.beyond << "groups than its repair data covers";
	if (pages_first && qEnvironmentVariableIntValue("QET_QETDB_SAVE_TIMES") == 1)
		qInfo().noquote() << "QetDbFile::read: pages checked in" << timer.elapsed() << "ms";
	if (!check(copy, error)) {
		if (again) {
			*again = true;
			return false;
		}
		if (repaired.beyond && error)
			*error = QObject::tr("it has %n damaged page(s), more than the repair data it keeps can rebuild",
								 nullptr, repaired.bad_pages);
		return false;
	}
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
		//A damaged index passes SQLite's quick check and the digest: the
		//pages checked too, on a thread of their own, nothing written
	QFuture<Repair> pages;
	if (again)
		pages = QtConcurrent::run([copy, path]() {
			return repairPages(copy, path, false, true);
		});
	const auto damaged = [&pages, &digest, again]() {
		if (!pages.isValid()) return false;
		const Repair found = pages.result();
		*again = !found.edited && (found.header || found.bad_pages || found.parity_damaged);
			//The copy is not removed while it is read
		if (*again && digest.isValid()) digest.waitForFinished();
		return *again;
	};
		//The checked copy, for the project database to start from (S4x X2):
		//if it did, the folios' items are left in it until each is built
		//(S4x X3)
	const bool deferred = with_copy && with_copy(copy);
	const bool read = projectDataBase::readDocument(copy, document, error, deferred);
	if (damaged()) return false;
	if (!saved_digest.isEmpty() && digest.result() != saved_digest) {
			//It passed every other check: the user may open it as it is,
			//rather than be locked out of it (the user, 2026-10-30)
		if (outside == OutsideEdit::KeepChanges) {
			repaired.kept_changes = true;
			if (repair) *repair = repaired;
			qWarning().noquote() << "QetDbFile:" << QFileInfo(path).fileName()
								 << "opened with content that is not what QElectroTech saved, as the user chose;"
									" its content check was skipped";
			return read;
		}
		if (again) {
			*again = true;
			return false;
		}
		repaired.content_differs = true;
		if (repair) *repair = repaired;
		if (error) *error = QObject::tr("its content is not what QElectroTech saved: it is damaged, "
										"or another program changed it");
		return false;
	}
	return read;
}

/**
	@brief QetDbFile::repair
	Write to @p output the .qetdb @p path with its damaged pages rebuilt from
	the repair data it carries (QETDB-S4C-PLAN.md P5, --qetdb-repair), and
	new repair data: written beside @p output, checked as an open checks a
	file, then put in place. @p path is never written. A file another
	program wrote to is repaired only with @p back_to_save.
	@return false, with @p error saying why, if no sound file came of it;
	@p report says what was found either way
*/
bool QetDbFile::repair(const QString &path, const QString &output, Repair *report, QString *error,
					   bool back_to_save)
{
	const QString saving = output + QStringLiteral(".saving");
	QFile::remove(saving);
	if (!QFile::copy(path, saving)) {
		if (error) *error = QObject::tr("the file could not be read");
		return false;
	}
	QFile::setPermissions(saving, QFile::permissions(saving) | QFile::WriteOwner);
	Repair found = repairPages(saving, path, back_to_save);
	bool ok = check(saving, error);
	if (ok) {
		QString saved_digest;
		withConnection(saving, nullptr, [&saved_digest](QSqlQuery &query) {
			if (query.exec(QStringLiteral("SELECT value FROM qet_format WHERE key = 'content_digest'"))
					&& query.next())
				saved_digest = query.value(0).toString();
			return true;
		});
		if (!saved_digest.isEmpty() && contentDigest(saving) != saved_digest) {
			found.content_differs = true;
			ok = false;
			if (error) *error = QObject::tr("its content is not what QElectroTech saved: it is damaged, "
											"or another program changed it");
		}
	}
	ok = ok && seal(saving, error) && check(saving, error) && flushFile(saving) && replace(saving, output, error);
	if (!ok) QFile::remove(saving);
	if (report) *report = found;
	return ok;
}

/**
	@brief QetDbFile::setHistory
	Make the .qetdb @p path keep a history of its saves (@p on), from its
	content now (an "import" revision), or drop the history it keeps: a
	new file in its place, as a save writes, so that a previous version
	sharing its bytes is not changed (QETDB-S6-SCOPE.md).
	@return false, with @p error saying why, if it could not
*/
bool QetDbFile::setHistory(const QString &path, bool on, QString *error)
{
	return rewrite(path, path, [on](Sqlite &file, QString *error) {
		bool ok;
		if (on) {
			ok = file.exec(history_schema);
			if (ok && file.select(QStringLiteral("SELECT 1 FROM revision LIMIT 1")).isEmpty())
				ok = file.run(QStringLiteral("INSERT INTO revision (uuid, ord, author, host, time, kind, digest) "
											 "VALUES (?, 1, ?, ?, ?, 'import', ?)"),
							  {QUuid::createUuid().toString(QUuid::WithoutBraces), author(),
							   QSysInfo::machineHostName(), QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
							   digestOf(file, QStringLiteral("main"))});
		} else {
				//Gone from the file, not only from its tables: the names
				//in it travel with every copy
			ok = file.exec(QStringLiteral("DROP TABLE IF EXISTS revision"))
					&& file.exec(QStringLiteral("DROP TABLE IF EXISTS revision_changes"))
					&& file.exec(QStringLiteral("DROP TABLE IF EXISTS revision_setting"))
					&& file.exec(QStringLiteral("VACUUM"));
		}
		if (!ok && error) *error = file.error();
		return ok;
	}, false, error);
}

/**
	@brief QetDbFile::exportDelivery
	Write the .qetdb @p path to @p output without the history it keeps:
	what to send a client, who should get the drawings, not who changed
	them and when (QET-SQL-FORMAT-SCOPE.md 6.5)
*/
bool QetDbFile::exportDelivery(const QString &path, const QString &output, QString *error)
{
	return rewrite(path, output, [](Sqlite &file, QString *error) {
		const bool ok = file.exec(QStringLiteral("DROP TABLE IF EXISTS revision"))
				&& file.exec(QStringLiteral("DROP TABLE IF EXISTS revision_changes"))
				&& file.exec(QStringLiteral("DROP TABLE IF EXISTS revision_setting"))
				&& file.exec(QStringLiteral("VACUUM"));
		if (!ok && error) *error = file.error();
		return ok;
	}, false, error);
}

/**
	@brief QetDbFile::setHistoryLimit
	Make the history the .qetdb @p path keeps hold only its last @p saves
	revisions, cut now and at every save after; 0 keeps them all
	(QETDB-S6-SCOPE.md S6-6). At least 2, so that one is left to go back
	to. A new file in its place, as a save writes.
*/
bool QetDbFile::setHistoryLimit(const QString &path, int saves, QString *error)
{
	if (saves == 1 || saves < 0) {
		if (error) *error = QObject::tr("keep at least 2 saves, or 0 for all");
		return false;
	}
	return rewrite(path, path, [saves](Sqlite &file, QString *error) {
		if (file.select(QStringLiteral(
				"SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'revision'")).isEmpty()) {
			if (error) *error = QObject::tr("it keeps no history");
			return false;
		}
		const bool ok = file.exec(QStringLiteral("BEGIN"))
				&& file.exec(history_setting_schema)
				&& file.run(QStringLiteral("INSERT OR REPLACE INTO revision_setting (key, value) VALUES ('keep', ?)"),
							{qlonglong(saves)})
				&& keepLimit(file)
				&& file.exec(QStringLiteral("COMMIT"))
				&& file.exec(QStringLiteral("VACUUM"));
		if (!ok) {
			if (error) *error = file.error();
			file.exec(QStringLiteral("ROLLBACK"));
		}
		return ok;
	}, false, error);
}

/**
	@return how many saves the history of the .qetdb @p path is limited to,
	0 if it keeps them all or keeps none
*/
int QetDbFile::historyLimit(const QString &path)
{
	Sqlite file(path, false);
	if (file.select(QStringLiteral(
			"SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'revision_setting'")).isEmpty())
		return 0;
	const auto rows = file.select(QStringLiteral(
		"SELECT CAST(value AS INTEGER) FROM revision_setting WHERE key = 'keep'"));
	return rows.isEmpty() ? 0 : int(rows.first().value(0).toLongLong());
}

/**
	@brief QetDbFile::writeRevision
	Write to @p output the project the .qetdb @p path holds as it was at
	its revision @p ord, with its history up to it: the saves after it
	undone from their recorded changes, newest first.
	@p same_digest, if given, says whether its content digest is the one
	that revision was saved with (undone rows come back at the end of
	their table, so a table's row order may differ)
*/
bool QetDbFile::writeRevision(const QString &path, qint64 ord, const QString &output, QString *error,
							  bool *same_digest)
{
	QString digest;
	{
		Sqlite file(path, false);
		const auto rows = file.select(QStringLiteral("SELECT digest FROM revision WHERE ord = ?"), {ord});
		if (!rows.isEmpty()) digest = rows.first().value(0).toString();
	}
	if (!rewrite(path, output, [ord](Sqlite &file, QString *error) { return windBack(file, ord, error); },
				 true, error))
		return false;
	if (same_digest) {
		Sqlite file(output, false);
		*same_digest = !digest.isEmpty() && digestOf(file, QStringLiteral("main")) == digest;
	}
	return true;
}

/**
	@brief QetDbFile::history
	The revisions the .qetdb @p path keeps, oldest first, one line each:
	its number, when, who and where, its kind, and for a save how many
	rows of which tables it changed. Empty with @p error if it keeps none.
*/
QStringList QetDbFile::history(const QString &path, QString *error)
{
	Sqlite file(path, false);
	QStringList lines;
	if (!file.db() || file.select(QStringLiteral(
			"SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'revision'")).isEmpty()) {
		if (error) *error = file.db() ? QObject::tr("it keeps no history") : file.error();
		return lines;
	}
	for (const QVariantList &row : file.select(QStringLiteral(
			"SELECT r.ord, r.time, r.author, r.host, r.kind, c.changeset, c.revision IS NOT NULL "
			"FROM revision r LEFT JOIN revision_changes c ON c.revision = r.uuid ORDER BY r.ord"))) {
		QString line = QStringLiteral("%1  %2  %3@%4  %5").arg(row.value(0).toString(), row.value(1).toString(),
																row.value(2).toString(), row.value(3).toString(),
																row.value(4).toString());
		if (row.value(4).toString() == QLatin1String("save")) {
			if (row.value(5).isNull())
				line += QStringLiteral(": changes not recorded");
			else
				line += QStringLiteral(": ") + changeSummary(qUncompress(row.value(5).toByteArray()));
		}
		lines << line;
	}
	return lines;
}

/**
	@brief QetDbFile::historyDiff
	What changed in the .qetdb @p path from its revision @p from to its
	revision @p to: the changes of every save between them made into one,
	a line saying how many symbols, wires, folios and drawn items and how
	many rows, then a line for each of them (QETDB-S6-SCOPE.md S6-3).
	Empty with @p error if it keeps no such revisions or one's changes
	were not recorded.
*/
QStringList QetDbFile::historyDiff(const QString &path, qint64 from, qint64 to, QString *error)
{
	Sqlite file(path, false);
	if (!file.db() || file.select(QStringLiteral(
			"SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'revision'")).isEmpty()) {
		if (error) *error = file.db() ? QObject::tr("it keeps no history") : file.error();
		return {};
	}
	if (from >= to) {
		if (error) *error = QObject::tr("revision %1 does not come before revision %2").arg(from).arg(to);
		return {};
	}
	for (const qint64 ord : {from, to})
		if (file.select(QStringLiteral("SELECT 1 FROM revision WHERE ord = ?"), {ord}).isEmpty()) {
			if (error) *error = QObject::tr("it keeps no revision %1").arg(ord);
			return {};
		}
	QByteArray changes;
	for (const QVariantList &row : file.select(QStringLiteral(
			"SELECT r.ord, c.changeset FROM revision r LEFT JOIN revision_changes c ON c.revision = r.uuid "
			"WHERE r.ord > ? AND r.ord <= ? ORDER BY r.ord"), {from, to})) {
		if (row.value(1).isNull()) {
			if (error) *error = QObject::tr("the changes of revision %1 were not recorded").arg(row.value(0).toString());
			return {};
		}
		QByteArray next = qUncompress(row.value(1).toByteArray());
		if (changes.isEmpty()) {
			changes = next;
			continue;
		}
		if (next.isEmpty()) continue;
		int size = 0;
		void *both = nullptr;
		if (sqlite3changeset_concat(int(changes.size()), changes.data(), int(next.size()), next.data(),
									&size, &both) != SQLITE_OK) {
			sqlite3_free(both);
			if (error) *error = QObject::tr("the changes of revision %1 do not follow on").arg(row.value(0).toString());
			return {};
		}
		changes = QByteArray(static_cast<const char *>(both), size);
		sqlite3_free(both);
	}
	QStringList objects;
	const QString summary = changeSummary(changes, &objects);
	return QStringList{QStringLiteral("%1 to %2: %3").arg(from).arg(to).arg(summary)} + objects;
}

/**
	@brief QetDbFile::lastChange
	Who last changed the symbol, wire or folio @p uuid in the .qetdb
	@p path: the newest save whose recorded changes name it, as
	"ord  time  who@host  added|changed" (QETDB-S6-SCOPE.md S6-5). Empty if
	none of the saves it keeps did, or it keeps no history (@p error then
	says so); a save whose changes were not recorded is passed over.
*/
QString QetDbFile::lastChange(const QString &path, const QString &uuid, QString *error)
{
	Sqlite file(path, false);
	if (!file.db() || file.select(QStringLiteral(
			"SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'revision'")).isEmpty()) {
		if (error) *error = file.db() ? QObject::tr("it keeps no history") : file.error();
		return {};
	}
		//changeSummary() names objects by the text of their key
	const QString suffix = QLatin1Char(' ') + uuid + QStringLiteral(": ");
	for (const QVariantList &row : file.select(QStringLiteral(
			"SELECT r.ord, r.time, r.author, r.host, c.changeset FROM revision r "
			"JOIN revision_changes c ON c.revision = r.uuid "
			"WHERE r.kind = 'save' AND c.changeset IS NOT NULL ORDER BY r.ord DESC"))) {
		QStringList objects;
		changeSummary(qUncompress(row.value(4).toByteArray()), &objects);
		for (const QString &object : std::as_const(objects)) {
			const int at = object.indexOf(suffix);
			if (at < 0) continue;
			return QStringLiteral("%1  %2  %3@%4  %5").arg(row.value(0).toString(), row.value(1).toString(),
														   row.value(2).toString(), row.value(3).toString(),
														   object.mid(at + suffix.size()));
		}
	}
	return {};
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
