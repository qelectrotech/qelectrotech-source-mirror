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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include <QDomDocument>
#include <QGraphicsSceneMouseEvent>
#include <QtTest>

/**
	A wire dragged from a terminal and released on the end of a terminal
	that already has a wire is made. Before, the end of the existing wire,
	drawn over the terminal, was the item under the pointer, and the new
	wire was refused without a word.
*/
class tst_wireonusedterminal : public QObject
{
	Q_OBJECT

		/// A symbol with two terminals, top and bottom
	static Element *symbol(Diagram *diagram, const QPointF &pos)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		const QString name = QStringLiteral("test_simple");
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name))) {
			QDomDocument definition;
			definition.setContent(
				QString(R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), name,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

	static Terminal *top(Element *element)
	{
		Terminal *found = element->terminals().value(0);
		for (Terminal *t : element->terminals())
			if (t->scenePos().y() < found->scenePos().y()) found = t;
		return found;
	}

	static void mouse(QGraphicsScene *scene, QGraphicsItem *item, QEvent::Type type,
					  const QPointF &pos)
	{
		QGraphicsSceneMouseEvent event(type);
		event.setScenePos(pos);
		event.setPos(item->mapFromScene(pos));
		event.setButton(Qt::LeftButton);
		event.setButtons(type == QEvent::GraphicsSceneMouseRelease ? Qt::NoButton
																	 : Qt::LeftButton);
		scene->sendEvent(item, &event);
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

	void aWireDroppedOnAUsedTerminalIsMade()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Element *k1 = symbol(diagram, {100, 100});
		Element *k2 = symbol(diagram, {300, 100});
		Element *k3 = symbol(diagram, {300, 300});
		QVERIFY(k1 && k2 && k3);
		Terminal *used = top(k2);
		diagram->addItem(new Conductor(used, top(k3)));
		QCOMPARE(used->conductors().size(), 1);

			//Drag from K1 and let go on the very end of K2's terminal,
			//where the existing wire is drawn over it
		Terminal *from = top(k1);
		mouse(diagram, from, QEvent::GraphicsSceneMousePress, from->dockConductor());
		mouse(diagram, from, QEvent::GraphicsSceneMouseRelease, used->dockConductor());

		QCOMPARE(used->conductors().size(), 2);
		QCOMPARE(from->conductors().size(), 1);
	}
};

QET_APP_TEST_MAIN(tst_wireonusedterminal)
#include "tst_wireonusedterminal.moc"
