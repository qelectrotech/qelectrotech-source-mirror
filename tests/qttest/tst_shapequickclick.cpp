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
	A second click on the same spot within the double-click interval
	reaches the line / rectangle / ellipse / arc tools as a double-click,
	not a press. Finishing a shape and quickly clicking its end must
	start the next shape, and the tool must keep the event: passed on,
	it opens the properties of the shape under the cursor.
*/
#include "qetapptest.h"

#include "diagramevent/diagrameventaddshape.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetproject.h"

#include <QGraphicsSceneMouseEvent>
#include <QUndoStack>
#include <QtTest>

class tst_ShapeQuickClick : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();
		void quickClickStartsNextShape_data();
		void quickClickStartsNextShape();
		void doubleClickMidShapeAddsNothing();
		void polygonDoubleClickStillFinishes();

	private:
		static void send(Diagram *d, QEvent::Type type, const QPointF &pos)
		{
			QGraphicsSceneMouseEvent e(type);
			e.setScenePos(pos);
			e.setButton(Qt::LeftButton);
			e.setButtons(type == QEvent::GraphicsSceneMouseRelease
						 ? Qt::NoButton : Qt::LeftButton);
			QCoreApplication::sendEvent(d, &e);
		}
		static void click(Diagram *d, const QPointF &pos)
		{
			send(d, QEvent::GraphicsSceneMousePress, pos);
			send(d, QEvent::GraphicsSceneMouseRelease, pos);
		}
		/*
			The double-click Qt sends instead of the second press, given
			to the tool the way Diagram does. Returns whether the tool
			kept it: if not, Diagram passes it on to the scene.
		*/
		static bool doubleClick(Diagram *d, DiagramEventInterface *tool,
								const QPointF &pos)
		{
			QGraphicsSceneMouseEvent e(
						QEvent::GraphicsSceneMouseDoubleClick);
			e.setScenePos(pos);
			e.setButton(Qt::LeftButton);
			e.setButtons(Qt::LeftButton);
			e.setAccepted(false);
			tool->mouseDoubleClickEvent(&e);
			send(d, QEvent::GraphicsSceneMouseRelease, pos);
			return e.isAccepted();
		}
		static Diagram *sheetWith(QETProject &project,
								  DiagramEventAddShape *&tool,
								  QetShapeItem::ShapeType type,
								  bool halfArc = false)
		{
			Diagram *d = project.addNewDiagram();
			tool = new DiagramEventAddShape(d, type, halfArc);
			d->setEventInterface(tool);
			return d;
		}
		static int shapeCount(Diagram *d)
		{
			int n = 0;
			for (QGraphicsItem *i : d->items())
				n += i->type() == QetShapeItem::Type;
			return n;
		}
};

void tst_ShapeQuickClick::initTestCase()
{
	QETProject::setBackupEnabled(false);
}

void tst_ShapeQuickClick::quickClickStartsNextShape_data()
{
	QTest::addColumn<int>("type");
	QTest::addColumn<bool>("halfArc");
	QTest::newRow("line")      << int(QetShapeItem::Line)      << false;
	QTest::newRow("rectangle") << int(QetShapeItem::Rectangle) << false;
	QTest::newRow("ellipse")   << int(QetShapeItem::Ellipse)   << false;
	QTest::newRow("arc")       << int(QetShapeItem::Ellipse)   << true;
}

void tst_ShapeQuickClick::quickClickStartsNextShape()
{
	QFETCH(int, type);
	QFETCH(bool, halfArc);
	QETProject project;
	DiagramEventAddShape *tool = nullptr;
	Diagram *d = sheetWith(project, tool, QetShapeItem::ShapeType(type),
						   halfArc);
	// a new sheet may start with undo steps of its own
	const int before = d->undoStack().count();

	const QPointF a(100, 100), b(200, 150), c(300, 100);
	click(d, a);
	click(d, b);                         // first shape done
	QVERIFY(doubleClick(d, tool, b));    // same spot, quickly
	click(d, c);                         // second shape done

	QCOMPARE(d->undoStack().count() - before, 2);
	QCOMPARE(shapeCount(d), 2);
}

void tst_ShapeQuickClick::doubleClickMidShapeAddsNothing()
{
	QETProject project;
	DiagramEventAddShape *tool = nullptr;
	Diagram *d = sheetWith(project, tool, QetShapeItem::Rectangle);
	const int before = d->undoStack().count();

	// A deliberate double-click: the press starts a rectangle, the
	// double-click on the same point must not finish it with no size.
	click(d, QPointF(100, 100));
	QVERIFY(doubleClick(d, tool, QPointF(100, 100)));
	QCOMPARE(d->undoStack().count() - before, 0);

	click(d, QPointF(200, 160));          // the rectangle goes on
	QCOMPARE(d->undoStack().count() - before, 1);
}

void tst_ShapeQuickClick::polygonDoubleClickStillFinishes()
{
	QETProject project;
	DiagramEventAddShape *tool = nullptr;
	Diagram *d = sheetWith(project, tool, QetShapeItem::Polygon);
	const int before = d->undoStack().count();

	// A real double-click is press, release, double-click, release.
	click(d, QPointF(100, 100));
	click(d, QPointF(200, 100));
	click(d, QPointF(200, 200));
	QVERIFY(doubleClick(d, tool, QPointF(200, 200)));

	QCOMPARE(d->undoStack().count() - before, 1);
	QCOMPARE(shapeCount(d), 1);
	for (QGraphicsItem *i : d->items())
		if (i->type() == QetShapeItem::Type)
			QCOMPARE(static_cast<QetShapeItem *>(i)->polygon().size(), 3);
}

QET_APP_TEST_MAIN(tst_ShapeQuickClick)
#include "tst_shapequickclick.moc"
