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
#include <QThread>
#include <QApplication>
#include <QDebug>
#include <QSysInfo>
#include <QStorageInfo>

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
	@brief Machine_info::i_max_screen_width
	@return max screen width
 */
int32_t Machine_info::i_max_screen_width()
{
	return Max_screen_width;
}

/**
	@brief Machine_info::i_max_screen_height
	@return max screen height
*/
int32_t Machine_info::i_max_screen_height()
{
	return Max_screen_height;
}

/**
	@brief Machine_info::compilation_info
	@return compilation_info
*/
QString Machine_info::compilation_info()
{
	QString compilation_info = "<br />" + tr("Compilation : ");
#ifdef __GNUC__
#ifdef __APPLE_CC__
	compilation_info += "  CLANG " + QString(__clang_version__ );
	compilation_info += " <br>Built with Qt " + QString(QT_VERSION_STR);
	compilation_info += " - Date : " + QString(__DATE__);
	compilation_info += " : " + QString(__TIME__);
	compilation_info += " <br>Run with Qt "+ QString(qVersion());
	compilation_info += " using" + QString(" %1 thread(s)").arg(
				QThread::idealThreadCount());
	QProcess macoscpuinfo;
	macoscpuinfo.start("bash",
			   QStringList()
			   << "-c"
			   << "sysctl -n machdep.cpu.brand_string");
	macoscpuinfo.waitForFinished();
	QString macosOutput = macoscpuinfo.readAllStandardOutput();
	compilation_info +=  "<br>"" CPU : "
			+ QString(macosOutput.toLocal8Bit().constData());

#else
	compilation_info += "  GCC " + QString(__VERSION__);
	compilation_info += "<br>Built with Qt " + QString(QT_VERSION_STR);
	compilation_info += " - Date : " + QString(__DATE__);
	compilation_info += " : " + QString(__TIME__);
	compilation_info += " <br>Run with Qt "+ QString(qVersion());
	compilation_info += " using"
			+ QString(" %1 thread(s)").arg(
				QThread::idealThreadCount());

	QString OSName = QSysInfo::kernelType();
	if (OSName == "linux")
	{
	QProcess linuxcpuinfo;
	linuxcpuinfo.start("bash", QStringList()
			   << "-c"
			   << "cat /proc/cpuinfo |grep 'model name' | uniq");
	linuxcpuinfo.waitForFinished();
	QString linuxOutput = linuxcpuinfo.readAllStandardOutput();
	compilation_info +=  "<br>"" CPU : "
			+ QString(linuxOutput.toLocal8Bit().constData());

	QProcess p;
	p.start("awk", QStringList()
		<< "/MemTotal/ { print $2 }"
		<< "/proc/meminfo");
	p.waitForFinished();
	QString memory = p.readAllStandardOutput();
	compilation_info += "<br>"
			+ QString("RAM Total : %1 MB").arg(
				memory.toLong() / 1024);
	p.close();

	QProcess qp;
	qp.start("awk",
		 QStringList()
		 << "/MemAvailable/ {print $2}"
		 << "/proc/meminfo");
	qp.waitForFinished();
	QString AvailableMemory = qp.readAllStandardOutput();
	compilation_info += "<br>"
			+ QString("RAM Available : %1 MB").arg(
				AvailableMemory.toLong() / 1024);
	qp.close();

	QProcess linuxgpuinfo;
	linuxgpuinfo.start("bash",
			   QStringList()
			   << "-c"
			   << "lspci | grep VGA | cut -d : -f 3");
	linuxgpuinfo.waitForFinished();
	QString linuxGPUOutput = linuxgpuinfo.readAllStandardOutput();
	compilation_info += "<br>"" GPU : "
			+ QString(linuxGPUOutput.toLocal8Bit().constData());

	}
	
	if(QSysInfo::kernelType() == "winnt")
	{
		QProcess wincpuinfo;
	{
		
		wincpuinfo.start("wmic",
				 QStringList() << "cpu" << "get" << "name");
		wincpuinfo.waitForFinished();
		QString windows_output = wincpuinfo.readAllStandardOutput().toUpper();
		compilation_info +=  "<br>"" CPU : "  
				+ QString(windows_output.toLocal8Bit().constData());
		
		QProcess wingpuinfo;
	{
		
		wingpuinfo.start("wmic",
				 QStringList()
				 << "PATH"
				 << "Win32_videocontroller"
				 << "get"
				 << "VideoProcessor ");
		wingpuinfo.waitForFinished();
		QString WinGPUOutput = wingpuinfo.readAllStandardOutput();
		compilation_info += "<br>" "GPU : "
				+ QString(WinGPUOutput.toLocal8Bit().constData()); 
			}
			
			QProcess wingpuraminfo;
	{
		
		wingpuraminfo.start("wmic",
				    QStringList()
				    << "PATH"
				    << "Win32_videocontroller"
				    << "get"
				    << "AdapterRAM ");
		wingpuraminfo.waitForFinished();
		QString WinGPURAMOutput = wingpuraminfo.readAllStandardOutput();
		compilation_info += "<br>" "GPU RAM : "
				+ QString(WinGPURAMOutput.toLocal8Bit().constData()); 
			}
		}
	}

#endif
#endif

	compilation_info += "<br>" "  OS : "
			+ QString(QSysInfo::kernelType());
	compilation_info += "  -   "
			+ QString(QSysInfo::currentCpuArchitecture());
	compilation_info += " -  Version :    "
			+ QString(QSysInfo::prettyProductName());
	compilation_info += "</br>" " -  Kernel :     "
			+ QString(QSysInfo::kernelVersion());
	compilation_info += "<br>  *** Qt screens *** </br>";
	const auto screens = qApp->screens();
	for (int ii = 0; ii < screens.count(); ++ii) {
		compilation_info += "<br> ( "
			+ QString::number(ii + 1)
			+ " : "
			+ QString::number(
				screens[ii]->geometry().width()
				* screens[ii]->devicePixelRatio())
			+ " x "
			+ QString::number(
				screens[ii]->geometry().height()
				* screens[ii]->devicePixelRatio())
			+ " ) </br>";
	}
	qDebug()<<compilation_info;
	return compilation_info;
}
