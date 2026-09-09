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
#include "devicebomexport.h"

#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QSaveFile>
#include <QStringList>
#include <algorithm>

namespace {
QByteArray csvRecord(const QStringList &values)
{
	QStringList escaped;
	for (QString value : values) {
		value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
		escaped.append(QLatin1Char('"') + value + QLatin1Char('"'));
	}
	return (escaped.join(QLatin1Char(',')) + QStringLiteral("\r\n")).toUtf8();
}
}

QList<DeviceBomExport::Row> DeviceBomExport::collect(const QETProject &project)
{
	QList<Row> rows;
	int page = 0;
	for (Diagram *diagram : project.diagrams())
	{
		const QString page_position = QString::number(++page);
		const QString page_label = diagram->border_and_titleblock.finalfolio().isEmpty()
				? page_position
				: diagram->border_and_titleblock.finalfolio();
		auto elements = diagram->elements();
		// Scene stacking order is not stable across save/load. UUID breaks tag ties.
		std::sort(elements.begin(), elements.end(), [](Element *a, Element *b) {
			const int compare = QString::compare(a->actualLabel(), b->actualLabel());
			return compare ? compare < 0 : a->uuid().toString() < b->uuid().toString();
		});
		for (Element *element : elements)
		{
			const auto type = element->elementData().m_type;
			// Slaves are representations of a master, not additional devices.
			// Terminal-specific inventory remains outside this MVP.
			if (type != ElementData::Simple && type != ElementData::Master) {
				continue;
			}
			const auto context = element->elementInformations();
			if (context.value(QStringLiteral("exclude_from_bom")).toBool()) {
				continue;
			}
			auto device = DeviceInformation::fromContext(context);
			device.m_device_tag = element->actualLabel();
			rows.append({device, page_label});
		}
	}
	return rows;
}

QByteArray DeviceBomExport::toCsv(const QList<Row> &rows)
{
	// UTF-8 BOM lets desktop spreadsheet applications detect Turkish text too.
	QByteArray csv("\xEF\xBB\xBF");
	csv += csvRecord({QStringLiteral("No"), QStringLiteral("Device tag"),
		QStringLiteral("Manufacturer"), QStringLiteral("Part number"),
		QStringLiteral("Model"), QStringLiteral("Description"),
		QStringLiteral("Category"), QStringLiteral("Quantity"),
		QStringLiteral("Page"), QStringLiteral("Notes")});
	int number = 0;
	for (const auto &row : rows)
	{
		const auto &device = row.m_device;
		csv += csvRecord({QString::number(++number), device.m_device_tag,
			device.m_manufacturer, device.m_part_number, device.m_model,
			device.m_description, device.m_category, device.m_quantity,
			row.m_page, device.m_notes});
	}
	return csv;
}

bool DeviceBomExport::writeCsv(const QETProject &project, const QString &file_path,
							  QString *error)
{
	if (error) {
		error->clear();
	}
	QSaveFile file(file_path);
	if (!file.open(QIODevice::WriteOnly)) {
		if (error) *error = file.errorString();
		return false;
	}
	const auto csv = toCsv(collect(project));
	if (file.write(csv) != csv.size()) {
		if (error) *error = file.errorString();
		file.cancelWriting();
		return false;
	}
	if (!file.commit()) {
		if (error) *error = file.errorString();
		return false;
	}
	return true;
}
