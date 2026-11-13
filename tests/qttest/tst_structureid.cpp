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
#include "autoNum/assignvariables.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetproject.h"
#include "qetresult.h"
#include "titleblockproperties.h"
#include "ui/structureboxpropertieswidget.h"

#include <QDomDocument>
#include <QLineEdit>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

/**
	%{structure_id_full} composes =plant+location-label (IEC 81346). An
	element that has no plant or location of its own takes the one of its
	folio's title block, and every part that is empty is left out.
	%{structure_id} is the same, without the =plant or +location part that
	equals the folio's own. A symbol inside a structure box takes the box's
	plant and location instead (the next box's, then the folio's, where the
	box leaves one empty), for both tokens. %{plant} and %{location} are not affected: they
	show the element's own value only, unless the project's setting
	(QETProject::plantAndLocationInherited()) makes them show that place.
*/
class tst_structureid : public QObject
{
	Q_OBJECT
	QTemporaryDir m_definitions;

	Element *symbol(Diagram *diagram)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/simple.elmt")))
		{
			QDomDocument definition;
			definition.setContent(
				QString(R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(),
						 QUuid::createUuid().toString(),
						 QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(
					QStringLiteral("import"), QStringLiteral("simple"),
					definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/simple.elmt"),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		return element;
	}

	static void setFolio(Diagram *diagram, const QString &plant, const QString &location)
	{
		TitleBlockProperties tb = diagram->border_and_titleblock.exportTitleBlock();
		tb.plant = plant;
		tb.locmach = location;
		diagram->border_and_titleblock.importTitleBlock(tb);
	}

	static DiagramContext info(const QString &label,
							   const QString &plant = {},
							   const QString &location = {})
	{
		DiagramContext dc;
		dc.addValue(QStringLiteral("label"), label);
		if (!plant.isEmpty())    dc.addValue(QStringLiteral("plant"), plant);
		if (!location.isEmpty()) dc.addValue(QStringLiteral("location"), location);
		return dc;
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

	static void setInfo(Element *element, const DiagramContext &values)
	{
		DiagramContext info = element->elementInformations();
		for (const QString &key : values.keys())
			info.addValue(key, values.value(key));
		element->setElementInformations(info);
	}

	static QString text(const QString &formula, const DiagramContext &dc,
						const Element *element)
	{
		return autonum::AssignVariables::replaceVariable(formula, dc, element);
	}

private slots:
	void elementWithoutValuesTakesTheFolios()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);

		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info("X1"), element),
				 QStringLiteral("=FACTORY+CP1-X1"));
	}

	void sameAsTheFolioIsLeftOut()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("E1"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);

