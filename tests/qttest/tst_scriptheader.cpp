// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QAction>

#include "scripting/scriptheader.h"
#include "shortcutmanager.h"

// The header that turns a stored script into a button, and taking a
// command out of ShortcutManager again when its script is deleted.
class tst_scriptheader : public QObject
{
	Q_OBJECT

	static QString script(const QString &lines)
	{
		return QStringLiteral("// ==QETScript==\n") + lines
				+ QStringLiteral("// ==/QETScript==\nqet.log('x');\n");
	}

private slots:
	void fullHeader()
	{
		const ScriptHeader h = ScriptHeader::parse(script(QStringLiteral(
			"// @name     Add revision note\n"
			"// @icon     note.svg\n"
			"// @tooltip  Puts a note on the folio\n"
			"// @shortcut Ctrl+Alt+R\n"
			"// @context  selection\n"
			"// @api      1\n")), QStringLiteral("add-note"));
		QVERIFY2(h.isValid(), qPrintable(h.error));
		QCOMPARE(h.id, QStringLiteral("add-note"));
		QCOMPARE(h.name, QStringLiteral("Add revision note"));
		QCOMPARE(h.icon, QStringLiteral("note.svg"));
		QCOMPARE(h.tooltip, QStringLiteral("Puts a note on the folio"));
		QCOMPARE(h.shortcut, QStringLiteral("Ctrl+Alt+R"));
		QCOMPARE(h.context, QStringLiteral("selection"));
		QCOMPARE(h.api, 1);
	}

	void nameAloneIsEnough()
	{
		const ScriptHeader h = ScriptHeader::parse(
			script(QStringLiteral("// @name Just a name\n")), QStringLiteral("a"));
		QVERIFY2(h.isValid(), qPrintable(h.error));
		QCOMPARE(h.context, QStringLiteral("canvas"));
		QVERIFY(h.icon.isEmpty());
	}

	void refused_data()
	{
		QTest::addColumn<QString>("text");
		QTest::addColumn<QString>("error");
		QTest::newRow("no header") << QStringLiteral("qet.log('x');\n")
					   << QStringLiteral("no // ==QETScript== header");
		QTest::newRow("no name") << script(QStringLiteral("// @icon x.svg\n"))
					 << QStringLiteral("@name is required");
		QTest::newRow("typo") << script(QStringLiteral("// @name A\n// @shortcutt Ctrl+K\n"))
				      << QStringLiteral("unknown header key @shortcutt");
		QTest::newRow("context") << script(QStringLiteral("// @name A\n// @context wires\n"))
					 << QStringLiteral("@context must be one of: canvas, selection, conductor");
		QTest::newRow("api") << script(QStringLiteral("// @name A\n// @api 2\n"))
				     << QStringLiteral("@api 2 is not supported by this version (1 is)");
	}

	void refused()
	{
		QFETCH(QString, text);
		QFETCH(QString, error);
		const ScriptHeader h = ScriptHeader::parse(text, QStringLiteral("a"));
		QVERIFY(!h.isValid());
		QCOMPARE(h.error, error);
	}

	// Only the header block counts: a "// @name" further down is code.
	void onlyTheBlock()
	{
		const ScriptHeader h = ScriptHeader::parse(
			script(QStringLiteral("// @name Real\n"))
			+ QStringLiteral("// @name Not this one\n// @bogus key\n"),
			QStringLiteral("a"));
		QVERIFY2(h.isValid(), qPrintable(h.error));
		QCOMPARE(h.name, QStringLiteral("Real"));
	}

	// What the script manager writes reads back as what was typed, and
	// leaves the script's code as it was.
	void composeReadsBack()
	{
		const QString body = QStringLiteral("var f = qet.currentFolio();\nqet.addText(f, 'x', 0, 0);\n");
		const QString text = ScriptHeader::compose(
			QStringLiteral("  Add   note "), QStringLiteral("note.svg"),
			QStringLiteral("A tip"), QStringLiteral("Ctrl+Alt+N"),
			QStringLiteral("conductor"), body);
		const ScriptHeader h = ScriptHeader::parse(text, QStringLiteral("add-note"));
		QVERIFY2(h.isValid(), qPrintable(h.error));
		QCOMPARE(h.name, QStringLiteral("Add note"));
		QCOMPARE(h.icon, QStringLiteral("note.svg"));
		QCOMPARE(h.tooltip, QStringLiteral("A tip"));
		QCOMPARE(h.shortcut, QStringLiteral("Ctrl+Alt+N"));
		QCOMPARE(h.context, QStringLiteral("conductor"));
		QCOMPARE(ScriptHeader::bodyOf(text), body);

			// empty fields and the default context are left out
		const QString bare = ScriptHeader::compose(QStringLiteral("A"), QString(), QString(),
							   QString(), QStringLiteral("canvas"),
							   QStringLiteral("qet.log(1);"));
		QCOMPARE(bare, QStringLiteral("// ==QETScript==\n// @name     A\n// @api      1\n"
					      "// ==/QETScript==\nqet.log(1);\n"));
	}

	void bodyWithoutHeader()
	{
		QCOMPARE(ScriptHeader::bodyOf(QStringLiteral("qet.log(1);\n")),
			 QStringLiteral("qet.log(1);\n"));
	}

	void idFor()
	{
		QCOMPARE(ScriptHeader::idFor(QStringLiteral("Numéroter les fils !"), {}),
			 QStringLiteral("numeroter-les-fils"));
		QCOMPARE(ScriptHeader::idFor(QStringLiteral("Add note"),
					     {QStringLiteral("add-note"), QStringLiteral("add-note-2")}),
			 QStringLiteral("add-note-3"));
		QCOMPARE(ScriptHeader::idFor(QStringLiteral("!!!"), {}), QStringLiteral("script"));
	}

	// Two windows register one id; the id stays until both are gone, and a
	// script that comes back under a new name is listed by that name.
	void unregisterAction()
	{
		ShortcutManager &m = ShortcutManager::instance();
		const QString id = QStringLiteral("diagrameditor.script.tst");
		auto listed = [&m, &id]() -> QString {
			for (const ShortcutManager::ShortcutInfo &info : m.allShortcuts())
				if (info.id == id) return info.description;
			return QString();
		};

		QAction a(QStringLiteral("Old name")), b(QStringLiteral("Old name"));
		m.registerAction(&a, id, QStringLiteral("Scripts"), QKeySequence());
		m.registerAction(&b, id, QStringLiteral("Scripts"), QKeySequence());
		QCOMPARE(listed(), QStringLiteral("Old name"));

		m.unregisterAction(&a, id);
		QCOMPARE(listed(), QStringLiteral("Old name"));
		QCOMPARE(m.action(id, nullptr), &b);

		m.unregisterAction(&b, id);
		QVERIFY(listed().isEmpty());
		QCOMPARE(m.action(id, nullptr), nullptr);

		QAction c(QStringLiteral("New name"));
		m.registerAction(&c, id, QStringLiteral("Scripts"), QKeySequence());
		QCOMPARE(listed(), QStringLiteral("New name"));
		m.unregisterAction(&c, id);

			// unknown id: nothing to do, nothing breaks
		m.unregisterAction(&c, QStringLiteral("diagrameditor.script.none"));
	}
};

QTEST_MAIN(tst_scriptheader)
#include "tst_scriptheader.moc"
