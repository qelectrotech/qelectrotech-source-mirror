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
#include "shortcutbarsettings.h"

#include <QCoreApplication>
#include <QSettings>

namespace {
QString settingsKey(ShortcutBarSettings::Context context)
{
	switch (context)
	{
		case ShortcutBarSettings::Canvas:
			return QStringLiteral("diagrameditor/shortcut_bar/canvas");
		case ShortcutBarSettings::Selection:
			return QStringLiteral("diagrameditor/shortcut_bar/selection");
		case ShortcutBarSettings::Conductor:
			return QStringLiteral("diagrameditor/shortcut_bar/conductor");
	}
	return QString();
}
}

/**
	@return every context, in the order they are shown to the user
*/
QList<ShortcutBarSettings::Context> ShortcutBarSettings::contexts()
{
	return {Canvas, Selection, Conductor};
}

/**
	@return the name of @a context, for the configuration page
*/
QString ShortcutBarSettings::title(Context context)
{
	switch (context)
	{
		case Canvas:
			return QCoreApplication::translate("ShortcutBarSettings", "Folio, rien de sélectionné");
		case Selection:
			return QCoreApplication::translate("ShortcutBarSettings", "Éléments sélectionnés");
		case Conductor:
			return QCoreApplication::translate("ShortcutBarSettings", "Conducteurs sélectionnés");
	}
	return QString();
}

/**
	@return the ids to show for @a context: the user's list if they saved
	one, the defaults otherwise. A saved empty list stays empty.
*/
QStringList ShortcutBarSettings::ids(Context context)
{
	QSettings settings;
	const QString key = settingsKey(context);
	if (!settings.contains(key)) {
		return defaultIds(context);
	}
	return settings.value(key).toStringList();
}

/**
	@return the commands a new user sees for @a context
*/
QStringList ShortcutBarSettings::defaultIds(Context context)
{
	switch (context)
	{
		case Canvas:
			return {QStringLiteral("diagrameditor.insert_last_element"),
				QStringLiteral("diagrameditor.show_element_picker"),
				QStringLiteral("diagrameditor.add_text"),
				QStringLiteral("diagrameditor.add_line"),
				QStringLiteral("diagrameditor.add_rectangle"),
				QStringLiteral("diagrameditor.add_terminal_strip"),
				QStringLiteral("diagrameditor.paste"),
				QStringLiteral("diagrameditor.edit_diagram_properties")};
		case Selection:
			return {QStringLiteral("diagrameditor.rotate_selection"),
				QStringLiteral("diagrameditor.rotate_texts"),
				QStringLiteral("diagrameditor.edit_selection"),
				QStringLiteral("diagrameditor.copy"),
				QStringLiteral("diagrameditor.cut"),
				QStringLiteral("diagrameditor.delete_selection")};
		case Conductor:
			return {QStringLiteral("diagrameditor.conductor_reset"),
				QStringLiteral("diagrameditor.edit_selection"),
				QStringLiteral("diagrameditor.delete_selection")};
	}
	return {};
}

/**
	@brief ShortcutBarSettings::setIds
	Save @a ids for @a context. Saving the defaults removes the key, so a
	later change of defaults still reaches this user.
*/
void ShortcutBarSettings::setIds(Context context, const QStringList &ids)
{
	QSettings settings;
	if (ids == defaultIds(context)) {
		settings.remove(settingsKey(context));
	} else {
		settings.setValue(settingsKey(context), ids);
	}
}
