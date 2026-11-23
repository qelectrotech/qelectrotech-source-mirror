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

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <QTreeWidget>

class tst_materiallistedit : public QObject
{
	Q_OBJECT

	private slots:
		void testUpdateRecordPreservesRowsAndExtra();
		void testUpdateRecordFallbackWhenRowShifted();
		void testSplitViewTreeFilterAndDetails();
		void testInlineNewAndEditEntry();
		void testImportAndExportCsv();
		void testRemoveEntry();
		void testDuplicateEntry();
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
	QCOMPARE(group_combo->currentData().toString(), QStringLiteral("category"));
	QCOMPARE(filter_field->count(), 1 + MaterialList::defaultColumns().size());
	QCOMPARE(tree->topLevelItemCount(), 2);
	QVERIFY(!edit_btn->isEnabled());
	QVERIFY(!filter_value->isEnabled());

	// Filter by any column, e.g. manufacturer_reference -> 5SY4116
	const int ref_field_idx = filter_field->findData(QStringLiteral("manufacturer_reference"));
	QVERIFY(ref_field_idx >= 0);
	filter_field->setCurrentIndex(ref_field_idx);
	QVERIFY(filter_value->isEnabled());
	const int ref_val_idx = filter_value->findText(QStringLiteral("5SY4116"));
	QVERIFY(ref_val_idx >= 0);
	filter_value->setCurrentIndex(ref_val_idx);
	QCOMPARE(tree->topLevelItemCount(), 1);
	QCOMPARE(tree->topLevelItem(0)->child(0)->text(0), QStringLiteral("ART-815"));

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
	QCOMPARE(details->rowCount(), MaterialList::defaultColumns().size());
	QCOMPARE(details->item(0, 1)->text(), QStringLiteral("ART-815"));
	QCOMPARE(details->item(1, 1)->text(), QStringLiteral("Fuse"));
	QCOMPARE(details->item(2, 1)->text(), QStringLiteral("Siemens"));
	QCOMPARE(details->item(3, 1)->text(), QStringLiteral("5SY4116"));
	QCOMPARE(details->item(4, 1)->text(), QStringLiteral("Protection"));

	// Select multiple rows in details table and copy as TSV + HTML for Excel
	QCOMPARE(details->selectionMode(), QAbstractItemView::ExtendedSelection);
	details->clearSelection();
	details->selectRow(0);
	details->selectionModel()->select(
		details->model()->index(2, 0),
		QItemSelectionModel::Select | QItemSelectionModel::Rows);
	auto *copy_shortcut = details->findChild<QShortcut *>();
	QVERIFY(copy_shortcut);
	emit copy_shortcut->activated();
	const QString expected_tsv = details->item(0, 0)->text() + QLatin1Char('\t') + QStringLiteral("ART-815\n")
							 + details->item(2, 0)->text() + QLatin1Char('\t') + QStringLiteral("Siemens");
	QCOMPARE(QApplication::clipboard()->text(), expected_tsv);
	QVERIFY(QApplication::clipboard()->mimeData()->hasHtml());
	QVERIFY(QApplication::clipboard()->mimeData()->html().contains(QStringLiteral("<table>")));

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
	auto *button_box = dialog.findChild<QDialogButtonBox *>(QStringLiteral("m_button_box"));
	QVERIFY(tree && details && new_btn && edit_btn && save_btn && button_box);

	// Select existing item and edit it inline on the right
	QTreeWidgetItem *child = tree->topLevelItem(0)->child(0);
	tree->setCurrentItem(child);
	QVERIFY(edit_btn->isEnabled());
	QVERIFY(!button_box->isHidden());
	QCOMPARE(details->columnCount(), 2);
	edit_btn->click();
	QVERIFY(button_box->isHidden());
	QCOMPARE(details->columnCount(), 3);
	QVERIFY(qobject_cast<QToolButton *>(details->cellWidget(1, 2)) != nullptr);

	details->item(1, 1)->setText(QStringLiteral("Main Switch\n230V"));
	int price_row = -1;
	for (int r = 0; r < details->rowCount(); ++r) {
		if (details->item(r, 0)->data(Qt::UserRole).toString() == QLatin1String("unit_price")) {
			price_row = r;
			break;
		}
	}
	QVERIFY(price_row >= 0);
	details->item(price_row, 1)->setText(QStringLiteral("12.50"));
	save_btn->click();
	QVERIFY(!button_box->isHidden());
	QCOMPARE(details->columnCount(), 2);

	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("description")),
			 QStringLiteral("Main Switch\n230V"));

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
			 QStringLiteral("Main Switch\n230V"));
	QCOMPARE(reloaded.records.at(0).value(QStringLiteral("unit_price")),
			 QStringLiteral("12.50"));
	QCOMPARE(reloaded.records.at(1).value(QStringLiteral("designation")),
			 QStringLiteral("ABB.2"));
}

