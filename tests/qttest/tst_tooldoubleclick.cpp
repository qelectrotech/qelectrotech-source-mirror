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
	While a drawing tool runs, a double-click it does not use (Qt sends
	one in place of the second of two quick clicks on the same spot)
	must reach the tool as a click and must not open the properties of
	the item under the cursor. With no tool, a double-click on an item
	still opens its properties.

	A dialog that opens is closed by a timer and counted, so the test
	fails instead of waiting on it.
*/
#include "qetapptest.h"

#include "diagramevent/diagrameventaddbridge.h"
#include "diagramevent/diagrameventadddimension.h"
#include "diagramevent/diagrameventfillet.h"
#include "diagramview.h"
#include "qetgraphicsitem/dimensionitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "undocommand/addgraphicsobjectcommand.h"

#include <QDialog>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QUndoStack>
#include <QtTest>

class tst_ToolDoubleClick : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();
		void init();
		void cleanup();
		void linearDimensionQuickClickStartsNext();
		void chainDimensionDoubleClickOpensNothing();
		void filletDoubleClickOnLineOpensNothing();
		void bridgeDoubleClickOnItemOpensNothing();
		void noToolDoubleClickStillEdits();
		void rightDoubleClickKeepsTool();
		void rightDoubleClickEndingToolOpensNothing();
		void titleBlockDoubleClickDuringTool_data();
		void titleBlockDoubleClickDuringTool();

	private:
		Diagram *m_diagram = nullptr;
		int m_baseItems = 0;      // items a new sheet has (its frame)
		QTimer m_closer;
		QStringList m_dialogs;

		void send(QEvent::Type type, const QPointF &pos,
				  Qt::MouseButton button = Qt::LeftButton)
		{
			QGraphicsSceneMouseEvent e(type);
			e.setScenePos(pos);
			e.setButton(button);
			e.setButtons(type == QEvent::GraphicsSceneMouseRelease
						 ? Qt::NoButton : Qt::MouseButtons(button));
			QCoreApplication::sendEvent(m_diagram, &e);
		}
		void click(const QPointF &pos, Qt::MouseButton b = Qt::LeftButton)
		{
			send(QEvent::GraphicsSceneMousePress, pos, b);
			send(QEvent::GraphicsSceneMouseRelease, pos, b);
		}
		/// The events of a real double-click on one spot.
		void doubleClick(const QPointF &pos,
						 Qt::MouseButton b = Qt::LeftButton)
		{
			click(pos, b);
			send(QEvent::GraphicsSceneMouseDoubleClick, pos, b);
			send(QEvent::GraphicsSceneMouseRelease, pos, b);
		}
		/// A quick second click on the spot of the last one.
		void quickRepeat(const QPointF &pos)
		{
			send(QEvent::GraphicsSceneMouseDoubleClick, pos);
			send(QEvent::GraphicsSceneMouseRelease, pos);
		}
		QetShapeItem *line(const QPointF &a, const QPointF &b)
		{
			auto *l = new QetShapeItem(a, b, QetShapeItem::Line);
			m_diagram->undoStack().push(
						new AddGraphicsObjectCommand(l, m_diagram));
			return l;
		}
};

void tst_ToolDoubleClick::initTestCase()
{
	Diagram *d = QetAppTest::diagram();
	QVERIFY(d);
	m_baseItems = d->items().size();
}

void tst_ToolDoubleClick::init()
{
	m_diagram = QetAppTest::diagram();
	QVERIFY(m_diagram);
	QCOMPARE(m_diagram->items().size() - m_baseItems, 0);  // clean sheet
	m_dialogs.clear();
	m_closer.setInterval(20);
	connect(&m_closer, &QTimer::timeout, this, [this] {
		if (auto *d = qobject_cast<QDialog *>(
				QApplication::activeModalWidget())) {
			m_dialogs << d->metaObject()->className();
			d->reject();
		}
	});
	m_closer.start();
}

void tst_ToolDoubleClick::cleanup()
{
	m_closer.stop();
	disconnect(&m_closer, nullptr, this, nullptr);
	if (!m_diagram)
		return;
		//Leave the sheet as found: the tool takes its unfinished item
		//with it, undo takes back everything that was added.
	m_diagram->clearEventInterface();
	while (m_diagram->undoStack().canUndo())
		m_diagram->undoStack().undo();
}

