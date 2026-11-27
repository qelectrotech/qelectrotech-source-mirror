// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "placecopy.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetproject.h"
#include "qetresult.h"
#include "ui/plantslocationsdialog.h"

#include <QPushButton>
#include <QSet>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

/**
	Copying a plant: its folios with all they hold, under a new plant name,
	in one undo step, cross-references kept inside the copy.
*/
class tst_placecopy : public QObject
{
	Q_OBJECT

	static void setPlant(Diagram *diagram, const QString &plant,
						 const QString &title = QString())
	{
		auto block = diagram->border_and_titleblock.exportTitleBlock();
		block.plant = plant;
		if (!title.isEmpty()) block.title = title;
		diagram->border_and_titleblock.importTitleBlock(block);
	}

	static QList<Element *> elementsOf(const Diagram *diagram)
	{
		QList<Element *> list;
		for (QGraphicsItem *item : diagram->items())
			if (auto *element = dynamic_cast<Element *>(item))
				list << element;
		return list;
	}

		/// "plant label" of every device, sorted
	static QStringList designations(QETProject &project)
	{
		project.dataBase()->updateDB();
		QStringList list;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
				"SELECT plant, label FROM qet_devices"));
		while (query.next())
			list << query.value(0).toString() + QLatin1Char(' ') + query.value(1).toString();
		list.sort();
		return list;
	}

	static QStringList places(QETProject &project)
	{
		project.dataBase()->updateDB();
		QStringList list;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
				"SELECT plant, location, symbols, folios FROM qet_locations "
				"ORDER BY plant, location"));
		while (query.next())
			list << QStringList{query.value(0).toString(), query.value(1).toString(),
								query.value(2).toString(), query.value(3).toString()}
					.join(QLatin1Char('/'));
		return list;
	}

	static bool run(QETProject &project, const QString &plant, const QString &to)
	{
		return PlaceCopy::copy(&project, plant, to);
	}

		/// No two items of the project answer to the same uuid
	static void uuidsAreOwn(QETProject &project)
	{
		QSet<QUuid> elements, conductors;
		int element_count = 0, conductor_count = 0;
		for (Diagram *diagram : project.folios()) {
			for (Element *element : elementsOf(diagram)) {
				elements.insert(element->uuid());
				++element_count;
			}
			for (Conductor *conductor : diagram->conductors()) {
				conductors.insert(conductor->uuid());
				++conductor_count;
			}
		}
		QCOMPARE(elements.size(), element_count);
		QCOMPARE(conductors.size(), conductor_count);
	}

  private slots:
	void initTestCase() {QETProject::setBackupEnabled(false);}

	void refusesBadNames()
	{
		QETProject project;
		Diagram *a = project.addNewDiagram();
		Diagram *b = project.addNewDiagram();
		setPlant(a, QStringLiteral("A"));
		setPlant(b, QStringLiteral("B"));
		QString why;
		QVERIFY(PlaceCopy::isValid(&project, QStringLiteral("A"), QStringLiteral("C"), &why));
		QVERIFY(!PlaceCopy::isValid(&project, QStringLiteral("A"), QStringLiteral("A"), &why));
		QVERIFY(!PlaceCopy::isValid(&project, QStringLiteral("A"), QString(), &why));
		QVERIFY(!PlaceCopy::isValid(&project, QStringLiteral("A"), QStringLiteral(" C"), &why));
		QVERIFY(!PlaceCopy::isValid(&project, QString(), QStringLiteral("C"), &why));
			//Onto an existing plant the copies would take its designations
		QVERIFY(!PlaceCopy::isValid(&project, QStringLiteral("A"), QStringLiteral("B"), &why));
		QVERIFY(why.contains(QStringLiteral("already exists")));
			//A plant no folio's title block holds has nothing to copy
		const int steps = project.undoStack()->count();
		QVERIFY(!run(project, QStringLiteral("NOWHERE"), QStringLiteral("C")));
		QCOMPARE(project.undoStack()->count(), steps);
	}

	void copiesLinkedFoliosInOneUndoStep()
	{
		QETProject project(QFINDTESTDATA("fixtures/linked_reports.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		project.loadFolios();
		QCOMPARE(project.folios().size(), 2);
		Diagram *one = project.folios().at(0);
		Diagram *two = project.folios().at(1);
		setPlant(one, QStringLiteral("A"));
		setPlant(two, QStringLiteral("A"));
		Diagram *other = project.addNewDiagram();
		setPlant(other, QStringLiteral("Z"));
		const QStringList before = places(project);
		const int steps = project.undoStack()->count();

		QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("B")));
		QCOMPARE(project.undoStack()->count(), steps + 1);
		QCOMPARE(project.folios().size(), 5);
			//The copies come right after the plant's last folio, in order
		QCOMPARE(project.folios().at(0), one);
		QCOMPARE(project.folios().at(1), two);
		Diagram *one_copy = project.folios().at(2);
		Diagram *two_copy = project.folios().at(3);
		QCOMPARE(project.folios().at(4), other);
		QCOMPARE(one_copy->border_and_titleblock.plant(), QStringLiteral("B"));
		QCOMPARE(two_copy->border_and_titleblock.plant(), QStringLiteral("B"));
		QCOMPARE(one_copy->border_and_titleblock.title(), one->border_and_titleblock.title());
		QCOMPARE(elementsOf(one_copy).size(), elementsOf(one).size());
		QCOMPARE(elementsOf(two_copy).size(), elementsOf(two).size());
		uuidsAreOwn(project);

			//Every link of a copy goes to a copy, every link of an
			//original to an original
		int copied_links = 0;
		for (Diagram *copy : {one_copy, two_copy})
			for (Element *element : elementsOf(copy))
				for (Element *partner : element->linkedElements()) {
					QVERIFY(partner->diagram() == one_copy || partner->diagram() == two_copy);
					++copied_links;
				}
		QVERIFY(copied_links > 0);
		int original_links = 0;
		for (Diagram *source : {one, two})
			for (Element *element : elementsOf(source))
				for (Element *partner : element->linkedElements()) {
					QVERIFY(partner->diagram() == one || partner->diagram() == two);
					++original_links;
				}
		QCOMPARE(copied_links, original_links);

			//One undo takes every copied folio away; redo brings the same back
		const QStringList after = places(project);
		QVERIFY(after.join(QLatin1Char(' ')).contains(QStringLiteral("B/")));
		project.undoStack()->undo();
		QCOMPARE(project.folios().size(), 3);
		QCOMPARE(places(project), before);
		project.undoStack()->redo();
		QCOMPARE(project.folios().size(), 5);
		QCOMPARE(project.folios().at(2), one_copy);
		QCOMPARE(places(project), after);
		uuidsAreOwn(project);
		int redone_links = 0;
		for (Diagram *copy : {one_copy, two_copy})
			for (Element *element : elementsOf(copy))
				redone_links += int(element->linkedElements().size());
		QCOMPARE(redone_links, copied_links);
	}

	void copiesGoAfterTheLastFolioOfThePlant()
	{
		QETProject project;
		Diagram *a1 = project.addNewDiagram();
		Diagram *z = project.addNewDiagram();
		Diagram *a2 = project.addNewDiagram();
		Diagram *y = project.addNewDiagram();
		setPlant(a1, QStringLiteral("A"), QStringLiteral("first"));
		setPlant(z, QStringLiteral("Z"));
		setPlant(a2, QStringLiteral("A"), QStringLiteral("second"));
		setPlant(y, QStringLiteral("Y"));
			//The folio's own numbering goes with it
		a1->setConductorsAutonumName(QStringLiteral("wires"));
		a1->m_elmt_unitfolio_max.insert(QStringLiteral("symbols"), {QStringLiteral("3")});
		QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("B")));
		const QList<Diagram *> folios = project.folios();
		QCOMPARE(folios.size(), 6);
		QCOMPARE(folios.at(0), a1);
		QCOMPARE(folios.at(1), z);
		QCOMPARE(folios.at(2), a2);
		QCOMPARE(folios.at(3)->border_and_titleblock.title(), QStringLiteral("first"));
		QCOMPARE(folios.at(4)->border_and_titleblock.title(), QStringLiteral("second"));
		QCOMPARE(folios.at(5), y);
		QCOMPARE(folios.at(3)->conductorsAutonumName(), QStringLiteral("wires"));
		QCOMPARE(folios.at(3)->m_elmt_unitfolio_max.value(QStringLiteral("symbols")),
				 QStringList({QStringLiteral("3")}));
	}

	void aLinkOutOfThePlantIsNotCopied()
	{
		QETProject project(QFINDTESTDATA("fixtures/linked_reports.qet"));
		project.loadFolios();
		Diagram *one = project.folios().at(0);
		Diagram *two = project.folios().at(1);
		setPlant(one, QStringLiteral("A"));
		setPlant(two, QStringLiteral("Z"));
		int linked_before = 0;
		for (Element *element : elementsOf(one))
			linked_before += int(element->linkedElements().size());
		QVERIFY(linked_before > 0);

		QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("B")));
		Diagram *copy = project.folios().at(1);
		QCOMPARE(copy->border_and_titleblock.plant(), QStringLiteral("B"));
		for (Element *element : elementsOf(copy)) {
			QVERIFY(element->linkedElements().isEmpty());
				//Nor does it show the partner it lost
			QVERIFY(element->elementInformations().value(QStringLiteral("label")).toString().isEmpty());
			for (DynamicElementTextItem *text : element->dynamicTextItems())
				if (text->textFrom() != DynamicElementTextItem::UserText)
					QVERIFY2(text->toPlainText().isEmpty(), qPrintable(text->toPlainText()));
		}
			//The originals keep their own links
		int linked_after = 0;
		for (Element *element : elementsOf(one))
			linked_after += int(element->linkedElements().size());
		QCOMPARE(linked_after, linked_before);
	}

	void keepsLabelsUnderTheNewPlant()
	{
		QETProject project(QFINDTESTDATA("fixtures/coil_contact_wired.qet"));
		project.loadFolios();
		Diagram *folio = project.folios().first();
		setPlant(folio, QStringLiteral("A"));
		const QStringList before = designations(project);
		QVERIFY(before.join(QLatin1Char(' ')).contains(QStringLiteral("A K1")));

		QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("B")));
		Diagram *copy = project.folios().at(1);
		uuidsAreOwn(project);
		QCOMPARE(copy->conductors().size(), folio->conductors().size());
		QVERIFY(!copy->conductors().isEmpty());
			//The labels are the same; the plant tells them apart
		QStringList expected = before;
		for (const QString &d : before)
			expected << QStringLiteral("B") + d.mid(1);
		expected.sort();
		QCOMPARE(designations(project), expected);
			//The coil's copy drives the contact's copy
		for (Element *element : elementsOf(copy))
			for (Element *partner : element->linkedElements())
				QCOMPARE(partner->diagram(), copy);
	}

	void renamesTheHeldPlantAndCountsOthers()
	{
		QETProject project(QFINDTESTDATA("fixtures/coil_contact_wired.qet"));
		project.loadFolios();
		Diagram *folio = project.folios().first();
		setPlant(folio, QStringLiteral("A"));
		const QList<Element *> elements = elementsOf(folio);
		QVERIFY(elements.size() >= 2);
		auto own = [](Element *element, const QString &plant) {
			auto info = element->elementInformations();
			info.addValue(QStringLiteral("plant"), plant);
			element->setElementInformations(info);
		};
		own(elements.at(0), QStringLiteral("A"));
		own(elements.at(1), QStringLiteral("Z"));
		auto *box = new StructureBoxItem(QPointF(-5000, -5000), QPointF(-4000, -4000));
		box->setPlant(QStringLiteral("A"));
		folio->addItem(box);

		const PlaceCopy::Plan plan = PlaceCopy::collect(&project, QStringLiteral("A"));
		QCOMPARE(plan.folios.size(), 1);
		QCOMPARE(plan.other_plant_symbols, 1);

		QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("B")));
		Diagram *copy = project.folios().at(1);
		QStringList held;
		for (Element *element : elementsOf(copy))
			held << element->elementInformations().value(QStringLiteral("plant")).toString();
		QVERIFY(held.contains(QStringLiteral("B")));
		QVERIFY(held.contains(QStringLiteral("Z")));
		QVERIFY(!held.contains(QStringLiteral("A")));
		const auto boxes = StructureBoxItem::boxesOf(copy);
		QCOMPARE(boxes.size(), 1);
		QCOMPARE(boxes.first()->plant(), QStringLiteral("B"));
		QCOMPARE(StructureBoxItem::boxesOf(folio).first()->plant(), QStringLiteral("A"));
	}

	void givesTheWireCountersToTheCopy()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlant(folio, QStringLiteral("A"));
		const QString scheme = QStringLiteral("s");
		project.setConductorPlantCounter(scheme, QStringLiteral("A"), {QStringLiteral("7")});
		QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("B")));
		QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("B")), QStringList({QStringLiteral("7")}));
		QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("A")), QStringList({QStringLiteral("7")}));
		project.undoStack()->undo();
		QVERIFY(project.conductorPlantCounter(scheme, QStringLiteral("B")).isEmpty());
		project.undoStack()->redo();
		QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("B")), QStringList({QStringLiteral("7")}));
		project.undoStack()->undo();

			//Counters left for a name no folio uses: the higher is kept
		project.setConductorPlantCounter(scheme, QStringLiteral("C"), {QStringLiteral("9")});
		QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("C")));
		QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("C")), QStringList({QStringLiteral("9")}));
		project.undoStack()->undo();
		QCOMPARE(project.conductorPlantCounter(scheme, QStringLiteral("C")), QStringList({QStringLiteral("9")}));
	}

	void survivesSaveAndReload()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath(QStringLiteral("copied.qet"));
		QStringList copied;
		{
			QETProject project(QFINDTESTDATA("fixtures/linked_reports.qet"));
			project.loadFolios();
			for (Diagram *d : project.folios()) setPlant(d, QStringLiteral("A"));
			QVERIFY(run(project, QStringLiteral("A"), QStringLiteral("B")));
			copied = places(project);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QCOMPARE(again.state(), QETProject::Ok);
		again.loadFolios();
		QCOMPARE(places(again), copied);
		QCOMPARE(again.folios().size(), 4);
		uuidsAreOwn(again);
		int links = 0;
		for (int i : {2, 3})
			for (Element *element : elementsOf(again.folios().at(i)))
				for (Element *partner : element->linkedElements()) {
					QVERIFY(partner->diagram() == again.folios().at(2)
							|| partner->diagram() == again.folios().at(3));
					++links;
				}
		QVERIFY(links > 0);
	}

	void dialogCopiesAPlant()
	{
		QETProject project;
		Diagram *a = project.addNewDiagram();
		project.addNewDiagram();
		setPlant(a, QStringLiteral("A"));
		PlantsLocationsDialog dialog(&project);
		auto *table = dialog.findChild<QTableWidget *>(QStringLiteral("placesTable"));
		auto *copy_button = dialog.findChild<QPushButton *>(QStringLiteral("copyPlant"));
		QVERIFY(table && copy_button);
		QCOMPARE(table->rowCount(), 2);
		table->setCurrentCell(0, 0);
		QCOMPARE(table->item(0, 0)->text(), QStringLiteral("(none)"));
		QVERIFY(!copy_button->isEnabled());
		table->setCurrentCell(1, 0);
		QVERIFY(copy_button->isEnabled());

		QVERIFY(dialog.applyCopy(QStringLiteral("A"), QStringLiteral("B")));
		QCOMPARE(project.folios().size(), 3);
		QCOMPARE(table->rowCount(), 3);
		QCOMPARE(table->item(2, 0)->text(), QStringLiteral("B"));
		QVERIFY(!dialog.applyCopy(QStringLiteral("A"), QStringLiteral("B")));
	}
};

QET_APP_TEST_MAIN(tst_placecopy)
#include "tst_placecopy.moc"
