// SPDX-License-Identifier: GPL-2.0-or-later
#include "../../sources/ElementsCollection/terminaluuids.h"

#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QUuid>

// Replacing a symbol's definition in a project must not cost the wires on
// it. A wire is saved against the uuids of the terminals it joins and is
// reattached on load to a terminal with that uuid, or not at all; the
// replacement is another copy of the symbol, whose terminals usually carry
// other uuids. TerminalUuids::keep() carries the old uuids over.
namespace {

QDomElement symbol(QDomDocument &doc, const QList<QStringList> &terminals)
{
	QDomElement element = doc.createElement(QStringLiteral("element"));
	QDomElement definition = doc.createElement(QStringLiteral("definition"));
	QDomElement description = doc.createElement(QStringLiteral("description"));
	element.appendChild(definition);
	definition.appendChild(description);
	for (const QStringList &t : terminals) {
		QDomElement terminal = doc.createElement(QStringLiteral("terminal"));
		terminal.setAttribute(QStringLiteral("x"), t.at(0));
		terminal.setAttribute(QStringLiteral("y"), t.at(1));
		terminal.setAttribute(QStringLiteral("orientation"), t.at(2));
		if (t.size() > 3)
			terminal.setAttribute(QStringLiteral("uuid"), t.at(3));
		description.appendChild(terminal);
	}
	return element;
}

QStringList uuids(const QDomElement &element)
{
	QStringList out;
	const QDomNodeList nodes = element.elementsByTagName(QStringLiteral("terminal"));
	for (int i = 0; i < nodes.size(); ++i)
		out << nodes.at(i).toElement().attribute(QStringLiteral("uuid"));
	return out;
}

QList<QDomElement> embeddedSymbols(const QDomDocument &doc)
{
	QList<QDomElement> out;
	const QDomNodeList nodes = doc.documentElement()
			.firstChildElement(QStringLiteral("collection"))
			.elementsByTagName(QStringLiteral("element"));
	for (int i = 0; i < nodes.size(); ++i)
		out << nodes.at(i).toElement();
	return out;
}

}