void tst_ToolDoubleClick::linearDimensionQuickClickStartsNext()
{
	m_diagram->setEventInterface(
				new DiagramEventAddDimension(m_diagram, DimensionItem::Linear));
	const int before = m_diagram->undoStack().count();

	click(QPointF(100, 100));
	click(QPointF(200, 100));
	click(QPointF(200, 130));           // offset: first dimension done
	quickRepeat(QPointF(200, 130));     // same spot: start the next one
	click(QPointF(300, 130));
	click(QPointF(300, 160));           // its offset: second done

	QCOMPARE(m_dialogs, QStringList());
	QCOMPARE(m_diagram->undoStack().count() - before, 2);
}

void tst_ToolDoubleClick::chainDimensionDoubleClickOpensNothing()
{
	m_diagram->setEventInterface(
				new DiagramEventAddDimension(m_diagram, DimensionItem::Chain));
	click(QPointF(100, 200));
	click(QPointF(180, 200));
	click(QPointF(180, 230));
	click(QPointF(260, 200));
	doubleClick(QPointF(340, 200));

	QCOMPARE(m_dialogs, QStringList());
}

void tst_ToolDoubleClick::filletDoubleClickOnLineOpensNothing()
{
	line(QPointF(120, 120), QPointF(120, 260));
	line(QPointF(120, 260), QPointF(300, 260));
	m_diagram->setEventInterface(new DiagramEventFillet(m_diagram));
	doubleClick(QPointF(120, 180));

	QCOMPARE(m_dialogs, QStringList());
}

void tst_ToolDoubleClick::bridgeDoubleClickOnItemOpensNothing()
{
	line(QPointF(100, 150), QPointF(300, 150));
	m_diagram->setEventInterface(new DiagramEventAddBridge(m_diagram));
	doubleClick(QPointF(200, 150));

	QCOMPARE(m_dialogs, QStringList());
}

void tst_ToolDoubleClick::noToolDoubleClickStillEdits()
{
	line(QPointF(100, 150), QPointF(300, 150));
	doubleClick(QPointF(200, 150));

	QCOMPARE(m_dialogs.size(), 1);
}

void tst_ToolDoubleClick::rightDoubleClickKeepsTool()
{
	// A right click takes back a half-drawn bridge line; a second one
	// leaves the tool. A right double-click is one right click: the
	// double-click must not be replayed as the second.
	m_diagram->setEventInterface(new DiagramEventAddBridge(m_diagram));
	click(QPointF(100, 150));
	doubleClick(QPointF(140, 150), Qt::RightButton);

	QVERIFY(m_diagram->eventInterfaceIsRunning());
	QCOMPARE(m_dialogs, QStringList());
}

void tst_ToolDoubleClick::rightDoubleClickEndingToolOpensNothing()
{
	// With no line picked, a right click leaves the fillet tool: the
	// double-click that follows is still not the line's to open.
	line(QPointF(100, 150), QPointF(300, 150));
	m_diagram->setEventInterface(new DiagramEventFillet(m_diagram));
	doubleClick(QPointF(200, 150), Qt::RightButton);

	QVERIFY(!m_diagram->eventInterfaceIsRunning());
	QCOMPARE(m_dialogs, QStringList());
}

void tst_ToolDoubleClick::titleBlockDoubleClickDuringTool_data()
{
	QTest::addColumn<bool>("tool");
	QTest::newRow("dimension tool running") << true;
	QTest::newRow("no tool: folio properties") << false;
}

void tst_ToolDoubleClick::titleBlockDoubleClickDuringTool()
{
	// Through the view, as a mouse does: DiagramView opens the folio
	// properties on a double-click over the title block.
	QFETCH(bool, tool);
	DiagramView *view = QetAppTest::view();
	QVERIFY(view);
	const QPointF target =
			m_diagram->border_and_titleblock.titleBlockRect().center();
	view->centerOn(target);
	QCoreApplication::processEvents();
	if (tool)
		m_diagram->setEventInterface(
					new DiagramEventAddDimension(m_diagram));
	QTest::mouseDClick(view->viewport(), Qt::LeftButton, {},
					   view->mapFromScene(target));

	QCOMPARE(m_dialogs.size(), tool ? 0 : 1);
}

QET_APP_TEST_MAIN(tst_ToolDoubleClick)
#include "tst_tooldoubleclick.moc"
