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
#include "ElementsCollection/elementslocation.h"
#include "dataBase/projectdatabase.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "duplicatelabels.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetproject.h"
#include "qetresult.h"
#include "titleblockproperties.h"

#include <QDomDocument>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

/**
	The project says what the label field of a symbol shows
	(QETProject::designationDisplay(), discussion #1878 item C5): the label
	only, as before; the short IEC 81346 reference designation, without
	the =plant and +location its structure box or folio already gives; or
	the full one. The label itself is never changed, and a text written
	with %{label} or %{structure_id} shows what it always did.
*/
class tst_designationdisplay : public QObject
{
	Q_OBJECT

	using Display = QETProject::DesignationDisplay;

	Element *symbol(Diagram *diagram, const QString &kind = QStringLiteral("simple"))
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(kind)))
		{
			QDomDocument definition;
			definition.setContent(QString(
R"(<definition type="element" link_type="%1" uuid="%2" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<kindInformations><kindInformation name="type">%5</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%3"/><terminal x="0" y="20" orientation="s" uuid="%4"/>
</description></definition>)")
				.arg(kind,
					 QUuid::createUuid().toString(),
					 QUuid::createUuid().toString(),
					 QUuid::createUuid().toString(),
					 kind == QLatin1String("slave") ? QStringLiteral("simple")
													: QStringLiteral("coil")));
			if (!collection->addElementDefinition(
					QStringLiteral("import"), kind, definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(kind),
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

	static void setHigherLevels(Diagram *diagram, const QString &higher_plant,
								const QString &higher_location)
	{
		TitleBlockProperties tb = diagram->border_and_titleblock.exportTitleBlock();
		tb.higher_plant = higher_plant;
		tb.higher_location = higher_location;
		diagram->border_and_titleblock.importTitleBlock(tb);
	}

	static QString deviceRow(QETProject &project, Element *element)
	{
		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id FROM qet_devices WHERE device_uuid = '%1'")
				.arg(element->uuid().toString()), &error);
		if (!error.isEmpty() || !query.next()) return QStringLiteral("<no row: %1>").arg(error);
		return query.value(0).toString();
	}

	static void setInfo(Element *element, const QString &label,
						const QString &plant = {}, const QString &location = {})
	{
		DiagramContext info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.addValue(QStringLiteral("plant"), plant);
		info.addValue(QStringLiteral("location"), location);
		element->setElementInformations(info);
	}

		//A text field bound to the label, as a symbol's label field is
	static DynamicElementTextItem *labelField(Element *element)
	{
		auto *text = new DynamicElementTextItem(element);
		text->setTextFrom(DynamicElementTextItem::ElementInfo);
		text->setInfoName(QStringLiteral("label"));
		element->addDynamicTextItem(text);
		return text;
	}

	static DynamicElementTextItem *composite(Element *element, const QString &formula)
	{
		auto *text = new DynamicElementTextItem(element);
		text->setTextFrom(DynamicElementTextItem::CompositeText);
		text->setCompositeText(formula);
		element->addDynamicTextItem(text);
		return text;
	}

private slots:
		//The three choices, and back to the label only
	void theLabelFieldShowsWhatTheProjectSays()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *same = symbol(diagram);
		Element *other = symbol(diagram);
		QVERIFY(same && other);
		setInfo(same, QStringLiteral("K1"));
		setInfo(other, QStringLiteral("K2"), {}, QStringLiteral("DB50"));
		DynamicElementTextItem *same_field = labelField(same);
		DynamicElementTextItem *other_field = labelField(other);

		QCOMPARE(project.designationDisplay(), Display::Label);
		QCOMPARE(same_field->toPlainText(), QStringLiteral("K1"));
		QCOMPARE(other_field->toPlainText(), QStringLiteral("K2"));

		project.setDesignationDisplay(Display::Short);
		QCOMPARE(same_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(other_field->toPlainText(), QStringLiteral("+DB50-K2"));

		project.setDesignationDisplay(Display::Full);
		QCOMPARE(same_field->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));
		QCOMPARE(other_field->toPlainText(), QStringLiteral("=FACTORY+DB50-K2"));

		project.setDesignationDisplay(Display::Label);
		QCOMPARE(same_field->toPlainText(), QStringLiteral("K1"));
		QCOMPARE(other_field->toPlainText(), QStringLiteral("K2"));

			//The label itself is never changed
		QCOMPARE(same->elementInformations().value(QStringLiteral("label")).toString(),
				 QStringLiteral("K1"));
	}

		//A text which builds the designation itself does not show it twice
	void compositeTextsAreUnchanged()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		setInfo(element, QStringLiteral("K1"));
		DynamicElementTextItem *label = composite(element, QStringLiteral("%{label}"));
		DynamicElementTextItem *short_id = composite(element, QStringLiteral("%{structure_id}"));
		DynamicElementTextItem *full_id = composite(element, QStringLiteral("%{structure_id_full}"));

		for (Display display : {Display::Label, Display::Short, Display::Full}) {
			project.setDesignationDisplay(display);
			QCOMPARE(label->toPlainText(), QStringLiteral("K1"));
			QCOMPARE(short_id->toPlainText(), QStringLiteral("-K1"));
			QCOMPARE(full_id->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));
		}
	}

		//A symbol without a label shows nothing, not a bare place
	void anEmptyLabelStaysEmpty()
	{
		QETProject project;
		project.setDesignationDisplay(Display::Full);
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		DynamicElementTextItem *field = labelField(element);
		QCOMPARE(field->toPlainText(), QString());
	}

		//The field follows the box it is in and the folio's title block
	void theLabelFieldFollowsTheBoxAndTheFolio()
	{
		QETProject project;
		project.setDesignationDisplay(Display::Full);
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *element = symbol(diagram);
		QVERIFY(element);
		element->setPos(200, 200);
		setInfo(element, QStringLiteral("K1"));
		DynamicElementTextItem *field = labelField(element);
		QCOMPARE(field->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));

		auto *box = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		diagram->addItem(box);
		box->setLocation(QStringLiteral("F1"));
		QCOMPARE(field->toPlainText(), QStringLiteral("=FACTORY+F1-K1"));
		element->setPos(800, 800);
		QCOMPARE(field->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));

		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP2"));
		QCOMPARE(field->toPlainText(), QStringLiteral("=FACTORY+CP2-K1"));
	}

		//Item C7 of #1878: the prefix of a structure box is the product
		//above the symbols in it (-A1-K1), the boxes nesting (-B2-A1-K1)
	void aBoxPrefixIsTheProductAboveTheLabel()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		auto *outer = new StructureBoxItem(QPointF(0, 0), QPointF(800, 800));
		auto *inner = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		diagram->addItem(outer);
		diagram->addItem(inner);
		Element *deep = symbol(diagram);    // in both boxes
		Element *middle = symbol(diagram);  // in the outer one only
		Element *none = symbol(diagram);    // in no box
		QVERIFY(deep && middle && none);
		deep->setPos(200, 200);
		middle->setPos(600, 600);
		none->setPos(1200, 1200);
		for (Element *e : {deep, middle, none})
			setInfo(e, QStringLiteral("K1"));
		DynamicElementTextItem *deep_field = labelField(deep);
		DynamicElementTextItem *middle_field = labelField(middle);
		DynamicElementTextItem *none_field = labelField(none);
		DynamicElementTextItem *deep_text = composite(deep, QStringLiteral("%{structure_id_full}"));

			//No prefix anywhere: as before
		project.setDesignationDisplay(Display::Full);
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));

			//A box typed with its sign or without it, an empty one skipped
		outer->setPrefix(QStringLiteral("B2"));
		inner->setPrefix(QStringLiteral("-A1"));
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("=FACTORY+CP1-B2-A1-K1"));
		QCOMPARE(deep_text->toPlainText(), QStringLiteral("=FACTORY+CP1-B2-A1-K1"));
		QCOMPARE(middle_field->toPlainText(), QStringLiteral("=FACTORY+CP1-B2-K1"));
		QCOMPARE(none_field->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));
		inner->setPrefix(QString());
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("=FACTORY+CP1-B2-K1"));
		inner->setPrefix(QStringLiteral("A1"));

			//The short form leaves out what the box it is drawn in gives;
			//the label itself is untouched
		project.setDesignationDisplay(Display::Short);
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(none_field->toPlainText(), QStringLiteral("-K1"));
		project.setDesignationDisplay(Display::Label);
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("K1"));
		QCOMPARE(deep->elementInformations().value(QStringLiteral("label")).toString(),
				 QStringLiteral("K1"));

			//A label typed in full is not doubled, and -A10 is not -A1
		project.setDesignationDisplay(Display::Full);
		setInfo(deep, QStringLiteral("-B2-A1-K1"));
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("=FACTORY+CP1-B2-A1-K1"));
		setInfo(deep, QStringLiteral("A10"));
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("=FACTORY+CP1-B2-A1-A10"));
		setInfo(deep, QString());
		QCOMPARE(deep_field->toPlainText(), QString());
		setInfo(deep, QStringLiteral("K1"));

			//The database says what the texts say
		project.dataBase()->updateDB();
		for (Element *e : {deep, middle, none}) {
			QString error;
			QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
				"SELECT structure_id FROM qet_devices WHERE device_uuid = '%1'")
					.arg(e->uuid().toString()), &error);
			QVERIFY2(error.isEmpty(), qPrintable(error));
			QVERIFY(query.next());
			QCOMPARE(query.value(0).toString(),
					 e->designationForDisplay(QStringLiteral("K1")));
		}
		QCOMPARE(deep->designationForDisplay(QStringLiteral("-B2-A1-K1")),
				 QStringLiteral("=FACTORY+CP1-B2-A1-K1"));
	}

		//A contact on another folio shows its coil with the levels of the
		//box the coil is in, which its own folio does not give it
	void aContactShowsTheBoxProductOfItsCoil()
	{
		QETProject project;
		Diagram *coil_folio = project.addNewDiagram();
		Diagram *contact_folio = project.addNewDiagram();
		setFolio(coil_folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setFolio(contact_folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		auto *box = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		coil_folio->addItem(box);
		box->setPrefix(QStringLiteral("A1"));
		Element *coil = symbol(coil_folio, QStringLiteral("master"));
		Element *contact = symbol(contact_folio, QStringLiteral("slave"));
		QVERIFY(coil && contact);
		coil->setPos(200, 200);
		setInfo(coil, QStringLiteral("K1"));
		DynamicElementTextItem *coil_field = labelField(coil);
		DynamicElementTextItem *contact_field = labelField(contact);
		coil->linkToElement(contact);

		project.setDesignationDisplay(Display::Short);
		QCOMPARE(coil_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(contact_field->toPlainText(), QStringLiteral("-A1-K1"));
	}

		//Item C7 of #1878: a folio's higher-level function (==) and
		//higher-level location (++) go above its plant and location:
		//==A1=FACTORY++S1+CP1-K1
	void theFolioHigherLevelsGoAboveThePlantAndTheLocation()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		auto *box = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		diagram->addItem(box);
		Element *boxed = symbol(diagram);
		Element *own = symbol(diagram);
		QVERIFY(boxed && own);
		boxed->setPos(200, 200);
		own->setPos(1200, 1200);
		setInfo(boxed, QStringLiteral("K1"));
		setInfo(own, QStringLiteral("K2"), QStringLiteral("P2"));
		DynamicElementTextItem *boxed_field = labelField(boxed);
		DynamicElementTextItem *own_field = labelField(own);
		DynamicElementTextItem *boxed_text = composite(boxed, QStringLiteral("%{structure_id_full}"));
		DynamicElementTextItem *own_short = composite(own, QStringLiteral("%{structure_id}"));

			//None set: as before, in the texts and the database
		project.setDesignationDisplay(Display::Full);
		QCOMPARE(boxed_field->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));
		QCOMPARE(deviceRow(project, boxed), QStringLiteral("=FACTORY+CP1-K1"));

			//Typed with their signs or without them
		setHigherLevels(diagram, QStringLiteral("A1"), QStringLiteral("++S1"));
		box->setPrefix(QStringLiteral("B2"));
		QCOMPARE(boxed_field->toPlainText(), QStringLiteral("==A1=FACTORY++S1+CP1-B2-K1"));
		QCOMPARE(boxed_text->toPlainText(), QStringLiteral("==A1=FACTORY++S1+CP1-B2-K1"));
		QCOMPARE(own_field->toPlainText(), QStringLiteral("==A1=P2++S1+CP1-K2"));

			//The short form leaves them out with the folio's plant and
			//location; an own plant says which == it is under
		project.setDesignationDisplay(Display::Short);
		QCOMPARE(boxed_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(own_field->toPlainText(), QStringLiteral("==A1=P2-K2"));
		QCOMPARE(own_short->toPlainText(), QStringLiteral("==A1=P2-K2"));

			//A folio with no plant still has its == level
		project.setDesignationDisplay(Display::Full);
		setFolio(diagram, QString(), QStringLiteral("CP1"));
		QCOMPARE(boxed_field->toPlainText(), QStringLiteral("==A1++S1+CP1-B2-K1"));
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));

			//The database says what the texts say
		for (Element *e : {boxed, own}) {
			const QString label = e->elementInformations().value(QStringLiteral("label")).toString();
			QCOMPARE(deviceRow(project, e), e->designationForDisplay(label));
		}

			//The duplicate check: -K1 under ==A1 and under ==A2 are two devices
		QCOMPARE(DuplicateLabels::place(boxed), QStringLiteral("==A1=FACTORY++S1+CP1-B2"));
	}

		//Item C7 of #1878: a structure box's == and ++ go over the folio's,
		//field by field, the smallest box first; saved only when set
	void aBoxHigherLevelGoesOverTheFolios()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setHigherLevels(diagram, QStringLiteral("A1"), QStringLiteral("S1"));
		auto *outer = new StructureBoxItem(QPointF(0, 0), QPointF(800, 800));
		auto *inner = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		diagram->addItem(outer);
		diagram->addItem(inner);
		Element *deep = symbol(diagram);    // in both boxes
		Element *middle = symbol(diagram);  // in the outer one only
		Element *none = symbol(diagram);    // in no box
		QVERIFY(deep && middle && none);
		deep->setPos(200, 200);
		middle->setPos(600, 600);
		none->setPos(1200, 1200);
		for (Element *e : {deep, middle, none})
			setInfo(e, QStringLiteral("K1"));
		DynamicElementTextItem *deep_field = labelField(deep);
		DynamicElementTextItem *middle_field = labelField(middle);
		DynamicElementTextItem *none_field = labelField(none);
		DynamicElementTextItem *deep_text = composite(deep, QStringLiteral("%{structure_id_full}"));
		project.setDesignationDisplay(Display::Full);

			//None on the boxes: the folio's, and the boxes save as before
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("==A1=FACTORY++S1+CP1-K1"));
		QDomDocument document;
		QVERIFY(!outer->toXml(document).hasAttribute(QStringLiteral("higher_plant")));
		QVERIFY(!outer->toXml(document).hasAttribute(QStringLiteral("higher_location")));

			//The outer box's == over the folio's; the inner box's ++ over
			//both; the folio's ++ still for the symbol in the outer box only
		outer->setHigherPlant(QStringLiteral("A2"));
		inner->setHigherLocation(QStringLiteral("++S2"));
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("==A2=FACTORY++S2+CP1-K1"));
		QCOMPARE(deep_text->toPlainText(), QStringLiteral("==A2=FACTORY++S2+CP1-K1"));
		QCOMPARE(middle_field->toPlainText(), QStringLiteral("==A2=FACTORY++S1+CP1-K1"));
		QCOMPARE(none_field->toPlainText(), QStringLiteral("==A1=FACTORY++S1+CP1-K1"));
			//The box's own name says them too
		inner->setPrefix(QStringLiteral("A1"));
		QCOMPARE(inner->structureId(), QStringLiteral("++S2-A1"));

			//The short form leaves out what the box gives
		project.setDesignationDisplay(Display::Short);
		QCOMPARE(deep_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(none_field->toPlainText(), QStringLiteral("-K1"));

			//The duplicate check: -K1 under ==A2 is not -K1 under ==A1
		QCOMPARE(DuplicateLabels::place(middle), QStringLiteral("==A2=FACTORY++S1+CP1"));
		QCOMPARE(DuplicateLabels::place(none), QStringLiteral("==A1=FACTORY++S1+CP1"));

			//The database says what the texts say, and gives the levels
		project.setDesignationDisplay(Display::Full);
		for (Element *e : {deep, middle, none})
			QCOMPARE(deviceRow(project, e), e->designationForDisplay(QStringLiteral("K1")));
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT higher_plant, higher_location FROM qet_devices WHERE device_uuid = '%1'")
				.arg(deep->uuid().toString()), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QVERIFY(query.next());
		QCOMPARE(query.value(0).toString(), QStringLiteral("A2"));
		QCOMPARE(query.value(1).toString(), QStringLiteral("S2"));
		query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id FROM qet_locations WHERE symbols > 0 ORDER BY structure_id"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QStringList places;
		while (query.next()) places << query.value(0).toString();
		QCOMPARE(places, QStringList({QStringLiteral("==A1=FACTORY++S1+CP1"),
									  QStringLiteral("==A2=FACTORY++S1+CP1"),
									  QStringLiteral("==A2=FACTORY++S2+CP1")}));

			//Saved with the box, read back
		const QDomElement saved = inner->toXml(document);
		QCOMPARE(saved.attribute(QStringLiteral("higher_location")), QStringLiteral("++S2"));
		StructureBoxItem read(QPointF(0, 0));
		QVERIFY(read.fromXml(saved));
		QCOMPARE(read.higherLocation(), QStringLiteral("++S2"));
		QVERIFY(read.higherPlant().isEmpty());
	}

		//Item C7 of #1878: a symbol's own == and ++ (its information) go
		//over its box's and its folio's, field by field
	void aSymbolsOwnHigherLevelGoesOverItsBoxAndFolio()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		setFolio(diagram, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setHigherLevels(diagram, QStringLiteral("A1"), QStringLiteral("S1"));
		auto *b = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		diagram->addItem(b);
		b->setHigherPlant(QStringLiteral("A2"));
		Element *own = symbol(diagram);
		Element *other = symbol(diagram);
		QVERIFY(own && other);
		own->setPos(100, 100);
		other->setPos(300, 300);
		setInfo(own, QStringLiteral("K5"));
		setInfo(other, QStringLiteral("K6"));
		DynamicElementTextItem *own_field = labelField(own);
		DynamicElementTextItem *other_field = labelField(other);
		DynamicElementTextItem *own_short = composite(own, QStringLiteral("%{structure_id}"));
		project.setDesignationDisplay(Display::Full);
		QCOMPARE(own_field->toPlainText(), QStringLiteral("==A2=FACTORY++S1+CP1-K5"));

			//Its own ==, typed with its sign: over the box's; its ++ is
			//still the folio's
		DiagramContext info = own->elementInformations();
		info.addValue(QStringLiteral("higher_plant"), QStringLiteral("==A3"));
		own->setElementInformations(info);
		QCOMPARE(own_field->toPlainText(), QStringLiteral("==A3=FACTORY++S1+CP1-K5"));
		QCOMPARE(other_field->toPlainText(), QStringLiteral("==A2=FACTORY++S1+CP1-K6"));

			//The short form shows what differs from where it is drawn
		project.setDesignationDisplay(Display::Short);
		QCOMPARE(own_field->toPlainText(), QStringLiteral("==A3=FACTORY-K5"));
		QCOMPARE(own_short->toPlainText(), QStringLiteral("==A3=FACTORY-K5"));
		QCOMPARE(other_field->toPlainText(), QStringLiteral("-K6"));

			//The duplicate check and the database agree
		QCOMPARE(DuplicateLabels::place(own), QStringLiteral("==A3=FACTORY++S1+CP1"));
		project.setDesignationDisplay(Display::Full);
		QCOMPARE(deviceRow(project, own), QStringLiteral("==A3=FACTORY++S1+CP1-K5"));
		QCOMPARE(deviceRow(project, other), QStringLiteral("==A2=FACTORY++S1+CP1-K6"));
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT higher_plant, higher_location FROM qet_devices WHERE device_uuid = '%1'")
				.arg(own->uuid().toString()), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QVERIFY(query.next());
		QCOMPARE(query.value(0).toString(), QStringLiteral("A3"));
		QCOMPARE(query.value(1).toString(), QStringLiteral("S1"));
			//An open query holds the database: a .qetdb could not be saved
		query.finish();

			//Kept through a save, in the symbol's information
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		for (const QString &name : {QStringLiteral("own.qet"), QStringLiteral("own.qetdb")}) {
			project.setFilePath(dir.filePath(name));
			const QETResult written = project.write();
			QVERIFY2(written.isOk(), qPrintable(name + QLatin1String(": ") + written.errorMessage()));
			QETProject reopened(project.filePath());
			QVERIFY(!reopened.diagrams().isEmpty());
			bool found = false;
			for (Element *e : reopened.diagrams().first()->elements()) {
				if (e->uuid() != own->uuid()) continue;
				found = true;
				QCOMPARE(e->elementInformations().value(QStringLiteral("higher_plant")).toString(),
						 QStringLiteral("==A3"));
			}
			QVERIFY2(found, qPrintable(name));
		}

			//Emptied, the box's again
		info.addValue(QStringLiteral("higher_plant"), QString());
		own->setElementInformations(info);
		QCOMPARE(own_field->toPlainText(), QStringLiteral("==A2=FACTORY++S1+CP1-K5"));
	}

		//A contact on a folio of another == shows its coil's ==
	void aContactShowsTheHigherLevelOfItsCoil()
	{
		QETProject project;
		Diagram *coil_folio = project.addNewDiagram();
		Diagram *contact_folio = project.addNewDiagram();
		setFolio(coil_folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setFolio(contact_folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setHigherLevels(coil_folio, QStringLiteral("A1"), QString());
		setHigherLevels(contact_folio, QStringLiteral("A2"), QString());
		Element *coil = symbol(coil_folio, QStringLiteral("master"));
		Element *contact = symbol(contact_folio, QStringLiteral("slave"));
		QVERIFY(coil && contact);
		setInfo(coil, QStringLiteral("K1"));
		DynamicElementTextItem *coil_field = labelField(coil);
		DynamicElementTextItem *contact_field = labelField(contact);
		coil->linkToElement(contact);

		project.setDesignationDisplay(Display::Short);
		QCOMPARE(coil_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(contact_field->toPlainText(), QStringLiteral("==A1=FACTORY-K1"));
		setHigherLevels(contact_folio, QStringLiteral("A1"), QString());
		QCOMPARE(contact_field->toPlainText(), QStringLiteral("-K1"));
	}

		//Saved only when set, so a project without them saves as before;
		//kept through .qet and .qetz
	void theHigherLevelsAreSavedOnlyWhenSet()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		const QString xml = project.toXml().toString();
		QVERIFY(!xml.contains(QLatin1String("higher_plant")));
		QVERIFY(!xml.contains(QLatin1String("higher_location")));

		setHigherLevels(diagram, QStringLiteral("A1"), QStringLiteral("S1"));
		for (const QString &name : {QStringLiteral("higher.qet"),
									QStringLiteral("higher.qetz")}) {
			project.setFilePath(dir.filePath(name));
			QVERIFY2(project.write().isOk(), qPrintable(name));
			QETProject reopened(project.filePath());
			QVERIFY2(!reopened.diagrams().isEmpty(), qPrintable(name));
			const BorderTitleBlock &tb = reopened.diagrams().first()->border_and_titleblock;
			QCOMPARE(tb.higherPlant(), QStringLiteral("A1"));
			QCOMPARE(tb.higherLocation(), QStringLiteral("S1"));
		}
	}

		//A contact shows its coil's designation; the short form leaves out
		//what the contact's own folio gives, not what the coil's does
	void aContactShowsItsCoilFromWhereItIsDrawn()
	{
		QETProject project;
		Diagram *coil_folio = project.addNewDiagram();
		Diagram *contact_folio = project.addNewDiagram();
		setFolio(coil_folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setFolio(contact_folio, QStringLiteral("FACTORY"), QStringLiteral("CP2"));
		Element *coil = symbol(coil_folio, QStringLiteral("master"));
		Element *contact = symbol(contact_folio, QStringLiteral("slave"));
		QVERIFY(coil && contact);
		setInfo(coil, QStringLiteral("K1"));
		DynamicElementTextItem *coil_field = labelField(coil);
		DynamicElementTextItem *contact_field = labelField(contact);
		coil->linkToElement(contact);

		project.setDesignationDisplay(Display::Short);
		QCOMPARE(coil_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(contact_field->toPlainText(), QStringLiteral("+CP1-K1"));

		project.setDesignationDisplay(Display::Full);
		QCOMPARE(contact_field->toPlainText(), QStringLiteral("=FACTORY+CP1-K1"));

			//The coil moving into a box reaches the contact on its folio
		project.setDesignationDisplay(Display::Short);
		coil->setPos(200, 200);
		auto *box = new StructureBoxItem(QPointF(0, 0), QPointF(400, 400));
		coil_folio->addItem(box);
		box->setLocation(QStringLiteral("F1"));
		QCOMPARE(coil_field->toPlainText(), QStringLiteral("-K1"));
		QCOMPARE(contact_field->toPlainText(), QStringLiteral("+F1-K1"));

			//So does a change of the coil's title block
		coil_folio->removeItem(box);
		delete box;
		setFolio(coil_folio, QStringLiteral("PLANT2"), QStringLiteral("CP1"));
		QCOMPARE(contact_field->toPlainText(), QStringLiteral("=PLANT2+CP1-K1"));
	}

		//Saved only when not the label only, so such a project saves as before
	void theSettingIsSavedOnlyWhenNotTheLabel()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		QETProject project;
		project.addNewDiagram();
		QVERIFY(!project.toXml().toString().contains(QLatin1String("designation_display")));

		for (Display display : {Display::Short, Display::Full}) {
			project.setDesignationDisplay(display);
			for (const QString &name : {QStringLiteral("designation.qet"),
										QStringLiteral("designation.qetz")}) {
				project.setFilePath(dir.filePath(name));
				QVERIFY2(project.write().isOk(), qPrintable(name));
				QETProject reopened(project.filePath());
				QVERIFY2(reopened.designationDisplay() == display, qPrintable(name));
			}
		}

		project.setDesignationDisplay(Display::Label);
		QVERIFY(!project.toXml().toString().contains(QLatin1String("designation_display")));
	}
};

QET_APP_TEST_MAIN(tst_designationdisplay)
#include "tst_designationdisplay.moc"
