// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QUuid>

// qet.terminalPosition(folio, element, terminal) and qet.conductorPath(folio,
// uuid): where a wire docks on a terminal and which way it leaves, and a
// wire's drawn path by its uuid. Checked against each other on every
// conductor of the fixture: a path starts and ends exactly where its two
// terminals say a wire docks, and leaves each the way it faces. Runs a
// script through the real binary's --run.
class tst_scriptlayoutreads : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// Run @p script on the fixture in a sandbox of its own and return the
	// JSON object it logged.
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
		if (!proc.waitForFinished(60000)) return {};
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

	void pathsStartAndEndAtTheirTerminals()
	{
		const QJsonObject r = run(QStringLiteral(
			"var out = [];\n"
			"var uuids = qet.conductorUuids(0);\n"
			"for (var i = 0; i < uuids.length; i++) {\n"
			"  var ends = qet.conductorEnds(0, uuids[i]);\n"
			"  var t = ends.map(function (e) { var m = e.split(' terminal ');\n"
			"    return qet.terminalPosition(0, m[0], parseInt(m[1], 10)); });\n"
			"  out.push({path: qet.conductorPath(0, uuids[i]), t: t});\n"
			"}\n"
			"qet.log('PROBE ' + JSON.stringify({wires: out,\n"
			"  unknown: qet.conductorPath(0, '{00000000-0000-0000-0000-000000000001}'),\n"
			"  junk: qet.conductorPath(0, 'not a uuid'),\n"
			"  badFolio: qet.conductorPath(99, uuids[0]),\n"
			"  badTerminal: qet.terminalPosition(0, qet.elementUuids(0)[0], 999)}));\n"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");

		const QJsonArray wires = r.value(QStringLiteral("wires")).toArray();
		QCOMPARE(wires.size(), 7);                    // the fixture's conductors
		const QHash<QString, QPointF> step{{QStringLiteral("n"), {0, -1}},
										   {QStringLiteral("e"), {1, 0}},
										   {QStringLiteral("s"), {0, 1}},
										   {QStringLiteral("w"), {-1, 0}}};
		for (const QJsonValue &v : wires) {
			const QJsonArray path = v.toObject().value(QStringLiteral("path")).toArray();
			const QJsonArray t = v.toObject().value(QStringLiteral("t")).toArray();
			QVERIFY(path.size() >= 2);
			QCOMPARE(t.size(), 2);
			const QJsonObject ends[2] = {path.first().toObject(), path.last().toObject()};
			const QJsonObject next[2] = {path.at(1).toObject(), path.at(path.size() - 2).toObject()};
			for (int k = 0; k < 2; ++k) {
				const QJsonObject term = t.at(k).toObject();
				QCOMPARE(ends[k].value(QStringLiteral("x")).toDouble(),
						 term.value(QStringLiteral("x")).toDouble());
				QCOMPARE(ends[k].value(QStringLiteral("y")).toDouble(),
						 term.value(QStringLiteral("y")).toDouble());
					// the first step out of a terminal goes the way it faces
				const QString facing = term.value(QStringLiteral("facing")).toString();
				QVERIFY2(step.contains(facing), qPrintable(facing));
				const QPointF d(next[k].value(QStringLiteral("x")).toDouble()
								- ends[k].value(QStringLiteral("x")).toDouble(),
								next[k].value(QStringLiteral("y")).toDouble()
								- ends[k].value(QStringLiteral("y")).toDouble());
				if (d.isNull()) continue;
				QVERIFY2(d.x() * step[facing].x() + d.y() * step[facing].y() > 0,
						 qPrintable(facing));
			}
		}
		QVERIFY(r.value(QStringLiteral("unknown")).toArray().isEmpty());
		QVERIFY(r.value(QStringLiteral("junk")).toArray().isEmpty());
		QVERIFY(r.value(QStringLiteral("badFolio")).toArray().isEmpty());
		QVERIFY(r.value(QStringLiteral("badTerminal")).toObject().isEmpty());
	}

	void pathMatchesConductorSegments()
	{
			// Where conductorSegments() can name the wire, both give the
			// same points.
		const QJsonObject r = run(QStringLiteral(
			"var same = 0, compared = 0, lines = qet.conductors(0);\n"
			"var uuids = qet.conductorUuids(0);\n"
			"for (var i = 0; i < uuids.length; i++) {\n"
			"  var e = qet.conductorEnds(0, uuids[i])[0], n = 0;\n"
			"  for (var l = 0; l < lines.length; l++) {\n"
			"    var p = lines[l].split(' : ')[0].split(' -- ');\n"
			"    if (p[0] === e || p[1] === e) n++;\n"
			"  }\n"
			"  if (n !== 1) continue;\n"
			"  var m = e.split(' terminal ');\n"
			"  var segs = qet.conductorSegments(0, m[0], parseInt(m[1], 10));\n"
			"  var pts = [];\n"
			"  segs.forEach(function (s, k) {\n"
			"    var c = s.match(/\\(([^,]+),([^)]+)\\)-\\(([^,]+),([^)]+)\\)/);\n"
			"    if (k === 0) pts.push([+c[1], +c[2]]);\n"
			"    pts.push([+c[3], +c[4]]); });\n"
			"  var path = qet.conductorPath(0, uuids[i]).map(function (q) { return [q.x, q.y]; });\n"
			"  compared++;\n"
			"  if (JSON.stringify(pts) === JSON.stringify(path)) same++;\n"
			"}\n"
			"qet.log('PROBE ' + JSON.stringify({same: same, compared: compared}));\n"));
		QVERIFY(r.value(QStringLiteral("compared")).toInt() > 0);
		QCOMPARE(r.value(QStringLiteral("same")).toInt(), r.value(QStringLiteral("compared")).toInt());
	}

	void facingTurnsWithTheElement()
	{
		const QJsonObject r = run(QStringLiteral(
			"var el = qet.elementUuids(0)[0];\n"
			"var before = qet.terminalPosition(0, el, 0).facing;\n"
			"qet.rotateElement(0, el, 90);\n"
			"var after = qet.terminalPosition(0, el, 0).facing;\n"
			"qet.log('PROBE ' + JSON.stringify({before: before, after: after}));\n"));
		const QString order = QStringLiteral("nesw");
		const int b = order.indexOf(r.value(QStringLiteral("before")).toString());
		const int a = order.indexOf(r.value(QStringLiteral("after")).toString());
		QVERIFY(b >= 0 && a >= 0);
		QCOMPARE(a, (b + 1) % 4);                    // a quarter turn clockwise
	}
};

QTEST_APPLESS_MAIN(tst_scriptlayoutreads)

#include "tst_scriptlayoutreads.moc"
