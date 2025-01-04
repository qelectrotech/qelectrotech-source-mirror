/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef FREETERMINALMODEL_H
#define FREETERMINALMODEL_H

#include <QAbstractTableModel>
#include <QPointer>
#include <QStyledItemDelegate>

#include "modelTerminalData.h"
#include "../../qetproject.h"

class RealTerminal;

/**
 * @brief The FreeTerminalModel class
 */
class FreeTerminalModel : public QAbstractTableModel
{
	public:
		enum Column {
			Label = 0,
			XRef = 1,
			Type = 2,
			Function = 3,
			Led = 4,
			Invalid = 99
		};

		static FreeTerminalModel::Column columnTypeForIndex(const QModelIndex &index);
		Q_OBJECT

	public:
		explicit FreeTerminalModel(QETProject *project, QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent) const override;
		int columnCount(const QModelIndex &parent) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
		Qt::ItemFlags flags(const QModelIndex &index) const override;

		void clear();
		QVector<modelRealTerminalData> modifiedModelRealTerminalData() const;
		modelRealTerminalData dataAtRow(int row) const;
		QVector<QSharedPointer<RealTerminal>> realTerminalForIndex(const QModelIndexList &index_list) const;

	private:
		void fillTerminalVector();

	private:
		QPointer<QETProject> m_project;
		QVector<QSharedPointer<RealTerminal>> m_terminal_vector;
		QVector<modelRealTerminalData> m_real_t_data;
		QHash<QSharedPointer<RealTerminal>, QVector<bool>> m_modified_cell;
};

class FreeTerminalModelDelegate : public QStyledItemDelegate
{
	Q_OBJECT

	public:
		FreeTerminalModelDelegate(QObject *parent  = nullptr);

		QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
		void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

#endif // FREETERMINALMODEL_H
