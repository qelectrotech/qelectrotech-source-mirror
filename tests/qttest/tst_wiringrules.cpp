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
	QProcessEnvironment sandbox(bool master_off = false, int application_max_wires = 0)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp%1").arg(m_run));
		const QString settings = m_dir.filePath(QStringLiteral("settings%1").arg(m_run));
		++m_run;
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QDir().mkpath(settings + QStringLiteral("/QElectroTech"));
		if (master_off || application_max_wires) {
			QFile ini(settings + QStringLiteral("/QElectroTech/QElectroTech.ini"));
			if (ini.open(QIODevice::WriteOnly | QIODevice::Text))
				ini.write(QStringLiteral("[diagrameditor]\nwiring_rules_enabled=%1\nwiring_rules_max_wires=%2\n")
						  .arg(master_off ? QStringLiteral("false") : QStringLiteral("true"))
						  .arg(application_max_wires).toUtf8());
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

		// On a fixture with @p rules: a symbol wired to four others is
		// deleted, so QElectroTech rewires the four to keep the potential.
		// Returns the most wires any one of them ends with.
	QString mostWiresAfterDeletingTheHub(const QString &rules)
	{
		const QString project = fixtureWith(rules);
		const QString script_path = m_dir.filePath(QStringLiteral("hub%1.js").arg(m_run));
		QFile script(script_path);
		if (project.isEmpty() || !script.open(QIODevice::WriteOnly))
			return {};
			// Placed on a diagonal, so no two terminals line up and nothing
			// is auto-connected.
		script.write(
			"var p = 'embed://import/probe/v2_fuse.elmt';\n"
			"var hub = qet.addElement(0, p, 400, 400);\n"
			"var others = [];\n"
			"for (var i = 1; i <= 4; ++i) others.push(qet.addElement(0, p, 400 + 70 * i, 400 + 90 * i));\n"
			"others.forEach(function (o) { qet.addConductor(0, hub, 0, o, 0); });\n"
			"qet.deleteElement(0, hub);\n"
			"var count = {};\n"
			"qet.conductorUuids(0).forEach(function (u) {\n"
			"  qet.conductorEnds(0, u).forEach(function (e) { count[e] = (count[e] || 0) + 1; }); });\n"
			"var most = 0;\n"
			"others.forEach(function (o) { most = Math.max(most, count[o + ' terminal 0'] || 0); });\n"
			"qet.log('PROBE ' + most);\n");
		script.close();

		QProcess proc;
		proc.setProcessEnvironment(sandbox());
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

	void chainsOnlyUnderALimit()
	{
		WiringRules::Settings rules;
		QVERIFY(!WiringRules::chainsWires(rules, true));
		rules.one_wire_per_report = true;
		QVERIFY(!WiringRules::chainsWires(rules, true));
		rules.max_wires = 2;
		QVERIFY(WiringRules::chainsWires(rules, true));
		QVERIFY(!WiringRules::chainsWires(rules, false));
	}

	void chainOrder()
	{
		QCOMPARE(WiringRules::chainOrder({}), QList<int>());
		QCOMPARE(WiringRules::chainOrder({QPointF(5, 5)}), QList<int>({0}));

			// Starts top left, then always the nearest along the grid
		const QList<QPointF> row {QPointF(300, 0), QPointF(0, 0), QPointF(200, 0), QPointF(100, 0)};
		QCOMPARE(WiringRules::chainOrder(row), QList<int>({1, 3, 2, 0}));

			// Same x: the higher one starts
		const QList<QPointF> column {QPointF(0, 100), QPointF(0, 0), QPointF(0, 50)};
		QCOMPARE(WiringRules::chainOrder(column), QList<int>({1, 2, 0}));

			// Each index exactly once
		const QList<QPointF> scattered {QPointF(40, 90), QPointF(10, 300), QPointF(220, 10),
										QPointF(10, 10), QPointF(220, 300)};
		QList<int> order = WiringRules::chainOrder(scattered);
		QCOMPARE(order.first(), 3);
		std::sort(order.begin(), order.end());
		QCOMPARE(order, QList<int>({0, 1, 2, 3, 4}));
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

	void warnsWhenARuleIsTurnedOn()
	{
		WiringRules::Settings off;
		WiringRules::Settings limit;
		limit.max_wires = 4;
		WiringRules::Settings report;
		report.one_wire_per_report = true;

		QVERIFY(WiringRules::turnsRuleOn(off, limit));
		QVERIFY(WiringRules::turnsRuleOn(off, report));
		QVERIFY(WiringRules::turnsRuleOn(limit, [&]{ auto s = limit; s.one_wire_per_report = true; return s; }()));
			// Changing a limit that was already on, or turning rules off: no warning
		QVERIFY(!WiringRules::turnsRuleOn(limit, [&]{ auto s = limit; s.max_wires = 2; return s; }()));
		QVERIFY(!WiringRules::turnsRuleOn(limit, off));
		QVERIFY(!WiringRules::turnsRuleOn(off, off));
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

	void deletingASymbolChainsTheWires()
	{
		SKIP_WITHOUT_SCRIPTING;
			// No rule: the four are wired to one of them, as on master
		QCOMPARE(mostWiresAfterDeletingTheHub(QString()), QStringLiteral("3"));

			// A limit: one after another, none gets more than two
		QCOMPARE(mostWiresAfterDeletingTheHub(
					 QStringLiteral("<wiring_rules max_wires_per_terminal=\"4\"/>")),
				 QStringLiteral("2"));
	}
};

QTEST_GUILESS_MAIN(tst_wiringrules)

#include "tst_wiringrules.moc"
