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
#include "changeelementinformationcommand.h"

#include "../diagram.h"
#include "../qetgraphicsitem/element.h"

#include <QObject>

/**
	@brief ChangeElementInformationCommand::ChangeElementInformationCommand
	Default constructor
	@param elmt : element to change information
	@param old_info : old info of element
	@param new_info : new info of element
	@param parent
*/
ChangeElementInformationCommand::ChangeElementInformationCommand(
		Element *elmt,
		const DiagramContext &old_info,
		const DiagramContext &new_info,
		QUndoCommand *parent) :
	QUndoCommand (parent)
{
	m_map.insert(QPointer<Element>(elmt), qMakePair(old_info, new_info));
	setText(QObject::tr("Modifier les informations de l'élément : %1")
			.arg(elmt -> name()));
}

ChangeElementInformationCommand::ChangeElementInformationCommand(QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext> > map,
																 QUndoCommand *parent) :
	QUndoCommand(parent),
	m_map(map)
{
	setText(QObject::tr("Modifier les informations de plusieurs éléments"));
}

bool ChangeElementInformationCommand::mergeWith(const QUndoCommand *other)
{
	if (id() != other->id())
		return false;

	ChangeElementInformationCommand const *other_undo = static_cast<const ChangeElementInformationCommand*>(other);

		//In case of other undo_undo have the same elements as keys
	if (m_map.size() == other_undo->m_map.size())
	{
		for (auto key : other_undo->m_map.keys()) {
			if (!m_map.keys().contains(key)) {
				return false;
			}
		}

			//Other_undo will be merged with this undo :
			//Replace the new_info values of this m_map
			//by the new_info values of other_undo's m_map
		for (auto key : other_undo->m_map.keys())
		{
			m_map.insert(key,
						 qMakePair(
							 m_map.value(key).first,
							 other_undo->m_map.value(key).second));
		}
		return true;
	}
	else {
		return false;
	}
}

/**
	@brief ChangeElementInformationCommand::undo
*/
void ChangeElementInformationCommand::undo()
{
	for (auto element : m_map.keys()) {
		element->setElementInformations(m_map.value(element).first);
	}
	updateProjectDB();
}

/**
	@brief ChangeElementInformationCommand::redo
*/
void ChangeElementInformationCommand::redo()
{
	for (auto element : m_map.keys()) {
		element->setElementInformations(m_map.value(element).second);
	}
	updateProjectDB();
}

void ChangeElementInformationCommand::updateProjectDB()
{
	auto elmt = m_map.keys().first().data();
	if(elmt && elmt->diagram())
	{
			//need to have a list of element instead of QPointer<Element>
			//for the function elementInfoChange of the database
		QList<Element *> list_;
		for (auto p_elmt : m_map.keys())
			list_ << p_elmt.data();

		elmt->diagram()->project()->dataBase()->elementInfoChanged(list_);
	}
}
