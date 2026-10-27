// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// The project's own symbols, their categories and its title-block templates
// are in the project database, each whole as the project saves it, one row
// each (symbol_def, symbol_category, titleblock_template). QET-SQL-FORMAT S3m.
//
// Runs the real binary (--run) on a small project written here, holding one
// embedded symbol, and on industrial.qet.
namespace {

const QString symbol_uuid = QStringLiteral("{33333333-3333-4333-8333-333333333333}");

QString project()
{
	return QStringLiteral(
				"<project title=\"collections\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements/><conductors/></diagram>"
				"<collection><category name=\"import\">"
				"<names><name lang=\"en\">Imported</name></names>"
				"<category name=\"t\">"
				"<element name=\"box.elmt\">"
				"<definition width=\"30\" hotspot_y=\"21\" version=\"0.100.0\" type=\"element\""
				" link_type=\"simple\" hotspot_x=\"15\" height=\"42\">"
				"<uuid uuid=\"%1\"/>"
				"<names><name lang=\"en\">box</name></names>"
				"<kindInformations/><informations/>"
				"<description><rect x=\"-5\" y=\"-6\" width=\"10\" height=\"12\" antialias=\"true\""
				" style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
				"</description></definition></element></category></category></collection>"
				"</project>").arg(symbol_uuid);
}

} // namespace

