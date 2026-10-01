// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// qet.currentFolio() is the folio on screen in the editor. Through --run
// there is none, so it stands in with the first folio, or -1 when the
// project has no folio. --run also keeps one undo step per call: only a run
// started from the editor is grouped into one, and a headless script that
// calls qet.undo() itself must go on working.
class tst_scriptcurrentfolio : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// Run @p script on @p project in a sandbox of its own and return the
	// JSON object it logged.
	QJsonObject run(const QString &script, const QString &project)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(script.toUtf8());
		f.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path, project});
		if (!proc.waitForFinished(120000)) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput()
											  + proc.readAllStandardError());
		const QString mark = QStringLiteral("PROBE ");
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(mark);
			if (i >= 0)
				return QJsonDocument::fromJson(line.mid(i + mark.size()).toUtf8()).object();
		}
		return {};
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void firstFolioWithoutAView()
	{
		const QJsonObject r = run(QStringLiteral(
			"qet.log('PROBE ' + JSON.stringify({current: qet.currentFolio(),\n"
			"  folios: qet.folioCount()}));\n"),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QVERIFY(r.value(QStringLiteral("folios")).toInt() > 1);
		QCOMPARE(r.value(QStringLiteral("current")).toInt(), 0);
	}

	void noFolioIsMinusOne()
	{
		const QString project = m_dir.filePath(QStringLiteral("empty.qet"));
		QFile out(project);
		QVERIFY(out.open(QIODevice::WriteOnly));
		out.write("<project version=\"0.100.0\" title=\"empty\">\n</project>\n");
		out.close();

		const QJsonObject r = run(QStringLiteral(
			"qet.log('PROBE ' + JSON.stringify({current: qet.currentFolio(),\n"
			"  folios: qet.folioCount()}));\n"), project);
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QCOMPARE(r.value(QStringLiteral("folios")).toInt(), 0);
		QCOMPARE(r.value(QStringLiteral("current")).toInt(), -1);
	}

	// The list is read from the build itself: the calls a script uses
	// are in it with their parameter names, each once.
	void apiSignaturesListed()
	{
		const QJsonObject r = run(QStringLiteral(
			"qet.log('PROBE ' + JSON.stringify({sigs: qet.apiSignatures()}));\n"),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QStringList sigs;
		for (const QJsonValue &v : r.value(QStringLiteral("sigs")).toArray())
			sigs << v.toString();
		QVERIFY(sigs.size() > 100);
		QVERIFY(sigs.contains(QStringLiteral("int currentFolio()")));
		QVERIFY(sigs.contains(QStringLiteral(
			"int addText(int folioIndex, QString text, double x, double y)")));
		QVERIFY(sigs.contains(QStringLiteral(
			"bool exportPdf(QString output, bool showTerminals)")));
		QCOMPARE(sigs.filter(QStringLiteral(" exportPdf(")).size(), 1);
		QVERIFY(sigs.filter(QStringLiteral("setUndoGrouped")).isEmpty());
	}

	void headlessUndoStaysPerCall()
	{
		const QJsonObject r = run(QStringLiteral(
			"var f = qet.currentFolio();\n"
			"var before = qet.texts(f).length;\n"
			"qet.addText(f, 'one', 40, 40);\n"
			"qet.addText(f, 'two', 40, 80);\n"
			"var undone = qet.undo();\n"
			"qet.log('PROBE ' + JSON.stringify({undone: undone,\n"
			"  added: qet.texts(f).length - before}));\n"),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QCOMPARE(r.value(QStringLiteral("undone")).toBool(), true);
		QCOMPARE(r.value(QStringLiteral("added")).toInt(), 1);
	}
};

QTEST_MAIN(tst_scriptcurrentfolio)
#include "tst_scriptcurrentfolio.moc"
