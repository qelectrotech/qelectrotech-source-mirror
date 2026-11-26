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
}

#endif // CHECKSETTINGS_H
