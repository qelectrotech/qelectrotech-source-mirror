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
};

QTEST_APPLESS_MAIN(tst_alignment)
#include "tst_alignment.moc"
