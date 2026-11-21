// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetype.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "materiallist/findpartdialog.h"
#include "materiallist/partcategory.h"
#include "materiallist/partsearch.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/placelabelpartdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QDomDocument>
#include <QtTest>

/**
	The Find part window (FindPartDialog): the filters and what they
	offer, the chips, the buttons which clear them, the empty state, and
	the categories of the parts (PartCategory).
*/
class tst_findpart : public QObject
{
	Q_OBJECT

	using Filter = FindPartDialog::Filter;

	static MaterialRecord part(const QString &number, const QString &description, const QString &manufacturer,
							   const QString &type = QString(), const QString &voltage = QString(),
							   const QString &supplier = QString())
	{
		MaterialRecord record;
		record.setValue(QStringLiteral("manufacturer_reference"), number);
		record.setValue(QStringLiteral("description"), description);
		record.setValue(QStringLiteral("manufacturer"), manufacturer);
		record.setValue(QStringLiteral("device_type"), type);
		record.setValue(QStringLiteral("voltage_rating"), voltage);
		record.setValue(QStringLiteral("supplier"), supplier);
		return record;
	}

	static MaterialListData sample()
	{
		MaterialListData data;
		data.records << part(QStringLiteral("LC1D09BD"), QStringLiteral("Contactor 3P 9A"), QStringLiteral("Schneider"), QStringLiteral("CONTR10"), QStringLiteral("24V"), QStringLiteral("RS"))
					 << part(QStringLiteral("LC1D12BD"), QStringLiteral("Contactor 3P 12A"), QStringLiteral("Schneider"), QStringLiteral("CONTR10"), QStringLiteral("24V"), QStringLiteral("RS"))
					 << part(QStringLiteral("LC1D09P7"), QStringLiteral("Contactor 3P 9A"), QStringLiteral("Schneider"), QStringLiteral("CONTR10"), QStringLiteral("230V"), QStringLiteral("Farnell"))
					 << part(QStringLiteral("RXM2AB2BD"), QStringLiteral("Relay 2 changeover"), QStringLiteral("Schneider"), QStringLiteral("RELAY2"), QStringLiteral("24V"), QStringLiteral("RS"))
					 << part(QStringLiteral("G2R-1"), QStringLiteral("Power relay"), QStringLiteral("Omron"), QStringLiteral("RELAY1"), QStringLiteral("12V"), QStringLiteral("Farnell"))
					 << part(QStringLiteral("ZB-FUSE"), QStringLiteral("Fuse holder"), QStringLiteral("Phoenix"), QString(), QString(), QString());
		return data;
	}

	static QComboBox *combo(FindPartDialog &dialog, const char *name)
	{
		return dialog.findChild<QComboBox *>(QString::fromLatin1(name));
	}

	static void choose(QComboBox *box, const QString &value)
	{
		const int index = box->findData(value);
		QVERIFY2(index >= 0, qPrintable(value));
		box->setCurrentIndex(index);
		emit box->activated(index);
	}

	static int chips(FindPartDialog &dialog)
	{
		return dialog.findChild<QWidget *>(QStringLiteral("findPartChips"))
				->findChildren<QToolButton *>(QStringLiteral("findPartChip")).size();
	}

	static void click(FindPartDialog &dialog, const char *name)
	{
		auto *button = dialog.findChild<QPushButton *>(QString::fromLatin1(name));
		QVERIFY(button);
		button->click();
	}

