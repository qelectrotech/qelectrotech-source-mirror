// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// Renaming a plant or a location from a script (item C10 of #1878, stage L4),
// through the real binary's --run on perceuse.qet: qet.renamePlant() and
// qet.renameLocation() push the same single undo step as Project > Plants
// and locations > Rename. A script cannot answer the screen's questions, so
// it states them: a name that exists is refused unless merge is true.
class tst_scriptplacerename : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

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
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
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
	void renamesFromAScript()
	{
		const QJsonObject r = run(QStringLiteral(
			// Two folios of two plants, FACTORY +CP1 and WORKS +CP1, and a
			// symbol of the first that carries its own plant
			"function rows() {\n"
			"  return qet.query('SELECT plant, location FROM qet_locations ORDER BY plant, location')\n"
			"    .map(function (r) { return r.plant + '/' + r.location; });\n"
			"}\n"
			"var out = {};\n"
			"var f = -1;\n"
			"for (var i = 0; i < qet.folioCount() && f < 0; i++)\n"
			"  if (qet.elementUuids(i).length > 0) f = i;\n"
			"var g = f === 0 ? 1 : 0;\n"
			"qet.setFolioProperty(f, 'plant', 'FACTORY');\n"
			"qet.setFolioProperty(f, 'locmach', 'CP1');\n"
			"qet.setFolioProperty(g, 'plant', 'WORKS');\n"
			"qet.setFolioProperty(g, 'locmach', 'CP1');\n"
			"var el = qet.elementUuids(f)[0];\n"
			"qet.setElementInfo(f, el, 'plant', 'FACTORY');\n"
			"out.before = rows();\n"
			// refused: the name exists and merge is not asked for
			"out.refused = qet.renamePlant('FACTORY', 'WORKS', false, 'higher');\n"
			"out.afterRefused = rows();\n"
			// refused: counters other than higher, new, old
			"out.badCounters = qet.renamePlant('FACTORY', 'DEPOT', false, 'mine');\n"
			// refused: a name nothing stores, an empty name, the old name
			"out.unknown = qet.renamePlant('NOPE', 'DEPOT', false, 'higher');\n"
			"out.empty = qet.renamePlant('FACTORY', '', false, 'higher');\n"
			"out.same = qet.renamePlant('FACTORY', 'FACTORY', false, 'higher');\n"
			// a plain rename, kept in the folio and the symbol, one undo step
			"out.renamed = qet.renamePlant('FACTORY', 'DEPOT', false, 'higher');\n"
			"out.afterRename = rows();\n"
			"out.folioPlant = qet.folioProperty(f, 'plant');\n"
			"out.symbolPlant = qet.elementInfo(f, el, 'plant');\n"
			"qet.undo();\n"
			"out.afterUndo = rows();\n"
			"out.symbolAfterUndo = qet.elementInfo(f, el, 'plant');\n"
			// a location, only within its plant
			"out.locRenamed = qet.renameLocation('FACTORY', 'CP1', 'CP2', false);\n"
			"out.afterLoc = rows();\n"
			"out.locRefused = qet.renameLocation('FACTORY', 'CP2', 'CP2', false);\n"
			// merging when asked
			"out.merged = qet.renamePlant('FACTORY', 'WORKS', true, 'higher');\n"
			"out.afterMerge = rows();\n"
			"qet.log('PROBE ' + JSON.stringify(out));\n"),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));

		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		auto list = [&r](const char *key) {
			QStringList out;
			for (const QJsonValue &v : r.value(QLatin1String(key)).toArray())
				out << v.toString();
			return out;
		};
		QVERIFY(list("before").contains(QStringLiteral("FACTORY/CP1")));
		QVERIFY(list("before").contains(QStringLiteral("WORKS/CP1")));

		QVERIFY(!r.value(QStringLiteral("refused")).toBool());
		QCOMPARE(list("afterRefused"), list("before"));
		QVERIFY(!r.value(QStringLiteral("badCounters")).toBool());
		QVERIFY(!r.value(QStringLiteral("unknown")).toBool());
		QVERIFY(!r.value(QStringLiteral("empty")).toBool());
		QVERIFY(!r.value(QStringLiteral("same")).toBool());

		QVERIFY(r.value(QStringLiteral("renamed")).toBool());
		QVERIFY(list("afterRename").contains(QStringLiteral("DEPOT/CP1")));
		QVERIFY(!list("afterRename").contains(QStringLiteral("FACTORY/CP1")));
		QCOMPARE(r.value(QStringLiteral("folioPlant")).toString(), QStringLiteral("DEPOT"));
		QCOMPARE(r.value(QStringLiteral("symbolPlant")).toString(), QStringLiteral("DEPOT"));
			// one undo puts the folio and the symbol back together
		QCOMPARE(list("afterUndo"), list("before"));
		QCOMPARE(r.value(QStringLiteral("symbolAfterUndo")).toString(), QStringLiteral("FACTORY"));

		QVERIFY(r.value(QStringLiteral("locRenamed")).toBool());
		QVERIFY(list("afterLoc").contains(QStringLiteral("FACTORY/CP2")));
		QVERIFY(list("afterLoc").contains(QStringLiteral("WORKS/CP1")));
		QVERIFY(!r.value(QStringLiteral("locRefused")).toBool());

		QVERIFY(r.value(QStringLiteral("merged")).toBool());
		QVERIFY(!list("afterMerge").join(QLatin1Char(' ')).contains(QStringLiteral("FACTORY")));
		QVERIFY(list("afterMerge").contains(QStringLiteral("WORKS/CP2")));
	}
};

QTEST_APPLESS_MAIN(tst_scriptplacerename)
#include "tst_scriptplacerename.moc"
