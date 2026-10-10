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
#include "qetgraphicsitem/diagramimageitem.h"

#include <QGraphicsSceneMouseEvent>

/**
	Escape while a handle of a picture is dragged (#1414): the key goes to
	the real DiagramView, whose own Escape clears the selection. Before
	#1414 that removed the handles mid-drag, so the picture kept the drag
	and no undo step was recorded. Now the picture is back as it was at
	the press, still selected, with no undo step, and the view's own
	Escape still clears the selection afterwards.

	The handles are pressed and dragged with scene mouse events, as the
	view sends them; the key goes through the view.
*/
class tst_imagehandleescape : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;
	DiagramView *m_view = nullptr;
	DiagramImageItem *m_picture = nullptr;

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

	/// The picture's handle nearest to @p scenePos.
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

	/// A second click on the selected picture: rotate/skew handles.
	void switchToRotateMode()
	{
		const QPointF centre = m_picture->mapToScene(m_picture->imageRect().center());
		send(m_picture, QEvent::GraphicsSceneMousePress, centre);
		send(m_picture, QEvent::GraphicsSceneMouseRelease, centre);
	}

	struct State
	{
		QPointF pos;
		qreal scaleX, scaleY, rotation;
		QPointF pivot;
		bool operator==(const State &o) const
		{
			return qFuzzyCompare(1 + QLineF(pos, o.pos).length(), 1.0)
					&& qFuzzyCompare(scaleX, o.scaleX) && qFuzzyCompare(scaleY, o.scaleY)
					&& qFuzzyCompare(1 + rotation, 1 + o.rotation)
					&& qFuzzyCompare(1 + QLineF(pivot, o.pivot).length(), 1.0);
		}
	};
	State state() const
	{
		return {m_picture->pos(), m_picture->scaleFactorX(), m_picture->scaleFactorY(),
				m_picture->rotationAngle(), m_picture->pivot()};
	}

	/// Press @p handle, drag it by @p delta, Escape on the view, release.
	void dragThenEscape(QGraphicsItem *handle, const QPointF &delta)
	{
		QVERIFY(handle);
		const State before = state();
		const int undoIndex = m_diagram->undoStack().index();
		const QPointF from = handle->scenePos(), to = from + delta;
		send(handle, QEvent::GraphicsSceneMousePress, from);
		send(handle, QEvent::GraphicsSceneMouseMove, from + delta / 2);
		send(handle, QEvent::GraphicsSceneMouseMove, to);
		QVERIFY2(!(state() == before), "the drag changed nothing");

		QTest::keyClick(m_view, Qt::Key_Escape);

		QVERIFY2(m_picture->isSelected(), "Escape cleared the selection under the drag");
		QVERIFY2(state() == before, "the picture was not put back");
		// The rest of the gesture: the handle is still under the mouse.
		send(handle, QEvent::GraphicsSceneMouseMove, to + delta);
		send(handle, QEvent::GraphicsSceneMouseRelease, to + delta);
		QVERIFY2(state() == before, "the rest of the gesture moved the picture");
		QCOMPARE(m_diagram->undoStack().index(), undoIndex);
	}

private slots:
	void initTestCase()
	{
		m_view = QetAppTest::view();
		QVERIFY(m_view);
		m_diagram = m_view->diagram();
	}

	void init()
	{
		QPixmap pixmap(100, 60);
		pixmap.fill(Qt::darkGreen);
		m_picture = new DiagramImageItem(pixmap);
		m_diagram->addItem(m_picture);
		m_picture->setPos(300, 300);
		m_picture->setSelected(true);
	}

	void cleanup()
	{
		m_diagram->clearSelection();
		delete m_picture;
		m_picture = nullptr;
		m_diagram->undoStack().clear();
	}

	void resize()
	{
		dragThenEscape(handleAt(m_picture->mapToScene(m_picture->imageRect().bottomRight())),
					   QPointF(40, 30));
	}

	void rotate()
	{
		switchToRotateMode();
		dragThenEscape(handleAt(m_picture->mapToScene(m_picture->imageRect().topRight())),
					   QPointF(0, 60));
	}

	void pivot()
	{
		switchToRotateMode();
		dragThenEscape(handleAt(m_picture->mapToScene(m_picture->pivot())), QPointF(30, 20));
	}

	// Without a drag, Escape is the view's again: it clears the selection.
	void escapeWithoutDragClearsTheSelection()
	{
		QTest::keyClick(m_view, Qt::Key_Escape);
		QVERIFY(!m_picture->isSelected());
	}
};

QET_APP_TEST_MAIN(tst_imagehandleescape)
#include "tst_imagehandleescape.moc"
