// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include "properties/projectlanguages.h"

// The texts of a project in other languages: what is shown for a text, how
// it is saved, and the CSV a translator works with.
class tst_projectlanguages : public QObject
{
	Q_OBJECT

	static ProjectLanguages german()
	{
		ProjectLanguages l;
		l.addLanguage(QStringLiteral("de"));
		l.addPhrase(QStringLiteral("Motor protection"));
		l.addPhrase(QStringLiteral("Total %{count} items"));
		l.setTranslation(QStringLiteral("Motor protection"), QStringLiteral("de"), QStringLiteral("Motorschutz"));
		return l;
	}

private slots:
	// Nothing changes until a language is chosen and has words for the text
	void sourceLanguageShowsTheSourceText()
	{
		ProjectLanguages l = german();
		QCOMPARE(l.activeLanguage(), QStringLiteral("en"));
		QCOMPARE(l.translate(QStringLiteral("Motor protection")), QStringLiteral("Motor protection"));
		l.setActiveLanguage(QStringLiteral("de"));
		QCOMPARE(l.translate(QStringLiteral("Motor protection")), QStringLiteral("Motorschutz"));
		// no words yet, or not a known text: shown as drawn
		QCOMPARE(l.translate(QStringLiteral("Total %{count} items")), QStringLiteral("Total %{count} items"));
		QCOMPARE(l.translate(QStringLiteral("Something else")), QStringLiteral("Something else"));
		l.setActiveLanguage(QStringLiteral("xx"));
		QCOMPARE(l.activeLanguage(), QStringLiteral("en"));
	}

	void referencesAreNotTranslatable_data()
	{
		QTest::addColumn<QString>("text");
		QTest::addColumn<bool>("translatable");
		QTest::newRow("words") << "Motor protection" << true;
		QTest::newRow("label") << "-K1" << false;
		QTest::newRow("value") << "24 VDC" << false;
		QTest::newRow("cross ref") << "/12.3" << false;
		QTest::newRow("number") << "12.5" << false;
		QTest::newRow("empty") << "" << false;
	}

	void referencesAreNotTranslatable()
	{
		QFETCH(QString, text);
		QFETCH(bool, translatable);
		QCOMPARE(ProjectLanguages::isTranslatable(text), translatable);
		ProjectLanguages l;
		l.addPhrase(text);
		QCOMPARE(!l.phrases().isEmpty(), translatable);
	}

	void xmlRoundTrip()
	{
		ProjectLanguages l = german();
		l.setActiveLanguage(QStringLiteral("de"));
		QDomDocument doc;
		const QDomElement xml = l.toXml(doc);
		QCOMPARE(xml.tagName(), QStringLiteral("languages"));
		QCOMPARE(ProjectLanguages::fromXml(xml), l);
		// a phrase with no words yet keeps its place
		QVERIFY(ProjectLanguages::fromXml(xml).phrases().contains(QStringLiteral("Total %{count} items")));
	}

	void emptyAndForeignXml()
	{
		QVERIFY(ProjectLanguages().isEmpty());
		QDomDocument doc;
		QVERIFY(ProjectLanguages::fromXml(doc.createElement(QStringLiteral("style"))).isEmpty());
		QDomElement bad = doc.createElement(QStringLiteral("languages"));
		bad.setAttribute(QStringLiteral("source"), QStringLiteral("not a code!"));
		QCOMPARE(ProjectLanguages::fromXml(bad).sourceLanguage(), QStringLiteral("en"));
	}

	void removingALanguageDropsItsWords()
	{
		ProjectLanguages l = german();
		l.setActiveLanguage(QStringLiteral("de"));
		l.removeLanguage(QStringLiteral("de"));
		QCOMPARE(l.languages(), QStringList{QStringLiteral("en")});
		QCOMPARE(l.activeLanguage(), QStringLiteral("en"));
		QVERIFY(l.translation(QStringLiteral("Motor protection"), QStringLiteral("de")).isEmpty());
	}

	void csvRoundTrip()
	{
		ProjectLanguages l = german();
		l.setTranslation(QStringLiteral("Motor protection"), QStringLiteral("de"), QStringLiteral("Motor, \"Schutz\"\nzwei"));
		const QByteArray csv = l.exportCsv();
		QVERIFY(csv.startsWith("\xEF\xBB\xBF"));
		ProjectLanguages other;
		other.addLanguage(QStringLiteral("de"));
		other.addPhrase(QStringLiteral("Motor protection"));
		other.addPhrase(QStringLiteral("Total %{count} items"));
		const auto result = other.importCsv(csv);
		QVERIFY2(result.ok, qPrintable(result.error));
		QCOMPARE(result.imported, 1);
		QCOMPARE(other.translation(QStringLiteral("Motor protection"), QStringLiteral("de")),
				 QStringLiteral("Motor, \"Schutz\"\nzwei"));
	}

	void csvSkipsWhatDoesNotFit()
	{
		ProjectLanguages l = german();
		const QByteArray csv =
				"Source,fr\r\n"
				"\"Motor protection\",\"Protection moteur\"\r\n"
				"\"Total %{count} items\",\"Total des articles\"\r\n"   // loses the field
				"\"Unknown text\",\"Inconnu\"\r\n";                    // not in the project
		const auto result = l.importCsv(csv);
		QVERIFY2(result.ok, qPrintable(result.error));
		QCOMPARE(result.imported, 1);
		QCOMPARE(result.warnings.size(), 2);
		QCOMPARE(l.translation(QStringLiteral("Motor protection"), QStringLiteral("fr")), QStringLiteral("Protection moteur"));
		QVERIFY(l.translation(QStringLiteral("Total %{count} items"), QStringLiteral("fr")).isEmpty());
		QVERIFY(l.languages().contains(QStringLiteral("fr")));
	}

	void csvRefusesAnythingBroken()
	{
		ProjectLanguages l = german();
		const ProjectLanguages before = l;
		QVERIFY(!l.importCsv("Name,de\r\nx,y\r\n").ok);          // no Source column
		QVERIFY(!l.importCsv("Source,de\r\n\"open,y\r\n").ok);   // unterminated quote
		QVERIFY(!l.importCsv("Source\r\nMotor protection\r\n").ok); // no language
		QVERIFY(!l.importCsv("Source,de\r\n\xFF\xFE\r\n").ok);   // not UTF-8
		QCOMPARE(l, before);
	}

	void semicolonCsvFromASpreadsheet()
	{
		ProjectLanguages l = german();
		const auto result = l.importCsv("Source;de\r\nMotor protection;Motorschutz 2\r\n");
		QVERIFY2(result.ok, qPrintable(result.error));
		QCOMPARE(l.translation(QStringLiteral("Motor protection"), QStringLiteral("de")), QStringLiteral("Motorschutz 2"));
	}
};

QTEST_GUILESS_MAIN(tst_projectlanguages)
#include "tst_projectlanguages.moc"
