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
#include <QThread>

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
	QString compilation_info();
	void send_info_to_debug();

signals:

private:
	void init_get_Screen_info();
	void init_get_cpu_info();
	void init_get_cpu_info_linux();
	void init_get_cpu_info_winnt();
	void init_get_cpu_info_macos();
	struct Pc
	{
		struct Screen
		{
			int32_t count;
			int32_t width[10];
			int32_t height[10];
			int32_t Max_width;
			int32_t Max_height;
		}screen;
		struct Built
		{
			QString version=
		#ifdef __GNUC__
		#ifdef __APPLE_CC__
					"CLANG " + QString(__clang_version__);
			#else
					"GCC " + QString(__VERSION__);
#endif
#endif
			QString QT=QString(QT_VERSION_STR);
			QString date=QString(__DATE__);
			QString time=QString(__TIME__);
		}built;
		struct CPU
		{
			QString info;
			QString Architecture=QString(
					QSysInfo::currentCpuArchitecture());
			int32_t ThreadCount=QThread::idealThreadCount();
		}cpu;
		struct RAM
		{
			QString Total;
			QString Available;
		}ram;
		struct GPU
		{
			QString info;
			QString RAM;

		}gpu;
		struct OS
		{
			QString type=QString(QSysInfo::kernelType());
			QString name=QString(QSysInfo::prettyProductName());
			QString kernel=QString(QSysInfo::kernelVersion());
		}os;
	}pc;


};

#endif // MACHINE_INFO_H
