// SPDX-License-Identifier: GPL-2.0-or-later
#include "materiallist/partsearch.h"
#include "qetapptest.h"

#include <QtTest>

/**
	Finding a part by the words typed (PartSearch): exact before prefix
	before inside before rough, every word needed, no rough match for
	short words or words with a digit, "*" as a wildcard.
*/
class tst_partsearch : public QObject
{
	Q_OBJECT

	static MaterialRecord part(const QString &number, const QString &description = QString(),
							   const QString &manufacturer = QString())
	{
		MaterialRecord record;
		record.setValue(QStringLiteral("manufacturer_reference"), number);
		record.setValue(QStringLiteral("description"), description);
		record.setValue(QStringLiteral("manufacturer"), manufacturer);
		return record;
	}

	static int scoreOf(const MaterialRecord &record, const QString &text)
	{
		return PartSearch::score(record, PartSearch::words(text));
	}

  private slots:
	void exactBeforePrefixBeforeInsideBeforeRough()
	{
		const int exact = scoreOf(part(QStringLiteral("X1"), QStringLiteral("relay")), QStringLiteral("relay"));
		const int prefix = scoreOf(part(QStringLiteral("X2"), QStringLiteral("relays")), QStringLiteral("relay"));
		const int inside = scoreOf(part(QStringLiteral("X3"), QStringLiteral("subrelay")), QStringLiteral("relay"));
		const int rough = scoreOf(part(QStringLiteral("X4"), QStringLiteral("relai")), QStringLiteral("relay"));
		QVERIFY(rough > 0);
		QVERIFY(exact > prefix);
		QVERIFY(prefix > inside);
		QVERIFY(inside > rough);
		QCOMPARE(scoreOf(part(QStringLiteral("X5"), QStringLiteral("lamp")), QStringLiteral("relay")), -1);

		const QList<MaterialRecord> records{
			part(QStringLiteral("X4"), QStringLiteral("relai")), part(QStringLiteral("X3"), QStringLiteral("subrelay")),
			part(QStringLiteral("X2"), QStringLiteral("relays")), part(QStringLiteral("X1"), QStringLiteral("relay"))};
		QCOMPARE(PartSearch::rank(records, QStringLiteral("relay")), (QList<int>{3, 2, 1, 0}));
		QCOMPARE(PartSearch::rank(records, QStringLiteral("relay"), 2), (QList<int>{3, 2}));
	}

	void misspelledWordsAreFound()
	{
		const MaterialRecord contactor = part(QStringLiteral("LC1D09BD"), QStringLiteral("CONTACTOR 3P 9A"), QStringLiteral("Schneider"));
		QVERIFY(scoreOf(contactor, QStringLiteral("contacter")) > 0);
		QVERIFY(scoreOf(contactor, QStringLiteral("scheider")) > 0);
		QVERIFY(scoreOf(contactor, QStringLiteral("shneider")) > 0);
			//Two letters off needs 8 letters
		QCOMPARE(scoreOf(part(QStringLiteral("A"), QStringLiteral("ABCDEF")), QStringLiteral("ABXXEF")), -1);
		QVERIFY(scoreOf(part(QStringLiteral("A"), QStringLiteral("CONTACTOR")), QStringLiteral("contxctor")) > 0);
		QCOMPARE(PartSearch::distance(QStringLiteral("ab"), QStringLiteral("ba")), 1);
	}

	void aWordBeingTypedMayHaveATypo()
	{
		MaterialRecord contactor;
		contactor.setValue(QStringLiteral("description"), QStringLiteral("MOTOR CONTACTOR 9A"));
			//"contc": a typo on the way to "contactor"
		QVERIFY(scoreOf(contactor, QStringLiteral("contc")) > 0);
		QVERIFY(scoreOf(contactor, QStringLiteral("conta")) > scoreOf(contactor, QStringLiteral("contc")));
		QCOMPARE(scoreOf(contactor, QStringLiteral("xyzq")), -1);
	}

