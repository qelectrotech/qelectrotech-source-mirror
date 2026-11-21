// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QDomDocument>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QUuid>

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "dataBase/projectdatabase.h"
#include "devicetype/devicetype.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "materiallist/materiallist.h"
#include "materiallist/parttypes.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/parttypesdialog.h"

// A part of the material list says its device type once: there, and on the
// project's symbols with that part.
namespace {

MaterialListData list(const QStringList &parts, const QStringList &types = {})
{
	MaterialListData data;
	data.columns = QStringList{QStringLiteral("manufacturer_reference"), QStringLiteral("designation"), QStringLiteral("model")};
	for (int i = 0 ; i < parts.size() ; ++i) {
		MaterialRecord record;
		record.setValue(QStringLiteral("manufacturer_reference"), parts.at(i));
		record.setValue(QStringLiteral("designation"), parts.at(i) + QStringLiteral(" text"));
		record.setValue(QStringLiteral("device_type"), types.value(i));
		data.records << record;
	}
	return data;
}

DeviceType aType(const QString &name)
{
	DeviceType type;
	type.name = name;
	type.names.insert(QStringLiteral("en"), name);
	return type;
}

} // namespace

class tst_parttypes : public QObject
{
	Q_OBJECT

	Element *symbol(Diagram *folio, const QString &label, const QString &part, const QString &model = QString())
	{
		auto *collection = folio->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/s.elmt"))) {
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				"<definition type=\"element\" link_type=\"simple\" width=\"20\" height=\"40\" hotspot_x=\"10\" hotspot_y=\"20\">"
				"<uuid uuid=\"%1\"/><names><name lang=\"en\">s</name></names>"
				"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/></description></definition>")
								  .arg(QUuid::createUuid().toString()));
			collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("s"), definition.documentElement());
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
					ElementsLocation(QStringLiteral("embed://import/s.elmt"), folio->project()), nullptr, &state);
		if (state || !element) return nullptr;
		folio->addItem(element);
		DiagramContext info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.addValue(QStringLiteral("manufacturer_reference"), part);
		if (!model.isEmpty()) info.addValue(QStringLiteral("device_type"), model);
		element->setElementInformations(info);
		return element;
	}

