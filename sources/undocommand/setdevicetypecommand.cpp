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
#include "setdevicetypecommand.h"

#include "../qetproject.h"

#include <QObject>

SetDeviceTypeCommand::SetDeviceTypeCommand(QETProject *project, const DeviceType &type,
										   QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project(project),
	m_new(type)
{
	if (project) m_old = project->deviceType(type.name);
	//: %1 is a device type's name, such as "CONTR10"
	setText(QObject::tr("Add the device type %1 to the project").arg(type.name));
}

void SetDeviceTypeCommand::undo()
{
	if (!m_project) return;
	if (m_old.isNull())
		m_project->removeDeviceType(m_new.name);
	else
		m_project->setDeviceType(m_old);
}

void SetDeviceTypeCommand::redo()
{
	if (m_project) m_project->setDeviceType(m_new);
}

/**
	@brief EditDeviceTypeCommand::EditDeviceTypeCommand
	@param project : the project whose types change
	@param name : the type's name
	@param type : what the type becomes; null to delete it
	@param parent : parent undo command
*/
EditDeviceTypeCommand::EditDeviceTypeCommand(QETProject *project, const QString &name,
											 const DeviceType &type, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project(project),
	m_name(name),
	m_new(type)
{
	if (project) m_old = project->deviceType(name);
	if (m_new.isNull())
		//: %1 is a device type's name, such as "CONTR10"
		setText(QObject::tr("Delete the device type %1").arg(name));
	else if (m_old.isNull())
		setText(QObject::tr("Add the device type %1 to the project").arg(name));
	else
		setText(QObject::tr("Edit the device type %1").arg(name));
}

void EditDeviceTypeCommand::undo()
{
	put(m_old);
}

void EditDeviceTypeCommand::redo()
{
	put(m_new);
}

/**
	@brief EditDeviceTypeCommand::put
	The project's type of this name becomes @p type; a null one deletes it
*/
void EditDeviceTypeCommand::put(const DeviceType &type)
{
	if (!m_project) return;
	if (type.isNull())
		m_project->removeDeviceType(m_name);
	else
		m_project->setDeviceType(type);
}