class tst_terminaluuids : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The real binary with @p args, in a sandbox of its own (so a running
	// QElectroTech cannot answer instead); false if it failed.
	bool runQet(const QStringList &args, QByteArray *out, QString *log)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), args);
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0)
			return false;
		*log = QString::fromUtf8(proc.readAllStandardError());
		*out = proc.readAllStandardOutput();
		return true;
	}

	// --info on @p project; returns the number of wires loaded and puts
	// what was written on stderr in @p log.
	int loadedWires(const QString &project, QString *log)
	{
		QByteArray out;
		if (!runQet({QStringLiteral("--info"), project}, &out, log))
			return -1;
		const QJsonDocument json = QJsonDocument::fromJson(out.mid(out.indexOf('{')));
		return json.object().value(QStringLiteral("conductors")).toInt(-1);
	}

	// --resave @p in to a new file; returns its path, empty on failure.
	QString resave(const QString &in)
	{
		const QString out = m_dir.filePath(QStringLiteral("resaved%1.qet").arg(m_run));
		QByteArray stdout_;
		QString log;
		if (!runQet({QStringLiteral("--resave"), in, out}, &stdout_, &log))
			return {};
		return out;
	}

	static QDomDocument load(const QString &path)
	{
		QDomDocument doc;
		QFile file(path);
		if (file.open(QIODevice::ReadOnly))
			doc.setContent(&file);
		return doc;
	}

	static QByteArray bytes(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	// 2612_ats_singlephase.qet with each embedded symbol replaced by a copy
	// whose terminals carry new uuids, passed through keep() or not.
	QString replacedSymbols(bool keep_uuids)
	{
		QFile in(QStringLiteral(QET_EXAMPLES_DIR "/2612_ats_singlephase.qet"));
		if (!in.open(QIODevice::ReadOnly)) return {};
		QDomDocument doc;
		if (!doc.setContent(&in)) return {};
		for (const QDomElement &old_symbol : embeddedSymbols(doc)) {
			QDomElement new_symbol = old_symbol.cloneNode().toElement();
			const QDomNodeList terminals = new_symbol.elementsByTagName(QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i)
				terminals.at(i).toElement().setAttribute(QStringLiteral("uuid"),
														 QUuid::createUuid().toString());
			if (keep_uuids)
				TerminalUuids::keep(old_symbol, new_symbol);
			old_symbol.parentNode().replaceChild(new_symbol, old_symbol);
		}
		const QString path = m_dir.filePath(keep_uuids ? QStringLiteral("kept.qet")
													   : QStringLiteral("lost.qet"));
		QFile out(path);
		if (!out.open(QIODevice::WriteOnly)) return {};
		out.write(doc.toByteArray());
		return path;
	}

private slots:
	void samePlaceTakesOldUuid()
	{
		QDomDocument doc;
		const QDomElement old_symbol = symbol(doc, {{"0", "10", "n", "{a}"},
													{"20", "10", "s", "{b}"}});
		QDomElement new_symbol = symbol(doc, {{"20.0", "10", "s", "{y}"},
											  {"0", "10.0", "n", "{x}"}});
		TerminalUuids::keep(old_symbol, new_symbol);
		QCOMPARE(uuids(new_symbol), (QStringList{"{b}", "{a}"}));
	}

	void movedOrNewTerminalKeepsItsOwn()
	{
		QDomDocument doc;
		const QDomElement old_symbol = symbol(doc, {{"0", "10", "n", "{a}"}});
		QDomElement new_symbol = symbol(doc, {{"0", "10", "e", "{x}"},
											  {"0", "20", "n", "{y}"},
											  {"0", "30", "n"}});
		TerminalUuids::keep(old_symbol, new_symbol);
		QCOMPARE(uuids(new_symbol), (QStringList{"{x}", "{y}", ""}));
	}

	void oldWithoutUuidChangesNothing()
	{
		QDomDocument doc;
		const QDomElement old_symbol = symbol(doc, {{"0", "10", "n"}});
		QDomElement new_symbol = symbol(doc, {{"0", "10", "n", "{x}"}});
		TerminalUuids::keep(old_symbol, new_symbol);
		QCOMPARE(uuids(new_symbol), (QStringList{"{x}"}));
	}

	void twoAtOnePlaceMatchedInOrder()
	{
		QDomDocument doc;
		const QDomElement old_symbol = symbol(doc, {{"0", "10", "n", "{a}"},
													{"0", "10", "n", "{b}"}});
		QDomElement new_symbol = symbol(doc, {{"0", "10", "n", "{x}"},
											  {"0", "10", "n", "{y}"},
											  {"0", "10", "n", "{z}"}});
		TerminalUuids::keep(old_symbol, new_symbol);
		QCOMPARE(uuids(new_symbol), (QStringList{"{a}", "{b}", "{z}"}));
	}

	// The new revision moved terminal {a} and put a new one where it was:
	// {a} is still {a}, and the new one is not given {a} a second time.
	void noUuidGivenTwice()
	{
		QDomDocument doc;
		const QDomElement old_symbol = symbol(doc, {{"0", "0", "n", "{a}"}});
		QDomElement new_symbol = symbol(doc, {{"0", "5", "n", "{a}"},
											  {"0", "0", "n", "{c}"}});
		TerminalUuids::keep(old_symbol, new_symbol);
		QCOMPARE(uuids(new_symbol), (QStringList{"{a}", "{c}"}));
	}

	// A terminal that already carries an old uuid is never renamed, even
	// when it moved onto the place of another old terminal.
	void movedTerminalNotRenamed()
	{
		QDomDocument doc;
		const QDomElement old_symbol = symbol(doc, {{"0", "0", "n", "{a}"},
													{"0", "5", "n", "{b}"}});
		QDomElement new_symbol = symbol(doc, {{"0", "5", "n", "{a}"},
											  {"0", "9", "n", "{x}"}});
		TerminalUuids::keep(old_symbol, new_symbol);
		QCOMPARE(uuids(new_symbol), (QStringList{"{a}", "{x}"}));
	}

	// Replacing a whole category: every symbol with a counterpart of the
	// same name, at any depth, keeps its terminal uuids.
	void keepInDirectory()
	{
		QDomDocument doc;
		auto category = [&doc](const QString &name) {
			QDomElement c = doc.createElement(QStringLiteral("category"));
			c.setAttribute(QStringLiteral("name"), name);
			return c;
		};
		auto named = [](QDomElement e, const QString &name) {
			e.setAttribute(QStringLiteral("name"), name);
			return e;
		};

		QDomElement old_dir = category(QStringLiteral("dir"));
		QDomElement old_sub = category(QStringLiteral("sub"));
		old_dir.appendChild(named(symbol(doc, {{"0", "0", "n", "{a}"}}), "top.elmt"));
		old_dir.appendChild(old_sub);
		old_sub.appendChild(named(symbol(doc, {{"0", "0", "n", "{b}"}}), "deep.elmt"));

		QDomElement new_dir = category(QStringLiteral("dir"));
		QDomElement new_sub = category(QStringLiteral("sub"));
		const QDomElement top = named(symbol(doc, {{"0", "0", "n", "{x}"}}), "top.elmt");
		const QDomElement other = named(symbol(doc, {{"0", "0", "n", "{y}"}}), "other.elmt");
		const QDomElement deep = named(symbol(doc, {{"0", "0", "n", "{z}"}}), "deep.elmt");
		new_dir.appendChild(top);
		new_dir.appendChild(other);
		new_dir.appendChild(new_sub);
		new_sub.appendChild(deep);

		TerminalUuids::keepInDirectory(old_dir, new_dir);
		QCOMPARE(uuids(top), QStringList{"{a}"});
		QCOMPARE(uuids(other), QStringList{"{y}"});
		QCOMPARE(uuids(deep), QStringList{"{b}"});
	}

	// The recipe is Terminal::stableUuid()'s, which the project database
	// and the wire uuids worked out from their ends already use: changing
	// it would change every such identity in every project.
	void derivedRecipeUnchanged()
	{
		const QUuid ns(QStringLiteral("{6b1f6d1e-6a1a-5f7e-9a3d-9c0a5b2d7e11}"));
		QCOMPARE(TerminalUuids::derived(0, 10, 2),
				 QUuid::createUuidV5(ns, QStringLiteral("0.0000|10.0000|2")));
		QCOMPARE(TerminalUuids::derived(-2.5, 4, 3, 1),
				 QUuid::createUuidV5(ns, QStringLiteral("-2.5000|4.0000|3|1")));
	}

	// Only terminals without a uuid get one, the derived value; two at one
	// point get distinct ones; a value already used in the symbol is never
	// given again; symbols in sub-categories are reached.
	void fillMissing()
	{
		QDomDocument doc;
		QDomElement root = doc.createElement(QStringLiteral("collection"));
		QDomElement sub = doc.createElement(QStringLiteral("category"));
		root.appendChild(sub);
		const QString taken = TerminalUuids::derived(0, 0, 0).toString();
		const QString own = QStringLiteral("{0f5d4b0c-2f7e-4a55-9a51-8c3a3e1c2d11}");
		QDomElement a = symbol(doc, {{"0", "10", "s"},
									 {"5", "0", "e", own},
									 {"0", "10", "s"},
									 {"0", "0", "n"},
									 {"9", "9", "w", taken}});
		QDomElement b = symbol(doc, {{"1", "2", "w"}});
		root.appendChild(a);
		sub.appendChild(b);

		QCOMPARE(TerminalUuids::fillMissing(root), 4);
		QCOMPARE(uuids(a), (QStringList{
					 TerminalUuids::derived(0, 10, 2).toString(),
					 own,
					 TerminalUuids::derived(0, 10, 2, 1).toString(),
					 TerminalUuids::derived(0, 0, 0, 1).toString(),
					 taken}));
		QCOMPARE(uuids(b), QStringList{TerminalUuids::derived(1, 2, 3).toString()});
		QCOMPARE(TerminalUuids::fillMissing(root), 0);
	}

	// The element editor fills a definition read from a file with exactly
	// what a project fills the same definition with.
	void fillMissingInDefinitionMatchesProject()
	{
		QDomDocument doc;
		const QString own = QStringLiteral("{0f5d4b0c-2f7e-4a55-9a51-8c3a3e1c2d11}");
		QDomElement in_project = symbol(doc, {{"0", "10", "s"},
											  {"0", "10", "s"},
											  {"5", "0", "e", own},
											  {"-3", "0", "w"}});
		QDomElement root = doc.createElement(QStringLiteral("collection"));
		root.appendChild(in_project);
		QDomElement definition = in_project.firstChildElement(QStringLiteral("definition"))
				.cloneNode(true).toElement();

		QCOMPARE(TerminalUuids::fillMissingInDefinition(definition), 3);
		QCOMPARE(TerminalUuids::fillMissing(root), 3);
		QCOMPARE(uuids(definition), uuids(in_project));
		QCOMPARE(uuids(definition).at(2), own);
		QCOMPARE(uuids(definition).at(1), TerminalUuids::derived(0, 10, 2, 1).toString());
		QCOMPARE(TerminalUuids::fillMissingInDefinition(definition), 0);
	}

	// An example whose symbols have no terminal uuids and whose wires are
	// all in the numbered form: once saved, every terminal has a uuid,
	// every wire names its ends by uuid, nothing is lost, and saving again
	// changes nothing.
	void resaveGivesEveryTerminalAUuid()
	{
		const QString original = QStringLiteral(QET_EXAMPLES_DIR "/tremie_vibrante.qet");
		QString log;
		const int wires = loadedWires(original, &log);
		QVERIFY(wires > 0);

		const QString saved = resave(original);
		QVERIFY2(!saved.isEmpty(), "--resave failed");
		const QDomDocument doc = load(saved);
		int terminals = 0;
		for (const QDomElement &e : embeddedSymbols(doc)) {
			for (const QString &uuid : uuids(e)) {
				++terminals;
				QVERIFY2(!QUuid(uuid).isNull(), "a terminal has no uuid");
			}
		}
		QVERIFY(terminals > 0);
		const QDomNodeList conductors = doc.elementsByTagName(QStringLiteral("conductor"));
		QCOMPARE(conductors.size(), wires);
		for (int i = 0; i < conductors.size(); ++i) {
			const QDomElement c = conductors.at(i).toElement();
			QVERIFY2(c.hasAttribute(QStringLiteral("element1"))
					 && c.hasAttribute(QStringLiteral("element2")),
					 "a wire is still in the numbered form");
		}

		QCOMPARE(loadedWires(saved, &log), wires);
		QVERIFY2(!log.contains(QStringLiteral("not loaded")), "a wire was reported lost");
		const QString again = resave(saved);
		QVERIFY2(!again.isEmpty(), "second --resave failed");
		QVERIFY2(bytes(again) == bytes(saved), "the second save changed the file");
	}

	// The uuids written on opening are derived from where each terminal
	// is: a wire saved against one still finds its terminal after the
	// symbol's definition was replaced by one with other terminal uuids.
	// perceuse.qet and industrial.qet have wires on the second of two
	// terminals at one point of a symbol, which get the next occurrence.
	void derivedUuidFoundAfterReplacement_data()
	{
		QTest::addColumn<QString>("project");
		for (const char *name : {"tremie_vibrante.qet", "perceuse.qet", "industrial.qet"})
			QTest::newRow(name) << QStringLiteral(QET_EXAMPLES_DIR "/") + QLatin1String(name);
	}

	void derivedUuidFoundAfterReplacement()
	{
		QFETCH(QString, project);
		const QString saved = resave(project);
		QVERIFY(!saved.isEmpty());
		QString log;
		const int wires = loadedWires(saved, &log);
		QVERIFY(wires > 0);

		QDomDocument doc = load(saved);
		for (QDomElement e : embeddedSymbols(doc)) {
			const QDomNodeList terminals = e.elementsByTagName(QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i)
				terminals.at(i).toElement().setAttribute(QStringLiteral("uuid"),
														 QUuid::createUuid().toString());
		}
		const QString replaced = m_dir.filePath(QStringLiteral("replaced%1.qet").arg(m_run));
		QFile out(replaced);
		QVERIFY(out.open(QIODevice::WriteOnly));
		out.write(doc.toByteArray());
		out.close();

		QCOMPARE(loadedWires(replaced, &log), wires);
		QVERIFY2(!log.contains(QStringLiteral("not loaded")), "a wire was reported lost");
	}

	// The real loader, on a project whose 131 wires are saved against
	// terminal uuids: replaced symbols lose 119 of them unless keep() ran,
	// and the ones lost are reported.
	void wiresSurviveReplacedSymbols()
	{
		QVERIFY(m_dir.isValid());
		QString log;

		const QString lost = replacedSymbols(false);
		QVERIFY(!lost.isEmpty());
		QCOMPARE(loadedWires(lost, &log), 12);
		QVERIFY2(log.contains(QStringLiteral("55 wire(s) not loaded"))
				 && log.contains(QStringLiteral("64 wire(s) not loaded")),
				 "the lost wires were not reported");

		const QString kept = replacedSymbols(true);
		QVERIFY(!kept.isEmpty());
		QCOMPARE(loadedWires(kept, &log), 131);
		QVERIFY2(!log.contains(QStringLiteral("not loaded")), "a wire was reported lost");
	}
};

QTEST_APPLESS_MAIN(tst_terminaluuids)

#include "tst_terminaluuids.moc"
