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
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "qetapptest.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QSqlQuery>
#include <QtTest>

/**
	qet_cables: the project's cables, one row each, read from the cables
	themselves. A cable's plant and location are its own, else those of
	the first folio it is drawn on; structure_id composes them with its
	designation like a device's. Also the rows of the cable list
	(Lists > cable list): where each cable starts and ends.
*/
class tst_cablelist : public QObject
{
	Q_OBJECT

	static void setPlace(Diagram *diagram, const QString &plant, const QString &location)
	{
		auto block = diagram->border_and_titleblock.exportTitleBlock();
		block.plant = plant;
		block.locmach = location;
		diagram->border_and_titleblock.importTitleBlock(block);
	}

	static QUuid drawOn(Cable *cable, Diagram *diagram)
	{
		CablePartData part;
		part.uuid = QUuid::createUuid();
		part.diagram = diagram->uuid();
		part.p1 = QPointF(100, 100);
		part.p2 = QPointF(100, 200);
		cable->addPart(part);
		return part.uuid;
	}

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

	void oneRowPerCore()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Cable *w1 = cable(project, QStringLiteral("W1"));
		w1->setCoreCount(3);
		const QUuid part = drawOn(w1, folio);
		CableCore free;
		free.core = 2;
		free.part = part;
		w1->setCore(free);
		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id, position, core, wire_number FROM qet_cable_cores"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QStringList rows;
		while (query.next())
			rows << query.value(0).toString() + QLatin1Char('|') + query.value(1).toString()
					+ QLatin1Char('|') + query.value(2).toString() + QLatin1Char('|')
					+ query.value(3).toString();
		QCOMPARE(rows, QStringList({QStringLiteral("=FACTORY+CP1-W1|0|1|"),
									QStringLiteral("=FACTORY+CP1-W1|1|2|"),
									QStringLiteral("=FACTORY+CP1-W1|2|3|")}));
	}

	void oneRowPerCable()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		setPlace(first, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setPlace(second, QStringLiteral("FACTORY"), QStringLiteral("CP2"));

			//Drawn on both folios: in the place of the first; two cores
			//drawn, one of them on a wire
		Cable *w1 = cable(project, QStringLiteral("W1"));
		w1->setType(QStringLiteral("CONTROL 6C+E"));
		w1->setCoreCount(7);
		drawOn(w1, second);
		const QUuid part = drawOn(w1, first);
		CableCore wired;
		wired.core = 0;
		wired.part = part;
		wired.conductor = QUuid::createUuid();
		w1->setCore(wired);
		CableCore free;
		free.core = 6;
		free.part = part;
		w1->setCore(free);
			//Its own place, drawn nowhere
		Cable *w2 = cable(project, QStringLiteral("W2"));
		w2->setInstallation(QStringLiteral("SILO"));
		w2->setLocation(QStringLiteral("P1"));
		w2->setCoreCount(3);
			//Drawn on the second folio only
		drawOn(cable(project, QStringLiteral("W3")), second);

		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id, designation, type, cores, drawn_cores, wired_cores, "
			"folios, folio_number, cable_uuid FROM qet_cables"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QStringList rows;
		QString w1_uuid;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < 8 ; ++i) row << query.value(i).toString();
			rows << row.join(QLatin1Char('|'));
			if (query.value(1).toString() == QLatin1String("W1"))
				w1_uuid = query.value(8).toString();
		}
		QCOMPARE(rows, QStringList({
			QStringLiteral("=FACTORY+CP1-W1|W1|CONTROL 6C+E|7|2|1|2|1"),
			QStringLiteral("=FACTORY+CP2-W3|W3||0|0|0|1|2"),
			QStringLiteral("=SILO+P1-W2|W2||3|0|0|0|")}));
		QCOMPARE(w1_uuid, w1->uuid().toString());
	}

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
