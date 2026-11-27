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
#include "qetapptest.h"
#include "qetgraphicsitem/qetshapeitem.h"

/**
	Issue #1997: a shape that was only moved saves its folio position in
	x1..y2 (and in the points of a polygon), as QElectroTech 0.100 did,
	so older versions and other programs reading the file place it
	right. Only a rotated or skewed shape writes posX/posY.
*/
class tst_shapesaveposition : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;
	QList<QetShapeItem *> m_shapes;

	QetShapeItem *add(QetShapeItem::ShapeType type, const QPointF &p1, const QPointF &p2)
	{
		auto *shape = new QetShapeItem(p1, p2, type);
		m_diagram->addItem(shape);
		m_shapes << shape;
		return shape;
	}

	/// A new shape, read from @a e, placed at @a pos first.
	QetShapeItem *read(const QDomElement &e, const QPointF &pos = QPointF())
	{
		QetShapeItem *shape = add(QetShapeItem::Rectangle, QPointF(), QPointF(10, 10));
		shape->QGraphicsItem::setPos(pos);
		if (!shape->fromXml(e))
			return nullptr;
		return shape;
	}

	static QPolygonF outline(const QetShapeItem *shape)
	{
		return shape->mapToScene(shape->shape().toFillPolygon());
	}

	static void compareOutlines(const QPolygonF &a, const QPolygonF &b)
	{
		QCOMPARE(a.size(), b.size());
		for (int i = 0; i < a.size(); ++i)
		{
			QVERIFY2(QLineF(a.at(i), b.at(i)).length() < 1e-6,
			         qPrintable(QStringLiteral("point %1 differs").arg(i)));
		}
	}

private slots:
	void initTestCase()
	{
		QVERIFY(QetAppTest::editor());
		m_diagram = QetAppTest::diagram();
		QVERIFY(m_diagram);
	}

	void cleanup()
	{
		qDeleteAll(m_shapes);
		m_shapes.clear();
	}

	void movedRectangleSavesItsFolioPosition()
	{
		QetShapeItem *shape = add(QetShapeItem::Rectangle, QPointF(0, 0), QPointF(100, 50));
		shape->QGraphicsItem::setPos(200, 300);

		QDomDocument document;
		const QDomElement e = shape->toXml(document);
		QCOMPARE(e.attribute("x1"), QStringLiteral("200"));
		QCOMPARE(e.attribute("y1"), QStringLiteral("300"));
		QCOMPARE(e.attribute("x2"), QStringLiteral("300"));
		QCOMPARE(e.attribute("y2"), QStringLiteral("350"));
		QVERIFY(!e.hasAttribute("posX"));
		QVERIFY(!e.hasAttribute("posY"));
		QVERIFY(e.firstChildElement("transform").isNull());

		QetShapeItem *back = read(e);
		QVERIFY(back);
		compareOutlines(outline(back), outline(shape));
	}

	void movedPolygonSavesFolioPoints()
	{
		QetShapeItem *shape = add(QetShapeItem::Polygon, QPointF(0, 0), QPointF(40, 0));
		QVERIFY(shape->setPolygon(QPolygonF({QPointF(0, 0), QPointF(40, 0), QPointF(40, 30)})));
		shape->QGraphicsItem::setPos(100, 100);

		QDomDocument document;
		const QDomElement e = shape->toXml(document);
		QVERIFY(!e.hasAttribute("posX"));
		const QDomElement first = e.firstChildElement("points").firstChildElement("point");
		QCOMPARE(first.attribute("x"), QStringLiteral("100"));
		QCOMPARE(first.attribute("y"), QStringLiteral("100"));

		QetShapeItem *back = read(e);
		QVERIFY(back);
		compareOutlines(outline(back), outline(shape));
	}

	void rotatedRectangleKeepsPosX()
	{
		QetShapeItem *shape = add(QetShapeItem::Rectangle, QPointF(0, 0), QPointF(100, 50));
		shape->QGraphicsItem::setPos(200, 300);
		shape->setRotation(30);

		QDomDocument document;
		const QDomElement e = shape->toXml(document);
		QVERIFY(!e.firstChildElement("transform").isNull());
		QCOMPARE(e.attribute("x1"), QStringLiteral("0"));
		QVERIFY(e.hasAttribute("posX"));

		QetShapeItem *back = read(e);
		QVERIFY(back);
		compareOutlines(outline(back), outline(shape));
	}

	/// The undo state of a shape keeps pos(): a move still on the undo
	/// stack sets pos() back, so restoring a state must not fold it into
	/// the geometry.
	void undoSnapshotKeepsPosition()
	{
		QetShapeItem *shape = add(QetShapeItem::Rectangle, QPointF(0, 0), QPointF(100, 50));
		shape->QGraphicsItem::setPos(200, 300);
		const QPolygonF before = outline(shape);

		const QDomElement snapshot = shape->snapshotXml();
		QCOMPARE(snapshot.attribute("x1"), QStringLiteral("0"));
		shape->fromXml(snapshot);
		QCOMPARE(shape->pos(), QPointF(200, 300));
		compareOutlines(outline(shape), before);

			//the move being undone afterwards: back where it started
		shape->QGraphicsItem::setPos(0, 0);
		QCOMPARE(outline(shape).boundingRect().topLeft(), before.boundingRect().topLeft() - QPointF(200, 300));
	}

	/// fromXml() also restores an item already on the folio (undoing a
	/// rectangle turned into a polygon): its old position must not be
	/// added to the one in the file.
	void readingIntoAPlacedShapeReplacesItsPosition()
	{
		QetShapeItem *shape = add(QetShapeItem::Rectangle, QPointF(0, 0), QPointF(100, 50));
		shape->QGraphicsItem::setPos(200, 300);
		QDomDocument document;
		const QDomElement e = shape->toXml(document);

		QetShapeItem *back = read(e, QPointF(500, 500));
		QVERIFY(back);
		compareOutlines(outline(back), outline(shape));
	}
};

QET_APP_TEST_MAIN(tst_shapesaveposition)
#include "tst_shapesaveposition.moc"
