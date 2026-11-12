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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "cable/cable.h"
#include "cable/cablemanager.h"
#include "cable/cablepart.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "diagramview.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QtTest>

/**
	Copying a drawing with a cable on it gives the copy a cable of its
	own, on the wires which came with the copy. The cable which was
	copied keeps every one of its cores.
*/
class tst_cablecopy : public QObject
{
	Q_OBJECT

	DiagramView *m_view = nullptr;
	Diagram *m_diagram = nullptr;

		/// A symbol with one terminal at the top and one at the bottom
	Element *symbol(const QPointF &pos)
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/twoends.elmt"),
							 m_diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		m_diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

	static Terminal *terminalAt(Element *element, bool bottom)
	{
		const QList<Terminal *> terminals = element->terminals();
		Terminal *found = terminals.value(0);
		for (Terminal *t : terminals)
			if ((t->scenePos().y() > found->scenePos().y()) == bottom && t != found)
				found = t;
		return found;
	}

	static int wiredCores(const Cable *cable)
	{
		int wired = 0;
		for (const CableCore &core : cable->usedCores())
			if (!core.conductor.isNull()) ++wired;
		return wired;
	}

  private slots:
	void initTestCase()
	{
		m_view = QetAppTest::view();
		QVERIFY(m_view);
		m_diagram = m_view->diagram();
		QDomDocument definition;
		definition.setContent(QStringLiteral(
			R"(<definition type="element" link_type="simple" uuid="{5c1e0d2e-6c55-4f0e-9d7e-1b0b8f3c2a11}" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Two ends</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-20" orientation="n" uuid="{8a0f6b55-2a0e-4b4e-8c7f-0d9b3c1e5f21}"/><terminal x="0" y="20" orientation="s" uuid="{4b7d2c19-9e3a-4f61-a2c5-6e8f0a1b3d42}"/>
</description></definition>)"));
		QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
			QStringLiteral("import"), QStringLiteral("twoends"), definition.documentElement()));
	}

		//Two wires carried by one two-core cable, everything copied and
		//pasted: the cable copied from still has both of its cores
	void theCopiedCableKeepsItsCores()
	{
		QETProject *project = m_diagram->project();
		QList<Conductor *> wires;
		for (const qreal x : {100.0, 140.0}) {
			Element *top = symbol(QPointF(x, 100));
			Element *bottom = symbol(QPointF(x, 300));
			QVERIFY(top && bottom);
			auto *wire = new Conductor(terminalAt(top, true), terminalAt(bottom, false));
			m_diagram->addItem(wire);
			wires << wire;
		}

		Cable *cable = project->newCable();
		cable->setDesignation(QStringLiteral("-W1"));
		cable->setCoreCount(2);
		CablePartData line;
		line.uuid = QUuid::createUuid();
		line.diagram = m_diagram->uuid();
		line.p1 = QPointF(80, 200);
		line.p2 = QPointF(160, 200);
		cable->addPart(line);
		auto *part = new CablePart(line);
		part->setCable(cable);
		m_diagram->addItem(part);
		for (int i = 0; i < wires.size(); ++i) {
			CableCore core;
			core.core = i;
			core.part = line.uuid;
			core.conductor = wires.at(i)->uuid();
			core.position = QPointF(100 + 40 * i, 200);
			cable->setCore(core);
		}
		CableManager::refreshLabels(project);
		QCOMPARE(wiredCores(cable), 2);

		for (QGraphicsItem *item : m_diagram->items())
			if (item->flags() & QGraphicsItem::ItemIsSelectable)
				item->setSelected(true);
		m_view->copy();
		m_view->paste(QPointF(100, 500));

		Cable *copy = nullptr;
		for (Cable *other : project->cables())
			if (other != cable) copy = other;
		QVERIFY2(copy, "the paste made no cable");
		QCOMPARE(wiredCores(copy), 2);
		QCOMPARE(wiredCores(cable), 2);
		for (Conductor *wire : std::as_const(wires))
			QCOMPARE(wire->properties().m_cable_uuid, cable->uuid());

			//Taking the paste back and doing it again changes nothing
			//for the cable it was copied from
		project->undoStack()->undo();
		QCOMPARE(wiredCores(cable), 2);
		project->undoStack()->redo();
		QCOMPARE(wiredCores(cable), 2);
		QCOMPARE(wiredCores(copy), 2);
	}
};

QET_APP_TEST_MAIN(tst_cablecopy)
#include "tst_cablecopy.moc"
