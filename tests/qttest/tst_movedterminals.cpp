// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QUuid>

// A project saves, for every placed symbol, where each terminal docked, and
// its wires name the terminals by that number or by the uuid worked out from
// that place. When the symbol's definition has since been replaced by one
// whose terminal sits a little elsewhere, the wire must still reattach --
// when the match is clear.
//
// Runs the real binary (--resave) on a small project written here: two
// copies of one embedded symbol, joined by one wire from the south terminal
// (the one the definition moves) to the north one (which stays).
namespace {

const QUuid ns(QStringLiteral("{6b1f6d1e-6a1a-5f7e-9a3d-9c0a5b2d7e11}"));

// What TerminalUuids::derived() works out for a terminal at x, y
QString derived(int y, int orientation)
{
	return QUuid::createUuidV5(ns, QStringLiteral("%1|%2|%3")
							   .arg(0.0, 0, 'f', 4).arg(double(y), 0, 'f', 4).arg(orientation))
			.toString();
}

const QString uuid_a = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QString uuid_b = QStringLiteral("{22222222-2222-4222-8222-222222222222}");

// The symbol as the project's embedded collection now holds it. The south
// terminals are at the given y (the project was saved with one at y=10);
// the north one has always been at y=-10.
QString definition(const QList<int> &south_ys)
{
	QString terminals = QStringLiteral(
				"<terminal orientation=\"n\" type=\"Generic\" name=\"n\" x=\"0\" y=\"-10\"/>");
	for (int y : south_ys)
		terminals += QStringLiteral(
					"<terminal orientation=\"s\" type=\"Generic\" name=\"s\" x=\"0\" y=\"%1\"/>").arg(y);
	return QStringLiteral(
				"<collection><category name=\"import\"><category name=\"t\">"
				"<element name=\"box.elmt\">"
				"<definition width=\"30\" hotspot_y=\"21\" version=\"0.100.0\" type=\"element\""
				" link_type=\"simple\" hotspot_x=\"15\" height=\"42\">"
				"<uuid uuid=\"{33333333-3333-4333-8333-333333333333}\"/>"
				"<names><name lang=\"en\">box</name></names>"
				"<kindInformations/><informations/>"
				"<description><rect x=\"-5\" y=\"-6\" width=\"10\" height=\"12\" antialias=\"true\""
				" style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>%1</description>"
				"</definition></element></category></category></collection>").arg(terminals);
}

// One placed symbol as the project saved it: south terminal docked at y=6
// (a terminal at y=10 less the 4-unit stub), north at y=-6.
QString placed(const QString &uuid, int first_id, int x)
{
	return QStringLiteral(
				"<element orientation=\"0\" type=\"embed://import/t/box.elmt\" z=\"10\""
				" uuid=\"%1\" x=\"%2\" y=\"200\"><terminals>"
				"<terminal orientation=\"2\" x=\"0\" y=\"6\" id=\"%3\"/>"
				"<terminal orientation=\"0\" x=\"0\" y=\"-6\" id=\"%4\"/>"
				"</terminals></element>").arg(uuid).arg(x).arg(first_id).arg(first_id + 1);
}

// The project, with the wire naming its ends as given
QString project(const QList<int> &south_ys, const QString &wire_ends)
{
	return QStringLiteral(
				"<project title=\"moved\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements>"
				"<conductors><conductor %3 type=\"multi\" num=\"\"/></conductors>"
				"</diagram>%4</project>")
			.arg(placed(uuid_a, 0, 100), placed(uuid_b, 2, 300), wire_ends, definition(south_ys));
}

QString byId() { return QStringLiteral("terminal1=\"0\" terminal2=\"3\""); }
QString byUuid(int old_south_y)
{
	return QStringLiteral("element1=\"%1\" terminal1=\"%2\" element2=\"%3\" terminal2=\"%4\"")
			.arg(uuid_a, derived(old_south_y, 2), uuid_b, derived(-10, 0));
}


// A symbol with any terminals, for the cases of several terminals moved at
// once. A terminal is its orientation letter and its position.
struct Term { QChar o; int x; int y; };

QString multiDefinition(const QList<Term> &terms)
{
	QString out;
	for (const Term &t : terms)
		out += QStringLiteral("<terminal orientation=\"%1\" type=\"Generic\" name=\"t\""
							  " x=\"%2\" y=\"%3\"/>").arg(t.o).arg(t.x).arg(t.y);
	QString d = definition({});
	// the lone north terminal of definition() is replaced
	d.replace(QRegularExpression(QStringLiteral("<terminal [^>]*/>")), QString());
	return d.replace(QStringLiteral("</description>"), out + QStringLiteral("</description>"));
}

// A placed symbol whose terminals were saved at the given positions (ids
// first_id, first_id + 1, ...). The saved docking point is the position less
// the stub of 4: north y+4, south y-4, east x-4, west x+4.
QString multiPlaced(const QString &uuid, int first_id, int x, const QList<Term> &saved)
{
	QString out;
	int id = first_id;
	for (const Term &t : saved) {
		int dx = t.x, dy = t.y, o = 0;
		switch (t.o.toLatin1()) {
			case 'n': dy += 4; o = 0; break;
			case 'e': dx -= 4; o = 1; break;
			case 's': dy -= 4; o = 2; break;
			default : dx += 4; o = 3; break;
		}
		out += QStringLiteral("<terminal orientation=\"%1\" x=\"%2\" y=\"%3\" id=\"%4\"/>")
				.arg(o).arg(dx).arg(dy).arg(id++);
	}
	return QStringLiteral(
				"<element orientation=\"0\" type=\"embed://import/t/box.elmt\" z=\"10\""
				" uuid=\"%1\" x=\"%2\" y=\"200\"><terminals>%3</terminals></element>")
			.arg(uuid).arg(x).arg(out);
}

// Two copies of the symbol saved with terminals `saved`, now defined with
// `now`; one wire joins each terminal of the first copy to the same terminal
// of the second (by legacy id).
QString multiProject(const QList<Term> &saved, const QList<Term> &now)
{
	QString wires;
	for (int i = 0; i < saved.size(); ++i)
		wires += QStringLiteral("<conductor terminal1=\"%1\" terminal2=\"%2\" type=\"multi\" num=\"\"/>")
				.arg(i).arg(saved.size() + i);
	return QStringLiteral(
				"<project title=\"moved\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements>%1%2</elements><conductors>%3</conductors></diagram>%4</project>")
			.arg(multiPlaced(uuid_a, 0, 100, saved),
				 multiPlaced(uuid_b, saved.size(), 300, saved),
				 wires, multiDefinition(now));
}

} // namespace

