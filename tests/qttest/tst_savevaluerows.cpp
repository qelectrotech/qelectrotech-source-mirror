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

// What a symbol, a folio or a cable saves that no other table holds is in
// the project database, one row per value: element_value (prefix, frozen
// label, whether it moves, a terminal's override, a PLC's data),
// folio_value (the folio's default wire, freeze and numbering settings),
// cable_value (each cable whole). QET-SQL-FORMAT S3n.
//
// Runs the real binary (--run) on a small project written here and on
// industrial.qet.
namespace {

const QString symbol_uuid = QStringLiteral("{44444444-4444-4444-8444-444444444444}");
const QString set_uuid = QStringLiteral("{55555555-5555-4555-8555-555555555551}");
const QString plain_uuid = QStringLiteral("{55555555-5555-4555-8555-555555555552}");

QString project()
{
	return QStringLiteral(
				"<project title=\"values\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\""
				" freezeNewElement=\"true\" freezeNewConductor=\"false\">"
				"<elements>"
				"<element type=\"embed://import/box.elmt\" uuid=\"%2\" x=\"100\" y=\"100\" z=\"10\""
				" orientation=\"0\" prefix=\"KA\" freezeLabel=\"true\" is_movable=\"0\">"
				"<terminals/><inputs/></element>"
				"<element type=\"embed://import/box.elmt\" uuid=\"%3\" x=\"300\" y=\"100\" z=\"10\""
				" orientation=\"0\"><terminals/><inputs/></element>"
				"</elements><conductors/></diagram>"
				"<collection><category name=\"import\">"
				"<names><name lang=\"en\">Imported</name></names>"
				"<element name=\"box.elmt\">"
				"<definition width=\"30\" hotspot_y=\"21\" version=\"0.100.0\" type=\"element\""
				" link_type=\"simple\" hotspot_x=\"15\" height=\"42\">"
				"<uuid uuid=\"%1\"/>"
				"<names><name lang=\"en\">box</name></names>"
				"<kindInformations/><informations/>"
				"<description><rect x=\"-5\" y=\"-6\" width=\"10\" height=\"12\" antialias=\"true\""
				" style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
				"</description></definition></element></category></collection>"
				"</project>").arg(symbol_uuid, set_uuid, plain_uuid);
}

const char dump_script[] =
		"function dump(name) {\n"
		"  var t = ['element_value', 'folio_value', 'cable_value'];\n"
		"  for (var i = 0 ; i < t.length ; ++i)\n"
		"    qet.log('ROWS ' + name + ' ' + t[i] + ' ' + JSON.stringify(qet.query("
		"\"SELECT * FROM \" + t[i] + \" ORDER BY 1, 2, 3, 4\")));\n"
		"}\n";

} // namespace

