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

#include "../materiallist/materiallist.h"

#include <QAbstractTableModel>
#include <QDialog>
#include <QSortFilterProxyModel>

class QModelIndex;
class QItemSelectionModel;
class QWheelEvent;

namespace Ui {
	class MaterialSelectionDialog;
}

/**
	@brief Table model showing a material file.

	Every column of the file is a column of the model, named the way the
	user reads it (translated), so the dialog stays a plain read only
	table whatever the file contains.
*/
class MaterialTableModel : public QAbstractTableModel
{
	Q_OBJECT

	public:
		explicit MaterialTableModel(QObject *parent = nullptr);

		void setMaterial(const QStringList &columns, const QList<MaterialRecord> &records);
		QStringList columns() const {return m_columns;}
		QList<MaterialRecord> records() const {return m_records;}
		MaterialRecord record(int row) const;

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		int columnCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	private:
		QStringList m_columns;
		QList<MaterialRecord> m_records;
};

/**
	@brief Filter keeping only the rows matching every word typed by the
	user, whatever the column each word is found in.

	"Hilfsschalter Schneider" keeps the rows holding both words, so adding
	the manufacturer to the search narrows the result down, which is the
	behaviour of a bill of material lookup.
*/
class MaterialFilterProxy : public QSortFilterProxyModel
{
	Q_OBJECT

	public:
		explicit MaterialFilterProxy(MaterialTableModel *model, QObject *parent = nullptr);

		void setTokens(const QStringList &tokens);
		QStringList tokens() const {return m_tokens;}

	protected:
		bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

	private:
		MaterialTableModel *m_model = nullptr;
		QStringList m_tokens;
};

/**
	@brief The dialog listing every article of the material file, letting
	the user search one and hand it back to the element properties.

	Three buttons: apply the selected row, do nothing, or add a new row to
	the file.
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
		bool eventFilter(QObject *watched, QEvent *event) override;

	private slots:
		void on_m_new_entry_btn_clicked();
		void searchChanged(const QString &text);

	private:
		void reload();
		void restoreViewState();
		void saveViewState();
		void resetSorting();
		void scrollTable(QWheelEvent *event);
		void updateCount();
		void updateApplyButton();
		bool selectRecord(const MaterialRecord &record);

		//ATTRIBUTES
	private:
		Ui::MaterialSelectionDialog *ui;
		MaterialTableModel         *m_model = nullptr;
		MaterialFilterProxy        *m_proxy = nullptr;
		QString                     m_path;
		MaterialListData            m_data;
};

#endif // MATERIALSELECTIONDIALOG_H