class tst_movedterminals : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	struct Result { int wires = -1; QString log; QStringList pairs; };

	// Open and save the project in a sandbox of its own (so a running
	// QElectroTech cannot answer instead): the wires that survive.
	Result resave(const QString &xml)
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(xml.toUtf8());
		f.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0) return {};
		Result r;
		r.log = QString::fromUtf8(proc.readAllStandardError() + proc.readAllStandardOutput());
		QDomDocument doc;
		QFile saved(out);
		if (!saved.open(QIODevice::ReadOnly) || !doc.setContent(&saved)) return {};
		r.wires = doc.elementsByTagName(QStringLiteral("conductor")).size();
		for (int i = 0; i < r.wires; ++i) {
			const QDomElement c = doc.elementsByTagName(QStringLiteral("conductor")).at(i).toElement();
			r.pairs << c.attribute(QStringLiteral("terminal1")) + QLatin1Char('>') + c.attribute(QStringLiteral("terminal2"));
		}
		return r;
	}

	static bool said(const Result &r)
	{
		return r.log.contains(QStringLiteral("matched to it by place"));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// The terminal has not moved: found as before, nothing to report.
	void exactMatchIsUnaffected()
	{
		const Result legacy = resave(project({10}, byId()));
		QCOMPARE(legacy.wires, 1);
		QVERIFY(!said(legacy));
		const Result uuid = resave(project({10}, byUuid(10)));
		QCOMPARE(uuid.wires, 1);
		QVERIFY(!said(uuid));
	}

	void legacyIdWireFollowsATerminalMovedThreeUnits()
	{
		const Result r = resave(project({13}, byId()));
		QCOMPARE(r.wires, 1);
		QVERIFY(said(r));
	}

	void derivedUuidWireFollowsATerminalMovedThreeUnits()
	{
		const Result r = resave(project({13}, byUuid(10)));
		QCOMPARE(r.wires, 1);
		QVERIFY(said(r));
	}

	void movedFiveUnitsStillMatches()
	{
		QCOMPARE(resave(project({15}, byId())).wires, 1);
	}

	void movedSixUnitsDoesNot()
	{
		QCOMPARE(resave(project({16}, byId())).wires, 0);
		QCOMPARE(resave(project({16}, byUuid(10))).wires, 0);
	}

	// Two live terminals in reach of one saved one: no way to tell which.
	void twoCandidatesPairNothing()
	{
		const Result legacy = resave(project({13, 14}, byId()));
		QCOMPARE(legacy.wires, 0);
		QVERIFY(!said(legacy));
		QCOMPARE(resave(project({13, 14}, byUuid(10))).wires, 0);
	}

	// A whole drawing moved by 5: two terminals 10 apart, so each saved
	// one has two live terminals at 5 units. Only one shift fits both.
	void wholeDrawingMovedPairsBothTerminals()
	{
		const Result r = resave(multiProject({{'n', -5, -10}, {'n', 5, -10}},
											 {{'n', -10, -10}, {'n', 0, -10}}));
		QCOMPARE(r.wires, 2);
		QVERIFY(said(r));
	}

	// Two terminals at one place, moved by 1: paired in order.
	void terminalsAtOnePlaceKeepTheirOrder()
	{
		const QList<Term> saved = {{'n', 1, -40}, {'n', 1, -40}};
		const Result r = resave(multiProject(saved, {{'n', 0, -40}, {'n', 0, -40}}));
		QCOMPARE(r.wires, 2);
		QVERIFY(said(r));
		// each wire runs between the same terminal of both copies, and the
		// first saved one took the first live one (the rank-0 uuid)
		QCOMPARE(r.pairs.size(), 2);
		QCOMPARE(r.pairs.at(0), derived(-40, 0) + QLatin1Char('>') + derived(-40, 0));
		QVERIFY(r.pairs.at(1) != r.pairs.at(0));
		const QStringList second = r.pairs.at(1).split(QLatin1Char('>'));
		QCOMPARE(second.at(0), second.at(1));
	}

	// Terminals that moved by different amounts have no common shift:
	// each is still paired by itself.
	void differentShiftsStillPairPerTerminal()
	{
		const Result r = resave(multiProject({{'n', -20, -10}, {'n', 20, -10}},
											 {{'n', -18, -10}, {'n', 23, -10}}));
		QCOMPARE(r.wires, 2);
	}

	// A terminal that moved while another did not leaves a lone saved
	// terminal: no rigid shift is needed, the per-terminal rule pairs it.
	void oneMovedAmongStaysStillPairs()
	{
		QCOMPARE(resave(multiProject({{'n', -20, -10}, {'n', 20, -10}},
									 {{'n', -20, -10}, {'n', 23, -10}})).wires, 2);
	}

	// Still ambiguous: the two saved terminals could each be either live
	// one, and no single shift maps both.
	void ambiguousStillRefuses()
	{
		QCOMPARE(resave(multiProject({{'n', 0, -10}, {'n', 4, -10}},
									 {{'n', 2, -10}, {'n', 6, -10}, {'n', -2, -10}})).wires, 0);
	}
};

QTEST_APPLESS_MAIN(tst_movedterminals)

#include "tst_movedterminals.moc"
