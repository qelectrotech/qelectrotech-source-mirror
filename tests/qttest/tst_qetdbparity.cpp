// SPDX-License-Identifier: GPL-2.0-or-later
// The repair data a .qetdb keeps, against the damage of
// tools/qet-sqlite/parity.py (QETDB-S4C-PLAN.md P6): through --qetdb-repair,
// each kind of damage the repair data covers comes back exactly; beyond it,
// the file is refused, never written wrong, and nothing crashes. Another
// program's changes are not taken for damage, and are undone on request.
#include <QtTest>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "qettesttimeout.h"

class tst_qetdbparity : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;
	static constexpr int page = 4096;

	QString run(const QStringList &args, int *code)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   QStringList{QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")} + args);
		*code = -1;
		if (!proc.waitForFinished(QetTest::timeout(300000))) return QStringLiteral("(timed out)");
		if (proc.exitStatus() == QProcess::NormalExit) *code = proc.exitCode();
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	QString saved(const QString &project)
	{
		return m_dir.filePath(project + QStringLiteral(".qetdb"));
	}

	static QByteArray bytesOf(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	static bool write(const QString &path, const QByteArray &bytes)
	{
		QFile::remove(path);
		QFile f(path);
		return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
	}

	static bool exec(const QString &path, const QStringList &sql)
	{
		bool ok = true;
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("edit"));
			db.setDatabaseName(path);
			ok = db.open();
			QSqlQuery query(db);
			for (const QString &statement : sql) ok = ok && query.exec(statement);
			db.close();
		}
		QSqlDatabase::removeDatabase(QStringLiteral("edit"));
		return ok;
	}

		//The tables of @p other that differ from @p path's, row for row
		//(the repair data and what the file says of itself left out)
	static QStringList differences(const QString &path, const QString &other)
	{
		QStringList differ;
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("compare"));
			db.setDatabaseName(path);
			db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
			if (!db.open()) return {QStringLiteral("(cannot open)")};
			QSqlQuery query(db);
			if (!query.exec(QStringLiteral("ATTACH DATABASE '%1' AS other").arg(other)))
				differ << QStringLiteral("(cannot attach)");
			QStringList tables;
			query.exec(QStringLiteral("SELECT name FROM main.sqlite_master WHERE type = 'table' "
									  "AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' "
									  "AND name NOT IN ('qet_parity', 'qet_format')"));
			while (query.next()) tables << query.value(0).toString();
			for (const QString &t : std::as_const(tables)) {
				const bool same = query.exec(QStringLiteral(
						"SELECT (SELECT count(*) FROM main.\"%1\") = (SELECT count(*) FROM other.\"%1\") "
						"AND NOT EXISTS (SELECT * FROM main.\"%1\" EXCEPT SELECT * FROM other.\"%1\")").arg(t))
						&& query.next() && query.value(0).toBool();
				if (!same) differ << t;
			}
			db.close();
		}
		QSqlDatabase::removeDatabase(QStringLiteral("compare"));
		return differ;
	}

		//parity.py's damage (and rescue.py's), seeded by @p seed
	static QByteArray damaged(QByteArray b, const QString &mode, quint32 seed)
	{
		QRandomGenerator rng(seed);
		const int pages = int(b.size() / page);
		const auto noise = [&rng](qsizetype size) {
			QByteArray r(size, Qt::Uninitialized);
			for (char &c : r) c = char(rng.bounded(256));
			return r;
		};
		const auto pick = [&rng, pages]() { return int(rng.bounded(1, pages)); };
		if (mode == QLatin1String("zero1")) {
			b.replace(qsizetype(pick()) * page, page, QByteArray(page, '\0'));
		} else if (mode == QLatin1String("noise8")) {
			const int p = int(rng.bounded(1, std::max(2, pages - 8)));
			b.replace(qsizetype(p) * page, 8 * page, noise(8 * page));
		} else if (mode == QLatin1String("bits")) {
			for (int i = 0 ; i < 32 ; ++i) {
				const qsizetype at = qsizetype(rng.bounded(qint64(page), qint64(b.size())));
				b[at] = char(b.at(at) ^ (1 << rng.bounded(8)));
			}
		} else if (mode == QLatin1String("header")) {
			b.replace(0, 100, noise(100));
		} else if (mode == QLatin1String("page1")) {
			b.replace(0, page, QByteArray(page, '\0'));
		} else if (mode == QLatin1String("trunc")) {
			b.truncate(qsizetype(rng.bounded(qint64(page), qint64(b.size()))));
		} else if (mode.startsWith(QLatin1String("scatter"))) {
			const int n = std::max(1, pages * mode.mid(7).toInt() / 100);
			QList<int> all;
			for (int p = 0 ; p < pages ; ++p) all << p;
			std::shuffle(all.begin(), all.end(), rng);
			for (int i = 0 ; i < n ; ++i) b.replace(qsizetype(all.at(i)) * page, page, noise(page));
		} else if (mode.startsWith(QLatin1String("burst"))) {
			const int n = std::max(1, pages * mode.mid(5).toInt() / 100);
			const int p = int(rng.bounded(0, pages - n));
			b.replace(qsizetype(p) * page, qsizetype(n) * page, noise(qsizetype(n) * page));
		} else if (mode.startsWith(QLatin1String("tail"))) {
			b.truncate(qsizetype(pages - std::max(1, pages * mode.mid(4).toInt() / 100)) * page);
		}
		return b;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		for (const QString &project : {QStringLiteral("industrial"),
									   QStringLiteral("weneedpolonez-Polonez_MR89_wiring_diagram")}) {
			int code = -1;
			const QString log = run({QStringLiteral("--resave"),
									 QStringLiteral(QET_EXAMPLES_DIR "/") + project + QStringLiteral(".qet"),
									 saved(project)}, &code);
			QVERIFY2(code == 0 && QFile::exists(saved(project)), qPrintable(log.right(800)));
		}
	}

		//The kinds of damage: those the repair data covers must come back
		//exactly; the others may be refused, never repaired wrong
	void damage_data()
	{
		QTest::addColumn<QString>("project");
		QTest::addColumn<QString>("mode");
		QTest::addColumn<bool>("covered");
		const QStringList covered{QStringLiteral("zero1"), QStringLiteral("noise8"), QStringLiteral("bits"),
								  QStringLiteral("header"), QStringLiteral("page1"), QStringLiteral("scatter1"),
								  QStringLiteral("scatter3"), QStringLiteral("burst5"), QStringLiteral("tail1")};
		const QStringList beyond{QStringLiteral("scatter5"), QStringLiteral("scatter8"), QStringLiteral("burst10"),
								 QStringLiteral("trunc")};
		for (const QString &project : {QStringLiteral("industrial"),
									   QStringLiteral("weneedpolonez-Polonez_MR89_wiring_diagram")})
			for (int seed = 0 ; seed < 2 ; ++seed) {
				for (const QString &mode : covered)
					QTest::addRow("%s %s %d", qPrintable(project.left(14)), qPrintable(mode), seed)
							<< project << mode << true;
				for (const QString &mode : beyond)
					QTest::addRow("%s %s %d", qPrintable(project.left(14)), qPrintable(mode), seed)
							<< project << mode << false;
			}
	}

	void damage()
	{
		QFETCH(QString, project);
		QFETCH(QString, mode);
		QFETCH(bool, covered);
		const QByteArray good = bytesOf(saved(project));
		QVERIFY(!good.isEmpty());
		const QByteArray bad = damaged(good, mode, quint32(qHash(QString(QTest::currentDataTag()))));
		const QString path = m_dir.filePath(QStringLiteral("bad.qetdb"));
		const QString out = m_dir.filePath(QStringLiteral("repaired.qetdb"));
		QFile::remove(out);
		QVERIFY(write(path, bad));
		int code = -1;
		const QString log = run({QStringLiteral("--qetdb-repair"), path, out}, &code);
		QVERIFY2(code == 0 || code == 1, qPrintable(QStringLiteral("exit %1: %2").arg(code).arg(log.right(800))));
		QVERIFY2(bytesOf(path) == bad, "the damaged file was written");
		if (covered)
			QVERIFY2(code == 0, qPrintable(log.right(800)));
		if (code == 0)
			QCOMPARE(differences(saved(project), out), QStringList());
		else
			QVERIFY(!QFile::exists(out));
	}

		//Another program's writes, through SQLite: not damage. Without
		//--back-to-save, exit 3 if the content changed (a VACUUM changes no
		//content); with it, the saved state, or what VACUUM left, which
		//holds the same
	void outsideEdits_data()
	{
		QTest::addColumn<QString>("project");
		QTest::addColumn<QString>("edit");
		for (const QString &project : {QStringLiteral("industrial"),
									   QStringLiteral("weneedpolonez-Polonez_MR89_wiring_diagram")})
			for (const char *edit : {"value", "vacuum", "wal", "newtable"})
				QTest::addRow("%s %s", qPrintable(project.left(14)), edit) << project << QString::fromLatin1(edit);
	}

	void outsideEdits()
	{
		QFETCH(QString, project);
		QFETCH(QString, edit);
		const QString path = m_dir.filePath(QStringLiteral("edited.qetdb"));
		QFile::remove(path);
		QVERIFY(QFile::copy(saved(project), path));
		QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
		QStringList sql;
		if (edit == QLatin1String("value"))
			sql << QStringLiteral("UPDATE element_info SET label = coalesce(label, '') || ' ' "
								  "WHERE rowid = (SELECT min(rowid) FROM element_info)");
		else if (edit == QLatin1String("vacuum"))
			sql << QStringLiteral("VACUUM");
		else if (edit == QLatin1String("wal"))
			sql << QStringLiteral("PRAGMA journal_mode = WAL")
				<< QStringLiteral("UPDATE element_info SET label = coalesce(label, '') || ' ' "
								  "WHERE rowid = (SELECT min(rowid) FROM element_info)");
		else
			sql << QStringLiteral("CREATE TABLE notes (x TEXT)")
				<< QStringLiteral("INSERT INTO notes VALUES ('checked by the electrician')");
		QVERIFY(exec(path, sql));
		const QByteArray edited = bytesOf(path);
		const bool content_changed = edit != QLatin1String("vacuum");

		const QString out = m_dir.filePath(QStringLiteral("edited-out.qetdb"));
		QFile::remove(out);
		int code = -1;
		QString log = run({QStringLiteral("--qetdb-repair"), path, out}, &code);
		QVERIFY2(code == (content_changed ? 3 : 0), qPrintable(QStringLiteral("exit %1: %2").arg(code).arg(log.right(800))));
		QVERIFY(!log.contains(QStringLiteral("(repaired)")));

		QFile::remove(out);
		log = run({QStringLiteral("--qetdb-repair"), path, out, QStringLiteral("--back-to-save")}, &code);
		QVERIFY2(code == 0, qPrintable(QStringLiteral("exit %1: %2").arg(code).arg(log.right(800))));
		QCOMPARE(differences(saved(project), out), QStringList());
		QVERIFY2(bytesOf(path) == edited, "the edited file was written");
	}
};

QTEST_GUILESS_MAIN(tst_qetdbparity)
#include "tst_qetdbparity.moc"
