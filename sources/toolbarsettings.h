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
#ifndef TOOLBARSETTINGS_H
#define TOOLBARSETTINGS_H

#include <QSize>
#include <Qt>

class QMainWindow;

/**
	@brief How every window's toolbars look: icon size, text with the
	icons, and whether they can be moved. Stored in QSettings; a value
	the user never changed keeps Qt's default, so nothing changes for
	anyone who does not set one.
*/
class ToolbarSettings
{
	public:
			/// Icon size in pixels, or 0 for the style's default size
		static int iconSize();
		static Qt::ToolButtonStyle buttonStyle();
		static bool locked();
		static void save(int icon_size, Qt::ToolButtonStyle style, bool locked);

		static void applyTo(QMainWindow *window);
		static void applyToAll();
};

#endif // TOOLBARSETTINGS_H
