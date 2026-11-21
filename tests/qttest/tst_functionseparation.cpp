// SPDX-License-Identifier: GPL-2.0-or-later
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
	// A terminal element with its function separation enabled keeps its
	// two sides apart: a new wire on one side takes from the other only
	// what the separation lets through -- the voltage / protocol and the
	// drawing of the wire, as if the terminal had no function separation
	// -- and a new number of its own.
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "conductorautonumerotation.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "utils/conductorcreator.h"

#include <QGraphicsSceneMouseEvent>

class tst_functionseparation : public QObject
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

		static Terminal *named(Element *element, const QString &name)
		{
			for (Terminal *t : element->terminals())
				if (t->name() == name)
					return t;
			return nullptr;
		}

			// Tick the function separation checkbox of @a element
		static void separate(Element *element)
		{
			DiagramContext info = element->elementInformations();
			info.addValue(QStringLiteral("potential_isolating"), true);
			element->setElementInformations(info);
		}

			// Draw a wire the way the user does: press on @a from,
			// release over @a to
		void drawWire(Terminal *from, Terminal *to)
		{
			QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
			press.setButton(Qt::LeftButton);
			press.setButtons(Qt::LeftButton);
			press.setScenePos(from->dockConductor());
			m_diagram->sendEvent(from, &press);

			QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
			release.setButton(Qt::LeftButton);
			release.setScenePos(to->dockConductor());
			m_diagram->sendEvent(from, &release);
		}

			// The wire on terminal @a t
		static Conductor *wire(Terminal *t)
		{
			return t->conductors().isEmpty() ? nullptr : t->conductors().first();
		}

			// A terminal block @a block with one wire on each side: the
			// left one says what the node says, the right side is wired
			// with a new wire. Returns the new wire.
		Conductor *wireTheFarSide(Element *block, Element *left,
					  Element *right, bool separated)
		{
			if (separated) {
				separate(block);
			}
			drawWire(named(left, QStringLiteral("w")),
				 named(block, QStringLiteral("1")));
			Conductor *far = wire(named(block, QStringLiteral("1")));
			if (!far) {
				return nullptr;
			}
			ConductorProperties p = far->properties();
			p.m_tension_protocol = QStringLiteral("24 V DC");
			p.color = QColor(255, 0, 0);
			p.style = Qt::DashLine;
			p.cond_size = 3;
			p.m_dash_pattern.enabled = true;
			p.m_dash_pattern.dash = 12;
			p.text = QStringLiteral("101");
			p.m_function = QStringLiteral("PWR");
			far->setProperties(p);

			drawWire(named(block, QStringLiteral("2")),
				 named(right, QStringLiteral("w")));
			return wire(named(block, QStringLiteral("2")));
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			const QList<QPair<QString, QString>> definitions {
					//A terminal block: two terminals facing outwards
					//from one line, one potential per side when the
					//function separation is on
				{QStringLiteral("block"), QStringLiteral(
					R"(<definition type="element" link_type="terminal" width="20" height="10" hotspot_x="0" hotspot_y="10">
<names><name lang="en">block</name></names>
<description><polygon x1="0" y1="0" x2="20" y2="0" closed="false" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="0" orientation="w" name="1"/><terminal x="20" y="0" orientation="e" name="2"/></description></definition>)")},
				{QStringLiteral("load"), QStringLiteral(
					R"(<definition type="element" link_type="simple" width="10" height="10" hotspot_x="0" hotspot_y="10">
<names><name lang="en">load</name></names>
<description><rect x="0" y="-10" width="10" height="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="0" orientation="w" name="w"/></description></definition>)")}};
			for (const auto &d : definitions) {
				QDomDocument doc;
				doc.setContent(d.second);
				QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
					QStringLiteral("import"), d.first, doc.documentElement()));
			}
		}

			// The case the fix is for: a new wire on the free side of a
			// function separated terminal takes the voltage / protocol and
			// the drawing from the other side, and nothing else.
		void newWireTakesNodePropertiesAcrossSeparation()
		{
			Element *block = place(QStringLiteral("block"), {100, 300});
			Element *left = place(QStringLiteral("load"), {40, 300});
			Element *right = place(QStringLiteral("load"), {200, 300});
			QVERIFY(block && left && right);

			Conductor *fresh = wireTheFarSide(block, left, right, true);
			QVERIFY(fresh);
			QCOMPARE(fresh->properties().m_tension_protocol,
				 QStringLiteral("24 V DC"));
			QCOMPARE(fresh->properties().color, QColor(255, 0, 0));
			QCOMPARE(static_cast<int>(fresh->properties().style),
				 static_cast<int>(Qt::DashLine));
			QCOMPARE(fresh->properties().cond_size, 3.0);
			QVERIFY(fresh->properties().m_dash_pattern.enabled);
			QCOMPARE(fresh->properties().m_dash_pattern.dash, 12.0);

				//The text, the number and the function describe one
				//side of the separation and stay new
			QVERIFY(fresh->properties().text != QStringLiteral("101"));
			QVERIFY(fresh->properties().m_function.isEmpty());
		}

			// Without the function separation the same drawing takes
			// everything from the other side, as it always did.
		void newWireWithoutSeparationTakesEverything()
		{
			Element *block = place(QStringLiteral("block"), {100, 500});
			Element *left = place(QStringLiteral("load"), {40, 500});
			Element *right = place(QStringLiteral("load"), {200, 500});
			QVERIFY(block && left && right);

			Conductor *fresh = wireTheFarSide(block, left, right, false);
			QVERIFY(fresh);
			QCOMPARE(fresh->properties().text, QStringLiteral("101"));
			QCOMPARE(fresh->properties().m_function,
				 QStringLiteral("PWR"));
			QCOMPARE(fresh->properties().m_tension_protocol,
				 QStringLiteral("24 V DC"));
			QCOMPARE(fresh->properties().color, QColor(255, 0, 0));
		}

			// What the wire's own side of the separation says wins over
			// the other side: a new wire joining a wired side takes that
			// side's properties whole, not the far side's.
		void ownSideOfTheSeparationWins()
		{
			Element *block = place(QStringLiteral("block"), {100, 700});
			Element *left = place(QStringLiteral("load"), {40, 700});
			Element *right = place(QStringLiteral("load"), {200, 700});
			Element *far_right = place(QStringLiteral("load"), {300, 700});
			QVERIFY(block && left && right && far_right);
			separate(block);

			drawWire(named(left, QStringLiteral("w")),
				 named(block, QStringLiteral("1")));
			Conductor *far = wire(named(block, QStringLiteral("1")));
			QVERIFY(far);
			ConductorProperties far_p = far->properties();
			far_p.m_tension_protocol = QStringLiteral("24 V DC");
			far_p.color = QColor(255, 0, 0);
			far_p.text = QStringLiteral("101");
			far->setProperties(far_p);

			drawWire(named(block, QStringLiteral("2")),
				 named(right, QStringLiteral("w")));
			Conductor *own = wire(named(block, QStringLiteral("2")));
			QVERIFY(own);
			ConductorProperties own_p = own->properties();
			own_p.m_tension_protocol = QStringLiteral("0 V");
			own_p.color = QColor(0, 0, 255);
			own_p.text = QStringLiteral("202");
			own->setProperties(own_p);

				//A new wire joining that side of the separation
			drawWire(named(right, QStringLiteral("w")),
				 named(far_right, QStringLiteral("w")));
			Conductor *fresh = wire(named(far_right, QStringLiteral("w")));
			QVERIFY(fresh);
			QCOMPARE(fresh->properties().m_tension_protocol,
				 QStringLiteral("0 V"));
			QCOMPARE(fresh->properties().color, QColor(0, 0, 255));
			QCOMPARE(fresh->properties().text, QStringLiteral("202"));
		}

			// The same rule where a selection of terminals is wired
			// (ConductorCreator): the drawing and the voltage come from
			// across the separation, the number stays new.
		void selectionWiringTakesNodePropertiesAcrossSeparation()
		{
			Element *block = place(QStringLiteral("block"), {100, 900});
			Element *left = place(QStringLiteral("load"), {40, 900});
			Element *right = place(QStringLiteral("load"), {200, 900});
			QVERIFY(block && left && right);
			separate(block);

			drawWire(named(left, QStringLiteral("w")),
				 named(block, QStringLiteral("1")));
			Conductor *far = wire(named(block, QStringLiteral("1")));
			QVERIFY(far);
			ConductorProperties far_p = far->properties();
			far_p.m_tension_protocol = QStringLiteral("24 V DC");
			far_p.color = QColor(255, 0, 0);
			far_p.style = Qt::DashLine;
			far_p.text = QStringLiteral("101");
			far->setProperties(far_p);

			{
				ConductorCreator cc(m_diagram,
					{named(block, QStringLiteral("2")),
					 named(right, QStringLiteral("w"))});
			}
			Conductor *fresh = wire(named(block, QStringLiteral("2")));
			QVERIFY(fresh);
			QCOMPARE(fresh->properties().m_tension_protocol,
				 QStringLiteral("24 V DC"));
			QCOMPARE(fresh->properties().color, QColor(255, 0, 0));
			QCOMPARE(static_cast<int>(fresh->properties().style),
				 static_cast<int>(Qt::DashLine));
			QVERIFY(fresh->properties().text != QStringLiteral("101"));
		}

			// The numbering of a wire that already exists speaks of the
			// number alone: multi-paste runs it on its pasted wires, and
			// a pasted wire keeps the voltage and the drawing it was
			// copied with (#review)
		void renumberingAnExistingWireKeepsItsProperties()
		{
			Element *block = place(QStringLiteral("block"), {100, 1100});
			Element *left = place(QStringLiteral("load"), {40, 1100});
			Element *right = place(QStringLiteral("load"), {200, 1100});
			QVERIFY(block && left && right);
			separate(block);

			drawWire(named(left, QStringLiteral("w")),
				 named(block, QStringLiteral("1")));
			Conductor *far = wire(named(block, QStringLiteral("1")));
			QVERIFY(far);
			ConductorProperties far_p = far->properties();
			far_p.m_tension_protocol = QStringLiteral("24 V DC");
			far_p.color = QColor(255, 0, 0);
			far->setProperties(far_p);

			drawWire(named(block, QStringLiteral("2")),
				 named(right, QStringLiteral("w")));
			Conductor *own = wire(named(block, QStringLiteral("2")));
			QVERIFY(own);
			ConductorProperties own_p = own->properties();
			own_p.m_tension_protocol = QStringLiteral("0 V");
			own_p.color = QColor(0, 0, 255);
			own->setProperties(own_p);

				//What multi-paste does to a pasted wire of the far side
				//of a separation: the number goes on, the voltage and
				//the drawing stay what the wire says
			ConductorAutoNumerotation can(own, m_diagram);
			can.numerate();
			QCOMPARE(own->properties().m_tension_protocol,
				 QStringLiteral("0 V"));
			QCOMPARE(own->properties().color, QColor(0, 0, 255));
		}
};

QET_APP_TEST_MAIN(tst_functionseparation)
#include "tst_functionseparation.moc"