	static QString define(QETProject &project, const QString &name)
	{
		QDomDocument document;
		document.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"simple\" width=\"80\" height=\"60\" hotspot_x=\"10\" hotspot_y=\"20\">"
			"<uuid uuid=\"%1\"/><names><name lang=\"en\">%2</name></names>"
			"<description><line x1=\"0\" y1=\"-10\" x2=\"0\" y2=\"10\"/>"
			"<terminal x=\"0\" y=\"20\" orientation=\"s\" uuid=\"%3\" name=\"A1\"/></description></definition>")
				.arg(QUuid::createUuid().toString(), name, QUuid::createUuid().toString()));
		project.embeddedElementCollection()->addElementDefinition(QStringLiteral("import"), name, document.documentElement());
		return QStringLiteral("embed://import/") + name + QStringLiteral(".elmt");
	}

	static Element *place(QETProject &project, Diagram *folio, const QString &location)
	{
		int state = 0;
		Element *symbol = ElementFactory::Instance()->createElement(ElementsLocation(location, &project), nullptr, &state);
		if (!symbol || state) return nullptr;
		folio->addItem(symbol);
		return symbol;
	}

	static MaterialListData typed()
	{
		MaterialListData data;
		data.records << part(QStringLiteral("C-1"), QStringLiteral("Coil one"), QStringLiteral("Acme"), QStringLiteral("SUIT-COIL"))
					 << part(QStringLiteral("C-2"), QStringLiteral("Coil two"), QStringLiteral("Acme"), QStringLiteral("SUIT-COIL"))
					 << part(QStringLiteral("L-1"), QStringLiteral("Lamp"), QStringLiteral("Acme"), QStringLiteral("SUIT-LAMP"))
					 << part(QStringLiteral("N-1"), QStringLiteral("Nothing"), QStringLiteral("Acme"));
		return data;
	}

  private slots:
	void suitsFollowsTheDeviceTypesOfTheSymbol()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString coil = define(project, QStringLiteral("suit-coil"));
		const QString lamp = define(project, QStringLiteral("suit-lamp"));
		const QString other = define(project, QStringLiteral("suit-none"));
		DeviceType coil_type;
		coil_type.name = QStringLiteral("SUIT-COIL");
		coil_type.parts << DeviceType::Part{QStringLiteral("master"), QStringLiteral("coil"), QString(),
											{{coil, {QStringLiteral("A1")}, {}}}};
		DeviceType lamp_type;
		lamp_type.name = QStringLiteral("SUIT-LAMP");
		lamp_type.parts << DeviceType::Part{QStringLiteral("simple"), QStringLiteral("simple"), QString(),
											{{lamp, {QStringLiteral("A1")}, {}}}};
		project.setDeviceType(coil_type);
		project.setDeviceType(lamp_type);
		Element *coil_symbol = place(project, folio, coil);
		Element *none_symbol = place(project, folio, other);
		QVERIFY(coil_symbol && none_symbol);

		const MaterialListData data = typed();
		QCOMPARE(FindPartDialog::suitingRows(data, coil_symbol), (QSet<int>{0, 1}));
		QVERIFY(FindPartDialog::suitingRows(data, none_symbol).isEmpty());
		QVERIFY(FindPartDialog::suitingRows(data, nullptr).isEmpty());

		{
			FindPartDialog dialog(data, coil_symbol, QStringLiteral("K1"), QStringList());
			auto *suits = dialog.findChild<QCheckBox *>(QStringLiteral("findPartSuits"));
			QVERIFY(suits && suits->isEnabled() && suits->isChecked());
			QCOMPARE(dialog.resultCount(), 2);
			QCOMPARE(chips(dialog), 1);
			suits->setChecked(false);
			QCOMPARE(dialog.resultCount(), 4);
			QCOMPARE(chips(dialog), 0);
		}
			//A symbol no device type lists keeps every part
		FindPartDialog dialog(data, none_symbol, QString(), QStringList());
		auto *suits = dialog.findChild<QCheckBox *>(QStringLiteral("findPartSuits"));
		QVERIFY(!suits->isChecked() && !suits->isEnabled());
		QCOMPARE(dialog.resultCount(), 4);
		QCOMPARE(chips(dialog), 0);
	}

	void usedPartsAreStarredAndFirst()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		const QString lamp = define(project, QStringLiteral("used-lamp"));
		Element *first = place(project, folio, lamp);
		Element *second = place(project, folio, lamp);
		QVERIFY(first && second);
		DiagramContext info = first->elementInformations();
		info.addValue(QStringLiteral("manufacturer_reference"), QStringLiteral("N-1"));
		first->setElementInformations(info);
		QCOMPARE(PlaceLabelPartDialog::usedParts(second), QStringList{QStringLiteral("N-1")});
		QVERIFY(PlaceLabelPartDialog::usedParts(first).isEmpty());

		const MaterialListData data = typed();
		FindPartDialog::Filter all;
		QCOMPARE(FindPartDialog::matches(data, all, {QStringLiteral("n-1")}).first(), 3);
		QCOMPARE(FindPartDialog::matches(data, all).first(), 0);

		FindPartDialog dialog(data, second, QString(), PlaceLabelPartDialog::usedParts(second));
		auto *list = dialog.findChild<QTreeWidget *>(QStringLiteral("findPartList"));
		QVERIFY(list->topLevelItem(0)->text(0).startsWith(QChar(0x2605)));
		QVERIFY(list->topLevelItem(0)->text(0).endsWith(QStringLiteral("N-1")));
		QCOMPARE(dialog.partNumber(), QStringLiteral("N-1"));
		QVERIFY(!list->topLevelItem(1)->text(0).startsWith(QChar(0x2605)));
	}

	void categoryOfMapsKeysNamesAndKeywords()
	{
		const auto category = [](const QString &type, const QString &description, const QString &column = QString()) {
			MaterialRecord record;
			record.setValue(QStringLiteral("device_type"), type);
			record.setValue(QStringLiteral("description"), description);
			record.setValue(QStringLiteral("category"), column);
			return PartCategory::categoryOf(record);
		};
			//The column: a key, the English name, in any case
		QCOMPARE(category(QString(), QString(), QStringLiteral("relay")), QStringLiteral("relay"));
		QCOMPARE(category(QString(), QString(), QStringLiteral("Circuit Breaker")), QStringLiteral("circuit_breaker"));
		QCOMPARE(category(QStringLiteral("CONTR10"), QString(), QStringLiteral("PLC")), QStringLiteral("plc"));
			//Else the keywords
		QCOMPARE(category(QStringLiteral("CONTR10"), QString()), QStringLiteral("contactor"));
		QCOMPARE(category(QString(), QStringLiteral("3P contactor 24V")), QStringLiteral("contactor"));
		QCOMPARE(category(QString(), QStringLiteral("Power relay")), QStringLiteral("relay"));
		QCOMPARE(category(QString(), QStringLiteral("PSU 24V 10A")), QStringLiteral("power_supply"));
		QCOMPARE(category(QString(), QStringLiteral("Power supply")), QStringLiteral("power_supply"));
		QCOMPARE(category(QString(), QStringLiteral("Fuse 10A")), QStringLiteral("fuse"));
		QCOMPARE(category(QString(), QStringLiteral("Thermal overload relay")), QStringLiteral("overload"));
		QCOMPARE(category(QString(), QStringLiteral("Miniature breaker")), QStringLiteral("circuit_breaker"));
		QCOMPARE(category(QStringLiteral("CB-1P"), QString()), QStringLiteral("circuit_breaker"));
		QCOMPARE(category(QString(), QStringLiteral("MPCB 6-10A")), QStringLiteral("motor_starter"));
		QCOMPARE(category(QString(), QStringLiteral("Motor protection switch")), QStringLiteral("motor_starter"));
		QCOMPARE(category(QStringLiteral("TB-2"), QString()), QStringLiteral("terminal"));
		QCOMPARE(category(QString(), QStringLiteral("Terminal block")), QStringLiteral("terminal"));
		QCOMPARE(category(QString(), QStringLiteral("Socket 16A")), QStringLiteral("connector"));
		QCOMPARE(category(QString(), QStringLiteral("CPU 1214")), QStringLiteral("plc"));
		QCOMPARE(category(QString(), QStringLiteral("Proximity switch")), QStringLiteral("sensor"));
		QCOMPARE(category(QString(), QStringLiteral("Level switch")), QStringLiteral("sensor"));
		QCOMPARE(category(QString(), QStringLiteral("Motor 3 kW")), QStringLiteral("motor"));
		QCOMPARE(category(QString(), QStringLiteral("Ethernet cable CAT6")), QStringLiteral("data_cable"));
		QCOMPARE(category(QString(), QStringLiteral("Shielded cable 2x0.5")), QStringLiteral("instrument_cable"));
		QCOMPARE(category(QString(), QStringLiteral("Cable 3G1.5")), QStringLiteral("power_cable"));
		QCOMPARE(category(QString(), QStringLiteral("PLC controller")), QStringLiteral("plc"));
		QCOMPARE(category(QString(), QStringLiteral("Something odd")), QStringLiteral("other"));
			//What a part is for wins over what it fits
		QCOMPARE(category(QString(), QStringLiteral("Transparent sleeve, for cable 3 - 4mm OD")), QStringLiteral("marking"));
		QCOMPARE(category(QString(), QStringLiteral("Terminal marking FW 1-50")), QStringLiteral("marking"));
		QCOMPARE(category(QString(), QStringLiteral("Auxiliary 1 N/O front mtg, for contactor")), QStringLiteral("accessory"));
		QCOMPARE(category(QString(), QStringLiteral("Control cable 6C+E 1.5mm2")), QStringLiteral("control_cable"));
		QCOMPARE(category(QString(), QStringLiteral("Cable for 32 I/O module 2.5m")), QStringLiteral("data_cable"));
		QCOMPARE(category(QString(), QStringLiteral("Variable speed drive 4kW")), QStringLiteral("drive"));
		QCOMPARE(category(QString(), QStringLiteral("Soft starter 22A")), QStringLiteral("drive"));
		QCOMPARE(category(QString(), QStringLiteral("Head for illum PB green")), QStringLiteral("pushbutton"));
		QCOMPARE(category(QString(), QStringLiteral("Main isolator 3P 63A")), QStringLiteral("switch"));
		QCOMPARE(category(QString(), QStringLiteral("Xenon beacon red")), QStringLiteral("pilot_light"));
		QCOMPARE(category(QString(), QStringLiteral("Midi solenoid valve 24 VDC")), QStringLiteral("valve"));
		QCOMPARE(category(QString(), QStringLiteral("Earth bar 165A 24 hole")), QStringLiteral("busbar"));
		QCOMPARE(category(QString(), QStringLiteral("Black closed slotted trunking 25Wx30H")), QStringLiteral("mounting"));
		QCOMPARE(category(QString(), QStringLiteral("Timer on delay 24V")), QStringLiteral("relay"));
		QCOMPARE(PartCategory::keys().size(), 28);
		QCOMPARE(PartCategory::keys().last(), QStringLiteral("other"));
		QCOMPARE(PartCategory::keyOf(QStringLiteral("push BUTTON")), QStringLiteral("pushbutton"));
		QCOMPARE(PartCategory::keyOf(QStringLiteral("nothing")), QString());
	}

	void facetsNeverOfferAValueWithoutResult()
	{
		const MaterialListData data = sample();
		Filter filter;
		auto counts = FindPartDialog::facetCounts(data, filter);
		QCOMPARE(counts.at(FindPartDialog::Category).value(QStringLiteral("contactor")), 3);
		QCOMPARE(counts.at(FindPartDialog::Category).value(QStringLiteral("relay")), 2);
		QCOMPARE(counts.at(FindPartDialog::Manufacturer).value(QStringLiteral("Schneider")), 4);

			//Other filters: Omron makes no contactor
		filter.values[FindPartDialog::Category] = QStringLiteral("contactor");
		counts = FindPartDialog::facetCounts(data, filter);
		QVERIFY(!counts.at(FindPartDialog::Manufacturer).contains(QStringLiteral("Omron")));
		QCOMPARE(counts.at(FindPartDialog::Manufacturer).value(QStringLiteral("Schneider")), 3);
			//Its own facet ignores its own filter: relays are still offered
		QCOMPARE(counts.at(FindPartDialog::Category).value(QStringLiteral("relay")), 2);

			//Every value offered gives results
		filter.values[FindPartDialog::Voltage] = QStringLiteral("230V");
		counts = FindPartDialog::facetCounts(data, filter);
		for (int f = 0 ; f < FindPartDialog::FacetCount ; ++f) {
			const auto values = counts.at(f);
			for (auto it = values.cbegin() ; it != values.cend() ; ++it) {
				Filter probe = filter;
				probe.values[f] = it.key();
				QCOMPARE(FindPartDialog::matches(data, probe).size(), it.value());
				QVERIFY(it.value() > 0);
			}
		}
		QCOMPARE(counts.at(FindPartDialog::Supplier).keys(), QStringList{QStringLiteral("Farnell")});

			//The search text narrows the facets too
		Filter text;
		text.text = QStringLiteral("omron");
		counts = FindPartDialog::facetCounts(data, text);
		QCOMPARE(counts.at(FindPartDialog::Manufacturer).keys(), QStringList{QStringLiteral("Omron")});
	}

	void filtersCombine()
	{
		const MaterialListData data = sample();
		Filter filter;
		QCOMPARE(FindPartDialog::matches(data, filter).size(), 6);
		filter.values[FindPartDialog::Category] = QStringLiteral("contactor");
		QCOMPARE(FindPartDialog::matches(data, filter), (QList<int>{0, 1, 2}));
		filter.values[FindPartDialog::Voltage] = QStringLiteral("24V");
		QCOMPARE(FindPartDialog::matches(data, filter), (QList<int>{0, 1}));
		filter.text = QStringLiteral("12A");
		QCOMPARE(FindPartDialog::matches(data, filter), (QList<int>{1}));
		QCOMPARE(filter.activeCount(), 3);
		filter.suits = true;
		filter.suitable = {0, 2};
		QCOMPARE(FindPartDialog::matches(data, filter).size(), 0);
		filter.text.clear();
		QCOMPARE(FindPartDialog::matches(data, filter), (QList<int>{0}));
	}

	void theDialogOffersCountsAndKeepsSelections()
	{
		FindPartDialog dialog(sample(), nullptr, QString(), QStringList());
		QCOMPARE(dialog.windowTitle(), QStringLiteral("Find part"));
		QComboBox *category = combo(dialog, "findPartCategory");
		QComboBox *manufacturer = combo(dialog, "findPartManufacturer");
		QVERIFY(category && manufacturer && combo(dialog, "findPartDeviceType") && combo(dialog, "findPartVoltage")
				&& combo(dialog, "findPartCurrent") && combo(dialog, "findPartSupplier"));
		QCOMPARE(category->itemText(0), QStringLiteral("All"));
		QVERIFY(category->findText(QStringLiteral("Contactor (3)")) >= 0);
		QVERIFY(category->findText(QStringLiteral("Relay (2)")) >= 0);

		choose(category, QStringLiteral("contactor"));
		QCOMPARE(category->currentData().toString(), QStringLiteral("contactor"));
		QCOMPARE(manufacturer->findData(QStringLiteral("Omron")), -1);
		QCOMPARE(dialog.resultCount(), 3);
		QCOMPARE(dialog.findChild<QLabel *>(QStringLiteral("findPartCount"))->text(), QStringLiteral("3 of 6 parts"));
		QCOMPARE(chips(dialog), 1);

		auto *list = dialog.findChild<QTreeWidget *>(QStringLiteral("findPartList"));
		QVERIFY(list);
		QCOMPARE(list->topLevelItemCount(), 3);
		QVERIFY(list->currentItem());
		QCOMPARE(dialog.partNumber(), QStringLiteral("LC1D09BD"));
	}

	void typingNarrowsTheListAtOnce()
	{
		FindPartDialog dialog(sample(), nullptr, QString("Q1"), QStringList());
		QCOMPARE(dialog.windowTitle(), QStringLiteral("Find part for Q1"));
		auto *search = dialog.findChild<QLineEdit *>(QStringLiteral("findPartSearch"));
		auto *list = dialog.findChild<QTreeWidget *>(QStringLiteral("findPartList"));
		QVERIFY(search && list);
		QTest::keyClicks(search, QStringLiteral("relay"));
		QCOMPARE(list->topLevelItemCount(), 2);
		QCOMPARE(list->topLevelItem(0)->text(0), QStringLiteral("G2R-1"));    // the whole word first
		QCOMPARE(list->currentItem(), list->topLevelItem(0));
		QCOMPARE(chips(dialog), 1);
	}

	void resetKeepsTheTextShowAllClearsIt()
	{
		FindPartDialog dialog(sample(), nullptr, QString(), QStringList());
		auto *search = dialog.findChild<QLineEdit *>(QStringLiteral("findPartSearch"));
		search->setText(QStringLiteral("schneider"));
		choose(combo(dialog, "findPartCategory"), QStringLiteral("relay"));
		choose(combo(dialog, "findPartVoltage"), QStringLiteral("24V"));
		QCOMPARE(dialog.resultCount(), 1);
		QCOMPARE(chips(dialog), 3);       // the text and two filters

		click(dialog, "findPartReset");
		QCOMPARE(search->text(), QStringLiteral("schneider"));
		QCOMPARE(combo(dialog, "findPartCategory")->currentData().toString(), QString());
		QCOMPARE(combo(dialog, "findPartVoltage")->currentData().toString(), QString());
		QCOMPARE(dialog.resultCount(), 4);
		QCOMPARE(chips(dialog), 1);

		choose(combo(dialog, "findPartCategory"), QStringLiteral("relay"));
		click(dialog, "findPartShowAll");
		QVERIFY(search->text().isEmpty());
		QCOMPARE(dialog.resultCount(), 6);
		QCOMPARE(chips(dialog), 0);
		QVERIFY(dialog.filter().activeCount() == 0);
	}

	void aChipRemovesItsFilter()
	{
		FindPartDialog dialog(sample(), nullptr, QString(), QStringList());
		choose(combo(dialog, "findPartCategory"), QStringLiteral("contactor"));
		choose(combo(dialog, "findPartVoltage"), QStringLiteral("24V"));
		QCOMPARE(chips(dialog), 2);
		auto *chip = dialog.findChild<QWidget *>(QStringLiteral("findPartChips"))
				->findChildren<QToolButton *>(QStringLiteral("findPartChip")).first();
		QVERIFY(chip->text().contains(QStringLiteral("Category: Contactor")));
		chip->click();
		QCOMPARE(chips(dialog), 1);
		QCOMPARE(combo(dialog, "findPartCategory")->currentData().toString(), QString());
		QCOMPARE(combo(dialog, "findPartVoltage")->currentData().toString(), QStringLiteral("24V"));
	}

	void theEmptyStateOffersWayOut()
	{
		FindPartDialog dialog(sample(), nullptr, QString(), QStringList());
		auto *pages = dialog.findChild<QWidget *>(QStringLiteral("findPartList"))->parentWidget();
		QVERIFY(qobject_cast<QStackedWidget *>(pages));
		auto *empty = dialog.findChild<QWidget *>(QStringLiteral("findPartEmpty"));
		QVERIFY(empty);
		QVERIFY(pages->isAncestorOf(empty));
		auto *search = dialog.findChild<QLineEdit *>(QStringLiteral("findPartSearch"));
		search->setText(QStringLiteral("schneider"));
		choose(combo(dialog, "findPartCategory"), QStringLiteral("relay"));
		choose(combo(dialog, "findPartVoltage"), QStringLiteral("24V"));
		QCOMPARE(dialog.resultCount(), 1);
			//The search narrows the list of the combos: a text no part has
		search->setText(QStringLiteral("omron"));
		QCOMPARE(dialog.resultCount(), 0);
		QCOMPARE(static_cast<QStackedWidget *>(pages)->currentWidget(), empty);
		auto *filters = dialog.findChild<QLabel *>(QStringLiteral("findPartEmptyFilters"));
		QVERIFY(filters->text().contains(QStringLiteral("Category: Relay")));
		auto *ok = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
		QVERIFY(!ok->isEnabled());
			//The category stays selected though nothing gives it now
		QCOMPARE(combo(dialog, "findPartCategory")->currentData().toString(), QStringLiteral("relay"));

		click(dialog, "findPartSearchAll");
		QCOMPARE(search->text(), QStringLiteral("omron"));
		QCOMPARE(dialog.resultCount(), 1);
		QVERIFY(static_cast<QStackedWidget *>(pages)->currentWidget() != empty);
		QVERIFY(ok->isEnabled());
		QCOMPARE(dialog.partNumber(), QStringLiteral("G2R-1"));
	}

	void openingStartsAfresh()
	{
		{
			FindPartDialog dialog(sample(), nullptr, QString(), QStringList());
			choose(combo(dialog, "findPartCategory"), QStringLiteral("relay"));
		}
		FindPartDialog again(sample(), nullptr, QString(), QStringList());
		QCOMPARE(again.filter().activeCount(), 0);
		QCOMPARE(again.resultCount(), 6);
	}
};

QET_APP_TEST_MAIN(tst_findpart)
#include "tst_findpart.moc"
