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
	What a selection rectangle selects (a setting): what it touches, as
	before; what lies wholly within it; or by the direction it is dragged
	in, wholly within to the right and touched to the left.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QMouseEvent>

class tst_selectionrectangle : public QObject
{
	Q_OBJECT

	private:
		DiagramView *m_view = nullptr;
		Element *m_element = nullptr;

		void send(QEvent::Type type, const QPointF &scene, Qt::MouseButton button,
				  Qt::MouseButtons buttons)
		{
			const QPointF pos = m_view->mapFromScene(scene);
			QMouseEvent event(type, pos, m_view->viewport()->mapToGlobal(pos),
							  button, buttons, Qt::NoModifier);
			QApplication::sendEvent(m_view->viewport(), &event);
		}

			/// Drag a selection rectangle from @p from to @p to (scene)
			/// and tell whether it selected the symbol
		bool band(const QPointF &from, const QPointF &to)
		{
			m_view->diagram()->clearSelection();
			send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
			for (int i = 1; i <= 10; ++i)
				send(QEvent::MouseMove, from + (to - from) * i / 10.0, Qt::NoButton, Qt::LeftButton);
			send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
			return m_element->isSelected();
		}

			//Partly over the symbol's frame, (190, 190) to (210, 210)
		bool touching(bool rightwards)
		{
			return rightwards ? band(QPointF(180, 205), QPointF(230, 230))
							  : band(QPointF(230, 230), QPointF(180, 205));
		}

		bool enclosing() { return band(QPointF(180, 180), QPointF(230, 230)); }

	private slots:
		void initTestCase()
		{
			m_view = QetAppTest::view();
			QVERIFY(m_view);
			Diagram *diagram = m_view->diagram();
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">box</name></names>
<description><line x1="0" y1="-5" x2="0" y2="5" style="line-style:normal;line-weight:normal;filling:none;color:black"/></description></definition>)"));
			QVERIFY(diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("box"), definition.documentElement()));
			int state = 0;
			m_element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/box.elmt"), diagram->project()),
				nullptr, &state);
			QVERIFY(!state && m_element);
			diagram->addItem(m_element);
			m_element->setPos(200, 200);
			m_view->resize(800, 600);
			m_view->centerOn(200, 200);
		}

			//By default, what the rectangle touches, as before
		void touchedByDefault()
		{
			QCOMPARE(DiagramView::selectionRectangle(), DiagramView::SelectionRectangle::Touched);
			QVERIFY(touching(true));
			QVERIFY(touching(false));
			QVERIFY(enclosing());
		}

		void enclosed()
		{
			DiagramView::setSelectionRectangle(DiagramView::SelectionRectangle::Enclosed);
			QVERIFY(!touching(true));
			QVERIFY(!touching(false));
			QVERIFY(enclosing());
		}

		void byDirection()
		{
			DiagramView::setSelectionRectangle(DiagramView::SelectionRectangle::ByDirection);
			QVERIFY(!touching(true));
			QVERIFY(touching(false));
			QVERIFY(enclosing());
			DiagramView::setSelectionRectangle(DiagramView::SelectionRectangle::Touched);
		}
};

QET_APP_TEST_MAIN(tst_selectionrectangle)
#include "tst_selectionrectangle.moc"
