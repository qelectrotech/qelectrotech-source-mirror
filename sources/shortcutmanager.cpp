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
#include "shortcutmanager.h"

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <algorithm>
#include <utility>

namespace {
	const QString SETTINGS_GROUP = QStringLiteral("shortcuts/");
}

ShortcutManager &ShortcutManager::instance()
{
	static ShortcutManager manager;
	return manager;
}

/**
	@brief ShortcutManager::savedSequence
	@param id
	@param default_sequence
	@return the user-overridden sequence stored for \a id, or \a default_sequence
	if the user never overrode it. A stored-but-empty sequence (the user
	cleared the shortcut) is returned as an empty QKeySequence, not the default.
*/
QKeySequence ShortcutManager::savedSequence(const QString &id, const QKeySequence &default_sequence) const
{
	QSettings settings;
	const QString key = SETTINGS_GROUP + id;
	if (!settings.contains(key)) {
		return default_sequence;
	}
	return QKeySequence::fromString(settings.value(key).toString());
}

/**
	@brief ShortcutManager::registerAction
	Register \a target under \a id, applying the user's saved override (if
	any) or \a default_sequence otherwise. \a target must be a QAction or a
	QAbstractButton -- both declare a "shortcut" QKeySequence property, which
	is what's actually read/written here, so this works for either without a
	separate code path. Safe to call once per instance that shares this id
	(e.g. once per open window of the same kind) -- the first call to reach a
	given id fixes its category, description (taken from the target's current
	"text" property) and default sequence for the lifetime of the
	application; later calls just register another live target to update.
*/
void ShortcutManager::registerAction(QObject *target, const QString &id,
				      const QString &category,
				      const QKeySequence &default_sequence)
{
	if (!target) {
		return;
	}

	auto it = m_entries.find(id);
	if (it == m_entries.end()) {
		Entry entry;
		entry.category = category;
		entry.description = target->property("text").toString();
		entry.description.remove(QLatin1Char('&'));
		entry.default_sequence = default_sequence;
		it = m_entries.insert(id, entry);
		m_order << id;
	}

	QList<QPointer<QObject>> &targets = it->targets;
	targets.erase(std::remove_if(targets.begin(), targets.end(),
				      [](const QPointer<QObject> &t) { return t.isNull(); }),
		      targets.end());
	if (!targets.contains(target)) {
		targets << target;
	}

	target->setProperty("shortcut", QVariant::fromValue(savedSequence(id, it->default_sequence)));
}

/**
	@return every registered shortcut, in registration order, along with its
	currently effective sequence -- for display in the Shortcuts config page.
*/
QList<ShortcutManager::ShortcutInfo> ShortcutManager::allShortcuts() const
{
	QList<ShortcutInfo> list;
	for (const QString &id : m_order) {
		const Entry &entry = m_entries.value(id);
		ShortcutInfo info;
		info.id = id;
		info.category = entry.category;
		info.description = entry.description;
		info.default_sequence = entry.default_sequence;
		info.current_sequence = savedSequence(id, entry.default_sequence);
		list << info;
	}
	return list;
}

QKeySequence ShortcutManager::currentSequence(const QString &id) const
{
	auto it = m_entries.find(id);
	if (it == m_entries.end()) {
		return QKeySequence();
	}
	return savedSequence(id, it->default_sequence);
}

/**
	@brief ShortcutManager::setSequence
	Persist \a sequence as the binding for \a id and apply it immediately to
	every currently live QAction registered under that id. Persisted as "no
	override" when \a sequence matches the id's default, so a future QET
	version raising that default takes effect for users who never customized it.
*/
void ShortcutManager::setSequence(const QString &id, const QKeySequence &sequence)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end()) {
		return;
	}

	QSettings settings;
	const QString key = SETTINGS_GROUP + id;
	if (sequence == it->default_sequence) {
		settings.remove(key);
	} else {
		settings.setValue(key, sequence.toString());
	}

	for (const QPointer<QObject> &target : std::as_const(it->targets)) {
		if (target) {
			target->setProperty("shortcut", QVariant::fromValue(sequence));
		}
	}
}

void ShortcutManager::resetToDefault(const QString &id)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end()) {
		return;
	}
	setSequence(id, it->default_sequence);
}

void ShortcutManager::resetAllToDefaults()
{
	for (const QString &id : m_order) {
		resetToDefault(id);
	}
}
