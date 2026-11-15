// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetype.h"
#include "devicetype/devicetypechange.h"
#include "devicetype/devicetypecheck.h"
#include "devicetype/devicetypelink.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "ui/devicetypechangedialog.h"

#include <QDomDocument>
#include <QUndoStack>
#include <QtTest>

/**
	Giving a device another device type (DeviceTypeChange): each symbol
	takes a part of the new type, matched automatically or chosen, and
	the whole change is one undo step.
*/
class tst_devicetypechange : public QObject
{
	Q_OBJECT

	static QString define(QETProject &project, const QString &name, const QString &link_type,
						  const QStringList &names)
	{
		QString lines;
		for (int i = 0 ; i < names.size() ; ++i)
			lines += QStringLiteral("<terminal x=\"%1\" y=\"20\" orientation=\"s\" uuid=\"%2\" name=\"%3\"/>")
					.arg(i * 20).arg(QUuid::createUuid().toString()).arg(names.at(i));
		const QString kinds = link_type == QLatin1String("slave")
				? QStringLiteral("<kindInformations><kindInformation name=\"type\">simple</kindInformation>"
								 "<kindInformation name=\"state\">NO</kindInformation></kindInformations>")
				: QStringLiteral("<kindInformations><kindInformation name=\"type\">coil</kindInformation></kindInformations>");
		QDomDocument document;
		document.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"%1\" width=\"80\" height=\"60\" hotspot_x=\"10\" hotspot_y=\"20\">"
			"<uuid uuid=\"%2\"/><names><name lang=\"en\">%3</name></names>%4"
			"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/>%5</description></definition>")
				.arg(link_type, QUuid::createUuid().toString(), name, kinds, lines));
		project.embeddedElementCollection()->addElementDefinition(QStringLiteral("import"), name, document.documentElement());
		return QStringLiteral("embed://import/") + name + QStringLiteral(".elmt");
	}

	static Element *place(Diagram *folio, const QString &location, QPointF pos)
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
					ElementsLocation(location, folio->project()), nullptr, &state);
		if (state || !element) return nullptr;
		folio->addItem(element);
		element->setPos(pos);
		return element;
	}

	static QStringList pins(Element *element)
	{
		QStringList names;
		for (Terminal *terminal : element->terminals()) names << terminal->baseName();
		return names;
	}

		//A relay: its coil, and two NO contacts 13 14 and 23 24
	struct Relay
	{
		QETProject project;
		Diagram *folio = nullptr;
		Element *coil = nullptr;
		QString contact;

		Relay()
		{
			folio = project.addNewDiagram();
			const QString coil_location = define(project, QStringLiteral("coil"), QStringLiteral("master"),
												 {QStringLiteral("A1"), QStringLiteral("A2")});
			contact = define(project, QStringLiteral("contact"), QStringLiteral("slave"),
							 {QString(), QString()});
			DeviceType type;
			type.name = QStringLiteral("RELAY-2NO");
			DeviceType::Part head{QStringLiteral("master"), QStringLiteral("coil"), QString(),
				{{coil_location, {QStringLiteral("A1"), QStringLiteral("A2")}, {}}}};
			DeviceType::Part first{QStringLiteral("slave"), QStringLiteral("simple"), QStringLiteral("NO"),
				{{contact, {QStringLiteral("13"), QStringLiteral("14")}, {}}}};
			DeviceType::Part second{QStringLiteral("slave"), QStringLiteral("simple"), QStringLiteral("NO"),
				{{contact, {QStringLiteral("23"), QStringLiteral("24")}, {}}}};
			type.parts = {head, first, second};
			project.setDeviceType(type);
			coil = place(folio, coil_location, QPointF(100, 100));
			DiagramContext info = coil->elementInformations();
			info.addValue(QStringLiteral("label"), QStringLiteral("K1"));
			info.addValue(QStringLiteral("device_type"), type.name);
			coil->setElementInformations(info);
		}
	};


		//The bigger type: its second NO contact has other terminals
	static DeviceType bigger(const Relay &relay, const QString &coil_location)
	{
		DeviceType type;
		type.name = QStringLiteral("RELAY-3");
		type.parts << DeviceType::Part{QStringLiteral("master"), QStringLiteral("coil"), QString(),
			{{coil_location, {QStringLiteral("A1"), QStringLiteral("A2")}, {}}}};
		type.parts << DeviceType::Part{QStringLiteral("slave"), QStringLiteral("simple"), QStringLiteral("NC"),
			{{relay.contact, {QStringLiteral("21"), QStringLiteral("22")}, {}}}};
		type.parts << DeviceType::Part{QStringLiteral("slave"), QStringLiteral("simple"), QStringLiteral("NO"),
			{{relay.contact, {QStringLiteral("13"), QStringLiteral("14")}, {}}}};
		type.parts << DeviceType::Part{QStringLiteral("slave"), QStringLiteral("simple"), QStringLiteral("NO"),
			{{relay.contact, {QStringLiteral("43"), QStringLiteral("44")}, {}}}};
		return type;
	}

	static QList<QStringList> contactPins(Diagram *folio)
	{
		QList<QStringList> list;
		for (Element *element : folio->elements())
			if (element->linkType() & Element::Slave) list << pins(element);
		std::sort(list.begin(), list.end());
		return list;
	}

	static QString typeOf(Element *element)
	{
		return element->elementInformations().value(QStringLiteral("device_type")).toString();
	}

		//The relay with its two contacts linked, drawing 13 14 and 23 24
	static void linkTwo(Relay &relay)
	{
		relay.project.setFollowDeviceTypes(true);
		for (int i = 0 ; i < 2 ; ++i) {
			Element *contact = place(relay.folio, relay.contact, QPointF(200 + 60 * i, 100));
			QVERIFY(contact);
			relay.coil->linkToElement(contact);
			QString error;
			QVERIFY2(DeviceTypeLink::takeNextFreePart(contact, &error), qPrintable(error));
		}
		relay.folio->undoStack().clear();
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}

	void theSymbolsTakeAlikeParts()
	{
		Relay relay;
		linkTwo(relay);
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(relay.coil);
		QVERIFY(device.known());
		QCOMPARE(device.symbols.size(), 3);
		const DeviceType type = bigger(relay, relay.project.deviceType(QStringLiteral("RELAY-2NO"))
										   .parts.at(0).symbols.at(0).location);
			//The coil the coil, each NO contact a free NO part, never the NC one
		QCOMPARE(DeviceTypeChange::automatic(device, type), QList<int>({0, 2, 3}));
			//A smaller type: what has no part left is beyond it
		DeviceType smaller = type;
		smaller.parts.removeLast();
		QCOMPARE(DeviceTypeChange::automatic(device, smaller),
				 QList<int>({0, 2, int(DeviceTypeChange::Beyond)}));
	}

	void changingIsOneUndoStep()
	{
		Relay relay;
		linkTwo(relay);
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(relay.coil);
		const DeviceType type = bigger(relay, relay.project.deviceType(QStringLiteral("RELAY-2NO"))
										   .parts.at(0).symbols.at(0).location);
		QVERIFY(relay.project.deviceType(type.name).isNull());
		const int steps = relay.folio->undoStack().count();
		QString error;
		QVERIFY2(DeviceTypeChange::apply(device, type, DeviceTypeChange::automatic(device, type), &error),
				 qPrintable(error));
		QCOMPARE(relay.folio->undoStack().count(), steps + 1);
		QCOMPARE(relay.project.deviceType(type.name), type);
		QCOMPARE(typeOf(relay.coil), type.name);
		QCOMPARE(contactPins(relay.folio),
				 QList<QStringList>({{QStringLiteral("13"), QStringLiteral("14")},
									 {QStringLiteral("43"), QStringLiteral("44")}}));
		for (Element *element : relay.folio->elements()) QCOMPARE(typeOf(element), type.name);
		QVERIFY(DeviceTypeCheck::checkDevice(relay.coil).beyond() == 0);

		relay.folio->undoStack().undo();
		QVERIFY(relay.project.deviceType(type.name).isNull());
		QCOMPARE(typeOf(relay.coil), QStringLiteral("RELAY-2NO"));
		QCOMPARE(contactPins(relay.folio),
				 QList<QStringList>({{QStringLiteral("13"), QStringLiteral("14")},
									 {QStringLiteral("23"), QStringLiteral("24")}}));
	}

	void eachSymbolItsChosenPart()
	{
		Relay relay;
		linkTwo(relay);
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(relay.coil);
		const DeviceType type = bigger(relay, relay.project.deviceType(QStringLiteral("RELAY-2NO"))
										   .parts.at(0).symbols.at(0).location);
		DeviceTypeChangeDialog dialog(device, type, DeviceTypeChange::automatic(device, type));
		QCOMPARE(dialog.parts(), QList<int>({0, 2, 3}));
			//The second contact left as it is, the first one the other NO part
		dialog.setPart(1, 3);
		dialog.setPart(2, DeviceTypeChange::Beyond);
		QCOMPARE(dialog.parts(), QList<int>({0, 3, int(DeviceTypeChange::Beyond)}));
		QString error;
		QVERIFY2(DeviceTypeChange::apply(device, type, dialog.parts(), &error), qPrintable(error));
		QCOMPARE(contactPins(relay.folio),
				 QList<QStringList>({{QStringLiteral("23"), QStringLiteral("24")},
									 {QStringLiteral("43"), QStringLiteral("44")}}));
	}
};

QET_APP_TEST_MAIN(tst_devicetypechange)
#include "tst_devicetypechange.moc"
