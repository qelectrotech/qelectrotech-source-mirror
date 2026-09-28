// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QDomDocument>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QUuid>

// qet.elementTerminals() ends each line with the terminal's uuid, and
// qet.terminalIndex(folio, element, terminalUuid) turns it back into the
// index the conductor calls take. Runs a script through the real binary's
// --run on perceuse.qet, which has symbols with two terminals at one point:
// their order in the index is undefined, their uuids are not.
class tst_scriptterminaluuid : public QObject
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

	void everyTerminalFoundByItsUuid()
	{
		const QJsonObject r = run(QStringLiteral(
			"var rows = [], pick = null;\n"
			"for (var f = 0; f < qet.folioCount(); f++) {\n"
			"  var els = qet.elementUuids(f);\n"
			"  for (var e = 0; e < els.length; e++) {\n"
			"    var lines = qet.elementTerminals(f, els[e]);\n"
			"    var back = lines.map(function (l) {\n"
			"      return qet.terminalIndex(f, els[e], l.substring(l.lastIndexOf(' ') + 1)); });\n"
			"    rows.push({lines: lines, back: back});\n"
			"    if (!pick && lines.length) pick = {f: f, e: els[e], t: lines[0].split(' ').pop()};\n"
			"  }\n"
			"}\n"
			"qet.log('PROBE ' + JSON.stringify({rows: rows,\n"
			"  unknown: qet.terminalIndex(pick.f, pick.e, '{00000000-0000-0000-0000-000000000001}'),\n"
			"  junk: qet.terminalIndex(pick.f, pick.e, 'not a uuid'),\n"
			"  noElement: qet.terminalIndex(pick.f, '{00000000-0000-0000-0000-000000000002}', pick.t),\n"
			"  badFolio: qet.terminalIndex(99, pick.e, pick.t)}));\n"),
			QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");

		const QRegularExpression line_re(QStringLiteral(
			"^(\\d+): .* \\(\\d+ conductor\\(s\\)\\) (\\{[0-9a-f-]{36}\\})$"));
		int terminals = 0;
		for (const QJsonValue &row : r.value(QStringLiteral("rows")).toArray()) {
			const QJsonArray lines = row.toObject().value(QStringLiteral("lines")).toArray();
			const QJsonArray back = row.toObject().value(QStringLiteral("back")).toArray();
			QCOMPARE(back.size(), lines.size());
			QSet<QString> uuids;
			for (int i = 0; i < lines.size(); ++i) {
				const QString line = lines.at(i).toString();
				const QRegularExpressionMatch m = line_re.match(line);
				QVERIFY2(m.hasMatch(), qPrintable(line));
				QCOMPARE(m.captured(1).toInt(), i);
				QVERIFY(!QUuid(m.captured(2)).isNull());
				uuids.insert(m.captured(2));
					// the uuid finds this very terminal again
				QCOMPARE(back.at(i).toInt(), i);
				++terminals;
			}
			QCOMPARE(uuids.size(), lines.size());
		}
		QVERIFY(terminals > 200);

		QCOMPARE(r.value(QStringLiteral("unknown")).toInt(), -1);
		QCOMPARE(r.value(QStringLiteral("junk")).toInt(), -1);
		QCOMPARE(r.value(QStringLiteral("noElement")).toInt(), -1);
		QCOMPARE(r.value(QStringLiteral("badFolio")).toInt(), -1);
	}

	// A "%3" or "%4" in a terminal's name is listed as written, not
	// replaced by the conductor count or the uuid.
	void percentInNameKept()
	{
		QFile in(QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));
		QVERIFY(in.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(bool(doc.setContent(&in)));
		const QDomNodeList terminals = doc.documentElement()
				.firstChildElement(QStringLiteral("collection"))
				.elementsByTagName(QStringLiteral("terminal"));
		QVERIFY(!terminals.isEmpty());
		terminals.at(0).toElement().setAttribute(QStringLiteral("name"),
												 QStringLiteral("x%3y%4"));
		const QString project = m_dir.filePath(QStringLiteral("percent.qet"));
		QFile out(project);
		QVERIFY(out.open(QIODevice::WriteOnly));
		out.write(doc.toByteArray());
		out.close();

		const QJsonObject r = run(QStringLiteral(
			"var hits = [];\n"
			"for (var f = 0; f < qet.folioCount(); f++) {\n"
			"  var els = qet.elementUuids(f);\n"
			"  for (var e = 0; e < els.length; e++)\n"
			"    qet.elementTerminals(f, els[e]).forEach(function (l) {\n"
			"      if (/: x.*y.* \\(/.test(l)) hits.push(l); });\n"
			"}\n"
			"qet.log('PROBE ' + JSON.stringify({hits: hits}));\n"), project);
		const QJsonArray hits = r.value(QStringLiteral("hits")).toArray();
		QVERIFY2(!hits.isEmpty(), "the renamed terminal was not listed");
		for (const QJsonValue &h : hits)
			QVERIFY2(h.toString().contains(QStringLiteral(": x%3y%4 (")), qPrintable(h.toString()));
	}
};

QTEST_APPLESS_MAIN(tst_scriptterminaluuid)

#include "tst_scriptterminaluuid.moc"
