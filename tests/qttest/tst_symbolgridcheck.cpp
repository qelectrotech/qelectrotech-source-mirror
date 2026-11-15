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
/*
	--check-elements --grid: a symbol lines up with the folio grid when its
	insertion point is a connection point and its frame and connection
	points are on the grid of 10.
*/
#include "editor/symbolgridcheck.h"

#include <QtTest>

class tst_symbolgridcheck : public QObject
{
	Q_OBJECT

	private slots:
		void onTheGrid()
		{
			QVERIFY(SymbolGridCheck::problems(10, 0, 20, 30, {QPointF(0, 0), QPointF(0, 20)}).isEmpty());
				//No connection point: only the frame counts
			QVERIFY(SymbolGridCheck::problems(10, 10, 20, 20, {}).isEmpty());
		}

		void insertionPointBesideTheConnectionPoints()
		{
			const QStringList found = SymbolGridCheck::problems(10, 20, 20, 40, {QPointF(0, -10), QPointF(0, 10)});
			QCOMPARE(found, QStringList{QStringLiteral("insertion point on no connection point and off the outline of the frame")});
		}

			//A corner or a T-junction: the origin where its lines meet, on
			//the outline of the frame, the connection points at their ends
		void insertionPointOnTheOutlineOfTheFrame()
		{
			QVERIFY(SymbolGridCheck::problems(0, 10, 10, 10, {QPointF(0, -10), QPointF(10, 0)}).isEmpty());
			QVERIFY(SymbolGridCheck::problems(10, 0, 20, 10,
				{QPointF(-10, 0), QPointF(10, 0), QPointF(0, 10)}).isEmpty());
				//Outside the frame is no part of its outline
			QVERIFY(!SymbolGridCheck::problems(-10, 0, 20, 10, {QPointF(10, 10)}).isEmpty());
		}

		void frameOffTheGrid()
		{
			const QStringList found = SymbolGridCheck::problems(15, 27, 30, 40, {QPointF(0, 0)});
			QCOMPARE(found, QStringList{QStringLiteral("frame off the grid (hotspot 15,27, size 30x40)")});
		}

		void connectionPointsOffTheGrid()
		{
			const QStringList found = SymbolGridCheck::problems(10, 0, 20, 30,
				{QPointF(0, 0), QPointF(5, 20), QPointF(0, 22.5)});
			QCOMPARE(found, QStringList{QStringLiteral("2 of 3 connection points off the grid")});
		}

		void negativeAndNearlyOnTheGrid()
		{
			QVERIFY(SymbolGridCheck::onGrid(-20));
			QVERIFY(SymbolGridCheck::onGrid(30.004));
			QVERIFY(!SymbolGridCheck::onGrid(-25));
		}
};

QTEST_APPLESS_MAIN(tst_symbolgridcheck)
#include "tst_symbolgridcheck.moc"
