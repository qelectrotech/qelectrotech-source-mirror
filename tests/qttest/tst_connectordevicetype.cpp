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

#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "TerminalStrip/terminalstripdata.h"
#include "TerminalStrip/ui/terminalstripeditor.h"
#include "dataBase/projectdatabase.h"
#include "devicetype/devicetypecheck.h"
#include "diagram.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/devicetypechooser.h"
#include "qetresult.h"
#include "ui/devicetypecheckdialog.h"

#include <QComboBox>
#include <QDomDocument>
#include <QLineEdit>
#include <QScopeGuard>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QUndoStack>
#include <QtTest>

/**
	A connector (a strip of kind plug or socket) has a device type: the name
	of a device type of the project, whose parts of role terminal are its
	pins. The check of the device types then reads the connector as one
	device: a pin the type lacks, a pin drawn twice, the free pins. A terminal
	strip, and a connector without a type, are checked as they were.
*/
class tst_connectordevicetype : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_position = 0;

	static QString xmlOf(const TerminalStripData &data)
	{
		QDomDocument doc;
		doc.appendChild(data.toXml(doc));
		return doc.toString(-1);
	}

	static TerminalStripData dataFromXml(const QString &xml)
	{
		QDomDocument doc;
		doc.setContent(xml);
		TerminalStripData data;
		data.fromXml(doc.documentElement());
		return data;
	}

		//The element import/pin.elmt: a terminal (a pin symbol)
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

		//A pin symbol labelled @p label, such as X1:L1
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

		//56P6: a 6 pin connector, L1 L2 L3 E C1 C2, one part of role terminal each
	static DeviceType connectorType(const QString &name = QStringLiteral("56P6"),
									const QStringList &pins = QStringList{
										QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"),
										QStringLiteral("E"), QStringLiteral("C1"), QStringLiteral("C2")})
	{
		DeviceType type;
		type.name = name;
		for (const QString &pin : pins)
			type.parts << DeviceType::Part{QStringLiteral("terminal"), QStringLiteral("terminal"), {},
				{{QStringLiteral("embed://import/pin.elmt"), {pin}, {}}}};
		return type;
	}

	static TerminalStrip *connector(QETProject &project, const QString &name,
									TerminalStripData::Kind kind, const QString &type)
	{
		TerminalStrip *strip = project.newTerminalStrip(QString(), QString(), name);
		strip->setKind(kind);
		strip->setDeviceType(type);
		return strip;
	}

		//Draws the pins @p pins of connector @p name
	void draw(Diagram *folio, const QString &name, const QStringList &pins)
	{
		for (const QString &pin : pins)
			QVERIFY(placePin(folio, name + QLatin1Char(':') + pin));
	}

	static QStringList summary(const DeviceTypeCheck::Device &device)
	{
		QStringList words;
		words << device.label << device.typeName << QString::number(device.symbols.size())
			  << QString::number(device.known()) << QString::number(device.complete())
			  << QString::number(device.beyond());
		for (int d : device.drawn) words << QString::number(d);
		for (int m : device.missing) words << QStringLiteral("m%1").arg(m);
		return words;
	}

