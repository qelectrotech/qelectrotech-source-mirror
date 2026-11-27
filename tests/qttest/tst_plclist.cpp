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
#include "dataBase/ui/plcquerywidget.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "factory/qetgraphicstablefactory.h"
#include "factory/ui/addtabledialog.h"
#include "qetapptest.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "titleblockproperties.h"
#include "undocommand/linkelementcommand.h"
#include "utils/qetutils.h"

#include <QComboBox>
#include <QDomDocument>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardItemModel>
#include <QUuid>
#include <QtTest>

/**
	qet_plc_list: the SPS list of a folio, one row per channel of a PLC
	master -- the master's BMK, place and part data on each of its
	rows, the channel's own type, address and function, and where the
	symbol drawing it stands -- and the way its pages are broken: a new
	master starts on a new page, a master longer than a sheet carries
	on to the next one.
*/
class tst_plclist : public QObject
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

	/// A card of @p count digital inputs, its BMK and (when they are
	/// given) the part data the list reads from the master
	static bool setCard(Element *card, const QString &label, int count,
						const QString &manufacturer = QString(),
						const QString &designation = QString())
	{
		ElementData data = card->elementData();
		auto plc = data.plcMasterData();
		plc.ios.clear();
		for (int i = 0 ; i < count ; ++i) {
			plc.ios << entry(ElementData::EntreeDigitale,
							 QStringLiteral("I:%1/%2").arg(count).arg(i),
							 QStringLiteral("FUNCTION %1").arg(i + 1));
		}
		data.setPlcMasterData(plc);
		card->setElementData(data);

		DiagramContext info = card->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		if (!manufacturer.isEmpty()) info.addValue(QStringLiteral("manufacturer"), manufacturer);
		if (!designation.isEmpty()) info.addValue(QStringLiteral("designation"), designation);
		card->setElementInformations(info);
		return true;
	}

	/// The number of a border cell label: "B12" -> "12"
	static QString fieldNumber(const QString &cell)
	{
		QString number;
		for (const QChar c : cell)
			if (c.isDigit()) number += c;
		return number;
	}

	static QStringList rows(QETProject &project, const QString &sql, QString *error = nullptr)
	{
		QString local_error;
		QSqlQuery query = project.dataBase()->newQuery(sql, &local_error);
		if (error) *error = local_error;
		QStringList list;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < query.record().count() ; ++i)
				row << query.value(i).toString();
			list << row.join(QLatin1Char('|'));
		}
		return list;
	}

