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

// The sequential numbers of a symbol and of a wire (the lists of a
// <sequentialNumbers> element) are in the project database, one
// sequential_number row per entry, whether the folio was built
// (QET_EAGER_FOLIOS=1) or read from the document (lazy folios).
// QET-SQL-FORMAT S3e.
//
// Runs the real binary (--run) on a small project written here: a symbol
// and a wire carrying numbers, a symbol and a wire carrying none.
namespace {

const QString uuid_a = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QString uuid_b = QStringLiteral("{22222222-2222-4222-8222-222222222222}");
const QString wire_numbered = QStringLiteral("{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa}");
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

QString placed(const QString &uuid, int first_id, const QString &extras, const QString &children)
{
	return QStringLiteral(
				"<element type=\"embed://import/t/box.elmt\""
				" uuid=\"%1\" %2><terminals>"
				"<terminal orientation=\"2\" x=\"0\" y=\"6\" id=\"%3\"/>"
				"<terminal orientation=\"0\" x=\"0\" y=\"-6\" id=\"%4\"/>"
				"</terminals>%5</element>").arg(uuid, extras).arg(first_id).arg(first_id + 1).arg(children);
}

QString project()
{
		// Every part of the symbol's numbers, a few of the wire's: the
		// parts left out are read as one empty entry
	const QString element_numbers = QStringLiteral(
				"<sequentialNumbers><unit>3;4</unit><wrap>a</wrap><unitFolio>5</unitFolio>"
				"<ten>07;08</ten><tenFolio>9</tenFolio><hundred>100</hundred>"
				"<hundredFolio>200</hundredFolio><alpha>x;y</alpha></sequentialNumbers>");
	const QString wire_numbers = QStringLiteral(
				"<sequentialNumbers><unit>1</unit><ten>02</ten></sequentialNumbers>");
	return QStringLiteral(
				"<project title=\"numbers\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements>"
				"<conductors>"
				"<conductor uuid=\"%3\" terminal1=\"0\" terminal2=\"3\" type=\"multi\" num=\"\">%6</conductor>"
				"<conductor uuid=\"%4\" terminal1=\"1\" terminal2=\"2\" type=\"multi\" num=\"\"/>"
				"</conductors>"
				"</diagram>%5</project>")
			.arg(placed(uuid_a, 0, QStringLiteral("x=\"100\" y=\"200\""), element_numbers),
					placed(uuid_b, 2, QStringLiteral("x=\"300\" y=\"200\""), QString()),
					wire_numbered, wire_plain, definition(), wire_numbers);
}

} // namespace

class tst_sequencerows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The rows of sequential_number after opening the project, as
	// "uuid|part|ord|value".
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
				"'SELECT item_uuid, part, ord, value FROM sequential_number "
				"ORDER BY item_uuid, part, ord')));");
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
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
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
				for (const QString &name : {QStringLiteral("item_uuid"), QStringLiteral("part"),
											QStringLiteral("ord"), QStringLiteral("value")})
					cells << text(o.value(name));
				result << cells.join(QLatin1Char('|'));
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

	// Each entry of each list is a row; an item with no numbers has none
	void numbersAreRows()
	{
		const QStringList expected = {
			uuid_a + "|alpha|0|x", uuid_a + "|alpha|1|y",
			uuid_a + "|hundred|0|100", uuid_a + "|hundred_folio|0|200",
			uuid_a + "|ten|0|07", uuid_a + "|ten|1|08", uuid_a + "|ten_folio|0|9",
			uuid_a + "|unit|0|3", uuid_a + "|unit|1|4", uuid_a + "|unit_folio|0|5",
			uuid_a + "|wrap|0|a",
			wire_numbered + "|alpha|0|", wire_numbered + "|hundred|0|",
			wire_numbered + "|hundred_folio|0|", wire_numbered + "|ten|0|02",
			wire_numbered + "|ten_folio|0|", wire_numbered + "|unit|0|1",
			wire_numbered + "|unit_folio|0|", wire_numbered + "|wrap|0|"};
		QCOMPARE(rows(true), expected);
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
	// once so that it carries the uuids lazy opening needs. Its symbols and
	// wires on folios not built come from the document.
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
		QVERIFY(proc.waitForFinished(QetTest::timeout(180000)));
		QVERIFY(QFile::exists(saved));

		QString lazy_log;
		const QStringList lazy = rows(false, saved, &lazy_log);
		const QStringList built = rows(true, saved);
		QVERIFY2(lazy_log.contains(QStringLiteral("Project database filled from the document")),
				 qPrintable(lazy_log.right(2000)));
		qInfo() << "sequential_number rows built:" << built.size() << "lazy:" << lazy.size();
		QVERIFY(built.size() > 100);
		QCOMPARE(lazy.size(), built.size());
		QCOMPARE(lazy, built);
	}
};

QTEST_GUILESS_MAIN(tst_sequencerows)
#include "tst_sequencerows.moc"
