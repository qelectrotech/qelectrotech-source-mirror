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
#include "materiallist/materiallist.h"
#include "materiallist/materialselectiondialog.h"
#include "qetapptest.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

class tst_materiallistedit : public QObject
{
	Q_OBJECT

	private slots:
		void testUpdateRecordPreservesRowsAndExtra();
		void testUpdateRecordFallbackWhenRowShifted();
		void testSplitViewTreeFilterAndDetails();
		void testInlineNewAndEditEntry();
};

void tst_materiallistedit::testUpdateRecordPreservesRowsAndExtra()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("materials.csv"));

	MaterialListData initial;
	initial.columns = {
		QStringLiteral("description"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference")
	};

	MaterialRecord first;
	first.setValue(QStringLiteral("description"), QStringLiteral("Contactor"));
	first.setValue(QStringLiteral("manufacturer"), QStringLiteral("Siemens"));
	first.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("3RT2015"));
	first.extra = {QStringLiteral("extra_col_val")};

	MaterialRecord second;
	second.setValue(QStringLiteral("description"), QStringLiteral("Relay"));
	second.setValue(QStringLiteral("manufacturer"), QStringLiteral("Finder"));
	second.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("40.52"));

	initial.records = {first, second};
	QString error;
	QVERIFY(MaterialList::writeFile(path, initial, &error));

	MaterialRecord edited = first;
	edited.setValue(QStringLiteral("description"), QStringLiteral("Power Contactor"));
	edited.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("3RT2016"));
	edited.extra.clear();

	QVERIFY(MaterialList::updateRecord(path, 0, first, edited, &error));
	QVERIFY(error.isEmpty());

	MaterialListData reloaded;
	QVERIFY(MaterialList::load(path, &reloaded, &error));
	QCOMPARE(reloaded.records.size(), 2);
	QCOMPARE(reloaded.records.at(0).value(QStringLiteral("description")),
			 QStringLiteral("Power Contactor"));
	QCOMPARE(reloaded.records.at(0).value(QStringLiteral("manufacturer_reference")),
			 QStringLiteral("3RT2016"));
	QCOMPARE(reloaded.records.at(0).extra, QStringList{QStringLiteral("extra_col_val")});
	QCOMPARE(reloaded.records.at(1), second);
}

void tst_materiallistedit::testUpdateRecordFallbackWhenRowShifted()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("materials.csv"));

	MaterialListData data;
	data.columns = {QStringLiteral("description"), QStringLiteral("manufacturer")};

	MaterialRecord inserted;
	inserted.setValue(QStringLiteral("description"), QStringLiteral("Fuse"));
	inserted.setValue(QStringLiteral("manufacturer"), QStringLiteral("ABB"));

	MaterialRecord target;
	target.setValue(QStringLiteral("description"), QStringLiteral("Breaker"));
	target.setValue(QStringLiteral("manufacturer"), QStringLiteral("Schneider"));

	data.records = {inserted, target};
	QString error;
	QVERIFY(MaterialList::writeFile(path, data, &error));

	MaterialRecord updated = target;
	updated.setValue(QStringLiteral("description"), QStringLiteral("MCB 16A"));

	QVERIFY(MaterialList::updateRecord(path, 0, target, updated, &error));

	MaterialListData reloaded;
	QVERIFY(MaterialList::load(path, &reloaded, &error));
	QCOMPARE(reloaded.records.size(), 2);
	QCOMPARE(reloaded.records.at(0), inserted);
	QCOMPARE(reloaded.records.at(1).value(QStringLiteral("description")),
			 QStringLiteral("MCB 16A"));

	MaterialRecord missing;
	missing.setValue(QStringLiteral("description"), QStringLiteral("DoesNotExist"));
	QVERIFY(!MaterialList::updateRecord(path, 0, missing, updated, &error));
	QVERIFY(!error.isEmpty());
}

