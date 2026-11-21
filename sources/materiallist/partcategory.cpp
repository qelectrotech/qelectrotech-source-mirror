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
#include "partcategory.h"

#include <QCoreApplication>
#include <QList>
#include <QPair>
#include <QRegularExpression>

namespace
{
	struct Category
	{
		const char *key;
		const char *english;
	};

		/// The names are listed here to be translated
	const QList<Category> &categories()
	{
		static const QList<Category> list = {
			{"power_cable", QT_TRANSLATE_NOOP("PartCategory", "Power cable")},
			{"instrument_cable", QT_TRANSLATE_NOOP("PartCategory", "Instrument cable")},
			{"control_cable", QT_TRANSLATE_NOOP("PartCategory", "Control cable")},
			{"data_cable", QT_TRANSLATE_NOOP("PartCategory", "Data cable")},
			{"contactor", QT_TRANSLATE_NOOP("PartCategory", "Contactor")},
			{"relay", QT_TRANSLATE_NOOP("PartCategory", "Relay")},
			{"power_supply", QT_TRANSLATE_NOOP("PartCategory", "Power supply")},
			{"fuse", QT_TRANSLATE_NOOP("PartCategory", "Fuse")},
			{"overload", QT_TRANSLATE_NOOP("PartCategory", "Overload")},
			{"circuit_breaker", QT_TRANSLATE_NOOP("PartCategory", "Circuit breaker")},
			{"motor_starter", QT_TRANSLATE_NOOP("PartCategory", "Motor starter")},
			{"terminal", QT_TRANSLATE_NOOP("PartCategory", "Terminal")},
			{"connector", QT_TRANSLATE_NOOP("PartCategory", "Connector")},
			{"plc", QT_TRANSLATE_NOOP("PartCategory", "PLC")},
			{"sensor", QT_TRANSLATE_NOOP("PartCategory", "Sensor")},
			{"switch", QT_TRANSLATE_NOOP("PartCategory", "Switch")},
			{"pushbutton", QT_TRANSLATE_NOOP("PartCategory", "Push button")},
			{"pilot_light", QT_TRANSLATE_NOOP("PartCategory", "Pilot light")},
			{"motor", QT_TRANSLATE_NOOP("PartCategory", "Motor")},
			{"transformer", QT_TRANSLATE_NOOP("PartCategory", "Transformers")},
			{"enclosure", QT_TRANSLATE_NOOP("PartCategory", "Enclosure")},
			{"drive", QT_TRANSLATE_NOOP("PartCategory", "Drive")},
			{"valve", QT_TRANSLATE_NOOP("PartCategory", "Valve")},
			{"busbar", QT_TRANSLATE_NOOP("PartCategory", "Busbar and link")},
			{"mounting", QT_TRANSLATE_NOOP("PartCategory", "Rail and trunking")},
			{"marking", QT_TRANSLATE_NOOP("PartCategory", "Marking")},
			{"accessory", QT_TRANSLATE_NOOP("PartCategory", "Accessory")},
			{"other", QT_TRANSLATE_NOOP("PartCategory", "Other")},
		};
		return list;
	}

		/// The words which tell a category, the first rule which matches wins
	struct Rule
	{
		const char *key;
		QRegularExpression words;
	};

	QRegularExpression words(const char *pattern)
	{
		return QRegularExpression(QString::fromLatin1(pattern),
								  QRegularExpression::CaseInsensitiveOption);
	}

