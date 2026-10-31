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

// The texts of a placed symbol and their groups are in the project
// database, one element_text_value row per value of what the symbol saves
// of them, whether its folio was built (QET_EAGER_FOLIOS=1) or read from
// the document (lazy folios). QET-SQL-FORMAT S3l.
//
// Runs the real binary (--run) on a small project written here: two copies
// of one embedded symbol, the first with a user text and an information
// text, the second held in a text group, the second copy with no texts.
namespace {

const QString uuid_a = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QString uuid_b = QStringLiteral("{22222222-2222-4222-8222-222222222222}");
const QString text_user = QStringLiteral("{44444444-4444-4444-8444-444444444444}");
const QString text_info = QStringLiteral("{55555555-5555-4555-8555-555555555555}");

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

QString texts()
{
	return QStringLiteral(
				"<dynamic_texts>"
				"<dynamic_elmt_text x=\"12\" y=\"-4\" rotation=\"90\" uuid=\"%1\" frame=\"true\""
				" text_width=\"40\" font=\"Sans Serif,9,-1,5,50,0,0,0,0,0\" keep_visual_rotation=\"false\""
				" rotation_point_center=\"false\" text_from=\"UserText\" Halignment=\"AlignLeft\""
				" Valignment=\"AlignTop\"><text>hello</text><color>#ff0000</color></dynamic_elmt_text>"
				"<dynamic_elmt_text x=\"20\" y=\"0\" rotation=\"0\" uuid=\"%2\" frame=\"false\""
				" text_width=\"-1\" font=\"Sans Serif,9,-1,5,50,0,0,0,0,0\" keep_visual_rotation=\"true\""
				" rotation_point_center=\"false\" text_from=\"ElementInfo\" Halignment=\"AlignLeft\""
				" Valignment=\"AlignTop\"><text>K1</text><info_name>label</info_name></dynamic_elmt_text>"
				"</dynamic_texts>"
				"<texts_groups><texts_group name=\"grp\" x=\"20\" y=\"0\" alignment=\"AlignLeft\""
				" rotation=\"0\" vertical_adjustment=\"0\" frame=\"false\" hold_to_bottom_page=\"false\">"
				"<texts><text uuid=\"%2\"/></texts></texts_group></texts_groups>").arg(text_user, text_info);
}

QString placed(const QString &uuid, int first_id, const QString &inner)
{
	return QStringLiteral(
				"<element type=\"embed://import/t/box.elmt\" x=\"%5\" y=\"100\" orientation=\"0\""
				" uuid=\"%1\"><terminals>"
				"<terminal orientation=\"2\" x=\"0\" y=\"6\" id=\"%3\"/>"
				"<terminal orientation=\"0\" x=\"0\" y=\"-6\" id=\"%4\"/>"
				"</terminals>"
				"<elementInformations><elementInformation name=\"label\" show=\"1\">K1</elementInformation></elementInformations>"
				"%2</element>").arg(uuid, inner).arg(first_id).arg(first_id + 1).arg(100 + 200 * first_id);
}

QString project()
{
	return QStringLiteral(
				"<project title=\"texts\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements><conductors/>"
				"</diagram>%3</project>")
			.arg(placed(uuid_a, 0, texts()), placed(uuid_b, 2, QString()), definition());
}

} // namespace

