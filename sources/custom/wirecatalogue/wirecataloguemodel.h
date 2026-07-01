/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WIRECATALOGUEMODEL_H
#define WIRECATALOGUEMODEL_H

#include "wirespec.h"

#include <QAbstractTableModel>
#include <QPointer>
#include <QVector>

class WireCatalogueDb;

/**
	@brief The WireCatalogueModel class
	Read-only table model over a WireCatalogueDb. The view shows a snapshot
	(QVector<WireSpec>) refreshed from the database; the current text filter is
	applied through WireCatalogueDb::search(). Editing happens through the
	add/edit dialog, after which the owner calls refresh().
*/
class WireCatalogueModel : public QAbstractTableModel
{
	Q_OBJECT

	public:
		enum Column {
			WireId = 0,
			Type,          ///< "Wire" (1 core) or "Cable" (>1 core)
			Manufacturer,
			ManufacturerPartNo,
			CrossSection,
			Cores,
			Colors,
			Shield,
			Voltage,
			Temp,
			ColumnCount   ///< keep last
		};

		explicit WireCatalogueModel(WireCatalogueDb *db, QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		int columnCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

		/// Reload rows from the database, honouring the current filter text.
		void refresh();
		/// Set the search text and reload.
		void setFilter(const QString &text);

		WireSpec wireAt(int row) const;
		WireSpec wireForIndex(const QModelIndex &index) const;

	private:
		QPointer<WireCatalogueDb> m_db;
		QVector<WireSpec>         m_rows;
		QString                   m_filter;
};

#endif // WIRECATALOGUEMODEL_H
