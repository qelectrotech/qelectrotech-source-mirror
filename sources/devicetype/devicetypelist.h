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
#ifndef DEVICETYPELIST_H
#define DEVICETYPELIST_H

#include "devicetype.h"

#include <QHash>
#include <QList>
#include <QString>

class QETProject;

/**
	@brief The DeviceTypeList namespace
	The device types as a list to read or to give to someone: the brief
	list names each type and says what it is, the full list gives every
	part of each type and the terminals of each of its symbols. Written
	as CSV (separated by ';'), as the other lists of QElectroTech.
*/
namespace DeviceTypeList
{
	enum class Kind {
		Brief,  ///< one row per type: name, description, parts, symbols using it
		Full    ///< one row per way of drawing each part of each type
	};

		/// How many symbols of @p project name each device type
	QHash<QString, int> usedBy(QETProject *project);
		/// The list of @p types, sorted by name, each described in @p lang
		/// (DeviceType::text()); with @p used, a column with the number of
		/// symbols using each type
	QString csv(QList<DeviceType> types, Kind kind,
				const QHash<QString, int> *used = nullptr,
				const QString &lang = QStringLiteral("en"));
		/// The list written to the file @p path; false and @p error when
		/// it cannot be written
	bool write(const QString &path, const QString &csv, QString *error = nullptr);
}

#endif // DEVICETYPELIST_H
