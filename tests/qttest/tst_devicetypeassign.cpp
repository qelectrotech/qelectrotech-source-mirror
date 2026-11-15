// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QDomDocument>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetypeassign.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetapp.h"
#include "qetproject.h"
#include "qetresult.h"
#include "ui/elementarticleswidget.h"
#include "ui/elementinfopartwidget.h"

// A placed symbol takes a part of its device type: the part's terminals, on
// a project copy of its own drawing or on the type's, keeping its wires.
class tst_devicetypeassign : public QObject
{
	Q_OBJECT

	static QString define(QETProject &project, const QString &name, const QString &link_type,
						  int terminals, const QStringList &names = {})
	{
		QString lines;
		for (int i = 0 ; i < terminals ; ++i)
			lines += QStringLiteral("<terminal x=\"%1\" y=\"20\" orientation=\"s\" uuid=\"%2\" name=\"%3\"/>")
					.arg(i * 20).arg(QUuid::createUuid().toString()).arg(names.value(i));
		const QString kinds = link_type == QLatin1String("slave")
				? QStringLiteral("<kindInformations><kindInformation name=\"type\">simple</kindInformation>"
								 "<kindInformation name=\"state\">NO</kindInformation></kindInformations>")
				: link_type == QLatin1String("master")
				  ? QStringLiteral("<kindInformations><kindInformation name=\"type\">coil</kindInformation></kindInformations>")
				  : QString();
		QDomDocument document;
		document.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"%1\" width=\"80\" height=\"60\" hotspot_x=\"10\" hotspot_y=\"20\">"
			"<uuid uuid=\"%2\"/><names><name lang=\"en\">%3</name></names>%4"
			"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/>%5</description></definition>")
				.arg(link_type, QUuid::createUuid().toString(), name, kinds, lines));
		project.embeddedElementCollection()->addElementDefinition(QStringLiteral("import"), name, document.documentElement());
		return QStringLiteral("embed://import/") + name + QStringLiteral(".elmt");
	}

	static Element *place(Diagram *folio, const QString &location, const QString &label, QPointF pos)
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
					ElementsLocation(location, folio->project()), nullptr, &state);
		if (state || !element) return nullptr;
		folio->addItem(element);
		element->setPos(pos);
		if (!label.isEmpty()) {
			DiagramContext info = element->elementInformations();
			info.addValue(QStringLiteral("label"), label);
			element->setElementInformations(info);
		}
		return element;
	}

	static DeviceType contactor(const QString &coil_location = QString())
	{
		DeviceType type;
		type.name = QStringLiteral("CONTR");
		type.names.insert(QStringLiteral("en"), QStringLiteral("Contactor"));
		type.informations << qMakePair(QStringLiteral("model"), QStringLiteral("CONTR"));
		DeviceType::Part coil;
		coil.role = QStringLiteral("master");
		coil.kind = QStringLiteral("coil");
		DeviceType::Symbol a;
		a.location = coil_location;
		a.pins = QStringList{QStringLiteral("A1"), QStringLiteral("A2")};
		coil.symbols << a;
		DeviceType::Part contact;
		contact.role = QStringLiteral("slave");
		contact.kind = QStringLiteral("simple");
		DeviceType::Symbol no;
		no.pins = QStringList{QStringLiteral("13"), QStringLiteral("14")};
		no.state = QStringLiteral("NO");
		DeviceType::Symbol nc;
		nc.pins = QStringList{QStringLiteral("21"), QStringLiteral("22")};
		nc.state = QStringLiteral("NC");
		contact.symbols << no << nc;
		type.parts << coil << contact;
		return type;
	}

	static QStringList pins(Element *element)
	{
		QStringList names;
		for (Terminal *terminal : element->terminals()) names << terminal->baseName();
		return names;
	}

	static DeviceTypeAssign::Choice own(const QList<DeviceTypeAssign::Choice> &choices, int part, int way)
	{
		for (const auto &choice : choices)
			if (choice.part == part && choice.way == way && choice.own_drawing) return choice;
		return DeviceTypeAssign::Choice();
	}

