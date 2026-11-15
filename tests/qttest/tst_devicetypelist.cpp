// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "cli_export.h"
#include "devicetype/devicetype.h"
#include "devicetype/devicetypecheck.h"
#include "devicetype/devicetypelist.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "qetresult.h"
#include "ui/devicetypesdialog.h"

#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
	The device types listed (DeviceTypeList: full and brief, from the
	Device types dialog and the command line), and the contacts the check
	of device types finds without their device or too many for it.
*/
class tst_devicetypelist : public QObject
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


	static QStringList lines(const QString &csv)
	{
		return csv.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}

	void briefAndFullLists()
	{
		Relay relay;
		DeviceType lamp;
		lamp.name = QStringLiteral("LAMP; RED");
		lamp.names.insert(QStringLiteral("en"), QStringLiteral("Red \"pilot\" lamp"));
		relay.project.setDeviceType(lamp);
		const QList<DeviceType> types = relay.project.deviceTypes();

		const QHash<QString, int> used{{QStringLiteral("RELAY-2NO"), 3}};
		QCOMPARE(lines(DeviceTypeList::csv(types, DeviceTypeList::Kind::Brief, &used)),
				 QStringList({QStringLiteral("device_type;description;parts;symbols"),
							  QStringLiteral("\"LAMP; RED\";\"Red \"\"pilot\"\" lamp\";0;0"),
							  QStringLiteral("RELAY-2NO;;3;3")}));
		QCOMPARE(lines(DeviceTypeList::csv(types, DeviceTypeList::Kind::Brief)).first(),
				 QStringLiteral("device_type;description;parts"));

		const QStringList full = lines(DeviceTypeList::csv(types, DeviceTypeList::Kind::Full));
		QCOMPARE(full.size(), 1 + 1 + 3);
		QCOMPARE(full.first(), QStringLiteral("device_type;description;part;role;kind;state;symbol;pins"));
		QVERIFY(full.at(2).startsWith(QStringLiteral("RELAY-2NO;;1;master;coil;;embed://import/coil.elmt;A1 A2")));
		QCOMPARE(full.at(3), QStringLiteral("RELAY-2NO;;2;slave;simple;NO;") + relay.contact
				 + QStringLiteral(";13 14"));
	}

	void theDialogListsItsTab()
	{
		Relay relay;
		DeviceTypesDialog dialog(&relay.project);
		const QStringList brief = lines(dialog.listCsv(DeviceTypeList::Kind::Brief));
		QCOMPARE(brief.first(), QStringLiteral("device_type;description;parts;symbols"));
			//The coil names its type
		QCOMPARE(brief.at(1), QStringLiteral("RELAY-2NO;;3;1"));
	}

	void theCommandLineWritesTheLists()
	{
		Relay relay;
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath(QStringLiteral("relay.qet"));
		relay.project.setFilePath(path);
		QVERIFY(relay.project.write().isOk());

		const QString brief = dir.filePath(QStringLiteral("brief.csv"));
		const QString full = dir.filePath(QStringLiteral("full.csv"));
		QCOMPARE(CLIExport::run({QStringLiteral("--export-device-types-brief"), path, brief}), 0);
		QCOMPARE(CLIExport::run({QStringLiteral("--export-device-types"), path, full}), 0);
		QFile brief_file(brief), full_file(full);
			//Written as text: on Windows their lines end in \r\n
		QVERIFY(brief_file.open(QIODevice::ReadOnly | QIODevice::Text));
		QVERIFY(full_file.open(QIODevice::ReadOnly | QIODevice::Text));
		QCOMPARE(lines(QString::fromUtf8(brief_file.readAll())).value(1), QStringLiteral("RELAY-2NO;;3;1"));
		QCOMPARE(lines(QString::fromUtf8(full_file.readAll())).size(), 4);
	}

	void contactsWithoutTheirDeviceOrTooMany()
	{
		Relay relay;
		Element *one = place(relay.folio, relay.contact, QPointF(200, 100));
		Element *two = place(relay.folio, relay.contact, QPointF(260, 100));
		Element *three = place(relay.folio, relay.contact, QPointF(320, 100));
		Element *loose = place(relay.folio, relay.contact, QPointF(380, 100));
		QVERIFY(one && two && three && loose);

			//Not followed: only a loose contact naming a type is listed
		QVERIFY(DeviceTypeCheck::unlinkedContacts(&relay.project).isEmpty());
		DiagramContext info = loose->elementInformations();
		info.addValue(QStringLiteral("device_type"), QStringLiteral("RELAY-2NO"));
		loose->setElementInformations(info);
		QCOMPARE(DeviceTypeCheck::unlinkedContacts(&relay.project), QList<Element *>({loose}));
			//Followed: every contact linked to no master
		relay.project.setFollowDeviceTypes(true);
		QCOMPARE(DeviceTypeCheck::unlinkedContacts(&relay.project).size(), 4);

		relay.coil->linkToElement(one);
		relay.coil->linkToElement(two);
		QCOMPARE(DeviceTypeCheck::extraContacts(DeviceTypeCheck::checkDevice(relay.coil)), 0);
		relay.project.setFollowDeviceTypes(false);
		relay.coil->linkToElement(three);
		QCOMPARE(DeviceTypeCheck::extraContacts(DeviceTypeCheck::checkDevice(relay.coil)), 1);
		QCOMPARE(DeviceTypeCheck::unlinkedContacts(&relay.project), QList<Element *>({loose}));
	}
};

QET_APP_TEST_MAIN(tst_devicetypelist)
#include "tst_devicetypelist.moc"
