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
#ifndef PLCIOPROJECTIONSERVICE_H
#define PLCIOPROJECTIONSERVICE_H

#include "properties/elementdata.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QUuid>

class QETProject;

struct PlcIoProjection
{
	enum Direction {
		Input,
		Output
	};

	QUuid master_uuid;
	QString master_label;
	int folio = -1;
	int io_index = -1;
	ElementData::PlcIOType type = ElementData::EntreeDigitale;
	Direction direction = Input;
	QString address;
	QString function;
	QString comment;
	int terminal_count = 0;
	QStringList terminal_labels;
	QUuid linked_slave_uuid;
	QString linked_slave_label;
	int linked_slave_folio = -1;
	bool unlinked = false;
	bool duplicate_group_index = false;
	bool out_of_range_group_index = false;
	bool empty_address = false;
	bool terminal_label_count_mismatch = false;
	QStringList warnings;
};

class PlcIoProjectionService
{
	public:
		QList<PlcIoProjection> channels(QETProject &project) const;
};

#endif // PLCIOPROJECTIONSERVICE_H
