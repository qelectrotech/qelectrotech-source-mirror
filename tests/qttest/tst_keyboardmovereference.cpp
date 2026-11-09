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
	Moving the selection with the arrow keys: the crosshair stands on the
	leftmost selected symbol's insertion point, the same from one key press to
	the next, or on the middle of a text; and the keys move by the folio's
	grid step when set so (off by default: the fixed step, as before).
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/terminal.h"
#include "foliogrid.h"
#include "qetproject.h"

#include <QCursor>
#include <QKeyEvent>
#include <QMouseEvent>

class tst_keyboardmovereference : public QObject
{
	Q_OBJECT

	private:
		DiagramView *m_view = nullptr;
		Diagram *m_diagram = nullptr;
		Element *m_left = nullptr, *m_right = nullptr;
		IndependentTextItem *m_text = nullptr;

		Element *place(const QPointF &pos)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/box.elmt"), m_diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(pos);
			return element;
		}

		void key(int key)
		{
			QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
			QApplication::sendEvent(m_view, &press);
			QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
			QApplication::sendEvent(m_view, &release);
		}

		void move(const QPoint &at)
		{
			QMouseEvent event(QEvent::MouseMove, at, m_view->viewport()->mapToGlobal(at),
					  Qt::NoButton, Qt::NoButton, Qt::NoModifier);
			QApplication::sendEvent(m_view->viewport(), &event);
		}

	private slots:
		void initTestCase()
		{
			m_view = QetAppTest::view();
			QVERIFY(m_view);
			m_diagram = m_view->diagram();
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">box</name></names>
<description><rect x="-10" y="-10" width="20" height="20" style="line-style:normal;line-weight:normal;filling:white;color:black"/>
<terminal x="0" y="-10" orientation="n"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("box"), definition.documentElement()));
			QVERIFY(m_left = place(QPointF(200, 200)));
			QVERIFY(m_right = place(QPointF(300, 200)));
			m_text = new IndependentTextItem(QStringLiteral("text"));
			m_diagram->addItem(m_text);
			m_text->setPos(400, 300);
			m_view->resize(900, 700);
			m_view->centerOn(300, 250);
			DiagramView::setCrosshairShown(true);
		}

		void cleanupTestCase()
		{
			DiagramView::setCrosshairShown(false);
		}

			//Two symbols: on the left one's insertion point, whatever was nearer
		void leftmostSymbol()
		{
			m_diagram->clearSelection();
			m_left->setSelected(true);
			m_right->setSelected(true);
			const QPointF before = m_left->terminals().first()->dockConductor();
			key(Qt::Key_Right);
			QCOMPARE(m_left->terminals().first()->dockConductor(), before + QPointF(10, 0));
			QVERIFY(m_view->crosshairVisible());
			QCOMPARE(m_view->crosshairPos(), m_left->scenePos());
			key(Qt::Key_Left);
			QCOMPARE(m_view->crosshairPos(), m_left->scenePos());
			QCOMPARE(m_left->terminals().first()->dockConductor(), before);
		}

			//Pressed near its terminal: the arrow keys keep the crosshair
			//on that terminal; pressed on the body, on the insertion point
		void pressedTerminalIsReference()
		{
			m_diagram->clearSelection();
			const QPoint near_terminal = m_view->mapFromScene(m_left->terminals().first()->dockConductor() + QPointF(5, 3));
			const auto click = [this](const QPoint &at) {
				QMouseEvent press(QEvent::MouseButtonPress, at, m_view->viewport()->mapToGlobal(at),
						  Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
				QApplication::sendEvent(m_view->viewport(), &press);
				QMouseEvent release(QEvent::MouseButtonRelease, at, m_view->viewport()->mapToGlobal(at),
						    Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
				QApplication::sendEvent(m_view->viewport(), &release);
			};
				//At the press already, before the symbol is selected
			{
				QMouseEvent press(QEvent::MouseButtonPress, near_terminal, m_view->viewport()->mapToGlobal(near_terminal),
						  Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
				QApplication::sendEvent(m_view->viewport(), &press);
				QCOMPARE(m_view->crosshairPos(), m_left->terminals().first()->dockConductor());
				QMouseEvent release(QEvent::MouseButtonRelease, near_terminal, m_view->viewport()->mapToGlobal(near_terminal),
						    Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
				QApplication::sendEvent(m_view->viewport(), &release);
			}
			QVERIFY(m_left->isSelected());
			key(Qt::Key_Right);
			QCOMPARE(m_view->crosshairPos(), m_left->terminals().first()->dockConductor());
			key(Qt::Key_Left);

			m_diagram->clearSelection();
			click(m_view->mapFromScene(m_left->scenePos() + QPointF(5, 8)));
			QVERIFY(m_left->isSelected());
			key(Qt::Key_Right);
			QCOMPARE(m_view->crosshairPos(), m_left->scenePos());
			key(Qt::Key_Left);
			m_diagram->clearSelection();
		}

			//A text: its middle, not its top left corner
		void middleOfText()
		{
			m_diagram->clearSelection();
			m_text->setSelected(true);
			key(Qt::Key_Down);
			QCOMPARE(m_view->crosshairPos(), m_text->sceneBoundingRect().center());
			key(Qt::Key_Up);
		}

			//A still mouse leaves it there; a moved mouse takes it back
		void stillMouse()
		{
			m_diagram->clearSelection();
			m_left->setSelected(true);
			const QPoint mouse = m_view->mapFromScene(QPointF(500, 400));
			QCursor::setPos(m_view->viewport()->mapToGlobal(mouse));
			key(Qt::Key_Right);
			const QPointF held = m_view->crosshairPos();
			move(mouse);
			QCOMPARE(m_view->crosshairPos(), held);
			QCursor::setPos(m_view->viewport()->mapToGlobal(mouse + QPoint(30, 0)));
			move(mouse + QPoint(30, 0));
			QVERIFY(m_view->crosshairPos() != held);
			key(Qt::Key_Left);
		}

			//By the grid step when set so
		void byGridStep()
		{
			QSettings settings;
			settings.setValue(QStringLiteral("diagrameditor/key_follows_grid"), true);
			settings.setValue(FolioGrid::x_key, 5);
			m_diagram->clearSelection();
			m_left->setSelected(true);
			const QPointF before = m_left->pos();
			key(Qt::Key_Right);
			QCOMPARE(m_left->pos(), before + QPointF(5, 0));
			settings.setValue(QStringLiteral("diagrameditor/key_follows_grid"), false);
			key(Qt::Key_Right);
			QCOMPARE(m_left->pos(), before + QPointF(15, 0));
			settings.remove(FolioGrid::x_key);
		}
};

QET_APP_TEST_MAIN(tst_keyboardmovereference)
#include "tst_keyboardmovereference.moc"
