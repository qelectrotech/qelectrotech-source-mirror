#include <QtTest>

#include "diagramsortkeys.h"

class tst_diagramsortkeys : public QObject
{
	Q_OBJECT

private slots:
	// positionKey() must sort the same way the underlying coordinates do,
	// including across differing integer-part digit widths and across the
	// negative/positive boundary. A previous implementation formatted
	// coordinates with plain "%.4f" and compared the resulting strings
	// directly, which sorted "15.0000" before "5.0000".
	void sortsLikeNumbers_data()
	{
		QTest::addColumn<QPointF>("smaller");
		QTest::addColumn<QPointF>("larger");

		QTest::newRow("single vs double digit")   << QPointF(5.0, 0.0)   << QPointF(15.0, 0.0);
		QTest::newRow("double vs triple digit")   << QPointF(0.0, 99.0)  << QPointF(0.0, 100.0);
		QTest::newRow("negative vs negative")     << QPointF(-15.0, 0.0) << QPointF(-5.0, 0.0);
		QTest::newRow("negative vs positive")     << QPointF(-1.0, 0.0)  << QPointF(1.0, 0.0);
		QTest::newRow("negative vs zero")         << QPointF(0.0, -0.0001) << QPointF(0.0, 0.0);
		QTest::newRow("fractional precision")     << QPointF(1.0001, 0.0) << QPointF(1.001, 0.0);
	}

	void sortsLikeNumbers()
	{
		QFETCH(QPointF, smaller);
		QFETCH(QPointF, larger);

		QVERIFY(DiagramSortKeys::positionKey(smaller) < DiagramSortKeys::positionKey(larger));
	}
};

QTEST_APPLESS_MAIN(tst_diagramsortkeys)

#include "tst_diagramsortkeys.moc"
