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
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "placesuggestions.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetproject.h"
#include "ui/elementinfopartwidget.h"
#include "ui/elementinfowidget.h"
#include "ui/multielementinfowidget.h"
#include "ui/structureboxpropertieswidget.h"
#include "ui/titleblockpropertieswidget.h"

#include <QCompleter>
#include <QDomDocument>
#include <QLineEdit>
#include <QStringListModel>
#include <QtTest>

/**
	The plants and locations a project already has, offered while a plant or
	a location is typed in folio properties, structure box properties and
	symbol information.
*/
class tst_placesuggestions : public QObject
{
	Q_OBJECT
	int m_position = 0;

	Element *symbol(Diagram *diagram, const QString &label)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/simple.elmt"))) {
			QDomDocument definition;
			if (!definition.setContent(QString(
					R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" name="1" uuid="%2"/><terminal x="0" y="20" orientation="s" name="2" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString())))
				return nullptr;
			if (!collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("simple"),
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/simple.elmt"), diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(100 + 80 * m_position++, 150);
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

		//FACTORY/CP1 and FACTORY/CP2 on folios; YARD/Z only a structure box
		//gives, around a symbol that has no plant of its own
	void build(QETProject &project)
	{
		Diagram *one = project.addNewDiagram();
		Diagram *two = project.addNewDiagram();
		Diagram *three = project.addNewDiagram();
		setPlace(one, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setPlace(two, QStringLiteral("FACTORY"), QStringLiteral("CP2"));
		setPlace(three, QStringLiteral("SILO"), QStringLiteral("P1"));
		QVERIFY(symbol(one, QStringLiteral("K1")));
		QVERIFY(symbol(two, QStringLiteral("K2")));
		Element *k3 = symbol(three, QStringLiteral("K3"));
		QVERIFY(k3);
		auto *box = new StructureBoxItem(k3->pos() - QPointF(60, 60), k3->pos() + QPointF(60, 60));
		box->setPlant(QStringLiteral("YARD"));
		box->setLocation(QStringLiteral("Z"));
		three->addItem(box);
	}

	static QStringList listOf(const QLineEdit *edit)
	{
		auto *completer = edit ? edit->completer() : nullptr;
		auto *model = completer ? qobject_cast<QStringListModel *>(completer->model()) : nullptr;
		return model ? model->stringList() : QStringList();
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}
	void init() {m_position = 0;}

	void mergesWithoutRepeats()
	{
		QCOMPARE(PlaceSuggestions::merged({QStringLiteral("b"), QStringLiteral("A")},
										  {QStringLiteral("a"), QStringLiteral("C"), QString()}),
				 QStringList({QStringLiteral("A"), QStringLiteral("b"), QStringLiteral("C")}));
	}

	void plantsIncludeThoseOnlyABoxGives()
	{
		QETProject project;
		build(project);
		QCOMPARE(PlaceSuggestions::plants(&project),
				 QStringList({QStringLiteral("FACTORY"), QStringLiteral("SILO"), QStringLiteral("YARD")}));
		QETProject empty;
		QVERIFY(PlaceSuggestions::plants(&empty).isEmpty());
		QVERIFY(PlaceSuggestions::plants(nullptr).isEmpty());
	}

	void locationsBelongToTheirPlant()
	{
		QETProject project;
		build(project);
		QCOMPARE(PlaceSuggestions::locations(&project, QStringLiteral("FACTORY")),
				 QStringList({QStringLiteral("CP1"), QStringLiteral("CP2")}));
		QCOMPARE(PlaceSuggestions::locations(&project, QStringLiteral("factory")),
				 QStringList({QStringLiteral("CP1"), QStringLiteral("CP2")}));
		QCOMPARE(PlaceSuggestions::locations(&project, QStringLiteral("YARD")),
				 QStringList({QStringLiteral("Z")}));
		QCOMPARE(PlaceSuggestions::locations(&project, QStringLiteral("NOWHERE")), QStringList());
		QCOMPARE(PlaceSuggestions::locations(&project).size(), 4);
	}

	void attachFollowsThePlantTyped()
	{
		QETProject project;
		build(project);
		QLineEdit plant, location;
		PlaceSuggestions::attach(&plant, &location, &project);
		QVERIFY(plant.completer());
		QCOMPARE(listOf(&plant).size(), 3);
		QCOMPARE(listOf(&location).size(), 4);
		plant.setText(QStringLiteral("FACTORY"));
		QCOMPARE(listOf(&location), QStringList({QStringLiteral("CP1"), QStringLiteral("CP2")}));
		plant.setText(QStringLiteral("SILO"));
		QCOMPARE(listOf(&location), QStringList({QStringLiteral("P1")}));
			//A name the project does not have can still be typed
		plant.setText(QStringLiteral("a brand new plant"));
		location.setText(QStringLiteral("somewhere new"));
		QCOMPARE(plant.text(), QStringLiteral("a brand new plant"));
		QCOMPARE(location.text(), QStringLiteral("somewhere new"));

		QLineEdit none_plant, none_location;
		PlaceSuggestions::attach(&none_plant, &none_location, nullptr);
		QVERIFY(!none_plant.completer());
		QVERIFY(!none_location.completer());
	}

	void aSymbolIsNotOfferedItsOwnName()
	{
		QETProject project;
		build(project);
			//The only holder of the plant "Pump house" is this symbol
		Element *pump = symbol(project.folios().first(), QStringLiteral("P1"));
		QVERIFY(pump);
		auto info = pump->elementInformations();
		info.addValue(QStringLiteral("plant"), QStringLiteral("Pump house"));
		pump->setElementInformations(info);
		const QString uuid = pump->uuid().toString();
		QVERIFY(PlaceSuggestions::plants(&project).contains(QStringLiteral("Pump house")));
		QVERIFY(!PlaceSuggestions::plants(&project, uuid).contains(QStringLiteral("Pump house")));
		QVERIFY(PlaceSuggestions::plants(&project, uuid).contains(QStringLiteral("FACTORY")));
		ElementInfoWidget panel(pump);
		ElementInfoPartWidget *plantPart = nullptr;
		for (auto *part : panel.findChildren<ElementInfoPartWidget *>())
			if (part->key() == QLatin1String("plant"))
				plantPart = part;
		QVERIFY(plantPart);
		const QStringList offered = listOf(plantPart->findChild<QLineEdit *>());
		QVERIFY(!offered.contains(QStringLiteral("Pump house")));
		QVERIFY(offered.contains(QStringLiteral("FACTORY")));
	}

	void anEmptiedListKeepsItsCompleter()
	{
		QLineEdit edit;
		QVERIFY(!edit.completer());
		PlaceSuggestions::setSuggestions(&edit, {});
		QVERIFY(!edit.completer());
		PlaceSuggestions::setSuggestions(&edit, {QStringLiteral("a")});
		QCompleter *first = edit.completer();
		QVERIFY(first);
		PlaceSuggestions::setSuggestions(&edit, {});
		QCOMPARE(edit.completer(), first);
		QVERIFY(listOf(&edit).isEmpty());
		PlaceSuggestions::setSuggestions(&edit, {QStringLiteral("b")});
		QCOMPARE(edit.completer(), first);
		QCOMPARE(listOf(&edit), QStringList({QStringLiteral("b")}));
	}

	void folioPropertiesOffersThePlaces()
	{
		QETProject project;
		build(project);
		TitleBlockPropertiesWidget with_project(TitleBlockProperties(), false, &project);
		auto *plant = with_project.findChild<QLineEdit *>(QStringLiteral("m_plant"));
		QVERIFY(plant);
		QCOMPARE(listOf(plant).size(), 3);
		plant->setText(QStringLiteral("brand new"));
		QCOMPARE(with_project.properties().plant, QStringLiteral("brand new"));

			//Opened from the configuration page: no project, no list
		TitleBlockPropertiesWidget without_project;
		QVERIFY(!without_project.findChild<QLineEdit *>(QStringLiteral("m_plant"))->completer());
	}

	void symbolInformationOffersThePlaces()
	{
		QETProject project;
		build(project);
		Element *k1 = symbol(project.folios().first(), QStringLiteral("K9"));
		QVERIFY(k1);
		ElementInfoWidget panel(k1);
		auto lineOf = [&panel](const QString &key) -> QLineEdit * {
			for (auto *part : panel.findChildren<ElementInfoPartWidget *>())
				if (part->key() == key) return part->findChild<QLineEdit *>();
			return nullptr;
		};
		QLineEdit *plant = lineOf(QStringLiteral("plant"));
		QLineEdit *location = lineOf(QStringLiteral("location"));
		QVERIFY(plant && location);
			//No symbol holds a plant itself: the list is the project's places,
			//the plant only a structure box gives (YARD) among them
		QVERIFY(listOf(plant).contains(QStringLiteral("YARD")));
		QVERIFY(listOf(plant).contains(QStringLiteral("FACTORY")));
		QVERIFY(listOf(location).contains(QStringLiteral("Z")));
		QVERIFY(listOf(location).contains(QStringLiteral("CP2")));
	}

	void severalSymbolsOfferThePlacesToo()
	{
		QETProject project;
		build(project);
		Element *k9 = symbol(project.folios().first(), QStringLiteral("K9"));
		Element *p1 = symbol(project.folios().first(), QStringLiteral("P1"));
		QVERIFY(k9 && p1);
		MultiElementInfoWidget panel(QList<Element *>{k9, p1});
		bool yard = false, cp2 = false;
		for (auto *edit : panel.findChildren<QLineEdit *>()) {
			const QStringList offered = listOf(edit);
			yard = yard || offered.contains(QStringLiteral("YARD"));
			cp2 = cp2 || offered.contains(QStringLiteral("CP2"));
		}
			//YARD is given only by a structure box, not by any symbol
		QVERIFY(yard);
		QVERIFY(cp2);
	}

	void structureBoxPropertiesOffersThePlaces()
	{
		QETProject project;
		build(project);
		Diagram *folio = project.folios().first();
		auto *box = new StructureBoxItem(QPointF(0, 0), QPointF(50, 50));
		folio->addItem(box);
		StructureBoxPropertiesWidget widget(box);
		auto *plant = widget.findChild<QLineEdit *>(QStringLiteral("plantEdit"));
		auto *location = widget.findChild<QLineEdit *>(QStringLiteral("locationEdit"));
		QVERIFY(plant && location);
		QCOMPARE(listOf(plant).size(), 3);
		plant->setText(QStringLiteral("YARD"));
		QCOMPARE(listOf(location), QStringList({QStringLiteral("Z")}));
	}
};

QET_APP_TEST_MAIN(tst_placesuggestions)
#include "tst_placesuggestions.moc"
