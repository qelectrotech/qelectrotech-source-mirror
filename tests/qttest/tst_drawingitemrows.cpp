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

// The shapes, free texts and tables of a folio are in the project database,
// one drawing_item row per value of what their toXml() writes, whether the
// folio was built (QET_EAGER_FOLIOS=1) or read from the document (lazy
// folios). QET-SQL-FORMAT S3g.
//
// Runs the real binary (--run) on a small project written here: a line, a
// rectangle with a radius, a polygon and an ellipse, a free text and a table.
namespace {

const QString folio_uuid = QStringLiteral("{99999999-9999-4999-8999-999999999999}");
const QString line_uuid = QStringLiteral("{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaa1}");
const QString rect_uuid = QStringLiteral("{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaa2}");
const QString poly_uuid = QStringLiteral("{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaa3}");
const QString ellipse_uuid = QStringLiteral("{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaa4}");
const QString text_uuid = QStringLiteral("{bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbb1}");
const QString table_uuid = QStringLiteral("{cccccccc-cccc-4ccc-8ccc-ccccccccccc1}");

QString shape(const QString &uuid, const QString &type, const QString &geometry, const QString &inside = QString())
{
	return QStringLiteral(
				"<shape uuid=\"%1\" type=\"%2\" z=\"0\" closed=\"0\" is_movable=\"1\" %3>"
				"<pen color=\"#000000\" widthF=\"0.6\" style=\"SolidLine\"/>"
				"<brush color=\"#000000\" style=\"NoBrush\"/>%4</shape>")
			.arg(uuid, type, geometry, inside);
}

QString project()
{
	return QStringLiteral(
				"<project title=\"drawn\" version=\"0.100.1\">"
				"<diagram uuid=\"%1\" title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<inputs><input uuid=\"%2\" x=\"30\" y=\"40\" rotation=\"0\""
				" font=\"Sans,9,-1,5,50,0,0,0,0,0\" text=\"hello\"/></inputs>"
				"<shapes>%3%4%5%6</shapes>"
				"<tables><graphics_table uuid=\"%7\" x=\"50\" y=\"70\" width=\"400\" height=\"200\""
				" name=\"t\" display_n_row=\"5\">"
				"<graphics_header><margins>5;5;10;5</margins></graphics_header>"
				"<model><project_data_base_model><identifier>summary</identifier>"
				"<query>SELECT folio, title FROM project_summary_view</query>"
				"<index00 alignment=\"AlignLeft\" font=\"Sans,9,-1,5,50,0,0,0,0,0\" margins=\"5;5;10;5\"/>"
				"<header_data/></project_data_base_model></model></graphics_table></tables>"
				"</diagram></project>")
			.arg(folio_uuid, text_uuid,
				 shape(line_uuid, QStringLiteral("Line"), QStringLiteral("x1=\"0\" y1=\"0\" x2=\"100\" y2=\"50\"")),
				 shape(rect_uuid, QStringLiteral("Rectangle"),
					   QStringLiteral("x1=\"10\" y1=\"20\" x2=\"110\" y2=\"90\" rx=\"4\" ry=\"6\"")),
				 shape(poly_uuid, QStringLiteral("Polygon"), QString(),
					   QStringLiteral("<points><point x=\"0\" y=\"0\"/><point x=\"40\" y=\"0\"/>"
									  "<point x=\"40\" y=\"30\"/></points>")),
				 shape(ellipse_uuid, QStringLiteral("Ellipse"),
					   QStringLiteral("x1=\"5\" y1=\"5\" x2=\"65\" y2=\"45\"")),
				 table_uuid);
}

} // namespace