private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
		QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech-tests"));
		QCoreApplication::setApplicationName(QStringLiteral("tst_devicetypeassign"));
	}

	void ownDrawingTakesThePartsPins()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString drawing = define(project, QStringLiteral("plain"), QStringLiteral("simple"), 2);
		Element *symbol = place(folio, drawing, QStringLiteral("K1"), QPointF(100, 100));
		QVERIFY(symbol);
		const DeviceType type = contactor();
		project.setDeviceType(type);

		const auto device = DeviceTypeAssign::deviceOf(symbol, type);
		const auto choice = own(DeviceTypeAssign::choices(symbol, device), 0, 0);
		QVERIFY(choice.possible);
		QString error;
		Element *fresh = DeviceTypeAssign::apply(symbol, device, choice, &error);
		QVERIFY2(fresh, qPrintable(error));
		QVERIFY(fresh != symbol);
		QCOMPARE(pins(fresh), QStringList({QStringLiteral("A1"), QStringLiteral("A2")}));
		QCOMPARE(fresh->pos(), QPointF(100, 100));
		QCOMPARE(fresh->elementInformations().value(QStringLiteral("label")).toString(), QStringLiteral("K1"));
		QCOMPARE(fresh->elementInformations().value(QStringLiteral("model")).toString(), QStringLiteral("CONTR"));
		QCOMPARE(fresh->elementInformations().value(QStringLiteral("device_type")).toString(), type.name);
		QCOMPARE(fresh->linkType(), Element::Master);
		QCOMPARE(folio->elements().size(), 1);
			//The drawing it came from is untouched
		QVERIFY(ElementsLocation(drawing, &project).exist());
		QVERIFY(fresh->location() != ElementsLocation(drawing, &project));
	}

	void keepsItsWiresAndUndoes()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString drawing = define(project, QStringLiteral("plain"), QStringLiteral("simple"), 2);
		const QString other = define(project, QStringLiteral("other"), QStringLiteral("simple"), 2);
		Element *symbol = place(folio, drawing, QStringLiteral("K1"), QPointF(100, 100));
		Element *neighbour = place(folio, other, QStringLiteral("X1"), QPointF(100, 300));
		QVERIFY(symbol && neighbour);
		auto *wire = new Conductor(symbol->terminals().at(1), neighbour->terminals().at(0));
		folio->addItem(wire);
		ConductorProperties properties = wire->properties();
		properties.text = QStringLiteral("W42");
		wire->setProperties(properties);
		const DeviceType type = contactor();
		project.setDeviceType(type);

		const auto device = DeviceTypeAssign::deviceOf(symbol, type);
		Element *fresh = DeviceTypeAssign::apply(symbol, device, own(DeviceTypeAssign::choices(symbol, device), 0, 0));
		QVERIFY(fresh && fresh != symbol);
		QCOMPARE(folio->elements().size(), 2);
		QCOMPARE(folio->conductors().size(), 1);
		Conductor *moved = *folio->conductors().begin();
		QCOMPARE(moved->properties().text, QStringLiteral("W42"));
		QVERIFY(moved->terminal1 == fresh->terminals().at(1) || moved->terminal2 == fresh->terminals().at(1));
		QVERIFY(moved->terminal1 == neighbour->terminals().at(0) || moved->terminal2 == neighbour->terminals().at(0));

		folio->undoStack().undo();
		QCOMPARE(folio->elements().size(), 2);
		QVERIFY(folio->elements().contains(symbol));
		QCOMPARE(folio->conductors().size(), 1);
		QCOMPARE((*folio->conductors().begin())->properties().text, QStringLiteral("W42"));

		folio->undoStack().redo();
		QVERIFY(folio->elements().contains(fresh));
		QVERIFY(!folio->elements().contains(symbol));
	}

	void aTerminalWithSeveralWiresKeepsThemAll()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString drawing = define(project, QStringLiteral("plain"), QStringLiteral("simple"), 2);
		const QString other = define(project, QStringLiteral("other"), QStringLiteral("simple"), 2);
		Element *symbol = place(folio, drawing, QStringLiteral("K1"), QPointF(100, 100));
		Element *one = place(folio, other, QStringLiteral("X1"), QPointF(100, 300));
		Element *two = place(folio, other, QStringLiteral("X2"), QPointF(200, 300));
		folio->addItem(new Conductor(symbol->terminals().at(0), one->terminals().at(0)));
		folio->addItem(new Conductor(symbol->terminals().at(0), two->terminals().at(0)));
		const DeviceType type = contactor();
		project.setDeviceType(type);
		const auto device = DeviceTypeAssign::deviceOf(symbol, type);
		Element *fresh = DeviceTypeAssign::apply(symbol, device, own(DeviceTypeAssign::choices(symbol, device), 0, 0));
		QVERIFY(fresh);
		QCOMPARE(folio->conductors().size(), 2);
		QCOMPARE(fresh->terminals().at(0)->conductors().size(), 2);
		folio->undoStack().undo();
		QCOMPARE(folio->conductors().size(), 2);
		QCOMPARE(symbol->terminals().at(0)->conductors().size(), 2);
	}

	void aDrawingOfTheUsersCollectionIsCopiedIntoTheProject()
	{
		QTemporaryDir collection;
		QVERIFY(collection.isValid());
		QSettings().setValue(QStringLiteral("elements-collections/custom-collection-path"), collection.path());
		QETApp::resetCollectionsPath();
		const QString folder = QDir(collection.path()).filePath(QStringLiteral("tst_assign"));
		QDir().mkpath(folder);
		const QString file = QDir(folder).filePath(QStringLiteral("LIBCOIL.elmt"));
		QFile elmt(file);
		QVERIFY(elmt.open(QIODevice::WriteOnly));
		elmt.write(QStringLiteral(
			"<definition type=\"element\" link_type=\"simple\" width=\"60\" height=\"40\" hotspot_x=\"10\" hotspot_y=\"20\">"
			"<uuid uuid=\"%1\"/><names><name lang=\"en\">LIBCOIL</name></names>"
			"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/>"
			"<terminal x=\"0\" y=\"20\" orientation=\"s\" uuid=\"%2\" name=\"\"/>"
			"<terminal x=\"20\" y=\"20\" orientation=\"s\" uuid=\"%3\" name=\"\"/>"
			"</description></definition>")
				   .arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(), QUuid::createUuid().toString()).toUtf8());
		elmt.close();

		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *symbol = place(folio, define(project, QStringLiteral("plain"), QStringLiteral("simple"), 2),
								QStringLiteral("K1"), QPointF(100, 100));
		const DeviceType type = contactor(QStringLiteral("custom://tst_assign/LIBCOIL.elmt"));
		project.setDeviceType(type);
		const auto device = DeviceTypeAssign::deviceOf(symbol, type);
		DeviceTypeAssign::Choice theirs;
		for (const auto &choice : DeviceTypeAssign::choices(symbol, device))
			if (choice.part == 0 && !choice.own_drawing) theirs = choice;
		QVERIFY2(theirs.part == 0, "the drawing of the collection is not offered");
		QString error;
		Element *fresh = DeviceTypeAssign::apply(symbol, device, theirs, &error);
		QVERIFY2(fresh, qPrintable(error));
		QVERIFY(fresh->location().isProject());
		QCOMPARE(pins(fresh), QStringList({QStringLiteral("A1"), QStringLiteral("A2")}));
		QCOMPARE(fresh->linkType(), Element::Master);
		QSettings().remove(QStringLiteral("elements-collections/custom-collection-path"));
		QETApp::resetCollectionsPath();
	}

	void survivesSavingAndOpeningAgain()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath(QStringLiteral("assigned.qet"));
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			const QString drawing = define(project, QStringLiteral("plain"), QStringLiteral("simple"), 2);
			Element *symbol = place(folio, drawing, QStringLiteral("K1"), QPointF(100, 100));
			Element *neighbour = place(folio, define(project, QStringLiteral("other"), QStringLiteral("simple"), 2),
									   QStringLiteral("X1"), QPointF(100, 300));
			folio->addItem(new Conductor(symbol->terminals().at(0), neighbour->terminals().at(0)));
			const DeviceType type = contactor();
			project.setDeviceType(type);
			const auto device = DeviceTypeAssign::deviceOf(symbol, type);
			QVERIFY(DeviceTypeAssign::apply(symbol, device, own(DeviceTypeAssign::choices(symbol, device), 0, 0)));
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		again.loadFolios();
		Diagram *folio = again.diagrams().first();
		Element *k1 = nullptr;
		for (Element *element : folio->elements())
			if (element->elementInformations().value(QStringLiteral("label")).toString() == QLatin1String("K1"))
				k1 = element;
		QVERIFY(k1);
		QCOMPARE(pins(k1), QStringList({QStringLiteral("A1"), QStringLiteral("A2")}));
		QCOMPARE(k1->conductors().size(), 1);
		QCOMPARE(k1->linkType(), Element::Master);
	}

	void thePanelOffersItOnceTheTypeIsKnown()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *symbol = place(folio, define(project, QStringLiteral("plain"), QStringLiteral("simple"), 2),
								QStringLiteral("K1"), QPointF(100, 100));
		const DeviceType type = contactor();
		project.setDeviceType(type);
		DiagramContext info = symbol->elementInformations();
		info.addValue(QStringLiteral("device_type"), type.name);
		symbol->setElementInformations(info);

		ElementArticlesWidget with_type(symbol);
		with_type.updateUi();
		auto *button = with_type.findChild<QPushButton *>(QStringLiteral("deviceTypeSymbolButton"));
		QVERIFY(button);
		QVERIFY(!button->isHidden());

		info.addValue(QStringLiteral("device_type"), QStringLiteral("NO SUCH TYPE"));
		symbol->setElementInformations(info);
		ElementArticlesWidget without(symbol);
		without.updateUi();
		QVERIFY(without.findChild<QPushButton *>(QStringLiteral("deviceTypeSymbolButton"))->isHidden());
	}

		//A Device type field naming a type of the user's library (a part
		//picked from the material file may set it) brings the type into
		//the project in the same undo step as the information
	void aTypeOfTheLibraryComesIntoTheProject()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *symbol = place(folio, define(project, QStringLiteral("coil"), QStringLiteral("master"), 2),
								QStringLiteral("K1"), QPointF(100, 100));
		DeviceType type = contactor();
		type.name = QStringLiteral("LIBRARY CONTACTOR");
		QVERIFY(DeviceTypeLibrary::save(type));
		QVERIFY(project.deviceType(type.name).isNull());

		ElementArticlesWidget widget(symbol);
		widget.updateUi();
		ElementInfoPartWidget *model = nullptr;
		for (ElementInfoPartWidget *row : widget.findChildren<ElementInfoPartWidget *>())
			if (row->key() == QLatin1String("device_type")) model = row;
		QVERIFY(model);
		model->setText(type.name);
		QUndoCommand *undo = widget.associatedUndo();
		QVERIFY(undo);
		project.undoStack()->push(undo);
		QCOMPARE(symbol->elementInformations().value(QStringLiteral("device_type")).toString(), type.name);
		QVERIFY(!project.deviceType(type.name).isNull());

		project.undoStack()->undo();
		QVERIFY(project.deviceType(type.name).isNull());
		QVERIFY(symbol->elementInformations().value(QStringLiteral("device_type")).toString().isEmpty());

			//A device type set before, whose type never reached the project:
			//the next apply brings it, with no field changed
		DiagramContext info = symbol->elementInformations();
		info.addValue(QStringLiteral("device_type"), type.name);
		symbol->setElementInformations(info);
		ElementArticlesWidget again(symbol);
		again.updateUi();
		QUndoCommand *bring = again.associatedUndo();
		QVERIFY(bring);
		project.undoStack()->push(bring);
		QVERIFY(!project.deviceType(type.name).isNull());
		QVERIFY(DeviceTypeLibrary::remove(type.name));
	}

	void terminalsWhichDoNotFitAreNotOffered()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *symbol = place(folio, define(project, QStringLiteral("three"), QStringLiteral("simple"), 3),
								QStringLiteral("K1"), QPointF(100, 100));
		QVERIFY(symbol);
		const DeviceType type = contactor();
		const auto device = DeviceTypeAssign::deviceOf(symbol, type);
		const auto choices = DeviceTypeAssign::choices(symbol, device);
		QVERIFY(!own(choices, 0, 0).possible);
		QVERIFY(!own(choices, 0, 0).reason.isEmpty());
	}

	void aWiredSymbolKeepsItsWiresOrIsLeftAlone()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString two = define(project, QStringLiteral("plain"), QStringLiteral("simple"), 2);
		const QString other = define(project, QStringLiteral("other"), QStringLiteral("simple"), 2);
		const QString four = define(project, QStringLiteral("fourway"), QStringLiteral("simple"), 4,
									{QStringLiteral("A1"), QStringLiteral("A2"), QStringLiteral("X1"), QStringLiteral("X2")});
		Element *symbol = place(folio, two, QStringLiteral("K1"), QPointF(100, 100));
		Element *neighbour = place(folio, other, QStringLiteral("X1"), QPointF(100, 300));
		folio->addItem(new Conductor(symbol->terminals().at(0), neighbour->terminals().at(0)));
		DeviceType type = contactor(four);
		project.setDeviceType(type);

		const auto device = DeviceTypeAssign::deviceOf(symbol, type);
		const auto choices = DeviceTypeAssign::choices(symbol, device);
		DeviceTypeAssign::Choice theirs;
		for (const auto &choice : choices)
			if (choice.part == 0 && !choice.own_drawing) theirs = choice;
		QVERIFY(theirs.part == 0);
		QString error;
		const int steps = folio->undoStack().count();
		QVERIFY(!DeviceTypeAssign::apply(symbol, device, theirs, &error));
		QVERIFY(!error.isEmpty());
		QVERIFY(folio->elements().contains(symbol));
		QCOMPARE(folio->conductors().size(), 1);
		QCOMPARE(folio->undoStack().count(), steps);
	}

	void aContactChoosesNoOrNcAndLinksToItsCoil()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil_drawing = define(project, QStringLiteral("coil"), QStringLiteral("master"), 2,
											{QStringLiteral("A1"), QStringLiteral("A2")});
		const QString contact_drawing = define(project, QStringLiteral("contact"), QStringLiteral("simple"), 2);
		Element *coil = place(folio, coil_drawing, QStringLiteral("K1"), QPointF(100, 100));
		Element *contact = place(folio, contact_drawing, QStringLiteral("K1"), QPointF(300, 100));
		QVERIFY(coil && contact);
		DiagramContext info = coil->elementInformations();
		info.addValue(QStringLiteral("device_type"), QStringLiteral("CONTR"));
		coil->setElementInformations(info);
		const DeviceType type = contactor(coil_drawing);
		project.setDeviceType(type);

		const auto device = DeviceTypeAssign::deviceOf(contact, type);
		const auto choices = DeviceTypeAssign::choices(contact, device);
		const auto nc = own(choices, 1, 1);
		QVERIFY(nc.possible);
		QVERIFY2(nc.text.contains(QStringLiteral("NC")), qPrintable(nc.text));
		QString error;
		Element *fresh = DeviceTypeAssign::apply(contact, device, nc, &error);
		QVERIFY2(fresh, qPrintable(error));
		QCOMPARE(pins(fresh), QStringList({QStringLiteral("21"), QStringLiteral("22")}));
		QCOMPARE(fresh->linkType(), Element::Slave);
		QVERIFY(fresh->linkedElements().contains(coil));
	}

	void aMasterKeepsItsContacts()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil_drawing = define(project, QStringLiteral("coil"), QStringLiteral("master"), 2);
		const QString contact_drawing = define(project, QStringLiteral("contact"), QStringLiteral("slave"), 2,
											   {QStringLiteral("13"), QStringLiteral("14")});
		Element *coil = place(folio, coil_drawing, QStringLiteral("K1"), QPointF(100, 100));
		Element *contact = place(folio, contact_drawing, QString(), QPointF(300, 100));
		QVERIFY(coil && contact);
		coil->linkToElement(contact);
		QVERIFY(contact->linkedElements().contains(coil));
		const DeviceType type = contactor();
		project.setDeviceType(type);

		const auto device = DeviceTypeAssign::deviceOf(coil, type);
		Element *fresh = DeviceTypeAssign::apply(coil, device, own(DeviceTypeAssign::choices(coil, device), 0, 0));
		QVERIFY(fresh && fresh != coil);
		QVERIFY(contact->linkedElements().contains(fresh));

			//One step back gives the coil and its link
		folio->undoStack().undo();
		QVERIFY(folio->elements().contains(coil));
		QVERIFY(contact->linkedElements().contains(coil));
	}
};

QTEST_MAIN(tst_devicetypeassign)
#include "tst_devicetypeassign.moc"
