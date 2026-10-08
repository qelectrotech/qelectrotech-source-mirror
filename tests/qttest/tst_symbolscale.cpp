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
#include <QtTest>

#include "editor/symbolscale.h"

class tst_symbolscale : public QObject
{
	Q_OBJECT

private slots:
		// Terminals 10 px apart: whole factors are safe, halving is not.
	void tenPitchTakesWholeFactors()
	{
		const QList<QPointF> terminals{{0, -20}, {0, 20}, {10, 0}};
		QCOMPARE(SymbolScale::safeFactors(terminals),
				 (QList<qreal>{2.0, 3.0, 4.0}));
	}

		// Terminals 20 px apart: every candidate is safe.
	void twentyPitchTakesEverything()
	{
		const QList<QPointF> terminals{{-20, 0}, {20, 40}};
		QCOMPARE(SymbolScale::safeFactors(terminals), SymbolScale::candidates());
	}

		// Off the grid by 5 px today: only even factors bring it on.
	void fivePxOffIsRescuedByEvenFactors()
	{
		const QList<QPointF> terminals{{-5, 0}, {5, 30}};
		QVERIFY(!SymbolScale::allOnGrid(terminals));
		QCOMPARE(SymbolScale::offGridCount(terminals), 2);
		QCOMPARE(SymbolScale::safeFactors(terminals),
				 (QList<qreal>{2.0, 4.0}));
	}

		// Off by an odd amount: nothing helps.
	void oddOffsetHasNoSafeFactor()
	{
		QVERIFY(SymbolScale::safeFactors({{-7, 0}, {0, 20}}).isEmpty());
	}

	void noTerminalTakesEverything()
	{
		QCOMPARE(SymbolScale::safeFactors({}), SymbolScale::candidates());
	}

		// Saved with two decimals: 19.999 still counts as 20.
	void savedRoundingIsTolerated()
	{
		QVERIFY(SymbolScale::onGrid(19.999));
		QVERIFY(SymbolScale::onGrid(-30.001));
		QVERIFY(!SymbolScale::onGrid(19.9));
	}

	void fontSizeIsWholeAndReadable()
	{
		QCOMPARE(SymbolScale::scaledFontSize(9, 2.0), 18);
		QCOMPARE(SymbolScale::scaledFontSize(9, 1.5), 14);
		QCOMPARE(SymbolScale::scaledFontSize(6, 0.5), 4);
	}
};

QTEST_APPLESS_MAIN(tst_symbolscale)
#include "tst_symbolscale.moc"
