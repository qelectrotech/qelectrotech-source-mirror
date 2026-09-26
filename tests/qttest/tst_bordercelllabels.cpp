#include <QtTest>

#include "bordercelllabels.h"

class tst_bordercelllabels : public QObject
{
	Q_OBJECT

	// A copy of BorderTitleBlock::incrementLetters(), the walk the folio
	// border used to draw its row labels with, as the reference rowLabel()
	// must reproduce.
	static QString incrementLetters(const QString &string)
	{
		if (string.isEmpty())
			return QStringLiteral("A");
		const QString first_digits(string.left(string.length() - 1));
		const QChar last_digit(string.at(string.length() - 1));
		if (last_digit != QLatin1Char('Z'))
			return first_digits + QChar(last_digit.unicode() + 1);
		return incrementLetters(first_digits) + QLatin1Char('A');
	}

private slots:
	void rowLabelsFollowTheBorderSequence()
	{
		QString expected(QStringLiteral("A"));
		for (int row = 1; row <= 1000; ++row) {
			QCOMPARE(BorderCellLabels::rowLabel(row), expected);
			expected = incrementLetters(expected);
		}
	}

	void rowLabelSamples()
	{
		QCOMPARE(BorderCellLabels::rowLabel(1), QStringLiteral("A"));
		QCOMPARE(BorderCellLabels::rowLabel(26), QStringLiteral("Z"));
		QCOMPARE(BorderCellLabels::rowLabel(27), QStringLiteral("AA"));
		QCOMPARE(BorderCellLabels::rowLabel(52), QStringLiteral("AZ"));
		QCOMPARE(BorderCellLabels::rowLabel(53), QStringLiteral("BA"));
		QCOMPARE(BorderCellLabels::rowLabel(702), QStringLiteral("ZZ"));
		QCOMPARE(BorderCellLabels::rowLabel(703), QStringLiteral("AAA"));
	}

	void columnLabels()
	{
		QCOMPARE(BorderCellLabels::columnLabel(1, true), QStringLiteral("0"));
		QCOMPARE(BorderCellLabels::columnLabel(1, false), QStringLiteral("1"));
		QCOMPARE(BorderCellLabels::columnLabel(17, true), QStringLiteral("16"));
		QCOMPARE(BorderCellLabels::columnLabel(17, false), QStringLiteral("17"));
	}
};

QTEST_APPLESS_MAIN(tst_bordercelllabels)

#include "tst_bordercelllabels.moc"