class tst_projectcollectionrows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	struct Row { QString content; QString hash; };
	// table ("symbol_def", "symbol_category", "titleblock_template") ->
	// path or name -> row
	using Rows = QHash<QString, QHash<QString, Row>>;

	// What @p script logs with dump('NAME'), by NAME.
	// @p source: a project file, or the one written here when empty.
	QHash<QString, Rows> run(const QString &script, const QString &source = QString(), QString *log = nullptr)
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
		s.write("function dump(name) { qet.log('ROWS ' + name + ' ' + JSON.stringify(qet.query("
				"\"SELECT 'symbol_def' AS t, path AS k, content, hash FROM symbol_def UNION ALL "
				"SELECT 'symbol_category', path, content, hash FROM symbol_category UNION ALL "
				"SELECT 'titleblock_template', name, content, hash FROM titleblock_template\"))); }\n");
		s.write(script.toUtf8());
		s.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
			//The shipped symbols and title blocks, so common:// resolves
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/"),
					QStringLiteral("--common-tbt-dir=" QET_TITLEBLOCKS_DIR "/"),
					QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(180000)) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		if (log) *log = out;
		QHash<QString, Rows> result;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			const QString rest = line.mid(i + 5);
			const int space = rest.indexOf(QLatin1Char(' '));
			Rows &rows = result[rest.left(space)];
			for (const QJsonValue &v : QJsonDocument::fromJson(rest.mid(space + 1).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				rows[o.value(QStringLiteral("t")).toString()].insert(
							o.value(QStringLiteral("k")).toString(),
							{o.value(QStringLiteral("content")).toString(), o.value(QStringLiteral("hash")).toString()});
			}
		}
		return result;
	}

	static QString sha256(const QString &text)
	{
		return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
	}

	// The paths of the symbols and categories in the <collection> of the
	// project file @p path, and the names of its templates
	static void fileCollection(const QString &path, QStringList &symbols, QStringList &categories,
							   QStringList &templates)
	{
		QFile f(path);
		QDomDocument document;
		if (!f.open(QIODevice::ReadOnly) || !document.setContent(&f)) return;
		std::function<void(const QDomElement &, const QString &)> walk =
				[&](const QDomElement &parent, const QString &at) {
			for (QDomElement child = parent.firstChildElement() ; !child.isNull()
				 ; child = child.nextSiblingElement()) {
				const QString p = (at.isEmpty() ? QString() : at + QLatin1Char('/'))
						+ child.attribute(QStringLiteral("name"));
				if (child.tagName() == QLatin1String("element")) symbols << p;
				else if (child.tagName() == QLatin1String("category")) { categories << p; walk(child, p); }
			}
		};
		walk(document.documentElement().firstChildElement(QStringLiteral("collection")), QString());
		const QDomElement tbt = document.documentElement().firstChildElement(QStringLiteral("titleblocktemplates"));
		for (QDomElement t = tbt.firstChildElement(QStringLiteral("titleblocktemplate")) ; !t.isNull()
			 ; t = t.nextSiblingElement(QStringLiteral("titleblocktemplate")))
			templates << t.attribute(QStringLiteral("name"));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// The symbol is one row, whole; each category one row with its names
	void collectionIsRows()
	{
		const Rows r = run(QStringLiteral("dump('open');")).value(QStringLiteral("open"));
		const QHash<QString, Row> symbols = r.value(QStringLiteral("symbol_def"));
		QCOMPARE(symbols.keys(), QStringList{QStringLiteral("import/t/box.elmt")});
		const Row box = symbols.value(QStringLiteral("import/t/box.elmt"));
		QVERIFY2(box.content.startsWith(QStringLiteral("<element name=\"box.elmt\">")), qPrintable(box.content));
		QVERIFY(box.content.contains(symbol_uuid));
		QVERIFY(box.content.contains(QStringLiteral("<rect ")));
		QCOMPARE(box.hash, sha256(box.content));

		QHash<QString, Row> categories = r.value(QStringLiteral("symbol_category"));
		QStringList paths = categories.keys();
		paths.sort();
		QCOMPARE(paths, (QStringList{QStringLiteral("import"), QStringLiteral("import/t")}));
		const QString import = categories.value(QStringLiteral("import")).content;
		QVERIFY2(import.contains(QStringLiteral("Imported")) && !import.contains(QStringLiteral("box.elmt")),
				 qPrintable(import));
		QVERIFY(r.value(QStringLiteral("titleblock_template")).isEmpty());
	}

	// Two symbols under one path: QElectroTech uses the first
	// (XmlElementCollection::child()), so the row is the first. A .qetdb
	// written from the rows would otherwise show the other one.
	void firstOfTwoUnderOnePath()
	{
		const QString one = QStringLiteral("<element name=\"box.elmt\">");
		QString text = project();
		const int at = text.indexOf(one);
		const int end = text.indexOf(QStringLiteral("</element>"), at) + int(qstrlen("</element>"));
		const QString second = text.mid(at, end - at).replace(QStringLiteral("<rect "), QStringLiteral("<ellipse "));
		text.insert(end, second);
		const QString in = m_dir.filePath(QStringLiteral("two-under-one-path.qet"));
		QFile f(in);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(text.toUtf8());
		f.close();

		const Rows r = run(QStringLiteral("dump('open');"), in).value(QStringLiteral("open"));
		const QString box = r.value(QStringLiteral("symbol_def")).value(QStringLiteral("import/t/box.elmt")).content;
		QVERIFY2(box.contains(QStringLiteral("<rect ")) && !box.contains(QStringLiteral("<ellipse ")), qPrintable(box));
	}

	// A symbol placed from the library and a template embedded each add
	// their rows
	void additionsAreWritten()
	{
			//Read first: the rows are written when read, so an addition
			//made before the first read would be in them however it came
		const QHash<QString, Rows> r = run(QStringLiteral(
				"dump('before');\n"
				"var p = 'common://10_electric/10_allpole/310_relays_contactors_contacts/"
				"01_coils/bobine_ka_a_remanence.elmt';\n"
				"qet.log('ADDED ' + qet.addElement(0, p, 100, 100));\n"
				"qet.log('EMBEDDED ' + qet.embedTitleBlockTemplate('DIN_A4'));\n"
				"dump('after');\n"));
		QCOMPARE(r.value(QStringLiteral("before")).value(QStringLiteral("symbol_def")).size(), 1);
		const Rows after = r.value(QStringLiteral("after"));
		const QString added = QStringLiteral(
				"import/10_electric/10_allpole/310_relays_contactors_contacts/01_coils/bobine_ka_a_remanence.elmt");
		QVERIFY2(after.value(QStringLiteral("symbol_def")).contains(added),
				 qPrintable(after.value(QStringLiteral("symbol_def")).keys().join(QLatin1Char('\n'))));
		QVERIFY(after.value(QStringLiteral("symbol_category"))
				.contains(QStringLiteral("import/10_electric/10_allpole")));
		const Row din = after.value(QStringLiteral("titleblock_template")).value(QStringLiteral("DIN_A4"));
		QVERIFY2(din.content.startsWith(QStringLiteral("<titleblocktemplate")), qPrintable(din.content.left(200)));
		QCOMPARE(din.hash, sha256(din.content));
	}

	// industrial.qet: a row for every symbol, category and template its
	// file holds, and nothing else
	void realProjectMatchesTheFile()
	{
		const QString file = QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet");
		QStringList symbols, categories, templates;
		fileCollection(file, symbols, categories, templates);
		QVERIFY(symbols.size() > 20);

		const Rows r = run(QStringLiteral("dump('open');"), file).value(QStringLiteral("open"));
		QStringList got_symbols = r.value(QStringLiteral("symbol_def")).keys();
		QStringList got_categories = r.value(QStringLiteral("symbol_category")).keys();
		QStringList got_templates = r.value(QStringLiteral("titleblock_template")).keys();
		for (QStringList *l : {&symbols, &categories, &templates, &got_symbols, &got_categories, &got_templates})
			l->sort();
		QCOMPARE(got_symbols, symbols);
		QCOMPARE(got_categories, categories);
		QCOMPARE(got_templates, templates);
	}
};

QTEST_GUILESS_MAIN(tst_projectcollectionrows)
#include "tst_projectcollectionrows.moc"
