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
#include <QProcess>
#include <QApplication>
#include <QDebug>
#include <QSysInfo>
#include <QStorageInfo>
#include <QLibraryInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "qet.h"

/**
	@brief Machine_info::Machine_info
	@param parent
*/
Machine_info::Machine_info(QObject *parent) : QObject(parent)
{
	init_get_Screen_info();
	init_get_cpu_info();
}

/**
	@brief Machine_info::send_info_to_debug
*/
void Machine_info::send_info_to_debug()
{
	qInfo()<<"debugging enabled:"
		<< QLibraryInfo::isDebugBuild();
	qInfo()<< "Qt library version:"
		<< QLibraryInfo::version();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
	qInfo()<< "Qt library location default prefix:"
		<< QLibraryInfo::location(QLibraryInfo::PrefixPath);
	qInfo()<< "Qt library location documentation:"
		<< QLibraryInfo::location(QLibraryInfo::DocumentationPath);
	qInfo()<< "Qt library location headers:"
		<< QLibraryInfo::location(QLibraryInfo::HeadersPath);
	qInfo()<< "Qt library location libraries:"
		<< QLibraryInfo::location(QLibraryInfo::LibrariesPath);
	qInfo()<< "Qt library location executables:"
		<< QLibraryInfo::location(QLibraryInfo::LibraryExecutablesPath);
	qInfo()<< "Qt library location Qt binaries:"
		<< QLibraryInfo::location(QLibraryInfo::BinariesPath);
	qInfo()<< "Qt library location Qt plugins:"
		<< QLibraryInfo::location(QLibraryInfo::PluginsPath);
	qInfo()<< "Qt library location installed QML extensions:"
		<< QLibraryInfo::location(QLibraryInfo::ImportsPath);
	qInfo()<< "Qt library location installed QML extensions:"
		<< QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
	qInfo()<< "Qt library location dependent Qt data:"
		<< QLibraryInfo::location(QLibraryInfo::ArchDataPath);
	qInfo()<< "Qt library location independent Qt data:"
		<< QLibraryInfo::location(QLibraryInfo::DataPath);
	qInfo()<< "Qt library location translation:"
		<< QLibraryInfo::location(QLibraryInfo::TranslationsPath);
	qInfo()<< "Qt library location examples:"
		<< QLibraryInfo::location(QLibraryInfo::ExamplesPath);
	qInfo()<< "Qt library location Qt testcases:"
		<< QLibraryInfo::location(QLibraryInfo::TestsPath);
#ifndef Q_OS_WIN
	qInfo()<< "Qt library location Qt settings:"
		<< QLibraryInfo::location(QLibraryInfo::SettingsPath);
#endif
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	qInfo()<< "Qt library path default prefix:"
		<< QLibraryInfo::path(QLibraryInfo::PrefixPath);
	qInfo()<< "Qt library path documentation:"
		<< QLibraryInfo::path(QLibraryInfo::DocumentationPath);
	qInfo()<< "Qt library path headers:"
		<< QLibraryInfo::path(QLibraryInfo::HeadersPath);
	qInfo()<< "Qt library path libraries:"
		<< QLibraryInfo::path(QLibraryInfo::LibrariesPath);
	qInfo()<< "Qt library path executables:"
		<< QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath);
	qInfo()<< "Qt library path Qt binaries:"
		<< QLibraryInfo::path(QLibraryInfo::BinariesPath);
	qInfo()<< "Qt library path Qt plugins:"
		<< QLibraryInfo::path(QLibraryInfo::PluginsPath);
//	qInfo()<< "Qt library path installed QML extensions:"
//		<< QLibraryInfo::path(QLibraryInfo::ImportsPath);
	qInfo()<< "Qt library path installed QML extensions:"
		<< QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath);
	qInfo()<< "Qt library path dependent Qt data:"
		<< QLibraryInfo::path(QLibraryInfo::ArchDataPath);
	qInfo()<< "Qt library path independent Qt data:"
		<< QLibraryInfo::path(QLibraryInfo::DataPath);
	qInfo()<< "Qt library path translation:"
		<< QLibraryInfo::path(QLibraryInfo::TranslationsPath);
	qInfo()<< "Qt library path examples:"
		<< QLibraryInfo::path(QLibraryInfo::ExamplesPath);
	qInfo()<< "Qt library path Qt testcases:"
		<< QLibraryInfo::path(QLibraryInfo::TestsPath);
#ifndef Q_OS_WIN
	qInfo()<< "Qt library path Qt settings:"
		<< QLibraryInfo::path(QLibraryInfo::SettingsPath);
