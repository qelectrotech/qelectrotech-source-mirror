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

#include "TerminalStrip/GraphicsItem/trueterminalstrip.h"
#include "TerminalStrip/GraphicsItem/terminalstripitem.h"
#include "TerminalStrip/connectorpinplan.h"
#include "TerminalStrip/ui/addterminalstripitemdialog.h"
#include "devicetype/devicetypecheck.h"
#include "diagram.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "qetresult.h"
#include "xml/terminalstripitemxml.h"

#include <QComboBox>
#include <QDomDocument>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

/**
	A connector drawn as a device, as a plug is on a motor circuit: a head
	symbol with its pins (L1 L2 L3 E) and child symbols for the others (C1,
	C2). Its type says "connector" in its informations; the plan gets the
	same rows as the plan of a connector of a strip, from the terminals of
	its symbols and their wires.
*/
class tst_connectordeviceplan : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_position = 0;

	static bool define(QETProject *project, const QString &file, const QString &name,
					   const QString &link_type, const QStringList &pins)
	{
		auto *collection = project->embeddedElementCollection();
		if (collection->exist(QStringLiteral("import/") + file + QStringLiteral(".elmt")))
			return true;
		QString terminals;
		for (int i = 0 ; i < pins.size() ; ++i)
			terminals += QStringLiteral("<terminal x=\"%1\" y=\"20\" orientation=\"s\" name=\"%2\" uuid=\"%3\"/>")
					.arg(10 * i).arg(pins.at(i), QUuid::createUuid().toString());
		QDomDocument definition;
		if (!definition.setContent(QString(
				R"(<definition type="element" link_type="%1" uuid="%2" width="60" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">%3</name></names>
<kindInformations><kindInformation name="type">%4</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>%5</description></definition>)")
				.arg(link_type, QUuid::createUuid().toString(), name,
					 link_type == QLatin1String("slave") ? QStringLiteral("simple") : QStringLiteral("coil"),
					 terminals)))
			return false;
		return collection->addElementDefinition(QStringLiteral("import"), file,
												definition.documentElement());
	}

	Element *place(Diagram *diagram, const QString &file, const QString &label,
				   const QString &type = QString())
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/") + file + QStringLiteral(".elmt"),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 120 * (position % 6), 150 + 120 * (position / 6));
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		if (!type.isEmpty()) info.addValue(QStringLiteral("device_type"), type);
		element->setElementInformations(info);
		return element;
	}

		//PLUG-6: a head with L1 L2 L3 E, two children with C1 and C2; says "connector"
	static DeviceType plugType(const QString &flag = QStringLiteral("plug"))
	{
		DeviceType type;
		type.name = QStringLiteral("PLUG-6");
		type.names.insert(QStringLiteral("en"), QStringLiteral("PLUG, 3 PHASE + EARTH, 2 CONTROL PINS"));
		if (!flag.isEmpty()) type.informations << qMakePair(QStringLiteral("connector"), flag);
		type.parts = {
			DeviceType::Part{QStringLiteral("master"), QStringLiteral("simple"), {},
				{{QStringLiteral("embed://import/head.elmt"),
				  {QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"), QStringLiteral("E")}, {}}}},
			DeviceType::Part{QStringLiteral("slave"), QStringLiteral("simple"), {},
				{{QStringLiteral("embed://import/pinc.elmt"), {QStringLiteral("C1")}, {}}}},
			DeviceType::Part{QStringLiteral("slave"), QStringLiteral("simple"), {},
				{{QStringLiteral("embed://import/pinc2.elmt"), {QStringLiteral("C2")}, {}}}}};
		return type;
	}

	bool defineAll(QETProject *project)
	{
		return define(project, QStringLiteral("head"), QStringLiteral("PLUG-HEAD"), QStringLiteral("master"),
					  {QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"), QStringLiteral("E")})
			&& define(project, QStringLiteral("pinc"), QStringLiteral("PIN-C1"), QStringLiteral("slave"), {QStringLiteral("C1")})
			&& define(project, QStringLiteral("pinc2"), QStringLiteral("PIN-C2"), QStringLiteral("slave"), {QStringLiteral("C2")})
			&& define(project, QStringLiteral("lamp"), QStringLiteral("LAMP"), QStringLiteral("simple"),
					  {QStringLiteral("X1"), QStringLiteral("X2")});
	}

		//XP1 drawn as a head with L1 L2 L3 E and the children C1, C2 (those in @p children)
	Element *plug(Diagram *folio, const QString &type, const QStringList &children)
	{
		Element *head = place(folio, QStringLiteral("head"), QStringLiteral("XP1"), type);
		if (!head) return nullptr;
		for (const QString &child : children) {
			Element *pin = place(folio, child == QLatin1String("C1") ? QStringLiteral("pinc") : QStringLiteral("pinc2"),
								 QStringLiteral("XP1"));
			if (!pin) return nullptr;
			head->linkToElement(pin);
		}
		return head;
	}

	static QStringList spareOf(const ConnectorPinPlan::Plan &plan)
	{
		QStringList list;
		for (const auto &row : plan.rows)
			if (row.spare) list << row.pin;
		return list;
	}

private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
		QVERIFY(m_dir.isValid());
	}

	void theRuleForAConnectorType()
	{
		QVERIFY(ConnectorPinPlan::isConnectorType(plugType()));
		QVERIFY(ConnectorPinPlan::isConnectorType(plugType(QStringLiteral("anything"))));
		QCOMPARE(ConnectorPinPlan::kindOfType(plugType(QStringLiteral("Socket"))), QStringLiteral("Socket"));
		QVERIFY(ConnectorPinPlan::kindOfType(plugType(QStringLiteral("yes"))).isEmpty());
			//No flag, no pins of role terminal: a contactor-like type is not a connector
		QVERIFY(!ConnectorPinPlan::isConnectorType(plugType(QString())));
		DeviceType terminals = plugType(QString());
		terminals.parts[1].role = QStringLiteral("terminal");
		QVERIFY(ConnectorPinPlan::isConnectorType(terminals));
			//A terminal strip's type: a terminal with no pin names
		DeviceType feed;
		feed.name = QStringLiteral("TB-FEED");
		feed.parts << DeviceType::Part{QStringLiteral("terminal"), QStringLiteral("terminal"), {},
			{{QStringLiteral("embed://import/pin.elmt"), {}, {}}}};
		QVERIFY(!ConnectorPinPlan::isConnectorType(feed));
		QVERIFY(!ConnectorPinPlan::isConnectorType(DeviceType()));
	}

		//XP1 of a 6 pin type drawn as a head (L1 L2 L3 E) and two children (C1, C2)
	void sixPinsWithTheirWires()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(defineAll(&project));
		project.setDeviceType(plugType());
		Element *head = plug(folio, QStringLiteral("PLUG-6"), {QStringLiteral("C1"), QStringLiteral("C2")});
		QVERIFY(head);
		Element *lamp = place(folio, QStringLiteral("lamp"), QStringLiteral("H1"));
		QVERIFY(lamp);
		Terminal *l1 = nullptr;
		for (Terminal *t : head->terminals())
			if (t->baseName() == QLatin1String("L1")) l1 = t;
		QVERIFY(l1);
		auto *wire = new Conductor(l1, lamp->terminals().first());
		QVERIFY(wire->isValid());
		folio->addItem(wire);

		const auto device = DeviceTypeCheck::checkDevice(head);
		QVERIFY(device.known());
		QCOMPARE(device.symbols.size(), 3);
		const auto plan = ConnectorPinPlan::ofDevice(device);
		QCOMPARE(plan.rows.size(), 6);
		QVERIFY(spareOf(plan).isEmpty());
		QStringList pins;
		for (const auto &row : plan.rows) pins << row.pin;
		QCOMPARE(pins, (QStringList{QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"),
									QStringLiteral("E"), QStringLiteral("C1"), QStringLiteral("C2")}));
		QCOMPARE(plan.rows.at(0).label, QStringLiteral("XP1:L1"));
		QCOMPARE(plan.rows.at(0).wires.size(), 1);
		QCOMPARE(plan.rows.at(0).wires.first().first, QStringLiteral("H1:X1"));
		for (int i = 1 ; i < 6 ; ++i) QVERIFY(plan.rows.at(i).wires.isEmpty());
		QCOMPARE(plan.header(), QStringLiteral("XP1\nPlug\nPLUG-6"));
			//The kind is the one the type says
		QCOMPARE(ConnectorPinPlan::ofDevice(device).kind, QStringLiteral("Plug"));
	}

	void aChildNotDrawnIsSpare()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(defineAll(&project));
		project.setDeviceType(plugType(QStringLiteral("socket")));
		Element *head = plug(folio, QStringLiteral("PLUG-6"), {QStringLiteral("C1")});
		QVERIFY(head);
		const auto plan = ConnectorPinPlan::ofDevice(DeviceTypeCheck::checkDevice(head));
		QCOMPARE(plan.rows.size(), 6);
		QCOMPARE(spareOf(plan), QStringList{QStringLiteral("C2")});
		QCOMPARE(plan.rows.last().label, QStringLiteral("XP1:C2"));
		QCOMPARE(plan.rows.last().xref, QStringLiteral("Spare"));
		QCOMPARE(plan.kind, QStringLiteral("Socket"));
	}

		//The drawer's data source gives the rows, and caches them until refresh
	void theDrawerIsGivenTheRows()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(defineAll(&project));
		project.setDeviceType(plugType());
		Element *head = plug(folio, QStringLiteral("PLUG-6"), {QStringLiteral("C1")});
		QVERIFY(head);
		TerminalStripDrawer::DevicePinPlanStrip data(head);
		QCOMPARE(data.physicalTerminal().size(), 6);
		QVERIFY(data.name().startsWith(QStringLiteral("XP1\nPlug\nPLUG-6")));
		QStringList labels;
		for (const auto &physical : data.physicalTerminal())
			for (const auto &real : physical->realTerminals()) labels << real->label();
		QCOMPARE(labels.last(), QStringLiteral("XP1:C2"));

		auto *item = new TerminalStripItem();
		folio->addItem(item);
		item->setDevice(head);
		QVERIFY(item->boundingRect().width() > 0);
		QCOMPARE(item->device().data(), head);
	}

	void aDeviceWithAnOrdinaryTypeIsNoConnector()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(defineAll(&project));
		project.setDeviceType(plugType(QString()));
		QVERIFY(plug(folio, QStringLiteral("PLUG-6"), {QStringLiteral("C1")}));
		QVERIFY(ConnectorPinPlan::connectorDevices(&project).isEmpty());
	}

	void theDialogOffersTheDevices()
	{
		QETProject project;
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		Diagram *folio = project.addNewDiagram();
		QVERIFY(defineAll(&project));
		project.setDeviceType(plugType());
		project.newTerminalStrip(QString(), QString(), QStringLiteral("X1"));
		Element *head = plug(folio, QStringLiteral("PLUG-6"), {QStringLiteral("C1"), QStringLiteral("C2")});
		QVERIFY(head);
		QCOMPARE(ConnectorPinPlan::connectorDevices(&project), QList<Element *>{head});

		AddTerminalStripItemDialog dialog(&project);
		auto *combo = dialog.findChild<QComboBox *>(QStringLiteral("m_terminal_strip_cb"));
		QVERIFY(combo);
		QCOMPARE(combo->count(), 2);
		QVERIFY(combo->itemText(1).contains(QStringLiteral("XP1")));
		combo->setCurrentIndex(0);
		QVERIFY(dialog.selectedTerminalStrip());
		QVERIFY(!dialog.selectedDevice());
		combo->setCurrentIndex(1);
		QVERIFY(!dialog.selectedTerminalStrip());
		QCOMPARE(dialog.selectedDevice(), head);
	}

		//Saved with the folio, found again by the uuid of its head symbol
	void thePlanIsSavedWithTheProject()
	{
		const QString path = m_dir.filePath(QStringLiteral("device-plan.qet"));
		QUuid head_uuid;
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			QVERIFY(defineAll(&project));
			project.setDeviceType(plugType());
			Element *head = plug(folio, QStringLiteral("PLUG-6"), {QStringLiteral("C1"), QStringLiteral("C2")});
			QVERIFY(head);
			head_uuid = head->uuid();
			auto *item = new TerminalStripItem();
			folio->addItem(item);
			item->setDevice(head);
			item->setPos(50, 50);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QCOMPARE(again.state(), QETProject::Ok);
		again.loadFolios();
		TerminalStripItem *found = nullptr;
		for (Diagram *folio : again.diagrams())
			for (QGraphicsItem *item : folio->items())
				if (item->type() == TerminalStripItem::Type)
					found = static_cast<TerminalStripItem *>(item);
		QVERIFY(found);
		QCOMPARE(found->deviceUuid(), head_uuid);
		QVERIFY(found->device());
		QCOMPARE(found->device()->uuid(), head_uuid);
	}
};

QET_APP_TEST_MAIN(tst_connectordeviceplan)
#include "tst_connectordeviceplan.moc"
