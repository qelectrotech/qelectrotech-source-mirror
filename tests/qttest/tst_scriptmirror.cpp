// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// Mirroring a symbol on its folio (#1335), through the real binary's --run
// on perceuse.qet: qet.mirrorElement() pushes the same undo command as
// Edit > Miroir horizontal / vertical. A horizontal mirror swaps the
// terminals left and right and turns the ones facing east to face west; a
// vertical one does the same top and bottom; mirroring twice the same way
// puts the symbol back exactly; and a save keeps the mirror.
class tst_scriptmirror : public QObject
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

	static QString facing(const QJsonArray &terminals, int i)
	{
		return terminals.at(i).toObject().value(QStringLiteral("facing")).toString();
	}
	static double coord(const QJsonArray &terminals, int i, const char *axis)
	{
		return terminals.at(i).toObject().value(QLatin1String(axis)).toDouble();
	}

	// The facing of a terminal once mirrored: across a vertical line
	// (horizontal mirror) east and west swap, across a horizontal one north
	// and south do.
	static QString mirrored(const QString &facing, bool vertical)
	{
		const QString swap = vertical ? QStringLiteral("ns") : QStringLiteral("ew");
		const int i = swap.indexOf(facing);
		return i < 0 ? facing : QString(swap.at(1 - i));
	}

	// @p after is @p before mirrored horizontally (or vertically): the
	// terminals face the mirrored way, keep the other coordinate, and are
	// mirrored across one same line, so x + x' (or y + y') is the same for
	// all of them.
	static void checkMirrored(const QJsonArray &before, const QJsonArray &after, bool vertical)
	{
		QCOMPARE(after.size(), before.size());
		const char *across = vertical ? "y" : "x";
		const char *along  = vertical ? "x" : "y";
		const double axis = coord(before, 0, across) + coord(after, 0, across);
		const double shift = coord(after, 0, along) - coord(before, 0, along);
		for (int i = 0; i < before.size(); ++i) {
			QCOMPARE(facing(after, i), mirrored(facing(before, i), vertical));
			QCOMPARE(coord(before, i, across) + coord(after, i, across), axis);
			QCOMPARE(coord(after, i, along) - coord(before, i, along), shift);
		}
	}

	static QString script(const QString &body)
	{
		return QStringLiteral(
			"function terms(f, el) {\n"
			"  var r = [], n = qet.elementTerminals(f, el).length;\n"
			"  for (var i = 0; i < n; i++) r.push(qet.terminalPosition(f, el, i));\n"
			"  return r;\n"
			"}\n") + body;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void mirrorBackAndSave()
	{
		const QString saved = m_dir.filePath(QStringLiteral("mirrored.qet"));
		const QJsonObject r = run(script(QStringLiteral(
			// a symbol with terminals at different x and different y, one
			// of them facing east or west
			"var pick = null;\n"
			"for (var f = 0; f < qet.folioCount() && !pick; f++) {\n"
			"  var els = qet.elementUuids(f);\n"
			"  for (var e = 0; e < els.length && !pick; e++) {\n"
			"    var t = terms(f, els[e]);\n"
			"    if (t.length < 2) continue;\n"
			"    var side = t.some(function (p) { return p.facing === 'e' || p.facing === 'w'; });\n"
			"    var xs = t.some(function (p) { return p.x !== t[0].x; });\n"
			"    var ys = t.some(function (p) { return p.y !== t[0].y; });\n"
			"    if (side && xs && ys) pick = {f: f, e: els[e]};\n"
			"  }\n"
			"}\n"
			"var f = pick.f, el = pick.e, out = {element: el};\n"
			"out.before = terms(f, el);\n"
			"qet.mirrorElement(f, el, false);\n"
			"out.h = terms(f, el); out.hMirror = qet.elementMirror(f, el);\n"
			"qet.mirrorElement(f, el, false);\n"
			"out.back = terms(f, el); out.backMirror = qet.elementMirror(f, el);\n"
			"qet.mirrorElement(f, el, true);\n"
			"out.v = terms(f, el); out.vMirror = qet.elementMirror(f, el);\n"
			"qet.undo();\n"
			"out.undone = terms(f, el);\n"
			"qet.mirrorElement(f, el, true);\n"
			"out.folio = f;\n"
			"out.saved = qet.save(%1);\n"
			"qet.log('PROBE ' + JSON.stringify(out));\n")
			.arg(QString::fromUtf8(QJsonDocument(QJsonArray{saved}).toJson(QJsonDocument::Compact)).mid(1).chopped(1))),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));

		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		const QJsonArray before = r.value(QStringLiteral("before")).toArray();
		QVERIFY(before.size() >= 2);

		QVERIFY(!r.value(QStringLiteral("hMirror")).toString().isEmpty());
		checkMirrored(before, r.value(QStringLiteral("h")).toArray(), false);

		QCOMPARE(r.value(QStringLiteral("backMirror")).toString(), QString());
		QCOMPARE(r.value(QStringLiteral("back")).toArray(), before);

		checkMirrored(before, r.value(QStringLiteral("v")).toArray(), true);
		QVERIFY(r.value(QStringLiteral("vMirror")).toString()
				!= r.value(QStringLiteral("hMirror")).toString());
		QCOMPARE(r.value(QStringLiteral("undone")).toArray(), before);

			//The save keeps the mirror, and only on that element
		QVERIFY(r.value(QStringLiteral("saved")).toBool());
		QFile file(saved);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(bool(doc.setContent(&file)));
		const QString element = r.value(QStringLiteral("element")).toString();
		const QDomNodeList elements = doc.elementsByTagName(QStringLiteral("element"));
		int mirrored_count = 0;
		for (int i = 0; i < elements.size(); ++i) {
			const QDomElement e = elements.at(i).toElement();
			if (e.hasAttribute(QStringLiteral("mirror"))) {
				++mirrored_count;
				QCOMPARE(e.attribute(QStringLiteral("uuid")), element);
				QCOMPARE(e.attribute(QStringLiteral("mirror")),
						 r.value(QStringLiteral("vMirror")).toString());
			}
		}
		QCOMPARE(mirrored_count, 1);

		const QJsonObject reloaded = run(script(QStringLiteral(
			"var f = %1, el = '%2';\n"
			"qet.log('PROBE ' + JSON.stringify({t: terms(f, el), m: qet.elementMirror(f, el)}));\n")
			.arg(r.value(QStringLiteral("folio")).toInt()).arg(element)), saved);
		QCOMPARE(reloaded.value(QStringLiteral("m")).toString(),
				 r.value(QStringLiteral("vMirror")).toString());
		QCOMPARE(reloaded.value(QStringLiteral("t")).toArray(),
				 r.value(QStringLiteral("v")).toArray());
	}

	/**
		Mirroring a symbol twice the same way puts it back where it was,
		for every symbol of the project, including those that do not sit
		on the grid. Before, the second mirror snapped such a symbol onto
		the grid, a few pixels away. Which symbol mirrorBackAndSave()
		picks changes from run to run, so this checks them all.
	*/
	void mirrorTwiceComesBack_data()
	{
		QTest::addColumn<bool>("vertical");
		QTest::newRow("horizontal") << false;
		QTest::newRow("vertical")   << true;
	}

	void mirrorTwiceComesBack()
	{
		QFETCH(bool, vertical);
		const QJsonObject r = run(script(QStringLiteral(
			"var out = {count: 0, moved: []};\n"
			"for (var f = 0; f < qet.folioCount(); f++) {\n"
			"  var els = qet.elementUuids(f);\n"
			"  for (var e = 0; e < els.length; e++) {\n"
			"    var before = JSON.stringify(terms(f, els[e]));\n"
			"    qet.mirrorElement(f, els[e], %1);\n"
			"    qet.mirrorElement(f, els[e], %1);\n"
			"    var after = JSON.stringify(terms(f, els[e]));\n"
			"    out.count++;\n"
			"    if (after !== before) out.moved.push(els[e] + ' ' + before + ' -> ' + after);\n"
			"  }\n"
			"}\n"
			"qet.log('PROBE ' + JSON.stringify(out));\n")
			.arg(vertical ? QStringLiteral("true") : QStringLiteral("false"))),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));

		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QVERIFY(r.value(QStringLiteral("count")).toInt() > 0);
		const QJsonArray moved = r.value(QStringLiteral("moved")).toArray();
		for (const QJsonValue &m : moved)
			qWarning("moved: %s", qPrintable(m.toString()));
		QCOMPARE(moved.size(), 0);
	}

	/**
		On a turned symbol a mirror of the folio is the other mirror of the
		symbol itself, and its rotation does not change: a label kept
		upright does not swing round.
	*/
	void turnedElement_data()
	{
		QTest::addColumn<bool>("vertical");
		QTest::newRow("horizontal") << false;
		QTest::newRow("vertical")   << true;
	}

	void turnedElement()
	{
		QFETCH(bool, vertical);
		const QJsonObject r = run(script(QStringLiteral(
			"var pick = null;\n"
			"for (var f = 0; f < qet.folioCount() && !pick; f++) {\n"
			"  var els = qet.elementUuids(f);\n"
			"  for (var e = 0; e < els.length && !pick; e++) {\n"
			"    var t = terms(f, els[e]);\n"
			"    if (t.length >= 2 && t.some(function (p) { return p.x !== t[0].x; })\n"
			"        && t.some(function (p) { return p.y !== t[0].y; })) pick = {f: f, e: els[e]};\n"
			"  }\n"
			"}\n"
			"var f = pick.f, el = pick.e, out = {};\n"
			"qet.rotateElement(f, el, 90);\n"
			"out.before = terms(f, el);\n"
			"qet.mirrorElement(f, el, %1);\n"
			"out.after = terms(f, el); out.mirror = qet.elementMirror(f, el);\n"
			"qet.log('PROBE ' + JSON.stringify(out));\n")
			.arg(vertical ? QStringLiteral("true") : QStringLiteral("false"))),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));

		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		checkMirrored(r.value(QStringLiteral("before")).toArray(),
					  r.value(QStringLiteral("after")).toArray(), vertical);
			//turned by 90 degrees, the symbol's own axes are the other way
		QCOMPARE(r.value(QStringLiteral("mirror")).toString(),
				 vertical ? QStringLiteral("horizontal") : QStringLiteral("vertical"));
	}
};

QTEST_APPLESS_MAIN(tst_scriptmirror)

#include "tst_scriptmirror.moc"
