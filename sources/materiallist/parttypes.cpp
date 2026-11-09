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
#include "parttypes.h"

#include "../dataBase/projectdatabase.h"
#include "../devicetype/devicetype.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "../undocommand/setdevicetypecommand.h"

#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QSqlQuery>
#include <QUndoCommand>

#include <algorithm>

namespace PartTypes
{
	namespace
	{
		const QString reference = QStringLiteral("manufacturer_reference");
			/// The materials list column and the element information naming
			/// a device type
		const QString type_key = QStringLiteral("device_type");
	}

	/**
		@return the first of @p part_numbers, joined by + or by a comma
	*/
	QString mainPart(const QString &part_numbers)
	{
		static const QRegularExpression separator(QStringLiteral("[+,]"));
		return part_numbers.section(separator, 0, 0).trimmed();
	}

	/**
		@return what the devices of @p project show of each main part
	*/
	QHash<QString, Usage> usageIn(QETProject *project)
	{
		QHash<QString, Usage> usage;
		if (!project || !project->dataBase()) return usage;
		project->dataBase()->updateDB();
		QSqlQuery query = project->dataBase()->newQuery(
					QStringLiteral("SELECT d.articles, i.device_type FROM qet_device_list d "
								   "LEFT JOIN element_info i ON i.element_uuid = d.head_uuid"));
		while (query.next()) {
			const QString part = mainPart(query.value(0).toString());
			if (part.isEmpty()) continue;
			Usage &entry = usage[part];
			++entry.devices;
			const QString type = query.value(1).toString().trimmed();
			if (!type.isEmpty()) ++entry.types[type];
		}
		return usage;
	}

	/**
		@return the parts of @p data without a device type, the ones the
		project uses most first, then by part number
	*/
	QList<Row> partsWithoutType(const MaterialListData &data, const QHash<QString, Usage> &usage)
	{
		QList<Row> rows;
		for (int i = 0 ; i < data.records.size() ; ++i) {
			const MaterialRecord &record = data.records.at(i);
			const QString part = record.value(reference).trimmed();
			if (part.isEmpty() || !record.value(type_key).trimmed().isEmpty()) continue;
			Row row;
			row.record = i;
			row.part = part;
			row.description = record.value(QStringLiteral("designation")).trimmed();
			if (row.description.isEmpty())
				row.description = record.value(QStringLiteral("description")).trimmed();
			row.manufacturer = record.value(QStringLiteral("manufacturer")).trimmed();
			const auto found = usage.constFind(part);
			if (found != usage.constEnd()) {
				row.used = found->devices;
				int most = 0;
				for (auto it = found->types.constBegin() ; it != found->types.constEnd() ; ++it)
					if (it.value() > most) {
						most = it.value();
						row.suggestion = it.key();
					}
				row.certain = found->types.size() == 1;
			}
			rows << row;
		}
		std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
			return a.used != b.used ? a.used > b.used : a.part < b.part;
		});
		return rows;
	}

	/**
		@brief saveTypes
		Write @p types (record of the list -> device type) into the
		material file. The file gains a device type column if it has none.
	*/
	bool saveTypes(const QString &path, MaterialListData &data, const QHash<int, QString> &types,
				   QString *error)
	{
		if (!data.columns.contains(type_key)) data.columns << type_key;
		for (auto it = types.constBegin() ; it != types.constEnd() ; ++it)
			data.records[it.key()].setValue(type_key, it.value());
		return MaterialList::writeFile(path, data, error);
	}

	/**
		@brief giveToSymbols
		@param part_types : main part -> device type
		@param symbols : how many symbols take a type
		@return the undo command giving the symbols of @p project with
		these main parts and no device type their type, the project taking a copy
		of a type only the user's library has; nullptr when there are none
	*/
	QUndoCommand *giveToSymbols(QETProject *project, const QHash<QString, QString> &part_types,
								int *symbols)
	{
		if (symbols) *symbols = 0;
		if (!project || part_types.isEmpty()) return nullptr;
		if (project->unloadedFolioCount()) project->loadFolios();

		QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> changes;
		QSet<QString> needed;
		for (Diagram *diagram : project->diagrams()) {
			for (Element *element : diagram->elements()) {
				const DiagramContext info = element->elementInformations();
				if (!info.value(type_key).toString().trimmed().isEmpty()) continue;
				const QString type = part_types.value(mainPart(info.value(reference).toString()));
				if (type.isEmpty()) continue;
				DiagramContext changed = info;
				changed.addValue(type_key, type);
				changes.insert(element, qMakePair(info, changed));
				needed.insert(type);
			}
		}
		if (changes.isEmpty()) return nullptr;
		if (symbols) *symbols = int(changes.size());

		auto *undo = new QUndoCommand(QObject::tr("Give the symbols of these parts their device type"));
		for (const QString &type : std::as_const(needed)) {
			if (!project->deviceType(type).isNull()) continue;
			const DeviceType from_library = DeviceTypeLibrary::load(type);
			if (!from_library.isNull()) new SetDeviceTypeCommand(project, from_library, undo);
		}
		new ChangeElementInformationCommand(changes, undo);
		return undo;
	}
}
