#include <QtTest>

#include "alignment.h"

class tst_alignment : public QObject
{
	Q_OBJECT

private slots:
	void gridOffset_data()
	{
		QTest::addColumn<QPointF>("pos");
		QTest::addColumn<int>("x_grid");
		QTest::addColumn<int>("y_grid");
		QTest::addColumn<qreal>("divisor");
		QTest::addColumn<QPointF>("expected");

		QTest::newRow("on the grid")       << QPointF(100, 50) << 10 << 10 << 1.0 << QPointF(0, 0);
		QTest::newRow("5 px right, fine nudge") << QPointF(105, 50) << 10 << 10 << 1.0 << QPointF(5, 0);
		QTest::newRow("either side")       << QPointF(103, 97) << 10 << 10 << 1.0 << QPointF(-3, 3);
		QTest::newRow("negative, fraction") << QPointF(-13.3, 13.7) << 10 << 10 << 1.0 << QPointF(3.3, -3.7);
		QTest::newRow("uneven grid")       << QPointF(12, 13) << 10 << 5 << 1.0 << QPointF(-2, 2);
		QTest::newRow("text grid 1:2")     << QPointF(103, 97) << 10 << 10 << 2.0 << QPointF(2, -2);
		QTest::newRow("text grid off")     << QPointF(103.4, 96.6) << 10 << 10 << 0.0 << QPointF(-0.4, 0.4);
	}

	void gridOffset()
	{
		QFETCH(QPointF, pos);
		QFETCH(int, x_grid);
		QFETCH(int, y_grid);
		QFETCH(qreal, divisor);
		QFETCH(QPointF, expected);

		const QPointF offset = Alignment::gridOffset(pos, x_grid, y_grid, divisor);
		QVERIFY2(qAbs(offset.x() - expected.x()) < 1e-9, qPrintable(QString::number(offset.x())));
		QVERIFY2(qAbs(offset.y() - expected.y()) < 1e-9, qPrintable(QString::number(offset.y())));
		QVERIFY(qAbs(pos.x() + offset.x() - TextGrid::snap(pos, x_grid, y_grid, divisor).x()) < 1e-9);
	}

	// An item already on the grid must give an exactly null offset, not a
	// rounding residue: the command pushes no undo step only when every
	// offset is null.
	void onGridIsExactlyNull()
	{
		for (int k = -50; k <= 50; ++k) {
			const QPointF on_grid(k * 10, -k * 10);
			QVERIFY(Alignment::gridOffset(on_grid, 10, 10).isNull());
			QVERIFY(Alignment::gridOffset(on_grid + QPointF(1e-10, -1e-10), 10, 10).isNull());
		}
	}

	// Three items: a 40x20 symbol with its origin 10 px in from its left
	// edge, a 20x40 one with its origin at its centre, and a 60x10 picture
	// whose reference point is its centre.
	static QList<Alignment::Item> threeItems()
	{
		return {
			{QRectF(100, 200, 40, 20), QPointF(110, 210)},
			{QRectF(150, 100, 20, 40), QPointF(160, 120)},
			{QRectF( 70, 300, 60, 10), QPointF(100, 305)}
		};
	}

	void alignOffsets_data()
	{
		QTest::addColumn<int>("edge");
		QTest::addColumn<QList<QPointF>>("expected");

		QTest::newRow("left, on the left-most edge")
			<< int(Alignment::Left)    << QList<QPointF>{{-30, 0}, {-80, 0}, {0, 0}};
		QTest::newRow("centre, on the mean origin x (123.33)")
			<< int(Alignment::HCenter) << QList<QPointF>{{370.0/3 - 110, 0}, {370.0/3 - 160, 0}, {370.0/3 - 100, 0}};
		QTest::newRow("right, on the right-most edge")
			<< int(Alignment::Right)   << QList<QPointF>{{30, 0}, {0, 0}, {40, 0}};
		QTest::newRow("top, on the top-most edge")
			<< int(Alignment::Top)     << QList<QPointF>{{0, -100}, {0, 0}, {0, -200}};
		QTest::newRow("middle, on the mean origin y (211.67)")
			<< int(Alignment::VCenter) << QList<QPointF>{{0, 635.0/3 - 210}, {0, 635.0/3 - 120}, {0, 635.0/3 - 305}};
		QTest::newRow("bottom, on the bottom-most edge")
			<< int(Alignment::Bottom)  << QList<QPointF>{{0, 90}, {0, 170}, {0, 0}};
	}

	void alignOffsets()
	{
		QFETCH(int, edge);
		QFETCH(QList<QPointF>, expected);

		const QList<QPointF> offsets = Alignment::alignOffsets(threeItems(), Alignment::Edge(edge));
		QCOMPARE(offsets.size(), expected.size());
		for (int i = 0 ; i < offsets.size() ; ++i) {
			QVERIFY2(qAbs(offsets.at(i).x() - expected.at(i).x()) < 1e-9, qPrintable(QString("item %1 x %2").arg(i).arg(offsets.at(i).x())));
			QVERIFY2(qAbs(offsets.at(i).y() - expected.at(i).y()) < 1e-9, qPrintable(QString("item %1 y %2").arg(i).arg(offsets.at(i).y())));
		}
	}

