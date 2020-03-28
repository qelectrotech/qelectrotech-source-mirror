/*
        Copyright 2006-2020 QElectroTech Team
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
#include "nomenclaturemodel.h"
#include "qetapp.h"

#include <QModelIndex>
#include <QFont>

/**
 * @brief NomenclatureModel::NomenclatureModel
 * @param project :project of this nomenclature
 * @param parent : parent QObject
 */
NomenclatureModel::NomenclatureModel(QETProject *project, QObject *parent) :
	QAbstractTableModel(parent),
	m_project(project),
	m_database(project)
{}

/**
 * @brief NomenclatureModel::rowCount
 * Reimplemented for QAbstractTableModel
 * @param parent
 * @return
 */
int NomenclatureModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

	return  m_record.count();
}

/**
 * @brief NomenclatureModel::columnCount
 * Reimplemented for QAbstractTableModel
 * @param parent
 * @return
 */
int NomenclatureModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

	if (m_record.count()) {
		return m_record.first().count();
	}

	return 0;
}

/**
 * @brief NomenclatureModel::setHeaderData
 * Reimplemented from QAbstractTableModel.
 * Only horizontal orientation is accepted.
 * @param section
 * @param orientation
 * @param value
 * @param role
 * @return
 */
bool NomenclatureModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if (orientation == Qt::Vertical) {
		return false;
	}
	auto hash_ = m_header_data.value(section);
	hash_.insert(role, value);
	m_header_data.insert(section, hash_);
	headerDataChanged(orientation, section, section);
	return true;
}

/**
 * @brief NomenclatureModel::headerData
 * Reimplemented from QAbstractTableModel.
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant NomenclatureModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical) {
		return QVariant();
	}

	auto hash_ = m_header_data.value(section);
	if (role == Qt::DisplayRole && !hash_.contains(Qt::DisplayRole)) { //special case to have the same behavior as Qt
		return hash_.value(Qt::EditRole);
	}
	return m_header_data.value(section).value(role);
}

/**
 * @brief NomenclatureModel::setData
 * Only store the data for the index 0.0
 * @param index
 * @param value
 * @param role
 * @return
 */
bool NomenclatureModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() != 0 || index.column() != 0) {
		return false;
	}
	m_index_0_0_data.insert(role, value);
	emit dataChanged(index, index, QVector<int>(role));
	return true;
}

/**
 * @brief NomenclatureModel::data
 * Reimplemented for QAbstractTableModel
 * @param index
 * @param role
 * @return
 */
QVariant NomenclatureModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

	if (index.row() == 0 &&
		index.column() == 0 &&
		role != Qt::DisplayRole) {
		return m_index_0_0_data.value(role);
	}

	if (role == Qt::DisplayRole) {
		QVariant v(m_record.at(index.row()).at(index.column()));
		return v;
	}

	return QVariant();
}

/**
 * @brief NomenclatureModel::query
 * Query the internall bd with @query.
 * @param query
 */
void NomenclatureModel::query(const QString &query)
{
	m_query = query;
	m_record.clear();
	m_record = m_database.elementsInfoFromQuery(query);
}
