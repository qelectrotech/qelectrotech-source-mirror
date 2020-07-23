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
#ifndef MACHINE_INFO_H
#define MACHINE_INFO_H

#include <QObject>

/**
	@brief The Machine_info class
	This class hold information from your PC.
*/
class Machine_info : public QObject
{
	Q_OBJECT
public:
	explicit Machine_info(QObject *parent = nullptr);
	int32_t i_max_screen_width();
	int32_t i_max_screen_height();

signals:

private:
	void init_get_Screen_info();
	int32_t Max_screen_width;
	int32_t Max_screen_height;


};

#endif // MACHINE_INFO_H
