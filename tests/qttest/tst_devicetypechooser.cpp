// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetype.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/devicetypechooser.h"

#include <QDomDocument>
#include <QUndoCommand>
#include <QUndoStack>
#include <QtTest>

/**
	Which device type is a symbol's device (DeviceTypeChooser): the types
	whose head the symbol can draw, the ones chosen lately first.
*/
class tst_devicetypechooser : public QObject
{
	Q_OBJECT

	static QString define(QETProject &project, const QString &name)
	{
		QDomDocument document;
		document.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"master\" width=\"80\" height=\"60\" hotspot_x=\"10\" hotspot_y=\"20\">"
			"<uuid uuid=\"%1\"/><names><name lang=\"en\">%2</name></names>"
			"<kindInformations><kindInformation name=\"type\">coil</kindInformation></kindInformations>"
			"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/>"
			"<terminal x=\"0\" y=\"20\" orientation=\"s\" uuid=\"%3\" name=\"A1\"/></description></definition>")
				.arg(QUuid::createUuid().toString(), name, QUuid::createUuid().toString()));
		project.embeddedElementCollection()->addElementDefinition(QStringLiteral("import"), name, document.documentElement());
		return QStringLiteral("embed://import/") + name + QStringLiteral(".elmt");
	}

	static DeviceType headedBy(const QString &name, const QString &location)
	{
		DeviceType type;
		type.name = name;
		type.names.insert(QStringLiteral("en"), name + QStringLiteral(" relay"));
		type.parts << DeviceType::Part{QStringLiteral("master"), QStringLiteral("coil"), QString(),
									   {{location, {QStringLiteral("A1")}, {}}}};
		return type;
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}

	void offersTheTypesItsSymbolDraws()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
		const QString lamp = define(project, QStringLiteral("lamp"));
		project.setDeviceType(headedBy(QStringLiteral("RELAY-A"), coil));
		project.setDeviceType(headedBy(QStringLiteral("LAMP-B"), lamp));
		int state = 0;
		Element *symbol = ElementFactory::Instance()->createElement(ElementsLocation(coil, &project), nullptr, &state);
		QVERIFY(symbol && !state);
		folio->addItem(symbol);

		QCOMPARE(DeviceTypeChooser::candidates(symbol, false), QStringList({QStringLiteral("RELAY-A")}));
		QCOMPARE(DeviceTypeChooser::candidates(symbol, true),
				 QStringList({QStringLiteral("LAMP-B"), QStringLiteral("RELAY-A")}));
			//The ones chosen lately first
		DeviceTypeChooser::remember(QStringLiteral("RELAY-A"));
		QCOMPARE(DeviceTypeChooser::candidates(symbol, true).first(), QStringLiteral("RELAY-A"));

		DeviceTypeChooser dialog(symbol);
		QCOMPARE(dialog.shown(), QStringList({QStringLiteral("RELAY-A")}));
		dialog.setShowAll(true);
		QCOMPARE(dialog.shown().size(), 2);
		dialog.setFilter(QStringLiteral("lamp"));
		QCOMPARE(dialog.shown(), QStringList({QStringLiteral("LAMP-B")}));
		QCOMPARE(dialog.chosen(), QStringLiteral("LAMP-B"));
	}

	void aCollectionSymbolImportedIntoTheProject()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
			//The type names the collection's symbol, the folio its copy
			//in the project (embed://import/coil.elmt)
		project.setDeviceType(headedBy(QStringLiteral("RELAY-C"), QStringLiteral("common://coil.elmt")));
		project.setDeviceType(headedBy(QStringLiteral("RELAY-D"), QStringLiteral("common://other/lamp.elmt")));
		int state = 0;
		Element *symbol = ElementFactory::Instance()->createElement(ElementsLocation(coil, &project), nullptr, &state);
		QVERIFY(symbol && !state);
		folio->addItem(symbol);
		QCOMPARE(DeviceTypeChooser::candidates(symbol, false), QStringList({QStringLiteral("RELAY-C")}));
	}

	void givingATypeIsOneUndoStep()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("coil"));
		const DeviceType library_type = headedBy(QStringLiteral("LIBRARY-RELAY"), coil);
		QVERIFY(DeviceTypeLibrary::save(library_type));
		int state = 0;
		Element *symbol = ElementFactory::Instance()->createElement(ElementsLocation(coil, &project), nullptr, &state);
		QVERIFY(symbol && !state);
		folio->addItem(symbol);

		QUndoCommand *undo = DeviceTypeChooser::give(symbol, library_type.name);
		QVERIFY(undo);
		folio->undoStack().push(undo);
		QCOMPARE(symbol->elementInformations().value(QStringLiteral("device_type")).toString(), library_type.name);
		QCOMPARE(project.deviceType(library_type.name), library_type);
		QVERIFY(!DeviceTypeChooser::give(symbol, library_type.name));

		folio->undoStack().undo();
		QVERIFY(symbol->elementInformations().value(QStringLiteral("device_type")).toString().isEmpty());
		QVERIFY(project.deviceType(library_type.name).isNull());
		QVERIFY(DeviceTypeLibrary::remove(library_type.name));
	}
};

QET_APP_TEST_MAIN(tst_devicetypechooser)
#include "tst_devicetypechooser.moc"