private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	void theFirstPartIsTheMainOne()
	{
		QCOMPARE(PartTypes::mainPart(QStringLiteral("A+B")), QStringLiteral("A"));
		QCOMPARE(PartTypes::mainPart(QStringLiteral(" A , B")), QStringLiteral("A"));
		QCOMPARE(PartTypes::mainPart(QStringLiteral("A")), QStringLiteral("A"));
	}

	void listsOnlyPartsWithoutAType()
	{
		const auto rows = PartTypes::partsWithoutType(list({QStringLiteral("B"), QStringLiteral("A"), QStringLiteral("C")},
														   {QString(), QString(), QStringLiteral("T1")}), {});
		QCOMPARE(rows.size(), 2);
		QCOMPARE(rows.first().part, QStringLiteral("A"));
		QCOMPARE(rows.first().description, QStringLiteral("A text"));
	}

	void theMostUsedPartsComeFirstWithTheirType()
	{
		QHash<QString, PartTypes::Usage> usage;
		usage[QStringLiteral("B")].devices = 5;
		usage[QStringLiteral("B")].types[QStringLiteral("T1")] = 4;
		usage[QStringLiteral("C")].devices = 3;
		usage[QStringLiteral("C")].types[QStringLiteral("T1")] = 2;
		usage[QStringLiteral("C")].types[QStringLiteral("T2")] = 1;
		const auto rows = PartTypes::partsWithoutType(list({QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")}), usage);
		QCOMPARE(rows.at(0).part, QStringLiteral("B"));
		QCOMPARE(rows.at(0).suggestion, QStringLiteral("T1"));
		QVERIFY(rows.at(0).certain);
		QCOMPARE(rows.at(1).part, QStringLiteral("C"));
		QCOMPARE(rows.at(1).suggestion, QStringLiteral("T1"));
		QVERIFY(!rows.at(1).certain);
		QVERIFY(rows.at(2).suggestion.isEmpty());
	}

	void aProjectShowsWhatItsDevicesUse()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		QVERIFY(symbol(folio, QStringLiteral("K1"), QStringLiteral("A+B"), QStringLiteral("T1")));
		QVERIFY(symbol(folio, QStringLiteral("K2"), QStringLiteral("A"), QStringLiteral("T1")));
		QVERIFY(symbol(folio, QStringLiteral("K3"), QStringLiteral("C")));
		const auto usage = PartTypes::usageIn(&project);
		QCOMPARE(usage.value(QStringLiteral("A")).devices, 2);
		QCOMPARE(usage.value(QStringLiteral("A")).types.value(QStringLiteral("T1")), 2);
		QVERIFY(!usage.contains(QStringLiteral("B")));      //an accessory
		QCOMPARE(usage.value(QStringLiteral("C")).devices, 1);
		QVERIFY(usage.value(QStringLiteral("C")).types.isEmpty());
	}

	void symbolsTakeTheTypeAndUndoGivesItBack()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		project.setDeviceType(aType(QStringLiteral("T1")));
		Element *plain = symbol(folio, QStringLiteral("K1"), QStringLiteral("A+B"));
		Element *typed = symbol(folio, QStringLiteral("K2"), QStringLiteral("A"), QStringLiteral("OTHER"));
		Element *other = symbol(folio, QStringLiteral("K3"), QStringLiteral("Z"));
		int count = 0;
		QUndoCommand *undo = PartTypes::giveToSymbols(&project, {{QStringLiteral("A"), QStringLiteral("T1")}}, &count);
		QVERIFY(undo);
		QCOMPARE(count, 1);
		project.undoStack()->push(undo);
		const auto model = [](Element *e) {return e->elementInformations().value(QStringLiteral("device_type")).toString();};
		QCOMPARE(model(plain), QStringLiteral("T1"));
		QCOMPARE(model(typed), QStringLiteral("OTHER"));    //what it says stays
		QVERIFY(model(other).isEmpty());
		project.undoStack()->undo();
		QVERIFY(model(plain).isEmpty());
	}

	void theDialogSavesAndGivesTheType()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath(QStringLiteral("material.csv"));
		MaterialListData own = list({QStringLiteral("A"), QStringLiteral("B")});
		QVERIFY(MaterialList::writeFile(path, own));

		QETProject project;
		Diagram *folio = project.addNewDiagram();
		project.setDeviceType(aType(QStringLiteral("T1")));
		QVERIFY(symbol(folio, QStringLiteral("K1"), QStringLiteral("A"), QStringLiteral("T1")));
		Element *k2 = symbol(folio, QStringLiteral("K2"), QStringLiteral("A"));
		QVERIFY(k2);

		PartTypesDialog dialog(&project, path);
		QCOMPARE(dialog.useSuggestions(), 1);
		dialog.accept();
		QCOMPARE(dialog.result(), int(QDialog::Accepted));

		MaterialListData back;
		QVERIFY(MaterialList::load(path, &back));
		QCOMPARE(back.records.at(0).value(QStringLiteral("device_type")), QStringLiteral("T1"));
		QVERIFY(back.records.at(1).value(QStringLiteral("device_type")).isEmpty());
		QCOMPARE(k2->elementInformations().value(QStringLiteral("device_type")).toString(), QStringLiteral("T1"));
	}

	void anUnknownTypeIsNotSaved()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath(QStringLiteral("material.csv"));
		QVERIFY(MaterialList::writeFile(path, list({QStringLiteral("A")})));
		const QByteArray before = [&path] {QFile f(path); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();}();

		PartTypesDialog dialog(nullptr, path);
		auto *table = dialog.findChild<QTableWidget *>();
		QVERIFY(table);
		table->item(0, PartTypesDialog::Type)->setText(QStringLiteral("NOPE"));
		QTimer::singleShot(0, this, [] {
			if (QWidget *box = QApplication::activeModalWidget()) box->close();
		});
		dialog.accept();
		const QByteArray after = [&path] {QFile f(path); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();}();
		QCOMPARE(after, before);
	}
};

QTEST_MAIN(tst_parttypes)
#include "tst_parttypes.moc"
