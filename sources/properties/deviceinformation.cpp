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
#include "deviceinformation.h"

#include "../qetinformation.h"

namespace {
struct DeviceField
{
	QString m_key;
	QString DeviceInformation::*m_member;
};

const DeviceField fields[] = {
	{QETInformation::ELMT_LABEL, &DeviceInformation::m_device_tag},
	{QETInformation::ELMT_MANUFACTURER, &DeviceInformation::m_manufacturer},
	{QETInformation::ELMT_MANUFACTURER_REF, &DeviceInformation::m_part_number},
	{QETInformation::ELMT_MODEL, &DeviceInformation::m_model},
	{QETInformation::ELMT_DESCRIPTION, &DeviceInformation::m_description},
	{QETInformation::ELMT_CATEGORY, &DeviceInformation::m_category},
	{QETInformation::ELMT_QUANTITY, &DeviceInformation::m_quantity},
	{QETInformation::ELMT_VOLTAGE_RATING, &DeviceInformation::m_voltage_rating},
	{QETInformation::ELMT_CURRENT_RATING, &DeviceInformation::m_current_rating},
	{QETInformation::ELMT_NOTES, &DeviceInformation::m_notes}
};
}

DeviceInformation DeviceInformation::fromContext(const DiagramContext &context)
{
	DeviceInformation info;
	for (const auto &field : fields) {
		info.*(field.m_member) = context.value(field.m_key).toString();
	}
	return info;
}

DiagramContext DeviceInformation::toContext(DiagramContext context) const
{
	for (const auto &field : fields)
	{
		const auto &value = this->*(field.m_member);
		if (value.isEmpty()) {
			context.remove(field.m_key);
		} else {
			const bool show = !context.contains(field.m_key) || context.keyMustShow(field.m_key);
			context.addValue(field.m_key, value, show);
		}
	}
	return context;
}
