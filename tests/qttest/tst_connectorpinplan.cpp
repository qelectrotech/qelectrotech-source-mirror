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
#include "qetapptest.h"

#include "TerminalStrip/GraphicsItem/properties/terminalstriplayoutshandler.h"
#include "TerminalStrip/GraphicsItem/terminalstripitem.h"
#include "TerminalStrip/GraphicsItem/trueterminalstrip.h"
#include "TerminalStrip/connectorpinplan.h"
#include "TerminalStrip/terminalstrip.h"
#include "diagram.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "project/projectpropertieshandler.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QtTest>

/**
	The plan of a connector, as data: one row for each pin, its terminals
	first, then the pins of its device type drawn nowhere as spare rows. The
	drawer is given these rows (TrueTerminalStrip) and knows nothing of types.
*/
class tst_connectorpinplan : public QObject
{
	Q_OBJECT

	int m_position = 0;

	static bool definePin(QETProject *project)
	{
		auto *collection = project->embeddedElementCollection();
		if (collection->exist(QStringLiteral("import/pin.elmt")))
			return true;
		QDomDocument definition;
		if (!definition.setContent(QString(
				R"(<definition type="element" link_type="terminal" uuid="%1" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">PIN</name></names>
<kindInformations><kindInformation name="type">generic</kindInformation></kindInformations>
<description><line x1="0" y1="-5" x2="0" y2="5"/><terminal x="0" y="5" orientation="s" uuid="%2"/></description></definition>)")
				.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString())))
			return false;
		return collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("pin.elmt"),
												definition.documentElement());
	}

	Element *placePin(Diagram *diagram, const QString &label)
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/pin.elmt"), diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 40 * (position % 8), 150 + 60 * (position / 8));
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		element->setElementInformations(info);
		return element;
	}

	static DeviceType connectorType()
	{
		DeviceType type;
		type.name = QStringLiteral("56P6");
		type.names.insert(QStringLiteral("en"), QStringLiteral("56 series socket, 6 pin"));
		for (const QString &pin : {QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"),
								   QStringLiteral("E"), QStringLiteral("C1"), QStringLiteral("C2")})
			type.parts << DeviceType::Part{QStringLiteral("terminal"), QStringLiteral("terminal"), {},
				{{QStringLiteral("embed://import/pin.elmt"), {pin}, {}}}};
		return type;
	}

		//Socket X1 of type 56P6, with the pins @p pins drawn and in its strip
	TerminalStrip *socket(QETProject &project, const QStringList &pins,
						  TerminalStripData::Kind kind = TerminalStripData::Kind::Socket,
						  const QString &type = QStringLiteral("56P6"))
	{
		Diagram *folio = project.diagrams().isEmpty() ? project.addNewDiagram() : project.diagrams().first();
		if (!definePin(&project)) return nullptr;
		project.setDeviceType(connectorType());
		TerminalStrip *strip = project.newTerminalStrip(QString(), QString(), QStringLiteral("X1"));
		strip->setKind(kind);
		strip->setDeviceType(type);
		for (const QString &pin : pins) {
			Element *element = placePin(folio, QStringLiteral("X1:") + pin);
			if (!element || !strip->addTerminal(element)) return nullptr;
		}
		return strip;
	}

	static QStringList labels(const TerminalStripDrawer::TrueTerminalStrip &data)
	{
		QStringList list;
		for (const auto &physical : data.physicalTerminal())
			for (const auto &real : physical->realTerminals())
				list << real->label();
		return list;
	}