void tst_materiallistedit::testImportAndExportCsv()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString target_path = dir.filePath(QStringLiteral("target.csv"));
	const QString import_path = dir.filePath(QStringLiteral("import.csv"));

	MaterialListData target;
	target.columns = {
		QStringLiteral("designation"),
		QStringLiteral("description"),
		QStringLiteral("manufacturer"),
		QStringLiteral("category")
	};
	MaterialRecord r1;
	r1.setValue(QStringLiteral("designation"), QStringLiteral("ART-1"));
	r1.setValue(QStringLiteral("description"), QStringLiteral("Old Fuse"));
	r1.setValue(QStringLiteral("manufacturer"), QStringLiteral("Siemens"));
	r1.setValue(QStringLiteral("category"), QStringLiteral("Protection"));
	target.records = {r1};
	QVERIFY(MaterialList::writeFile(target_path, target));

	MaterialListData incoming;
	incoming.columns = {
		QStringLiteral("designation"),
		QStringLiteral("description"),
		QStringLiteral("manufacturer"),
		QStringLiteral("category"),
		QStringLiteral("supplier")
	};
	MaterialRecord r1_updated;
	r1_updated.setValue(QStringLiteral("designation"), QStringLiteral("ART-1"));
	r1_updated.setValue(QStringLiteral("description"), QStringLiteral("New Fuse"));
	r1_updated.setValue(QStringLiteral("manufacturer"), QStringLiteral("Siemens"));
	r1_updated.setValue(QStringLiteral("category"), QStringLiteral("Protection"));
	r1_updated.setValue(QStringLiteral("supplier"), QStringLiteral("Rexel"));

	MaterialRecord r2;
	r2.setValue(QStringLiteral("designation"), QStringLiteral("ART-2"));
	r2.setValue(QStringLiteral("description"), QStringLiteral("Relay"));
	r2.setValue(QStringLiteral("manufacturer"), QStringLiteral("ABB"));
	r2.setValue(QStringLiteral("category"), QStringLiteral("Relays"));
	incoming.records = {r1_updated, r2};
	QVERIFY(MaterialList::writeFile(import_path, incoming));

	QCOMPARE(MaterialList::countImportConflicts(target, incoming), 1);

	MaterialList::ImportResult res;
	QVERIFY(MaterialList::importFromFile(
		target_path, import_path, MaterialList::ImportConflictPolicy::UpdateExisting, &res));
	QCOMPARE(res.added, 1);
	QCOMPARE(res.updated, 1);
	QCOMPARE(res.skipped, 0);

	MaterialListData merged;
	QVERIFY(MaterialList::load(target_path, &merged));
	QCOMPARE(merged.records.size(), 2);
	QVERIFY(merged.columns.contains(QStringLiteral("supplier")));
	QCOMPARE(merged.records.at(0).value(QStringLiteral("description")), QStringLiteral("New Fuse"));
	QCOMPARE(merged.records.at(0).value(QStringLiteral("supplier")), QStringLiteral("Rexel"));
	QCOMPARE(merged.records.at(1).value(QStringLiteral("designation")), QStringLiteral("ART-2"));

	// Re-importing the exact same data skips everything
	MaterialList::ImportResult res_skip;
	QVERIFY(MaterialList::importFromFile(
		target_path, import_path, MaterialList::ImportConflictPolicy::SkipExisting, &res_skip));
	QCOMPARE(res_skip.added, 0);
	QCOMPARE(res_skip.updated, 0);
	QCOMPARE(res_skip.skipped, 2);

	// Dialog exposes Import/Export buttons and disables Edit when a group is selected
	MaterialSelectionDialog dialog(target_path);
	auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("m_tree_widget"));
	auto *edit_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_edit_entry_btn"));
	auto *import_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_import_btn"));
	auto *export_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_export_btn"));
	QVERIFY(tree && edit_btn && import_btn && export_btn);
	QVERIFY(import_btn->isEnabled());
	QVERIFY(export_btn->isEnabled());

	tree->setCurrentItem(tree->topLevelItem(0));
	QVERIFY(!edit_btn->isEnabled());
}

