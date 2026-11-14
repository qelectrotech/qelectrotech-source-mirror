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
#include "devicetype/devicetypecheck.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/devicetypecheckdialog.h"

#include <QDomDocument>
#include <QCheckBox>
#include <QLabel>
#include <QTreeWidget>
#include <QtTest>

/**
	Is a device drawn the way its type says: the parts of the type not
	drawn, and the symbols beyond it (DeviceTypeCheck).
*/
class tst_devicetypecheck : public QObject
{
	Q_OBJECT

	int m_position = 0;

		//An element import/<file>.elmt named @p name, with terminals @p pins
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

	Element *place(Diagram *diagram, const QString &file, const QString &label = QString(),
				   const QString &model = QString())
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/") + file + QStringLiteral(".elmt"),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 80 * (position % 8), 150 + 100 * (position / 8));
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		if (!model.isEmpty()) info.addValue(QStringLiteral("device_type"), model);
		element->setElementInformations(info);
		return element;
	}

		//CONTR10: a coil, three power contacts, an auxiliary NO
	static DeviceType contactor()
	{
		DeviceType type;
		type.name = QStringLiteral("CONTR10");
		type.names.insert(QStringLiteral("en"), QStringLiteral("MOTOR CONTR, 1 N/O CONTACT"));
		DeviceType::Part coil{QStringLiteral("master"), QStringLiteral("coil"), {},
			{{QStringLiteral("embed://import/coil.elmt"), {QStringLiteral("A1"), QStringLiteral("A2")}, {}}}};
		DeviceType::Part power{QStringLiteral("slave"), QStringLiteral("power"), {},
			{{QStringLiteral("embed://import/power.elmt"),
			  QStringLiteral("1 2 3 4 5 6").split(QLatin1Char(' ')), {}}}};
		DeviceType::Part aux{QStringLiteral("slave"), QStringLiteral("simple"), {},
			{{QStringLiteral("embed://import/aux.elmt"), {QStringLiteral("13"), QStringLiteral("14")},
			  QStringLiteral("NO")}}};
		type.parts = {coil, power, aux};
		return type;
	}

	void defineAll(QETProject *project)
	{
		QVERIFY(define(project, QStringLiteral("coil"), QStringLiteral("CONTR-COIL"),
					   QStringLiteral("master"), {QStringLiteral("A1"), QStringLiteral("A2")}));
		QVERIFY(define(project, QStringLiteral("power"), QStringLiteral("CONTR-3PH"), QStringLiteral("slave"),
					   QStringLiteral("1 2 3 4 5 6").split(QLatin1Char(' '))));
		QVERIFY(define(project, QStringLiteral("aux"), QStringLiteral("CONTR-AUX-NO"),
					   QStringLiteral("slave"), {QStringLiteral("13"), QStringLiteral("14")}));
			//The aux contact drawn turned: a copy, same name, own file
		QVERIFY(define(project, QStringLiteral("sym7"), QStringLiteral("CONTR-AUX-NO"),
					   QStringLiteral("slave"), {QStringLiteral("53"), QStringLiteral("54")}));
			//A table of the contacts beside a coil: no terminals
		QVERIFY(define(project, QStringLiteral("table"), QStringLiteral("XREF12"),
					   QStringLiteral("simple"), {}));
		QVERIFY(define(project, QStringLiteral("lamp"), QStringLiteral("LAMP"),
					   QStringLiteral("slave"), {QStringLiteral("X1"), QStringLiteral("X2")}));
		project->setDeviceType(contactor());
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}
	void init() {m_position = 0;}

	void partsNotDrawn()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		defineAll(&project);
		Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K12"), QStringLiteral("CONTR10"));
		Element *aux = place(folio, QStringLiteral("aux"));
		QVERIFY(coil && aux);
		coil->linkToElement(aux);

		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		const auto &k12 = devices.first();
		QVERIFY(k12.designation.endsWith(QStringLiteral("-K12")));
		QCOMPARE(k12.typeName, QStringLiteral("CONTR10"));
		QVERIFY(k12.known());
		QCOMPARE(k12.symbols.first().data(), coil);
		QCOMPARE(k12.drawn, QList<int>({0, 2}));
		QCOMPARE(k12.missing, QList<int>({1}));
		QVERIFY(!k12.complete());
		QCOMPARE(DeviceTypeCheck::partText(k12.type.parts.at(2)), QStringLiteral("simple NO 13 14"));

			//The contact's device is the coil's
		const auto of_aux = DeviceTypeCheck::checkDevice(aux);
		QCOMPARE(of_aux.designation, k12.designation);
		QCOMPARE(of_aux.missing, QList<int>({1}));
	}

	void oneSignWhetherTheLabelHasItOrNot()
	{
			//A label typed with its sign (-K12) and one typed without (K13)
			//are both named with one '-' after the place
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		defineAll(&project);
		QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("-K12"), QStringLiteral("CONTR10")));
		QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K13"), QStringLiteral("CONTR10")));

		QStringList names;
		for (const auto &device : DeviceTypeCheck::check(&project))
			names << device.designation;
		names.sort();
		QCOMPARE(names.size(), 2);
		QVERIFY2(names.at(0).endsWith(QStringLiteral("-K12")) && !names.at(0).contains(QStringLiteral("--")),
				 qPrintable(names.at(0)));
		QVERIFY2(names.at(1).endsWith(QStringLiteral("-K13")) && !names.at(1).contains(QStringLiteral("--")),
				 qPrintable(names.at(1)));
	}

	void copyOfASymbolAndOneBeyond()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		defineAll(&project);
		Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
		Element *power = place(folio, QStringLiteral("power"));
		Element *turned = place(folio, QStringLiteral("sym7"));   //other pins, same symbol
		Element *lamp = place(folio, QStringLiteral("lamp"));
		QVERIFY(coil && power && turned && lamp);
		coil->linkToElement(power);
		coil->linkToElement(turned);
		coil->linkToElement(lamp);

		const auto device = DeviceTypeCheck::checkDevice(coil);
		QVERIFY(device.missing.isEmpty());
		QCOMPARE(device.beyond(), 1);
		QCOMPARE(device.drawn.at(device.symbols.indexOf(turned)), 2);
		QCOMPARE(device.drawn.at(device.symbols.indexOf(lamp)), -1);
		QVERIFY(!device.complete());

		coil->unlinkElement(lamp);
		QVERIFY(DeviceTypeCheck::checkDevice(coil).complete());
	}

	void sameDesignationIsOneDevice()
	{
			//Nothing links them: one designation names one device
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		defineAll(&project);
		QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K2"), QStringLiteral("CONTR10")));
		QVERIFY(place(folio, QStringLiteral("aux"), QStringLiteral("K2")));
		QVERIFY(place(folio, QStringLiteral("power"), QStringLiteral("K2")));
		Element *table = place(folio, QStringLiteral("table"), QStringLiteral("K2"));
		QVERIFY(table);
		QVERIFY(place(folio, QStringLiteral("aux"), QStringLiteral("K3")));   //no model
		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QVERIFY(devices.first().complete());
		QCOMPARE(devices.first().symbols.size(), 4);
		QCOMPARE(devices.first().drawn.at(devices.first().symbols.indexOf(table)),
				 int(DeviceTypeCheck::NotAPart));
	}

		//A strip is checked terminal by terminal, each against its own
		//model's type: feed-through and earth terminals in one strip
	void aStripIsCheckedTerminalByTerminal()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(define(&project, QStringLiteral("term"), QStringLiteral("TERMINAL"),
					   QStringLiteral("terminal"), {QString(), QString()}));
		for (const QString &name : {QStringLiteral("TB-FEED"), QStringLiteral("TB-PE")}) {
			DeviceType type;
			type.name = name;
			type.parts = {DeviceType::Part{QStringLiteral("terminal"), QStringLiteral("terminal"), {},
				{{QStringLiteral("embed://import/term.elmt"), {}, {}}}}};
			project.setDeviceType(type);
		}
		QVERIFY(place(folio, QStringLiteral("term"), QStringLiteral("X1:1"), QStringLiteral("TB-FEED")));
		QVERIFY(place(folio, QStringLiteral("term"), QStringLiteral("X1:2"), QStringLiteral("TB-FEED")));
		QVERIFY(place(folio, QStringLiteral("term"), QStringLiteral("X1:PE"), QStringLiteral("TB-PE")));
		auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QCOMPARE(devices.first().label, QStringLiteral("X1"));
		QCOMPARE(devices.first().symbols.size(), 3);
		QVERIFY(devices.first().complete());

			//A terminal without a model is beyond the strip's types
		QVERIFY(place(folio, QStringLiteral("term"), QStringLiteral("X1:3")));
		devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QVERIFY(!devices.first().complete());
		QCOMPARE(devices.first().beyond(), 1);
	}

	void unknownType()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		defineAll(&project);
		QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K4"), QStringLiteral("NOSUCH")));
		const auto devices = DeviceTypeCheck::check(&project);
		QCOMPARE(devices.size(), 1);
		QVERIFY(!devices.first().known());
		QVERIFY(!devices.first().complete());
		QVERIFY(devices.first().missing.isEmpty());
	}

	void pinsOnlyWhenTheTypeHasNoSymbol()
	{
			//A part without a symbol yet is matched by its pins
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		defineAll(&project);
		DeviceType type = contactor();
		type.name = QStringLiteral("CONTR-LAMP");
		DeviceType::Part lamp{QStringLiteral("slave"), QStringLiteral("simple"), {},
			{{QString(), {QStringLiteral("X2"), QStringLiteral("X1")}, {}}}};
		type.parts.append(lamp);
		project.setDeviceType(type);
		Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K5"), QStringLiteral("CONTR-LAMP"));
		Element *l = place(folio, QStringLiteral("lamp"));
		QVERIFY(coil && l);
		coil->linkToElement(l);
		const auto device = DeviceTypeCheck::checkDevice(coil);
		QCOMPARE(device.drawn, QList<int>({0, 3}));
		QCOMPARE(device.missing, QList<int>({1, 2}));
	}
	void dialogListsIncompleteDevices()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		defineAll(&project);
		Element *k1 = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
		Element *k2 = place(folio, QStringLiteral("coil"), QStringLiteral("K2"), QStringLiteral("CONTR10"));
		QVERIFY(k1 && k2);
		for (const QString &file : {QStringLiteral("power"), QStringLiteral("aux")}) {
			Element *contact = place(folio, file);
			QVERIFY(contact);
			k2->linkToElement(contact);
		}
		DeviceTypeCheckDialog dialog(&project);
		auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("deviceTypeDevices"));
		auto *summary = dialog.findChild<QLabel *>(QStringLiteral("deviceTypeSummary"));
		QVERIFY(tree && summary);
		QCOMPARE(summary->text(), QStringLiteral("2 device(s) with a device type: 1 complete, 1 incomplete, "
												 "0 of a type the project does not have."));
		QCOMPARE(tree->topLevelItemCount(), 1);
		QVERIFY(tree->topLevelItem(0)->text(0).endsWith(QStringLiteral("-K1: 1 of 3 parts drawn")));
		QCOMPARE(tree->topLevelItem(0)->childCount(), 2);
		dialog.findChild<QCheckBox *>(QStringLiteral("deviceTypeShowComplete"))->setChecked(true);
		QCOMPARE(tree->topLevelItemCount(), 2);
	}
};

QET_APP_TEST_MAIN(tst_devicetypecheck)
#include "tst_devicetypecheck.moc"
