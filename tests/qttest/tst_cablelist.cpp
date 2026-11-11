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
#include "cable/cable.h"
#include "cablelist/cablelistrows.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QtTest>

/**
	The rows of the cable list (Lists > cable list): where each cable
	starts and ends.
*/
class tst_cablelist : public QObject
{
	Q_OBJECT

	/// A symbol with two terminals, top and bottom, of the given link type
	static Element *symbol(Diagram *diagram, const QString &link_type, const QString &label,
						   const QPointF &pos)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		const QString name = QStringLiteral("test_") + link_type;
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name))) {
			QDomDocument definition;
			definition.setContent(
				QString(R"(<definition type="element" link_type="%1" uuid="%2" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%3"/><terminal x="0" y="20" orientation="s" uuid="%4"/>
</description></definition>)")
					.arg(link_type, QUuid::createUuid().toString(),
						 QUuid::createUuid().toString(), QUuid::createUuid().toString()));
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
		DiagramContext info = element->elementInformations();
		info.addValue(QETInformation::ELMT_LABEL, label);
		element->setElementInformations(info);
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

	static Cable *cable(QETProject &project, const QString &designation)
	{
		auto *cable = new Cable(&project);
		cable->setDesignation(designation);
		project.addCable(cable);
		return cable;
	}

  private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

		//A cable whose cores land on the terminals of one strip ends at the
		//strip, not at its first two terminals
	void coresOnOneStripAreOneEnd()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Cable *w1 = cable(project, QStringLiteral("-W1"));
		w1->setCoreCount(2);
		Element *x1_1 = symbol(folio, QStringLiteral("terminal"), QStringLiteral("-X1:1"), {100, 100});
		Element *x1_2 = symbol(folio, QStringLiteral("terminal"), QStringLiteral("-X1:2"), {140, 100});
		Element *motor = symbol(folio, QStringLiteral("simple"), QStringLiteral("-M1"), {120, 300});
		QVERIFY(x1_1 && x1_2 && motor);
		for (Element *terminal : {x1_1, x1_2}) {
			auto *core = new Conductor(terminalAt(terminal, true), terminalAt(motor, false));
			folio->addItem(core);
			ConductorProperties properties = core->properties();
			properties.m_cable_uuid = w1->uuid();
			core->setProperties(properties);
		}
		const auto rows = CableList::rows(&project);
		QCOMPARE(rows.size(), 1);
		QCOMPARE(rows.first().value(QStringLiteral("bmk_start")), QStringLiteral("-X1"));
		QCOMPARE(rows.first().value(QStringLiteral("bmk_end")), QStringLiteral("-M1"));
	}
};

QET_APP_TEST_MAIN(tst_cablelist)
#include "tst_cablelist.moc"