class tst_savevaluerows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// dump name -> table -> rows, each "key section ord name=value"
	using Dump = QHash<QString, QHash<QString, QStringList>>;

	QProcessEnvironment environment(const QString &home) const
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		return env;
	}

	QString writeProject()
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return QString();
		f.write(project().toUtf8());
		return in;
	}

	// What @p script logs with dump('NAME'); @p eager builds every folio
	Dump run(const QString &script, const QString &in, bool eager = false)
	{
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		++m_run;
		QDir().mkpath(home);
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write(dump_script);
		s.write(script.toUtf8());
		s.close();

		QProcessEnvironment env = environment(home);
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		QProcess proc;
		proc.setProcessEnvironment(env);
			//The shipped symbols, so common:// resolves
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/"),
					QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(QetTest::timeout(180000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		Dump result;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			const QStringList head = line.mid(i + 5).section(QLatin1Char(' '), 0, 1).split(QLatin1Char(' '));
			const QString json = line.mid(i + 5).section(QLatin1Char(' '), 2);
			QStringList &rows = result[head.value(0)][head.value(1)];
			for (const QJsonValue &v : QJsonDocument::fromJson(json.toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				const QString key = o.value(QStringLiteral("element_uuid")).toString()
						+ o.value(QStringLiteral("diagram_uuid")).toString()
						+ o.value(QStringLiteral("cable_uuid")).toString();
				rows << QStringLiteral("%1 %2 %3 %4=%5").arg(key, o.value(QStringLiteral("section")).toString())
						.arg(o.value(QStringLiteral("ord")).toInt())
						.arg(o.value(QStringLiteral("name")).toString(), o.value(QStringLiteral("value")).toString());
			}
		}
		return result;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// A symbol's values as the file holds them, and as they are read when
	// left out. Its only folio is built either way: the rows of folios not
	// built are realProjectLazyAndBuiltAgree()'s
	void symbolValuesAreRows()
	{
		const QString in = writeProject();
		const Dump lazy = run(QStringLiteral("dump('open');"), in);
		const Dump built = run(QStringLiteral("dump('open');"), in, true);
		const QStringList rows = lazy.value(QStringLiteral("open")).value(QStringLiteral("element_value"));
		QCOMPARE(rows, (QStringList{
						  set_uuid + QStringLiteral(" element 0 freezeLabel=true"),
						  set_uuid + QStringLiteral(" element 0 is_movable=0"),
						  set_uuid + QStringLiteral(" element 0 prefix=KA"),
						  plain_uuid + QStringLiteral(" element 0 freezeLabel=false"),
						  plain_uuid + QStringLiteral(" element 0 is_movable=1"),
						  plain_uuid + QStringLiteral(" element 0 prefix=")}));
		QCOMPARE(built.value(QStringLiteral("open")).value(QStringLiteral("element_value")), rows);
	}

	// The folio's freeze settings from the file, and a change of its
	// default wire written
	void folioValuesAreWritten()
	{
		const QString in = writeProject();
		const Dump r = run(QStringLiteral(
				"dump('before');\n"
				"qet.log('SET ' + qet.setConductorDefault(0, 'color', '#ff0000'));\n"
				"dump('after');\n"), in);
		const QStringList before = r.value(QStringLiteral("before")).value(QStringLiteral("folio_value"));
		const auto has = [](const QStringList &rows, const QString &end) {
			for (const QString &row : rows)
				if (row.endsWith(end)) return true;
			return false;
		};
		QVERIFY2(has(before, QStringLiteral(" diagram 0 freezeNewElement=true")), qPrintable(before.join('\n')));
		QVERIFY(has(before, QStringLiteral(" diagram 0 freezeNewConductor=false")));
		QVERIFY(!has(before, QStringLiteral("color=#ff0000")));
		const QStringList after = r.value(QStringLiteral("after")).value(QStringLiteral("folio_value"));
		QVERIFY2(has(after, QStringLiteral(" diagram/defaultconductor 0 color=#ff0000")), qPrintable(after.join('\n')));
	}

	// The project's uuid, with its settings
	void projectUuidIsARow()
	{
		const QString in = writeProject();
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		++m_run;
		QDir().mkpath(home);
		QFile s(js);
		QVERIFY(s.open(QIODevice::WriteOnly));
		s.write("qet.log('PU ' + JSON.stringify(qet.query(\"SELECT value FROM project_setting "
				"WHERE section = 'project' AND name = 'uuid'\")));\n");
		s.close();
		QProcess proc;
		proc.setProcessEnvironment(environment(home));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, in});
		QVERIFY(proc.waitForFinished(QetTest::timeout(180000)));
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		QVERIFY2(out.contains(QRegularExpression(QStringLiteral("PU \\[\\{\"value\":\"\\{[0-9a-f-]{36}\\}\"\\}\\]"))),
				 qPrintable(out));
	}

	// A cable drawn is written whole, and goes when it is undone
	void cableIsWrittenAndUndone()
	{
		const QString in = writeProject();
		const Dump r = run(QStringLiteral(
				"var p = 'common://10_electric/10_allpole/130_terminals_terminal_strips/bornedouble.elmt';\n"
				"var a = qet.addElement(0, p, 100, 300), b = qet.addElement(0, p, 400, 300);\n"
				"qet.addConductor(0, a, 0, b, 0);\n"
				"var y = qet.terminalPosition(0, a, 0).y;\n"
				"dump('before');\n"
				"qet.log('CABLE ' + qet.addCable(0, 'W1', 'H07', ['BK', 'BU'], '5', 250, y - 50, 250, y + 50));\n"
				"dump('drawn');\n"
				"qet.undo();\n"
				"dump('undone');\n"), in);
		QVERIFY(r.value(QStringLiteral("before")).value(QStringLiteral("cable_value")).isEmpty());
		const QStringList drawn = r.value(QStringLiteral("drawn")).value(QStringLiteral("cable_value"));
		QVERIFY2(drawn.filter(QStringLiteral(" cable 0 designation=W1")).size() == 1, qPrintable(drawn.join('\n')));
		QCOMPARE(drawn.filter(QStringLiteral(" cable 0 type=H07")).size(), 1);
		QCOMPARE(drawn.filter(QStringLiteral(" cable/colors/color ")).size(), 2);
		QVERIFY(r.value(QStringLiteral("undone")).value(QStringLiteral("cable_value")).isEmpty());
	}

	// industrial.qet, saved once by this build so it is in today's form:
	// the same rows opened lazily or built
	void realProjectLazyAndBuiltAgree()
	{
		const QString home = m_dir.filePath(QStringLiteral("resave"));
		QDir().mkpath(home);
		const QString saved = m_dir.filePath(QStringLiteral("industrial.qet"));
		QProcess resave;
		resave.setProcessEnvironment(environment(home));
		resave.start(QStringLiteral(QET_TEST_BINARY_PATH),
					 {QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), saved});
		QVERIFY(resave.waitForFinished(QetTest::timeout(180000)));
		QVERIFY(QFile::exists(saved));

		const Dump lazy = run(QStringLiteral("dump('open');"), saved);
		const Dump built = run(QStringLiteral("dump('open');"), saved, true);
		for (const QString &table : {QStringLiteral("element_value"), QStringLiteral("folio_value")}) {
			const QStringList rows = lazy.value(QStringLiteral("open")).value(table);
			QVERIFY2(rows.size() > 500, qPrintable(table));
			QCOMPARE(built.value(QStringLiteral("open")).value(table), rows);
		}
	}
};

QTEST_GUILESS_MAIN(tst_savevaluerows)
#include "tst_savevaluerows.moc"
