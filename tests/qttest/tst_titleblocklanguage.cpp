// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QProcess>
#include <QTemporaryDir>

#include "diagram.h"
#include "properties/projectlanguages.h"
#include "qetproject.h"
#include "qettesttimeout.h"
#include "titleblockproperties.h"

// A title block draws its fields in the project's active language and keeps
// the words as written for the project database. A DXF writes the words
// themselves, so it follows the same language.
namespace {

ProjectLanguages german()
{
	ProjectLanguages languages;
	languages.addLanguage(QStringLiteral("de"));
	languages.addPhrase(QStringLiteral("Motor feed"));
	languages.addPhrase(QStringLiteral("Grain store"));
	languages.setTranslation(QStringLiteral("Motor feed"), QStringLiteral("de"), QStringLiteral("Motorzufuhr"));
	languages.setTranslation(QStringLiteral("Grain store"), QStringLiteral("de"), QStringLiteral("Getreidelager"));
	languages.setActiveLanguage(QStringLiteral("de"));
	return languages;
}

const QString project_xml = QStringLiteral(
		"<project title=\"l\" version=\"0.100.1\">"
		"<diagram title=\"Motor feed\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
		"<elements/><conductors/>"
		"<inputs><input x=\"40\" y=\"40\" text=\"Grain store\" rotation=\"0\"/></inputs>"
		"</diagram>"
		"<languages source=\"en\" active=\"%1\"><language code=\"de\"/>"
		"<phrase source=\"Motor feed\"><text language=\"de\" value=\"Motorzufuhr\"/></phrase>"
		"<phrase source=\"Grain store\"><text language=\"de\" value=\"Getreidelager\"/></phrase>"
		"</languages></project>");

} // namespace

class tst_titleblocklanguage : public QObject
{
	Q_OBJECT

	QStringList dxfOf(const QString &active)
	{
		QTemporaryDir dir;
		const QString in = dir.filePath(QStringLiteral("in.qet"));
		QFile file(in);
		if (!file.open(QIODevice::WriteOnly)) return {};
		file.write(project_xml.arg(active).toUtf8());
		file.close();
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--export-dxf"), in, dir.path()});
		if (!proc.waitForFinished(QetTest::timeout(60000))) return {};
		QStringList words;
		const QStringList files = QDir(dir.path()).entryList({QStringLiteral("*.dxf")});
		for (const QString &name : files) {
			QFile dxf(QDir(dir.path()).filePath(name));
			if (dxf.open(QIODevice::ReadOnly | QIODevice::Text))
				for (const QString &line : QString::fromUtf8(dxf.readAll()).split(QLatin1Char('\n')))
					words << line.trimmed();    //a title block cell starts with a space
		}
		return words;
	}

private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	void titleBlockShowsTheActiveLanguage()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		TitleBlockProperties title_block;
		title_block.title = QStringLiteral("Motor feed");
		title_block.context.addValue(QStringLiteral("area"), QStringLiteral("Grain store"));
		folio->border_and_titleblock.importTitleBlock(title_block);
		folio->border_and_titleblock.setFolioData(1, 1);
		project.setLanguages(german());

		const DiagramContext shown = folio->border_and_titleblock.shownTitleblockInformation();
		QCOMPARE(shown[QStringLiteral("title")].toString(), QStringLiteral("Motorzufuhr"));
		QCOMPARE(shown[QStringLiteral("area")].toString(), QStringLiteral("Getreidelager"));

			//What the project database reads stays as written
		const DiagramContext written = folio->border_and_titleblock.titleblockInformation();
		QCOMPARE(written[QStringLiteral("title")].toString(), QStringLiteral("Motor feed"));
		QCOMPARE(written[QStringLiteral("area")].toString(), QStringLiteral("Grain store"));
	}

	void collectsTitleBlocks()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		TitleBlockProperties title_block;
		title_block.title = QStringLiteral("Motor feed");
		title_block.context.addValue(QStringLiteral("area"), QStringLiteral("Grain store"));
		folio->border_and_titleblock.importTitleBlock(title_block);

		const QStringList texts = project.translatableTexts();
		for (const QString &expected : {QStringLiteral("Motor feed"), QStringLiteral("Grain store")})
			QVERIFY2(texts.contains(expected), qPrintable(expected));
	}

	void dxfWritesTheActiveLanguage()
	{
		const QStringList words = dxfOf(QStringLiteral("de"));
		QVERIFY(!words.isEmpty());
		QVERIFY(words.contains(QStringLiteral("Motorzufuhr")));
		QVERIFY(words.contains(QStringLiteral("Getreidelager")));
		QVERIFY(!words.contains(QStringLiteral("Motor feed")));
		QVERIFY(!words.contains(QStringLiteral("Grain store")));
	}

	void dxfOfTheSourceLanguageIsUnchanged()
	{
		const QStringList words = dxfOf(QStringLiteral("en"));
		QVERIFY(words.contains(QStringLiteral("Motor feed")));
		QVERIFY(words.contains(QStringLiteral("Grain store")));
		QVERIFY(!words.contains(QStringLiteral("Motorzufuhr")));
	}
};

QTEST_MAIN(tst_titleblocklanguage)
#include "tst_titleblocklanguage.moc"
