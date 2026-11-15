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
	//Text grid, "Snap the middle of the text": a dragged text puts its
	//middle on the grid instead of the top left corner of its frame
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetproject.h"
#include "textgrid.h"

#include <QMouseEvent>

class tst_textgridmiddle : public QObject
{
	Q_OBJECT

	private:
		DiagramView *m_view = nullptr;
		Diagram *m_diagram = nullptr;
		Element *m_element = nullptr;
		DynamicElementTextItem *m_label = nullptr;
		IndependentTextItem *m_free = nullptr;

		void send(QEvent::Type type, const QPointF &scene, Qt::MouseButton button,
				  Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
		{
			const QPointF pos = m_view->mapFromScene(scene);
			QMouseEvent event(type, pos, m_view->viewport()->mapToGlobal(pos),
							  button, buttons, modifiers);
			QApplication::sendEvent(m_view->viewport(), &event);
		}

			/// Drag @p text alone (Shift) by @p by
		void drag(QGraphicsItem *text, const QPointF &by)
		{
			m_diagram->clearSelection();
			const QPointF from = text->sceneBoundingRect().center();
			const QPointF to = from + by;
			send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
			for (int i = 1; i <= 10; ++i)
				send(QEvent::MouseMove, from + (to - from) * i / 10.0, Qt::NoButton,
					 Qt::LeftButton, Qt::ShiftModifier);
			send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
		}

		static bool onGrid(const QPointF &p)
		{
			auto on = [](qreal v) { return qAbs(v / 10 - qRound(v / 10)) < 1e-6; };
			return on(p.x()) && on(p.y());
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
			int state = 0;
			m_element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/box.elmt"), m_diagram->project()),
				nullptr, &state);
			QVERIFY(m_element && !state);
			m_diagram->addItem(m_element);
			m_element->setPos(200, 200);
			m_label = new DynamicElementTextItem(m_element);
			m_label->setText(QStringLiteral("1"));
			m_element->addDynamicTextItem(m_label);
			m_free = new IndependentTextItem(QStringLiteral("free"));
			m_diagram->addItem(m_free);
			m_view->resize(900, 700);
			m_view->centerOn(300, 250);
			QSettings().setValue(TextGrid::settings_key, 1);
		}

		void cleanupTestCase()
		{
			QSettings().remove(TextGrid::middle_key);
		}

			//Off, the default: the corner of the frame on the grid, as before
		void cornerByDefault()
		{
			QSettings().remove(TextGrid::middle_key);
			m_label->setPos(-17.89, -4.5);
			drag(m_label, QPointF(3, 12));
			QVERIFY(onGrid(m_label->pos()));
			QVERIFY(!onGrid(m_label->sceneBoundingRect().center()));
		}

			//On: the middle of a symbol's text and of a free text
		void middleWhenSet()
		{
			QSettings().setValue(TextGrid::middle_key, true);
			m_label->setPos(-17.89, -4.5);
			drag(m_label, QPointF(3, 12));
			QVERIFY(onGrid(m_label->sceneBoundingRect().center()));
			m_free->setPos(403.3, 301.7);
			drag(m_free, QPointF(-7, 9));
			QVERIFY(onGrid(m_free->sceneBoundingRect().center()));
		}
};

QET_APP_TEST_MAIN(tst_textgridmiddle)
#include "tst_textgridmiddle.moc"
