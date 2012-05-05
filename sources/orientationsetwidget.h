/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef ORIENTATION_SET_WIDGET_H
#define ORIENTATION_SET_WIDGET_H
#include <QtGui>
#include "orientationset.h"
/**
	Widget permettant d'editer un OrientationSet
*/
class OrientationSetWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	OrientationSetWidget(QWidget * = 0);
	virtual ~OrientationSetWidget() {};
	
	private:
	OrientationSetWidget(const OrientationSetWidget &);
	
	// attributs
	private:
	OrientationSet ori;
	QComboBox *north_orientation;
	QComboBox *east_orientation;
	QComboBox *south_orientation;
	QComboBox *west_orientation;
	QRadioButton *north_default;
	QRadioButton *east_default;
	QRadioButton *south_default;
	QRadioButton *west_default;
	QButtonGroup *default_radios;
	
	// methodes
	public:
	OrientationSet orientationSet() const;
	void setOrientationSet(const OrientationSet &);
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	private:
	void updateForm();
	
	public slots:
	void slot_defaultChanged(QAbstractButton *);
	void updateOrientationSet();
};
#endif
