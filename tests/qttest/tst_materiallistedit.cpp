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
#include "materiallist/materialentrydialog.h"
#include "materiallist/materiallist.h"
#include "materiallist/materialselectiondialog.h"
#include "qetapptest.h"

#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class tst_materiallistedit : public QObject
{
	Q_OBJECT

	private slots:
		void testUpdateRecordPreservesRowsAndExtra();
		void testUpdateRecordFallbackWhenRowShifted();
		void testEntryDialogEditMode();
		void testSelectionDialogEditButton();
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

	// Pass old row index 0 even though target is now at row 1
	QVERIFY(MaterialList::updateRecord(path, 0, target, updated, &error));

	MaterialListData reloaded;
	QVERIFY(MaterialList::load(path, &reloaded, &error));
	QCOMPARE(reloaded.records.size(), 2);
	QCOMPARE(reloaded.records.at(0), inserted);
	QCOMPARE(reloaded.records.at(1).value(QStringLiteral("description")),
			 QStringLiteral("MCB 16A"));

	// Missing original record must fail cleanly
	MaterialRecord missing;
	missing.setValue(QStringLiteral("description"), QStringLiteral("DoesNotExist"));
	QVERIFY(!MaterialList::updateRecord(path, 0, missing, updated, &error));
	QVERIFY(!error.isEmpty());
}

void tst_materiallistedit::testEntryDialogEditMode()
{
	const QStringList columns = {
		QStringLiteral("description"),
		QStringLiteral("manufacturer"),
		QStringLiteral("unit_price")
	};

	MaterialRecord initial;
	initial.setValue(QStringLiteral("description"), QStringLiteral("Terminal"));
	initial.setValue(QStringLiteral("manufacturer"), QStringLiteral("Phoenix"));
	initial.setValue(QStringLiteral("unit_price"), QStringLiteral("1.25"));
	initial.extra = {QStringLiteral("keep_me")};

	MaterialEntryDialog dialog(columns, initial);
	QCOMPARE(dialog.windowTitle(), QStringLiteral("Edit entry"));

	const QList<QLineEdit *> edits = dialog.findChildren<QLineEdit *>();
	QCOMPARE(edits.size(), 3);
	QCOMPARE(edits.at(0)->text(), QStringLiteral("Terminal"));
	QCOMPARE(edits.at(1)->text(), QStringLiteral("Phoenix"));
	QCOMPARE(edits.at(2)->text(), QStringLiteral("1.25"));

	edits.at(0)->setText(QStringLiteral("Feed-through terminal"));
	const MaterialRecord result = dialog.record();
	QCOMPARE(result.value(QStringLiteral("description")), QStringLiteral("Feed-through terminal"));
	QCOMPARE(result.value(QStringLiteral("manufacturer")), QStringLiteral("Phoenix"));
	QCOMPARE(result.value(QStringLiteral("unit_price")), QStringLiteral("1.25"));
	QCOMPARE(result.extra, QStringList{QStringLiteral("keep_me")});
}

void tst_materiallistedit::testSelectionDialogEditButton()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("materials.csv"));

	MaterialListData initial;
	initial.columns = {QStringLiteral("description"), QStringLiteral("manufacturer")};
	MaterialRecord rec;
	rec.setValue(QStringLiteral("description"), QStringLiteral("Switch"));
	rec.setValue(QStringLiteral("manufacturer"), QStringLiteral("Eaton"));
	initial.records = {rec};
	QVERIFY(MaterialList::writeFile(path, initial));

	MaterialSelectionDialog dialog(path);
	auto *edit_btn = dialog.findChild<QPushButton *>(QStringLiteral("m_edit_entry_btn"));
	auto *table = dialog.findChild<QTableView *>(QStringLiteral("m_table_view"));
	QVERIFY(edit_btn != nullptr);
	QVERIFY(table != nullptr);

	// Disabled when nothing is selected
	QVERIFY(!edit_btn->isEnabled());

	// Selecting the row enables Edit entry
	table->selectRow(0);
	QVERIFY(edit_btn->isEnabled());

	// Open the edit dialog via clicking Edit entry and update the description
	QTimer::singleShot(0, &dialog, [&dialog]() {
		auto *entry_dialog = dialog.findChild<MaterialEntryDialog *>();
		QVERIFY(entry_dialog != nullptr);
		const QList<QLineEdit *> edits = entry_dialog->findChildren<QLineEdit *>();
		QVERIFY(edits.size() >= 1);
		edits.at(0)->setText(QStringLiteral("Main Switch"));
		entry_dialog->accept();
	});

	edit_btn->click();

	QCOMPARE(dialog.selectedRecord().value(QStringLiteral("description")),
			 QStringLiteral("Main Switch"));

	MaterialListData reloaded;
	QVERIFY(MaterialList::load(path, &reloaded));
	QCOMPARE(reloaded.records.size(), 1);
	QCOMPARE(reloaded.records.at(0).value(QStringLiteral("description")),
			 QStringLiteral("Main Switch"));
}

QET_APP_TEST_MAIN(tst_materiallistedit)
#include "tst_materiallistedit.moc"
