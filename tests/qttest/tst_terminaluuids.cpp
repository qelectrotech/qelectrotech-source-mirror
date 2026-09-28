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

	// --info on @p project in a sandbox of its own; returns the number of
	// wires loaded and puts what was written on stderr in @p log.
	int loadedWires(const QString &project, QString *log)
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
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--info"), project});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0)
			return -1;
		*log = QString::fromUtf8(proc.readAllStandardError());
		const QByteArray out = proc.readAllStandardOutput();
		const QJsonDocument json = QJsonDocument::fromJson(out.mid(out.indexOf('{')));
		return json.object().value(QStringLiteral("conductors")).toInt(-1);
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
