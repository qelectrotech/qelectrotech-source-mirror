#include <QtTest>

#include "editor/terminalnamecheck.h"

class tst_terminalnamecheck : public QObject
{
	Q_OBJECT

private slots:
	void uniqueNamesPass()
	{
		QVERIFY(TerminalNameCheck::repeatedNames({"A1", "A2", "13", "14"}).isEmpty());
	}

	// Shelly Pro 2PM in the shipped collection: three terminals named N.
	void repeatedNamesAreCounted()
	{
		const auto repeated = TerminalNameCheck::repeatedNames(
			{"L", "N", "O1", "N", "O2", "N", "L"});
		QCOMPARE(repeated.size(), 2);
		QCOMPARE(repeated.at(0), qMakePair(QString("L"), 2));
		QCOMPARE(repeated.at(1), qMakePair(QString("N"), 3));
		QCOMPARE(TerminalNameCheck::describe(repeated),
				 QString::fromUtf8("L ×2, N ×3"));
	}

	void surroundingSpacesAreIgnored()
	{
		QCOMPARE(TerminalNameCheck::repeatedNames({"PE", " PE "}).size(), 1);
	}

	void caseMatters()
	{
		QVERIFY(TerminalNameCheck::repeatedNames({"n", "N"}).isEmpty());
	}

	// Unnamed terminals are reported as unnamed, never as a repeat of "".
	void unnamedAreNotRepeats()
	{
		const QStringList names{"", " ", "1", ""};
		QVERIFY(TerminalNameCheck::repeatedNames(names).isEmpty());
		QCOMPARE(TerminalNameCheck::unnamedCount(names), 3);
	}

	// describe() goes into a tr() message with %1: a name holding %2 stays literal.
	void describeKeepsPercentLiteral()
	{
		QCOMPARE(TerminalNameCheck::describe({qMakePair(QString("%2"), 2)}),
				 QString::fromUtf8("%2 ×2"));
	}
};

QTEST_GUILESS_MAIN(tst_terminalnamecheck)

#include "tst_terminalnamecheck.moc"
