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
	const QString branches_attribute = QStringLiteral("branches");
	const QString angled_value = QStringLiteral("angled");

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
		application_cache.angled_branches = settings.value(angled_branches_key, false).toBool();
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
	qsettings.setValue(angled_branches_key, settings.angled_branches);
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
	settings.angled_branches = rules.attribute(branches_attribute) == angled_value;
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
	if (settings.angled_branches) {
		rules.setAttribute(branches_attribute, angled_value);
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
	@brief WiringRules::angledBranches
	@return true if, where wires branch, each one's corner is cut diagonally
	instead of a junction dot being drawn (discussion #1158). Display only:
	no wire is changed.
*/
bool WiringRules::angledBranches(const Settings &settings, bool master_enabled)
{
	return master_enabled && settings.angled_branches;
}

/**
	@brief WiringRules::angledCorners
	Two wires leaving one line each turn at the same point, where a dot is
	drawn. Cutting each one's corner diagonally instead draws a "Y": the
	line splits, and each wire visibly goes its own way, so the drawing
	shows which wire runs where.
	@param wire : the points of a wire, corners included
	@param corners : the corners of \a wire to cut; points that are not
	a corner of \a wire are ignored
	@param size : how far back along each side the cut starts, never more
	than half of either side, so two cuts on one side cannot cross
	@return \a wire with each listed corner replaced by the two ends of
	its cut
*/
QVector<QPointF> WiringRules::angledCorners(const QVector<QPointF> &wire,
											const QList<QPointF> &corners,
											qreal size)
{
	if (wire.size() < 3 || corners.isEmpty() || size <= 0) {
		return wire;
	}

	auto length = [](const QPointF &a, const QPointF &b) {
		return qAbs(a.x() - b.x()) + qAbs(a.y() - b.y());
	};
	auto towards = [](const QPointF &from, const QPointF &to, qreal distance) {
		const QPointF d = to - from;
		const qreal l = qAbs(d.x()) + qAbs(d.y());
		return l > 0 ? from + d * (distance / l) : from;
	};

	QVector<QPointF> result;
	result << wire.first();
	for (int i = 1 ; i < wire.size() - 1 ; ++i)
	{
		const QPointF &before = wire.at(i - 1);
		const QPointF &corner = wire.at(i);
		const QPointF &after = wire.at(i + 1);
		bool listed = false;
		for (const QPointF &c : corners) {
			if (length(c, corner) < 0.01) {
				listed = true;
				break;
			}
		}
		const qreal cut = qMin(size, qMin(length(before, corner), length(corner, after)) / 2);
		if (!listed || cut <= 0) {
			result << corner;
			continue;
		}
		result << towards(corner, before, cut) << towards(corner, after, cut);
	}
	result << wire.last();
	return result;
}
