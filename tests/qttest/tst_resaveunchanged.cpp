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
// one that used to crash on opening (bugtracker #345).
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
