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
#include "devicetypelist.h"

#include "../dataBase/projectdatabase.h"
#include "../qetproject.h"

#include <QFile>
#include <QObject>
#include <QSqlQuery>
#include <QStringList>
#include <algorithm>

namespace DeviceTypeList
{
	namespace
	{
			/// A CSV field: quoted when it holds the separator, a quote
			/// or a line break
		QString field(const QString &value)
		{
			if (!value.contains(QLatin1Char(';')) && !value.contains(QLatin1Char('"'))
				&& !value.contains(QLatin1Char('\n')))
				return value;
			QString quoted = value;
			quoted.replace(QLatin1String("\""), QLatin1String("\"\""));
			return QLatin1Char('"') + quoted + QLatin1Char('"');
		}

		QString row(const QStringList &fields)
		{
			QStringList quoted;
			for (const QString &value : fields) quoted << field(value);
			return quoted.join(QLatin1Char(';')) + QLatin1Char('\n');
		}
	}

	QHash<QString, int> usedBy(QETProject *project)
	{
		QHash<QString, int> used;
		if (!project || !project->dataBase()) return used;
		QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
			"SELECT device_type, COUNT(*) FROM element_info "
			"WHERE device_type IS NOT NULL AND device_type != '' GROUP BY device_type"));
		while (query.next())
			used.insert(query.value(0).toString(), query.value(1).toInt());
		return used;
	}

	QString csv(QList<DeviceType> types, Kind kind, const QHash<QString, int> *used,
				const QString &lang)
	{
		std::sort(types.begin(), types.end(), [](const DeviceType &a, const DeviceType &b) {
			return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
		});
		QString text;
		if (kind == Kind::Brief) {
			QStringList head{QStringLiteral("device_type"), QStringLiteral("description"),
							 QStringLiteral("parts")};
			if (used) head << QStringLiteral("symbols");
			text += row(head);
			for (const DeviceType &type : types) {
				QStringList fields{type.name, type.text(lang), QString::number(type.parts.size())};
				if (used) fields << QString::number(used->value(type.name));
				text += row(fields);
			}
			return text;
		}

		text += row({QStringLiteral("device_type"), QStringLiteral("description"),
					 QStringLiteral("part"), QStringLiteral("role"), QStringLiteral("kind"),
					 QStringLiteral("state"), QStringLiteral("symbol"), QStringLiteral("pins")});
		for (const DeviceType &type : types) {
			if (type.parts.isEmpty())
				text += row({type.name, type.text(lang), QString(), QString(), QString(),
							 QString(), QString(), QString()});
			for (int i = 0 ; i < type.parts.size() ; ++i) {
				const DeviceType::Part &part = type.parts.at(i);
				const QString number = QString::number(i + 1);
					//A part not drawn yet still has its row
				QList<DeviceType::Symbol> ways = part.symbols;
				if (ways.isEmpty()) ways << DeviceType::Symbol();
				for (const DeviceType::Symbol &way : ways) {
					const QString state = part.state.isEmpty() ? way.state : part.state;
					text += row({type.name, type.text(lang), number, part.role, part.kind, state,
								 way.location, way.pins.join(QLatin1Char(' '))});
				}
			}
		}
		return text;
	}

	bool write(const QString &path, const QString &csv, QString *error)
	{
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			if (error) *error = file.errorString();
			return false;
		}
		const QByteArray bytes = csv.toUtf8();
		if (file.write(bytes) != bytes.size()) {
			if (error) *error = file.errorString();
			return false;
		}
		return true;
	}
}
