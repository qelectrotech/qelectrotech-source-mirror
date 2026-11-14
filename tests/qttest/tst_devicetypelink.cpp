// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetype.h"
#include "devicetype/devicetypelink.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/masterelement.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QDomDocument>
#include <QTemporaryDir>
#include <QtTest>

/**
	Linking a contact to a device with a device type, when the project
	follows the device types (DeviceTypeLink): only a free part of the
	type, which the contact then takes.
*/
class tst_devicetypelink : public QObject
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

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}

	void settingIsSavedOnlyWhenOn()
	{
		QETProject project;
		project.addNewDiagram();
		QVERIFY(!project.followDeviceTypes());
		QVERIFY(!project.toXml().toString().contains(QStringLiteral("devicetype_rules")));

		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath(QStringLiteral("rules.qet"));
		project.setFollowDeviceTypes(true);
		QVERIFY(project.toXml().toString().contains(QStringLiteral("devicetype_rules")));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
		QETProject back(path);
		QVERIFY(back.followDeviceTypes());
	}

	void onlyAFreePartOfTheType()
	{
		Relay relay;
		auto *master = qobject_cast<MasterElement *>(relay.coil);
		QVERIFY(master);
		Element *one = place(relay.folio, relay.contact, QPointF(200, 100));
		Element *two = place(relay.folio, relay.contact, QPointF(260, 100));
		Element *three = place(relay.folio, relay.contact, QPointF(320, 100));
		QVERIFY(one && two && three);

			//Not followed: nothing limits, as before
		QVERIFY(!DeviceTypeLink::limits(relay.coil));
		QVERIFY(!DeviceTypeLink::refuses(relay.coil, one));
		QVERIFY(!master->isFull());

		relay.project.setFollowDeviceTypes(true);
		QVERIFY(DeviceTypeLink::limits(relay.coil));
		QCOMPARE(DeviceTypeLink::freeParts(relay.coil, one), QList<int>({1, 2}));
		relay.coil->linkToElement(one);
		relay.coil->linkToElement(two);
		QVERIFY(DeviceTypeLink::full(relay.coil));
		QVERIFY(master->isFull());
		QVERIFY(DeviceTypeLink::refuses(relay.coil, three));
			//A linked contact keeps its own part free for itself
		QVERIFY(!DeviceTypeLink::freeParts(relay.coil, one).isEmpty());

		relay.project.setFollowDeviceTypes(false);
		QVERIFY(!master->isFull());
		QVERIFY(!DeviceTypeLink::refuses(relay.coil, three));
	}

	void aLinkedContactTakesTheFreePartsTerminals()
	{
		Relay relay;
		relay.project.setFollowDeviceTypes(true);
		Element *one = place(relay.folio, relay.contact, QPointF(200, 100));
		QVERIFY(one);
		QCOMPARE(pins(one), QStringList({QString(), QString()}));
		relay.coil->linkToElement(one);
		QString error;
		Element *fresh = DeviceTypeLink::takeNextFreePart(one, &error);
		QVERIFY2(fresh, qPrintable(error));
		QCOMPARE(pins(fresh), QStringList({QStringLiteral("13"), QStringLiteral("14")}));
		QVERIFY(fresh->linkedElements().contains(relay.coil));

			//The next one takes the other part
		Element *two = place(relay.folio, relay.contact, QPointF(260, 100));
		relay.coil->linkToElement(two);
		Element *second = DeviceTypeLink::takeNextFreePart(two, &error);
		QVERIFY2(second, qPrintable(error));
		QCOMPARE(pins(second), QStringList({QStringLiteral("23"), QStringLiteral("24")}));

			//Taken again: nothing changes
		QCOMPARE(DeviceTypeLink::takeNextFreePart(fresh), fresh);
	}
};

QET_APP_TEST_MAIN(tst_devicetypelink)
#include "tst_devicetypelink.moc"