#endif
#endif
	qInfo()<< "GitRevision " + QString(GIT_COMMIT_SHA);
	qInfo()<< "QElectroTech V " + QET::displayedVersion;
	qInfo()<< tr("Compilation : ") + pc.built.version;
	qInfo()<< "Built with Qt " + pc.built.QT
		  + " - Date : " + pc.built.date
		  + " : " + pc.built.time;
	qInfo()<< "Run with Qt "+ QString(qVersion())
		  + " using"
		  + QString(" %1 thread(s)").arg(pc.cpu.ThreadCount);
	qInfo()<< "CPU : " + pc.cpu.info;
	qInfo()<< pc.ram.Total;
	qInfo()<< pc.ram.Available;
	qInfo()<< "GPU : " + pc.gpu.info;
	qInfo()<< "GPU RAM : " + pc.gpu.RAM;

	qInfo()<< "OS : " + pc.os.type
		  + "  - " + pc.cpu.Architecture
		  + " - Version : "+pc.os.name
		  + " - Kernel : "+pc.os.kernel;
	qInfo()<< "*** Qt screens ***";

	for (int ii = 0; ii < pc.screen.count; ++ii) {
		qInfo()<<"( "
			+ QString::number(ii + 1)
			+ " : "
			+ QString::number(pc.screen.width[ii])
			+ " x "
			+ QString::number(pc.screen.height[ii])
			+ " )";
	}
}

/**
	@brief Machine_info::init_get_Screen_info
	Finds the largest screen and saves the values
*/
void Machine_info::init_get_Screen_info()
{
	const auto screens = qApp->screens();
	pc.screen.count=screens.count();
	for (int ii = 0; ii < pc.screen.count; ++ii) {
		pc.screen.width[ii]=
				screens[ii]->geometry().width()
				* screens[ii]->devicePixelRatio();
		if(pc.screen.Max_width<pc.screen.width[ii])
			pc.screen.Max_width=pc.screen.width[ii];

		pc.screen.height[ii]=
				screens[ii]->geometry().height()
				* screens[ii]->devicePixelRatio();
		if(pc.screen.Max_height<pc.screen.height[ii])
			pc.screen.Max_height=pc.screen.height[ii];
	}
}

/**
	@brief Machine_info::init_get_cpu_info
*/
void Machine_info::init_get_cpu_info()
{
#ifdef __GNUC__
#ifdef __APPLE_CC__
	init_get_cpu_info_macos();
#else
	if (pc.os.type == "linux")
		init_get_cpu_info_linux();
	if (pc.os.type == "winnt")
		init_get_cpu_info_winnt();
#endif
#endif
}

/**
	@brief Machine_info::init_get_cpu_info_linux
*/
void Machine_info::init_get_cpu_info_linux()
{
	QProcess linuxcpuinfo;
	linuxcpuinfo.start("bash", QStringList()
			   << "-c"
			   << "cat /proc/cpuinfo |grep 'model name' | uniq");
	linuxcpuinfo.waitForFinished();
	QString linuxOutput = linuxcpuinfo.readAllStandardOutput();
	pc.cpu.info=QString(linuxOutput.toLocal8Bit().constData());

	QProcess p;
	p.start("awk", QStringList()
		<< "/MemTotal/ { print $2 }"
		<< "/proc/meminfo");
	p.waitForFinished();
	QString memory = p.readAllStandardOutput();
	//compilation_info += "<br>"
	pc.ram.Total=QString("RAM Total : %1 MB").arg(
				memory.toLong() / 1024);
	p.close();

	QProcess qp;
	qp.start("awk",
		 QStringList()
		 << "/MemAvailable/ {print $2}"
		 << "/proc/meminfo");
	qp.waitForFinished();
	QString AvailableMemory = qp.readAllStandardOutput();
	pc.ram.Available=QString("RAM Available : %1 MB").arg(
				AvailableMemory.toLong() / 1024);
	qp.close();

	QProcess linuxgpuinfo;
	linuxgpuinfo.start("bash",
			   QStringList()
			   << "-c"
			   << "lspci | grep VGA | cut -d : -f 3");
	linuxgpuinfo.waitForFinished();
	QString linuxGPUOutput = linuxgpuinfo.readAllStandardOutput();
	pc.gpu.info=QString(linuxGPUOutput.toLocal8Bit().constData());
	pc.gpu.RAM="@ToDo";
}

