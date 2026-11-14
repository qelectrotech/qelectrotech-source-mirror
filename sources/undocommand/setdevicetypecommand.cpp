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