private slots:
	/**
		Every channel of every master is a row, with the master's BMK
		and part data on it, the channel's own type, address and
		function, and the place of the symbol drawing it -- empty for a
		channel no symbol is drawn for.
	*/
	void everyChannelOneRow()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		TitleBlockProperties tb = folio->border_and_titleblock.exportTitleBlock();
		tb.plant = QStringLiteral("FACTORY");
		tb.locmach = QStringLiteral("CP1");
		folio->border_and_titleblock.importTitleBlock(tb);

			//A201, a card of three points, the second one drawn;
			//A202, a card of two points, none drawn
		Element *card = symbol(folio, true, {100, 100});
		Element *other = symbol(folio, true, {100, 400});
		Element *point = symbol(folio, false, {400, 300});
		QVERIFY(card && other && point);
		ElementData data = card->elementData();
		auto plc = data.plcMasterData();
		plc.ios = {entry(ElementData::EntreeDigitale, QStringLiteral("I:3/0"), QStringLiteral("COOLER 1 DRIVE")),
				   entry(ElementData::SortieDigitale, QStringLiteral("O:4/1"), QStringLiteral("COOLER 1 FAN"), QStringLiteral("24 V")),
				   entry(ElementData::EntreeAnalogique, QStringLiteral("1"), QStringLiteral("SILO LEVEL"))};
		data.setPlcMasterData(plc);
		card->setElementData(data);
		DiagramContext info = card->elementInformations();
		info.addValue(QStringLiteral("label"), QStringLiteral("A201"));
		info.addValue(QStringLiteral("manufacturer"), QStringLiteral("Deos"));
		info.addValue(QStringLiteral("designation"), QStringLiteral("DS-390036"));
		card->setElementInformations(info);
		QVERIFY(setCard(other, QStringLiteral("A202"), 2));

		auto *link = new LinkElementCommand(point);
		link->setLink(card);
		link->setGroupIndex(1);
		project.undoStack()->push(link);
		QCOMPARE(card->groupIndexForElement(point), 1);

		project.dataBase()->updateDB();
		QString error;
		const QStringList list = rows(project, QStringLiteral(
			"SELECT label, plant, location, manufacturer, designation, entry, "
			"plc_type, plc_address, plc_function FROM qet_plc_list "
			"ORDER BY structure_id, plc_uuid, entry"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QCOMPARE(list, QStringList({
			QStringLiteral("A201|FACTORY|CP1|Deos|DS-390036|1|DI|I:3/0|COOLER 1 DRIVE"),
			QStringLiteral("A201|FACTORY|CP1|Deos|DS-390036|2|DO|O:4/1|COOLER 1 FAN"),
			QStringLiteral("A201|FACTORY|CP1|Deos|DS-390036|3|AI|1|SILO LEVEL"),
			QStringLiteral("A202|FACTORY|CP1|||1|DI|I:2/0|FUNCTION 1"),
			QStringLiteral("A202|FACTORY|CP1|||2|DI|I:2/1|FUNCTION 2")}));

			//Only the drawn channel has a place; the ones no symbol is
			//drawn for keep the empty columns
		const QStringList places = rows(project, QStringLiteral(
			"SELECT entry, position, diagram_position, xref, plc_crossref "
			"FROM qet_plc_list ORDER BY structure_id, plc_uuid, entry"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QCOMPARE(int(places.size()), 5);
		QCOMPARE(places.at(0), QStringLiteral("1||||"));
		QCOMPARE(places.at(2), QStringLiteral("3||||"));
		QCOMPARE(places.at(3), QStringLiteral("1||||"));
		QCOMPARE(places.at(4), QStringLiteral("2||||"));
		const QStringList drawn = places.at(1).split(QLatin1Char('|'));
		QCOMPARE(int(drawn.size()), 5);
		QCOMPARE(drawn.at(0), QStringLiteral("2"));
		QVERIFY2(!drawn.at(1).isEmpty(), "the drawn channel has no position");
		QCOMPARE(drawn.at(2), QStringLiteral("1"));

			//The type written out is what the symbol drawing the channel
			//keeps of it -- the same words the drawing shows. An entry
			//no symbol is drawn for keeps none, so its column is empty
		const QStringList kinds = rows(project, QStringLiteral(
			"SELECT plc_type, plc_type_full FROM qet_plc_list "
			"ORDER BY structure_id, plc_uuid, entry"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QCOMPARE(int(kinds.size()), 5);
		QCOMPARE(kinds.at(1), QStringLiteral("DO|")
				 + ElementData::translatedPlcIOType(ElementData::SortieDigitale));
		QCOMPARE(kinds.at(0), QStringLiteral("DI|"));
		QCOMPARE(kinds.at(3), QStringLiteral("DI|"));

			//The SPS cross-reference of a drawn channel is its own
			//place: the sheet it is on and the cell it stands in, made
			//the way the drawing makes its own cross-references. The
			//stored one the drawing keeps would read the same on every
			//row of a card, because it is made on the master.
		const QStringList refs = rows(project, QStringLiteral(
			"SELECT plc_crossref, folio, diagram_position, position "
			"FROM qet_plc_list ORDER BY structure_id, plc_uuid, entry"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		const QStringList mine = refs.at(1).split(QLatin1Char('|'));
		QCOMPARE(int(mine.size()), 4);
		QVERIFY2(!mine.at(0).isEmpty(), "the drawn channel has no SPS cross-reference");
		const QString sheet = mine.at(1).isEmpty() ? mine.at(2) : mine.at(1);
		QCOMPARE(mine.at(0), sheet + QLatin1Char('.') + fieldNumber(mine.at(3)));
			//and the sheet no symbol is drawn for has none
		QCOMPARE(refs.at(0).section(QLatin1Char('|'), 0, 0), QStringLiteral(""));
	}

	/**
		The pages of the list: every page after the first starts at the
		first channel of a master, and every row is on a page.
	*/
	void pagesStartAtAMaster()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		TitleBlockProperties tb = folio->border_and_titleblock.exportTitleBlock();
		tb.plant = QStringLiteral("FACTORY");
		tb.locmach = QStringLiteral("CP1");
		folio->border_and_titleblock.importTitleBlock(tb);

		AddTableDialog dialog(new PlcQueryWidget());
		dialog.setTableName(QStringLiteral("SPS"));
		dialog.setAdjustTableToFolio(true);
		dialog.setAddNewTableToNewDiagram(true);

			//How many rows one sheet takes, measured the way create()
			//measures it: same margins, same font, same sheet
		QStandardItemModel probe_model(1, 1);
		probe_model.setData(probe_model.index(0, 0), dialog.tableFont(), Qt::FontRole);
		probe_model.setData(probe_model.index(0, 0),
							QETUtils::marginsToString(dialog.headerMargins()),
							Qt::UserRole + 1);
		QetGraphicsTableItem probe_table;
		probe_table.setModel(&probe_model);
		folio->addItem(&probe_table);
		QetGraphicsTableItem::adjustTableToFolio(&probe_table);
		const int fit = probe_table.displayNRow();
		folio->removeItem(&probe_table);
		QVERIFY2(fit > 0, "no row fits on a sheet, nothing to split");

			//More channels than two sheets take, in cards shorter than
			//one sheet, so that the break has to fall at a card
		const int entries = qBound(2, fit / 2, 40);
		const int masters = (2 * fit) / entries + 3;
		for (int i = 1 ; i <= masters ; ++i) {
			Element *card = symbol(folio, true, {100.0 + 40 * i, 100});
			QVERIFY(card);
			QVERIFY(setCard(card, QStringLiteral("A%1").arg(i, 2, 10, QChar('0')), entries));
		}

		auto *widget = dynamic_cast<PlcQueryWidget *>(dialog.contentWidget());
		QVERIFY(widget);
		widget->setQuery(QStringLiteral(
			"SELECT label, plc_address FROM qet_plc_list "
			"ORDER BY structure_id, plc_uuid, entry"));
		QetGraphicsTableFactory::create(folio, &dialog);

		QString error;
		const QStringList uuids = rows(project, QStringLiteral(
			"SELECT plc_uuid FROM qet_plc_list ORDER BY structure_id, plc_uuid, entry"), &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		QCOMPARE(int(uuids.size()), masters * entries);

		QetGraphicsTableItem *first = nullptr;
		for (QGraphicsItem *item : folio->items()) {
			if (auto *table = qgraphicsitem_cast<QetGraphicsTableItem *>(item)) {
				if (!table->previousTable()) {
					first = table;
					break;
				}
			}
		}
		QVERIFY2(first, "the list put no table on the sheet");

		int pages = 0;
		int start = 0;
		for (QetGraphicsTableItem *table = first ; table ; table = table->nextTable().data()) {
			const int rows_of_page = table->displayNRow();
			QVERIFY2(rows_of_page > 0, "a page of the list shows no row");
			if (pages > 0) {
				QVERIFY2(start < int(uuids.size()), "a page starts past the last row");
				QVERIFY2(uuids.at(start) != uuids.at(start - 1),
						 qPrintable(QStringLiteral("page %1 starts inside the master of row %2")
									.arg(pages).arg(start)));
			}
			start += rows_of_page;
			++pages;
		}
		QCOMPARE(start, int(uuids.size()));
		QVERIFY2(pages >= 2, qPrintable(QStringLiteral(
			"every row fitted on one sheet (%1 rows), the page break was not tested")
			.arg(uuids.size())));
	}

	/**
		How many rows a page takes: the whole of the master the page
		starts with, so that the next page starts with the next master,
		and as much of one master as fits when it alone is longer than
		a page.
	*/
	void rowsOnPageStopsAtAMaster()
	{
		const QVector<QString> mixed{QStringLiteral("A"), QStringLiteral("A"),
									 QStringLiteral("B"), QStringLiteral("B"),
									 QStringLiteral("B"), QStringLiteral("C")};
			//A takes the page, however much room is left over: the next
			//one starts with B
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(mixed, 0, 4), 2);
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(mixed, 2, 4), 3);
			//One row fits of a two-row master: it carries on next page
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(mixed, 0, 1), 1);
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(mixed, 1, 1), 1);
			//B does not fit whole: it takes two pages, and C starts
			//on a third one
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(mixed, 2, 2), 2);
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(mixed, 4, 2), 1);
			//A page holds one master, never two
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(mixed, 0, 40), 2);

		const QVector<QString> one{QStringLiteral("A"), QStringLiteral("A"),
								   QStringLiteral("A")};
			//One master longer than a page: the page takes what fits
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(one, 0, 2), 2);
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(one, 2, 2), 1);
			//Nothing fits, or the page would start past the last row
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(one, 0, 0), 0);
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(one, 3, 10), 0);
		QCOMPARE(QetGraphicsTableFactory::rowsOnPage(QVector<QString>(), 0, 10), 0);
	}

	/**
		Two saved column sets: each is written under its own name and
		loads back. A save that wrote only its own entry replaced every
		saved set with {"fields": ...}, lost the name it was saved under,
		and left the list offering "fields" and loading nothing.
	*/
	void savedSetsLoadBack()
	{
		const QString path = QETApp::configDir() + QStringLiteral("/plc_list.json");
		QFile::remove(path);

		PlcQueryWidget widget;
		auto *name = widget.findChild<QLineEdit *>(QStringLiteral("m_line_edit"));
		QVERIFY(name);

		widget.setQuery(QStringLiteral("SELECT label, plant FROM qet_plc_list "
									   "ORDER BY structure_id, plc_uuid, entry"));
		name->setText(QStringLiteral("first"));
		QVERIFY(QMetaObject::invokeMethod(&widget, "saveConfig"));

		widget.setQuery(QStringLiteral("SELECT label, plc_address FROM qet_plc_list "
									   "ORDER BY structure_id, plc_uuid, entry"));
		name->setText(QStringLiteral("second"));
		QVERIFY(QMetaObject::invokeMethod(&widget, "saveConfig"));

			//Both sets are in the file, each under its own name
		QFile file(path);
		QVERIFY2(file.open(QFile::ReadOnly), qPrintable(file.errorString()));
		const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
		QCOMPARE(int(root.keys().size()), 2);
		QVERIFY(root.contains(QStringLiteral("first")));
		QVERIFY(root.contains(QStringLiteral("second")));

			//Each loads back into a widget built after the saves, so
			//that its list of names comes out of the file as it does
			//for a person opening the dialog again
		const QList<QStringList> expected{
			QStringList({QStringLiteral("label"), QStringLiteral("plant")}),
			QStringList({QStringLiteral("label"), QStringLiteral("plc_address")})};
		const QStringList names{QStringLiteral("first"), QStringLiteral("second")};
		for (int i = 0 ; i < names.size() ; ++i) {
			PlcQueryWidget loaded;
			auto *combo = loaded.findChild<QComboBox *>();
			QVERIFY(combo);
			combo->setCurrentText(names.at(i));
			QCOMPARE(combo->currentText(), names.at(i));
			QVERIFY(QMetaObject::invokeMethod(&loaded, "loadConfig"));
			QCOMPARE(loaded.selectedKeys(), expected.at(i));
		}
	}
};

QET_APP_TEST_MAIN(tst_plclist)
#include "tst_plclist.moc"
