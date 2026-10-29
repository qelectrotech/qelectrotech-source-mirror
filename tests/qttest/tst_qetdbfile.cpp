// SPDX-License-Identifier: GPL-2.0-or-later
// A project saved as one SQLite file, .qetdb (experimental, QET-SQL-FORMAT
// S4a): what the file is, that it gives back the project, that a file this
// QElectroTech cannot read is refused, and that a failed save leaves the
// previous file. Runs the real binary.

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSet>
#include <QProcessEnvironment>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

class tst_qetdbfile : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QString run(const QStringList &args, bool eager = false, int *code = nullptr,
				const QStringList &extra_env = {})
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
		for (const QString &pair : extra_env)
			env.insert(pair.section(QLatin1Char('='), 0, 0), pair.section(QLatin1Char('='), 1));
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
	// with the database's own indexes, without the working copy's row
	void fileIsOneDatabase()
	{
		const QString path = industrial();
		QVERIFY(QFile::exists(path));
		QCOMPARE(pragma(path, QStringLiteral("PRAGMA application_id")).toInt(), 0x51455444);
		QCOMPARE(pragma(path, QStringLiteral("PRAGMA user_version")).toInt(), 1);
		QCOMPARE(pragma(path, QStringLiteral("PRAGMA journal_mode")).toString(), QStringLiteral("delete"));
		QCOMPARE(pragma(path, QStringLiteral("SELECT value FROM qet_format WHERE key = 'format'")).toString(),
				 QStringLiteral("1"));
			//The public views other tools read it through (QETDB-S10)
		QCOMPARE(pragma(path, QStringLiteral("SELECT value FROM qet_format WHERE key = 'views'")).toString(),
				 QStringLiteral("1"));
		QCOMPARE(pragma(path, QStringLiteral("SELECT count(*) FROM sqlite_master WHERE type = 'view' "
											 "AND name IN ('qet_folios', 'qet_devices', 'qet_wires', 'qet_links')")).toInt(), 4);
			//The indexes stay: speed matters more than size
		QVERIFY(pragma(path, QStringLiteral("SELECT count(*) FROM sqlite_master WHERE name = 'idx_conductor_diagram'")).toInt() == 1);
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

	// Opened from its own tables, a .qetdb gives the project database that
	// filling it from the document read out of it gives: every table and
	// every cache in front of them (QET_DUMP_DB_STATE). Shapes, texts and
	// pictures may have more rows when adopted, never fewer: the file keeps
	// those of folios the document's fill has not built (F140)
	void tablesGiveTheDocumentsDatabase_data()
	{
		QTest::addColumn<QString>("example");
		for (const char *name : {"industrial", "affuteuse_250h", "Projet_vierge", "m_000",
								 "2612_ats_singlephase", "tremie_vibrante"})
			QTest::newRow(name) << QString::fromLatin1(name);
	}

	void tablesGiveTheDocumentsDatabase()
	{
		QFETCH(QString, example);
		const QString file = m_dir.filePath(example + QStringLiteral(".dump.qetdb"));
		run({QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/") + example + QStringLiteral(".qet"), file});
		QVERIFY(QFile::exists(file));
		const QString adopted = m_dir.filePath(example + QStringLiteral(".adopted.txt"));
		const QString filled = m_dir.filePath(example + QStringLiteral(".filled.txt"));
		const QString out = m_dir.filePath(QStringLiteral("dump-out.qet"));
		const QString log = run({QStringLiteral("--resave"), file, out}, false, nullptr,
								{QStringLiteral("QET_DUMP_DB_STATE=") + adopted});
		QVERIFY2(log.contains(QStringLiteral("Project database read from the file")), qPrintable(log.right(600)));
		run({QStringLiteral("--resave"), file, out}, false, nullptr,
			{QStringLiteral("QET_DUMP_DB_STATE=") + filled, QStringLiteral("QET_QETDB_NO_ADOPT=1")});
			//Each section by its name, without its count
		const auto sections = [](const QString &path) {
			QFile f(path);
			QMap<QString, QSet<QString>> result;
			if (!f.open(QIODevice::ReadOnly)) return result;
			QString current;
			for (const QString &line : QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'))) {
				if (line.startsWith(QStringLiteral("== "))) {
					current = line.section(QStringLiteral(" ("), 0, 0);
					result[current];	//an empty section is one too
				} else result[current] << line;
			}
			return result;
		};
		const auto a = sections(adopted), b = sections(filled);
		QVERIFY(a.size() > 30);
		QCOMPARE(a.keys(), b.keys());
		static const QSet<QString> index_tables{QStringLiteral("== table shape"),
			QStringLiteral("== table independent_text"), QStringLiteral("== table image")};
		QStringList differing;
		for (auto it = a.constBegin() ; it != a.constEnd() ; ++it) {
			const QSet<QString> &from_document = b.value(it.key());
			if (index_tables.contains(it.key()) ? !it.value().contains(from_document)
												: it.value() != from_document)
				differing << it.key();
		}
		QVERIFY2(differing.isEmpty(), qPrintable(differing.join(QLatin1Char('\n'))));
	}

	// A symbol whose definition is missing is kept as read (F127): opened
	// from the file's tables and saved again, a folio not built keeps it
	void keptSymbolSurvivesResave()
	{
		QFile source(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY(source.open(QIODevice::ReadOnly));
		QByteArray text = source.readAll();
		const qsizetype last = text.lastIndexOf("<elements>");
		QVERIFY(last > 0);
		text.insert(last + qsizetype(qstrlen("<elements>")),
					"<element type=\"embed://import/missing/nowhere.elmt\" x=\"100\" y=\"100\" z=\"10\" "
					"orientation=\"0\" prefix=\"\" freezeLabel=\"false\" "
					"uuid=\"{5e1f0c3a-7d2b-4c11-9a4e-0b6c2d8e9f10}\"><terminals/><inputs/></element>");
		const QString with_missing = m_dir.filePath(QStringLiteral("missing.qet"));
		QFile out(with_missing);
		QVERIFY(out.open(QIODevice::WriteOnly));
		out.write(text);
		out.close();
		const QString first = m_dir.filePath(QStringLiteral("missing.qetdb"));
		const QString second = m_dir.filePath(QStringLiteral("missing-again.qetdb"));
		resave(with_missing, first);
		const QString log = resave(first, second);
		QVERIFY2(log.contains(QStringLiteral("Project database read from the file")), qPrintable(log.right(600)));
		const QString count = QStringLiteral("SELECT count(*) FROM kept_item WHERE content LIKE '%5e1f0c3a%'");
		QCOMPARE(pragma(first, count).toInt(), 1);
		QCOMPARE(pragma(second, count).toInt(), 1);
		const QString back = m_dir.filePath(QStringLiteral("missing-back.qet"));
		resave(second, back);
		QFile f(back);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QVERIFY(f.readAll().contains("5e1f0c3a-7d2b-4c11-9a4e-0b6c2d8e9f10"));
	}

	// The shape and text rows a file holds stay when it is opened from its
	// tables and saved again with its folios not built
	void drawingRowsSurviveResave()
	{
		const QString first = m_dir.filePath(QStringLiteral("drawn.qetdb"));
		const QString second = m_dir.filePath(QStringLiteral("drawn-again.qetdb"));
		run({QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), first}, true);
		const QString log = resave(first, second);
		QVERIFY2(log.contains(QStringLiteral("Project database read from the file")), qPrintable(log.right(600)));
		for (const QString &table : {QStringLiteral("shape"), QStringLiteral("independent_text")}) {
			const QString count = QStringLiteral("SELECT count(*) FROM %1").arg(table);
			QVERIFY2(pragma(first, count).toInt() > 0, qPrintable(table));
			QCOMPARE(pragma(second, count).toInt(), pragma(first, count).toInt());
		}
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

	// What SQLite's own check passes but the file must not hold: a value
	// changed since the save, SQL that would run on its own, an item
	// referring to one that is not there. Each refused, with why.
	void tamperedRefused_data()
	{
		QTest::addColumn<QString>("sql");
		QTest::addColumn<QString>("why");
		QTest::newRow("value changed") << QStringLiteral(
				"UPDATE element_info SET label = 'CHANGED' WHERE rowid = (SELECT min(rowid) FROM element_info)")
				<< QStringLiteral("not what QElectroTech saved");
		QTest::newRow("trigger") << QStringLiteral(
				"CREATE TRIGGER t AFTER INSERT ON element BEGIN DELETE FROM element; END")
				<< QStringLiteral("does not expect (t)");
		QTest::newRow("view") << QStringLiteral("CREATE VIEW extra AS SELECT 1")
				<< QStringLiteral("does not expect (extra)");
		QTest::newRow("missing item") << QStringLiteral(
				"DELETE FROM diagram WHERE rowid = (SELECT min(rowid) FROM diagram)")
				<< QStringLiteral("refers to one that is not there");
	}

	void tamperedRefused()
	{
		QFETCH(QString, sql);
		QFETCH(QString, why);
		const QString path = m_dir.filePath(QStringLiteral("tampered%1.qetdb").arg(m_run));
		QVERIFY(QFile::copy(industrial(), path));
		QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
		QVERIFY(exec(path, sql));
		QCOMPARE(pragma(path, QStringLiteral("PRAGMA quick_check")).toString(), QStringLiteral("ok"));
		int code = 0;
		const QString out = path + QStringLiteral(".qet");
		const QString log = resave(path, out, &code);
		QVERIFY2(code != 0 && log.contains(why), qPrintable(log.right(800)));
		QVERIFY(!QFile::exists(out));
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
