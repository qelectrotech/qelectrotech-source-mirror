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
#ifndef PARTTYPES_H
#define PARTTYPES_H

#include "materiallist.h"

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>

class QETProject;
class QUndoCommand;

/**
	@brief The PartTypes namespace
	The parts of the material list which do not say their device type, and
	what a project's own devices show of them. A part's device type is what
	gives a symbol that part's terminals, so each needs it said once.

	The main part of a device is its first part number: "A+B" is A with the
	accessory B, whose type is not the device's.
*/
namespace PartTypes
{
	struct Usage
	{
		int devices = 0;                ///< devices with this main part
		QMap<QString, int> types;       ///< their device types, how many each
	};

	struct Row
	{
		int record = -1;                ///< its place in the material list
		QString part;
		QString description;
		QString manufacturer;
		int used = 0;
		QString suggestion;             ///< the type most devices give it
		bool certain = false;           ///< no device gives it another
	};

	QString mainPart(const QString &part_numbers);
	QHash<QString, Usage> usageIn(QETProject *project);
	QList<Row> partsWithoutType(const MaterialListData &data, const QHash<QString, Usage> &usage);
	bool saveTypes(const QString &path, MaterialListData &data, const QHash<int, QString> &types,
				   QString *error = nullptr);
	QUndoCommand *giveToSymbols(QETProject *project, const QHash<QString, QString> &part_types,
								int *symbols = nullptr);
}

#endif // PARTTYPES_H