void tst_materiallistedit::testRemoveEntry()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString csv_path = dir.filePath(QStringLiteral("remove_test.csv"));

	{
		QFile f(csv_path);
		QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
		f.write("designation;description;manufacturer\n"
				"A1;Desc 1;M1\n"
				"A2;Desc 2;M2\n"
				"A3;Desc 3;M3\n");
	}

	QString err;
	MaterialListData initial;
	QVERIFY2(MaterialList::load(csv_path, &initial, &err), qPrintable(err));
	QCOMPARE(initial.records.size(), 3);

	// Remove single middle record
	QVERIFY2(MaterialList::removeRecords(csv_path, {1}, {initial.records.at(1)}, &err),
			 qPrintable(err));
	MaterialListData after_one;
	QVERIFY2(MaterialList::load(csv_path, &after_one, &err), qPrintable(err));
	QCOMPARE(after_one.records.size(), 2);
	QCOMPARE(after_one.records.at(0).value(QStringLiteral("designation")), QStringLiteral("A1"));
	QCOMPARE(after_one.records.at(1).value(QStringLiteral("designation")), QStringLiteral("A3"));

	// Dialog UI checks: trash button & tree context menu
	MaterialSelectionDialog dialog(csv_path);
	auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("m_tree_widget"));
	auto *edit_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_edit_entry_btn"));
	auto *remove_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_remove_entry_btn"));
	QVERIFY(tree && edit_btn && remove_btn);
	QCOMPARE(tree->contextMenuPolicy(), Qt::CustomContextMenu);
	QVERIFY(!remove_btn->icon().isNull());
	QVERIFY(!remove_btn->isEnabled());

	// Group item selection keeps remove disabled
	QTreeWidgetItem *group0 = tree->topLevelItem(0);
	QVERIFY(group0 && group0->childCount() > 0);
	tree->setCurrentItem(group0);
	QVERIFY(!remove_btn->isEnabled());

	// Single article selection enables both edit and remove
	QTreeWidgetItem *item0 = group0->child(0);
	tree->setCurrentItem(item0);
	QVERIFY(edit_btn->isEnabled());
	QVERIFY(remove_btn->isEnabled());

	// Multi-selection enables remove while disabling single-row edit
	QVERIFY(group0->childCount() >= 2);
	QTreeWidgetItem *item1 = group0->child(1);
	item1->setSelected(true);
	QVERIFY(!edit_btn->isEnabled());
	QVERIFY(remove_btn->isEnabled());

	// Remove multiple records at once
	QVERIFY2(MaterialList::removeRecords(
				 csv_path,
				 {0, 1},
				 {after_one.records.at(0), after_one.records.at(1)},
				 &err),
			 qPrintable(err));
	MaterialListData after_all;
	QVERIFY2(MaterialList::load(csv_path, &after_all, &err), qPrintable(err));
	QCOMPARE(after_all.records.size(), 0);
}

void tst_materiallistedit::testDuplicateEntry()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString csv_path = dir.filePath(QStringLiteral("duplicate_test.csv"));

	{
		QFile f(csv_path);
		QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
		f.write("designation;description;manufacturer\n"
				"ART100;Contactor;Siemens\n");
	}

	MaterialSelectionDialog dialog(csv_path);
	auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("m_tree_widget"));
	auto *dup_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_duplicate_entry_btn"));
	QVERIFY(tree && dup_btn);
	QVERIFY(!dup_btn->icon().isNull());
	QVERIFY(!dup_btn->isEnabled());

	QTreeWidgetItem *group0 = tree->topLevelItem(0);
	QVERIFY(group0 && group0->childCount() == 1);
	tree->setCurrentItem(group0->child(0));
	QVERIFY(dup_btn->isEnabled());

	// Duplicate 3 times -> (1), (2), (3)
	dup_btn->click();
	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("designation")),
			 QStringLiteral("ART100 (1)"));
	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("description")),
			 QStringLiteral("Contactor"));

	dup_btn->click();
	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("designation")),
			 QStringLiteral("ART100 (2)"));

	dup_btn->click();
	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("designation")),
			 QStringLiteral("ART100 (3)"));

	MaterialListData reloaded;
	QString err;
	QVERIFY2(MaterialList::load(csv_path, &reloaded, &err), qPrintable(err));
	QCOMPARE(reloaded.records.size(), 4);
	QCOMPARE(reloaded.records.at(0).value(QStringLiteral("designation")), QStringLiteral("ART100"));
	QCOMPARE(reloaded.records.at(1).value(QStringLiteral("designation")), QStringLiteral("ART100 (1)"));
	QCOMPARE(reloaded.records.at(2).value(QStringLiteral("designation")), QStringLiteral("ART100 (2)"));
	QCOMPARE(reloaded.records.at(3).value(QStringLiteral("designation")), QStringLiteral("ART100 (3)"));
	QCOMPARE(reloaded.records.at(3).value(QStringLiteral("manufacturer")), QStringLiteral("Siemens"));
}

QET_APP_TEST_MAIN(tst_materiallistedit)
#include "tst_materiallistedit.moc"
