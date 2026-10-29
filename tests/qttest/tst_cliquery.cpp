// SPDX-License-Identifier: GPL-2.0-or-later
// --query (QETDB-S10, Q1 + Q2): one read-only query over several projects,
// through the public qet_ views; their columns pinned; writes refused; the
// same rows from a project's .qet, .qetz and .qetdb. Runs the real binary.

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

class tst_cliquery : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	struct Result {
		int code = -1;
		QString out, err;
		QStringList lines() const {return out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);}
	};

	Result run(const QStringList &args, const QStringList &extra_env = {})
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		for (const QString &pair : extra_env)
			env.insert(pair.section(QLatin1Char('='), 0, 0), pair.section(QLatin1Char('='), 1));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   QStringList{QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")} + args);
		Result result;
		if (!proc.waitForFinished(300000)) return result;
		result.code = proc.exitCode();
		result.out = QString::fromUtf8(proc.readAllStandardOutput());
		result.err = QString::fromUtf8(proc.readAllStandardError());
		return result;
	}

	static QString example(const QString &name)
	{
		return QStringLiteral(QET_EXAMPLES_DIR "/") + name;
	}

	// The rows of @p result without the project column, sorted
	static QStringList rowsOnly(const Result &result)
	{
		QStringList rows = result.lines().mid(1);
		for (QString &row : rows) row = row.section(QLatin1Char(','), 1);
		rows.sort();
		return rows;
	}

	// industrial saved as a .qetdb, once
	QString industrialDb()
	{
		const QString path = m_dir.filePath(QStringLiteral("direct.qetdb"));
		if (!QFile::exists(path))
			run({QStringLiteral("--resave"), example(QStringLiteral("industrial.qet")), path});
		return path;
	}

	static QByteArray digest(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly)
				? QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256) : QByteArray();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// One query, two projects: one header, then every folio of each, the
	// project it came from first
	void twoProjects()
	{
		const QString a = example(QStringLiteral("industrial.qet"));
		const QString b = example(QStringLiteral("grafcet.qet"));
		const Result r = run({QStringLiteral("--query"),
							  QStringLiteral("SELECT folio_number, title FROM qet_folios"), a, b});
		QVERIFY2(r.code == 0, qPrintable(r.err.right(600)));
		const QStringList lines = r.lines();
		QCOMPARE(lines.value(0), QStringLiteral("project,folio_number,title"));
		const int from_a = lines.filter(QRegularExpression(QStringLiteral("^") + QRegularExpression::escape(a) + QStringLiteral(","))).size();
		const int from_b = lines.filter(QRegularExpression(QStringLiteral("^") + QRegularExpression::escape(b) + QStringLiteral(","))).size();
		QCOMPARE(from_a, 50);
		QVERIFY(from_b > 0);
		QCOMPARE(lines.size(), 1 + from_a + from_b);
	}

	// The same as JSON: an array of objects, numbers as numbers
	void json()
	{
		const Result r = run({QStringLiteral("--query"),
							  QStringLiteral("SELECT folio_number, title FROM qet_folios WHERE folio_number = 4"),
							  example(QStringLiteral("industrial.qet")), QStringLiteral("--json")});
		QVERIFY2(r.code == 0, qPrintable(r.err.right(600)));
		const QJsonArray rows = QJsonDocument::fromJson(r.out.toUtf8()).array();
		QCOMPARE(rows.size(), 1);
		const QJsonObject row = rows.at(0).toObject();
		QCOMPARE(row.value(QStringLiteral("project")).toString(), example(QStringLiteral("industrial.qet")));
		QVERIFY(row.value(QStringLiteral("folio_number")).isDouble());
		QCOMPARE(row.value(QStringLiteral("folio_number")).toInt(), 4);
		QCOMPARE(row.value(QStringLiteral("title")).toString(), QStringLiteral("Mains Power Supply"));
	}

	// Each public view's columns, in order: renaming or removing one fails
	// here. A column is only ever added (at the end).
	void viewColumns_data()
	{
		QTest::addColumn<QString>("view");
		QTest::addColumn<QString>("columns");
		QTest::newRow("qet_folios") << QStringLiteral("qet_folios")
			<< QStringLiteral("folio_number,folio,title,author,plant,location,revision,date,filename,folio_uuid");
		QTest::newRow("qet_devices") << QStringLiteral("qet_devices")
			<< QStringLiteral("folio_number,folio,position,label,plant,location,kind,sub_kind,function,"
							  "description,designation,manufacturer,article,supplier,quantity,comment,"
							  "symbol,device_uuid,folio_uuid");
		QTest::newRow("qet_wires") << QStringLiteral("qet_wires")
			<< QStringLiteral("folio_number,folio,wire_number,from_device,from_terminal,to_device,"
							  "to_terminal,colour,section,cable,bus,function,voltage,wire_uuid,"
							  "from_device_uuid,to_device_uuid");
		QTest::newRow("qet_links") << QStringLiteral("qet_links")
			<< QStringLiteral("link_kind,device,folio_number,folio,position,linked_device,"
							  "linked_folio_number,linked_folio,linked_position,device_uuid,linked_uuid");
	}

	void viewColumns()
	{
		QFETCH(QString, view);
		QFETCH(QString, columns);
		const Result r = run({QStringLiteral("--query"), QStringLiteral("SELECT * FROM %1 LIMIT 0").arg(view),
							  example(QStringLiteral("industrial.qet"))});
		QVERIFY2(r.code == 0, qPrintable(r.err.right(600)));
		QCOMPARE(r.lines().value(0), QStringLiteral("project,") + columns);
	}

	// Each view gives a row for each row of the table it lists: no wire,
	// device or folio lost to a join
	void viewsLoseNothing_data()
	{
		QTest::addColumn<QString>("view");
		QTest::addColumn<QString>("table");
		QTest::newRow("folios") << QStringLiteral("qet_folios") << QStringLiteral("diagram");
		QTest::newRow("devices") << QStringLiteral("qet_devices") << QStringLiteral("element");
		QTest::newRow("wires") << QStringLiteral("qet_wires") << QStringLiteral("conductor");
	}

	void viewsLoseNothing()
	{
		QFETCH(QString, view);
		QFETCH(QString, table);
		const Result r = run({QStringLiteral("--query"),
							  QStringLiteral("SELECT (SELECT count(*) FROM %1), (SELECT count(*) FROM %2)").arg(view, table),
							  example(QStringLiteral("industrial.qet"))});
		QVERIFY2(r.code == 0, qPrintable(r.err.right(600)));
		const QStringList counts = r.lines().value(1).split(QLatin1Char(','));
		QVERIFY(counts.value(2).toInt() > 0);
		QCOMPARE(counts.value(1), counts.value(2));
	}

	// Writing is refused, with the reason, and the exit code says so
	void writesRefused_data()
	{
		QTest::addColumn<QString>("sql");
		QTest::newRow("cte delete") << QStringLiteral("WITH x AS (SELECT 1) DELETE FROM element");
		QTest::newRow("attach") << QStringLiteral("ATTACH DATABASE 'other.db' AS other");
		QTest::newRow("pragma") << QStringLiteral("PRAGMA writable_schema = ON");
		QTest::newRow("two statements") << QStringLiteral("SELECT 1; DROP TABLE element");
	}

	void writesRefused()
	{
		QFETCH(QString, sql);
			//Through the project, and on a .qetdb read itself
		for (const QString &project : {example(QStringLiteral("industrial.qet")), industrialDb()}) {
			const QByteArray before = digest(project);
			const Result r = run({QStringLiteral("--query"), sql, project});
			QCOMPARE(r.code, 1);
			QVERIFY2(r.err.contains(project + QStringLiteral(": ")), qPrintable(r.err.right(600)));
			QVERIFY(r.out.trimmed().isEmpty());
			QCOMPARE(digest(project), before);
		}
	}

	// A file that is missing is reported; the others still give their rows
	void missingFileReported()
	{
		const QString missing = m_dir.filePath(QStringLiteral("missing.qet"));
		const Result r = run({QStringLiteral("--query"), QStringLiteral("SELECT title FROM qet_folios"),
							  missing, example(QStringLiteral("grafcet.qet"))});
		QCOMPARE(r.code, 1);
		QVERIFY(r.err.contains(missing));
		QVERIFY(r.lines().size() > 1);
	}

	// A .qetdb read itself gives the rows of the project opened from it,
	// and the file is not changed
	void directEqualsOpened()
	{
		const QString path = industrialDb();
		const QByteArray before = digest(path);
		QVERIFY(!before.isEmpty());
		const QString sql = QStringLiteral("SELECT folio_number, position, label, kind FROM qet_devices");
		const Result direct = run({QStringLiteral("--query"), sql, path});
		const Result opened = run({QStringLiteral("--query"), sql, path}, {QStringLiteral("QET_QUERY_OPEN_PROJECT=1")});
		QVERIFY2(direct.code == 0 && opened.code == 0, qPrintable(direct.err + opened.err));
		QVERIFY(rowsOnly(direct).size() > 100);
		QCOMPARE(rowsOnly(direct), rowsOnly(opened));
		QCOMPARE(digest(path), before);
	}

	// A .qetdb saved before the views existed answers the same: they are
	// made for the query, not read from the file
	void fileWithoutViews()
	{
		const QString path = m_dir.filePath(QStringLiteral("noviews.qetdb"));
		QVERIFY(QFile::copy(industrialDb(), path));
		QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("drop"));
			db.setDatabaseName(path);
			QVERIFY(db.open());
			QSqlQuery query(db);
			for (const char *view : {"qet_folios", "qet_devices", "qet_wires", "qet_links"})
				QVERIFY(query.exec(QStringLiteral("DROP VIEW IF EXISTS %1").arg(QLatin1String(view))));
			db.close();
		}
		QSqlDatabase::removeDatabase(QStringLiteral("drop"));
		const QString sql = QStringLiteral("SELECT wire_number, from_device, to_device FROM qet_wires");
		const Result a = run({QStringLiteral("--query"), sql, path});
		const Result b = run({QStringLiteral("--query"), sql, industrialDb()});
		QVERIFY2(a.code == 0, qPrintable(a.err.right(600)));
		QVERIFY(rowsOnly(a).size() > 100);
		QCOMPARE(rowsOnly(a), rowsOnly(b));
	}

	// A damaged file is refused with its name, the others still answer
	void damagedRefused()
	{
		const QString bad = m_dir.filePath(QStringLiteral("damaged.qetdb"));
		QFile f(bad);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(QByteArray("SQLite format 3") + QByteArray(4096, 'x'));
		f.close();
		const Result r = run({QStringLiteral("--query"), QStringLiteral("SELECT title FROM qet_folios"),
							  bad, industrialDb()});
		QCOMPARE(r.code, 1);
		QVERIFY2(r.err.contains(bad + QStringLiteral(": ")), qPrintable(r.err.right(600)));
		QCOMPARE(r.lines().size(), 1 + 50);
	}

	// A project's .qet, .qetz and .qetdb give the same rows
	void sameRowsEveryFile()
	{
		const QString qet = example(QStringLiteral("industrial.qet"));
		const QString qetz = m_dir.filePath(QStringLiteral("industrial.qetz"));
		const QString qetdb = m_dir.filePath(QStringLiteral("industrial.qetdb"));
		QCOMPARE(run({QStringLiteral("--resave"), qet, qetz}).code, 0);
		QCOMPARE(run({QStringLiteral("--resave"), qet, qetdb}).code, 0);
		const QStringList queries{
			QStringLiteral("SELECT folio_number, folio, title FROM qet_folios"),
			QStringLiteral("SELECT folio_number, position, label, kind, article FROM qet_devices"),
			QStringLiteral("SELECT folio_number, wire_number, from_device, to_device, colour FROM qet_wires"),
			QStringLiteral("SELECT link_kind, device, folio_number, linked_folio_number FROM qet_links")};
		for (const QString &sql : queries) {
			const Result a = run({QStringLiteral("--query"), sql, qet});
			const Result b = run({QStringLiteral("--query"), sql, qetz});
			const Result c = run({QStringLiteral("--query"), sql, qetdb});
			QVERIFY2(a.code == 0 && b.code == 0 && c.code == 0, qPrintable(sql));
			QVERIFY2(rowsOnly(a).size() > 0, qPrintable(sql));
			QVERIFY2(rowsOnly(a) == rowsOnly(b), qPrintable(sql));
			QVERIFY2(rowsOnly(a) == rowsOnly(c), qPrintable(sql));
		}
	}
};

QTEST_MAIN(tst_cliquery)
#include "tst_cliquery.moc"
