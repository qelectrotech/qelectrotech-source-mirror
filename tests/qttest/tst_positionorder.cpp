/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "positionorder.h"

#include <QtTest>
#include <algorithm>
#include <functional>

/**
	PositionOrder -- the position comparisons behind renumbering elements
	(comparPos) and numbering terminals. std::sort needs a strict weak
	ordering; the old comparisons used "<=" and a 1 px tolerance, which
	are not one, so two elements at the same position (or three terminals
	a fraction apart) were undefined behaviour.
*/
class tst_positionorder : public QObject
{
	Q_OBJECT

	using Less = std::function<bool(const QPointF &, const QPointF &)>;

	static QList<QPair<QString, Less>> orders()
	{
		return {{QStringLiteral("xThenY"), PositionOrder::xThenY},
				{QStringLiteral("yThenX"), PositionOrder::yThenX},
				{QStringLiteral("roundedXThenY"), PositionOrder::roundedXThenY},
				{QStringLiteral("roundedYThenX"), PositionOrder::roundedYThenX}};
	}

private slots:
	// Nothing is before itself, and equal points are before each other
	// in neither direction: what "<=" broke.
	void equalPointsAreNotOrdered()
	{
		const QPointF p(100, 200), q(100, 200);
		for (const auto &order : orders()) {
			QVERIFY2(!order.second(p, p), qPrintable(order.first));
			QVERIFY2(!order.second(p, q), qPrintable(order.first));
			QVERIFY2(!order.second(q, p), qPrintable(order.first));
		}
	}

	void leftToRightThenTopToBottom()
	{
		QVERIFY(PositionOrder::xThenY(QPointF(10, 500), QPointF(20, 0)));
		QVERIFY(!PositionOrder::xThenY(QPointF(20, 0), QPointF(10, 500)));
		QVERIFY(PositionOrder::xThenY(QPointF(10, 0), QPointF(10, 5)));
		QVERIFY(!PositionOrder::xThenY(QPointF(10, 5), QPointF(10, 0)));
	}

	void topToBottomThenLeftToRight()
	{
		QVERIFY(PositionOrder::yThenX(QPointF(500, 10), QPointF(0, 20)));
		QVERIFY(!PositionOrder::yThenX(QPointF(0, 20), QPointF(500, 10)));
		QVERIFY(PositionOrder::yThenX(QPointF(0, 10), QPointF(5, 10)));
		QVERIFY(!PositionOrder::yThenX(QPointF(5, 10), QPointF(0, 10)));
	}

	// Items a fraction of a pixel apart (placed with Ctrl, or after a
	// rotation) count as aligned, and are then ordered on the other axis.
	void roundedTreatsFractionsAsAligned()
	{
		QVERIFY(PositionOrder::roundedXThenY(QPointF(10.2, 0), QPointF(9.8, 5)));
		QVERIFY(!PositionOrder::roundedXThenY(QPointF(9.8, 5), QPointF(10.2, 0)));
		QVERIFY(PositionOrder::roundedYThenX(QPointF(0, 10.2), QPointF(5, 9.8)));
		QVERIFY(!PositionOrder::roundedYThenX(QPointF(5, 9.8), QPointF(0, 10.2)));
	}

	// The three terminals that broke the tolerance comparison: with
	// "within 1 px counts as aligned", a < b (by y), b < c (by y) and
	// c < a (by x), a cycle. Rounded, they are simply ordered by x.
	void roundedIsTransitiveWhereToleranceWasNot()
	{
		const QPointF a(2, 0), b(1.1, 1), c(0.2, 2);
		QVERIFY(PositionOrder::roundedXThenY(c, b));
		QVERIFY(PositionOrder::roundedXThenY(b, a));
		QVERIFY(PositionOrder::roundedXThenY(c, a));
		QVERIFY(!PositionOrder::roundedXThenY(a, c));
	}

	// Every order is a strict weak ordering on a grid of awkward points:
	// irreflexive, asymmetric, transitive, and with transitive
	// equivalence. Checked by brute force on the whole set.
	void strictWeakOrdering_data()
	{
		QTest::addColumn<QString>("name");
		for (const auto &order : orders())
			QTest::newRow(qPrintable(order.first)) << order.first;
	}

	void strictWeakOrdering()
	{
		QFETCH(QString, name);
		Less less;
		for (const auto &order : orders())
			if (order.first == name) less = order.second;
		QVERIFY(less);

		QList<QPointF> points;
		const QList<qreal> values{-1, -0.6, -0.4, 0, 0.2, 0.5, 0.9, 1, 1.1, 2, 10.5, 61.3, 61.7};
		for (qreal x : values)
			for (qreal y : values)
				points << QPointF(x, y);
		points << points.first() << QPointF(2, 0) << QPointF(1.1, 1) << QPointF(0.2, 2);

		auto equiv = [&](const QPointF &p, const QPointF &q) { return !less(p, q) && !less(q, p); };
		for (const QPointF &p : points) {
			QVERIFY(!less(p, p));
			for (const QPointF &q : points) {
				if (less(p, q)) QVERIFY(!less(q, p));
				for (const QPointF &r : points) {
					if (less(p, q) && less(q, r)) QVERIFY2(less(p, r), "transitive");
					if (equiv(p, q) && equiv(q, r)) QVERIFY2(equiv(p, r), "equivalence transitive");
				}
			}
		}

		// And std::sort on it ends sorted, with the list intact.
		QList<QPointF> sorted = points;
		std::sort(sorted.begin(), sorted.end(), less);
		QCOMPARE(sorted.size(), points.size());
		for (int i = 1; i < sorted.size(); ++i)
			QVERIFY(!less(sorted.at(i), sorted.at(i - 1)));
	}

	// Twenty items at one position, as pasting at the origin leaves
	// them: a list std::sort could scan past the end of with "<=".
	void manyEqualPositionsSort()
	{
		QList<QPointF> points(20, QPointF(0, 0));
		points << QPointF(-10, 0) << QPointF(10, 0);
		std::sort(points.begin(), points.end(), PositionOrder::xThenY);
		QCOMPARE(points.size(), 22);
		QCOMPARE(points.first(), QPointF(-10, 0));
		QCOMPARE(points.last(), QPointF(10, 0));
	}
};

QTEST_GUILESS_MAIN(tst_positionorder)
#include "tst_positionorder.moc"