	void shortWordsAndNumbersAreNeverRough()
	{
		const MaterialRecord twelve = part(QStringLiteral("P1"), QStringLiteral("Supply 12v"));
		QCOMPARE(scoreOf(twelve, QStringLiteral("24v")), -1);
		QVERIFY(scoreOf(twelve, QStringLiteral("12v")) > 0);
		QCOMPARE(scoreOf(part(QStringLiteral("P2"), QStringLiteral("rated 10A")), QStringLiteral("9a")), -1);
		QVERIFY(scoreOf(part(QStringLiteral("P3"), QStringLiteral("fuse")), QStringLiteral("fus")) > 0);
		QCOMPARE(scoreOf(part(QStringLiteral("P3"), QStringLiteral("fuse")), QStringLiteral("fue")), -1);
		QCOMPARE(scoreOf(part(QStringLiteral("LC1D09"), QStringLiteral("x")), QStringLiteral("LC1D90")), -1);
	}

	void everyWordIsNeeded()
	{
		const MaterialRecord contactor = part(QStringLiteral("LC1D09BD"), QStringLiteral("Contactor 24V"), QStringLiteral("Schneider"));
		QVERIFY(scoreOf(contactor, QStringLiteral("contactor 24v")) > 0);
		QVERIFY(scoreOf(contactor, QStringLiteral("24v schneider contactor")) > 0);
		QCOMPARE(scoreOf(contactor, QStringLiteral("contactor 48v")), -1);
		QCOMPARE(scoreOf(contactor, QStringLiteral("contactor siemens")), -1);
	}

	void emptyTextReturnsAllInListOrder()
	{
		const QList<MaterialRecord> records{part(QStringLiteral("Z")), part(QStringLiteral("A")), part(QStringLiteral("M"))};
		QCOMPARE(PartSearch::rank(records, QString()), (QList<int>{0, 1, 2}));
		QCOMPARE(PartSearch::rank(records, QStringLiteral("   ")), (QList<int>{0, 1, 2}));
		QCOMPARE(PartSearch::rank(records, QString(), 2), (QList<int>{0, 1}));
	}

	void equalScoresGoByPartNumber()
	{
		const QList<MaterialRecord> records{part(QStringLiteral("B2"), QStringLiteral("relay")),
											part(QStringLiteral("A1"), QStringLiteral("relay"))};
		QCOMPARE(PartSearch::rank(records, QStringLiteral("relay")), (QList<int>{1, 0}));
	}

	void accentsAndCaseAreIgnored()
	{
		const MaterialRecord record = part(QStringLiteral("R1"), QStringLiteral("Relais électrique"));
		QVERIFY(scoreOf(record, QStringLiteral("ELECTRIQUE")) > 0);
		QVERIFY(scoreOf(record, QStringLiteral("électrique")) > 0);
	}

	void partNumbersAreKeptWhole()
	{
		const MaterialRecord record = part(QStringLiteral("XB5-100-M09"), QStringLiteral("button"));
		QVERIFY(scoreOf(record, QStringLiteral("100-M09")) > 0);
		QVERIFY(scoreOf(record, QStringLiteral("100")) > 0);
		QVERIFY(scoreOf(record, QStringLiteral("xb5-100-m09")) > scoreOf(record, QStringLiteral("100-M09")));
	}

	void wildcard()
	{
		const MaterialRecord match = part(QStringLiteral("LC1D09BD"));
		QVERIFY(scoreOf(match, QStringLiteral("LC1D*BD")) > 0);
		QVERIFY(scoreOf(part(QStringLiteral("LC1D25BD")), QStringLiteral("LC1D*BD")) > 0);
		QCOMPARE(scoreOf(part(QStringLiteral("LC1D09B7")), QStringLiteral("LC1D*BD")), -1);
		QCOMPARE(scoreOf(part(QStringLiteral("XLC1D09BD")), QStringLiteral("LC1D*BD")), -1);
		QVERIFY(scoreOf(part(QStringLiteral("XLC1D09BD")), QStringLiteral("*LC1D*BD")) > 0);
	}
};

QET_APP_TEST_MAIN(tst_partsearch)
#include "tst_partsearch.moc"
