// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

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
	QString resave(const QString &in)
	{
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
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
