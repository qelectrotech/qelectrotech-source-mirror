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
#ifndef SHORTCUTBARSETTINGS_H
#define SHORTCUTBARSETTINGS_H

#include <QList>
#include <QString>
#include <QStringList>

/**
	@brief The commands shown on the diagram editor's shortcut bar.

	The bar opens at the cursor and shows a different row of commands
	depending on what is selected. Each row is a list of ShortcutManager ids,
	so any registered command can go on it and the bar needs no command list
	of its own. Stored in QSettings, one key per context; a context the user
	never changed uses the defaults below.
*/
class ShortcutBarSettings
{
	public:
		enum Context {
			Canvas,     ///< nothing selected
			Selection,  ///< elements, texts or shapes selected
			Conductor   ///< only conductors selected
		};

		static QList<Context> contexts();
		static QString title(Context context);
		static QStringList ids(Context context);
		static QStringList defaultIds(Context context);
		static void setIds(Context context, const QStringList &ids);
		static QStringList availableIds();
};

#endif // SHORTCUTBARSETTINGS_H
