// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QtEndian>

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

		//@p document without what QElectroTech computes, which a .qetz
		//does not keep (format 4); tst_resaveunchanged checks that
		//QElectroTech gives it back
	static QDomDocument withoutComputed(const QDomDocument &document)
	{
		QDomDocument copy = document.cloneNode(true).toDocument();
		const bool own_uuids = QetContainer::ownUuids(copy.documentElement());
		for (QDomElement diagram = copy.documentElement().firstChildElement(QStringLiteral("diagram"));
			 !diagram.isNull() ; diagram = diagram.nextSiblingElement(QStringLiteral("diagram")))
			QetContainer::dropComputed(diagram, own_uuids);
		return copy;
	}

	static QByteArray entry(const QList<QetZip::Entry> &entries, const QString &name)
	{
		for (const QetZip::Entry &e : entries)
			if (e.name == name) return e.data;
		return {};
	}

	static QByteArray folioFiles(const QList<QetZip::Entry> &entries)
	{
		QByteArray all;
		for (const QetZip::Entry &e : entries)
			if (e.name.startsWith(QStringLiteral("folios/"))) all += e.data;
		return all;
	}

		// The first column of the first row @p sql gives, from @p database
	static QString query(const QByteArray &database, const char *sql)
	{
		sqlite3 *db = nullptr;
		sqlite3_open(":memory:", &db);
		unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(database.size())));
		memcpy(copy, database.constData(), size_t(database.size()));
		sqlite3_deserialize(db, "main", copy, database.size(), database.size(), SQLITE_DESERIALIZE_FREEONCLOSE);
		sqlite3_stmt *st = nullptr;
		QString out;
		if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW)
			out = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(st, 0)));
		sqlite3_finalize(st);
		sqlite3_close(db);
		return out;
	}

		// grafcet.qet as QElectroTech saves it: every folio, symbol and wire
		// with a uuid of its own, which the examples' wires and folios lack
	static QDomDocument savedProject()
	{
		QList<QetZip::Entry> entries;
		QDomDocument project;
		QetZip::fromBytes(read(QStringLiteral(QET_FIXTURES_DIR "/qetz_format2.qetz")), &entries);
		QetContainer::join(entries, &project);
		return project;
	}

	static QString backDifference(const QDomDocument &original, const QList<QetZip::Entry> &entries)
	{
		QDomDocument back;
		QString error;
		if (!QetContainer::join(entries, &back, &error)) return error;
		return difference(withoutComputed(original).documentElement(), back.documentElement(), QString());
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
		const QString d = difference(withoutComputed(original).documentElement(),
									 back.documentElement(), QString());
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
	// fields, nor what QElectroTech computes from them (a wire's
	// description of its ends, a symbol text showing its information), and
	// the database holds them all (industrial.qet: 50 folios).
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

	// Nor does a folio file hold what QElectroTech computes (format 3): a
	// wire's description of its ends, a symbol text showing its information.
	// A file holding both, so that the check can fail.
	void computedNotKept()
	{
		QDomDocument original;
		QString error;
		QVERIFY(QetContainer::parse(read(QStringLiteral(QET_EXAMPLES_DIR "/schema_unifilaire_voltaique2.qet")),
									&original, &error));
		const QDomNodeList texts = original.elementsByTagName(QStringLiteral("dynamic_elmt_text"));
		bool filled = false;
		for (int i = 0 ; i < texts.size() && !filled ; ++i) {
			QDomElement text = texts.at(i).toElement();
			if (text.attribute(QStringLiteral("text_from")) != QLatin1String("ElementInfo")) continue;
			QDomElement content = text.firstChildElement(QStringLiteral("text"));
			while (content.hasChildNodes()) content.removeChild(content.firstChild());
			content.appendChild(original.createTextNode(QStringLiteral("COMPUTED-LABEL")));
			filled = true;
		}
		QVERIFY(filled);
		QVERIFY(original.toString().contains(QStringLiteral("element1_label=")));
		const QList<QetZip::Entry> entries = QetContainer::split(original, QString(), &error);
		QVERIFY2(!entries.isEmpty(), qPrintable(error));
		for (const QetZip::Entry &e : entries) {
			if (!e.name.startsWith(QStringLiteral("folios/"))) continue;
			QVERIFY2(!e.data.contains("element1_label=") && !e.data.contains("terminalname2="),
					 qPrintable(e.name));
			QVERIFY2(!e.data.contains("COMPUTED-LABEL"), qPrintable(e.name));
		}
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
		const QString d = difference(withoutComputed(original).documentElement(),
									 back.documentElement(), QString());
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

	// Given a map, join() hands the pictures over as bytes instead of
	// putting them back in the document as base64
	void picturesKeptAside()
	{
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(
					QStringLiteral(QET_EXAMPLES_DIR "/weneedpolonez-Polonez_MR89_wiring_diagram.qet"), &original);
		QVERIFY(!entries.isEmpty());
		QHash<QString, QByteArray> files;
		for (const QetZip::Entry &e : entries) files.insert(e.name, e.data);

		QDomDocument back;
		QetContainer::Pictures pictures;
		QString error;
		QVERIFY2(QetContainer::join(entries, &back, &error, nullptr, &pictures), qPrintable(error));
		const QDomNodeList images = back.elementsByTagName(QStringLiteral("image"));
		const QDomNodeList original_images = original.elementsByTagName(QStringLiteral("image"));
		QCOMPARE(images.size(), 121);
		QCOMPARE(images.size(), original_images.size());
		for (int i = 0 ; i < images.size() ; ++i) {
			const QDomElement image = images.at(i).toElement();
			const QString file = QetContainer::pictureFile(image);
			QVERIFY(!file.isEmpty());
			QVERIFY(!image.firstChild().isText());
			QCOMPARE(pictures.value(file), files.value(file));
			QCOMPARE(pictures.value(file),
					 QByteArray::fromBase64(original_images.at(i).firstChild().toText().data().toLatin1()));
		}

		// still refused when a picture is missing
		for (int i = 0 ; i < entries.size() ; ++i)
			if (entries.at(i).name.startsWith(QStringLiteral("images/"))) { entries.removeAt(i); break; }
		QDomDocument broken;
		QVERIFY(!QetContainer::join(entries, &broken, &error, nullptr, &pictures));
		QVERIFY2(error.contains(QStringLiteral("missing")), qPrintable(error));
	}

	// A file whose manifest needs a later reader is refused, not half read
	// A later format whose min-reader allows this one opens, and join() says
	// it is later: QElectroTech opens it read-only
	void laterFormatAllowedRead()
	{
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		QVERIFY(!entries.isEmpty());
		const int later = QetContainer::Format + 1;
		for (QetZip::Entry &e : entries)
			if (e.name == QLatin1String("manifest.xml"))
				e.data = QString::fromUtf8(e.data).replace(QRegularExpression(QStringLiteral(" format=\"\\d+\"")),
														   QStringLiteral(" format=\"%1\"").arg(later)).toUtf8();
		alterDatabase(entries, QStringLiteral("UPDATE meta SET value = '%1' WHERE key = 'format'")
					  .arg(later).toUtf8().constData());
		QDomDocument back;
		QString error;
		int format = 0;
		QVERIFY2(QetContainer::join(entries, &back, &error, nullptr, nullptr, &format), qPrintable(error));
		QCOMPARE(format, later);
		const QString d = difference(withoutComputed(original).documentElement(), back.documentElement(), QString());
		QVERIFY2(d.isEmpty(), qPrintable(d));
	}

	// The database holds data, not where a block sat in the XML: the folio
	// file's marker says where it goes back
	void noPositionColumns()
	{
		QDomDocument original;
		const QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		const QByteArray database = entry(entries, QStringLiteral("project.sqlite"));
		QCOMPARE(query(database, "SELECT group_concat(name) FROM pragma_table_info('element')"),
				 QStringLiteral("uuid,folio_uuid,prefix,freezeLabel,info_attrs,links_attrs"));
		QCOMPARE(query(database, "SELECT count(*) FROM pragma_table_info('folio') WHERE name LIKE '%_at'"),
				 QStringLiteral("0"));
		QCOMPARE(query(database, "SELECT count(*) FROM meta WHERE key = 'props_at'"), QStringLiteral("0"));
	}

	// A project as QElectroTech saves it needs no JSON: every attribute it
	// writes has a column, the show flag of a property too
	void savedProjectNeedsNoJson()
	{
		QString error;
		const QList<QetZip::Entry> entries = QetContainer::split(savedProject(), QString(), &error);
		QVERIFY2(!entries.isEmpty(), qPrintable(error));
		const QByteArray database = entry(entries, QStringLiteral("project.sqlite"));
		QVERIFY(query(database, "SELECT count(*) FROM project_property").toInt() > 0);
		QCOMPARE(query(database, "SELECT count(*) FROM project_property WHERE show IS NOT NULL"),
				 query(database, "SELECT count(*) FROM project_property"));
		for (const char *sql : {"SELECT count(*) FROM project_property WHERE attrs IS NOT NULL",
								"SELECT count(*) FROM folio_property WHERE attrs IS NOT NULL",
								"SELECT count(*) FROM element_info WHERE attrs IS NOT NULL",
								"SELECT count(*) FROM link WHERE attrs IS NOT NULL",
								"SELECT count(*) FROM folio WHERE props_attrs IS NOT NULL",
								"SELECT count(*) FROM element WHERE info_attrs IS NOT NULL OR links_attrs IS NOT NULL"})
			QVERIFY2(query(database, sql) == QLatin1String("0"), sql);
	}

	// The schema in QETZ-FORMAT.md is the one written: same tables, same
	// columns in the same order, same types; written without keys (format 4)
	void specMatchesSchema()
	{
		// A Windows checkout may give the file CRLF line endings
		const QString spec = QString::fromUtf8(read(QStringLiteral(QET_FORMAT_SPEC)))
								 .remove(QLatin1Char('\r'));
		const int from = spec.indexOf(QStringLiteral("```sql\n"));
		const int to = spec.indexOf(QStringLiteral("```"), from + 7);
		QVERIFY2(from >= 0 && to > from, "no schema in QETZ-FORMAT.md");
		const QByteArray sql = spec.mid(from + 7, to - from - 7).toUtf8();

		QDomDocument original;
		const QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		const QByteArray written = entry(entries, QStringLiteral("project.sqlite"));

		auto describe = [](sqlite3 *db, const char *columns = "m.name, p.name, p.type") {
			QStringList out;
			sqlite3_stmt *st = nullptr;
			sqlite3_prepare_v2(db, QByteArray("SELECT ") + columns + " FROM sqlite_master m, "
									"pragma_table_info(m.name) p WHERE m.type = 'table' ORDER BY m.name, p.cid",
							   -1, &st, nullptr);
			while (st && sqlite3_step(st) == SQLITE_ROW) {
				QStringList row;
				for (int i = 0 ; i < sqlite3_column_count(st) ; ++i)
					row << QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(st, i)));
				out << row.join(QLatin1Char(' '));
			}
			sqlite3_finalize(st);
			return out;
		};
		sqlite3 *from_spec = nullptr;
		sqlite3_open(":memory:", &from_spec);
		QCOMPARE(sqlite3_exec(from_spec, sql.constData(), nullptr, nullptr, nullptr), SQLITE_OK);
		sqlite3 *from_file = nullptr;
		sqlite3_open(":memory:", &from_file);
		unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(written.size())));
		memcpy(copy, written.constData(), size_t(written.size()));
		sqlite3_deserialize(from_file, "main", copy, written.size(), written.size(), SQLITE_DESERIALIZE_FREEONCLOSE);
		const QStringList spec_tables = describe(from_spec), file_tables = describe(from_file);
		const QStringList keys = describe(from_file, "p.pk + p.\"notnull\"");
		QVERIFY(!keys.isEmpty());
		QCOMPARE(keys.count(QStringLiteral("0")), keys.size());
		sqlite3_close(from_spec);
		sqlite3_close(from_file);
		QVERIFY(!spec_tables.isEmpty());
		QCOMPARE(spec_tables, file_tables);
	}

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
	void garbageDatabaseRefused()
	{
			//Bytes that are no database: refused, not read (crashed reading
			//the first statement until stage D7)
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet"), &original);
		bool replaced = false;
		for (QetZip::Entry &entry : entries)
			if (entry.name == QLatin1String("project.sqlite")) {
				entry.data = QByteArray("not a database").repeated(100);
				replaced = true;
			}
		QVERIFY(replaced);
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join(entries, &back, &error));
		QVERIFY(!error.isEmpty());
	}

	// A page of the database overwritten, as a bad disk or a broken copy
	// would: refused, and said to be damaged (quick_check on opening),
	// whichever table the page held
	void damagedDatabaseRefused_data()
	{
		QTest::addColumn<double>("where");
		QTest::newRow("second page") << 0.0;
		QTest::newRow("middle page") << 0.5;
		QTest::newRow("last page") << 1.0;
	}
	void damagedDatabaseRefused()
	{
		QFETCH(double, where);
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		bool damaged = false;
		for (QetZip::Entry &entry : entries) {
			if (entry.name != QLatin1String("project.sqlite")) continue;
			const int page_size = qFromBigEndian<quint16>(entry.data.constData() + 16);
			const int pages = int(entry.data.size() / page_size);
			QVERIFY(pages > 3);
			const int page = 1 + int(where * (pages - 2));
			entry.data.replace(page * page_size, page_size, QByteArray(page_size, char(0xff)));
			damaged = true;
		}
		QVERIFY(damaged);
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join(entries, &back, &error));
		QVERIFY2(error.contains(QStringLiteral("damaged")), qPrintable(error));
	}

	// project.sqlite changed by @p sql in @p entries
	static void alterDatabase(QList<QetZip::Entry> &entries, const char *sql)
	{
		for (QetZip::Entry &e : entries) {
			if (e.name != QLatin1String("project.sqlite")) continue;
			sqlite3 *db = nullptr;
			sqlite3_open(":memory:", &db);
			unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(e.data.size())));
			memcpy(copy, e.data.constData(), size_t(e.data.size()));
			sqlite3_deserialize(db, "main", copy, e.data.size(), e.data.size(),
								SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_RESIZEABLE);
			QCOMPARE(sqlite3_exec(db, sql, nullptr, nullptr, nullptr), SQLITE_OK);
			sqlite3_int64 size = 0;
			unsigned char *bytes = sqlite3_serialize(db, "main", &size, 0);
			e.data = QByteArray(reinterpret_cast<const char *>(bytes), qsizetype(size));
			sqlite3_free(bytes);
			sqlite3_close(db);
		}
	}

	// A database that does not match its folio files -- a row no item has,
	// an item's row on another folio, entries of no symbol -- is refused
	// with what is wrong, not read as far as it goes
	void mismatchRefused_data()
	{
		QTest::addColumn<QByteArray>("sql");
		QTest::addColumn<QString>("said");
		QTest::newRow("symbol row of no symbol")
				<< QByteArray("INSERT INTO element SELECT 'x', folio_uuid, prefix, freezeLabel, info_attrs,"
							  " links_attrs FROM element LIMIT 1")
				<< QStringLiteral("symbol rows");
		QTest::newRow("symbol row twice")
				<< QByteArray("INSERT INTO element SELECT * FROM element LIMIT 1")
				<< QStringLiteral("symbol rows");
		QTest::newRow("wire row of no wire")
				<< QByteArray("INSERT INTO conductor SELECT 'x', folio_uuid, element1, terminal1, element2, terminal2,"
							  " num, formula, function, tension_protocol, conductor_color, conductor_section, cable,"
							  " bus, freezeLabel FROM conductor LIMIT 1")
				<< QStringLiteral("wire rows");
		QTest::newRow("symbol on another folio")
				<< QByteArray("UPDATE element SET folio_uuid = (SELECT uuid FROM folio WHERE pos = 2)"
							  " WHERE folio_uuid = (SELECT uuid FROM folio WHERE pos = 1)")
				<< QStringLiteral("the database says another");
		QTest::newRow("information of no symbol")
				<< QByteArray("UPDATE element_info SET element_uuid = 'x' WHERE rowid = 1")
				<< QStringLiteral("information rows of no symbol");
	}
	void mismatchRefused()
	{
		QFETCH(QByteArray, sql);
		QFETCH(QString, said);
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		QVERIFY(!entries.isEmpty());
		{
			QDomDocument back;
			QString error;
			QVERIFY2(QetContainer::join(entries, &back, &error), qPrintable(error));
		}
		alterDatabase(entries, sql.constData());
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join(entries, &back, &error));
		QVERIFY2(error.contains(said), qPrintable(error));
	}

	// A folio file says which symbol each item is (its type), read alone
	void folioSaysWhichSymbol()
	{
		QDomDocument original;
		const QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		QVERIFY(!entries.isEmpty());
		int typed = 0, symbols = 0;
		for (const QetZip::Entry &e : entries) {
			if (!e.name.startsWith(QStringLiteral("folios/"))) continue;
			const QString text = QString::fromUtf8(e.data);
			symbols += int(text.count(QStringLiteral("<element ")));
			typed += int(text.count(QRegularExpression(QStringLiteral("<element [^>]*type=\"embed://"))));
		}
		QVERIFY(symbols > 0);
		QCOMPARE(typed, symbols);
	}

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

	// A row's key is its item's uuid, which the folio file keeps: no key
	// made up from where an item is (format 2 had f1, f2...), no key
	// attribute in the folio files of a project as QElectroTech saves it,
	// true/false held as 1/0, and the format in the database's
	// user_version.
	void keysAreTheUuids()
	{
		const QDomDocument original = savedProject();
		original.elementsByTagName(QStringLiteral("conductor")).at(0).toElement()
				.setAttribute(QStringLiteral("freezeLabel"), QStringLiteral("true"));
		QString error;
		const QList<QetZip::Entry> entries = QetContainer::split(original, QString(), &error);
		QVERIFY2(!entries.isEmpty(), qPrintable(error));
		const QByteArray database = entry(entries, QStringLiteral("project.sqlite"));
		const QByteArray folios = folioFiles(entries);
		QVERIFY(!folios.contains("qetz-key"));
		QVERIFY(!folios.contains("qetz-folio"));
		QCOMPARE(query(database, "PRAGMA user_version"), QString::number(QetContainer::Format));
		const QString first = original.elementsByTagName(QStringLiteral("diagram")).at(0).toElement()
				.attribute(QStringLiteral("uuid"));
		QCOMPARE(query(database, "SELECT uuid FROM folio WHERE pos = 1"), first);
		QCOMPARE(query(database, "SELECT count(*) FROM folio WHERE uuid NOT LIKE '{%}'"), QStringLiteral("0"));
		QCOMPARE(query(database, "SELECT count(*) FROM element WHERE uuid NOT LIKE '{%}'"), QStringLiteral("0"));
		QCOMPARE(query(database, "SELECT count(*) FROM conductor WHERE uuid NOT LIKE '{%}'"), QStringLiteral("0"));
		QCOMPARE(query(database, "SELECT count(*) FROM element WHERE typeof(freezeLabel) <> 'integer'"),
				 QStringLiteral("0"));
		QCOMPARE(query(database, "SELECT count(*) FROM conductor WHERE typeof(freezeLabel) <> 'integer'"),
				 QStringLiteral("0"));
		QCOMPARE(query(database, "SELECT sum(freezeLabel) FROM conductor"), QStringLiteral("1"));
		const QString d = backDifference(original, entries);
		QVERIFY2(d.isEmpty(), qPrintable(d));
		QCOMPARE(query(database, "SELECT count(*) FROM element_info WHERE typeof(show) <> 'integer'"),
				 QStringLiteral("0"));
		QCOMPARE(query(database, "SELECT count(*) FROM element_info WHERE attrs LIKE '%\"tag\"%'"),
				 QStringLiteral("0"));
	}

	// Two symbols sharing a uuid (F100) each get a key made from what they
	// hold, the same on every save, and come back as they were.
	void sharedUuidKeyed()
	{
		QDomDocument original = savedProject();
		const QDomNodeList elements = original.elementsByTagName(QStringLiteral("element"));
		QDomElement a, b;
		for (int i = 0 ; i < elements.size() && b.isNull() ; ++i) {
			const QDomElement e = elements.at(i).toElement();
			if (e.parentNode().nodeName() != QLatin1String("elements")) continue;
			if (a.isNull()) a = e; else b = e;
		}
		QVERIFY(!b.isNull());
		b.setAttribute(QStringLiteral("uuid"), a.attribute(QStringLiteral("uuid")));
		QString error;
		const QList<QetZip::Entry> once = QetContainer::split(original, QString(), &error);
		const QList<QetZip::Entry> twice = QetContainer::split(original, QString(), &error);
		QVERIFY2(!once.isEmpty(), qPrintable(error));
		QCOMPARE(folioFiles(once).count("qetz-key"), 2);
		QCOMPARE(entry(once, QStringLiteral("project.sqlite")), entry(twice, QStringLiteral("project.sqlite")));
		const QString d = backDifference(original, once);
		QVERIFY2(d.isEmpty(), qPrintable(d));
	}

	// A true/false attribute QElectroTech would not write stays in the
	// folio file as it was.
	void oddBooleanKept()
	{
		QDomDocument original;
		QVERIFY(QetContainer::parse(read(QStringLiteral(QET_EXAMPLES_DIR "/perceuse.qet")), &original));
		const QDomNodeList conductors = original.elementsByTagName(QStringLiteral("conductor"));
		QVERIFY(conductors.size() > 0);
		conductors.at(0).toElement().setAttribute(QStringLiteral("freezeLabel"), QStringLiteral("yes"));
		QString error;
		const QList<QetZip::Entry> entries = QetContainer::split(original, QString(), &error);
		QVERIFY2(!entries.isEmpty(), qPrintable(error));
		QVERIFY(folioFiles(entries).contains("freezeLabel=\"yes\""));
		const QString d = backDifference(original, entries);
		QVERIFY2(d.isEmpty(), qPrintable(d));
	}

	// A format 2 file, as QElectroTech 0.200.1 development builds wrote it
	// (grafcet.qet saved as .qetz), still opens, and saved again as the
	// current format comes back the same.
	void format2Read()
	{
		QList<QetZip::Entry> entries;
		QString error;
		QVERIFY2(QetZip::fromBytes(read(QStringLiteral(QET_FIXTURES_DIR "/qetz_format2.qetz")), &entries, &error),
				 qPrintable(error));
		QVERIFY(entry(entries, QStringLiteral("manifest.xml")).contains("format=\"2\""));
		QDomDocument project;
		QVERIFY2(QetContainer::join(entries, &project, &error), qPrintable(error));
		QVERIFY(project.toString().contains(QStringLiteral("<elementInformation ")));
		QVERIFY(!project.toString().contains(QStringLiteral("qetz-")));
			//Format 2 moved the uuids of symbols and wires and the symbols'
			//type out of the folio files: they come back
		int symbols = 0, wires = 0;
		const QDomNodeList diagrams = project.elementsByTagName(QStringLiteral("diagram"));
		for (int i = 0 ; i < diagrams.size() ; ++i) {
			const QDomElement d = diagrams.at(i).toElement();
			for (QDomElement e = d.firstChildElement(QStringLiteral("elements")).firstChildElement(QStringLiteral("element"));
				 !e.isNull() ; e = e.nextSiblingElement(QStringLiteral("element")), ++symbols) {
				QVERIFY(!QUuid(e.attribute(QStringLiteral("uuid"))).isNull());
				QVERIFY(e.attribute(QStringLiteral("type")).startsWith(QStringLiteral("embed://")));
			}
			for (QDomElement c = d.firstChildElement(QStringLiteral("conductors")).firstChildElement(QStringLiteral("conductor"));
				 !c.isNull() ; c = c.nextSiblingElement(QStringLiteral("conductor")), ++wires)
				QVERIFY(!QUuid(c.attribute(QStringLiteral("uuid"))).isNull());
		}
		QVERIFY(symbols > 0 && wires > 0);
		const QList<QetZip::Entry> resaved = QetContainer::split(project, QString(), &error);
		QVERIFY2(!resaved.isEmpty(), qPrintable(error));
		const QString d = backDifference(project, resaved);
		QVERIFY2(d.isEmpty(), qPrintable(d));
	}

	void notAProjectRefused()
	{
		QDomDocument back;
		QString error;
		QVERIFY(!QetContainer::join({{QStringLiteral("mimetype"), "application/zip", false}}, &back, &error));
	}

	// recover() reads what it can of a damaged .qetz: each kind of damage
	// costs what it held and nothing else, and is said
	void recoverLeavesOutOnlyTheDamage_data()
	{
		QTest::addColumn<QString>("damage");
		QTest::addColumn<QString>("said");
		QTest::addColumn<int>("folios");
		QTest::addColumn<int>("wires");
		QTest::newRow("nothing") << QString() << QString() << 0 << 0;
		QTest::newRow("folio file missing") << QStringLiteral("folio") << QStringLiteral("is missing") << -1 << 0;
		QTest::newRow("wire row deleted") << QStringLiteral("row") << QStringLiteral("not in the database") << 0 << -1;
		QTest::newRow("database page overwritten") << QStringLiteral("page") << QStringLiteral("is damaged") << 0 << -1000;
	}
	void recoverLeavesOutOnlyTheDamage()
	{
		QFETCH(QString, damage);
		QFETCH(QString, said);
		QFETCH(int, folios);
		QFETCH(int, wires);
		QDomDocument original;
		QList<QetZip::Entry> entries = splitFile(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), &original);
		QVERIFY(!entries.isEmpty());
		auto count = [](const QDomDocument &d, const QString &tag) {
			return int(d.documentElement().elementsByTagName(tag).size());
		};
		const int folios_before = count(original, QStringLiteral("diagram"));
		const int wires_before = count(original, QStringLiteral("conductor"));
		if (damage == QLatin1String("folio")) {
			for (int i = 0 ; i < entries.size() ; ++i)
				if (entries.at(i).name.startsWith(QStringLiteral("folios/"))) { entries.removeAt(i); break; }
		} else if (damage == QLatin1String("row")) {
			alterDatabase(entries, "DELETE FROM conductor WHERE rowid = (SELECT min(rowid) FROM conductor)");
		} else if (damage == QLatin1String("page")) {
			for (QetZip::Entry &entry : entries) {
				if (entry.name != QLatin1String("project.sqlite")) continue;
				const int page_size = qFromBigEndian<quint16>(entry.data.constData() + 16);
				const int page = int(entry.data.size() / page_size / 2);
				entry.data.replace(page * page_size, page_size, QByteArray(page_size, char(0xff)));
			}
		}
		QByteArray zip;
		QString error;
		QVERIFY2(QetZip::toBytes(entries, &error).size() > 0, qPrintable(error));
		zip = QetZip::toBytes(entries, &error);

		QDomDocument back;
		QStringList lost;
		QVERIFY2(QetContainer::recover(zip, &back, &lost, &error), qPrintable(error));
		if (said.isEmpty()) {
			QVERIFY2(lost.isEmpty(), qPrintable(lost.join(QLatin1Char('\n'))));
		} else {
			QVERIFY2(lost.join(QLatin1Char('\n')).contains(said), qPrintable(lost.join(QLatin1Char('\n'))));
				// Strict reading refuses it
			QDomDocument strict;
			QVERIFY(!QetContainer::join(entries, &strict));
		}
		QCOMPARE(count(back, QStringLiteral("diagram")), folios_before + folios);
		if (wires == -1000)
			QCOMPARE(count(back, QStringLiteral("conductor")), 0);
		else
			QCOMPARE(count(back, QStringLiteral("conductor")), wires_before + wires);
			// Nothing of the container's own left in the document
		QVERIFY(!back.toString().contains(QStringLiteral("qetz-")));
	}

	// A picture's file missing (F118 wrote such projects): the picture is
	// left out, the rest read
	void recoverMissingPicture()
	{
		QList<QetZip::Entry> entries;
		QString error;
		QVERIFY2(QetZip::fromBytes(read(QStringLiteral(QET_FIXTURES_DIR "/qetz_format2.qetz")), &entries, &error),
				 qPrintable(error));
		QDomDocument document;
		QVERIFY2(QetContainer::join(entries, &document, &error), qPrintable(error));
			// A picture on the first folio, then saved and its file dropped
		QDomElement folio = document.documentElement().firstChildElement(QStringLiteral("diagram"));
		QDomElement images = folio.firstChildElement(QStringLiteral("images"));
		if (images.isNull()) images = folio.appendChild(document.createElement(QStringLiteral("images"))).toElement();
		QDomElement image = document.createElement(QStringLiteral("image"));
		image.setAttribute(QStringLiteral("type"), QStringLiteral("png"));
		image.appendChild(document.createTextNode(QStringLiteral(
			"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==")));
		images.appendChild(image);
		QList<QetZip::Entry> saved = QetContainer::split(document, QString(), &error);
		QVERIFY2(!saved.isEmpty(), qPrintable(error));
		bool dropped = false;
		for (int i = 0 ; i < saved.size() ; ++i)
			if (saved.at(i).name.startsWith(QStringLiteral("images/"))) { saved.removeAt(i); dropped = true; break; }
		QVERIFY(dropped);
		QDomDocument strict;
		QVERIFY(!QetContainer::join(saved, &strict, &error));
		QVERIFY(error.contains(QStringLiteral("picture")));

		QDomDocument back;
		QStringList lost;
		QVERIFY2(QetContainer::recover(QetZip::toBytes(saved), &back, &lost, &error), qPrintable(error));
		QCOMPARE(lost.size(), 1);
		QVERIFY(lost.first().contains(QStringLiteral("picture")));
		QCOMPARE(int(back.documentElement().elementsByTagName(QStringLiteral("image")).size()), 0);
		QVERIFY(back.toString().contains(QStringLiteral("<elementInformation ")));
	}

	// What cannot be read as a zip at all is still refused
	void recoverNotAZipRefused()
	{
		QDomDocument back;
		QStringList lost;
		QString error;
		QVERIFY(!QetContainer::recover(QByteArray("not a zip").repeated(10), &back, &lost, &error));
		QVERIFY(!error.isEmpty());
	}
};

QTEST_APPLESS_MAIN(tst_qetcontainer)

#include "tst_qetcontainer.moc"
