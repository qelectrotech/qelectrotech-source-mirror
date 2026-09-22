/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef CONTACTCROSSREFPROJECTIONSERVICE_H
#define CONTACTCROSSREFPROJECTIONSERVICE_H

#include "contactusage.h"
#include "properties/elementdata.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QUuid>

class QETProject;

struct ContactGroupProjection
{
	int index = -1;
	ElementData::SlaveState type = ElementData::Other;
	ElementData::SlaveType subtype = ElementData::SSimple;
	int contact_count = 1;
	int terminal_count = 1;
	QStringList terminal_labels;
};

struct ContactMasterProjection
{
	QUuid uuid;
	QString label;
	int folio = -1;
	ElementData::MasterType master_type = ElementData::Coil;
	QList<ContactGroupProjection> groups;
	ContactUsage usage;
	ContactUsage capacity;
	int linked_slave_count = 0;
	QStringList validation_messages;
};

struct ContactAssignmentProjection
{
	QUuid master_uuid;
	QUuid slave_uuid;
	QString master_label;
	QString slave_label;
	int master_folio = -1;
	int slave_folio = -1;
	int group_index = -1;
	bool group_index_resolves = false;
	ElementData::SlaveState slave_contact_type = ElementData::Other;
	ElementData::SlaveType slave_contact_subtype = ElementData::SSimple;
	int slave_contact_count = 1;
	ContactGroupProjection group;
	bool duplicate_group_assignment = false;
	QStringList validation_messages;
};

class ContactCrossRefProjectionService
{
	public:
		QList<ContactMasterProjection> masters(QETProject &project) const;
		QList<ContactAssignmentProjection> assignments(QETProject &project) const;
};

#endif // CONTACTCROSSREFPROJECTIONSERVICE_H
