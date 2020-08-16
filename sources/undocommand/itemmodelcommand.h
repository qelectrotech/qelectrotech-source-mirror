/*
	Copyright 2006-2020 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ITEMMODELCOMMAND_H
#define ITEMMODELCOMMAND_H

#include <QUndoCommand>
#include <QModelIndex>
#include <QPointer>

class QAbstractItemModel;

/**
	@brief The ModelIndexCommand class
	Change a data of an index of QAbstractItemModel
*/
class ModelIndexCommand : public QUndoCommand
{
    public:
		ModelIndexCommand(QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr);
		void setData(const QVariant &value, int role = Qt::DisplayRole);

		virtual void redo() override;
		virtual void undo() override;

    private:
        QPointer<QAbstractItemModel> m_model;
		QModelIndex m_index;
		QVariant m_old_value,
				 m_new_value;
		int m_role=0;
};

/**
	@brief The ModelHeaderDataCommand class
	Change the data of a header
*/
class ModelHeaderDataCommand : public QUndoCommand
{
	public:
		ModelHeaderDataCommand(QAbstractItemModel *model, QUndoCommand *parent = nullptr);
		void setData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::DisplayRole);

		virtual void redo() override;
		virtual void undo() override;

	private:
		QPointer<QAbstractItemModel> m_model;
		Qt::Orientation m_orientation;
		QVariant m_old_value,
				 m_new_value;
		int m_section,
			m_role;
};

#endif // ITEMMODELCOMMAND_H