private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
		QVERIFY(m_dir.isValid());
	}

	void noDeviceTypeIsNotWritten()
	{
		TerminalStripData data;
		QVERIFY(data.deviceType().isEmpty());
		QVERIFY(!xmlOf(data).contains(QLatin1String("device_type")));
		data.setKind(TerminalStripData::Kind::Socket);
		QVERIFY(!xmlOf(data).contains(QLatin1String("device_type")));
	}

	void deviceTypeRoundTrip()
	{
		TerminalStripData data;
		data.setKind(TerminalStripData::Kind::Socket);
		data.setDeviceType(QStringLiteral("56P6"));
		const QString xml = xmlOf(data);
		QVERIFY(xml.contains(QStringLiteral("<information name=\"device_type\">56P6</information>")));
		const auto back = dataFromXml(xml);
		QCOMPARE(back.deviceType(), QStringLiteral("56P6"));
		QCOMPARE(back.kind(), TerminalStripData::Kind::Socket);
		QCOMPARE(TerminalStripData(back).deviceType(), QStringLiteral("56P6"));
	}

	void projectKeepsTheDeviceType_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("type.qet");
		QTest::newRow("qetz") << QStringLiteral("type.qetz");
		QTest::newRow("qetdb") << QStringLiteral("type.qetdb");
	}

	void projectKeepsTheDeviceType()
	{
		QFETCH(QString, file);
		const QString path = m_dir.filePath(file);
		{
			QETProject project;
			project.addNewDiagram();
			project.setDeviceType(connectorType());
			TerminalStrip *strip = connector(project, QStringLiteral("X1"),
											 TerminalStripData::Kind::Socket, QStringLiteral("56P6"));
			ElementData spare_data;
			spare_data.m_informations.addValue(QStringLiteral("label"), QStringLiteral("X1:L1"));
			strip->addTerminal(RealTerminal::createSpare(spare_data));
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QCOMPARE(again.state(), QETProject::Ok);
		QCOMPARE(again.terminalStrip().size(), 1);
		QCOMPARE(again.terminalStrip().first()->deviceType(), QStringLiteral("56P6"));
		QCOMPARE(again.terminalStrip().first()->kind(), TerminalStripData::Kind::Socket);
	}

	void viewTellsTheDeviceType()
	{
		QETProject project;
		project.addNewDiagram();
		for (const QString &name : {QStringLiteral("X1"), QStringLiteral("X2")}) {
			TerminalStrip *strip = connector(project, name, TerminalStripData::Kind::Socket,
											 name == QLatin1String("X1") ? QStringLiteral("56P6") : QString());
			ElementData spare_data;
			spare_data.m_informations.addValue(QStringLiteral("label"), name + QStringLiteral(":L1"));
			strip->addTerminal(RealTerminal::createSpare(spare_data));
		}
		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(
			QStringLiteral("SELECT strip, strip_device_type FROM qet_terminals ORDER BY strip"), &error);
		QVERIFY(error.isEmpty());
		QVERIFY(query.next());
		QCOMPARE(query.value(0).toString(), QStringLiteral("X1"));
		QCOMPARE(query.value(1).toString(), QStringLiteral("56P6"));
		QVERIFY(query.next());
		QCOMPARE(query.value(1).toString(), QString());
	}

	void editorChangesTheDeviceTypeInOneUndoStep()
	{
		QETProject project;
		project.addNewDiagram();
		project.setDeviceType(connectorType());
		TerminalStrip *strip = connector(project, QStringLiteral("X1"),
										 TerminalStripData::Kind::Socket, QString());
		TerminalStripEditor editor(&project);
		editor.setCurrentStrip(strip);
		auto *field = editor.findChild<QLineEdit *>(QStringLiteral("m_device_type_le"));
		auto *kind = editor.findChild<QComboBox *>(QStringLiteral("m_kind_cb"));
		QVERIFY(field && kind);
		QVERIFY(!field->isHidden());

		const int steps_before = project.undoStack()->count();
		field->setText(QStringLiteral("56P6"));
		editor.apply();
		QCOMPARE(strip->deviceType(), QStringLiteral("56P6"));
		QCOMPARE(project.undoStack()->count(), steps_before + 1);
		project.undoStack()->undo();
		QVERIFY(strip->deviceType().isEmpty());
		project.undoStack()->redo();
		QCOMPARE(strip->deviceType(), QStringLiteral("56P6"));

			//A terminal strip has none: the field goes, and so does the type
		kind->setCurrentIndex(0);
		QVERIFY(field->isHidden());
		editor.apply();
		QVERIFY(strip->deviceType().isEmpty());
		QCOMPARE(strip->kind(), TerminalStripData::Kind::Terminals);
	}

		//X1 = socket of type 56P6 (L1 L2 L3 E C1 C2), pins drawn L1 L2 L3 E C1 C1
	void aPinDrawnTwiceAndAFreePin()
	{
		QETProject project;
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		Diagram *folio = project.addNewDiagram();
		QVERIFY(definePin(&project));
		project.setDeviceType(connectorType());
		connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Socket, QStringLiteral("56P6"));
		draw(folio, QStringLiteral("X1"), {QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"),
										   QStringLiteral("E"), QStringLiteral("C1"), QStringLiteral("C1")});

		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		const auto &device = devices.first();
		QCOMPARE(device.label, QStringLiteral("X1"));
		QCOMPARE(device.typeName, QStringLiteral("56P6"));
		QVERIFY(device.connector);
		QVERIFY(device.known());
		QCOMPARE(device.symbols.size(), 6);
		QCOMPARE(device.doubled, QStringList{QStringLiteral("C1")});
		QCOMPARE(DeviceTypeCheck::freePins(device), QStringList{QStringLiteral("C2")});
		QCOMPARE(device.beyond(), 1);
		QVERIFY(!device.complete());

		DeviceTypeCheckDialog dialog(&project);
		auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("deviceTypeDevices"));
		QVERIFY(tree);
		QCOMPARE(tree->topLevelItemCount(), 1);
		QStringList rows;
		auto *root = tree->topLevelItem(0);
		QVERIFY(root->text(0).endsWith(QStringLiteral("X1: 5 of 6 pins drawn")));
		for (int i = 0 ; i < root->childCount() ; ++i) rows << root->child(i)->text(0);
		QCOMPARE(rows, (QStringList{QStringLiteral("Free pin: C2"), QStringLiteral("Pin C1 is drawn twice")}));
	}

	void aPinTheTypeLacks()
	{
		QETProject project;
		QETApp::registerProject(&project);
		const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
		Diagram *folio = project.addNewDiagram();
		QVERIFY(definePin(&project));
		project.setDeviceType(connectorType());
		connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Plug, QStringLiteral("56P6"));
		draw(folio, QStringLiteral("X1"), {QStringLiteral("L1"), QStringLiteral("7")});
		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QVERIFY(devices.first().doubled.isEmpty());
		QCOMPARE(devices.first().beyond(), 1);
		QCOMPARE(DeviceTypeCheck::freePins(devices.first()).size(), 5);

		DeviceTypeCheckDialog dialog(&project);
		auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("deviceTypeDevices"));
		QVERIFY(tree);
		QStringList rows;
		auto *root = tree->topLevelItem(0);
		for (int i = 0 ; i < root->childCount() ; ++i) rows << root->child(i)->text(0);
		QVERIFY(rows.contains(QStringLiteral("X1 has no pin 7")));
	}

	void aCompleteConnector()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(definePin(&project));
		project.setDeviceType(connectorType());
		connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Socket, QStringLiteral("56P6"));
		draw(folio, QStringLiteral("X1"), {QStringLiteral("C2"), QStringLiteral("L1"), QStringLiteral("L2"),
										   QStringLiteral("L3"), QStringLiteral("E"), QStringLiteral("C1")});
		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QVERIFY(devices.first().complete());
		QVERIFY(DeviceTypeCheck::freePins(devices.first()).isEmpty());
	}

		//Terminal strips and connectors without a type give what they gave
		//before the strip data had a device type: no strip object, no change
	void stripsAreCheckedAsBefore()
	{
		const QStringList pins{QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("2")};
		const auto run = [this, &pins](int variant) {
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			QList<QStringList> result;
			if (!definePin(&project)) return result;
			project.setDeviceType(connectorType(QStringLiteral("T"), {QStringLiteral("1"), QStringLiteral("2")}));
			if (variant == 1) {          //a connector without a device type
				connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Plug, QString());
			} else if (variant == 2) {   //a terminal strip, whatever its data says
				connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Terminals, QStringLiteral("T"));
			}
			for (const QString &pin : pins) {
				Element *element = placePin(folio, QStringLiteral("X1:") + pin);
				if (!element) return result;
				auto info = element->elementInformations();
				info.addValue(QStringLiteral("device_type"), QStringLiteral("T"));
				element->setElementInformations(info);
			}
			for (const auto &device : DeviceTypeCheck::check(&project)) {
				if (device.connector || !device.pinNames.isEmpty() || !device.doubled.isEmpty())
					result << QStringList{QStringLiteral("a strip is no connector")};
				result << summary(device);
			}
			return result;
		};
		const auto plain = run(0);
		QCOMPARE(plain.size(), 1);
		QCOMPARE(run(1), plain);
		QCOMPARE(run(2), plain);
	}

		//A part may name several pins: a connector symbol drawn as one terminal each
	void aPartWithSeveralPins()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(definePin(&project));
		DeviceType type;
		type.name = QStringLiteral("56P6");
		type.parts << DeviceType::Part{QStringLiteral("terminal"), QStringLiteral("terminal"), {},
			{{QStringLiteral("embed://import/pin.elmt"),
			  {QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("L3"), QStringLiteral("E")}, {}}}};
		type.parts << DeviceType::Part{QStringLiteral("terminal"), QStringLiteral("terminal"), {},
			{{QStringLiteral("embed://import/pin.elmt"), {QStringLiteral("C1")}, {}}}};
		project.setDeviceType(type);
		connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Plug, QStringLiteral("56P6"));
		draw(folio, QStringLiteral("X1"), {QStringLiteral("L1"), QStringLiteral("L2"), QStringLiteral("E")});
		auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QVERIFY(devices.first().doubled.isEmpty());
		QCOMPARE(devices.first().beyond(), 0);
		QCOMPARE(DeviceTypeCheck::freePins(devices.first()),
				 (QStringList{QStringLiteral("L3"), QStringLiteral("C1")}));
		draw(folio, QStringLiteral("X1"), {QStringLiteral("L3"), QStringLiteral("C1")});
		devices = DeviceTypeCheck::check(&project);
		QVERIFY(devices.first().complete());
	}

		//A type made for a plug drawn as a head and its pins has no part of
		//role terminal: its parts with named pins are the pins
	void aTypeWithoutTerminalParts()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(definePin(&project));
		DeviceType type;
		type.name = QStringLiteral("PLUG-3P+E-2C");
		for (const QStringList &pins : {QStringList{QStringLiteral("L1"), QStringLiteral("L2")},
										QStringList{QStringLiteral("C1")}, QStringList{QStringLiteral("C2")}})
			type.parts << DeviceType::Part{QStringLiteral("simple"), QStringLiteral("simple"), {},
				{{QStringLiteral("embed://import/pin.elmt"), pins, {}}}};
		project.setDeviceType(type);
		connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Plug, QStringLiteral("PLUG-3P+E-2C"));
		draw(folio, QStringLiteral("X1"), {QStringLiteral("L1"), QStringLiteral("C1")});
		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QCOMPARE(DeviceTypeCheck::freePins(devices.first()),
				 (QStringList{QStringLiteral("L2"), QStringLiteral("C2")}));
	}

		//The chooser offers the types for a connector: one with parts of role
		//terminal, and one drawn as a head with its pins that says so
	void theChooserOffersConnectorTypes()
	{
		QETProject project;
		project.setDeviceType(connectorType());
		DeviceType head;
		head.name = QStringLiteral("SOCKET-3P+E-2C");
		head.informations << qMakePair(QStringLiteral("connector"), QStringLiteral("socket"));
		head.parts << DeviceType::Part{QStringLiteral("simple"), QStringLiteral("simple"), {},
			{{QStringLiteral("embed://import/pin.elmt"), {QStringLiteral("L1")}, {}}}};
		project.setDeviceType(head);
		DeviceType relay;
		relay.name = QStringLiteral("RELAY");
		relay.parts << DeviceType::Part{QStringLiteral("coil"), QStringLiteral("master"), {},
			{{QStringLiteral("embed://import/pin.elmt"), {QStringLiteral("A1")}, {}}}};
		project.setDeviceType(relay);
		const QStringList offered = DeviceTypeChooser::connectorCandidates(&project, false);
		QVERIFY(offered.contains(QStringLiteral("56P6")));
		QVERIFY(offered.contains(QStringLiteral("SOCKET-3P+E-2C")));
		QVERIFY(!offered.contains(QStringLiteral("RELAY")));
		QVERIFY(DeviceTypeChooser::connectorCandidates(&project, true).contains(QStringLiteral("RELAY")));
	}

	void unknownTypeIsReported()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(definePin(&project));
		connector(project, QStringLiteral("X1"), TerminalStripData::Kind::Socket, QStringLiteral("NOSUCH"));
		draw(folio, QStringLiteral("X1"), {QStringLiteral("L1")});
		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QVERIFY(!devices.first().known());
		QCOMPARE(devices.first().typeName, QStringLiteral("NOSUCH"));
	}
};

QET_APP_TEST_MAIN(tst_connectordevicetype)
#include "tst_connectordevicetype.moc"
