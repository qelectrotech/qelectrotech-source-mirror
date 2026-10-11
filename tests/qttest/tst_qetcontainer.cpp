// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>

#include <functional>

#include "container/qetcontainer.h"

#include <sqlite3.h>

// A project document split into the parts of a .qetz, zipped, unzipped and
// joined back is the document it was: every example, and every .qet in the
// folder QET_CONTAINER_CORPUS names, if set -- exactly, whitespace text
// included: QETProject writes the symbol definitions back as it read them,
// spacing and all, so a part that came back re-indented would change the
// next save.
class tst_qetcontainer : public QObject
{
	Q_OBJECT

	static QList<QDomNode> children(const QDomNode &node)
	{
		QList<QDomNode> all;
		for (QDomNode c = node.firstChild() ; !c.isNull() ; c = c.nextSibling())
			all << c;
		return all;
	}

	// empty when equal, else where they differ
	static QString difference(const QDomNode &a, const QDomNode &b, const QString &path)
	{
		const QString here = path + QLatin1Char('/') + a.nodeName();
		if (a.nodeType() != b.nodeType() || a.nodeName() != b.nodeName())
			return here + QStringLiteral(": %1 vs %2").arg(a.nodeName(), b.nodeName());
		if (!a.isElement() && !a.isDocument() && a.nodeValue() != b.nodeValue())
			return here + QStringLiteral(": value differs");
		if (a.isElement()) {
			const QDomNamedNodeMap aa = a.attributes(), ba = b.attributes();
			if (aa.count() != ba.count())
				return here + QStringLiteral(": %1 vs %2 attributes").arg(aa.count()).arg(ba.count());
			for (int i = 0 ; i < aa.count() ; ++i) {
				const QDomAttr attr = aa.item(i).toAttr();
				if (!ba.contains(attr.name()) || ba.namedItem(attr.name()).nodeValue() != attr.value())
					return here + QStringLiteral(": attribute %1 differs").arg(attr.name());
			}
		}
		const QList<QDomNode> ac = children(a), bc = children(b);
		if (ac.size() != bc.size())
			return here + QStringLiteral(": %1 vs %2 children").arg(ac.size()).arg(bc.size());
		for (int i = 0 ; i < ac.size() ; ++i) {
			const QString d = difference(ac.at(i), bc.at(i), here + QStringLiteral("[%1]").arg(i));
			if (!d.isEmpty()) return d;
		}
		return {};
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	static QList<QetZip::Entry> splitFile(const QString &path, QDomDocument *original)
	{
		QString error;
		if (!QetContainer::parse(read(path), original, &error)) return {};
		return QetContainer::split(*original, QStringLiteral("tst_qetcontainer"), &error);
	}

private slots:
	void sameDocumentBack_data()
	{
		QTest::addColumn<QString>("file");
		QStringList dirs{QStringLiteral(QET_EXAMPLES_DIR)};
		const QString corpus = qEnvironmentVariable("QET_CONTAINER_CORPUS");
		if (!corpus.isEmpty()) dirs << corpus;
		for (const QString &dir : dirs) {
			QDirIterator it(dir, {QStringLiteral("*.qet")}, QDir::Files, QDirIterator::Subdirectories);
			QStringList files;
			while (it.hasNext()) files << it.next();
			files.sort();
			for (const QString &f : files)
				QTest::newRow(qPrintable(QDir(dir).relativeFilePath(f))) << f;
		}
	}
	void sameDocumentBack()
	{
		QFETCH(QString, file);
		QDomDocument original;
		QString error;
		if (!QetContainer::parse(read(file), &original, &error))
			QSKIP(qPrintable(QStringLiteral("not readable as XML, as QElectroTech finds too: ") + error));
		const QList<QetZip::Entry> entries = QetContainer::split(original, QString(), &error);
		QVERIFY2(!entries.isEmpty(), qPrintable(error));
		QCOMPARE(entries.first().name, QStringLiteral("mimetype"));

		QList<QetZip::Entry> unzipped;
		QVERIFY2(QetZip::fromBytes(QetZip::toBytes(entries, &error), &unzipped, &error), qPrintable(error));
		QDomDocument back;
		QVERIFY2(QetContainer::join(unzipped, &back, &error), qPrintable(error));
		const QString d = difference(original.documentElement(), back.documentElement(), QString());
		QVERIFY2(d.isEmpty(), qPrintable(d));
	}

	// Splitting and joining adds no whitespace text. QETProject keeps the
	// symbol definitions as it read them and writes them back as they
	// are, so indentation a part gained would change the next save
	// (forum attachments 1328, 1978, 2057, 3003, 3004 did, with indented
	// parts). Here: a document without any whitespace-only text, as
	// QETProject::toXml() builds one, must come back without any.
	void noWhitespaceAdded()
	{
		QDomDocument original;
		QVERIFY(original.setContent(read(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"))));
		auto whitespace_nodes = [](const QDomDocument &d) {
			int n = 0;
			std::function<void(const QDomNode &)> walk = [&](const QDomNode &node) {
				for (QDomNode c = node.firstChild() ; !c.isNull() ; c = c.nextSibling()) {
					if (c.isText() && c.nodeValue().trimmed().isEmpty()) ++n;
					walk(c);
				}
			};
			walk(d);
			return n;
		};
		QCOMPARE(whitespace_nodes(original), 0);
		QString error;
		const QList<QetZip::Entry> entries = QetContainer::split(original, QString(), &error);
		QVERIFY2(!entries.isEmpty(), qPrintable(error));
		QDomDocument back;
		QVERIFY2(QetContainer::join(entries, &back, &error), qPrintable(error));
		QCOMPARE(whitespace_nodes(back), 0);
	}

	// The engineering data is in project.sqlite and nowhere else: the folio
	// files hold no symbol information, links, wire numbers or title-block
	// fields, and the database holds them all (industrial.qet: 50 folios).
	void databaseHoldsTheData()
	{
		QDomDocument original;
		const QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		QVERIFY(!entries.isEmpty());
		QByteArray database;
		for (const QetZip::Entry &e : entries) {
			if (e.name == QLatin1String("project.sqlite")) database = e.data;
			if (e.name.startsWith(QStringLiteral("folios/")) || e.name == QLatin1String("project.xml")) {
				QVERIFY2(!e.data.contains("<elementInformation "), qPrintable(e.name));
				QVERIFY2(!e.data.contains("<link_uuid "), qPrintable(e.name));
				QVERIFY2(!e.data.contains("<property "), qPrintable(e.name));
				QVERIFY2(!QString::fromUtf8(e.data).contains(QRegularExpression(QStringLiteral("<conductor [^>]* num="))),
						 qPrintable(e.name));
			}
		}
		QVERIFY(database.startsWith("SQLite format 3"));

		sqlite3 *db = nullptr;
		QCOMPARE(sqlite3_open(":memory:", &db), SQLITE_OK);
		unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(database.size())));
		memcpy(copy, database.constData(), size_t(database.size()));
		QCOMPARE(sqlite3_deserialize(db, "main", copy, database.size(), database.size(),
									 SQLITE_DESERIALIZE_FREEONCLOSE), SQLITE_OK);
		auto count = [db](const char *table) {
			sqlite3_stmt *st = nullptr;
			sqlite3_prepare_v2(db, (QByteArray("SELECT count(*) FROM ") + table).constData(), -1, &st, nullptr);
			sqlite3_step(st);
			const int n = sqlite3_column_int(st, 0);
			sqlite3_finalize(st);
			return n;
		};
		const QDomNodeList diagrams = original.elementsByTagName(QStringLiteral("diagram"));
		QCOMPARE(count("folio"), int(diagrams.size()));
		QCOMPARE(count("element"), int(original.elementsByTagName(QStringLiteral("element")).size())
				 - int(original.elementsByTagName(QStringLiteral("collection")).at(0).toElement()
					   .elementsByTagName(QStringLiteral("element")).size()));
		QCOMPARE(count("element_info"), int(original.elementsByTagName(QStringLiteral("elementInformation")).size()));
		QCOMPARE(count("link"), int(original.elementsByTagName(QStringLiteral("link_uuid")).size()));
		QCOMPARE(count("conductor"), int(original.elementsByTagName(QStringLiteral("conductor")).size()));
		QVERIFY(count("folio_property") + count("project_property") > 0);
		sqlite3_close(db);
	}

	// Terminal strips move into the database, and come back exactly:
	// industrial.qet with two strips on its own terminal symbols, parsed as
	// a save leaves it (no whitespace text).
	void terminalStripsInDatabase()
	{
		QByteArray xml = read(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		const QByteArray strips =
			"<terminal_strips>"
			"<terminal_strip><terminal_strip_data uuid=\"{11111111-0000-4000-8000-000000000001}\"><informations>"
			"<information name=\"installation\">=A1</information><information name=\"location\">+CAB1</information>"
			"<information name=\"name\">X1</information><information name=\"comment\">main &amp; aux</information>"
			"</informations></terminal_strip_data>"
			"<layout><physical_terminal><real_terminal element_uuid=\"{c2180165-8d35-44eb-83c4-100395451915}\"/></physical_terminal>"
			"<physical_terminal><real_terminal element_uuid=\"{05f681ee-04dd-489d-afc8-a646ec6016f9}\"/>"
			"<real_terminal element_uuid=\"{a33935cf-671a-480f-9497-1a632eab7cbd}\"/></physical_terminal></layout>"
			"<terminal_strip_bridge uuid=\"{22222222-0000-4000-8000-000000000001}\" color=\"#ff0000\"><real_terminals>"
			"<real_terminal uuid=\"{c2180165-8d35-44eb-83c4-100395451915}\"/>"
			"<real_terminal uuid=\"{05f681ee-04dd-489d-afc8-a646ec6016f9}\"/></real_terminals></terminal_strip_bridge>"
			"</terminal_strip>"
			"<terminal_strip><terminal_strip_data uuid=\"{11111111-0000-4000-8000-000000000002}\"><informations>"
			"<information name=\"name\">X2</information></informations></terminal_strip_data>"
			"<layout><physical_terminal><real_terminal element_uuid=\"{c96008ed-1c74-4c0a-836a-bd8458491a16}\"/></physical_terminal></layout>"
			"</terminal_strip></terminal_strips>";
		const int at = xml.indexOf("<collection");
		QVERIFY(at > 0);
		xml.insert(at, strips);
		QDomDocument original;
		QVERIFY(original.setContent(xml));   // as a save leaves it: no whitespace text
		QString error;
		const QList<QetZip::Entry> entries = QetContainer::split(original, QString(), &error);
		QVERIFY2(!entries.isEmpty(), qPrintable(error));
		for (const QetZip::Entry &e : entries)
			if (e.name == QLatin1String("project.xml"))
				QVERIFY2(!e.data.contains("<terminal_strip>"), "the strips stayed in project.xml");
		QDomDocument back;
		QVERIFY2(QetContainer::join(entries, &back, &error), qPrintable(error));
		const QString d = difference(original.documentElement(), back.documentElement(), QString());
		QVERIFY2(d.isEmpty(), qPrintable(d));
	}

	// The folio pictures leave the folios, once each
	void picturesAreFiles()
	{
		QDomDocument original;
		const QList<QetZip::Entry> entries = splitFile(
					QStringLiteral(QET_EXAMPLES_DIR "/weneedpolonez-Polonez_MR89_wiring_diagram.qet"), &original);
		QVERIFY(!entries.isEmpty());
		int pictures = 0;
		for (const QetZip::Entry &e : entries) {
			if (e.name.startsWith(QStringLiteral("images/"))) {
				++pictures;
				QVERIFY(e.data.startsWith("\x89PNG"));
				QVERIFY(!e.compress);
			}
			if (e.name.startsWith(QStringLiteral("folios/")))
				QVERIFY2(!e.data.contains("iVBORw0KGgo"), "a folio still holds a PNG in base64");
		}
		QVERIFY(pictures > 0);
		QVERIFY(pictures < 121);   // the 121 placed pictures repeat a few files
	}

	// A file whose manifest needs a later reader is refused, not half read
	void laterFormatRefused()
	{
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"), &original);
		QVERIFY(!entries.isEmpty());
		for (QetZip::Entry &e : entries)
			if (e.name == QLatin1String("manifest.xml"))
				e.data = QString::fromUtf8(e.data).replace(QRegularExpression(QStringLiteral("min-reader=\"\\d+\"")),
														   QStringLiteral("min-reader=\"9\"")).toUtf8();
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join(entries, &back, &error));
		QVERIFY2(error.contains(QStringLiteral("later")), qPrintable(error));
	}

	void missingPartRefused()
	{
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"), &original);
		for (int i = 0 ; i < entries.size() ; ++i)
			if (entries.at(i).name.startsWith(QStringLiteral("folios/"))) { entries.removeAt(i); break; }
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join(entries, &back, &error));
		QVERIFY2(error.contains(QStringLiteral("missing")), qPrintable(error));
	}

	// A crafted project.sqlite whose "element" is a view, not a table, is
	// refused: nothing in a file's schema is trusted.
	void viewInsteadOfTableRefused()
	{
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"), &original);
		QVERIFY(!entries.isEmpty());
		for (QetZip::Entry &e : entries) {
			if (e.name != QLatin1String("project.sqlite")) continue;
			sqlite3 *db = nullptr;
			sqlite3_open(":memory:", &db);
			unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(e.data.size())));
			memcpy(copy, e.data.constData(), size_t(e.data.size()));
			sqlite3_deserialize(db, "main", copy, e.data.size(), e.data.size(),
								SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_RESIZEABLE);
			QCOMPARE(sqlite3_exec(db, "ALTER TABLE element RENAME TO element_data;"
									  "CREATE VIEW element AS SELECT * FROM element_data;",
								  nullptr, nullptr, nullptr), SQLITE_OK);
			sqlite3_int64 size = 0;
			unsigned char *bytes = sqlite3_serialize(db, "main", &size, 0);
			e.data = QByteArray(reinterpret_cast<const char *>(bytes), qsizetype(size));
			sqlite3_free(bytes);
			sqlite3_close(db);
		}
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join(entries, &back, &error));
		QVERIFY2(error.contains(QStringLiteral("element")), qPrintable(error));
	}

	void notAProjectRefused()
	{
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join({{QStringLiteral("mimetype"), "application/zip", false}}, &back, &error));
	}
};

QTEST_APPLESS_MAIN(tst_qetcontainer)

#include "tst_qetcontainer.moc"
