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
#include "spacemousebuttonmap.h"

#include <QSettings>

namespace {
	const QString SETTINGS_GROUP = QStringLiteral("spacemouse/buttons/");
}

/**
	@brief SpaceMouseButtonMap::actionId
	@param button
	@return see the declaration's doc comment
*/
QString SpaceMouseButtonMap::actionId(int button)
{
	QSettings settings;
	return settings.value(SETTINGS_GROUP + QString::number(button)).toString();
}

/**
	@brief SpaceMouseButtonMap::setActionId
	@param button
	@param action_id
*/
void SpaceMouseButtonMap::setActionId(int button, const QString &action_id)
{
	QSettings settings;
	const QString key = SETTINGS_GROUP + QString::number(button);
	if (action_id.isEmpty()) {
		settings.remove(key);
	} else {
		settings.setValue(key, action_id);
	}
}

/**
	@brief SpaceMouseButtonMap::allBindings
	@return see the declaration's doc comment
*/
QMap<int, QString> SpaceMouseButtonMap::allBindings()
{
	QMap<int, QString> bindings;
	QSettings settings;
	settings.beginGroup(QStringLiteral("spacemouse/buttons"));
	for (const QString &key : settings.childKeys())
	{
		bool ok = false;
		const int button = key.toInt(&ok);
		if (ok) {
			bindings.insert(button, settings.value(key).toString());
		}
	}
	settings.endGroup();
	return bindings;
}
