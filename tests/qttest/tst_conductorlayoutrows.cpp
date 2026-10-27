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

// Where a wire sits and how its text was placed (x, y, freezeLabel, userx,
// usery, rotation) is in the project database, one conductor_layout row per
// wire, whether its folio was built (QET_EAGER_FOLIOS=1) or read from the
// document (lazy folios). QET-SQL-FORMAT S3c.
//
// Runs the real binary (--run) on a small project written here: two copies
// of one embedded symbol, joined by two wires -- one with a frozen label and
// a text the user moved and rotated, one with none of that.
namespace {

const QString uuid_a = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QString uuid_b = QStringLiteral("{22222222-2222-4222-8222-222222222222}");
const QString wire_extras = QStringLiteral("{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa}");
const QString wire_plain = QStringLiteral("{bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb}");

QString definition()
{
	return QStringLiteral(
				"<collection><category name=\"import\"><category name=\"t\">"
				"<element name=\"box.elmt\">"
				"<definition width=\"30\" hotspot_y=\"21\" version=\"0.100.0\" type=\"element\""
				" link_type=\"simple\" hotspot_x=\"15\" height=\"42\">"
				"<uuid uuid=\"{33333333-3333-4333-8333-333333333333}\"/>"
				"<names><name lang=\"en\">box</name></names>"
				"<kindInformations/><informations/>"
				"<description><rect x=\"-5\" y=\"-6\" width=\"10\" height=\"12\" antialias=\"true\""
				" style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
				"<terminal orientation=\"n\" type=\"Generic\" name=\"n\" x=\"0\" y=\"-10\"/>"
				"<terminal orientation=\"s\" type=\"Generic\" name=\"s\" x=\"0\" y=\"10\"/>"
				"</description></definition></element></category></category></collection>");
}

QString placed(const QString &uuid, int first_id, int x)
{
	return QStringLiteral(
				"<element orientation=\"0\" type=\"embed://import/t/box.elmt\" z=\"10\""
				" uuid=\"%1\" x=\"%2\" y=\"200\"><terminals>"
				"<terminal orientation=\"2\" x=\"0\" y=\"6\" id=\"%3\"/>"
				"<terminal orientation=\"0\" x=\"0\" y=\"-6\" id=\"%4\"/>"
				"</terminals></element>").arg(uuid).arg(x).arg(first_id).arg(first_id + 1);
}

QString project()
{
	return QStringLiteral(
				"<project title=\"layout\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements>"
				"<conductors>"
				"<conductor uuid=\"%3\" terminal1=\"0\" terminal2=\"3\" type=\"multi\" num=\"\""
				" x=\"5\" y=\"7\" freezeLabel=\"true\" userx=\"10\" usery=\"20\" rotation=\"90\"/>"
				"<conductor uuid=\"%4\" terminal1=\"1\" terminal2=\"2\" type=\"multi\" num=\"\"/>"
				"</conductors>"
				"</diagram>%5</project>")
			.arg(placed(uuid_a, 0, 100), placed(uuid_b, 2, 300), wire_extras, wire_plain, definition());
}

} // namespace

class tst_conductorlayoutrows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The rows of conductor_layout after opening the project, as
	// "uuid|x|y|freeze|text_x|text_y|text_rotation" (NULL for a null; the
	// query turns it into text, as the script API gives a null as "").
	// @p source: a project file, or the one written here when empty.
	// @p log: what the binary printed.
	QStringList rows(bool eager, const QString &source = QString(), QString *log = nullptr)
	{
		QString in = source;
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		if (in.isEmpty()) {
			in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
			QFile f(in);
			if (!f.open(QIODevice::WriteOnly)) return {};
			f.write(project().toUtf8());
			f.close();
		}
		++m_run;
		QDir().mkpath(home);
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write("qet.log('ROWS ' + JSON.stringify(qet.query("
				"'SELECT conductor_uuid, x, y, freeze_label, "
				"CASE WHEN text_x IS NULL THEN \\'NULL\\' ELSE text_x END AS text_x, "
				"CASE WHEN text_y IS NULL THEN \\'NULL\\' ELSE text_y END AS text_y, "
				"CASE WHEN text_rotation IS NULL THEN \\'NULL\\' ELSE text_rotation END AS text_rotation "
				"FROM conductor_layout ORDER BY conductor_uuid')));");
		s.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(120000)) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		if (log) *log = out;
		QStringList result;
		auto text = [](const QJsonValue &v) {
			return v.isNull() || v.isUndefined() ? QStringLiteral("NULL")
					: v.isString() ? v.toString() : QString::number(v.toDouble());
		};
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			for (const QJsonValue &v : QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				QStringList cells;
				for (const QString &name : {QStringLiteral("conductor_uuid"), QStringLiteral("x"),
											QStringLiteral("y"), QStringLiteral("freeze_label"),
											QStringLiteral("text_x"), QStringLiteral("text_y"),
											QStringLiteral("text_rotation")})
					cells << text(o.value(name));
				result << cells.join(QLatin1Char('|'));
			}
		}
		return result;
	}

	static QString rowOf(const QStringList &rows, const QString &uuid)
	{
		for (const QString &row : rows)
			if (row.startsWith(uuid)) return row;
		return {};
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// The wire's text placement is a row, the other wire's text columns are NULL
	void layoutIsARow()
	{
		const QStringList r = rows(true);
		QCOMPARE(r.size(), 2);
		const QString extras = rowOf(r, wire_extras);
		QVERIFY2(extras.endsWith(QStringLiteral("|1|10|20|90")), qPrintable(r.join('\n')));
		const QString plain = rowOf(r, wire_plain);
		QVERIFY2(plain.endsWith(QStringLiteral("|0|NULL|NULL|NULL")), qPrintable(r.join('\n')));
	}

	// Opened with QET_EAGER_FOLIOS or not, the same rows (this project is
	// built whole either way: realProjectLazyAndBuiltAgree() covers the
	// folios not built)
	void documentAndBuildAgree()
	{
		const QStringList built = rows(true);
		const QStringList lazy = rows(false);
		QVERIFY(!built.isEmpty());
		QCOMPARE(lazy, built);
	}

	// The same on a real project opened lazily: industrial.qet, saved
	// once so that it carries the uuids lazy opening needs (the project
	// above names its wires' ends by terminal number, so it is always
	// built whole). Its wires on folios not built come from the document.
	void realProjectLazyAndBuiltAgree()
	{
		const QString saved = m_dir.filePath(QStringLiteral("industrial.qet"));
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), m_dir.filePath(QStringLiteral("home-resave")));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), saved});
		QVERIFY(proc.waitForFinished(180000));
		QVERIFY(QFile::exists(saved));

		QString lazy_log;
		const QStringList lazy = rows(false, saved, &lazy_log);
		const QStringList built = rows(true, saved);
		QVERIFY2(lazy_log.contains(QStringLiteral("Project database filled from the document")),
				 qPrintable(lazy_log.right(2000)));
		QVERIFY(built.size() > 100);
		QCOMPARE(lazy.size(), built.size());
		QCOMPARE(lazy, built);
	}
};

QTEST_GUILESS_MAIN(tst_conductorlayoutrows)
#include "tst_conductorlayoutrows.moc"
