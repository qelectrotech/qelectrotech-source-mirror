// SPDX-License-Identifier: GPL-2.0-or-later
// Which working copies a QElectroTech that did not close normally left are
// offered at the next start (QET-SQL-FORMAT S3z3): those whose lock is
// free, ready, with edits the project's file lacks. The others are deleted,
// except one a running QElectroTech holds.

#include "../../sources/dataBase/workingcopyrecovery.h"

#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class tst_workingcopyrecovery : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QString path(const QString &name) const {return m_dir.filePath(name + QStringLiteral(".sqlite"));}

	/// A working copy as projectDataBase leaves it, its row as given
	/// (no row at all when @p ready is negative)
	void make(const QString &name, const QString &project_file, const QString &committed,
			  int modified, int ready)
	{
		const QString connection = QStringLiteral("make_") + name;
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
			db.setDatabaseName(path(name));
			QVERIFY(db.open());
			QSqlQuery query(db);
			QVERIFY(query.exec(QStringLiteral("PRAGMA journal_mode = WAL")));
			QVERIFY(query.exec(QStringLiteral("CREATE TABLE element (uuid TEXT)")));
			if (ready >= 0) {
				QVERIFY(query.exec(QStringLiteral("CREATE TABLE working_copy (project_file TEXT, version TEXT, "
												  "opened TEXT, committed TEXT, modified INTEGER, ready INTEGER)")));
				query.prepare(QStringLiteral("INSERT INTO working_copy VALUES (:file, '0.100', :committed, "
											 ":committed, :modified, :ready)"));
				query.bindValue(QStringLiteral(":file"), project_file);
				query.bindValue(QStringLiteral(":committed"), committed);
				query.bindValue(QStringLiteral(":modified"), modified);
				query.bindValue(QStringLiteral(":ready"), ready);
				QVERIFY(query.exec());
			}
			db.close();
		}
		QSqlDatabase::removeDatabase(connection);
		QFile wal(path(name) + QStringLiteral("-wal"));
		QVERIFY(wal.open(QIODevice::WriteOnly));
	}

	/// The lock a QElectroTech killed while it held it leaves behind
	void deadLock(const QString &name)
	{
		const QString lock_path = m_dir.filePath(name + QStringLiteral(".lock"));
		qint64 pid;
		QString host, application;
		{
				//The host as QLockFile names it: on Windows, not as QSysInfo does
			QLockFile own(lock_path);
			QVERIFY(own.tryLock(0));
			QVERIFY(own.getLockInfo(&pid, &host, &application));
		}
		QFile lock(lock_path);
		QVERIFY(lock.open(QIODevice::WriteOnly));
			//A process id no process has (Linux's highest is 2^22)
		lock.write(QByteArray("99999999\n") + application.toUtf8() + "\n" + host.toUtf8() + "\n");
	}

	bool gone(const QString &name) const
	{
		return !QFile::exists(path(name)) && !QFile::exists(path(name) + QStringLiteral("-wal"))
				&& !QFile::exists(m_dir.filePath(name + QStringLiteral(".lock")));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void leftWorkingCopies()
	{
		make(QStringLiteral("older"), QStringLiteral("/p/a.qet"), QStringLiteral("2026-10-27T08:00:00.000Z"), 1, 1);
		make(QStringLiteral("newer"), QString(), QStringLiteral("2026-10-27T09:00:00.000Z"), 1, 1);
		deadLock(QStringLiteral("newer"));
		make(QStringLiteral("saved"), QStringLiteral("/p/b.qet"), QStringLiteral("2026-10-27T10:00:00.000Z"), 0, 1);
		make(QStringLiteral("opening"), QStringLiteral("/p/c.qet"), QString(), 1, 0);
		make(QStringLiteral("norow"), QString(), QString(), 0, -1);
		QFile damaged(path(QStringLiteral("damaged")));
		QVERIFY(damaged.open(QIODevice::WriteOnly));
		damaged.write("not a database");
		damaged.close();
		make(QStringLiteral("open"), QStringLiteral("/p/d.qet"), QStringLiteral("2026-10-27T11:00:00.000Z"), 1, 1);
		QLockFile held(m_dir.filePath(QStringLiteral("open.lock")));
		QVERIFY(held.tryLock(0));

		const auto left = WorkingCopyRecovery::leftWorkingCopies(m_dir.path());

		QCOMPARE(left.size(), 2);
		QCOMPARE(left.at(0).path, path(QStringLiteral("newer")));
		QCOMPARE(left.at(0).project_file, QString());
		QCOMPARE(left.at(0).committed, QDateTime(QDate(2026, 10, 27), QTime(9, 0), QTimeZone::UTC));
		QCOMPARE(left.at(1).path, path(QStringLiteral("older")));
		QCOMPARE(left.at(1).project_file, QStringLiteral("/p/a.qet"));
			//Kept for the dialog, their lock released
		for (const auto &copy : left) {
			QVERIFY(QFile::exists(copy.path));
		}
		QVERIFY(!QFile::exists(m_dir.filePath(QStringLiteral("newer.lock"))));
			//Nothing to recover
		for (const char *name : {"saved", "opening", "norow", "damaged"}) {
			QVERIFY2(gone(QLatin1String(name)), name);
		}
			//Its QElectroTech still runs
		QVERIFY(QFile::exists(path(QStringLiteral("open"))));
		QVERIFY(held.isLocked());
	}

	void remove()
	{
		make(QStringLiteral("removed"), QStringLiteral("/p/e.qet"), QStringLiteral("2026-10-27T12:00:00.000Z"), 1, 1);
		deadLock(QStringLiteral("removed"));
		WorkingCopyRecovery::remove(path(QStringLiteral("removed")));
		QVERIFY(gone(QStringLiteral("removed")));
	}
};

QTEST_GUILESS_MAIN(tst_workingcopyrecovery)
#include "tst_workingcopyrecovery.moc"
