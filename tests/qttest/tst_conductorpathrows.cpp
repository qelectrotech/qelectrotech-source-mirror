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

// The segments of a wire the user reshaped are in the project database, one
// conductor_path row per segment, whether the folio was built
// (QET_EAGER_FOLIOS=1) or read from the document (lazy folios). A wire that
// was not reshaped has no rows. QET-SQL-FORMAT S3f.
//
// Runs the real binary (--run) on a small project written here: two copies
// of one embedded symbol, joined by two wires -- one with <segment> children
// that fit its terminals and one without.
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

const QString seg1 = QStringLiteral("10.12345678");
const QString seg2 = QStringLiteral("200");
const QString seg3 = QStringLiteral("-30.12345678");

QString project()
{
	return QStringLiteral(
				"<project title=\"path\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements>"
				"<conductors>"
				"<conductor uuid=\"%3\" terminal1=\"0\" terminal2=\"3\" type=\"multi\" num=\"\""
				">"
				"<segment orientation=\"vertical\" length=\"%6\"/>"
				"<segment orientation=\"horizontal\" length=\"%7\"/>"
				"<segment orientation=\"vertical\" length=\"%8\"/>"
				"</conductor>"
				"<conductor uuid=\"%4\" terminal1=\"1\" terminal2=\"2\" type=\"multi\" num=\"\"/>"
				"</conductors>"
				"</diagram>%5</project>")
			.arg(placed(uuid_a, 0, 100), placed(uuid_b, 2, 300), wire_extras, wire_plain, definition(),
				 seg1, seg2, seg3);
}

} // namespace

class tst_conductorpathrows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The rows of conductor_path after opening the project, as
	// "uuid|ord|horizontal|length".
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
				"'SELECT conductor_uuid, ord, horizontal, length "
				"FROM conductor_path ORDER BY conductor_uuid, ord')));");
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
				for (const QString &name : {QStringLiteral("conductor_uuid"), QStringLiteral("ord"),
											QStringLiteral("horizontal"), QStringLiteral("length")})
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

	// The same on a real project opened lazily, saved once so that it
	// carries the uuids lazy opening needs (the project above names its
	// wires' ends by terminal number, so it is always built whole). Its
	// wires on folios not built come from the document.
	void lazyAndBuiltAgree(const QString &example, int least)
	{
		const QString saved = m_dir.filePath(example);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), m_dir.filePath(QStringLiteral("home-resave")));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/") + example, saved});
		QVERIFY(proc.waitForFinished(180000));
		QVERIFY(QFile::exists(saved));

		QString lazy_log;
		const QStringList lazy = rows(false, saved, &lazy_log);
		const QStringList built = rows(true, saved);
		QVERIFY2(lazy_log.contains(QStringLiteral("Project database filled from the document")),
				 qPrintable(lazy_log.right(2000)));
		qInfo() << example << "segment rows:" << built.size() << "lazy:" << lazy.size();
		QVERIFY(built.size() >= least);
		QCOMPARE(lazy.size(), built.size());
		QCOMPARE(lazy, built);
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// The reshaped wire's segments are rows, with the lengths a save writes
	// (six significant digits); the other wire has none
	void pathIsRows()
	{
		const QStringList r = rows(true);
		QVERIFY2(r.size() == 3, qPrintable(r.join('\n')));
		QCOMPARE(r.at(0), wire_extras + QStringLiteral("|0|0|10.1235"));
		QCOMPARE(r.at(1), wire_extras + QStringLiteral("|1|1|200"));
		QCOMPARE(r.at(2), wire_extras + QStringLiteral("|2|0|-30.1235"));
		QVERIFY(rowOf(r, wire_plain).isEmpty());
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

	void realProjectLazyAndBuiltAgree()
	{
		lazyAndBuiltAgree(QStringLiteral("industrial.qet"), 30);
	}

	void secondRealProjectLazyAndBuiltAgree()
	{
		lazyAndBuiltAgree(QStringLiteral("perceuse.qet"), 100);
	}
};

QTEST_GUILESS_MAIN(tst_conductorpathrows)
#include "tst_conductorpathrows.moc"
