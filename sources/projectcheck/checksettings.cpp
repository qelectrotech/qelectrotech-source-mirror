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
#include "checksettings.h"

#include "../diagramcontext.h"
#include "../qet.h"
#include "../qetproject.h"
#include "../utils/qetsettings.h"

namespace {
		/// Every property of the checks starts so
	const QString key_prefix = QStringLiteral("check_");

	QString text(bool value)
	{
		return value ? QStringLiteral("true") : QStringLiteral("false");
	}
}

QList<CheckSettings::Option> CheckSettings::options()
{
	return {Option::DuplicatesCaseSensitive, Option::DuplicatesByDesignation,
			Option::DuplicatesSignalsByPlant, Option::UnwiredSkipWirelessFolios,
			Option::UnwiredSkipStripTerminals, Option::UnwiredSkipContacts};
}

QString CheckSettings::key(Option option)
{
	switch (option) {
		case Option::DuplicatesCaseSensitive:
			return key_prefix + QStringLiteral("duplicates_case_sensitive");
		case Option::DuplicatesByDesignation:
			return key_prefix + QStringLiteral("duplicates_by_designation");
		case Option::DuplicatesSignalsByPlant:
			return key_prefix + QStringLiteral("duplicates_signals_by_plant");
		case Option::UnwiredSkipWirelessFolios:
			return key_prefix + QStringLiteral("unwired_skip_wireless_folios");
		case Option::UnwiredSkipStripTerminals:
			return key_prefix + QStringLiteral("unwired_skip_strip_terminals");
		case Option::UnwiredSkipContacts:
			return key_prefix + QStringLiteral("unwired_skip_contacts");
	}
	return QString();
}

bool CheckSettings::isCheckKey(const QString &key)
{
	if (!key.startsWith(key_prefix)) return false;
	for (Option option : options())
		if (key == CheckSettings::key(option)) return true;
	return false;
}

bool CheckSettings::applicationValue(Option option)
{
	switch (option) {
		case Option::DuplicatesCaseSensitive: return QetSettings::duplicateLabelsCaseSensitive();
		case Option::DuplicatesByDesignation: return QetSettings::duplicateLabelsByDesignation();
		case Option::DuplicatesSignalsByPlant: return QetSettings::duplicateLabelsSignalsByPlant();
		case Option::UnwiredSkipWirelessFolios: return QetSettings::unwiredSymbolsSkipWirelessFolios();
		case Option::UnwiredSkipStripTerminals: return QetSettings::unwiredSymbolsSkipStripTerminals();
		case Option::UnwiredSkipContacts: return QetSettings::unwiredSymbolsSkipContacts();
	}
	return false;
}

void CheckSettings::setApplicationValue(Option option, bool value)
{
	switch (option) {
		case Option::DuplicatesCaseSensitive: QetSettings::setDuplicateLabelsCaseSensitive(value); break;
		case Option::DuplicatesByDesignation: QetSettings::setDuplicateLabelsByDesignation(value); break;
		case Option::DuplicatesSignalsByPlant: QetSettings::setDuplicateLabelsSignalsByPlant(value); break;
		case Option::UnwiredSkipWirelessFolios: QetSettings::setUnwiredSymbolsSkipWirelessFolios(value); break;
		case Option::UnwiredSkipStripTerminals: QetSettings::setUnwiredSymbolsSkipStripTerminals(value); break;
		case Option::UnwiredSkipContacts: QetSettings::setUnwiredSymbolsSkipContacts(value); break;
	}
}

/**
	@brief CheckSettings::hasOwnOptions
	A project which stores any option has its own: one whose file was
	edited by hand to keep only some takes the application's for the
	others.
*/
bool CheckSettings::hasOwnOptions(QETProject *project)
{
	if (!project) return false;
	const DiagramContext properties = project->projectProperties();
	for (Option option : options())
		if (properties.contains(key(option))) return true;
	return false;
}

bool CheckSettings::value(QETProject *project, Option option)
{
	if (project) {
		const DiagramContext properties = project->projectProperties();
		if (properties.contains(key(option)))
			return QET::infoFlagIsTrue(properties.value(key(option)).toString());
	}
	return applicationValue(option);
}

void CheckSettings::setValue(QETProject *project, Option option, bool value)
{
	if (!hasOwnOptions(project)) {
		setApplicationValue(option, value);
		return;
	}
	if (project->isReadOnly()) return;
	DiagramContext properties = project->projectProperties();
	if (properties.value(key(option)).toString() == text(value)) return;
	properties.addValue(key(option), text(value));
	project->setUnshownProjectProperties(properties);
	project->setModified(true);
}

DiagramContext CheckSettings::withOptions(DiagramContext properties,
										  const QHash<Option, bool> &own)
{
	for (Option option : options()) {
		if (own.isEmpty())
			properties.remove(key(option));
		else
			properties.addValue(key(option), text(own.value(option)));
	}
	return properties;
}
