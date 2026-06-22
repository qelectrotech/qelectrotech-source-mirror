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
#include "wirecataloguemodel.h"
#include "wirecataloguedb.h"
#include "iec60757.h"

WireCatalogueModel::WireCatalogueModel(WireCatalogueDb *db, QObject *parent) :
	QAbstractTableModel(parent),
	m_db(db)
{
	refresh();
}

int WireCatalogueModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	return m_rows.size();
}

int WireCatalogueModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	return ColumnCount;
}

QVariant WireCatalogueModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
		return QVariant();

	const WireSpec &w = m_rows.at(index.row());

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
			case WireId:             return w.wireId;
			case Manufacturer:       return w.manufacturerName;
			case ManufacturerPartNo: return w.manufacturerPartNo;
			case CrossSection:       return w.crossSectionMm2;
			case Cores:              return w.numCores;
			case Colors:             return w.coreColors.join(QStringLiteral(", "));
			case Shield:             return w.hasShield ? w.shieldType : QString();
			case Voltage:            return w.voltageRatingV;
			case Temp:               return w.tempRatingC;
			default:                 return QVariant();
		}
	}

	if (role == Qt::DecorationRole) {
		// Colour swatches live only in the Colours column (the Wire ID column
		// is kept plain so every row looks consistent).
		if (index.column() == Colors && !w.coreColors.isEmpty())
			return Iec60757::swatchStrip(w.coreColors, 14);
		return QVariant();
	}

	if (role == Qt::TextAlignmentRole) {
		switch (index.column()) {
			case CrossSection:
			case Cores:
			case Voltage:
			case Temp:
				return int(Qt::AlignRight | Qt::AlignVCenter);
			default:
				return int(Qt::AlignLeft | Qt::AlignVCenter);
		}
	}

	return QVariant();
}

QVariant WireCatalogueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QAbstractTableModel::headerData(section, orientation, role);

	switch (section) {
		case WireId:             return tr("Wire ID");
		case Manufacturer:       return tr("Manufacturer");
		case ManufacturerPartNo: return tr("Mfr part no.");
		case CrossSection:       return tr("Cross-sectional area (mm²)");
		case Cores:              return tr("Cores");
		case Colors:             return tr("Colours");
		case Shield:             return tr("Shield");
		case Voltage:            return tr("V");
		case Temp:               return tr("°C");
		default:                 return QVariant();
	}
}

void WireCatalogueModel::refresh()
{
	beginResetModel();
	if (m_db)
		m_rows = m_db->search(m_filter);
	else
		m_rows.clear();
	endResetModel();
}

void WireCatalogueModel::setFilter(const QString &text)
{
	m_filter = text;
	refresh();
}

WireSpec WireCatalogueModel::wireAt(int row) const
{
	if (row >= 0 && row < m_rows.size())
		return m_rows.at(row);
	return WireSpec();
}

WireSpec WireCatalogueModel::wireForIndex(const QModelIndex &index) const
{
	return wireAt(index.row());
}
