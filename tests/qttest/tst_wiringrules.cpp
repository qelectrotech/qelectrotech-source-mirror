// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

#include "wiringrules.h"

	// The checks that drive QElectroTech through a script (--run) need a
	// build with scripting; without it --run is not an option and the
	// process never exits (QET_HAS_SCRIPTING, top-level CMakeLists.txt).
#ifdef QET_HAS_SCRIPTING
#	define SKIP_WITHOUT_SCRIPTING
#else
#	define SKIP_WITHOUT_SCRIPTING QSKIP("built without scripting: --run is not available")
#endif

// How many wires a terminal may take (discussion #1158). The rules are
// tested on their own; the project setting and the refusal are tested
// through the real binary: --resave keeps a <wiring_rules> element and
// adds none to a project without it, and qet.addConductor() is refused
// past the limit unless the master switch is off.
class tst_wiringrules : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

		// Environment of one sandboxed run; @p master_off writes the master
		// switch off into that run's own settings, @p application_max_wires
		// the limit every project follows unless it sets its own.
	QProcessEnvironment sandbox(bool master_off = false, int application_max_wires = 0,
							   bool application_angled = false)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp%1").arg(m_run));
		const QString settings = m_dir.filePath(QStringLiteral("settings%1").arg(m_run));
		++m_run;
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QDir().mkpath(settings + QStringLiteral("/QElectroTech"));
		if (master_off || application_max_wires || application_angled) {
			QFile ini(settings + QStringLiteral("/QElectroTech/QElectroTech.ini"));
			if (ini.open(QIODevice::WriteOnly | QIODevice::Text))
				ini.write(QStringLiteral("[diagrameditor]\nwiring_rules_enabled=%1\n"
										 "wiring_rules_max_wires=%2\nwiring_rules_angled_branches=%3\n")
						  .arg(master_off ? QStringLiteral("false") : QStringLiteral("true"))
						  .arg(application_max_wires)
						  .arg(application_angled ? QStringLiteral("true") : QStringLiteral("false")).toUtf8());
		}
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("QET_SETTINGS_DIR"), settings);
		env.insert(QStringLiteral("TMPDIR"), tmp);
		return env;
	}

		// The fixture with @p rules inserted before <newdiagrams>, as a new file
	QString fixtureWith(const QString &rules)
	{
		QFile source(QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet"));
		if (!source.open(QIODevice::ReadOnly | QIODevice::Text))
			return {};
		QString text = QString::fromUtf8(source.readAll());
		const int newdiagrams = text.indexOf(QLatin1String("<newdiagrams"));
		if (newdiagrams < 0)
			return {};
		text.insert(newdiagrams, rules + QStringLiteral("\n    "));
		const QString path = m_dir.filePath(QStringLiteral("fixture%1.qet").arg(m_run));
		QFile out(path);
		if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
			return {};
		out.write(text.toUtf8());
		return path;
	}

		// Runs --resave on @p in, returns the saved file's text (empty on failure)
	QString resave(const QString &in)
	{
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		QProcess proc;
		proc.setProcessEnvironment(sandbox());
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0)
			return {};
		QFile file(out);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return {};
		return QString::fromUtf8(file.readAll());
	}

		// Wires a free terminal to a terminal that already has a wire, through
		// qet.addConductor(); returns what the script printed after "PROBE ".
	QString addWireToWiredTerminal(const QString &project, bool master_off,
								   int application_max_wires = 0)
	{
		const QString script_path = m_dir.filePath(QStringLiteral("probe%1.js").arg(m_run));
		QFile script(script_path);
		if (!script.open(QIODevice::WriteOnly))
			return {};
			// The first end of the first conductor is a terminal with a wire.
			// Any terminal of another element with no wire is the other end.
		script.write(
			"var end = qet.conductorEnds(0, qet.conductorUuids(0)[0])[0].split(' terminal ');\n"
			"var wired = {};\n"
			"qet.conductorUuids(0).forEach(function (u) {\n"
			"  qet.conductorEnds(0, u).forEach(function (e) { wired[e] = true; }); });\n"
			"var free = null;\n"
			"qet.elementUuids(0).forEach(function (el) {\n"
			"  if (free || el == end[0]) return;\n"
			"  var n = qet.elementTerminals(0, el).length;\n"
			"  for (var i = 0; i < n && !free; ++i)\n"
			"    if (!wired[el + ' terminal ' + i]) free = [el, i]; });\n"
			"qet.log('PROBE ' + (free ? qet.addConductor(0, end[0], parseInt(end[1]), free[0], free[1])\n"
			"                         : 'nofree'));\n");
		script.close();

		QProcess proc;
		proc.setProcessEnvironment(sandbox(master_off, application_max_wires));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), script_path, project});
		if (!proc.waitForFinished(60000))
			return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput()
											  + proc.readAllStandardError());
		const QString mark = QStringLiteral("PROBE ");
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(mark);
			if (i >= 0)
				return line.mid(i + mark.size()).trimmed();
		}
		return {};
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void limit()
	{
		WiringRules::Settings rules;
		QCOMPARE(WiringRules::limit(rules, true, false), 0);
		QCOMPARE(WiringRules::limit(rules, true, true), 0);

		rules.max_wires = 4;
		QCOMPARE(WiringRules::limit(rules, true, false), 4);
		QCOMPARE(WiringRules::limit(rules, true, true), 4);

		rules.one_wire_per_report = true;
		QCOMPARE(WiringRules::limit(rules, true, true), 1);
		QCOMPARE(WiringRules::limit(rules, true, false), 4);

			// The master switch turns every rule off
		QCOMPARE(WiringRules::limit(rules, false, false), 0);
		QCOMPARE(WiringRules::limit(rules, false, true), 0);
	}

	void hasRoom()
	{
		QVERIFY(WiringRules::hasRoom(0, 0));
		QVERIFY(WiringRules::hasRoom(0, 19));
		QVERIFY(WiringRules::hasRoom(2, 1));
		QVERIFY(!WiringRules::hasRoom(2, 2));
		QVERIFY(!WiringRules::hasRoom(2, 3));
		QVERIFY(!WiringRules::hasRoom(1, 1));
	}

	void angledCorners()
	{
		using V = QVector<QPointF>;
			// Down then right: the corner at (0, 20) becomes a diagonal
		const V wire {QPointF(0, 0), QPointF(0, 20), QPointF(30, 20)};
		QCOMPARE(WiringRules::angledCorners(wire, {QPointF(0, 20)}),
				 V({QPointF(0, 0), QPointF(0, 15), QPointF(5, 20), QPointF(30, 20)}));

			// A corner not listed, or not a corner of this wire, stays
		QCOMPARE(WiringRules::angledCorners(wire, {}), wire);
		QCOMPARE(WiringRules::angledCorners(wire, {QPointF(0, 10)}), wire);
		QCOMPARE(WiringRules::angledCorners(wire, {QPointF(0, 0)}), wire);

			// Never more than half of the shorter side
		const V short_side {QPointF(0, 0), QPointF(0, 4), QPointF(30, 4)};
		QCOMPARE(WiringRules::angledCorners(short_side, {QPointF(0, 4)}),
				 V({QPointF(0, 0), QPointF(0, 2), QPointF(2, 4), QPointF(30, 4)}));

			// Too short to cut: a wire of two points is returned as it is
		const V straight {QPointF(0, 0), QPointF(40, 0)};
		QCOMPARE(WiringRules::angledCorners(straight, {QPointF(40, 0)}), straight);
	}

	void angledOnlyWithTheMasterSwitch()
	{
		WiringRules::Settings rules;
		QVERIFY(!WiringRules::angledBranches(rules, true));
		rules.angled_branches = true;
		QVERIFY(WiringRules::angledBranches(rules, true));
		QVERIFY(!WiringRules::angledBranches(rules, false));
	}

	void projectOverridesApplication()
	{
		WiringRules::Settings application;
		application.max_wires = 2;
		application.one_wire_per_report = true;

			// A project that follows the application gets its rules
		WiringRules::Settings follows;
		QCOMPARE(WiringRules::effective(follows, application).max_wires, 2);
		QVERIFY(WiringRules::effective(follows, application).one_wire_per_report);
		QVERIFY(!WiringRules::effective(follows, application).own);

			// Its own rules win, "no limit" included
		WiringRules::Settings own;
		own.own = true;
		QCOMPARE(WiringRules::effective(own, application).max_wires, 0);
		QVERIFY(!WiringRules::effective(own, application).one_wire_per_report);
		QVERIFY(WiringRules::effective(own, application).own);
		own.max_wires = 6;
		QCOMPARE(WiringRules::effective(own, application).max_wires, 6);
	}

	void xmlRoundTrip()
	{
		QDomDocument doc;
		QDomElement root = doc.createElement(QStringLiteral("project"));
		doc.appendChild(root);

			// Following the application: nothing written, and reading
			// nothing follows the application
		WiringRules::Settings follows;
		follows.max_wires = 3;       // not the project's: not written
		WiringRules::toXml(follows, root);
		QVERIFY(root.firstChildElement().isNull());
		QVERIFY(WiringRules::fromXml(root).isDefault());

			// Its own rules, even "none", are written and read back as its own
		WiringRules::Settings none;
		none.own = true;
		WiringRules::toXml(none, root);
		QCOMPARE(root.firstChildElement().tagName(), QStringLiteral("wiring_rules"));
		QVERIFY(WiringRules::fromXml(root) == none);
		root.removeChild(root.firstChildElement());

		WiringRules::Settings rules;
		rules.own = true;
		rules.max_wires = 2;
		rules.one_wire_per_report = true;
		rules.angled_branches = true;
		WiringRules::toXml(rules, root);
		QCOMPARE(root.firstChildElement().tagName(), QStringLiteral("wiring_rules"));
		QVERIFY(WiringRules::fromXml(root) == rules);
	}

	void savedSettingSurvivesResave()
	{
		const QString plain = resave(QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet"));
		QVERIFY2(!plain.isEmpty(), "--resave failed");
		QVERIFY(!plain.contains(QLatin1String("wiring_rules")));

		const QString with_rules = fixtureWith(
					QStringLiteral("<wiring_rules max_wires_per_terminal=\"2\" one_wire_per_report=\"true\"/>"));
		QVERIFY(!with_rules.isEmpty());
		const QString saved = resave(with_rules);
		QVERIFY2(!saved.isEmpty(), "--resave failed");
		QVERIFY2(saved.contains(QLatin1String("max_wires_per_terminal=\"2\""))
				 && saved.contains(QLatin1String("one_wire_per_report=\"true\"")),
				 "the setting was lost on save");

			// A project's own "no rule" is kept too: it overrides the application's
		const QString own_none = fixtureWith(QStringLiteral("<wiring_rules/>"));
		QVERIFY(!own_none.isEmpty());
		const QString saved_none = resave(own_none);
		QVERIFY2(!saved_none.isEmpty(), "--resave failed");
		QVERIFY2(saved_none.contains(QLatin1String("<wiring_rules/>")),
				 "a project's own rules were lost on save");
	}

	void wirePastTheLimitIsRefused()
	{
		SKIP_WITHOUT_SCRIPTING;
			// Without a rule the wire is drawn, as on master
		QCOMPARE(addWireToWiredTerminal(QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet"), false),
				 QStringLiteral("true"));

			// One wire per terminal: the terminal is full
		const QString limited = fixtureWith(
					QStringLiteral("<wiring_rules max_wires_per_terminal=\"1\"/>"));
		QVERIFY(!limited.isEmpty());
		QCOMPARE(addWireToWiredTerminal(limited, false), QStringLiteral("false"));

			// The master switch off: the project's rule does nothing
		QCOMPARE(addWireToWiredTerminal(limited, true), QStringLiteral("true"));

			// The application's limit applies to a project that sets none...
		const QString plain = QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet");
		QCOMPARE(addWireToWiredTerminal(plain, false, 1), QStringLiteral("false"));
			// ...not to one that sets its own, here "no limit"
		const QString own_none = fixtureWith(QStringLiteral("<wiring_rules/>"));
		QVERIFY(!own_none.isEmpty());
		QCOMPARE(addWireToWiredTerminal(own_none, false, 1), QStringLiteral("true"));
			// ...and the master switch still turns it off
		QCOMPARE(addWireToWiredTerminal(plain, true, 1), QStringLiteral("true"));
	}

		// Junction entities in the DXF export of perceuse.qet, with @p rules
		// in the project and the master switch on or off
	int dxfJunctions(const QString &rules, bool master_off, bool application_angled = false)
	{
		QFile source(QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"));
		if (!source.open(QIODevice::ReadOnly | QIODevice::Text))
			return -1;
		QString text = QString::fromUtf8(source.readAll());
		text.insert(text.indexOf(QLatin1String("<newdiagrams")), rules);
		const QString project = m_dir.filePath(QStringLiteral("perceuse%1.qet").arg(m_run));
		QFile out(project);
		if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
			return -1;
		out.write(text.toUtf8());
		out.close();

		const QString dxf_dir = m_dir.filePath(QStringLiteral("dxf%1").arg(m_run));
		QDir().mkpath(dxf_dir);
		const QString script_path = m_dir.filePath(QStringLiteral("dxf%1.js").arg(m_run));
		QFile script(script_path);
		if (!script.open(QIODevice::WriteOnly))
			return -1;
		script.write(QStringLiteral("qet.exportDxf('%1', false);\n").arg(dxf_dir).toUtf8());
		script.close();

		QProcess proc;
		proc.setProcessEnvironment(sandbox(master_off, 0, application_angled));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), script_path, project});
		if (!proc.waitForFinished(120000))
			return -1;
		int count = 0;
		const QStringList files = QDir(dxf_dir).entryList({QStringLiteral("*.dxf")});
		for (const QString &name : files) {
			QFile dxf(QDir(dxf_dir).filePath(name));
			if (!dxf.open(QIODevice::ReadOnly | QIODevice::Text))
				return -1;
				// Once per file in the layer table, then once per junction
			count += QString::fromUtf8(dxf.readAll()).count(QLatin1String("\nQET_JUNCTIONS\n")) - 1;
		}
		return files.isEmpty() ? -1 : count;
	}

		// Angled branches replace the junction dots, in the DXF export as on
		// the folio, unless the master switch is off.
	void angledBranchesReplaceDots()
	{
		const int dots = dxfJunctions(QString(), false);
		QVERIFY2(dots > 100, qPrintable(QString::number(dots)));
		const QString angled = QStringLiteral("<wiring_rules branches=\"angled\"/>\n    ");
		QCOMPARE(dxfJunctions(angled, false), 0);
		QCOMPARE(dxfJunctions(angled, true), dots);

			// Set for the application: a project without its own rules follows
			// it, one with its own (here dots) does not
		QCOMPARE(dxfJunctions(QString(), false, true), 0);
		QCOMPARE(dxfJunctions(QStringLiteral("<wiring_rules/>\n    "), false, true), dots);
	}
};

QTEST_GUILESS_MAIN(tst_wiringrules)

#include "tst_wiringrules.moc"
