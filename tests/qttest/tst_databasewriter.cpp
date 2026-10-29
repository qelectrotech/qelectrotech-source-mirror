// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// A project written back from its database alone (qet.saveFromDatabase(),
// projectDataBase::toDocument()) saves as the project itself does: the
// database holds the whole project. QET-SQL-FORMAT S3z1; the corpus-wide
// form of this check is tools/db-roundtrip/roundtrip.py.
//
// Runs the real binary: opens a project, writes it from its database, then
// saves the original and the written file with every folio built and
// compares the two saves byte for byte.
class tst_databasewriter : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QProcessEnvironment environment(const QString &home, bool eager) const
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		return env;
	}

	bool runQet(const QStringList &arguments, const QString &home, bool eager, QString *log = nullptr)
	{
		QProcess proc;
		proc.setProcessEnvironment(environment(home, eager));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   QStringList{QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")} + arguments);
		if (!proc.waitForFinished(300000)) return false;
		if (log) *log = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		return true;
	}

	// The save of @p source, and the save of what its database gives back,
	// written with folios built on demand (or all, @p eager_write).
	// @p script runs before the database is written.
	void roundTrip(const QString &source, const QString &script, bool eager_write,
				   QByteArray *original, QByteArray *written)
	{
		const QString work = m_dir.filePath(QStringLiteral("run%1").arg(m_run++));
		const QString home = work + QStringLiteral("/home");
		QDir().mkpath(work + QStringLiteral("/written"));
		QDir().mkpath(home);
			//Its own name: a project without a title takes the file's
		const QString from_db = work + QStringLiteral("/written/") + QFileInfo(source).fileName();
		const QString edited = work + QStringLiteral("/") + QFileInfo(source).fileName();
		QFile js(work + QStringLiteral("/write.js"));
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write(script.toUtf8());
		js.write(QStringLiteral("qet.log('WROTE ' + qet.saveFromDatabase('%1'));\n"
								"qet.log('SAVED ' + qet.save('%2'));\n").arg(from_db, edited).toUtf8());
		js.close();
		QString log;
		QVERIFY(runQet({QStringLiteral("--run"), js.fileName(), source}, home, eager_write, &log));
		QVERIFY2(log.contains(QStringLiteral("WROTE true")), qPrintable(log.right(2000)));
		QVERIFY2(log.contains(QStringLiteral("SAVED true")), qPrintable(log.right(2000)));

			//The project as edited, saved by itself, against the one written
			//from its database: each saved again with every folio built
		const QString a = work + QStringLiteral("/a.qet"), b = work + QStringLiteral("/b.qet");
		QVERIFY(runQet({QStringLiteral("--resave"), edited, a}, home, true));
		QVERIFY(runQet({QStringLiteral("--resave"), from_db, b}, home, true));
		QFile fa(a), fb(b);
		QVERIFY(fa.open(QIODevice::ReadOnly));
		QVERIFY(fb.open(QIODevice::ReadOnly));
		*original = fa.readAll();
		*written = fb.readAll();
	}

	static QList<QByteArray> trimmedLines(const QByteArray &text)
	{
		QList<QByteArray> lines = text.split('\n');
		for (QByteArray &line : lines) line = line.trimmed();
		return lines;
	}

	static QString firstDifference(const QByteArray &a, const QByteArray &b)
	{
		const QList<QByteArray> la = a.split('\n'), lb = b.split('\n');
		for (int i = 0 ; i < qMin(la.size(), lb.size()) ; ++i)
			if (la.at(i) != lb.at(i))
				return QStringLiteral("line %1:\n  saved:   %2\n  written: %3")
						.arg(i + 1).arg(QString::fromUtf8(la.at(i).trimmed()), QString::fromUtf8(lb.at(i).trimmed()));
		return QStringLiteral("%1 lines against %2").arg(la.size()).arg(lb.size());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void realProject_data()
	{
		QTest::addColumn<bool>("eager");
		QTest::newRow("folios built on demand") << false;
		QTest::newRow("every folio built") << true;
	}

	// industrial.qet: 50 folios, its own symbols and a template
	void realProject()
	{
		QFETCH(bool, eager);
		QByteArray original, written;
		roundTrip(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), QString(), eager, &original, &written);
		QVERIFY(original.size() > 100000);
		QVERIFY2(original == written, qPrintable(firstDifference(original, written)));
	}

	// Edited first: a symbol placed with a text emptied, a wire, a cable
	// across it, a folio's default wire changed, a folio added
	void editedProject()
	{
		const QString edits = QStringLiteral(
				"var p = 'common://10_electric/10_allpole/130_terminals_terminal_strips/bornedouble.elmt';\n"
				"var a = qet.addElement(0, p, 100, 300), b = qet.addElement(0, p, 400, 300);\n"
				"qet.addConductor(0, a, 0, b, 0);\n"
				"var y = qet.terminalPosition(0, a, 0).y;\n"
				"qet.log('CABLE ' + qet.addCable(0, 'W1', 'H07', ['BK', 'BU'], '5', 250, y - 50, 250, y + 50));\n"
				"qet.setConductorDefault(0, 'color', '#ff0000');\n"
				"qet.addFolio();\n");
		QByteArray original, written;
		roundTrip(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), edits, false, &original, &written);
		QVERIFY(original.contains("designation=\"W1\""));
			//The symbol placed from the library brings its categories into
			//the project's collection, where QET's own save writes one of
			//them indented a level short (a spacing-only text node it keeps
			//in front of it): the lines must match, not their indentation
		QCOMPARE(trimmedLines(written), trimmedLines(original));
	}
};

QTEST_GUILESS_MAIN(tst_databasewriter)
#include "tst_databasewriter.moc"
