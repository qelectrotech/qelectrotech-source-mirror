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
#ifndef WIRINGRULES_H
#define WIRINGRULES_H

#include <QList>
#include <QPointF>

class QDomElement;

/**
	@brief The WiringRules namespace
	How many wires a terminal may take (discussion #1158). A real terminal
	takes one or two wires, sometimes four with double ferrules; a folio
	report is a virtual point that should carry one. When a project sets a
	limit, a new wire that would go past it is refused. Wires already in
	the drawing are never touched.

	The rules are set once for the application (Settings > General,
	applicationSettings()) and apply to every project; a project can set
	its own instead (Settings::own), saved in the project so it travels
	with the drawing. Every rule is off by default, and the application
	has a master switch (masterEnabled()) that turns all of them off in
	every project at once. Kept free of any QGraphicsItem so the rules can
	be tested on their own.
*/
namespace WiringRules
{
		///The QSettings key of the master switch
	constexpr const char *settings_key = "diagrameditor/wiring_rules_enabled";
		///The QSettings keys of the application's rules
	constexpr const char *max_wires_key = "diagrameditor/wiring_rules_max_wires";
	constexpr const char *one_wire_per_report_key = "diagrameditor/wiring_rules_one_wire_per_report";

		///A set of rules: the application's, or a project's. The default is
		///"no rule".
	struct Settings
	{
			///For a project: it uses these values rather than the
			///application's. Ignored in the application's own settings.
		bool own = false;
			///Most wires a terminal may take, 0 for no limit
		int max_wires = 0;
			///A folio report takes one wire only
		bool one_wire_per_report = false;

		bool isDefault() const { return *this == Settings(); }
		bool operator==(const Settings &other) const {
			return own == other.own
					&& max_wires == other.max_wires
					&& one_wire_per_report == other.one_wire_per_report;
		}
		bool operator!=(const Settings &other) const { return !(*this == other); }
	};

	bool masterEnabled();
	void setMasterEnabled(bool enabled);

	Settings applicationSettings();
	void setApplicationSettings(const Settings &settings);
	Settings effective(const Settings &project, const Settings &application);
	bool turnsRuleOn(const Settings &before, const Settings &after);

	Settings fromXml(const QDomElement &project_root);
	void toXml(const Settings &settings, QDomElement &project_root);

	int limit(const Settings &settings, bool master_enabled, bool is_report);
	bool hasRoom(int limit, int wires);

	bool chainsWires(const Settings &settings, bool master_enabled);
	QList<int> chainOrder(const QList<QPointF> &points);
}

#endif // WIRINGRULES_H
