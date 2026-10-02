// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include "editor/elementviewgrid.h"

// The symbol editor's snap step at each zoom (bugtracker #112): below
// 100 % the grid is hidden but parts must still snap every 10 units.
class tst_elementviewgrid : public QObject
{
	Q_OBJECT

private slots:
	void stepAtZoom_data()
	{
		QTest::addColumn<qreal>("zoom");
		QTest::addColumn<int>("step");
		QTest::addColumn<bool>("drawGrid");
		QTest::addColumn<bool>("drawCross");

		QTest::newRow("10 % (fit to a big symbol)") << 0.10 << 10 << false << false;
		QTest::newRow("62.5 %, the #112 case")      << 0.625 << 10 << false << false;
		QTest::newRow("100 %")                      << 1.0  << 10 << true  << false;
		QTest::newRow("200 %")                      << 2.0  << 10 << true  << false;
		QTest::newRow("400 %")                      << 4.0  << 5  << true  << true;
		QTest::newRow("800 %")                      << 8.0  << 2  << true  << true;
		QTest::newRow("1000 %")                     << 10.0 << 1  << true  << true;

		// Just below each threshold: the comparisons are strict (<), so a
		// change to <= would move these rows into the next band.
		QTest::newRow("99.9 %")                     << 0.999 << 10 << false << false;
		QTest::newRow("399.9 %")                    << 3.999 << 10 << true  << false;
		QTest::newRow("401 %")                      << 4.01  << 5  << true  << true;
		QTest::newRow("799.9 %")                    << 7.999 << 5  << true  << true;
		QTest::newRow("999.9 %")                    << 9.999 << 2  << true  << true;
	}

	void stepAtZoom()
	{
		QFETCH(qreal, zoom);
		QFETCH(int, step);
		QFETCH(bool, drawGrid);
		QFETCH(bool, drawCross);

		const ElementViewGrid grid = ElementViewGrid::forZoom(zoom);
		QCOMPARE(grid.step, step);
		QCOMPARE(grid.draw_grid, drawGrid);
		QCOMPARE(grid.draw_cross, drawCross);
	}
};

QTEST_APPLESS_MAIN(tst_elementviewgrid)

#include "tst_elementviewgrid.moc"
