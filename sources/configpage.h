/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef CONFIGPAGE_H
#define CONFIGPAGE_H

#include <QWidget>

/**
	This abstract class specifies methods all derived classes should
	implement.
*/
class ConfigPage : public QWidget {
	Q_OBJECT
	public:
	/**
		Constructor
		@param parent Parent QWidget
	*/
	ConfigPage(QWidget *parent) : QWidget(parent) {};
	/// Destructor
	virtual ~ConfigPage() {};
	/// Apply the configuration after user input
	virtual void applyConf() = 0;
	/// @return the configuration page title
	virtual QString title() const = 0;
	/// @return the configuration page icon
	virtual QIcon icon() const = 0;
};

#endif
