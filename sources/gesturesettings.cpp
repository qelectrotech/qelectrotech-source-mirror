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
#include "gesturesettings.h"

#include <QSettings>

namespace {
const QString directions_key = QStringLiteral("diagrameditor/gestures/directions");

QString settingsKey(ShortcutBarSettings::Context context)
{
	switch (context)
	{
		case ShortcutBarSettings::Canvas:
			return QStringLiteral("diagrameditor/gestures/canvas");
		case ShortcutBarSettings::Selection:
			return QStringLiteral("diagrameditor/gestures/selection");
		case ShortcutBarSettings::Conductor:
			return QStringLiteral("diagrameditor/gestures/conductor");
	}
	return QString();
}
}

/**
	@return how many directions the ring has: 8 (the default) or 4
*/
int GestureSettings::directions()
{
	return QSettings().value(directions_key, 8).toInt() == 4 ? 4 : 8;
}

/**
	@brief GestureSettings::setDirections
	Save 4 or 8 directions. 8 is the default and removes the key.
*/
void GestureSettings::setDirections(int directions)
{
	QSettings settings;
	if (directions == 4) {
		settings.setValue(directions_key, 4);
	} else {
		settings.remove(directions_key);
	}
}

/**
	@return true if the user picked the commands for @a context, false
	when the ring follows the shortcut bar
*/
bool GestureSettings::isCustom(ShortcutBarSettings::Context context)
{
	return QSettings().contains(settingsKey(context));
}

/**
	@return the ids on the ring for @a context: the user's list if they
	picked one, the defaults otherwise
*/
QStringList GestureSettings::ids(ShortcutBarSettings::Context context)
{
	QSettings settings;
	const QString key = settingsKey(context);
	if (!settings.contains(key)) {
		return defaultIds(context);
	}
	return settings.value(key).toStringList();
}

/**
	@return the shortcut bar's commands for @a context, without its pinned
	elements: what the ring has shown since it was added
*/
QStringList GestureSettings::defaultIds(ShortcutBarSettings::Context context)
{
	QStringList ids;
	for (const QString &id : ShortcutBarSettings::ids(context)) {
		if (!ShortcutBarSettings::isElement(id)) {
			ids << id;
		}
	}
	return ids;
}

/**
	@brief GestureSettings::setIds
	Save @a ids for @a context; trailing empty directions are dropped.
	Saving the defaults removes the key, so the ring follows the shortcut
	bar again.
*/
void GestureSettings::setIds(ShortcutBarSettings::Context context, QStringList ids)
{
	while (!ids.isEmpty() && ids.constLast().isEmpty()) {
		ids.removeLast();
	}
	if (ids == defaultIds(context)) {
		reset(context);
	} else {
		QSettings().setValue(settingsKey(context), ids);
	}
}

/**
	@brief GestureSettings::reset
	Make the ring for @a context follow the shortcut bar again.
*/
void GestureSettings::reset(ShortcutBarSettings::Context context)
{
	QSettings().remove(settingsKey(context));
}
