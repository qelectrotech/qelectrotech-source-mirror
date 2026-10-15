// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
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

} // namespace

class tst_movedterminals : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	struct Result { int wires = -1; QString log; };

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
};

QTEST_APPLESS_MAIN(tst_movedterminals)

#include "tst_movedterminals.moc"
