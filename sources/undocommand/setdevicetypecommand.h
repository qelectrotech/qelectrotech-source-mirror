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
#ifndef SETDEVICETYPECOMMAND_H
#define SETDEVICETYPECOMMAND_H

#include "../devicetype/devicetype.h"

#include <QPointer>
#include <QUndoCommand>

class QETProject;

/**
	@brief The SetDeviceTypeCommand class
	Keeps a device type in a project (QETProject::setDeviceType), in place
	of the one of the same name if any: a type taken from the user's
	library when a device is given it. Undo gives back what was there.
*/
class SetDeviceTypeCommand : public QUndoCommand
{
	public:
		SetDeviceTypeCommand(QETProject *project, const DeviceType &type,
							 QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<QETProject> m_project;
		DeviceType m_new;
		DeviceType m_old;   ///< null when the project had no such type
};

/**
	@brief The EditDeviceTypeCommand class
	A device type of a project made, changed or deleted in Project > Device
	types: the project's type named @p name becomes @p type, a null @p type
	deleting it. Undo gives back what was there.
*/
class EditDeviceTypeCommand : public QUndoCommand
{
	public:
		EditDeviceTypeCommand(QETProject *project, const QString &name,
							  const DeviceType &type, QUndoCommand *parent = nullptr);
		void undo() override;
		void redo() override;

	private:
		void put(const DeviceType &type);

		QPointer<QETProject> m_project;
		QString m_name;
		DeviceType m_new;   ///< null to delete
		DeviceType m_old;   ///< null when the project had no such type
};

#endif // SETDEVICETYPECOMMAND_H
