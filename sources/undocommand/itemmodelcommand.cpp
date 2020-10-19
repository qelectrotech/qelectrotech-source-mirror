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
#include "itemmodelcommand.h"

/**
	@brief ModelIndexCommand::ModelIndexCommand
	@param model
	@param index
	@param parent
*/
ModelIndexCommand::ModelIndexCommand(QAbstractItemModel *model, const QModelIndex &index, QUndoCommand *parent):
	QUndoCommand(parent),
	m_model(model),
	m_index(index)
{}

/**
	@brief ModelIndexCommand::setData
	@param value
	@param role
*/
void ModelIndexCommand::setData(const QVariant &value, int role)
{
	m_new_value = value;
	if (m_model) {
		m_old_value = m_model->data(m_index, role);
	} else {
		m_old_value = value;
	}

	m_role = role;
}

/**
	@brief ModelIndexCommand::redo
	Reimplemented from QUndoCommand
*/
void ModelIndexCommand::redo()
{
	if (m_model && m_index.isValid()) {
		m_model->setData(m_index, m_new_value, m_role);
	}
}

/**
	@brief ModelIndexCommand::undo
	Reimplemented from QUndoCommand
*/
void ModelIndexCommand::undo()
{
	if (m_model && m_index.isValid()) {
		m_model->setData(m_index, m_old_value, m_role);
	}
}

/**
	@brief ModelHeaderDataCommand::ModelHeaderDataCommand
	@param model
	@param parent
*/
ModelHeaderDataCommand::ModelHeaderDataCommand(QAbstractItemModel *model, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_model(model)
{}

/**
	@brief ModelHeaderDataCommand::setData
	See QAbstractItemModel::setHeaderData
	@param section
	@param orientation
	@param value
	@param role
*/
void ModelHeaderDataCommand::setData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	m_section = section;
	m_orientation = orientation;
	m_new_value = value;
	if (m_model) {
		m_old_value = m_model->headerData(section, orientation, role);
	} else {
		m_old_value = m_new_value;
	}
	m_role = role;
}

/**
	@brief ModelHeaderDataCommand::redo
	Reimplemented from QUndoCommand
*/
void ModelHeaderDataCommand::redo()
{
	if (m_model) {
		m_model->setHeaderData(m_section, m_orientation, m_new_value, m_role);
	}
}

/**
	@brief ModelHeaderDataCommand::undo
	Reimplemented from QUndoCommand
*/
void ModelHeaderDataCommand::undo()
{
	if (m_model) {
		m_model->setHeaderData(m_section, m_orientation, m_old_value, m_role);
	}
}