class tst_elementtextrows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// What @p script logs with dump('NAME'): the rows of element_text_value
	// at that point, as "uuid|section|ord|name|value", by NAME.
	// @p source: a project file, or the one written here when empty.
	// @p log: what the binary printed.
	QHash<QString, QStringList> run(bool eager, const QString &script,
									const QString &source = QString(), QString *log = nullptr)
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
		s.write("function dump(name) { qet.log('ROWS ' + name + ' ' + JSON.stringify(qet.query("
				"'SELECT element_uuid, section, ord, name, value FROM element_text_value "
				"ORDER BY element_uuid, section, ord, name'))); }\n");
		s.write(script.toUtf8());
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
		if (!proc.waitForFinished(QetTest::timeout(180000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		if (log) *log = out;
		QHash<QString, QStringList> result;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			const QString rest = line.mid(i + 5);
			const int space = rest.indexOf(QLatin1Char(' '));
			QStringList &rows = result[rest.left(space)];
			for (const QJsonValue &v : QJsonDocument::fromJson(rest.mid(space + 1).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				QStringList cells;
				for (const QString &name : {QStringLiteral("element_uuid"), QStringLiteral("section"),
											QStringLiteral("ord"), QStringLiteral("name"),
											QStringLiteral("value")}) {
					const QJsonValue c = o.value(name);
					cells << (c.isString() ? c.toString() : c.isNull() ? QStringLiteral("NULL")
																	: QString::number(c.toDouble()));
				}
				rows << cells.join(QLatin1Char('|'));
			}
		}
		return result;
	}

	// The rows of a symbol with no text and no group: its two empty lists
	static QStringList noTexts(const QString &uuid)
	{
		return {uuid + QStringLiteral("|dynamic_texts|0|#text|"),
				uuid + QStringLiteral("|texts_groups|0|#text|")};
	}

	static QStringList rowsOf(const QStringList &rows, const QString &uuid)
	{
		QStringList result;
		for (const QString &row : rows)
			if (row.startsWith(uuid)) result << row;
		return result;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// Each value of the saved texts and group is a row; the symbol with no
	// texts has its two empty lists only (no texts, which is not the same
	// as none saved)
	void textsAreRows()
	{
		const QStringList r = run(true, QStringLiteral("dump('open');")).value(QStringLiteral("open"));
		const QStringList a = rowsOf(r, uuid_a);
		QVERIFY2(!a.isEmpty(), qPrintable(r.join(QLatin1Char('\n'))));
		QCOMPARE(rowsOf(r, uuid_b), noTexts(uuid_b));
		const QString first = uuid_a + QStringLiteral("|dynamic_texts/dynamic_elmt_text|0|");
		const QString second = uuid_a + QStringLiteral("|dynamic_texts/dynamic_elmt_text|1|");
		for (const QString &row : {first + QStringLiteral("uuid|") + text_user,
								   first + QStringLiteral("frame|true"),
								   first + QStringLiteral("rotation|90"),
								   first + QStringLiteral("text_width|40"),
								   first + QStringLiteral("text_from|UserText"),
								   uuid_a + QStringLiteral("|dynamic_texts/dynamic_elmt_text[0]/text|0|#text|hello"),
								   uuid_a + QStringLiteral("|dynamic_texts/dynamic_elmt_text[0]/color|0|#text|#ff0000"),
								   second + QStringLiteral("uuid|") + text_info,
								   second + QStringLiteral("text_from|ElementInfo"),
								   uuid_a + QStringLiteral("|dynamic_texts/dynamic_elmt_text[1]/info_name|0|#text|label"),
								   uuid_a + QStringLiteral("|texts_groups/texts_group|0|name|grp"),
								   uuid_a + QStringLiteral("|texts_groups/texts_group/texts/text|0|uuid|") + text_info})
			QVERIFY2(a.contains(row), qPrintable(row + QStringLiteral("\n\nnot in\n") + a.join(QLatin1Char('\n'))));
	}

	// Opened with QET_EAGER_FOLIOS or not, the same rows (this project is
	// built whole either way: realProjectLazyAndBuiltAgree() covers the
	// folios not built)
	void documentAndBuildAgree()
	{
		const QString script = QStringLiteral("dump('open');");
		const QStringList built = run(true, script).value(QStringLiteral("open"));
		const QStringList lazy = run(false, script).value(QStringLiteral("open"));
		QVERIFY(!built.isEmpty());
		QCOMPARE(lazy, built);
	}

	// Editing a text, adding one and undoing the addition each write the
	// rows again
	void editsAreWritten()
	{
		const QHash<QString, QStringList> r = run(true, QStringLiteral(
				"var a = '%1', b = '%2';\n"
				"qet.setElementTextProperty(0, a, qet.elementTextIndex(0, a, '%3'), 'text', 'changed');\n"
				"dump('edited');\n"
				"qet.addElementText(0, b, 'text', 'added', 5, 5);\n"
				"dump('added');\n"
				"qet.undo();\n"
				"dump('undone');\n").arg(uuid_a, uuid_b, text_user));
		const QString changed = uuid_a + QStringLiteral("|dynamic_texts/dynamic_elmt_text[0]/text|0|#text|changed");
		QVERIFY2(r.value(QStringLiteral("edited")).contains(changed),
				 qPrintable(r.value(QStringLiteral("edited")).join(QLatin1Char('\n'))));
		QCOMPARE(rowsOf(r.value(QStringLiteral("edited")), uuid_b), noTexts(uuid_b));
		QVERIFY(rowsOf(r.value(QStringLiteral("added")), uuid_b)
				.contains(uuid_b + QStringLiteral("|dynamic_texts/dynamic_elmt_text/text|0|#text|added")));
		QCOMPARE(rowsOf(r.value(QStringLiteral("undone")), uuid_b), noTexts(uuid_b));
		QVERIFY(r.value(QStringLiteral("undone")).contains(changed));
	}

	// The same on a real project opened lazily: industrial.qet, saved
	// once so that it carries the uuids lazy opening needs. Its symbols on
	// folios not built come from the document.
	void realProjectLazyAndBuiltAgree()
	{
		const QString saved = m_dir.filePath(QStringLiteral("industrial.qet"));
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
			//Its own settings too: a label made from the folio's position
			//follows them (industrial.qet: "15-A0" with the defaults)
		const QString home = m_dir.filePath(QStringLiteral("home-resave"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), saved});
		QVERIFY(proc.waitForFinished(QetTest::timeout(180000)));
		QVERIFY(QFile::exists(saved));

		const QString script = QStringLiteral("dump('open');");
		QString lazy_log;
		const QStringList lazy = run(false, script, saved, &lazy_log).value(QStringLiteral("open"));
		const QStringList built = run(true, script, saved).value(QStringLiteral("open"));
		QVERIFY2(lazy_log.contains(QStringLiteral("Project database filled from the document")),
				 qPrintable(lazy_log.right(2000)));
		QVERIFY(built.size() > 1000);
		QCOMPARE(lazy.size(), built.size());
		QCOMPARE(lazy, built);
	}
};

QTEST_GUILESS_MAIN(tst_elementtextrows)
#include "tst_elementtextrows.moc"
