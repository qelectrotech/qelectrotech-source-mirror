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
#include "TerminalStrip/physicalterminal.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QSqlQuery>
#include <QtTest>

/**
	qet_terminals: the terminals of each terminal strip, one row each, in
	the strip's own order -- position and level counted from 0, as the
	terminal strip editor shows them.
*/
class tst_terminallist : public QObject
{
	Q_OBJECT

	int m_position = 0;

	Element *terminal(Diagram *diagram, const QString &label)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/terminal.elmt"))) {
			QDomDocument definition;
			if (!definition.setContent(QString(
					R"(<definition type="element" link_type="terminal" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test terminal</name></names>
<kindInformations><kindInformation name="type">generic</kindInformation><kindInformation name="function">generic</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString())))
				return nullptr;
			if (!collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("terminal"),
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/terminal.elmt"), diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 80 * position, 150);
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		element->setElementInformations(info);
		return element;
	}

	static void setPlace(Diagram *diagram, const QString &plant, const QString &location)
	{
		auto block = diagram->border_and_titleblock.exportTitleBlock();
		block.plant = plant;
		block.locmach = location;
		diagram->border_and_titleblock.importTitleBlock(block);
	}

	static QStringList rows(QETProject &project, const QString &sql, int columns)
	{
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(sql, &error);
		if (!error.isEmpty()) {
			qWarning() << error;
			return {};
		}
		QStringList result;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < columns ; ++i) row << query.value(i).toString();
			result << row.join(QLatin1Char('|'));
		}
		return result;
	}

  private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }
	void init() { m_position = 0; }

	void aTerminalListsItsWires()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *x1 = terminal(folio, QStringLiteral("X1:1"));
		Element *x2 = terminal(folio, QStringLiteral("X1:2"));
		Element *x3 = terminal(folio, QStringLiteral("X1:3"));
		QVERIFY(x1 && x2 && x3);
		TerminalStrip *strip = project.newTerminalStrip(QString(), QString(), QStringLiteral("X1"));
		QVERIFY(strip->addTerminal(x1));
		QVERIFY(strip->addTerminal(x2));
		QVERIFY(strip->addTerminal(x3));
		auto *wire = new Conductor(x1->terminals().at(1), x2->terminals().at(0));
		folio->addItem(wire);
		ConductorProperties properties = wire->properties();
		properties.text = QStringLiteral("101");
		wire->setProperties(properties);
		project.dataBase()->updateDB();
		QCOMPARE(rows(project, QStringLiteral(
					 "SELECT label, wires, connected_to FROM qet_terminals"), 3),
				 QStringList({QStringLiteral("X1:1|101|=FACTORY+CP1-X1:2"),
							  QStringLiteral("X1:2|101|=FACTORY+CP1-X1:1"),
							  QStringLiteral("X1:3||")}));
	}

	void oneRowPerTerminalInStripOrder()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));

		Element *t1 = terminal(folio, QStringLiteral("X1:1"));
		Element *t2 = terminal(folio, QStringLiteral("X1:2"));
		Element *t2b = terminal(folio, QStringLiteral("X1:2"));
		Element *t3 = terminal(folio, QStringLiteral("X1:3"));
		Element *free = terminal(folio, QStringLiteral("X9:1"));
		QVERIFY(t1 && t2 && t2b && t3 && free);

			//The strip's order is its own: 3 before 1 -- and 2 is a terminal
			//of two levels
		TerminalStrip *strip = project.newTerminalStrip(
			QStringLiteral("FACTORY"), QStringLiteral("CP1"), QStringLiteral("X1"));
		QVERIFY(strip->addTerminal(t3));
		QVERIFY(strip->addTerminal(t1));
		QVERIFY(strip->addTerminal(t2));
		QVERIFY(strip->addTerminal(t2b));
		const auto physical = strip->physicalTerminal();
		QCOMPARE(physical.size(), 4);
		QVERIFY(strip->groupTerminals(physical.at(2), physical.at(3)->realTerminals()));
		ElementData spare_data;
		spare_data.m_informations.addValue(QStringLiteral("label"), QStringLiteral("X1:4"));
		QVERIFY(strip->addTerminal(RealTerminal::createSpare(spare_data)));

		project.dataBase()->updateDB();
		QCOMPARE(rows(project, QStringLiteral(
					 "SELECT strip_id, strip, position, level, label, terminal_id, spare, "
					 "folio_number FROM qet_terminals"), 8),
				 QStringList({
					 QStringLiteral("=FACTORY+CP1-X1|X1|0|0|X1:3|=FACTORY+CP1-X1:3|0|1"),
					 QStringLiteral("=FACTORY+CP1-X1|X1|1|0|X1:1|=FACTORY+CP1-X1:1|0|1"),
					 QStringLiteral("=FACTORY+CP1-X1|X1|2|0|X1:2|=FACTORY+CP1-X1:2|0|1"),
					 QStringLiteral("=FACTORY+CP1-X1|X1|2|1|X1:2|=FACTORY+CP1-X1:2|0|1"),
					 QStringLiteral("=FACTORY+CP1-X1|X1|3|0|||1|")}));

			//A strip of one terminal writes it without an index
		TerminalStrip *single = project.newTerminalStrip(
			QString(), QString(), QStringLiteral("X9"));
		QVERIFY(single->addTerminal(free));
		project.dataBase()->updateDB();
		QCOMPARE(rows(project, QStringLiteral(
					 "SELECT strip_id, position, level, label FROM qet_terminals "
					 "WHERE strip = 'X9'"), 4),
				 QStringList({QStringLiteral("-X9|0|0|X9:1")}));

	}
};

QET_APP_TEST_MAIN(tst_terminallist)
#include "tst_terminallist.moc"
