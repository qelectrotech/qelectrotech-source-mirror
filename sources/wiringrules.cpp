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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "wiringrules.h"

#include <QDomDocument>
#include <QDomElement>
#include <QSettings>

namespace {
	const QString element_name = QStringLiteral("wiring_rules");
	const QString max_wires_attribute = QStringLiteral("max_wires_per_terminal");
	const QString report_attribute = QStringLiteral("one_wire_per_report");

		//The master switch is read on every wire drawn, so it is read from
		//the settings once and kept; setMasterEnabled() keeps both in step.
	int master_cache = -1;
		//Same for the application's rules
	bool application_cached = false;
	WiringRules::Settings application_cache;
}

/**
	@brief WiringRules::masterEnabled
	@return false when the user turned every rule of this feature off for
	every project (Settings > General). On by default: a project still has
	to turn a rule on before anything changes.
*/
bool WiringRules::masterEnabled()
{
	if (master_cache < 0) {
		master_cache = QSettings().value(settings_key, true).toBool() ? 1 : 0;
	}
	return master_cache == 1;
}

/**
	@brief WiringRules::setMasterEnabled
	Save the master switch.
	@param enabled
*/
void WiringRules::setMasterEnabled(bool enabled)
{
	QSettings().setValue(settings_key, enabled);
	master_cache = enabled ? 1 : 0;
}

/**
	@brief WiringRules::applicationSettings
	@return the rules set for every project in Settings > General; a
	project that sets its own (Settings::own) uses those instead.
*/
WiringRules::Settings WiringRules::applicationSettings()
{
	if (!application_cached) {
		const QSettings settings;
		application_cache = Settings();
		application_cache.max_wires = qMax(0, settings.value(max_wires_key, 0).toInt());
		application_cache.one_wire_per_report = settings.value(one_wire_per_report_key, false).toBool();
		application_cached = true;
	}
	return application_cache;
}

/**
	@brief WiringRules::setApplicationSettings
	Save the rules set for every project.
	@param settings
*/
void WiringRules::setApplicationSettings(const Settings &settings)
{
	QSettings qsettings;
	qsettings.setValue(max_wires_key, qMax(0, settings.max_wires));
	qsettings.setValue(one_wire_per_report_key, settings.one_wire_per_report);
	application_cache = settings;
	application_cache.own = false;
	application_cached = true;
}

/**
	@brief WiringRules::effective
	@return the rules that apply to a project: its own when it sets them,
	otherwise the application's.
*/
WiringRules::Settings WiringRules::effective(const Settings &project, const Settings &application)
{
	Settings result = project.own ? project : application;
	result.own = project.own;
	return result;
}

/**
	@brief WiringRules::fromXml
	@param project_root : the root element of a project
	@return the rules stored in the <wiring_rules> child of \a project_root,
	which the project then uses instead of the application's (Settings::own);
	the default, which follows the application, when there is none.
*/
WiringRules::Settings WiringRules::fromXml(const QDomElement &project_root)
{
	Settings settings;
	const QDomElement rules = project_root.firstChildElement(element_name);
	if (rules.isNull()) {
		return settings;
	}
	settings.own = true;
	settings.max_wires = qMax(0, rules.attribute(max_wires_attribute, QStringLiteral("0")).toInt());
	settings.one_wire_per_report = rules.attribute(report_attribute) == QLatin1String("true");
	return settings;
}

/**
	@brief WiringRules::toXml
	Write \a settings as a <wiring_rules> child of \a project_root, only
	when the project sets its own rules: a project that follows the
	application's saves exactly as before.
*/
void WiringRules::toXml(const Settings &settings, QDomElement &project_root)
{
	if (!settings.own) {
		return;
	}
	QDomElement rules = project_root.ownerDocument().createElement(element_name);
	if (settings.max_wires > 0) {
		rules.setAttribute(max_wires_attribute, settings.max_wires);
	}
	if (settings.one_wire_per_report) {
		rules.setAttribute(report_attribute, QStringLiteral("true"));
	}
	project_root.appendChild(rules);
}

/**
	@brief WiringRules::limit
	@param settings : the project's rules
	@param master_enabled : the master switch, masterEnabled()
	@param is_report : the terminal belongs to a folio report
	@return the most wires the terminal may take, 0 for no limit.
*/
int WiringRules::limit(const Settings &settings, bool master_enabled, bool is_report)
{
	if (!master_enabled) {
		return 0;
	}
	if (is_report && settings.one_wire_per_report) {
		return 1;
	}
	return settings.max_wires;
}

/**
	@brief WiringRules::hasRoom
	@param limit : WiringRules::limit()
	@param wires : the wires the terminal already has
	@return true if one more wire may be connected
*/
bool WiringRules::hasRoom(int limit, int wires)
{
	return limit <= 0 || wires < limit;
}

/**
	@brief WiringRules::chainsWires
	@return true if QElectroTech's own tools that wire several terminals at
	once must wire them one after another (a chain) rather than all to one
	of them (a star): a star gives that one terminal a wire per other
	terminal, which the project's limit forbids.
*/
bool WiringRules::chainsWires(const Settings &settings, bool master_enabled)
{
	return master_enabled && settings.max_wires > 0;
}

/**
	@brief WiringRules::chainOrder
	The order in which to wire terminals at \a points one after another:
	from the top left one (smallest x, then smallest y, the same terminal
	the star used as its hub), each time to the nearest terminal not yet
	wired. Distance is along the grid (|dx| + |dy|), since wires run
	horizontally and vertically; a tie goes to the earlier point.
	@return indexes into \a points, each once
*/
QList<int> WiringRules::chainOrder(const QList<QPointF> &points)
{
	QList<int> order;
	if (points.isEmpty()) {
		return order;
	}

	int current = 0;
	for (int i = 1 ; i < points.size() ; ++i) {
		const QPointF &p = points.at(i);
		const QPointF &c = points.at(current);
		if (p.x() < c.x() || (p.x() == c.x() && p.y() < c.y())) {
			current = i;
		}
	}

	QList<bool> done(points.size(), false);
	order << current;
	done[current] = true;
	while (order.size() < points.size())
	{
		int nearest = -1;
		qreal nearest_distance = 0;
		for (int i = 0 ; i < points.size() ; ++i) {
			if (done.at(i)) {
				continue;
			}
			const QPointF d = points.at(i) - points.at(current);
			const qreal distance = qAbs(d.x()) + qAbs(d.y());
			if (nearest < 0 || distance < nearest_distance) {
				nearest = i;
				nearest_distance = distance;
			}
		}
		order << nearest;
		done[nearest] = true;
		current = nearest;
	}
	return order;
}

/**
	@brief WiringRules::turnsRuleOn
	@param before, after : the rules in force before and after a change
	@return true if the change turns on a rule that was off: QElectroTech
	then warns that the rules are experimental (review of #1272).
*/
bool WiringRules::turnsRuleOn(const Settings &before, const Settings &after)
{
	return (after.max_wires > 0 && before.max_wires <= 0)
			|| (after.one_wire_per_report && !before.one_wire_per_report);
}
