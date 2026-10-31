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
#include "qettesttimeout.h"

// A master (a relay, a contactor) can show its contacts its own way: a cross
// or contact symbols, under its label, below the symbol, in the folio footer
// or at a position given by hand. It is an <xrefDisplay> child of the
// element, written only when set, and the project database holds it (the
// element_value rows), whether the folio was built or read from the
// document.
//
// Runs the real binary on a small project written here: one coil with its
// own display, one without.
namespace {

const QString uuid_own = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QString uuid_plain = QStringLiteral("{22222222-2222-4222-8222-222222222222}");

QString definition()
{
	return QStringLiteral(
				"<collection><category name=\"import\"><category name=\"t\">"
				"<element name=\"coil.elmt\">"
				"<definition width=\"30\" hotspot_y=\"21\" version=\"0.100.0\" type=\"element\""
				" link_type=\"master\" hotspot_x=\"15\" height=\"42\">"
				"<uuid uuid=\"{33333333-3333-4333-8333-333333333333}\"/>"
				"<names><name lang=\"en\">coil</name></names>"
				"<kindInformations><kindInformation name=\"type\" show=\"1\">coil</kindInformation></kindInformations>"
				"<informations/>"
				"<description><rect x=\"-5\" y=\"-6\" width=\"10\" height=\"12\" antialias=\"true\""
				" style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
				"</description></definition></element></category></category></collection>");
}

QString placed(const QString &uuid, int x, const QString &display)
{
	return QStringLiteral(
				"<element type=\"embed://import/t/coil.elmt\" uuid=\"%1\" x=\"%2\" y=\"100\" z=\"10\""
				" orientation=\"0\"><terminals/><inputs/>%3</element>").arg(uuid).arg(x).arg(display);
}

QString project(const QString &display)
{
	return QStringLiteral(
				"<project title=\"xref\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements><conductors/></diagram>%3</project>")
			.arg(placed(uuid_own, 100, display), placed(uuid_plain, 300, QString()), definition());
}

const QString own_display = QStringLiteral(
			"<xrefDisplay appearance=\"contacts\" position=\"manual\" width=\"1.5\" gap=\"12\""
			" x=\"120\" y=\"340\" show_unlinked=\"true\"/>");

} // namespace

class tst_xrefdisplay : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QProcessEnvironment environment(bool eager)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		return env;
	}

	QString writeProject(const QString &display)
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(project(display).toUtf8());
		return in;
	}

	// The <xrefDisplay> element of the placed symbol @p uuid after the
	// project is opened and saved again, or a null element
	QDomElement savedDisplay(const QString &display, bool eager, const QString &uuid, bool *saved = nullptr)
	{
		const QString in = writeProject(display);
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		QProcess proc;
		proc.setProcessEnvironment(environment(eager));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		const bool finished = proc.waitForFinished(QetTest::timeout(120000));
		QFile f(out);
		if (saved) *saved = finished && f.open(QIODevice::ReadOnly);
		else if (!finished || !f.open(QIODevice::ReadOnly)) return {};
		static QDomDocument document;
		if (!document.setContent(&f)) return {};
		const QDomNodeList elements = document.elementsByTagName(QStringLiteral("element"));
		for (int i = 0 ; i < elements.size() ; ++i) {
			const QDomElement element = elements.at(i).toElement();
			if (element.attribute(QStringLiteral("uuid")) == uuid)
				return element.firstChildElement(QStringLiteral("xrefDisplay"));
		}
		return {};
	}

	// The element_value rows of the symbols' xrefDisplay, as "uuid|name|value"
	QStringList databaseRows(const QString &display, bool eager)
	{
		const QString in = writeProject(display);
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write("qet.log('ROWS ' + JSON.stringify(qet.query("
				"\"SELECT element_uuid, name, value FROM element_value "
				"WHERE section LIKE 'xrefDisplay%' ORDER BY element_uuid, name\")));");
		s.close();
		QProcess proc;
		proc.setProcessEnvironment(environment(eager));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		QStringList rows;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			for (const QJsonValue &v : QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				rows << o.value(QStringLiteral("element_uuid")).toString() + QLatin1Char('|')
						+ o.value(QStringLiteral("name")).toString() + QLatin1Char('|')
						+ o.value(QStringLiteral("value")).toString();
			}
		}
		return rows;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// What the master was given is saved again as it was
	void displayIsSaved_data()
	{
		QTest::addColumn<bool>("eager");
		QTest::newRow("built") << true;
		QTest::newRow("from the document") << false;
	}

	void displayIsSaved()
	{
		QFETCH(bool, eager);
		const QDomElement display = savedDisplay(own_display, eager, uuid_own);
		QVERIFY(!display.isNull());
		QCOMPARE(display.attribute(QStringLiteral("appearance")), QStringLiteral("contacts"));
		QCOMPARE(display.attribute(QStringLiteral("position")), QStringLiteral("manual"));
		QCOMPARE(display.attribute(QStringLiteral("width")).toDouble(), 1.5);
		QCOMPARE(display.attribute(QStringLiteral("gap")).toDouble(), 12.0);
		QCOMPARE(display.attribute(QStringLiteral("x")).toDouble(), 120.0);
		QCOMPARE(display.attribute(QStringLiteral("y")).toDouble(), 340.0);
		QCOMPARE(display.attribute(QStringLiteral("show_unlinked")), QStringLiteral("true"));
	}

	// A master with no display of its own saves exactly as before
	void nothingIsWrittenWhenUnset()
	{
		bool saved = false;
		const QDomElement display = savedDisplay(own_display, true, uuid_plain, &saved);
		QVERIFY(saved);
		QVERIFY(display.isNull());
	}

	// An unknown position is not a display: the project's setting applies
	void unknownPositionIsDropped()
	{
		bool saved = false;
		const QDomElement display = savedDisplay(
					QStringLiteral("<xrefDisplay appearance=\"contacts\" position=\"sideways\"/>"),
					true, uuid_own, &saved);
		QVERIFY(saved);
		QVERIFY(display.isNull());
	}

	// The project database holds it, built or not
	void databaseHoldsTheDisplay()
	{
		const QStringList built = databaseRows(own_display, true);
		QVERIFY(!built.isEmpty());
		QVERIFY2(built.join(QLatin1Char(';')).contains(uuid_own + QStringLiteral("|position|manual")),
				 qPrintable(built.join(QLatin1Char('\n'))));
		for (const QString &row : built)
			QVERIFY2(!row.startsWith(uuid_plain), qPrintable(row));
		QCOMPARE(databaseRows(own_display, false), built);
	}
};

QTEST_GUILESS_MAIN(tst_xrefdisplay)
#include "tst_xrefdisplay.moc"
