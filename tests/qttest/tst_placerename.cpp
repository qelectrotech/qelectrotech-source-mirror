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
#include "placerename.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetproject.h"
#include "qetresult.h"
#include "ui/plantslocationsdialog.h"

#include <QDomDocument>
#include <QPushButton>
#include <QTableWidget>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

/**
	Renaming a plant or a location in the whole project: what changes, what
	must not, one undo step, and the choice for clashing wire counters.
*/
class tst_placerename : public QObject
{
	Q_OBJECT
	int m_position = 0;

	Element *symbol(Diagram *diagram, const QString &label,
					const QString &plant = QString(), const QString &location = QString())
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
		if (!plant.isEmpty()) info.addValue(QStringLiteral("plant"), plant);
		if (!location.isEmpty()) info.addValue(QStringLiteral("location"), location);
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

	static QStringList places(QETProject &project)
	{
		project.dataBase()->updateDB();
		QStringList list;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
				"SELECT plant, location, symbols, folios FROM qet_locations "
				"ORDER BY plant, location"));
		while (query.next())
			list << query.value(0).toString() + QLatin1Char('/') + query.value(1).toString()
					+ QLatin1Char('/') + query.value(2).toString() + QLatin1Char('/')
					+ query.value(3).toString();
		return list;
	}

	static PlaceRename::Request plant(const QString &from, const QString &to)
	{
		PlaceRename::Request r;
		r.plant = from;
		r.new_name = to;
		return r;
	}
	static PlaceRename::Request loc(const QString &p, const QString &l, const QString &to)
	{
		PlaceRename::Request r;
		r.plant = p;
		r.location = l;
		r.new_name = to;
		r.rename_location = true;
		return r;
	}

	static bool run(QETProject &project, const PlaceRename::Request &request,
					PlaceRename::CounterPolicy policy = PlaceRename::CounterPolicy::MergeHigher)
	{
		QUndoCommand *command = PlaceRename::command(&project, request, policy);
		if (!command) return false;
		project.undoStack()->push(command);
		return true;
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}
	void init() {m_position = 0;}

	void higherCounterValues()
	{
		using PlaceRename::higherCounter;
		QCOMPARE(higherCounter(QStringLiteral("9"), QStringLiteral("10")), QStringLiteral("10"));
		QCOMPARE(higherCounter(QStringLiteral("10"), QStringLiteral("9")), QStringLiteral("10"));
		QCOMPARE(higherCounter(QStringLiteral("Z"), QStringLiteral("AA")), QStringLiteral("AA"));
		QCOMPARE(higherCounter(QStringLiteral("B"), QStringLiteral("C")), QStringLiteral("C"));
	}

	void mergingCounters()
	{
		using PlaceRename::CounterPolicy;
		using PlaceRename::mergeCounter;
		const QStringList old_plant{QStringLiteral("12"), QStringLiteral("3")};
		const QStringList new_plant{QStringLiteral("5"), QStringLiteral("7"), QStringLiteral("2")};
		QCOMPARE(mergeCounter(old_plant, new_plant, CounterPolicy::MergeHigher),
				 QStringList({QStringLiteral("12"), QStringLiteral("7"), QStringLiteral("2")}));
		QCOMPARE(mergeCounter(old_plant, new_plant, CounterPolicy::KeepNew), new_plant);
		QCOMPARE(mergeCounter(old_plant, new_plant, CounterPolicy::KeepOld), old_plant);
			//No clash: the old plant's counters just move
		QCOMPARE(mergeCounter(old_plant, {}, CounterPolicy::KeepNew), old_plant);
	}

	void rejectsBadRequests()
	{
		QString why;
		QVERIFY(PlaceRename::isValid(plant(QStringLiteral("A"), QStringLiteral("B")), &why));
		QVERIFY(!PlaceRename::isValid(plant(QString(), QStringLiteral("B")), &why));
		QVERIFY(!PlaceRename::isValid(plant(QStringLiteral("A"), QString()), &why));
		QVERIFY(!PlaceRename::isValid(plant(QStringLiteral("A"), QStringLiteral(" B")), &why));
		QVERIFY(!PlaceRename::isValid(plant(QStringLiteral("A"), QStringLiteral("A")), &why));
		QVERIFY(!PlaceRename::isValid(loc(QStringLiteral("A"), QString(), QStringLiteral("B")), &why));
	}

	void renamesAPlantEverywhere()
	{
		QETProject project;
		Diagram *factory = project.addNewDiagram();
		Diagram *silo = project.addNewDiagram();
		setPlace(factory, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setPlace(silo, QStringLiteral("SILO"), QStringLiteral("P1"));
		Element *k1 = symbol(factory, QStringLiteral("K1"));
			//Holds the old name itself, as a symbol of another plant can
		Element *k2 = symbol(silo, QStringLiteral("K2"), QStringLiteral("FACTORY"), QStringLiteral("CP9"));
		QVERIFY(k1 && k2);
		const QStringList before = places(project);

		QVERIFY(run(project, plant(QStringLiteral("FACTORY"), QStringLiteral("WORKS"))));
		QCOMPARE(factory->border_and_titleblock.plant(), QStringLiteral("WORKS"));
		QCOMPARE(silo->border_and_titleblock.plant(), QStringLiteral("SILO"));
		QCOMPARE(k2->elementInformations().value(QStringLiteral("plant")).toString(), QStringLiteral("WORKS"));
			//The symbol that had no plant of its own still has none
		QVERIFY(!k1->elementInformations().contains(QStringLiteral("plant")));
		const QStringList after = places(project);
		QVERIFY(!after.join(QLatin1Char(' ')).contains(QStringLiteral("FACTORY")));
		QVERIFY(after.join(QLatin1Char(' ')).contains(QStringLiteral("WORKS/CP1")));

			//One undo step puts everything back, one redo does it again
		project.undoStack()->undo();
		QCOMPARE(places(project), before);
		QCOMPARE(factory->border_and_titleblock.plant(), QStringLiteral("FACTORY"));
		QCOMPARE(k2->elementInformations().value(QStringLiteral("plant")).toString(), QStringLiteral("FACTORY"));
		project.undoStack()->redo();
		QCOMPARE(places(project), after);
	}

	void aLocationBelongsToItsPlant()
	{
		QETProject project;
		Diagram *factory = project.addNewDiagram();
		Diagram *silo = project.addNewDiagram();
		setPlace(factory, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setPlace(silo, QStringLiteral("SILO"), QStringLiteral("CP1"));
		QVERIFY(symbol(factory, QStringLiteral("K1")));
		QVERIFY(symbol(silo, QStringLiteral("K2")));

		QVERIFY(run(project, loc(QStringLiteral("FACTORY"), QStringLiteral("CP1"), QStringLiteral("CP2"))));
		QCOMPARE(factory->border_and_titleblock.locmach(), QStringLiteral("CP2"));
		QCOMPARE(silo->border_and_titleblock.locmach(), QStringLiteral("CP1"));
		project.undoStack()->undo();
		QCOMPARE(factory->border_and_titleblock.locmach(), QStringLiteral("CP1"));
	}

	void nothingToRenameIsNotACommand()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("FACTORY"), QString());
		const int steps = project.undoStack()->count();
		QVERIFY(!run(project, plant(QStringLiteral("NOWHERE"), QStringLiteral("X"))));
		QCOMPARE(project.undoStack()->count(), steps);
	}

	void renamesStructureBoxesAndDefaults()
	{
		QETProject project(QFINDTESTDATA("fixtures/structure_box.qet"));
		project.loadFolios();
		auto boxPlant = [&project]() {
			for (Diagram *d : project.folios())
				for (const StructureBoxItem *box : StructureBoxItem::boxesOf(d))
					return box->plant() + QLatin1Char('/') + box->location();
			return QString();
		};
		QCOMPARE(boxPlant(), QStringLiteral("E1/FIELD"));
		auto defaults = project.defaultTitleBlockProperties();
		defaults.plant = QStringLiteral("E1");
		project.setDefaultTitleBlockProperties(defaults);

		QVERIFY(run(project, plant(QStringLiteral("E1"), QStringLiteral("E2"))));
		QCOMPARE(boxPlant(), QStringLiteral("E2/FIELD"));
		QCOMPARE(project.defaultTitleBlockProperties().plant, QStringLiteral("E2"));
		QVERIFY(run(project, loc(QStringLiteral("E2"), QStringLiteral("FIELD"), QStringLiteral("YARD"))));
		QCOMPARE(boxPlant(), QStringLiteral("E2/YARD"));
		project.undoStack()->undo();
		project.undoStack()->undo();
		QCOMPARE(boxPlant(), QStringLiteral("E1/FIELD"));
		QCOMPARE(project.defaultTitleBlockProperties().plant, QStringLiteral("E1"));
	}

	void undoLeavesOtherDefaultsAlone()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("A"), QString());
		auto defaults = project.defaultTitleBlockProperties();
		defaults.plant = QStringLiteral("A");
		defaults.author = QStringLiteral("first");
		project.setDefaultTitleBlockProperties(defaults);
		QVERIFY(run(project, plant(QStringLiteral("A"), QStringLiteral("B"))));
			//Another default changes before the rename is undone
		defaults = project.defaultTitleBlockProperties();
		defaults.author = QStringLiteral("second");
		project.setDefaultTitleBlockProperties(defaults);
		project.undoStack()->undo();
		QCOMPARE(project.defaultTitleBlockProperties().plant, QStringLiteral("A"));
		QCOMPARE(project.defaultTitleBlockProperties().author, QStringLiteral("second"));
	}

	static StructureBoxItem *box(Diagram *diagram, const QRectF &rect,
								 const QString &plant, const QString &location)
	{
		auto *b = new StructureBoxItem(rect.topLeft(), rect.bottomRight());
		b->setPlant(plant);
		b->setLocation(location);
		diagram->addItem(b);
		return b;
	}

	void anInnerBoxBelongsToTheBoxAroundIt()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("FACTORY"), QString());
			//The outer box puts everything inside it in SILO; the inner one
			//has no plant, so it lies in SILO/CP1, not FACTORY/CP1
		box(folio, QRectF(0, 0, 400, 400), QStringLiteral("SILO"), QStringLiteral("A"));
		StructureBoxItem *inner = box(folio, QRectF(100, 100, 100, 100), QString(), QStringLiteral("CP1"));

		QVERIFY(!run(project, loc(QStringLiteral("FACTORY"), QStringLiteral("CP1"), QStringLiteral("CP2"))));
		QCOMPARE(inner->location(), QStringLiteral("CP1"));
		QVERIFY(run(project, loc(QStringLiteral("SILO"), QStringLiteral("CP1"), QStringLiteral("CP2"))));
		QCOMPARE(inner->location(), QStringLiteral("CP2"));
		QVERIFY(inner->plant().isEmpty());
		project.undoStack()->undo();
		QCOMPARE(inner->location(), QStringLiteral("CP1"));
	}

	void aSymbolInAPlantBoxBelongsToThatPlant()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *k1 = symbol(folio, QStringLiteral("K1"), QString(), QStringLiteral("CP1"));
		QVERIFY(k1);
		box(folio, QRectF(k1->pos() - QPointF(60, 60), QSizeF(120, 120)), QStringLiteral("SILO"), QString());
		const QString before = places(project).join(QLatin1Char(' '));
		QVERIFY2(before.contains(QStringLiteral("SILO/CP1")), qPrintable(before));

			//The symbol's own CP1 is SILO's: FACTORY/CP1 leaves it alone
		QVERIFY(run(project, loc(QStringLiteral("FACTORY"), QStringLiteral("CP1"), QStringLiteral("CP2"))));
		QCOMPARE(folio->border_and_titleblock.locmach(), QStringLiteral("CP2"));
		QCOMPARE(k1->elementInformations().value(QStringLiteral("location")).toString(), QStringLiteral("CP1"));
		QVERIFY(run(project, loc(QStringLiteral("SILO"), QStringLiteral("CP1"), QStringLiteral("CP3"))));
		QCOMPARE(k1->elementInformations().value(QStringLiteral("location")).toString(), QStringLiteral("CP3"));
	}

	void aLocationCanMergeIntoAnother()
	{
		QETProject project;
		Diagram *a = project.addNewDiagram();
		Diagram *b = project.addNewDiagram();
		setPlace(a, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setPlace(b, QStringLiteral("FACTORY"), QStringLiteral("CP2"));
		QVERIFY(symbol(a, QStringLiteral("K1")));
		QVERIFY(symbol(b, QStringLiteral("K2")));
		const QStringList before = places(project);
		QCOMPARE(before.size(), 2);
		const auto request = loc(QStringLiteral("FACTORY"), QStringLiteral("CP1"), QStringLiteral("CP2"));
		QVERIFY(PlaceRename::targetExists(&project, request));
		QVERIFY(run(project, request));
		QCOMPARE(places(project), QStringList({QStringLiteral("FACTORY/CP2/2/2")}));
		project.undoStack()->undo();
		QCOMPARE(places(project), before);
	}

	void movesWireCounters()
	{
		using PlaceRename::CounterPolicy;
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("A"), QString());
		const QString scheme = QStringLiteral("s");
		project.setConductorPlantCounter(scheme, QStringLiteral("A"), {QStringLiteral("12")});

			//The new name is free: the counters move with the plant
		QVERIFY(run(project, plant(QStringLiteral("A"), QStringLiteral("B"))));
		QVERIFY(project.conductorPlantCounter(scheme, QStringLiteral("A")).isEmpty());
		QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("B")), QStringList({QStringLiteral("12")}));
		project.undoStack()->undo();
		QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("A")), QStringList({QStringLiteral("12")}));
		QVERIFY(project.conductorPlantCounter(scheme, QStringLiteral("B")).isEmpty());
	}

	void clashingWireCountersFollowTheChoice()
	{
		using PlaceRename::CounterPolicy;
		const QString scheme = QStringLiteral("s");
		struct Case {CounterPolicy policy; QString expected;};
		const QList<Case> cases{{CounterPolicy::MergeHigher, QStringLiteral("12")},
								{CounterPolicy::KeepNew, QStringLiteral("5")},
								{CounterPolicy::KeepOld, QStringLiteral("12")}};
		for (const Case &c : cases) {
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			setPlace(folio, QStringLiteral("A"), QString());
			project.setConductorPlantCounter(scheme, QStringLiteral("A"), {QStringLiteral("12")});
			project.setConductorPlantCounter(scheme, QStringLiteral("B"), {QStringLiteral("5")});
			const auto request = plant(QStringLiteral("A"), QStringLiteral("B"));
			QVERIFY(PlaceRename::counterClash(&project, request));
			QVERIFY(run(project, request, c.policy));
			QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("B")), QStringList({c.expected}));
			QVERIFY(project.conductorPlantCounter(scheme, QStringLiteral("A")).isEmpty());
				//Undo gives both plants their own numbers back
			project.undoStack()->undo();
			QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("A")), QStringList({QStringLiteral("12")}));
			QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("B")), QStringList({QStringLiteral("5")}));
		}
		QETProject none;
		QVERIFY(!PlaceRename::counterClash(&none, plant(QStringLiteral("A"), QStringLiteral("B"))));
	}

	void dialogRenamesAndShowsIt()
	{
		QETProject project;
		Diagram *factory = project.addNewDiagram();
		Diagram *loose = project.addNewDiagram();
		setPlace(factory, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		QVERIFY(symbol(factory, QStringLiteral("K1")));
		QVERIFY(symbol(loose, QStringLiteral("Q1")));
		PlantsLocationsDialog dialog(&project);
		auto *table = dialog.findChild<QTableWidget *>(QStringLiteral("placesTable"));
		auto *plant_button = dialog.findChild<QPushButton *>(QStringLiteral("renamePlant"));
		auto *location_button = dialog.findChild<QPushButton *>(QStringLiteral("renameLocation"));
		QVERIFY(table && plant_button && location_button);
		QCOMPARE(table->rowCount(), 2);
			//The row with no plant has nothing to rename
		table->setCurrentCell(0, 0);
		QCOMPARE(table->item(0, 0)->text(), QStringLiteral("(none)"));
		QVERIFY(!plant_button->isEnabled());
		QVERIFY(!location_button->isEnabled());
		table->setCurrentCell(1, 0);
		QVERIFY(plant_button->isEnabled());
		QVERIFY(location_button->isEnabled());

		QVERIFY(dialog.applyRename(plant(QStringLiteral("FACTORY"), QStringLiteral("WORKS"))));
		QCOMPARE(table->item(1, 0)->text(), QStringLiteral("WORKS"));
		project.undoStack()->undo();
		dialog.refresh();
		QCOMPARE(table->item(1, 0)->text(), QStringLiteral("FACTORY"));
			//A name nothing stores is refused, not a silent no-op
		QVERIFY(!dialog.applyRename(plant(QStringLiteral("NOWHERE"), QStringLiteral("X"))));
	}

	void survivesSaveAndReload()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath(QStringLiteral("renamed.qet"));
		const QString scheme = QStringLiteral("s");
		QStringList renamed;
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			setPlace(folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
			QVERIFY(symbol(folio, QStringLiteral("K1"), QStringLiteral("FACTORY"), QStringLiteral("CP1")));
			project.setConductorPlantCounter(scheme, QStringLiteral("FACTORY"), {QStringLiteral("7")});
			QVERIFY(run(project, plant(QStringLiteral("FACTORY"), QStringLiteral("WORKS"))));
			renamed = places(project);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QCOMPARE(again.state(), QETProject::Ok);
		again.loadFolios();
		QCOMPARE(places(again), renamed);
		QVERIFY(again.conductorPlantCounter(scheme, QStringLiteral("FACTORY")).isEmpty());
		QCOMPARE(again.conductorPlantCounter(scheme, QStringLiteral("WORKS")), QStringList({QStringLiteral("7")}));
		QVERIFY(renamed.join(QLatin1Char(' ')).contains(QStringLiteral("WORKS/CP1/1/1")));
	}

	void knowsWhenTheNameExists()
	{
		QETProject project;
		Diagram *a = project.addNewDiagram();
		Diagram *b = project.addNewDiagram();
		setPlace(a, QStringLiteral("A"), QStringLiteral("L1"));
		setPlace(b, QStringLiteral("B"), QStringLiteral("L1"));
		QVERIFY(symbol(a, QStringLiteral("K1")));
		QVERIFY(symbol(b, QStringLiteral("K2")));
		QVERIFY(PlaceRename::targetExists(&project, plant(QStringLiteral("A"), QStringLiteral("B"))));
		QVERIFY(!PlaceRename::targetExists(&project, plant(QStringLiteral("A"), QStringLiteral("C"))));
		QVERIFY(!PlaceRename::targetExists(&project, loc(QStringLiteral("A"), QStringLiteral("L1"), QStringLiteral("L2"))));
	}
};

QET_APP_TEST_MAIN(tst_placerename)
#include "tst_placerename.moc"