class tst_drawingitemrows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The rows of drawing_item after opening the project, as
	// "item|folio|kind|section|ord|name|value", in table order.
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
				"'SELECT item_uuid, folio_uuid, kind, section, ord, name, value "
				"FROM drawing_item ORDER BY item_uuid, section, ord, name')));");
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
				for (const QString &name : {QStringLiteral("item_uuid"), QStringLiteral("folio_uuid"),
											QStringLiteral("kind"), QStringLiteral("section"),
											QStringLiteral("ord"), QStringLiteral("name"),
											QStringLiteral("value")})
					cells << text(o.value(name));
				result << cells.join(QLatin1Char('|'));
			}
		}
		return result;
	}

	static QStringList rowsOf(const QStringList &rows, const QString &uuid)
	{
		QStringList result;
		for (const QString &row : rows)
			if (row.startsWith(uuid)) result << row;
		return result;
	}

	static QString prefix(const QString &uuid, const QString &kind, const QString &section, int ord)
	{
		return uuid + QLatin1Char('|') + folio_uuid + QLatin1Char('|') + kind + QLatin1Char('|')
				+ section + QLatin1Char('|') + QString::number(ord) + QLatin1Char('|');
	}

	// A real project opened lazily, saved once so that it carries the uuids
	// lazy opening needs. Its drawn items on folios not built come from the
	// document.
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
		qInfo() << example << "drawing_item rows:" << built.size() << "lazy:" << lazy.size();
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

	// An item's rows are rewritten by its uuid and a folio's read by the
	// folio's: both are indexed, or a project with many shapes and texts
	// looks through every row for each (a 136-folio one took 6 s to open)
	void drawnRowsAreIndexed()
	{
		const QString js = m_dir.filePath(QStringLiteral("indexes.js"));
		const QString home = m_dir.filePath(QStringLiteral("home-indexes"));
		QDir().mkpath(home);
		QFile s(js);
		QVERIFY(s.open(QIODevice::WriteOnly));
		s.write("qet.log('INDEXES ' + JSON.stringify(qet.query(\"SELECT sql FROM sqlite_master "
				"WHERE type = 'index' AND tbl_name = 'drawing_item' ORDER BY name\")));");
		s.close();
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), js, QStringLiteral(QET_EXAMPLES_DIR "/grafcet.qet")});
		QVERIFY(proc.waitForFinished(120000));
		const QString log = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		QVERIFY2(log.contains(QStringLiteral("ON drawing_item (folio_uuid)"))
				 && log.contains(QStringLiteral("ON drawing_item (item_uuid)")), qPrintable(log.right(600)));
	}

	// Each shape is rows of what it saves: its attributes in name order,
	// the pen and brush as children, the points of a polygon with their order
	void shapesAreRows()
	{
		const QStringList r = rows(true);
		const QStringList rect = rowsOf(r, rect_uuid);
		QVERIFY2(!rect.isEmpty(), qPrintable(r.join('\n')));
		QVERIFY(rect.contains(prefix(rect_uuid, "shape", "shape", 0) + QStringLiteral("type|Rectangle")));
		QVERIFY(rect.contains(prefix(rect_uuid, "shape", "shape", 0) + QStringLiteral("rx|4")));
		QVERIFY(rect.contains(prefix(rect_uuid, "shape", "shape", 0) + QStringLiteral("ry|6")));
		QVERIFY(rect.contains(prefix(rect_uuid, "shape", "shape/pen", 0) + QStringLiteral("widthF|0.6")));
		QVERIFY(rect.contains(prefix(rect_uuid, "shape", "shape/brush", 0) + QStringLiteral("style|NoBrush")));
		QVERIFY(rowsOf(r, line_uuid).contains(prefix(line_uuid, "shape", "shape", 0) + QStringLiteral("x2|100")));
		QVERIFY(rowsOf(r, ellipse_uuid).contains(prefix(ellipse_uuid, "shape", "shape", 0) + QStringLiteral("type|Ellipse")));
		const QStringList poly = rowsOf(r, poly_uuid);
		QVERIFY(poly.contains(prefix(poly_uuid, "shape", "shape/points/point", 0) + QStringLiteral("x|0")));
		QVERIFY(poly.contains(prefix(poly_uuid, "shape", "shape/points/point", 1) + QStringLiteral("x|40")));
		QVERIFY(poly.contains(prefix(poly_uuid, "shape", "shape/points/point", 2) + QStringLiteral("y|30")));
	}

	void textAndTableAreRows()
	{
		const QStringList r = rows(true);
		const QStringList text = rowsOf(r, text_uuid);
		QVERIFY2(!text.isEmpty(), qPrintable(r.join('\n')));
		QVERIFY(text.contains(prefix(text_uuid, "input", "input", 0) + QStringLiteral("x|30")));
		QVERIFY(text.contains(prefix(text_uuid, "input", "input", 0) + QStringLiteral("y|40")));
		const QStringList table = rowsOf(r, table_uuid);
		QVERIFY2(!table.isEmpty(), qPrintable(r.join('\n')));
		QVERIFY(table.contains(prefix(table_uuid, "graphics_table", "graphics_table", 0) + QStringLiteral("name|t")));
		QVERIFY(table.contains(prefix(table_uuid, "graphics_table", "graphics_table", 0) + QStringLiteral("display_n_row|5")));
		QVERIFY(table.contains(prefix(table_uuid, "graphics_table", "graphics_table/graphics_header/margins", 0)
							   + QStringLiteral("#text|5;5;10;5")));
		QVERIFY(table.contains(prefix(table_uuid, "graphics_table", "graphics_table/model/project_data_base_model/identifier", 0)
							   + QStringLiteral("#text|summary")));
	}

	void realProjectLazyAndBuiltAgree()
	{
		lazyAndBuiltAgree(QStringLiteral("industrial.qet"), 100);
	}

	void secondRealProjectLazyAndBuiltAgree()
	{
		lazyAndBuiltAgree(QStringLiteral("2612_ats_singlephase.qet"), 100);
	}
};

QTEST_GUILESS_MAIN(tst_drawingitemrows)
#include "tst_drawingitemrows.moc"
