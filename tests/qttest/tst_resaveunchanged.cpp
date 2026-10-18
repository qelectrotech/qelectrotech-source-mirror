// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

#include "container/qetzip.h"

#include <sqlite3.h>

// Saving a project that was just saved must change nothing. Two things
// made the second save differ from the first, both cleanup done on save
// but not on load:
//  - symbol information whose values were all empty was written as an
//    empty <elementInformations/> block, which the next load read as no
//    information and the next save dropped (Projet_vierge.qet);
//  - information values were trimmed on save but not on load, so a label
//    with stray spaces kept them in its displayed copy until the project
//    was opened again (m_000.qet).
// Runs the real binary's --resave twice on every example, on a project
// whose title block holds a value that is a single space (#973), and on
// one that used to crash on opening (bugtracker #345); and that a coil's
// contacts keep the order the file saved them in.
class tst_resaveunchanged : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// --resave @p in to a new file, in a sandbox of its own (so a running
	// QElectroTech cannot answer instead); returns the new file's path.
	QString resave(const QString &in, const QString &suffix = QStringLiteral(".qet"))
	{
			//Always p.qet, in its own folder: an untitled project takes its
			//file name as title, so two names would make two projects.
		QDir().mkpath(m_dir.filePath(QStringLiteral("run%1").arg(m_run)));
		const QString out = m_dir.filePath(QStringLiteral("run%1/p").arg(m_run) + suffix);
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0) return {};
		return out;
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// Projet_vierge.qet has empty information values, m_000.qet values
	// with stray spaces; every other example is here so a new cause shows.
	void secondSaveChangesNothing_data()
	{
		QTest::addColumn<QString>("project");
		const QDir examples(QStringLiteral(QET_EXAMPLES_DIR));
		const QStringList projects =
				examples.entryList({QStringLiteral("*.qet")}, QDir::Files, QDir::Name);
		QVERIFY(!projects.isEmpty());
		for (const QString &project : projects)
			QTest::newRow(project.toUtf8().constData()) << examples.filePath(project);
	}

	void secondSaveChangesNothing()
	{
		QFETCH(QString, project);
		const QString first = resave(project);
		QVERIFY2(!first.isEmpty(), "first --resave failed");
		const QString second = resave(first);
		QVERIFY2(!second.isEmpty(), "second --resave failed");
		const QByteArray a = read(first), b = read(second);
		QVERIFY(!a.isEmpty());
		QVERIFY2(a == b, "the second save changed the file");
	}

	// A contact not linked to a coil, carrying a text built from %{label}:
	// opening it dereferenced the missing coil and crashed (bugtracker #345).
	// The fixture is a blank project with one such contact.
	void unlinkedContactLabelTextOpens()
	{
		const QString fixture = QFINDTESTDATA("fixtures/unlinked_contact_label.qet");
		QVERIFY(!fixture.isEmpty());
		const QString first = resave(fixture);
		QVERIFY2(!first.isEmpty(), "--resave failed: QElectroTech crashed opening the project");
		const QString second = resave(first);
		QVERIFY2(!second.isEmpty(), "second --resave failed");
		QVERIFY2(read(first) == read(second), "the second save changed the file");
	}

	// %{machine_manufacturer_reference_auxiliary1..4} resolve like their
	// neighbours: they were missing from AssignVariables::replaceVariable()
	// and printed as literal text. The fixture has one terminal whose
	// four texts are "[%{machine_..._auxiliaryN}|%{manufacturer_reference_auxiliaryN}]",
	// saved by a build without the fix, so the literal text is in the file.
	void auxiliaryMachineReferenceResolves()
	{
		const QString fixture = QFINDTESTDATA("fixtures/aux_machine_reference.qet");
		QVERIFY(!fixture.isEmpty());
		const QString saved = resave(fixture);
		QVERIFY2(!saved.isEmpty(), "--resave failed");
		const QString xml = QString::fromUtf8(read(saved));
		for (int n = 1; n <= 4; ++n) {
			const QString shown = QStringLiteral("<text>[MMR-AUX%1|MR-AUX%1]</text>").arg(n);
			QVERIFY2(xml.contains(shown), qPrintable(shown + QStringLiteral(" not in the saved file")));
		}
		QVERIFY2(!xml.contains(QStringLiteral("<text>[%{machine")),
				 "a machine manufacturer reference variable was left unresolved");
	}

	// A coil's contacts are saved in the order the file listed them.
	// They used to come back in the order the folios were visited, so a
	// list that disagreed with it changed on every save, and contacts on
	// one folio swapped places from run to run. m_000.qet's coil b02216df
	// (folio 13) lists its contacts on folios 7 and 12; the test lists
	// them the other way round.
	void savedLinkOrderKept()
	{
		QByteArray xml = read(QStringLiteral(QET_EXAMPLES_DIR "/m_000.qet"));
		const QRegularExpression coil(QStringLiteral(
				"<element [^>]*uuid=\"\\{b02216df-0851-4732-893b-901fea80703e\\}\""));
		// byte offset of the coil's <element> tag in @p file, or -1
		const auto coilAt = [&coil](const QByteArray &file) {
			const QRegularExpressionMatch m = coil.match(QString::fromLatin1(file));
			return m.hasMatch() ? int(m.capturedStart()) : -1;
		};
		const QByteArray a = "<link_uuid uuid=\"{998190fa-5b4c-4be3-bc67-8a828eaab2b4}\"/>";
		const QByteArray b = "<link_uuid uuid=\"{112aa911-",
				b_end = "\"/>";
		const int start = coilAt(xml);
		QVERIFY(start > 0);
		const int ia = xml.indexOf(a, start), ib = xml.indexOf(b, start);
		QVERIFY2(ia > 0 && ib > ia, "m_000.qet no longer lists the coil's contacts as expected");
		const QByteArray link_b = xml.mid(ib, xml.indexOf(b_end, ib) + b_end.size() - ib);
		xml.replace(ib, link_b.size(), a);
		xml.replace(ia, a.size(), link_b);
		const QString in = m_dir.filePath(QStringLiteral("links.qet"));
		QFile f(in);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(xml);
		f.close();

		const QByteArray saved = read(resave(in));
		QVERIFY2(!saved.isEmpty(), "--resave failed");
		const int s = coilAt(saved);
		QVERIFY(s > 0);
		const int sa = saved.indexOf(a, s), sb = saved.indexOf(b, s);
		QVERIFY2(sa > 0 && sb > 0, "a link was lost");
		QVERIFY2(sb < sa, "the coil's contacts were saved in another order than the file's");
	}

	// A table split over several folios whose part names a previous part
	// that is not in the file: opening and closing it crashed (the part
	// has no data of its own and handed its missing data on when it was
	// destroyed). industrial.qet's third <previous_table> is changed.
	void missingPreviousTableOpens()
	{
		QByteArray xml = read(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		int at = -1;
		for (int n = 0 ; n < 3 ; ++n) {
			at = xml.indexOf("<previous_table uuid=\"", at + 1);
			QVERIFY(at > 0);
		}
		at += int(qstrlen("<previous_table uuid=\""));
		xml.replace(at, 38, "{00000000-0000-4000-8000-000000000000}");
		const QString in = m_dir.filePath(QStringLiteral("missing_previous.qet"));
		QFile f(in);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(xml);
		f.close();
		QVERIFY2(!resave(in).isEmpty(), "--resave failed: QElectroTech crashed closing the project");
	}

	// Deleting the folio that holds a middle part of a split table, then
	// saving: every <previous_table> in the file names a table that is in
	// it. It used to name the deleted part, so every part after it opened
	// with no data. industrial.qet's folio 46 (index 45) holds such a part.
	void deletedFolioKeepsTableChain()
	{
#ifndef QET_HAS_SCRIPTING
		QSKIP("needs --run: this QElectroTech is built without Qt Qml");
#endif
		const QString out = m_dir.filePath(QStringLiteral("deleted_folio.qet"));
		const QString script = m_dir.filePath(QStringLiteral("deleted_folio.js"));
		QFile js(script);
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write(QStringLiteral("qet.log('REMOVED ' + qet.removeFolio(45));\n"
								"qet.log('SAVED ' + qet.save('%1'));\n").arg(out).toUtf8());
		js.close();
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), script, QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet")});
		QVERIFY(proc.waitForFinished(180000));
		const QString log = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		QVERIFY2(log.contains(QStringLiteral("REMOVED true")) && log.contains(QStringLiteral("SAVED true")),
				 qPrintable(log.right(400)));

		const QString saved = QString::fromUtf8(read(out));
		QSet<QString> tables;
		for (const auto &m : QRegularExpression(QStringLiteral("<graphics_table [^>]*uuid=\"([^\"]+)\"")).globalMatch(saved))
			tables.insert(m.captured(1));
		int references = 0;
		for (const auto &m : QRegularExpression(QStringLiteral("<previous_table uuid=\"([^\"]+)\"")).globalMatch(saved)) {
			++references;
			QVERIFY2(tables.contains(m.captured(1)),
					 qPrintable(QStringLiteral("a table names %1, which is not in the file").arg(m.captured(1))));
		}
		QVERIFY(references > 0);
	}

	// Deleting the folio that holds the first part of a split table, with
	// folios opened lazily (the default), then saving: the next part keeps
	// the table's query (F121). The first part holds it; the next part takes
	// it over only if it was built when the folio went, and it used to be
	// built later, from the file, naming a part that was gone and saving no
	// query. industrial.qet's folio 41 (index 40) holds the first part of its
	// 10-part parts list. It is resaved first: its folios have no saved uuid,
	// and such a project opens with every folio built. Every table in the
	// file must name a table that is in it, or have a query.
	void deletedFirstPartKeepsTableQuery()
	{
#ifndef QET_HAS_SCRIPTING
		QSKIP("needs --run: this QElectroTech is built without Qt Qml");
#endif
		const QString in = resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY(!in.isEmpty());
		const QString out = m_dir.filePath(QStringLiteral("deleted_first_part.qet"));
		const QString script = m_dir.filePath(QStringLiteral("deleted_first_part.js"));
		QFile js(script);
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write(QStringLiteral("qet.log('REMOVED ' + qet.removeFolio(40));\n"
								"qet.log('SAVED ' + qet.save('%1'));\n").arg(out).toUtf8());
		js.close();
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), script, in});
		QVERIFY(proc.waitForFinished(180000));
		const QString log = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		QVERIFY2(log.contains(QStringLiteral("REMOVED true")) && log.contains(QStringLiteral("SAVED true")),
				 qPrintable(log.right(400)));

		const QString saved = QString::fromUtf8(read(out));
		const QRegularExpression table_re(
					QStringLiteral("<graphics_table [^>]*uuid=\"([^\"]+)\"[^>]*>(.*?)</graphics_table>"),
					QRegularExpression::DotMatchesEverythingOption);
		QHash<QString, QString> tables;   // uuid -> what is inside the table
		for (const auto &m : table_re.globalMatch(saved))
			tables.insert(m.captured(1), m.captured(2));
		QVERIFY(tables.size() >= 9);
		const QRegularExpression previous_re(QStringLiteral("<previous_table uuid=\"([^\"]+)\""));
		const QRegularExpression query_re(QStringLiteral("<query>\\s*\\S"));
		for (auto it = tables.cbegin() ; it != tables.cend() ; ++it) {
			const auto previous = previous_re.match(it.value());
			QVERIFY2(query_re.match(it.value()).hasMatch()
					 || (previous.hasMatch() && tables.contains(previous.captured(1))),
					 qPrintable(QStringLiteral("table %1 has no query and names no table in the file")
								.arg(it.key())));
		}
	}

	// A folio report arrow linked to one that has no terminal -- such as
	// ref_voyant_2_h.elmt from the collection -- crashed QElectroTech on
	// opening: the arrow's text looked for a wire on the other arrow's
	// first terminal. Projet_vierge.qet with a previous-folio arrow (one
	// terminal, a label text) on folio 1 linked to a ref_voyant_2_h on
	// folio 2.
	void reportArrowWithoutTerminalOpens()
	{
		QByteArray xml = read(QStringLiteral(QET_EXAMPLES_DIR "/Projet_vierge.qet"));
		const QByteArray a = "{0a000000-0000-4000-8000-00000000000a}",
				b = "{0b000000-0000-4000-8000-00000000000b}";
		const QByteArray arrow =
				"<element type=\"embed://import/10_electric/10_allpole/100_sheet_referencing/01previous_folio.elmt\""
				" x=\"200\" y=\"200\" z=\"10\" orientation=\"0\" prefix=\"\" freezeLabel=\"false\" uuid=\"" + a + "\">"
				"<terminals/><inputs/><links_uuids><link_uuid uuid=\"" + b + "\"/></links_uuids>"
				"<dynamic_texts><dynamic_elmt_text text_from=\"ElementInfo\" x=\"5\" y=\"-10\""
				" uuid=\"{0c000000-0000-4000-8000-00000000000c}\" rotation=\"0\" frame=\"false\" text_width=\"-1\">"
				"<text>x</text><info_name>label</info_name></dynamic_elmt_text></dynamic_texts>"
				"<texts_groups/></element>";
		const QByteArray no_terminal =
				"<element type=\"embed://import/10_electric/98_graphics/01_auxiliary_symbols/01_cross_ref_symbols/"
				"01_with_linking_function/01_parents/ref_voyant_2_h.elmt\""
				" x=\"300\" y=\"200\" z=\"10\" orientation=\"0\" prefix=\"\" freezeLabel=\"false\" uuid=\"" + b + "\">"
				"<terminals/><inputs/><links_uuids><link_uuid uuid=\"" + a + "\"/></links_uuids>"
				"<dynamic_texts/><texts_groups/></element>";
		QVERIFY(xml.contains("ref_voyant_2_h.elmt") && xml.contains("01previous_folio.elmt"));
		// put @p element first in the elements of folio @p n
		auto put = [&xml](int n, const QByteArray &element) {
			int at = -1;
			for (int i = 0 ; i <= n ; ++i) {
				at = xml.indexOf("<diagram ", at + 1);
				if (at < 0) return false;
			}
			const int empty = xml.indexOf("<elements/>", at), open = xml.indexOf("<elements>", at),
					end = xml.indexOf("</diagram>", at);
			if (empty >= 0 && empty < end && (open < 0 || empty < open))
				xml.replace(empty, int(qstrlen("<elements/>")), "<elements>" + element + "</elements>");
			else if (open >= 0 && open < end)
				xml.insert(open + int(qstrlen("<elements>")), element);
			else
				return false;
			return true;
		};
		QVERIFY(put(0, arrow));
		QVERIFY(put(1, no_terminal));
		const QString in = m_dir.filePath(QStringLiteral("report_without_terminal.qet"));
		QFile f(in);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(xml);
		f.close();
		const QString saved = resave(in);
		QVERIFY2(!saved.isEmpty(), "--resave failed: QElectroTech crashed opening the project");
		QVERIFY2(read(saved).contains(b), "the arrow without a terminal was not kept");
	}

	// Saved as a .qetz (the zipped project, #1440) and that saved again as a
	// .qet, a project is what it is saved straight as a .qet: the same
	// file but for when and where it was saved. The examples with pictures
	// (weneedpolonez), tables and links (industrial) and several title
	// blocks (m_000).
	void containerSavesTheSameProject_data()
	{
		QTest::addColumn<QString>("project");
		for (const char *name : {"Projet_vierge.qet", "industrial.qet", "m_000.qet",
								 "weneedpolonez-Polonez_MR89_wiring_diagram.qet"})
			QTest::newRow(name) << QStringLiteral(QET_EXAMPLES_DIR "/") + QLatin1String(name);
	}
	void containerSavesTheSameProject()
	{
		QFETCH(QString, project);
		const QString straight = resave(project);
		QVERIFY2(!straight.isEmpty(), "--resave as .qet failed");
		const QString zipped = resave(project, QStringLiteral(".qetz"));
		QVERIFY2(!zipped.isEmpty(), "--resave as .qetz failed");
		QVERIFY2(read(zipped).startsWith("PK\x03\x04"), "the .qetz is not a zip");
		QVERIFY2(read(zipped).size() < read(straight).size() / 2, "the .qetz is not smaller");
		QCOMPARE(databaseTables(zipped), QStringList({
			"conductor", "element", "element_info", "folio", "folio_property", "link", "meta",
			"project_property", "strip_bridge", "strip_bridge_terminal", "strip_terminal",
			"terminal_strip"}));
		const QString back = resave(zipped);
		QVERIFY2(!back.isEmpty(), "opening the .qetz failed");

		auto without_save_stamps = [](QByteArray xml) {
			static const QRegularExpression stamp(QStringLiteral(
				"\\s*<property [^>]*name=\"saved(filename|filepath|filenamedir|date|date-us|date-eu|time)\"[^>]*>[^<]*</property>"));
			return QString::fromUtf8(xml).remove(stamp);
		};
		QCOMPARE(without_save_stamps(read(back)), without_save_stamps(read(straight)));
	}

	// The tables of the database in the .qetz @p path: the document's
	// engineering data only, each fact once -- not QElectroTech's own
	// project database, built from it when the project opens
	static QStringList databaseTables(const QString &path)
	{
		QList<QetZip::Entry> entries;
		if (!QetZip::fromBytes(read(path), &entries)) return {QStringLiteral("not a zip")};
		QByteArray database;
		for (const QetZip::Entry &entry : std::as_const(entries))
			if (entry.name == QLatin1String("project.sqlite")) database = entry.data;
		sqlite3 *db = nullptr;
		sqlite3_open(":memory:", &db);
		unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(database.size())));
		memcpy(copy, database.constData(), size_t(database.size()));
		sqlite3_deserialize(db, "main", copy, database.size(), database.size(), SQLITE_DESERIALIZE_FREEONCLOSE);
		sqlite3_stmt *list = nullptr;
		QStringList tables;
		sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name", -1, &list, nullptr);
		while (list && sqlite3_step(list) == SQLITE_ROW)
			tables << QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(list, 0)));
		sqlite3_finalize(list);
		sqlite3_close(db);
		return tables;
	}

	// QElectroTech run in a sandbox of its own with @p args, the store
	// check on (QET_CHECK_ELEMENT_INFO_STORE); its output
	QString runChecked(const QStringList &args, bool eager = false,
					   const char *check = "QET_CHECK_ELEMENT_INFO_STORE")
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QString::fromLatin1(check), QStringLiteral("1"));
		if (eager) {
			env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1")); //every folio built on opening
		}
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), args);
		if (!proc.waitForFinished(180000)) return {};
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	// The stores of symbol information and folio title blocks agree with
	// every placed symbol
	// and folio once a project is open: every example.
	void elementInfoStoreAgrees_data()
	{
		QTest::addColumn<QString>("project");
		const QDir examples(QStringLiteral(QET_EXAMPLES_DIR));
		for (const QString &f : examples.entryList({QStringLiteral("*.qet")}, QDir::Files, QDir::Name))
			QTest::newRow(f.toUtf8().constData()) << examples.filePath(f);
	}
	void elementInfoStoreAgrees()
	{
		QFETCH(QString, project);
		const QString out = runChecked({QStringLiteral("--resave"), project,
										m_dir.filePath(QStringLiteral("store%1.qet").arg(m_run))});
		QVERIFY2(out.contains(QStringLiteral("element information store: 0 differ")), qPrintable(out.right(600)));
		QVERIFY2(out.contains(QStringLiteral("folio title block store: 0 differ")), qPrintable(out.right(600)));
		QVERIFY2(out.contains(QStringLiteral("wire properties store: 0 differ")), qPrintable(out.right(600)));
	}

	// The link rows say what each symbol saves, in its order, also after
	// an edit builds a partner's folio (lazy folios): m_000 as a .qetz
	// saved with every folio built, a symbol of its middle folio moved. The rows used to follow the order
	// the partners were built in (QETZ-DB-FIRST stage 2a).
	void linkRowsAfterLazyEdit()
	{
#ifndef QET_HAS_SCRIPTING
		QSKIP("needs --run: this QElectroTech is built without Qt Qml");
#endif
		const QString qetz = m_dir.filePath(QStringLiteral("links%1.qetz").arg(m_run));
		QVERIFY(!runChecked({QStringLiteral("--resave"),
							 QStringLiteral(QET_EXAMPLES_DIR "/m_000.qet"), qetz}, true).isEmpty());
		const QString saved = m_dir.filePath(QStringLiteral("links-moved%1.qetz").arg(m_run));
		const QString js = m_dir.filePath(QStringLiteral("links%1.js").arg(m_run));
		QFile script(js);
		QVERIFY(script.open(QIODevice::WriteOnly));
		script.write(QStringLiteral(
			"var f = Math.floor(qet.folioCount() / 2);\n"
			"var u = qet.elementUuids(f).slice().sort()[0];\n"
			"qet.log('MOVED ' + qet.moveElement(f, u, 0, 100));\n"
			"qet.save('%1');\n").arg(saved).toUtf8());
		script.close();
		const QString out = runChecked({QStringLiteral("--run"), js, qetz}, false,
									   "QET_CHECK_LINK_ROWS");
		QVERIFY2(out.contains(QStringLiteral("MOVED true")), qPrintable(out.right(600)));
		QVERIFY2(out.contains(QStringLiteral("link rows: 0 differ")), qPrintable(out.right(600)));
	}

	// The stores filled from the document before the folios are built
	// hold what building them gives, for a
	// project saved by this version (every symbol, folio and wire carries
	// a uuid): nothing differs, nothing is left unplaced.
	void storesFilledBeforeBuilding()
	{
		const QString saved = resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		QVERIFY2(!saved.isEmpty(), "--resave failed");
		const QString out = runChecked({QStringLiteral("--resave"), saved,
										m_dir.filePath(QStringLiteral("prefilled.qet"))}, true);
		const QRegularExpression line(QStringLiteral(
				"filled before building: symbols (\\d+) agree, 0 differ, 0 not placed; "
				"folios (\\d+) agree, 0 differ, 0 not placed; wires (\\d+) agree, 0 differ, 0 not placed"));
		const QRegularExpressionMatch m = line.match(out);
		QVERIFY2(m.hasMatch(), qPrintable(out.right(600)));
		QVERIFY(m.captured(1).toInt() > 600 && m.captured(2).toInt() == 50 && m.captured(3).toInt() > 600);
	}

	// Title block fields stamped on every folio go through the store, and
	// the saved folios carry them (a folio saves what the store holds).
	void folioTitleBlockStoreAfterSetTitleBlock()
	{
		const QString saved = m_dir.filePath(QStringLiteral("stamped.qet"));
		const QString out = runChecked({QStringLiteral("--set-titleblock"),
										QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), saved,
										QStringLiteral("author=Ann"), QStringLiteral("extrafield=Hello")});
		QVERIFY2(out.contains(QStringLiteral("folio title block store: 0 differ")), qPrintable(out.right(600)));
		const QByteArray xml = read(saved);
		QCOMPARE(xml.count("<diagram "), xml.count("author=\"Ann\"") - 1);
		QCOMPARE(xml.count("<diagram "), xml.count("name=\"extrafield\"") - 1);
	}

	// ... and after a copy of a symbol, which briefly shares its original's
	// uuid, and an undo: the original kept no row once (duplicateElements
	// is the copy the paste and folio duplication make).
	void elementInfoStoreAfterCopyAndUndo()
	{
#ifndef QET_HAS_SCRIPTING
		QSKIP("needs --run: this QElectroTech is built without Qt Qml");
#endif
		const QString script = m_dir.filePath(QStringLiteral("copy.js"));
		QFile js(script);
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write(QStringLiteral(
				 "var f = 0; while (qet.elementUuids(f).length < 3) ++f;\n"
				 "var e = qet.elementUuids(f);\n"
				 "qet.log('COPY ' + qet.duplicateElements(f, [e[0], e[1]], f, 300, 300).length);\n"
				 "qet.log('DELETE ' + qet.deleteElement(f, e[2]) + ' UNDO ' + qet.undo());\n"
				 "qet.log('SAVED ' + qet.save('%1'));\n").arg(m_dir.filePath(QStringLiteral("copied.qet"))).toUtf8());
		js.close();
		const QString out = runChecked({QStringLiteral("--run"), script,
										QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet")});
		QVERIFY2(out.contains(QStringLiteral("COPY 2")) && out.contains(QStringLiteral("UNDO true"))
				 && out.contains(QStringLiteral("SAVED true")), qPrintable(out.right(600)));
		QVERIFY2(out.contains(QStringLiteral("element information store: 0 differ")), qPrintable(out.right(600)));
	}

	// After copies of linked symbols, of wired symbols, and the undo of a
	// folio's removal, the project database's symbol, link, wire and
	// terminal rows are those a fresh open of the saved project builds
	// (F106, F107): a copy is placed while it still carries its original's
	// uuid, and an undone folio removal put back its symbols but not their
	// rows.
	void symbolRowsAfterCopyAndFolioUndo()
	{
#ifndef QET_HAS_SCRIPTING
		QSKIP("needs --run: this QElectroTech is built without Qt Qml");
#endif
		const QString rows = QStringLiteral(
				 "function rows() { return JSON.stringify(["
				 "qet.query('SELECT uuid, diagram_uuid, type FROM element ORDER BY uuid'),"
				 "qet.query('SELECT element_uuid, label, comment FROM element_info ORDER BY element_uuid'),"
				 "qet.query('SELECT element_uuid, linked_uuid FROM link ORDER BY element_uuid, linked_uuid'),"
				 "qet.query('SELECT uuid, terminal1_element_uuid, terminal2_element_uuid, text FROM conductor ORDER BY uuid'),"
				 "qet.query('SELECT uuid, element_uuid FROM terminal ORDER BY uuid, element_uuid')]); }\n");
		const QString edit = m_dir.filePath(QStringLiteral("rows_edit.js")),
				dump = m_dir.filePath(QStringLiteral("rows_dump.js")),
				saved = m_dir.filePath(QStringLiteral("rows.qet"));
		QFile js(edit);
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write((rows + QStringLiteral(
				 "var linked = -1;\n"
				 "for (var f = 0; f < qet.folioCount() && linked < 0; ++f)\n"
				 "  qet.elementUuids(f).forEach(function (u) { if (qet.linkedElements(f, u).length) linked = f; });\n"
				 "qet.log('UNDONE ' + (qet.removeFolio(linked) && qet.undo()));\n"
				 "var copied = 0;\n"
				 "for (var f = 0; f < qet.folioCount() && copied < 3; ++f) {\n"
				 "  var e = qet.elementUuids(f);\n"
				 "  for (var i = 0; i < e.length && copied < 3; ++i)\n"
				 "    if (qet.linkedElements(f, e[i]).length)\n"
				 "      copied += qet.duplicateElements(f, [e[i]], f, 300, 300).length;\n"
				 "}\n"
				 "qet.log('COPIED ' + copied);\n"
				 "var wf = 0; while (qet.elementUuids(wf).length < 6) ++wf;\n"
				 "var wires = qet.conductorCount(wf), w = qet.elementUuids(wf);\n"
				 "qet.log('WIRED ' + qet.duplicateElements(wf, w.slice(0, 6), wf, 300, 300).length"
				 " + (qet.conductorCount(wf) > wires ? ' with wires' : ''));\n"
				 "qet.log('ROWS ' + rows());\n"
				 "qet.log('SAVED ' + qet.save('%1'));\n").arg(saved)).toUtf8());
		js.close();
		QFile js2(dump);
		QVERIFY(js2.open(QIODevice::WriteOnly));
		js2.write((rows + QStringLiteral("qet.log('ROWS ' + rows());\n")).toUtf8());
		js2.close();

		const QString edited = runChecked({QStringLiteral("--run"), edit,
										   QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet")});
		QVERIFY2(edited.contains(QStringLiteral("COPIED 3")) && edited.contains(QStringLiteral("WIRED 6 with wires"))
				 && edited.contains(QStringLiteral("UNDONE true"))
				 && edited.contains(QStringLiteral("SAVED true")), qPrintable(edited.right(600)));
		const QString reopened = runChecked({QStringLiteral("--run"), dump, saved});
		auto rowsOf = [](const QString &log) {
			const int at = log.indexOf(QStringLiteral("ROWS "));
			return at < 0 ? QString() : log.mid(at + 5, log.indexOf(QLatin1Char('\n'), at) - at - 5);
		};
		QVERIFY(!rowsOf(reopened).isEmpty());
		QCOMPARE(rowsOf(edited), rowsOf(reopened));
	}

	// A project read from a .qet and saved where it is, is saved as a .qetz
	// beside it, and the .qet is left as it was (stage D1).
	// What the .qetz holds is what a plain save of the .qet holds.
	void qetSavedAsQetzBeside()
	{
#ifndef QET_HAS_SCRIPTING
		QSKIP("needs --run: this QElectroTech is built without Qt Qml");
#endif
		const QString qet = m_dir.filePath(QStringLiteral("beside.qet"));
		QFile::remove(qet);
		QVERIFY(QFile::copy(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), qet));
		const QByteArray before = read(qet);
		const QString script = m_dir.filePath(QStringLiteral("beside.js"));
		QFile js(script);
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write("qet.log('SAVED ' + qet.save(''));\n");
		js.close();
		const QString log = runChecked({QStringLiteral("--run"), script, qet});
		QVERIFY2(log.contains(QStringLiteral("SAVED true")), qPrintable(log.right(400)));

		QCOMPARE(read(qet), before);
		const QString qetz = m_dir.filePath(QStringLiteral("beside.qetz"));
		QVERIFY2(read(qetz).startsWith("PK\x03\x04"), "no .qetz beside the .qet");

		auto without_save_stamps = [](QByteArray xml) {
			static const QRegularExpression stamp(QStringLiteral(
				"\\s*<property [^>]*name=\"saved(filename|filepath|filenamedir|date|date-us|date-eu|time)\"[^>]*>[^<]*</property>"));
			return QString::fromUtf8(xml).remove(stamp);
		};
		const QString straight = resave(qet);
		const QString back = resave(qetz);
		QVERIFY(!straight.isEmpty() && !back.isEmpty());
		QCOMPARE(without_save_stamps(read(back)), without_save_stamps(read(straight)));
	}

	// A title-block value that is a single space is kept through two saves
	// (#973), and a value with accents comes back as it went in.
	void singleSpaceValueKept()
	{
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
		QSKIP("QDomDocument::PreserveSpacingOnlyNodes needs Qt 6.5 (see QETProject::openFile)");
#endif
		QByteArray xml = read(QStringLiteral(QET_EXAMPLES_DIR "/Projet_vierge.qet"));
		QVERIFY(xml.contains("<properties>"));
		xml.replace("<properties>",
					"<properties>"
					"<property show=\"1\" name=\"space\"> </property>"
					"<property show=\"1\" name=\"accents\">Armoire façade été</property>");
		const QString in = m_dir.filePath(QStringLiteral("space.qet"));
		QFile f(in);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(xml);
		f.close();

		const QString first = resave(in);
		QVERIFY2(!first.isEmpty(), "first --resave failed");
		const QString second = resave(first);
		QVERIFY2(!second.isEmpty(), "second --resave failed");
		const QByteArray a = read(first), b = read(second);
		QVERIFY2(a == b, "the second save changed the file");

		const QString saved = QString::fromUtf8(b);
		QVERIFY2(saved.contains(QRegularExpression(
					 QStringLiteral("<property [^>]*name=\"space\"[^>]*> </property>"))),
				 "the single-space value was lost");
		QVERIFY2(saved.contains(QRegularExpression(
					 QStringLiteral("<property [^>]*name=\"accents\"[^>]*>Armoire façade été</property>"))),
				 "the accented value changed");
	}
};

QTEST_APPLESS_MAIN(tst_resaveunchanged)

#include "tst_resaveunchanged.moc"
