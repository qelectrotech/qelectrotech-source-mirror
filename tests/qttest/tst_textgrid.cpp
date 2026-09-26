#include <QtTest>

#include "textgrid.h"

class tst_textgrid : public QObject
{
	Q_OBJECT

private slots:
	// A label starting off the grid at (-13.3, 13.7), the case from
	// discussion #1020: the sideways jump shrinks with a finer text grid.
	void snapsToStep_data()
	{
		QTest::addColumn<int>("grid");
		QTest::addColumn<qreal>("divisor");
		QTest::addColumn<QPointF>("expected");

		QTest::newRow("off rounds to pixel") << 10 << 0.0  << QPointF(-13, 14);
		QTest::newRow("1:1 is the folio grid") << 10 << 1.0 << QPointF(-10, 10);
		QTest::newRow("1:2")                 << 10 << 2.0  << QPointF(-15, 15);
		QTest::newRow("1:5")                 << 10 << 5.0  << QPointF(-14, 14);
		QTest::newRow("1:10")                << 10 << 10.0 << QPointF(-13, 14);
		QTest::newRow("grid 7, 1:2 steps 3.5") << 7 << 2.0 << QPointF(-14, 14);
	}

	void snapsToStep()
	{
		QFETCH(int, grid);
		QFETCH(qreal, divisor);
		QFETCH(QPointF, expected);

		const QPointF snapped = TextGrid::snap(QPointF(-13.3, 13.7), grid, grid, divisor);
		QCOMPARE(snapped.x(), expected.x());
		QCOMPARE(snapped.y(), expected.y());
	}

	// Every folio grid point is also a text grid point, so a text can
	// always sit exactly where an element or a wire does, and texts of
	// different elements can line up. This is why every divisor offered
	// is a whole number: 1:2.5 on a grid of 10 steps by 4, which misses 10.
	void folioGridPointsAreKept_data()
	{
		QTest::addColumn<int>("grid");
		QTest::addColumn<qreal>("divisor");

		for (int grid : {10, 7, 5})
			for (qreal divisor : TextGrid::divisors)
				if (divisor > 0)
					QTest::newRow(qPrintable(QStringLiteral("grid %1, %2")
											 .arg(grid).arg(TextGrid::ratioLabel(divisor))))
						<< grid << divisor;
	}

	void folioGridPointsAreKept()
	{
		QFETCH(int, grid);
		QFETCH(qreal, divisor);

		for (int k = -20; k <= 20; ++k) {
			const QPointF on_grid(k * grid, -k * grid);
			QCOMPARE(TextGrid::snap(on_grid, grid, grid, divisor), on_grid);
		}
	}

	// Separate X and Y grid sizes are honoured independently.
	void usesEachAxisGrid()
	{
		QCOMPARE(TextGrid::snap(QPointF(13, 13), 10, 20, 2), QPointF(15, 10));
	}

	void ratioLabel()
	{
		QCOMPARE(TextGrid::ratioLabel(2), QStringLiteral("1:2"));
		QCOMPARE(TextGrid::ratioLabel(10), QStringLiteral("1:10"));
	}
};

QTEST_GUILESS_MAIN(tst_textgrid)

#include "tst_textgrid.moc"
