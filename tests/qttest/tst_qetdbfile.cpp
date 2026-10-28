// SPDX-License-Identifier: GPL-2.0-or-later
// A project saved as one SQLite file, .qetdb (experimental, QET-SQL-FORMAT
// S4a): what the file is, that it gives back the project, that a file this
// QElectroTech cannot read is refused, and that a failed save leaves the
// previous file. Runs the real binary.

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

class tst_qetdbfile : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QString run(const QStringList &args, bool eager = false, int *code = nullptr)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   QStringList{QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")} + args);
		if (!proc.waitForFinished(300000)) return {};
		if (code) *code = proc.exitCode();
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	QString resave(const QString &in, const QString &out, int *code = nullptr)
	{
		return run({QStringLiteral("--resave"), in, out}, false, code);
	}

	// @p path saved again with every folio built, as bytes
	QByteArray built(const QString &path)
	{
		const QString out = path + QStringLiteral(".built.qet");
		run({QStringLiteral("--resave"), path, out}, true);
		QFile f(out);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	static QVariant pragma(const QString &path, const QString &sql)
	{
		QVariant value;
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("look"));
			db.setDatabaseName(path);
			if (db.open()) {
				QSqlQuery query(db);
				if (query.exec(sql) && query.next()) value = query.value(0);
			}
			db.close();
		}
		QSqlDatabase::removeDatabase(QStringLiteral("look"));
		return value;
	}

	static bool exec(const QString &path, const QString &sql)
	{
		bool ok = false;
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("change"));
			db.setDatabaseName(path);
			ok = db.open() && QSqlQuery(db).exec(sql);
			db.close();
		}
		QSqlDatabase::removeDatabase(QStringLiteral("change"));
		return ok;
	}

	QString industrial()
	{
		const QString path = m_dir.filePath(QStringLiteral("industrial.qetdb"));
		if (!QFile::exists(path))
			resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), path);
		return path;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// What the file is: ours, its format, at rest with nothing beside it,
	// without the database's own indexes or the working copy's row
	void fileIsOneDatabase()
	{
		const QString path = industrial();
		QVERIFY(QFile::exists(path));
		QCOMPARE(pragma(path, QStringLiteral("PRAGMA application_id")).toInt(), 0x51455444);
		QCOMPARE(pragma(path, QStringLiteral("PRAGMA user_version")).toInt(), 1);
		QCOMPARE(pragma(path, QStringLiteral("PRAGMA journal_mode")).toString(), QStringLiteral("delete"));
		QCOMPARE(pragma(path, QStringLiteral("SELECT value FROM qet_format WHERE key = 'format'")).toString(),
				 QStringLiteral("1"));
		QCOMPARE(pragma(path, QStringLiteral("SELECT count(*) FROM sqlite_master WHERE name LIKE 'idx_%'")).toInt(), 0);
		QCOMPARE(pragma(path, QStringLiteral("SELECT count(*) FROM sqlite_master WHERE name = 'working_copy'")).toInt(), 0);
		const QStringList beside = QDir(m_dir.path()).entryList({QStringLiteral("industrial.qetdb*")}, QDir::Files);
		QCOMPARE(beside, QStringList{QStringLiteral("industrial.qetdb")});
	}

	// Read back, it is the project: saved again as a .qet, the same bytes
	// as the project saved as a .qet
	void givesBackTheProject()
	{
		const QString back = m_dir.filePath(QStringLiteral("back.qet"));
		const QString plain = m_dir.filePath(QStringLiteral("plain.qet"));
		resave(industrial(), back);
		resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), plain);
		const QByteArray a = built(back), b = built(plain);
		QVERIFY(a.size() > 100000);
		QVERIFY(a == b);
	}

	// An edit saved through the project's own save is in the file
	void editIsSaved()
	{
#ifndef QET_HAS_SCRIPTING
		QSKIP("needs --run: this QElectroTech is built without Qt Qml");
#endif
		const QString path = m_dir.filePath(QStringLiteral("edited.qetdb"));
		QVERIFY(QFile::copy(industrial(), path));
		QFile js(m_dir.filePath(QStringLiteral("edit.js")));
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write("var u = qet.elementUuids(0)[0];\n"
				 "qet.log('LABEL ' + qet.setElementLabel(0, u, 'QETDB1'));\n"
				 "qet.log('SAVED ' + qet.save(''));\n");
		js.close();
		const QString log = run({QStringLiteral("--run"), js.fileName(), path});
		QVERIFY2(log.contains(QStringLiteral("LABEL true")) && log.contains(QStringLiteral("SAVED true")),
				 qPrintable(log.right(800)));
		const QString back = m_dir.filePath(QStringLiteral("edited.qet"));
		resave(path, back);
		QFile f(back);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QVERIFY(f.readAll().contains("QETDB1"));
	}

	// A file from a newer QElectroTech, or a damaged one, is refused
	void unreadableRefused()
	{
		const QString newer = m_dir.filePath(QStringLiteral("newer.qetdb"));
		QVERIFY(QFile::copy(industrial(), newer));
		QFile::setPermissions(newer, QFile::ReadOwner | QFile::WriteOwner);
		QVERIFY(exec(newer, QStringLiteral("UPDATE qet_format SET value = '2' WHERE key = 'min_reader'")));
		int code = 0;
		QString log = resave(newer, m_dir.filePath(QStringLiteral("newer.qet")), &code);
		QVERIFY2(code != 0 && log.contains(QStringLiteral("newer QElectroTech")), qPrintable(log.right(800)));

		const QString damaged = m_dir.filePath(QStringLiteral("damaged.qetdb"));
		QVERIFY(QFile::copy(industrial(), damaged));
		QFile::setPermissions(damaged, QFile::ReadOwner | QFile::WriteOwner);
		QFile f(damaged);
		QVERIFY(f.open(QIODevice::ReadWrite));
		QVERIFY(f.seek(f.size() / 2));
		f.write(QByteArray(8192, '\xa5'));
		f.close();
		log = resave(damaged, m_dir.filePath(QStringLiteral("damaged.qet")), &code);
		QVERIFY2(code != 0, qPrintable(log.right(800)));
		QVERIFY(!QFile::exists(m_dir.filePath(QStringLiteral("damaged.qet"))));
	}

	// A save that cannot finish leaves the previous file as it was
	void failedSaveKeepsTheFile()
	{
		const QString folder = m_dir.filePath(QStringLiteral("readonly"));
		QVERIFY(QDir().mkpath(folder));
		const QString path = folder + QStringLiteral("/kept.qetdb");
		QVERIFY(QFile::copy(industrial(), path));
		QFile before(path);
		QVERIFY(before.open(QIODevice::ReadOnly));
		const QByteArray bytes = before.readAll();
		before.close();
		QFile::setPermissions(folder, QFile::ReadOwner | QFile::ExeOwner);
			//Root writes into a folder it made read-only (CI runs as root)
		QFile probe(folder + QStringLiteral("/probe"));
		if (probe.open(QIODevice::WriteOnly)) {
			probe.close();
			probe.remove();
			QFile::setPermissions(folder, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
			QSKIP("a read-only folder is still writable here (running as root)");
		}
		int code = 0;
		const QString log = resave(QStringLiteral(QET_EXAMPLES_DIR "/grafcet.qet"), path, &code);
		QFile::setPermissions(folder, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
		QVERIFY2(code != 0, qPrintable(log.right(800)));
		QFile after(path);
		QVERIFY(after.open(QIODevice::ReadOnly));
		QVERIFY(after.readAll() == bytes);
		QCOMPARE(QDir(folder).entryList(QDir::Files), QStringList{QStringLiteral("kept.qetdb")});
	}
};

QTEST_GUILESS_MAIN(tst_qetdbfile)
#include "tst_qetdbfile.moc"