void tst_materiallistedit::testSplitViewTreeFilterAndDetails()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("materials.csv"));

	MaterialListData initial;
	initial.columns = {
		QStringLiteral("designation"),
		QStringLiteral("description"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("category")
	};

	MaterialRecord siemens;
	siemens.setValue(QStringLiteral("designation"), QStringLiteral("ART-815"));
	siemens.setValue(QStringLiteral("description"), QStringLiteral("Fuse"));
	siemens.setValue(QStringLiteral("manufacturer"), QStringLiteral("Siemens"));
	siemens.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("5SY4116"));
	siemens.setValue(QStringLiteral("category"), QStringLiteral("Protection"));

	MaterialRecord hager;
	hager.setValue(QStringLiteral("designation"), QStringLiteral("ART-100"));
	hager.setValue(QStringLiteral("description"), QStringLiteral("SLS 50A"));
	hager.setValue(QStringLiteral("manufacturer"), QStringLiteral("Hager"));
	hager.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("HTS350E"));
	hager.setValue(QStringLiteral("category"), QStringLiteral("Main Breaker"));

	initial.records = {siemens, hager};
	QVERIFY(MaterialList::writeFile(path, initial));

	MaterialSelectionDialog dialog(path);
	auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("m_tree_widget"));
	auto *details = dialog.findChild<QTableWidget *>(QStringLiteral("m_details_table"));
	auto *group_combo = dialog.findChild<QComboBox *>(QStringLiteral("m_group_combo"));
	auto *filter_field = dialog.findChild<QComboBox *>(QStringLiteral("m_filter_field_combo"));
	auto *filter_value = dialog.findChild<QComboBox *>(QStringLiteral("m_filter_value_combo"));
	auto *search = dialog.findChild<QLineEdit *>(QStringLiteral("m_search_edit"));
	auto *edit_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_edit_entry_btn"));
	auto *settings_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_settings_btn"));

	QVERIFY(tree && details && group_combo && filter_field && filter_value && search && edit_btn && settings_btn);
	QCOMPARE(tree->topLevelItemCount(), 2);
	QVERIFY(!edit_btn->isEnabled());
	QVERIFY(!filter_value->isEnabled());

	// Filter by Manufacturer -> Siemens
	const int mfr_field_idx = filter_field->findData(QStringLiteral("manufacturer"));
	QVERIFY(mfr_field_idx >= 0);
	filter_field->setCurrentIndex(mfr_field_idx);
	QVERIFY(filter_value->isEnabled());
	const int siemens_idx = filter_value->findText(QStringLiteral("Siemens"));
	QVERIFY(siemens_idx >= 0);
	filter_value->setCurrentIndex(siemens_idx);
	QCOMPARE(tree->topLevelItemCount(), 1);
	QTreeWidgetItem *group = tree->topLevelItem(0);
	QCOMPARE(group->childCount(), 1);
	// Column 0 shows designation (Article number), NOT manufacturer_reference (Order number)
	QCOMPARE(group->child(0)->text(0), QStringLiteral("ART-815"));

	// Select Siemens item -> details table on the right updates
	tree->setCurrentItem(group->child(0));
	QVERIFY(edit_btn->isEnabled());
	QCOMPARE(details->rowCount(), 5);
	QCOMPARE(details->item(0, 1)->text(), QStringLiteral("ART-815"));
	QCOMPARE(details->item(1, 1)->text(), QStringLiteral("Fuse"));
	QCOMPARE(details->item(2, 1)->text(), QStringLiteral("Siemens"));
	QCOMPARE(details->item(3, 1)->text(), QStringLiteral("5SY4116"));
	QCOMPARE(details->item(4, 1)->text(), QStringLiteral("Protection"));

	// Filter by Category -> Main Breaker
	const int cat_field_idx = filter_field->findData(QStringLiteral("category"));
	QVERIFY(cat_field_idx >= 0);
	filter_field->setCurrentIndex(cat_field_idx);
	const int breaker_idx = filter_value->findText(QStringLiteral("Main Breaker"));
	QVERIFY(breaker_idx >= 0);
	filter_value->setCurrentIndex(breaker_idx);
	QCOMPARE(tree->topLevelItemCount(), 1);
	QCOMPARE(tree->topLevelItem(0)->child(0)->text(0), QStringLiteral("ART-100"));

	// Turn filter Off and switch Group to None (flat list)
	filter_field->setCurrentIndex(0);
	QVERIFY(!filter_value->isEnabled());
	group_combo->setCurrentIndex(0);
	QCOMPARE(tree->topLevelItemCount(), 2);
	QCOMPARE(tree->topLevelItem(0)->childCount(), 0);

	// Search mismatch hides items
	search->setText(QStringLiteral("NonExistent"));
	QCOMPARE(tree->topLevelItemCount(), 0);
}

void tst_materiallistedit::testInlineNewAndEditEntry()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("materials.csv"));

	MaterialListData initial;
	initial.columns = {
		QStringLiteral("designation"),
		QStringLiteral("description"),
		QStringLiteral("manufacturer")
	};
	MaterialRecord rec;
	rec.setValue(QStringLiteral("designation"), QStringLiteral("EAT.1"));
	rec.setValue(QStringLiteral("description"), QStringLiteral("Switch"));
	rec.setValue(QStringLiteral("manufacturer"), QStringLiteral("Eaton"));
	initial.records = {rec};
	QVERIFY(MaterialList::writeFile(path, initial));

	MaterialSelectionDialog dialog(path);
	auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("m_tree_widget"));
	auto *details = dialog.findChild<QTableWidget *>(QStringLiteral("m_details_table"));
	auto *new_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_new_entry_btn"));
	auto *edit_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_edit_entry_btn"));
	auto *save_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_save_entry_btn"));
	QVERIFY(tree && details && new_btn && edit_btn && save_btn);

	// Select existing item and edit it inline on the right
	QTreeWidgetItem *child = tree->topLevelItem(0)->child(0);
	tree->setCurrentItem(child);
	QVERIFY(edit_btn->isEnabled());
	edit_btn->click();

	details->item(1, 1)->setText(QStringLiteral("Main Switch"));
	save_btn->click();

	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("description")),
			 QStringLiteral("Main Switch"));

	// Create a new entry inline on the right
	new_btn->click();
	details->item(0, 1)->setText(QStringLiteral("ABB.2"));
	details->item(1, 1)->setText(QStringLiteral("Breaker"));
	details->item(2, 1)->setText(QStringLiteral("ABB"));
	save_btn->click();

	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("designation")),
			 QStringLiteral("ABB.2"));

	MaterialListData reloaded;
	QVERIFY(MaterialList::load(path, &reloaded));
	QCOMPARE(reloaded.records.size(), 2);
	QCOMPARE(reloaded.records.at(0).value(QStringLiteral("description")),
			 QStringLiteral("Main Switch"));
	QCOMPARE(reloaded.records.at(1).value(QStringLiteral("designation")),
			 QStringLiteral("ABB.2"));
}

QET_APP_TEST_MAIN(tst_materiallistedit)
#include "tst_materiallistedit.moc"