private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
	}

	void pinOfALabel()
	{
		QCOMPARE(ConnectorPinPlan::pinOfLabel(QStringLiteral("X1:L1")), QStringLiteral("L1"));
		QCOMPARE(ConnectorPinPlan::pinOfLabel(QStringLiteral("X1: C2 ")), QStringLiteral("C2"));
		QVERIFY(ConnectorPinPlan::pinOfLabel(QStringLiteral("X1")).isEmpty());
	}

		//A 6 pin socket with 4 pins drawn: 6 rows, 2 spare
	void aSocketOfSixPinsWithFourDrawn()
	{
		QETProject project;
		TerminalStrip *strip = socket(project, {QStringLiteral("L1"), QStringLiteral("L2"),
												QStringLiteral("L3"), QStringLiteral("E")});
		QVERIFY(strip);
		const auto plan = ConnectorPinPlan::ofStrip(strip);
		QCOMPARE(plan.rows.size(), 6);
		QCOMPARE(plan.spareCount(), 2);
		QStringList pins, spare;
		for (const auto &row : plan.rows) {
			pins << row.pin;
			if (row.spare) spare << row.pin;
		}
		QCOMPARE(pins, (QStringList{QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"),
									QStringLiteral("E"), QStringLiteral("C1"), QStringLiteral("C2")}));
		QCOMPARE(spare, (QStringList{QStringLiteral("C1"), QStringLiteral("C2")}));
		QCOMPARE(plan.rows.at(5).label, QStringLiteral("X1:C2"));
		QCOMPARE(plan.rows.at(5).xref, QStringLiteral("Spare"));
		QVERIFY(plan.rows.at(0).physical);
		QVERIFY(!plan.rows.at(5).physical);
		QCOMPARE(plan.header(), QStringLiteral("X1\nSocket\n56P6"));
	}

		//The drawer's data source gives the same rows
	void theDrawerIsGivenTheRows()
	{
		QETProject project;
		TerminalStrip *strip = socket(project, {QStringLiteral("L1"), QStringLiteral("L2"),
												QStringLiteral("L3"), QStringLiteral("E")});
		QVERIFY(strip);
		const TerminalStripDrawer::TrueTerminalStrip data(strip);
		QCOMPARE(data.physicalTerminal().size(), 6);
		QCOMPARE(labels(data), (QStringList{QStringLiteral("X1:L1"), QStringLiteral("X1:L2"),
											QStringLiteral("X1:L3"), QStringLiteral("X1:E"),
											QStringLiteral("X1:C1"), QStringLiteral("X1:C2")}));
		QVERIFY(data.name().startsWith(QStringLiteral("X1\nSocket\n56P6")));
	}

	void aPlugAndAConnectorWithoutAType()
	{
		QETProject project;
		TerminalStrip *plug = socket(project, {QStringLiteral("L1")}, TerminalStripData::Kind::Plug);
		QVERIFY(plug);
		QCOMPARE(ConnectorPinPlan::ofStrip(plug).kind, QStringLiteral("Plug"));
		QCOMPARE(ConnectorPinPlan::ofStrip(plug).rows.size(), 6);
		plug->setDeviceType(QString());
		const auto plan = ConnectorPinPlan::ofStrip(plug);
		QCOMPARE(plan.rows.size(), 1);          //nothing to say about free pins
		QCOMPARE(plan.spareCount(), 0);
		QCOMPARE(plan.header(), QStringLiteral("X1\nPlug"));
	}

		//A terminal strip is drawn as before: its own terminals, its own name
	void aTerminalStripIsUnchanged()
	{
		QETProject project;
		TerminalStrip *strip = socket(project, {QStringLiteral("L1"), QStringLiteral("L2")},
									  TerminalStripData::Kind::Terminals);
		QVERIFY(strip);
		QVERIFY(ConnectorPinPlan::ofStrip(strip).rows.isEmpty());
		const TerminalStripDrawer::TrueTerminalStrip data(strip);
		QCOMPARE(data.name(), QStringLiteral("X1"));
		QCOMPARE(labels(data), (QStringList{QStringLiteral("X1:L1"), QStringLiteral("X1:L2")}));
	}

		//A pin the type lacks and a pin drawn twice stay rows: the plan shows what is drawn
	void whatIsDrawnStaysInThePlan()
	{
		QETProject project;
		TerminalStrip *strip = socket(project, {QStringLiteral("L1"), QStringLiteral("L1"),
												QStringLiteral("7")});
		QVERIFY(strip);
		const auto plan = ConnectorPinPlan::ofStrip(strip);
		QCOMPARE(plan.rows.size(), 3 + 5);
		QCOMPARE(plan.spareCount(), 5);
	}

	void theItemTakesTheConnectorLayout()
	{
		QETProject project;
		TerminalStrip *strip = socket(project, {QStringLiteral("L1")});
		QVERIFY(strip);
		auto &handler = project.projectPropertiesHandler().terminalStripLayoutHandler();
		QVERIFY(handler.connectorLayout() != handler.defaultLayout());
		QVERIFY(handler.connectorLayout()->m_header_rect.width() > handler.defaultLayout()->m_header_rect.width());
		TerminalStripItem item(strip);
		TerminalStrip *other = project.newTerminalStrip(QString(), QString(), QStringLiteral("X2"));
		TerminalStripItem plain(other);
			//Six rows, the connector's wider header: wider than a strip of one terminal
		QVERIFY(item.boundingRect().width() > 0);
		strip->setKind(TerminalStripData::Kind::Terminals);
		QVERIFY(item.boundingRect().width() > 0);
	}
};

QET_APP_TEST_MAIN(tst_connectorpinplan)
#include "tst_connectorpinplan.moc"
