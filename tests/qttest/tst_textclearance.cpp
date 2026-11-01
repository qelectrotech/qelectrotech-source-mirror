// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// "Keep texts clear of wires" moves a text that sits on a wire the shortest
// way that leaves a gap, and leaves every other text where it is. Run twice
// on the same folio it moves nothing the second time.
//
// Runs the real binary (qet.clearTextsFromWires, the call the command makes)
// on a small project written here: two symbols joined by a wire, with free
// texts on and away from the wire.
namespace {

const QString uuid_a = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QString uuid_b = QStringLiteral("{22222222-2222-4222-8222-222222222222}");

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

// Texts along the wire from the first symbol to the second, and one far away
QString texts(bool on_wire)
{
	QString inputs;
	if (on_wire)
		for (int x = 130 ; x <= 270 ; x += 20)
			inputs += QStringLiteral("<input x=\"%1\" y=\"196\" text=\"on wire\" rotation=\"0\"/>").arg(x);
	inputs += QStringLiteral("<input x=\"20\" y=\"20\" text=\"far away\" rotation=\"0\"/>");
	return QStringLiteral("<inputs>%1</inputs>").arg(inputs);
}

QString project(bool on_wire)
{
	return QStringLiteral(
				"<project title=\"clear\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements>"
				"<conductors><conductor uuid=\"{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa}\""
				" terminal1=\"1\" terminal2=\"2\" type=\"multi\" num=\"\"/></conductors>"
				"%3</diagram>%4</project>")
			.arg(placed(uuid_a, 0, 100), placed(uuid_b, 2, 300), texts(on_wire), definition());
}

} // namespace

class tst_textclearance : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// "first second" moved counts, or an empty string when the run failed
	QString moved(bool on_wire, double distance)
	{
		const QString run = QString::number(m_run++);
		const QString home = m_dir.filePath(QStringLiteral("home") + run);
		QDir().mkpath(home);
		const QString in = m_dir.filePath(QStringLiteral("in") + run + QStringLiteral(".qet"));
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(project(on_wire).toUtf8());
		f.close();
		const QString js = m_dir.filePath(QStringLiteral("script") + run + QStringLiteral(".js"));
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write(QStringLiteral("var a = qet.clearTextsFromWires(0, %1, 60);"
							   "var b = qet.clearTextsFromWires(0, %1, 60);"
							   "qet.log('MOVED ' + a + ' ' + b);").arg(distance).toUtf8());
		s.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		const auto match = QRegularExpression(QStringLiteral("MOVED (-?\\d+) (-?\\d+)")).match(out);
		return match.hasMatch() ? match.captured(1) + QLatin1Char(' ') + match.captured(2) : QString();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// The control: the texts on the wire do move, the far one does not, and
	// a second pass finds nothing left on a wire
	void textsOnTheWireMove()
	{
		const QString result = moved(true, 4);
		QVERIFY(!result.isEmpty());
		const QStringList counts = result.split(QLatin1Char(' '));
			//The wire turns, so not every text along the line is on it
		QVERIFY2(counts.value(0).toInt() >= 5, qPrintable(result));
		QCOMPARE(counts.value(1).toInt(), 0);
	}

	// A folio whose only text is far from every wire has nothing to move
	void textsAwayFromTheWireStay()
	{
		QCOMPARE(moved(false, 4), QStringLiteral("0 0"));
	}
};

QTEST_GUILESS_MAIN(tst_textclearance)
#include "tst_textclearance.moc"
