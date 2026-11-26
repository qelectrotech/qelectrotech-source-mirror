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
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"
#include "../utils/qetsettings.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
		/// Every property of the checks starts so
	const QString key_prefix = QStringLiteral("check_");
		/// The ids of the checks switched off, comma separated
	const QString rules_off_key = QStringLiteral("check_rules_off");
		/// The accepted findings: a JSON object, findingId() to note
	const QString accepted_key = QStringLiteral("check_accepted");

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
	if (key == rules_off_key || key == accepted_key) return true;
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

namespace {
	QStringList rulesOff(QETProject *project)
	{
		if (!project) return {};
		return project->projectProperties().value(rules_off_key).toString()
				.split(QLatin1Char(','), Qt::SkipEmptyParts);
	}

	QString uuidId(const char *prefix, const QUuid &uuid)
	{
		return QLatin1String(prefix) + uuid.toString(QUuid::WithoutBraces);
	}

	void setProperties(QETProject *project, const DiagramContext &properties)
	{
		if (project->isReadOnly() || project->projectProperties() == properties) return;
		project->setUnshownProjectProperties(properties);
		project->setModified(true);
	}
}

bool CheckSettings::ruleEnabled(QETProject *project, ProjectCheck::Kind kind)
{
	return !rulesOff(project).contains(ProjectCheck::id(kind));
}

void CheckSettings::setRuleEnabled(QETProject *project, ProjectCheck::Kind kind, bool enabled)
{
	if (!project) return;
	QList<ProjectCheck::Kind> off;
	for (ProjectCheck::Kind other : ProjectCheck::kinds())
		if (other != kind && !ruleEnabled(project, other)) off << other;
	if (!enabled) off << kind;
	setProperties(project, withRulesOff(project->projectProperties(), off));
}

DiagramContext CheckSettings::withRulesOff(DiagramContext properties,
										   const QList<ProjectCheck::Kind> &off)
{
	QStringList ids;
	for (const QString &id : properties.value(rules_off_key).toString()
			.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
		ProjectCheck::Kind kind;
		if (!ProjectCheck::kindFromId(id, &kind)) ids << id;
	}
	for (ProjectCheck::Kind kind : off) ids << ProjectCheck::id(kind);
	ids.removeDuplicates();
	ids.sort();
	if (ids.isEmpty())
		properties.remove(rules_off_key);
	else
		properties.addValue(rules_off_key, ids.join(QLatin1Char(',')));
	return properties;
}

DiagramContext CheckSettings::withPanelState(DiagramContext properties,
											 const DiagramContext &from)
{
	for (const QString &key : {rules_off_key, accepted_key}) {
		properties.remove(key);
		if (from.contains(key)) properties.addValue(key, from.value(key));
	}
	return properties;
}

QString CheckSettings::findingId(ProjectCheck::Kind kind, const ProjectCheck::Finding &finding)
{
		// A wire's uuid lasts: one an older file does not give is worked
		// out from its two ends (QETProject::derivedItemUuid())
	QStringList parts = finding.ids;
	for (const ProjectCheck::Place &place : finding.places) {
		if (auto *element = qobject_cast<Element *>(place.item.data()))
			parts << uuidId("e:", element->uuid());
		else if (auto *conductor = qobject_cast<Conductor *>(place.item.data()))
			parts << uuidId("w:", conductor->uuid());
		if (!place.element.isNull())
			parts << uuidId("e:", place.element);
		for (const QUuid &wire : place.wires)
			parts << uuidId("w:", wire);
	}
	parts.removeDuplicates();
	parts.sort();
	const QByteArray what = parts.isEmpty()
		? QByteArrayLiteral("text:") + finding.text.toUtf8()
		: parts.join(QLatin1Char(' ')).toUtf8();
		// A potential may have hundreds of wires: the id stays short
	return ProjectCheck::id(kind) + QLatin1Char(':')
			+ QString::fromLatin1(QCryptographicHash::hash(what, QCryptographicHash::Sha1).toHex());
}

QHash<QString, QString> CheckSettings::acceptedFindings(QETProject *project)
{
	QHash<QString, QString> accepted;
	if (!project) return accepted;
	const QJsonObject object = QJsonDocument::fromJson(
		project->projectProperties().value(accepted_key).toString().toUtf8()).object();
	for (auto it = object.constBegin() ; it != object.constEnd() ; ++it)
		accepted.insert(it.key(), it.value().toString());
	return accepted;
}

namespace {
	void setAccepted(QETProject *project, const QHash<QString, QString> &accepted)
	{
		DiagramContext properties = project->projectProperties();
		if (accepted.isEmpty())
			properties.remove(accepted_key);
		else {
			QJsonObject object;
			for (auto it = accepted.cbegin() ; it != accepted.cend() ; ++it)
				object.insert(it.key(), it.value());
				// Keys sorted: the same findings save the same bytes
			properties.addValue(accepted_key, QString::fromUtf8(
				QJsonDocument(object).toJson(QJsonDocument::Compact)));
		}
		setProperties(project, properties);
	}
}

void CheckSettings::acceptFinding(QETProject *project, const QString &id, const QString &note)
{
	if (!project || id.isEmpty()) return;
	auto accepted = acceptedFindings(project);
	accepted.insert(id, note);
	setAccepted(project, accepted);
}

void CheckSettings::unacceptFinding(QETProject *project, const QString &id)
{
	if (!project) return;
	auto accepted = acceptedFindings(project);
	if (accepted.remove(id)) setAccepted(project, accepted);
}