			//No own value, or the same value as the folio: just the label
		QCOMPARE(text(QStringLiteral("%{structure_id}"), info("K1"), element),
				 QStringLiteral("-K1"));
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
					  info("K1", QStringLiteral("E1"), QStringLiteral("CP1")), element),
				 QStringLiteral("-K1"));
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"),
					  info("K1", QStringLiteral("E1"), QStringLiteral("CP1")), element),
				 QStringLiteral("=E1+CP1-K1"));
	}

	void differentFromTheFolioIsKept()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("E1"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);

			//Own plant differs: =OTHER kept, +CP1 (the folio's) left out
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
					  info("K1", QStringLiteral("OTHER")), element),
				 QStringLiteral("=OTHER-K1"));
			//Own location differs: +FIELD kept, =E1 left out
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
					  info("K1", {}, QStringLiteral("FIELD")), element),
				 QStringLiteral("+FIELD-K1"));
			//Both differ
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
					  info("K1", QStringLiteral("OTHER"), QStringLiteral("FIELD")), element),
				 QStringLiteral("=OTHER+FIELD-K1"));
			//The full form never leaves a part out
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"),
					  info("K1", QStringLiteral("OTHER")), element),
				 QStringLiteral("=OTHER+CP1-K1"));
	}

	void plantAndLocationTokensAreUnchanged()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("E1"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);

			//As before: the element's own value only, no folio fallback
		QCOMPARE(text(QStringLiteral("%{plant}|%{location}"), info("X1"), element),
				 QStringLiteral("|"));
		QCOMPARE(text(QStringLiteral("%{plant}|%{location}"),
					  info("X1", QStringLiteral("P"), QStringLiteral("L")), element),
				 QStringLiteral("P|L"));
		QCOMPARE(text(QStringLiteral("%{plant}"), info("X1"), element), QString());
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info("X1"), element),
				 QStringLiteral("=E1+CP1-X1"));
	}

	void elementsOwnValueWins()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);

			//A device drawn on the CP1 folio that is mounted in the field
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"),
					  info("S5", {}, QStringLiteral("FIELD")), element),
				 QStringLiteral("=FACTORY+FIELD-S5"));
		QCOMPARE(text(QStringLiteral("%{plant}|%{location}"),
					  info("S5", {}, QStringLiteral("FIELD")), element),
				 QStringLiteral("|FIELD"));
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
					  info("S5", QStringLiteral("SILO"), QStringLiteral("CP1000")), element),
				 QStringLiteral("=SILO+CP1000-S5"));
	}

	void emptyPartsAreLeftOut()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, {}, QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);

		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info("K1"), element),
				 QStringLiteral("+CP1-K1"));
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info({}), element),
				 QStringLiteral("+CP1"));
		QCOMPARE(text(QStringLiteral("%{structure_id}"), info({}), element),
				 QString());
	}

	void aSignTypedWithTheValueIsNotDoubled()
	{
			//Many people type the label as "-K1": one sign, not "--K1",
			//in the drawing and in the database alike
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("=FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		setInfo(element, info("-K1", {}, QStringLiteral("+FIELD")));

		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), element->elementInformations(), element),
				 QStringLiteral("=FACTORY+FIELD-K1"));
		QCOMPARE(text(QStringLiteral("%{structure_id}"), element->elementInformations(), element),
				 QStringLiteral("+FIELD-K1"));

		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id FROM qet_devices WHERE device_uuid = '%1'")
				.arg(element->uuid().toString()), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QVERIFY2(query.next(), "the element is not in qet_devices");
		QCOMPARE(query.value(0).toString(), QStringLiteral("=FACTORY+FIELD-K1"));
	}

	void databaseSaysWhatTheDrawingSays()
	{
			//qet_devices.structure_id is the full designation,
			//=plant+location-label; plant and location in the view fall back
			//to the folio, where %{plant} / %{location} show the element's own.
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *on_folio = symbol(diagram);
		Element *in_field = symbol(diagram);
		QVERIFY(on_folio && in_field);
		setInfo(on_folio, info("X1"));
		setInfo(in_field, info("S5", {}, QStringLiteral("FIELD")));

		project.dataBase()->updateDB();
		for (Element *element : {on_folio, in_field})
		{
			QString error;
			QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
				"SELECT plant, location, structure_id FROM qet_devices WHERE device_uuid = '%1'")
					.arg(element->uuid().toString()), &error);
			QVERIFY2(error.isEmpty(), qPrintable(error));
			QVERIFY2(query.next(), "the element is not in qet_devices");
			QCOMPARE(query.value(0).toString(), QStringLiteral("FACTORY"));
			QCOMPARE(query.value(1).toString(), element == on_folio ? QStringLiteral("CP1")
																   : QStringLiteral("FIELD"));
			QCOMPARE(query.value(2).toString(), element == on_folio
					 ? QStringLiteral("=FACTORY+CP1-X1")
					 : QStringLiteral("=FACTORY+FIELD-S5"));
		}
	}

	void symbolInsideABoxTakesTheBox()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		box(diagram, QRectF(0, 0, 400, 400), {}, QStringLiteral("FIELD"));
		Element *inside = symbol(diagram);
		Element *outside = symbol(diagram);
		QVERIFY(inside && outside);
			//In the middle of the box, far from its outline
		inside->setPos(200, 200);
		outside->setPos(800, 800);

			//The box gives +FIELD only: the =plant stays the folio's
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info("S5"), inside),
				 QStringLiteral("=FACTORY+FIELD-S5"));
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info("K1"), outside),
				 QStringLiteral("=FACTORY+CP1-K1"));
	}

	void smallestBoxWins()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		box(diagram, QRectF(0, 0, 800, 800), QStringLiteral("SILO"), QStringLiteral("CP1000"));
		box(diagram, QRectF(100, 100, 200, 200), QStringLiteral("SILO"), QStringLiteral("J3"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		element->setPos(200, 200);

		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info("M1"), element),
				 QStringLiteral("=SILO+J3-M1"));
	}

	void anEmptyFieldOfABoxIsTheNextOnes()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		box(diagram, QRectF(0, 0, 800, 800), QStringLiteral("SILO"), QStringLiteral("CP1000"));
		box(diagram, QRectF(100, 100, 200, 200), {}, QStringLiteral("J3"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		element->setPos(200, 200);

			//+J3 from the inner box, =SILO from the outer one
		QCOMPARE(text(QStringLiteral("%{structure_id_full}"), info("M1"), element),
				 QStringLiteral("=SILO+J3-M1"));
	}

		//The public rule other features use (terminal strip manager, checks):
		//the boxes a symbol is in, the place they give it, and its own place
	void thePublicRuleSaysWhereASymbolIs()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		const StructureBoxItem *outer = box(diagram, QRectF(0, 0, 800, 800), QStringLiteral("SILO"), QString());
		const StructureBoxItem *inner = box(diagram, QRectF(100, 100, 200, 200), QString(), QStringLiteral("J3"));
		Element *in_both = symbol(diagram);
		Element *own = symbol(diagram);
		Element *outside = symbol(diagram);
		QVERIFY(in_both && own && outside);
		in_both->setPos(200, 200);
		own->setPos(210, 210);
		setInfo(own, info("X1", {}, QStringLiteral("DB50")));
		outside->setPos(1200, 1200);

		QCOMPARE(StructureBoxItem::enclosing(in_both),
				 (QList<const StructureBoxItem *>{inner, outer}));
		QVERIFY(StructureBoxItem::enclosing(outside).isEmpty());
		QVERIFY(StructureBoxItem::enclosing(nullptr).isEmpty());

		const StructureBoxItem::Place reference = StructureBoxItem::referencePlace(in_both);
		QCOMPARE(reference.plant, QStringLiteral("SILO"));
		QCOMPARE(reference.location, QStringLiteral("J3"));

			//Its own location wins, the empty plant comes from the boxes
		const StructureBoxItem::Place place = StructureBoxItem::placeOf(own);
		QCOMPARE(place.plant, QStringLiteral("SILO"));
		QCOMPARE(place.location, QStringLiteral("DB50"));

			//Outside every box: the folio
		const StructureBoxItem::Place folio = StructureBoxItem::placeOf(outside);
		QCOMPARE(folio.plant, QStringLiteral("FACTORY"));
		QCOMPARE(folio.location, QStringLiteral("CP1"));
	}

	void ownValueBeatsTheBox()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		box(diagram, QRectF(0, 0, 400, 400), QStringLiteral("FACTORY"), QStringLiteral("FIELD"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		element->setPos(200, 200);

		QCOMPARE(text(QStringLiteral("%{structure_id_full}"),
					  info("S5", {}, QStringLiteral("DB50")), element),
				 QStringLiteral("=FACTORY+DB50-S5"));
	}

	void plantAndLocationStayTheElementsOwnInsideABox()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		box(diagram, QRectF(0, 0, 400, 400), QStringLiteral("FACTORY"), QStringLiteral("FIELD"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		element->setPos(200, 200);

		QCOMPARE(text(QStringLiteral("%{plant}|%{location}"), info("S5"), element),
				 QStringLiteral("|"));
	}

		//Behind the project's setting, %{plant} and %{location} give the
		//place the element takes: its own, the box's, else the folio's
	void plantAndLocationFollowThePlaceWhenTheProjectSaysSo()
	{
		QETProject project;
		project.setPlantAndLocationInherited(true);
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		box(diagram, QRectF(0, 0, 400, 400), {}, QStringLiteral("FIELD"));
		Element *inside = symbol(diagram);
		Element *outside = symbol(diagram);
		QVERIFY(inside && outside);
		inside->setPos(200, 200);
		outside->setPos(800, 800);

		const QString place = QStringLiteral("%{plant}|%{location}");
			//The box gives +FIELD only: the =plant stays the folio's
		QCOMPARE(text(place, info("S5"), inside), QStringLiteral("FACTORY|FIELD"));
		QCOMPARE(text(place, info("S5", {}, QStringLiteral("DB50")), inside),
				 QStringLiteral("FACTORY|DB50"));
		QCOMPARE(text(place, info("K1"), outside), QStringLiteral("FACTORY|CP1"));
		QCOMPARE(text(place, info("K1", QStringLiteral("SILO")), outside),
				 QStringLiteral("SILO|CP1"));
	}

		//Switching the setting redraws the texts that use the variables, and
		//it is saved only when on, so a project with it off saves as before
	void theSettingRedrawsTextsAndIsSavedOnlyWhenOn()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		setInfo(element, info("K1"));
		auto *text = new DynamicElementTextItem(element);
		text->setTextFrom(DynamicElementTextItem::CompositeText);
		text->setCompositeText(QStringLiteral("%{plant}|%{location}"));
		element->addDynamicTextItem(text);
		QCOMPARE(text->toPlainText(), QStringLiteral("|"));
		QVERIFY(!project.toXml().toString().contains(QLatin1String("plant_location")));

		project.setPlantAndLocationInherited(true);
		QCOMPARE(text->toPlainText(), QStringLiteral("FACTORY|CP1"));
			//And it follows the title block from then on
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP2"));
		QCOMPARE(text->toPlainText(), QStringLiteral("FACTORY|CP2"));

		for (const QString &name : {QStringLiteral("inherited.qet"),
									QStringLiteral("inherited.qetz")}) {
			project.setFilePath(dir.filePath(name));
			QVERIFY2(project.write().isOk(), qPrintable(name));
			QETProject reopened(project.filePath());
			QVERIFY2(reopened.plantAndLocationInherited(), qPrintable(name));
		}

		project.setPlantAndLocationInherited(false);
		QCOMPARE(text->toPlainText(), QStringLiteral("|"));
	}

	void shortFormLeavesOutWhatTheBoxSays()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		box(diagram, QRectF(0, 0, 400, 400), QStringLiteral("SILO"), QStringLiteral("FIELD"));
		Element *inside = symbol(diagram);
		Element *outside = symbol(diagram);
		QVERIFY(inside && outside);
		inside->setPos(200, 200);
		outside->setPos(800, 800);

			//Inside the box, the box's =SILO+FIELD is implied
		QCOMPARE(text(QStringLiteral("%{structure_id}"), info("S5"), inside),
				 QStringLiteral("-S5"));
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
					  info("S5", {}, QStringLiteral("DB50")), inside),
				 QStringLiteral("+DB50-S5"));
			//Outside it, the folio's =FACTORY+CP1 is
		QCOMPARE(text(QStringLiteral("%{structure_id}"), info("K1"), outside),
				 QStringLiteral("-K1"));
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
					  info("K1", QStringLiteral("SILO")), outside),
				 QStringLiteral("=SILO-K1"));
	}

	void boxSurvivesSaveAndReload()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		box(diagram, QRectF(10, 20, 300, 200), QStringLiteral("SILO"), QStringLiteral("J3"));
		diagram->addItem(new QetShapeItem(QPointF(0, 0), QPointF(50, 50), QetShapeItem::Rectangle));
		QDomDocument saved = diagram->toXml();

		QETProject other;
		Diagram *reloaded = other.addNewDiagram();
		QVERIFY(reloaded->fromXml(saved));
		int boxes = 0, shapes = 0;
		for (QGraphicsItem *item : reloaded->items()) {
			if (item->type() == StructureBoxItem::Type) {
				auto *b = static_cast<StructureBoxItem *>(item);
				QCOMPARE(b->plant(), QStringLiteral("SILO"));
				QCOMPARE(b->location(), QStringLiteral("J3"));
				QCOMPARE(b->mapRectToScene(b->rect().normalized()), QRectF(10, 20, 300, 200));
				++boxes;
			}
			else if (item->type() == QetShapeItem::Type)
				++shapes;
		}
		QCOMPARE(boxes, 1);
		QCOMPARE(shapes, 1);	//a plain rectangle stays a plain rectangle
	}

		//A new box is drawn dash-dot, a location boundary (IEC 60617);
		//its line is saved with it, and a plain rectangle stays solid
	void aNewBoxIsDashDot()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		StructureBoxItem *b = box(diagram, QRectF(10, 20, 300, 200), QStringLiteral("SILO"), QString());
		QCOMPARE(b->pen().style(), Qt::DashDotLine);
		QCOMPARE(QetShapeItem(QPointF(0, 0), QPointF(50, 50), QetShapeItem::Rectangle).pen().style(),
				 Qt::SolidLine);

		QPen solid = b->pen();
		solid.setStyle(Qt::SolidLine);
		b->setPen(solid);
		QDomDocument saved = diagram->toXml();
		QETProject other;
		Diagram *reloaded = other.addNewDiagram();
		QVERIFY(reloaded->fromXml(saved));
		for (QGraphicsItem *item : reloaded->items())
			if (item->type() == StructureBoxItem::Type)
				QCOMPARE(static_cast<StructureBoxItem *>(item)->pen().style(), Qt::SolidLine);
	}

		//A text showing the reference designation follows the boxes: drawn
		//around the symbol, given a location, moved away, deleted, and a
		//project read back with the box (boxes load after the symbols)
	void aSymbolTextFollowsTheBox()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		element->setPos(200, 200);
		setInfo(element, info("K1"));
		auto *text = new DynamicElementTextItem(element);
		text->setTextFrom(DynamicElementTextItem::CompositeText);
		text->setCompositeText(QStringLiteral("%{structure_id_full}"));
		element->addDynamicTextItem(text);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));

		StructureBoxItem *b = box(diagram, QRectF(0, 0, 400, 400), QString(), QString());
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));
		b->setLocation(QStringLiteral("F1"));
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+F1-K1"));
		b->setPos(1000, 1000);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));
		b->setPos(0, 0);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+F1-K1"));

			//Read back (into the same project, which holds the symbol's
			//definition): the symbol is built before the box
		QDomDocument saved = diagram->toXml();
		Diagram *reloaded = project.addNewDiagram();
		QVERIFY(reloaded->fromXml(saved));
		bool found = false;
		for (Element *e : reloaded->elements())
			for (DynamicElementTextItem *t : e->dynamicTextItems())
				if (t->textFrom() == DynamicElementTextItem::CompositeText) {
					QCOMPARE(t->toPlainText(), QStringLiteral("=FACTORY+F1-K1"));
					found = true;
				}
		QVERIFY(found);

		diagram->removeItem(b);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));
		delete b;
	}

	void boxIdentityIsInTheDatabase()
	{
			//Its row is what a .qetdb saves: it must carry the identity
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		StructureBoxItem *b = box(diagram, QRectF(0, 0, 100, 100),
								  QStringLiteral("SILO"), QStringLiteral("J3"));
		b->setLocation(QStringLiteral("J4"));	//a change after it was added

		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT name, value FROM drawing_item WHERE item_uuid = '%1' "
			"AND name IN ('structure_box', 'plant', 'location')").arg(b->uuid().toString()), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QHash<QString, QString> values;
		while (query.next()) values.insert(query.value(0).toString(), query.value(1).toString());
		QCOMPARE(values.value(QStringLiteral("structure_box")), QStringLiteral("true"));
		QCOMPARE(values.value(QStringLiteral("plant")), QStringLiteral("SILO"));
		QCOMPARE(values.value(QStringLiteral("location")), QStringLiteral("J4"));
	}

	void theSidePanelAppliesALeftField()
	{
			//In the Selection properties panel there is no Apply button:
			//leaving a field applies it
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		StructureBoxItem *b = box(diagram, QRectF(0, 0, 100, 100), {}, QStringLiteral("J3"));
		StructureBoxPropertiesWidget editor(b);
		QVERIFY(editor.setLiveEdit(true));
		for (QLineEdit *edit : editor.findChildren<QLineEdit *>()) {
			if (edit->text() == QLatin1String("J3")) {
				edit->setText(QStringLiteral("F1"));
				emit edit->editingFinished();
			}
		}
		QCOMPARE(b->location(), QStringLiteral("F1"));
		diagram->undoStack().undo();
		QCOMPARE(b->location(), QStringLiteral("J3"));
	}

	void editingABoxCanBeUndone()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		StructureBoxItem *b = box(diagram, QRectF(0, 0, 100, 100),
								  QStringLiteral("SILO"), QStringLiteral("J3"));
		StructureBoxPropertiesWidget editor(b);
		for (QLineEdit *edit : editor.findChildren<QLineEdit *>()) {
			if (edit->text() == QLatin1String("J3")) edit->setText(QStringLiteral("CP1000"));
		}
		editor.apply();
		QCOMPARE(b->location(), QStringLiteral("CP1000"));
		diagram->undoStack().undo();
		QCOMPARE(b->location(), QStringLiteral("J3"));
		QCOMPARE(b->plant(), QStringLiteral("SILO"));
	}

		//A symbol's text shows the short form against its folio: it
		//follows the folio's plant and location when the title block is
		//changed, not only when the symbol changes
		//A symbol moved into or out of a box: its text follows. Dragging,
		//the arrow keys and an undo all move it by setting its position
	void aSymbolTextFollowsTheSymbol()
	{
		QETProject project;
		project.setPlantAndLocationInherited(true);
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		box(diagram, QRectF(0, 0, 400, 400), {}, QStringLiteral("CP10"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		setInfo(element, info("K10"));
		auto *text = new DynamicElementTextItem(element);
		text->setTextFrom(DynamicElementTextItem::CompositeText);
		text->setCompositeText(QStringLiteral("%{structure_id_full} %{location}"));
		element->addDynamicTextItem(text);

		element->setPos(800, 800);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP1-K10 CP1"));
		element->setPos(200, 200);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP10-K10 CP10"));
		element->setPos(800, 800);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP1-K10 CP1"));
	}

	void aSymbolTextFollowsTheTitleBlock()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Element *element = symbol(diagram);
		QVERIFY(element);
		setInfo(element, info("K1", QStringLiteral("FACTORY"), QStringLiteral("CP1")));
		auto *text = new DynamicElementTextItem(element);
		text->setTextFrom(DynamicElementTextItem::CompositeText);
		text->setCompositeText(QStringLiteral("%{structure_id}"));
		element->addDynamicTextItem(text);
		QCOMPARE(text->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));

		setFolio(diagram, QStringLiteral("FACTORY"), QString());
		QCOMPARE(text->toPlainText(), QStringLiteral("+CP1-K1"));
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		QCOMPARE(text->toPlainText(), QStringLiteral("-K1"));
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP2"));
		QCOMPARE(text->toPlainText(), QStringLiteral("+CP1-K1"));
	}

	void withoutAnElementOnlyItsOwnValuesCount()
	{
			//Callers with no element (e.g. a symbol in the element editor)
			//keep the old behaviour: no folio to fall back to.
		QCOMPARE(text(QStringLiteral("%{plant}|%{location}|%{structure_id}"
									 "|%{structure_id_full}"),
					  info("Q1"), nullptr),
				 QStringLiteral("||-Q1|-Q1"));
		QCOMPARE(text(QStringLiteral("%{structure_id}|%{structure_id_full}"),
					  info("Q1", QStringLiteral("P"), QStringLiteral("L")), nullptr),
				 QStringLiteral("=P+L-Q1|=P+L-Q1"));
	}
};

QET_APP_TEST_MAIN(tst_structureid)
#include "tst_structureid.moc"
