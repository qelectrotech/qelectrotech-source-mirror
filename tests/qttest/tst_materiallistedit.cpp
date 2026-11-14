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
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("description"),
		QStringLiteral("manufacturer"),
		QStringLiteral("order_number")
	};

	MaterialRecord siemens;
	siemens.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("SIE.815"));
	siemens.setValue(QStringLiteral("description"), QStringLiteral("Fuse"));
	siemens.setValue(QStringLiteral("manufacturer"), QStringLiteral("Siemens"));
	siemens.setValue(QStringLiteral("order_number"), QStringLiteral("815"));

	MaterialRecord hager;
	hager.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("HAG.100"));
	hager.setValue(QStringLiteral("description"), QStringLiteral("SLS 50A"));
	hager.setValue(QStringLiteral("manufacturer"), QStringLiteral("Hager"));

	initial.records = {siemens, hager};
	QVERIFY(MaterialList::writeFile(path, initial));

	MaterialSelectionDialog dialog(path);
	auto *tree = dialog.findChild<QTreeWidget *>(QStringLiteral("m_tree_widget"));
	auto *details = dialog.findChild<QTableWidget *>(QStringLiteral("m_details_table"));
	auto *filter = dialog.findChild<QComboBox *>(QStringLiteral("m_filter_combo"));
	auto *search = dialog.findChild<QLineEdit *>(QStringLiteral("m_search_edit"));
	auto *edit_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_edit_entry_btn"));
	auto *settings_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_settings_btn"));

	QVERIFY(tree && details && filter && search && edit_btn && settings_btn);
	QCOMPARE(tree->topLevelItemCount(), 2);
	QVERIFY(!edit_btn->isEnabled());

	// Filter to Siemens only
	const int siemens_idx = filter->findText(QStringLiteral("Siemens"));
	QVERIFY(siemens_idx >= 0);
	filter->setCurrentIndex(siemens_idx);
	QCOMPARE(tree->topLevelItemCount(), 1);
	QTreeWidgetItem *group = tree->topLevelItem(0);
	QCOMPARE(group->childCount(), 1);

	// Select Siemens item -> details table on the right updates
	tree->setCurrentItem(group->child(0));
	QVERIFY(edit_btn->isEnabled());
	QCOMPARE(details->rowCount(), 4);
	QCOMPARE(details->item(0, 1)->text(), QStringLiteral("SIE.815"));
	QCOMPARE(details->item(1, 1)->text(), QStringLiteral("Fuse"));
	QCOMPARE(details->item(2, 1)->text(), QStringLiteral("Siemens"));
	QCOMPARE(details->item(3, 1)->text(), QStringLiteral("815"));

	// Search mismatch hides item
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
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("description"),
		QStringLiteral("manufacturer")
	};
	MaterialRecord rec;
	rec.setValue(QStringLiteral("manufacturer_reference"), QStringLiteral("EAT.1"));
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

	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("manufacturer_reference")),
			 QStringLiteral("ABB.2"));

	MaterialListData reloaded;
	QVERIFY(MaterialList::load(path, &reloaded));
	QCOMPARE(reloaded.records.size(), 2);
	QCOMPARE(reloaded.records.at(0).value(QStringLiteral("description")),
			 QStringLiteral("Main Switch"));
	QCOMPARE(reloaded.records.at(1).value(QStringLiteral("manufacturer_reference")),
			 QStringLiteral("ABB.2"));
}

QET_APP_TEST_MAIN(tst_materiallistedit)
#include "tst_materiallistedit.moc"