	// Aligning items that are already lined up, and on the grid, moves
	// nothing: the command then pushes no undo step.
	void alreadyAligned()
	{
		const QList<Alignment::Item> items {
			{QRectF(100, 200, 40, 20), QPointF(110, 210)},
			{QRectF(100, 300, 40, 20), QPointF(110, 310)}
		};
		for (int e = Alignment::Left ; e <= Alignment::Bottom ; ++e)
		{
			const auto edge = Alignment::Edge(e);
			if (edge == Alignment::Top || edge == Alignment::VCenter || edge == Alignment::Bottom)
				continue; // they sit one above the other, so these do move
			const QList<QPointF> offsets = Alignment::alignOffsets(items, edge);
			for (int i = 0 ; i < items.size() ; ++i)
				QVERIFY(Alignment::snappedOffset(items.at(i).ref, offsets.at(i), edge, 10, 10).isNull());
		}
	}

	// The result lands on the grid along the aligned line, and the other
	// coordinate is left alone even when it is off the grid.
	void snapsOnlyAlongTheLine()
	{
		// raw offset puts x at 123.33: rounded to 120; y 207 stays 207
		const QPointF pos(110, 207);
		const QPointF offset = Alignment::snappedOffset(pos, QPointF(370.0/3 - 110, 0), Alignment::HCenter, 10, 10);
		QVERIFY(qAbs(offset.x() - 10) < 1e-9);
		QCOMPARE(offset.y(), 0.0);

		const QPointF v = Alignment::snappedOffset(QPointF(113, 207), QPointF(0, 7), Alignment::Bottom, 10, 10);
		QCOMPARE(v.x(), 0.0);
		QVERIFY(qAbs(v.y() - 3) < 1e-9); // 207 + 7 = 214, rounded to 210
	}

	// Two symbols on the grid whose left edges are 5 px apart relative to
	// their origins cannot both have their origin on the grid and their
	// left edges on one line. The origins win: nothing leaves the grid.
	void mixedWidthsStayOnTheGrid()
	{
		const QList<Alignment::Item> items {
			{QRectF(95, 200, 30, 20), QPointF(110, 210)},  // origin 15 px in
			{QRectF(200, 300, 20, 20), QPointF(210, 310)}  // origin 10 px in
		};
		const QList<QPointF> offsets = Alignment::alignOffsets(items, Alignment::Left);
		for (int i = 0 ; i < items.size() ; ++i)
		{
			const QPointF moved = items.at(i).ref + Alignment::snappedOffset(items.at(i).ref, offsets.at(i), Alignment::Left, 10, 10);
			QVERIFY(Alignment::gridOffset(moved, 10, 10).isNull());
		}
		// the second one moves 105 px left to 105, a tie, rounded up to 110
		QVERIFY(qAbs(Alignment::snappedOffset(items.at(1).ref, offsets.at(1), Alignment::Left, 10, 10).x() + 100) < 1e-9);
	}

	// Free texts go to the text grid, which can be finer than the folio's.
	void textGridDivisor()
	{
		const QPointF offset = Alignment::snappedOffset(QPointF(103, 50), QPointF(0, 0), Alignment::Left, 10, 10, 2.0);
		QVERIFY(qAbs(offset.x() - 2) < 1e-9); // 103 -> 105 on a 5 px text grid
		QCOMPARE(offset.y(), 0.0);
	}

	// A group lines up on the middle of its members' box; a unit with a
	// single member, such as a shape whose group-mate is locked, keeps
	// that member's own centre instead of the origin of the folio.
	void combinedUnits()
	{
		const Alignment::Item a{QRectF(100, 200, 40, 20), QPointF(110, 210)};
		const Alignment::Item b{QRectF(300, 260, 20, 60), QPointF(310, 270)};

		const Alignment::Item one = Alignment::combined({a});
		QCOMPARE(one.edges, a.edges);
		QCOMPARE(one.ref, a.ref);

		const Alignment::Item both = Alignment::combined({a, b});
		QCOMPARE(both.edges, QRectF(100, 200, 220, 120));
		QCOMPARE(both.ref, QPointF(210, 260));

		// a lone shape centred on x = 200 and a symbol at x = 400 meet
		// half way, at 300; with the folio origin as the shape's centre
		// they would meet at 200 and the shape would move 200 px
		const QRectF shape(180, 50, 40, 40);
		const QList<QPointF> offsets = Alignment::alignOffsets(
			{Alignment::combined({{shape, shape.center()}}), {QRectF(390, 0, 20, 20), QPointF(400, 10)}},
			Alignment::HCenter);
		QCOMPARE(offsets.at(0), QPointF(100, 0));
		QCOMPARE(offsets.at(1), QPointF(-100, 0));
	}

	void emptySelection()
	{
		QVERIFY(Alignment::alignOffsets({}, Alignment::Left).isEmpty());
	}
};

QTEST_APPLESS_MAIN(tst_alignment)
#include "tst_alignment.moc"
