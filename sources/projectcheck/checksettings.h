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
#ifndef CHECKSETTINGS_H
#define CHECKSETTINGS_H

#include "../projectcheck.h"

#include <QHash>
#include <QList>
#include <QString>

class DiagramContext;
class QETProject;

/**
	@brief The CheckSettings namespace
	The choices of the checks kept in a project (#1878 B12), so that two
	machines which open one project get the same check results.

	Each option is an on/off switch of the application (QetSettings) which
	changes what a check finds. A project follows the application's
	switches until it is given its own: then it stores every option, as a
	project property ("check_..." in <properties>, like drawing_standard),
	which an older QElectroTech keeps when it saves the project. A project
	that follows the application stores nothing, and saves the same bytes
	as before.

	A project can also switch a check off, and accept a finding with a note
	so that the Check panel no longer lists it (#1878 B8): stored the same
	way, in check_rules_off (the ids of the checks switched off) and
	check_accepted (each accepted finding's findingId() and its note), only
	when there is something to store.
*/
namespace CheckSettings
{
	enum class Option {
		DuplicatesCaseSensitive,    ///< QetSettings::duplicateLabelsCaseSensitive()
		DuplicatesByDesignation,    ///< QetSettings::duplicateLabelsByDesignation()
		DuplicatesSignalsByPlant,   ///< QetSettings::duplicateLabelsSignalsByPlant()
		UnwiredSkipWirelessFolios,  ///< QetSettings::unwiredSymbolsSkipWirelessFolios()
		UnwiredSkipStripTerminals,  ///< QetSettings::unwiredSymbolsSkipStripTerminals()
		UnwiredSkipContacts         ///< QetSettings::unwiredSymbolsSkipContacts()
	};

		/// Every option, in the order the project properties list them
	QList<Option> options();
		/// The project property an option is stored as
	QString key(Option option);
		/// Whether @p key is a property of the checks, which the table
		/// of project variables does not show
	bool isCheckKey(const QString &key);

	bool applicationValue(Option option);
	void setApplicationValue(Option option, bool value);

		/// Whether @p project has its own options rather than the
		/// application's
	bool hasOwnOptions(QETProject *project);
		/// The option as it applies to @p project: its own, else the
		/// application's. The application's when @p project is null
	bool value(QETProject *project, Option option);
		/// Change @p option where it applies: in @p project when it has
		/// its own options, else in the application
	void setValue(QETProject *project, Option option, bool value);

		/// @p properties with every option set to @p own, or with none
		/// (the project follows the application) when @p own is empty
	DiagramContext withOptions(DiagramContext properties,
							   const QHash<Option, bool> &own);

		/// Whether @p kind runs for @p project: every check does until
		/// the project switches it off (B8)
	bool ruleEnabled(QETProject *project, ProjectCheck::Kind kind);
	void setRuleEnabled(QETProject *project, ProjectCheck::Kind kind, bool enabled);
		/// @p properties with the checks of @p off switched off and every
		/// other check this QElectroTech knows on; the ids of checks it
		/// does not know (from a newer one) are kept
	DiagramContext withRulesOff(DiagramContext properties,
								const QList<ProjectCheck::Kind> &off);

		/// @p properties with the checks switched off and the findings
		/// accepted of @p from
	DiagramContext withPanelState(DiagramContext properties, const DiagramContext &from);

		/// What a finding is about, the same in every session: its check
		/// and the uuids of its symbols and wires, and
		/// any ids the check gives (ProjectCheck::Finding::ids), never
		/// where they are drawn nor the order of the file. A finding
		/// about none of them is known by its text.
	QString findingId(ProjectCheck::Kind kind, const ProjectCheck::Finding &finding);
		/// The findings accepted in @p project: id to note
	QHash<QString, QString> acceptedFindings(QETProject *project);
	void acceptFinding(QETProject *project, const QString &id, const QString &note);
	void unacceptFinding(QETProject *project, const QString &id);
}

#endif // CHECKSETTINGS_H
