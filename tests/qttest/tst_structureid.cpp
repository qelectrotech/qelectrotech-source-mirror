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
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "titleblockproperties.h"

#include <QDomDocument>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

/**
	%{structure_id} composes =plant+location-label (IEC 81346). An element
	that has no plant or location of its own takes the one of its folio's
	title block, and every part that is empty is left out. %{plant} and
	%{location} are not affected: they show the element's own value only.
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

		QCOMPARE(text(QStringLiteral("%{structure_id}"), info("X1"), element),
				 QStringLiteral("=FACTORY+CP1-X1"));
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
		QCOMPARE(text(QStringLiteral("%{structure_id}"), info("X1"), element),
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
		QCOMPARE(text(QStringLiteral("%{structure_id}"),
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

		QCOMPARE(text(QStringLiteral("%{structure_id}"), info("K1"), element),
				 QStringLiteral("+CP1-K1"));
		QCOMPARE(text(QStringLiteral("%{structure_id}"), info({}), element),
				 QStringLiteral("+CP1"));
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

	void withoutAnElementOnlyItsOwnValuesCount()
	{
			//Callers with no element (e.g. a symbol in the element editor)
			//keep the old behaviour: no folio to fall back to.
		QCOMPARE(text(QStringLiteral("%{plant}|%{location}|%{structure_id}"),
					  info("Q1"), nullptr),
				 QStringLiteral("||-Q1"));
	}
};

QET_APP_TEST_MAIN(tst_structureid)
#include "tst_structureid.moc"