/**
	@brief Machine_info::init_get_cpu_info_winnt
*/
void Machine_info::init_get_cpu_info_winnt()
{
	QProcess wincpuinfo;
	wincpuinfo.start("wmic",
			 QStringList() << "cpu" << "get" << "name");
	wincpuinfo.waitForFinished();
	QString windows_output = wincpuinfo.readAllStandardOutput().toUpper();
	pc.cpu.info=QString(windows_output.toLocal8Bit().constData());

	QProcess wingpuinfo;
	wingpuinfo.start("wmic",
			 QStringList()
			 << "PATH"
			 << "Win32_videocontroller"
			 << "get"
			 << "VideoProcessor ");
	wingpuinfo.waitForFinished();
	QString WinGPUOutput = wingpuinfo.readAllStandardOutput();
	pc.gpu.info=QString(WinGPUOutput.toLocal8Bit().constData());

	QProcess wingpuraminfo;
	wingpuraminfo.start("wmic",
				QStringList()
				<< "PATH"
				<< "Win32_videocontroller"
				<< "get"
				<< "AdapterRAM ");
	wingpuraminfo.waitForFinished();
	QString WinGPURAMOutput = wingpuraminfo.readAllStandardOutput();
	pc.gpu.RAM=QString("RAM Total : %1 B").arg(
	WinGPURAMOutput.toLocal8Bit().constData());
	wingpuraminfo.close();

#ifdef Q_OS_WIN
	MEMORYSTATUSEX memory_status;
	ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
	memory_status.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memory_status)) {
		pc.ram.Total .append(
					QString("RAM Total : %1 MB")
					.arg(memory_status.ullTotalPhys / (1024 * 1024)));
		pc.ram.Available .append(
					QString("RAM Available : %1 MB")
					.arg(memory_status.ullAvailPhys / (1024 * 1024)));
	} else {
		pc.ram.Total.append("Unknown RAM");
		pc.ram.Available.append("Unknown RAM");
	}
#endif
}

/**
	@brief Machine_info::init_get_cpu_info_macos
*/
void Machine_info::init_get_cpu_info_macos()
{
	QProcess macoscpuinfo;
	macoscpuinfo.start("bash",
			   QStringList()
			   << "-c"
			   << "sysctl -n machdep.cpu.brand_string");
	macoscpuinfo.waitForFinished();
	QString macosOutput = macoscpuinfo.readAllStandardOutput();
	pc.cpu.info=QString(macosOutput.toLocal8Bit().constData());

	QProcess macosraminfo;
	macosraminfo.start("bash",
			   QStringList()
			   << "-c"
			   << "sysctl -n hw.memsize");
	macosraminfo.waitForFinished();
	QString macosRAMOutput = macosraminfo.readAllStandardOutput();
	pc.ram.Total=QString("RAM Total : %1 MB").arg(
	macosRAMOutput.toLongLong() /1024 / 1024);
	macosraminfo.close();
}

/**
	@brief Machine_info::i_max_screen_width
	@return max screen width
*/
int32_t Machine_info::i_max_screen_width()
{
	return pc.screen.Max_width;
}

/**
	@brief Machine_info::i_max_screen_height
	@return max screen height
*/
int32_t Machine_info::i_max_screen_height()
{
	return pc.screen.Max_height;
}

/**
	@brief Machine_info::compilation_info
	@return compilation_info
*/
QString Machine_info::compilation_info()
{
	QString compilation_info = "<br />" + tr("Compilation :   ");
	compilation_info +=pc.built.version;
	compilation_info += "<br>Built with Qt " + pc.built.QT;
	compilation_info += " - Date : " + pc.built.date;
	compilation_info += " : " + pc.built.time;
	compilation_info += " <br>Run with Qt "+ QString(qVersion());
	compilation_info += " using"
			+ QString(" %1 thread(s)").arg(pc.cpu.ThreadCount);
	compilation_info +=  "<br> CPU : " + pc.cpu.info;
	compilation_info += "<br>" + pc.ram.Total;
	compilation_info += "<br>" + pc.ram.Available;
	compilation_info += "<br>GPU : " + pc.gpu.info;
	compilation_info += "<br>GPU RAM : " + pc.gpu.RAM;

	compilation_info += "<br>  OS : " + pc.os.type;
	compilation_info += "  -   " + pc.cpu.Architecture;
	compilation_info += " -  Version :    "+pc.os.name;
	compilation_info += "</br> -  Kernel :     "+pc.os.kernel;
	compilation_info += "<br>  *** Qt screens *** </br>";

	for (int ii = 0; ii < pc.screen.count; ++ii) {
		compilation_info += "<br> ( "
			+ QString::number(ii + 1)
			+ " : "
			+ QString::number(pc.screen.width[ii])
			+ " x "
			+ QString::number(pc.screen.height[ii])
			+ " ) </br>";
	}
	return compilation_info;
}
