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
	//Settings > General: dragging with Ctrl held leaves a copy to put down,
	//and Duplicate can hang the copy on the mouse until a click
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagramevent/diagrameventaddpaste.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QGraphicsSceneMouseEvent>

class tst_ctrldragcopy : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;

		int elementCount() const
		{
			int count = 0;
			for (QGraphicsItem *item : m_diagram->items())
				if (item->type() == Element::Type)
					++count;
			return count;
		}

		void send(QEvent::Type type, const QPointF &pos, const QPointF &down,
				  Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
		{
			QGraphicsSceneMouseEvent event(type);
			event.setScenePos(pos);
			event.setButtonDownScenePos(Qt::LeftButton, down);
			event.setButton(type == QEvent::GraphicsSceneMouseMove ? Qt::NoButton : Qt::LeftButton);
			event.setButtons(buttons);
			event.setModifiers(modifiers);
			QApplication::sendEvent(m_diagram, &event);
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="40" height="40" hotspot_x="20" hotspot_y="20">
<names><name lang="en">box</name></names>
<description><rect x="-10" y="-10" width="20" height="20" style="line-style:normal;line-weight:normal;filling:black;color:black"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("box"), definition.documentElement()));
		}

		void init()
		{
			for (QGraphicsItem *item : m_diagram->items())
				if (item->type() == Element::Type)
					delete item;
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/box.elmt"), m_diagram->project()),
				nullptr, &state);
			QVERIFY(element && !state);
			m_diagram->addItem(element);
			element->setPos(100, 100);
			element->setSelected(true);
		}

		void cleanupTestCase()
		{
			DiagramEventAddPaste::setCtrlDragCopies(false);
			DiagramEventAddPaste::setDuplicateFollowsMouse(false);
		}

			//Off by default: a Ctrl click toggles the selection at once
		void offByDefault()
		{
			QVERIFY(!DiagramEventAddPaste::ctrlDragCopies());
			QVERIFY(!DiagramEventAddPaste::duplicateFollowsMouse());
		}

		void ctrlDragLeavesACopy()
		{
			DiagramEventAddPaste::setCtrlDragCopies(true);
			const QPointF at(100, 100);
			send(QEvent::GraphicsSceneMousePress, at, at, Qt::LeftButton, Qt::ControlModifier);
			send(QEvent::GraphicsSceneMouseMove, at + QPointF(30, 0), at, Qt::LeftButton, Qt::ControlModifier);
			send(QEvent::GraphicsSceneMouseMove, at + QPointF(60, 0), at, Qt::LeftButton, Qt::ControlModifier);
			QCOMPARE(elementCount(), 2);
			send(QEvent::GraphicsSceneMouseRelease, at + QPointF(60, 0), at, Qt::NoButton, Qt::ControlModifier);
			QCOMPARE(elementCount(), 2);
			QList<QPointF> places;
			for (QGraphicsItem *item : m_diagram->items())
				if (item->type() == Element::Type)
					places << item->pos();
			QVERIFY(places.contains(QPointF(100, 100)));
			QVERIFY(places.contains(QPointF(160, 100)));
			m_diagram->undoStack().undo();
			QCOMPARE(elementCount(), 1);
		}

			//A Ctrl click without a drag still takes it out of the selection
		void ctrlClickStillToggles()
		{
			DiagramEventAddPaste::setCtrlDragCopies(true);
			const QPointF at(100, 100);
			send(QEvent::GraphicsSceneMousePress, at, at, Qt::LeftButton, Qt::ControlModifier);
			send(QEvent::GraphicsSceneMouseRelease, at, at, Qt::NoButton, Qt::ControlModifier);
			QCOMPARE(m_diagram->selectedItems().count(), 0);
			QCOMPARE(elementCount(), 1);
		}

			//Duplicate with the copy on the mouse: nothing lands until the click
		void duplicateFollowsTheMouse()
		{
			const QString fragment = m_diagram->selectionFragment();
			m_diagram->setEventInterface(new DiagramEventAddPaste(
				m_diagram, QPointF(300, 300), DiagramEventAddPaste::UnderCursor, fragment));
			QCOMPARE(elementCount(), 2);
			const int steps = m_diagram->undoStack().index();
			send(QEvent::GraphicsSceneMouseMove, QPointF(340, 300), QPointF(), Qt::NoButton, Qt::NoModifier);
			QCOMPARE(m_diagram->undoStack().index(), steps);
			send(QEvent::GraphicsSceneMousePress, QPointF(340, 300), QPointF(340, 300), Qt::LeftButton, Qt::NoModifier);
			send(QEvent::GraphicsSceneMouseRelease, QPointF(340, 300), QPointF(340, 300), Qt::NoButton, Qt::NoModifier);
			QCOMPARE(m_diagram->undoStack().index(), steps + 1);
			QCOMPARE(elementCount(), 2);
		}
};

QET_APP_TEST_MAIN(tst_ctrldragcopy)
#include "tst_ctrldragcopy.moc"
