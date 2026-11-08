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
	//Automatic connections lay the wires: they cannot be selected, and
	//while they are on, the user draws no wires of their own
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>

class tst_autoconnecthand : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;

		Element *place(const QString &name, const QPointF &pos)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name), m_diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(pos);
			return element;
		}

			//Press on @a terminal: whether the dashed line of a new wire
			//shows
		bool startsWire(Terminal *terminal)
		{
			QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
			press.setButton(Qt::LeftButton);
			press.setButtons(Qt::LeftButton);
			press.setScenePos(terminal->dockConductor());
			m_diagram->sendEvent(terminal, &press);
			bool shown = false;
			for (QGraphicsItem *item : m_diagram->items())
				if (item->type() == QGraphicsLineItem::Type)
					shown = true;
			m_diagram->setConductor(false);
			return shown;
		}

		static Terminal *named(Element *element, const QString &name)
		{
			for (Terminal *t : element->terminals())
				if (t->name() == name)
					return t;
			return nullptr;
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			const QList<QPair<QString, QString>> definitions {
					{QStringLiteral("corner"), QStringLiteral(
					R"(<definition type="element" link_type="terminal" width="10" height="10" hotspot_x="0" hotspot_y="10">
<names><name lang="en">corner</name></names><kindInformations><kindInformation name="corner">true</kindInformation></kindInformations>
<description><polygon x1="0" y1="-10" x2="0" y2="0" x3="10" y3="0" closed="false" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-10" orientation="n" name="1"/><terminal x="10" y="0" orientation="e" name="2"/></description></definition>)")},
				{QStringLiteral("target"), QStringLiteral(
					R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="0" hotspot_y="10">
<names><name lang="en">target</name></names><description>
<rect x="0" y="-10" width="20" height="20" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="0" orientation="w" name="w"/></description></definition>)")}};
			for (const auto &d : definitions) {
				QDomDocument doc;
				doc.setContent(d.second);
				QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
					QStringLiteral("import"), d.first, doc.documentElement()));
			}
		}

			//A selection rectangle across an automatic wire catches the
			//symbols at its ends, not the wire
		void automaticWireNotSelected()
		{
			Element *corner = place(QStringLiteral("corner"), QPointF(100, 500));
			Element *target = place(QStringLiteral("target"), QPointF(200, 500));
			QVERIFY(corner && target);
			auto *wire = new Conductor(named(corner, "2"), named(target, "w"));
			m_diagram->addItem(wire);
			wire->setSelected(true);
			wire->setAutomatic(true);
			QVERIFY(!wire->isSelected());

			QPainterPath band;
			band.addRect(QRectF(90, 480, 140, 40));
			m_diagram->setSelectionArea(band, Qt::ReplaceSelection, Qt::IntersectsItemShape);
			QVERIFY(corner->isSelected());
			QVERIFY(target->isSelected());
			QVERIFY(!wire->isSelected());

			wire->setAutomatic(false);
			m_diagram->setSelectionArea(band, Qt::ReplaceSelection, Qt::IntersectsItemShape);
			QVERIFY(wire->isSelected());
			m_diagram->clearSelection();
		}

			//With live automatic connections, a wire of one's own cannot be
			//started, on a corner or on any other symbol
		void noHandWire()
		{
			Element *corner = place(QStringLiteral("corner"), QPointF(100, 700));
			Element *target = place(QStringLiteral("target"), QPointF(200, 700));
			QVERIFY(corner && target);
			m_diagram->project()->setLiveAutoConnect(true);
			QVERIFY(!startsWire(named(corner, "2")));
			QVERIFY(!startsWire(named(target, "w")));
			m_diagram->project()->setLiveAutoConnect(false);
			QVERIFY(startsWire(named(corner, "2")));
			QVERIFY(startsWire(named(target, "w")));
		}
};

QET_APP_TEST_MAIN(tst_autoconnecthand)
#include "tst_autoconnecthand.moc"
