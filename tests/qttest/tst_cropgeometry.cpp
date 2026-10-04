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
#include "qetgraphicsitem/cropgeometry.h"

#include <QtTest>

/**
	Cropping a picture on the folio: which edges a handle moves, how far
	they may go, and moving the crop window over the original.
*/
class tst_cropgeometry : public QObject
{
	Q_OBJECT

	const QRectF m_bounds{0, 0, 400, 300};   // the original, in its pixels
	const QRectF m_crop{100, 50, 200, 100};

private slots:
	void handlesMoveTheEdgesTheySitOn_data()
	{
		QTest::addColumn<QPointF>("handle");
		QTest::addColumn<bool>("left");
		QTest::addColumn<bool>("top");
		QTest::addColumn<bool>("right");
		QTest::addColumn<bool>("bottom");
		QTest::newRow("top-left corner") << QPointF(0, 0) << true << true << false << false;
		QTest::newRow("top edge") << QPointF(1, 0) << false << true << false << false;
		QTest::newRow("right edge") << QPointF(2, 1) << false << false << true << false;
		QTest::newRow("bottom-right corner") << QPointF(2, 2) << false << false << true << true;
	}

	void handlesMoveTheEdgesTheySitOn()
	{
		QFETCH(QPointF, handle);
		QFETCH(bool, left);
		QFETCH(bool, top);
		QFETCH(bool, right);
		QFETCH(bool, bottom);
		const CropGeometry::Edges e = CropGeometry::edgesForHandle(handle, QSizeF(2, 2));
		QCOMPARE(e.left, left);
		QCOMPARE(e.top, top);
		QCOMPARE(e.right, right);
		QCOMPARE(e.bottom, bottom);
	}

	void aCornerMovesTwoEdges()
	{
		CropGeometry::Edges e;
		e.left = e.top = true;
		QCOMPARE(CropGeometry::dragEdges(m_crop, e, QPointF(120, 60), m_bounds, 4),
				 QRectF(QPointF(120, 60), m_crop.bottomRight()));
	}

	void edgesStayOnTheOriginal()
	{
		CropGeometry::Edges e;
		e.right = e.bottom = true;
		QCOMPARE(CropGeometry::dragEdges(m_crop, e, QPointF(900, 900), m_bounds, 4),
				 QRectF(m_crop.topLeft(), m_bounds.bottomRight()));
		CropGeometry::Edges l;
		l.left = true;
		QCOMPARE(CropGeometry::dragEdges(m_crop, l, QPointF(-50, 0), m_bounds, 4).left(), 0.0);
	}

	// Dragged past the opposite edge, an edge stops a minimum away
	// instead of turning the frame inside out.
	void edgesNeverCross()
	{
		CropGeometry::Edges e;
		e.left = true;
		const QRectF r = CropGeometry::dragEdges(m_crop, e, QPointF(380, 80), m_bounds, 4);
		QCOMPARE(r.left(), m_crop.right() - 4);
		QCOMPARE(r.width(), 4.0);
	}

	void theWindowMovesWithinTheOriginal()
	{
		QCOMPARE(CropGeometry::moveWithin(m_crop, QPointF(30, -20), m_bounds),
				 m_crop.translated(30, -20));
		QCOMPARE(CropGeometry::moveWithin(m_crop, QPointF(500, -500), m_bounds),
				 QRectF(QPointF(200, 0), m_crop.size()));
	}

	void cropsLandOnWholePixels()
	{
		QCOMPARE(CropGeometry::toPixels(QRectF(10.4, 9.6, 20.2, 30.3)), QRect(10, 10, 21, 30));
		QCOMPARE(CropGeometry::toPixels(m_bounds), m_bounds.toRect());
	}
};

QTEST_GUILESS_MAIN(tst_cropgeometry)
#include "tst_cropgeometry.moc"
