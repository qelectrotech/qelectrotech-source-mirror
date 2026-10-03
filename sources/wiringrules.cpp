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
	@brief WiringRules::fromXml
	@param project_root : the root element of a project
	@return the rules stored in the <wiring_rules> child of \a project_root,
	the default (no rule) when there is none.
*/
WiringRules::Settings WiringRules::fromXml(const QDomElement &project_root)
{
	Settings settings;
	const QDomElement rules = project_root.firstChildElement(element_name);
	if (rules.isNull()) {
		return settings;
	}
	settings.max_wires = qMax(0, rules.attribute(max_wires_attribute, QStringLiteral("0")).toInt());
	settings.one_wire_per_report = rules.attribute(report_attribute) == QLatin1String("true");
	return settings;
}

/**
	@brief WiringRules::toXml
	Write \a settings as a <wiring_rules> child of \a project_root. Nothing
	is written when no rule is on, so a project that never used them saves
	exactly as before.
*/
void WiringRules::toXml(const Settings &settings, QDomElement &project_root)
{
	if (settings.isDefault()) {
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
	@param symbol_limit : the limit the symbol gives this terminal
	(TerminalData::m_max_wires): -1 to follow the project, 0 for none. It
	applies only when the project sets a limit, so a symbol's own value
	never refuses a wire in a project that did not ask for any.
	@return the most wires the terminal may take, 0 for no limit.
*/
int WiringRules::limit(const Settings &settings, bool master_enabled, bool is_report,
					   int symbol_limit)
{
	if (!master_enabled) {
		return 0;
	}
	if (is_report && settings.one_wire_per_report) {
		return 1;
	}
	if (settings.max_wires <= 0 || symbol_limit < 0) {
		return settings.max_wires;
	}
	return symbol_limit;
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
	@brief WiringRules::isReportType
	@param element_type : an element type as the project database stores it
	(ElementData::typeToString())
	@return true for a folio report, next or previous
*/
bool WiringRules::isReportType(const QString &element_type)
{
	return element_type == QLatin1String("next_report")
			|| element_type == QLatin1String("previous_report");
}
