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
#ifndef GESTURESETTINGS_H
#define GESTURESETTINGS_H

#include "shortcutbarsettings.h"

#include <QStringList>

/**
	@brief The commands on the mouse gesture ring (right-drag on a folio).

	Until the user picks them, the ring shows the shortcut bar's commands
	for the selection, as it always has. A picked list is stored in
	QSettings, one key per context, and is positional: entry i is the
	command in direction i, clockwise from the top, and an empty entry is
	an empty direction. Pinned elements are never on the ring.
*/
class GestureSettings
{
	public:
		static int directions();
		static void setDirections(int directions);

		static bool isCustom(ShortcutBarSettings::Context context);
		static QStringList ids(ShortcutBarSettings::Context context);
		static QStringList defaultIds(ShortcutBarSettings::Context context);
		static void setIds(ShortcutBarSettings::Context context, QStringList ids);
		static void reset(ShortcutBarSettings::Context context);
};

#endif // GESTURESETTINGS_H