	const QList<Rule> &rules()
	{
		static const QList<Rule> list = {
			// what a part is for before what it fits: "SLEEVE FOR CABLE" is a marking
			{"marking", words("MARKER|MARKING|SLEEVE|LABEL")},
			{"accessory", words("AUXILIARY|INTERLOCK|ACCESSORY|END\\s*PLATE|END\\s*COVER|CONNECTING\\s*MODULE|CONNECTION\\s*LINK|DEVICE\\s*CARRIER")},
			{"data_cable", words("CABLE.*(DATA|ETHERNET|CAT\\s?\\d|I/O|INTERCONNECT|USB|RS\\s?232|RS\\s?485|PROFIBUS|DEVICENET)|(DATA|ETHERNET|CAT\\s?\\d|INTERCONNECT|USB|RS\\s?232|RS\\s?485|PROFIBUS|DEVICENET).*CABLE")},
			{"instrument_cable", words("CABLE.*(INSTR|SCREEN|SHIELD)|(INSTR|SCREEN|SHIELD).*CABLE")},
			{"control_cable", words("CABLE.*CONTROL|CONTROL.*CABLE")},
			{"power_cable", words("CABLE")},
			{"drive", words("VARIABLE\\s*SPEED|\\bVSD\\b|SOFT\\s*STARTER|INVERTER|\\bDRIVE\\b")},
			{"motor_starter", words("MPCB|MOTOR\\s*PROTECT")},
			{"contactor", words("CONTACTOR|CONTR(?!OL)")},
			{"overload", words("OVERLOAD|OLOAD")},
			{"power_supply", words("PSU|POWER\\s*SUPPLY")},
			{"relay", words("RELAY|TIMER")},
			{"fuse", words("FUSE")},
			{"circuit_breaker", words("BREAKER|MCB|CB-")},
			{"busbar", words("BUSBAR|EARTH\\s*BAR|NEUTRAL\\s*BAR|LINK\\s*BAR|JUMPER")},
			{"terminal", words("TERMINAL|TB-")},
			{"connector", words("PLUG|SOCKET|CONNECTOR")},
			{"plc", words("PLC|CPU|\\bI/O\\b|(ANALOG|DC|AC)\\s*(INPUT|OUTPUT)")},
			{"sensor", words("SENSOR|SWITCH\\s*LEVEL|LEVEL\\s*SWITCH|PROXIM|PHOTO")},
			{"pushbutton", words("PUSH\\s*BUTTON|PUSHBUTTON|\\bPB\\b")},
			{"pilot_light", words("PILOT\\s*LIGHT|INDICATOR\\s*LIGHT|BEACON|LAMP")},
			{"switch", words("SWITCH|ISOLATOR")},
			{"valve", words("VALVE|SOLENOID")},
			{"motor", words("MOTOR")},
			{"transformer", words("TRANSFORMER")},
			{"enclosure", words("ENCLOSURE|CABINET|SIDE\\s*PANEL|PLAN\\s*POCKET")},
			{"mounting", words("TRUNKING|DIN\\s*RAIL|\\bDUCT\\b|\\bRAIL\\b")},
		};
		return list;
	}
}

QStringList PartCategory::keys()
{
	QStringList list;
	for (const Category &category : categories()) list << QString::fromLatin1(category.key);
	return list;
}

QString PartCategory::translated(const QString &key)
{
	for (const Category &category : categories())
		if (key == QLatin1String(category.key))
			return QCoreApplication::translate("PartCategory", category.english);
	return key;
}

QString PartCategory::keyOf(const QString &text)
{
	const QString wanted = text.trimmed();
	if (wanted.isEmpty()) return QString();
	for (const Category &category : categories()) {
		if (wanted.compare(QLatin1String(category.key), Qt::CaseInsensitive) == 0
			|| wanted.compare(QLatin1String(category.english), Qt::CaseInsensitive) == 0
			|| wanted.compare(translated(QString::fromLatin1(category.key)), Qt::CaseInsensitive) == 0)
			return QString::fromLatin1(category.key);
	}
	return QString();
}

QString PartCategory::categoryOf(const MaterialRecord &record)
{
	const QString own = keyOf(record.value(QStringLiteral("category")));
	if (!own.isEmpty()) return own;
	const QString text = record.value(QStringLiteral("device_type")) + QLatin1Char(' ')
			+ record.value(QStringLiteral("description")) + QLatin1Char(' ')
			+ record.value(QStringLiteral("designation"));
	for (const Rule &rule : rules())
		if (rule.words.match(text).hasMatch()) return QString::fromLatin1(rule.key);
	return QStringLiteral("other");
}
