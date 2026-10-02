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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "conductorrouter.h"

#include <QtTest>

/**
	The geometry of ConductorRouter on its own: that a route is made of
	horizontal and vertical segments, leaves and enters each terminal in
	the terminal's direction, and goes around obstacles rather than
	through them.
*/
class TestConductorRouter : public QObject
{
	Q_OBJECT

	using Direction = ConductorRouter::Direction;

	static bool crosses(const QList<QPointF> &points, const QRectF &r)
	{
		for (int i = 0; i + 1 < points.size(); ++i) {
			const QPointF a = points.at(i), b = points.at(i + 1);
			const QRectF seg = QRectF(a, b).normalized();
			const qreal left = qMax(seg.left(), r.left()), right = qMin(seg.right(), r.right());
			const qreal top = qMax(seg.top(), r.top()), bottom = qMin(seg.bottom(), r.bottom());
				// a segment is a degenerate rectangle: it crosses the
				// interior when it overlaps it on one axis and lies
				// strictly inside it on the other
			if (seg.width() == 0 && seg.left() > r.left() && seg.left() < r.right() && bottom > top)
				return true;
			if (seg.height() == 0 && seg.top() > r.top() && seg.top() < r.bottom() && right > left)
				return true;
		}
		return false;
	}

	static void checkShape(const QList<QPointF> &p, const ConductorRouter::Request &r)
	{
		QVERIFY(p.size() >= 3);
		QCOMPARE(p.first(), r.start);
		QCOMPARE(p.last(), r.end);
		for (int i = 0; i + 1 < p.size(); ++i)
			QVERIFY2(p.at(i).x() == p.at(i + 1).x() || p.at(i).y() == p.at(i + 1).y(),
					 "a segment is neither horizontal nor vertical");
	}

	static QPointF unit(Direction d)
	{
		switch (d) {
			case Direction::North: return {0, -1};
			case Direction::East:  return {1, 0};
			case Direction::South: return {0, 1};
			case Direction::West:  return {-1, 0};
		}
		return {};
	}

	static QPointF direction(QPointF from, QPointF to)
	{
		const QPointF d = to - from;
		return {d.x() > 0 ? 1. : d.x() < 0 ? -1. : 0., d.y() > 0 ? 1. : d.y() < 0 ? -1. : 0.};
	}

private slots:
	void straightWhenNothingIsInTheWay()
	{
		ConductorRouter::Request r;
		r.start = {100, 100};
		r.start_direction = Direction::East;
		r.end = {300, 100};
		r.end_direction = Direction::West;
		const auto result = ConductorRouter::route(r);
		checkShape(result.points, r);
			// no bend at all: start, the two exit points, end
		for (const QPointF &p : result.points) QCOMPARE(p.y(), 100.);
	}

	void goesAroundASymbol()
	{
		ConductorRouter::Request r;
		r.start = {100, 100};
		r.start_direction = Direction::East;
		r.end = {300, 100};
		r.end_direction = Direction::West;
		const QRectF symbol(170, 60, 60, 80);
		r.obstacles << symbol;
		const auto result = ConductorRouter::route(r);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		checkShape(result.points, r);
		QVERIFY(!crosses(result.points, symbol.adjusted(-r.margin, -r.margin, r.margin, r.margin)));
	}

		// The test that the check above can fail: a straight line through
		// the symbol is reported as crossing it.
	void crossingCheckCanFail()
	{
		QVERIFY(crosses({{100, 100}, {300, 100}}, QRectF(170, 60, 60, 80)));
	}

	void leavesAndEntersInTheTerminalsDirections_data()
	{
		QTest::addColumn<int>("from");
		QTest::addColumn<int>("to");
		for (int a = 0; a < 4; ++a)
			for (int b = 0; b < 4; ++b)
				QTest::addRow("%d-%d", a, b) << a << b;
	}

	void leavesAndEntersInTheTerminalsDirections()
	{
		QFETCH(int, from);
		QFETCH(int, to);
		ConductorRouter::Request r;
		r.start = {100, 100};
		r.start_direction = Direction(from);
		r.end = {250, 180};
		r.end_direction = Direction(to);
			// each terminal's own symbol, on the side opposite the way
			// the terminal points
		const QPointF s = unit(r.start_direction), e = unit(r.end_direction);
		const QRectF own_start(r.start - s * 40 - QPointF(20, 20), QSizeF(40, 40));
		const QRectF own_end(r.end - e * 40 - QPointF(20, 20), QSizeF(40, 40));
		r.obstacles << own_start << own_end;
		const auto result = ConductorRouter::route(r);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		checkShape(result.points, r);
		const auto &p = result.points;
		QCOMPARE(direction(p.at(0), p.at(1)), s);
		QCOMPARE(direction(p.at(p.size() - 1), p.at(p.size() - 2)), e);
		QVERIFY(!crosses(p, own_start));
		QVERIFY(!crosses(p, own_end));
	}

	void prefersNotToRunAlongAnotherWire()
	{
		ConductorRouter::Request r;
		r.start = {100, 100};
		r.start_direction = Direction::East;
		r.end = {300, 100};
		r.end_direction = Direction::West;
			// a wire lying exactly on the straight route
		r.wires << QVector<QPointF>{{110, 100}, {290, 100}};
		const auto result = ConductorRouter::route(r);
		checkShape(result.points, r);
		bool on_it = false;
		for (int i = 1; i + 2 < result.points.size(); ++i)
			if (result.points.at(i).y() == 100 && result.points.at(i + 1).y() == 100)
				on_it = true;
		QVERIFY(!on_it);
	}

	void staysOnTheFolio()
	{
		ConductorRouter::Request r;
		r.start = {100, 30};
		r.start_direction = Direction::East;
		r.end = {300, 30};
		r.end_direction = Direction::West;
		r.obstacles << QRectF(170, 0, 60, 200);
		r.bounds = QRectF(0, 0, 500, 400);
		const auto result = ConductorRouter::route(r);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		for (const QPointF &p : result.points) QVERIFY(r.bounds.contains(p));
	}

	void reportsWhenThereIsNoWay()
	{
		ConductorRouter::Request r;
		r.start = {100, 100};
		r.start_direction = Direction::East;
		r.end = {300, 100};
		r.end_direction = Direction::West;
			// a wall from the top of the folio to the bottom
		r.obstacles << QRectF(170, -10, 60, 520);
		r.bounds = QRectF(0, 0, 500, 500);
		const auto result = ConductorRouter::route(r);
		QVERIFY(result.points.isEmpty());
		QVERIFY(!result.error.isEmpty());
	}

	void terminalsOffTheGrid()
	{
		ConductorRouter::Request r;
		r.start = {103, 97};
		r.start_direction = Direction::South;
		r.end = {287, 213};
		r.end_direction = Direction::North;
		r.obstacles << QRectF(120, 140, 200, 30);
		const auto result = ConductorRouter::route(r);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		checkShape(result.points, r);
	}
};

QTEST_MAIN(TestConductorRouter)
#include "tst_conductorrouter.moc"
