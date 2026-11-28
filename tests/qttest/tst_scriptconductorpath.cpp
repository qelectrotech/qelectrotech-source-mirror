// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// qet.setConductorPathBetween(): an importer gives a conductor the path its
// source drawing had, through the corners in between. Corners a hair off a
// straight line still give straight segments; corners that do not make
// horizontal and vertical segments leave the path as it was ("kept"); a
// corner that is not x,y or a pair of terminals no conductor joins is
// refused. Runs a script through the real binary's --run.
class tst_scriptconductorpath : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QJsonObject run(const QString &script)
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
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path,
					QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet")});
		if (!proc.waitForFinished(QetTest::timeout(60000))) return {};
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

	void pathThroughCorners()
	{
		const QJsonObject r = run(QStringLiteral(
			"var p = 'embed://import/probe/v2_fuse.elmt';\n"
			"var a = qet.addElement(0, p, 400, 300);\n"
			"var b = qet.addElement(0, p, 600, 500);\n"
			"qet.addConductor(0, a, 1, b, 0);\n"
			"var pa = qet.terminalPosition(0, a, 1), pb = qet.terminalPosition(0, b, 0);\n"
			"var mid = Math.round((pa.y + pb.y) / 2);\n"
				// the second corner half a unit off the first one's y
			"var corners = pa.x + ',' + mid + ';' + pb.x + ',' + (mid + 0.5);\n"
			"var set = qet.setConductorPathBetween(0, a, 1, b, 0, corners);\n"
			"var segs = qet.conductorSegments(0, a, 1);\n"
			"var reversed = qet.setConductorPathBetween(0, b, 0, a, 1, pb.x + ',' + (mid + 20) + ';' + pa.x + ',' + (mid + 20));\n"
			"var segs2 = qet.conductorSegments(0, a, 1);\n"
			"var kept = qet.setConductorPathBetween(0, a, 1, b, 0, (pa.x + 50) + ',' + (pa.y + 50));\n"
			"var segs3 = qet.conductorSegments(0, a, 1);\n"
				// the last corner up to one unit off the far terminal's x
			"var near = [0, 0.3, 0.7, 1.0].map(function (d) {\n"
			"  return qet.setConductorPathBetween(0, a, 1, b, 0,\n"
			"    pa.x + ',' + (mid + 40) + ';' + (pb.x + d) + ',' + (mid + 40)); });\n"
			"qet.log('PROBE ' + JSON.stringify({set: set, segs: segs, mid: mid, reversed: reversed,\n"
			"  segs2: segs2, kept: kept, segs3: segs3, near: near,\n"
			"  junk: qet.setConductorPathBetween(0, a, 1, b, 0, 'one,two'),\n"
			"  none: qet.setConductorPathBetween(0, a, 0, b, 1, '')}));\n"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");

		QCOMPARE(r.value(QStringLiteral("set")).toString(), QStringLiteral("set"));
		const QJsonArray segs = r.value(QStringLiteral("segs")).toArray();
		QCOMPARE(segs.size(), 3);
		const QString mid = QString::number(r.value(QStringLiteral("mid")).toInt());
		const QString across = segs.at(1).toString();
		QVERIFY2(across.contains(QStringLiteral("horizontal")), qPrintable(across));
			// both ends of the middle segment on the first corner's y: snapped
		QVERIFY2(across.contains(QStringLiteral(",%1)-(").arg(mid))
				 && across.contains(QStringLiteral(",%1) ").arg(mid)), qPrintable(across + QStringLiteral(" | ") + mid));

			// given from the far end, the corners still apply in order
		QCOMPARE(r.value(QStringLiteral("reversed")).toString(), QStringLiteral("set"));
		const QString across2 = r.value(QStringLiteral("segs2")).toArray().at(1).toString();
		QVERIFY2(across2.contains(QStringLiteral(",%1)-(").arg(r.value(QStringLiteral("mid")).toInt() + 20)),
				 qPrintable(across2));

			// a diagonal: the conductor keeps the path it had
		QCOMPARE(r.value(QStringLiteral("kept")).toString(), QStringLiteral("kept"));
		QCOMPARE(r.value(QStringLiteral("segs3")).toArray(), r.value(QStringLiteral("segs2")).toArray());

			// a corner a hair off the far terminal is put onto it
		const QJsonArray near = r.value(QStringLiteral("near")).toArray();
		QCOMPARE(near.size(), 4);
		for (const QJsonValue &v : near)
			QCOMPARE(v.toString(), QStringLiteral("set"));

		QCOMPARE(r.value(QStringLiteral("junk")).toString(), QString());
		QCOMPARE(r.value(QStringLiteral("none")).toString(), QString());
	}
};

QTEST_APPLESS_MAIN(tst_scriptconductorpath)

#include "tst_scriptconductorpath.moc"
