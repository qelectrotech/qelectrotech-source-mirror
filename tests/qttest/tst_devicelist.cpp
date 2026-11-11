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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

/**
	qet_device_list: the project's devices, one row each. A device is what
	one reference designation (=plant+location-label, IEC 81346) names:
	the symbols sharing it, and a contact with its coil wherever it is
	drawn. Nothing is stored for it; the list is derived from the symbols.
*/
class tst_devicelist : public QObject
{
	Q_OBJECT

	QTemporaryDir m_definitions;
	int			  m_position = 0;

	Element *symbol(Diagram *diagram, const QString &kind, const QString &label)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/") + kind + QStringLiteral(".elmt"))) {
			QDomDocument definition;
			if (!definition.setContent(QString(
					R"(<definition type="element" link_type="%1" uuid="%2" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<kindInformations><kindInformation name="type">%5</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%3"/><terminal x="0" y="20" orientation="s" uuid="%4"/>
</description></definition>)")
					.arg(kind, QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString(),
						 kind == QLatin1String("slave") ? QStringLiteral("simple") : QStringLiteral("coil"))))
				return nullptr;
			if (!collection->addElementDefinition(QStringLiteral("import"), kind,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/") + kind + QStringLiteral(".elmt"),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 80 * (position % 8), 150 + 100 * (position / 8));
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

	static void setInfo(Element *element, const QString &name, const QString &value)
	{
		auto info = element->elementInformations();
		info.addValue(name, value);
		element->setElementInformations(info);
	}

  private slots:
	void initTestCase()
	{
		QVERIFY(m_definitions.isValid());
		QETProject::setBackupEnabled(false);
	}

	void init() { m_position = 0; }

	void deviceTypeAsModel()
	{
			//A device's type (e.g. CONTR10) is its model:
			//each symbol's in qet_devices, the head's in qet_device_list
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		setPlace(folio, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		Element *coil = symbol(folio, QStringLiteral("master"), QStringLiteral("K12"));
		Element *contact = symbol(folio, QStringLiteral("slave"), QString());
		QVERIFY(coil && contact);
		coil->linkToElement(contact);
		setInfo(coil, QStringLiteral("model"), QStringLiteral("CONTR10"));
		setInfo(coil, QStringLiteral("description"), QStringLiteral("MOTOR CONTR, 1 N/O CONTACT"));

		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT model FROM qet_devices WHERE device_uuid = '%1'")
				.arg(coil->uuid().toString()), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QVERIFY(query.next());
		QCOMPARE(query.value(0).toString(), QStringLiteral("CONTR10"));
		query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id, model, description FROM qet_device_list"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QVERIFY(query.next());
		QCOMPARE(query.value(0).toString(), QStringLiteral("=FACTORY+CP1-K12"));
		QCOMPARE(query.value(1).toString(), QStringLiteral("CONTR10"));
		QCOMPARE(query.value(2).toString(), QStringLiteral("MOTOR CONTR, 1 N/O CONTACT"));
	}

	void oneRowPerDevice()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		setPlace(first, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setPlace(second, QStringLiteral("FACTORY"), QStringLiteral("CP2"));

			//A coil and its contact on another folio, of another location:
			//one device, where its coil is
		Element *coil = symbol(first, QStringLiteral("master"), QStringLiteral("K1"));
		Element *contact = symbol(second, QStringLiteral("slave"), QString());
		QVERIFY(coil && contact);
		coil->linkToElement(contact);
		setInfo(coil, QStringLiteral("manufacturer_reference"), QStringLiteral("LC1D09"));
			//A socket drawn twice: one designation, one device
		QVERIFY(symbol(first, QStringLiteral("simple"), QStringLiteral("XS13")));
		QVERIFY(symbol(first, QStringLiteral("simple"), QStringLiteral("XS13")));
			//One label in two locations: two devices
		QVERIFY(symbol(first, QStringLiteral("simple"), QStringLiteral("X1")));
		QVERIFY(symbol(second, QStringLiteral("simple"), QStringLiteral("X1")));
			//Left out: a symbol without a label, folio reports
		QVERIFY(symbol(first, QStringLiteral("simple"), QString()));
		Element *going = symbol(first, QStringLiteral("next_report"), QStringLiteral("R1"));
		Element *coming = symbol(second, QStringLiteral("previous_report"), QStringLiteral("R1"));
		QVERIFY(going && coming);
		going->linkToElement(coming);

		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id, label, symbols, folios, folio_number, articles, head_uuid "
			"FROM qet_device_list"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QStringList rows;
		QString k1_head;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < 6 ; ++i) row << query.value(i).toString();
			rows << row.join(QLatin1Char('|'));
			if (query.value(1).toString() == QLatin1String("K1"))
				k1_head = query.value(6).toString();
		}
		QCOMPARE(rows, QStringList({
			QStringLiteral("=FACTORY+CP1-K1|K1|2|2|1|LC1D09"),
			QStringLiteral("=FACTORY+CP1-X1|X1|1|1|1|"),
			QStringLiteral("=FACTORY+CP1-XS13|XS13|2|1|1|"),
			QStringLiteral("=FACTORY+CP2-X1|X1|1|1|2|")}));
		QCOMPARE(k1_head, coil->uuid().toString());
	}
};

QET_APP_TEST_MAIN(tst_devicelist)
#include "tst_devicelist.moc"
