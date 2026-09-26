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
#ifndef DEVICEINFORMATION_H
#define DEVICEINFORMATION_H

#include "../diagramcontext.h"

#include <QString>

/**
 * Device properties backed by ElementData::m_informations, not a second store.
 * Missing properties remain empty. Quantity and ratings retain the existing
 * free-text representation (including decimal quantities and units).
 */
struct DeviceInformation
{
	QString m_device_tag;
	QString m_manufacturer;
	QString m_part_number;
	QString m_model;
	QString m_description;
	QString m_category;
	QString m_quantity;
	QString m_voltage_rating;
	QString m_current_rating;
	QString m_notes;

	static DeviceInformation fromContext(const DiagramContext &context);
	DiagramContext toContext(DiagramContext context = DiagramContext()) const;
};

#endif // DEVICEINFORMATION_H
