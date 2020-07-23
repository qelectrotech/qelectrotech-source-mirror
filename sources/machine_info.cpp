/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "machine_info.h"
#include <QScreen>
#include <QApplication>

/**
	@brief Machine_info::Machine_info
	@param parent
*/
Machine_info::Machine_info(QObject *parent) : QObject(parent)
{
	init_get_Screen_info();
}

/**
	@brief Machine_info::init_get_Screen_info
	Finds the largest screen and saves the values
*/
void Machine_info::init_get_Screen_info()
{
	const auto screens = qApp->screens();
	for (int ii = 0; ii < screens.count(); ++ii)
	{
		if(
				Max_screen_width
				<
				screens[ii]->geometry().width()
				*
				screens[ii]->devicePixelRatio()
				)
		{
			Max_screen_width =
					screens[ii]->geometry().width()
					*
					screens[ii]->devicePixelRatio();
		}
		if(
				Max_screen_height
				<
				screens[ii]->geometry().height()
				*
				screens[ii]->devicePixelRatio()
				)
		{
			Max_screen_height =
					screens[ii]->geometry().height()
					*
					screens[ii]->devicePixelRatio();
		}
	}
}

/**
	@brief Machine_info::get_max_screen_width
	@return max screen width
 */
int32_t Machine_info::get_max_screen_width()
{
	return Max_screen_width;
}

/**
	@brief Machine_info::get_max_screen_height
	@return max screen height
*/
int32_t Machine_info::get_max_screen_height()
{
	return Max_screen_height;
}
