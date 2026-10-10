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
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "qetgraphicsitem/qetshapeitem.h"

#include <QGraphicsSceneMouseEvent>

/**
	A shape's pivot follows its centre when it is resized, unless it was
	placed by hand. Rectangles and ellipses did; a line did not: dragging
	one of its ends moved the end and left the pivot where it was, so the
	next rotation turned the line around a point beside it.

	The handles are pressed and dragged with scene mouse events, as the
	view sends them.
*/
class tst_linepivotresize : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;
	QetShapeItem *m_shape = nullptr;

	void send(QGraphicsItem *item, QEvent::Type type, const QPointF &scenePos)
	{
		QGraphicsSceneMouseEvent event(type);
		event.setButton(Qt::LeftButton);
		event.setButtons(type == QEvent::GraphicsSceneMouseRelease ? Qt::NoButton : Qt::LeftButton);
		event.setScenePos(scenePos);
		event.setPos(item->mapFromScene(scenePos));
		event.setButtonDownScenePos(Qt::LeftButton, scenePos);
		m_diagram->sendEvent(item, &event);
	}

	QGraphicsItem *handleAt(const QPointF &scenePos)
	{
		QGraphicsItem *best = nullptr;
		qreal bestDistance = 0;
		for (QGraphicsItem *item : m_diagram->items())
		{
			if (item->type() != QetGraphicsHandlerItem::Type)
				continue;
			const qreal d = QLineF(item->scenePos(), scenePos).length();
			if (!best || d < bestDistance)
			{
				best = item;
				bestDistance = d;
			}
		}
		return best;
	}

	void drag(QGraphicsItem *handle, const QPointF &delta)
	{
		QVERIFY(handle);
		const QPointF from = handle->scenePos();
		send(handle, QEvent::GraphicsSceneMousePress, from);
		send(handle, QEvent::GraphicsSceneMouseMove, from + delta / 2);
		send(handle, QEvent::GraphicsSceneMouseMove, from + delta);
		send(handle, QEvent::GraphicsSceneMouseRelease, from + delta);
	}

	/// A click on the selected shape: the next handle mode.
	void click(const QPointF &local)
	{
		const QPointF at = m_shape->mapToScene(local);
		send(m_shape, QEvent::GraphicsSceneMousePress, at);
		send(m_shape, QEvent::GraphicsSceneMouseRelease, at);
	}

	QetShapeItem *add(QetShapeItem::ShapeType type, const QPointF &p1, const QPointF &p2)
	{
		auto *shape = new QetShapeItem(p1, p2, type);
		m_diagram->addItem(shape);
		shape->setPos(200, 200);
		shape->setSelected(true);
		return shape;
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
		m_diagram->clearSelection();
		delete m_shape;
		m_shape = nullptr;
		m_diagram->undoStack().clear();
	}

	void lineEndDragKeepsThePivotAtTheCentre()
	{
		m_shape = add(QetShapeItem::Line, QPointF(0, 0), QPointF(100, 0));
		QCOMPARE(m_shape->pivot(), QPointF(50, 0));

		drag(handleAt(m_shape->mapToScene(QPointF(100, 0))), QPointF(40, 30));

		QCOMPARE(m_shape->line().p2(), QPointF(140, 30));
		QCOMPARE(m_shape->pivot(), m_shape->line().center());
		QVERIFY(!m_shape->pivotIsCustom());
	}

	void undoAndRedoKeepItAtTheCentre()
	{
		m_shape = add(QetShapeItem::Line, QPointF(0, 0), QPointF(100, 0));
		drag(handleAt(m_shape->mapToScene(QPointF(100, 0))), QPointF(40, 30));
		for (int round = 0; round < 2; ++round)
		{
			m_diagram->undoStack().undo();
			QCOMPARE(m_shape->line().p2(), QPointF(100, 0));
			QCOMPARE(m_shape->pivot(), QPointF(50, 0));
			m_diagram->undoStack().redo();
			QCOMPARE(m_shape->pivot(), m_shape->line().center());
		}
	}

	void handPlacedPivotStays()
	{
		m_shape = add(QetShapeItem::Line, QPointF(0, 0), QPointF(100, 0));
		click(QPointF(25, 0));   // rotate/skew mode
		drag(handleAt(m_shape->mapToScene(m_shape->pivot())), QPointF(20, 10));
		QVERIFY(m_shape->pivotIsCustom());
		const QPointF pivot = m_shape->pivot();
		click(QPointF(25, 0));   // back to resize mode

		drag(handleAt(m_shape->mapToScene(QPointF(100, 0))), QPointF(40, 30));

		QCOMPARE(m_shape->line().p2(), QPointF(140, 30));
		QCOMPARE(m_shape->pivot(), pivot);
	}

	// What a line now does, as a rectangle always did.
	void rectangleResizeKeepsThePivotAtTheCentre()
	{
		m_shape = add(QetShapeItem::Rectangle, QPointF(0, 0), QPointF(100, 60));
		drag(handleAt(m_shape->mapToScene(QPointF(100, 60))), QPointF(40, 30));
		QCOMPARE(m_shape->pivot(), QRectF(QPointF(0, 0), QPointF(140, 90)).center());
	}
};

QET_APP_TEST_MAIN(tst_linepivotresize)
#include "tst_linepivotresize.moc"
