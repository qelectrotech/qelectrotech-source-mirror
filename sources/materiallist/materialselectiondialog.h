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
#ifndef MATERIALSELECTIONDIALOG_H
#define MATERIALSELECTIONDIALOG_H

#include <materiallist/materiallist.h>

#include <QDialog>

class QTreeWidgetItem;

namespace Ui {
	class MaterialSelectionDialog;
}

/**
	@brief The dialog listing every article of the material file in a
	split view: a searchable/filterable tree grouped by manufacturer on the
	left, and a Field/Value inspector and editor on the right.

	Bottom actions allow creating a new entry, editing the selected entry,
	opening QElectroTech settings to configure the materials list file, or
	applying the selected entry to the element.
*/
class MaterialSelectionDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit MaterialSelectionDialog(const QString &path,
										 QWidget *parent = nullptr);
		~MaterialSelectionDialog() override;

		MaterialRecord selectedRecord() const;
		void accept() override;
		void done(int result) override;

	private slots:
		void on_m_new_entry_btn_clicked();
		void on_m_edit_entry_btn_clicked();
		void on_m_duplicate_entry_btn_clicked();
		void on_m_remove_entry_btn_clicked();
		void on_m_import_btn_clicked();
		void on_m_export_btn_clicked();
		void on_m_settings_btn_clicked();
		void on_m_save_entry_btn_clicked();
		void on_m_cancel_entry_btn_clicked();
		void filterFieldChanged();
		void filterUpdated();
		void treeSelectionChanged();
		void treeItemDoubleClicked(QTreeWidgetItem *item, int column);

	private:
		enum class EditMode {
			View,
			New,
			Edit
		};

		void reload();
		void populateFilterFields(const QString &keep_field);
		void populateFilterValues(const QString &keep_value);
		void rebuildTree();
		void populateDetails(const MaterialRecord &record);
		void setEditMode(EditMode mode);
		MaterialRecord detailsRecord() const;
		int selectedSourceRow() const;
		QList<int> selectedSourceRows() const;
		bool selectRecord(const MaterialRecord &record);
		void updateButtons();
		void updateCount(int visible_count);
		QStringList effectiveColumns() const;
		QList<MaterialRecord> selectedRecordsForExport() const;
		QList<MaterialRecord> visibleRecordsForExport() const;
		void exportRecordsToCsv(const QList<MaterialRecord> &records);
		void copyDetailsSelection();
		void showDetailsContextMenu(const QPoint &pos);
		void showTreeContextMenu(const QPoint &pos);

		Ui::MaterialSelectionDialog *ui;
		QString m_path;
		MaterialListData m_data;
		EditMode m_edit_mode = EditMode::View;
		int m_editing_row = -1;
		MaterialRecord m_editing_original;
};

#endif // MATERIALSELECTIONDIALOG_H
