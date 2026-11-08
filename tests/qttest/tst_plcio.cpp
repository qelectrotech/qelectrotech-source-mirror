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
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "titleblockproperties.h"
#include "undocommand/linkelementcommand.h"

#include <QDomDocument>
#include <QSqlQuery>
#include <QUuid>
#include <QtTest>

/**
	qet_plc_io: the project's PLC I/O list, one row per entry of each PLC
	master's I/O table -- card, address, type, function, comment -- and,
	where a PLC slave is linked to the entry, where it is drawn. Entries no
	slave is linked to are listed too, as RSWire's plc.lst lists them.
*/
class tst_plcio : public QObject
{
	Q_OBJECT

	Element *symbol(Diagram *diagram, bool master, QPointF at)
	{
		const QString kind = master ? QStringLiteral("plc_card") : QStringLiteral("plc_point");
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/") + kind + QStringLiteral(".elmt"))) {
			QDomDocument definition;
			definition.setContent(QString(
				R"(<definition type="element" link_type="%1" uuid="%2" width="40" height="60" hotspot_x="20" hotspot_y="30">
<names><name lang="en">%3</name></names>
<kindInformations><kindInformation name="type">plc</kindInformation></kindInformations>
<description><line x1="-20" y1="-30" x2="20" y2="-30"/>
<terminal x="0" y="30" orientation="s" uuid="%4"/></description></definition>)")
				.arg(master ? QStringLiteral("master") : QStringLiteral("slave"),
					 QUuid::createUuid().toString(), kind, QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), kind,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/") + kind + QStringLiteral(".elmt"),
							 diagram->project()), nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(at);
		return element;
	}

	static ElementData::PlcIO entry(ElementData::PlcIOType type, const QString &address,
										 const QString &function, const QString &comment = {})
	{
		ElementData::PlcIO io;
		io.type = type;
		io.address = address;
		io.functionText = function;
		io.comment = comment;
		io.terminalCount = 1;
		return io;
	}

private slots:
	void everyEntryOneRow()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		TitleBlockProperties tb = folio->border_and_titleblock.exportTitleBlock();
		tb.plant = QStringLiteral("FACTORY");
		tb.locmach = QStringLiteral("CP1");
		folio->border_and_titleblock.importTitleBlock(tb);

			//A201, a card of three points; the second one is drawn
		Element *card = symbol(folio, true, {100, 100});
		Element *point = symbol(folio, false, {400, 300});
		QVERIFY(card && point);
		QCOMPARE(card->elementData().m_master_type, ElementData::PLC);
		ElementData data = card->elementData();
		auto plc = data.plcMasterData();
		plc.ios = {entry(ElementData::EntreeDigitale, QStringLiteral("I:3/0"), QStringLiteral("COOLER 1 DRIVE")),
				   entry(ElementData::SortieDigitale, QStringLiteral("O:4/1"), QStringLiteral("COOLER 1 FAN"), QStringLiteral("24 V")),
				   entry(ElementData::EntreeAnalogique, QStringLiteral("1"), QStringLiteral("SILO LEVEL"))};
		data.setPlcMasterData(plc);
		card->setElementData(data);
		DiagramContext info = card->elementInformations();
		info.addValue(QStringLiteral("label"), QStringLiteral("A201"));
		card->setElementInformations(info);

		auto *link = new LinkElementCommand(point);
		link->setLink(card);
		link->setGroupIndex(1);
		project.undoStack()->push(link);
		QCOMPARE(card->groupIndexForElement(point), 1);

		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT structure_id, plc, entry, address, type, function, comment, "
			"folio_number, slave_uuid FROM qet_plc_io ORDER BY entry"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QStringList rows;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < 8 ; ++i) row << query.value(i).toString();
			row << (query.value(8).toString() == point->uuid().toString()
					? QStringLiteral("slave") : query.value(8).toString());
			rows << row.join(QLatin1Char('|'));
		}
		QCOMPARE(rows, QStringList({
			QStringLiteral("=FACTORY+CP1-A201|A201|1|I:3/0|DI|COOLER 1 DRIVE|||"),
			QStringLiteral("=FACTORY+CP1-A201|A201|2|O:4/1|DO|COOLER 1 FAN|24 V|1|slave"),
			QStringLiteral("=FACTORY+CP1-A201|A201|3|1|AI|SILO LEVEL|||")}));
	}
};

QET_APP_TEST_MAIN(tst_plcio)
#include "tst_plcio.moc"
