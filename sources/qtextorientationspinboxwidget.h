/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef Q_TEXT_ORIENTATION_SPINBOX_WIDGET_H
#define Q_TEXT_ORIENTATION_SPINBOX_WIDGET_H
#include <QtGui>
#include "qtextorientationwidget.h"
/**
	This class provides a widget grouping a QTextOrientationWidget and QSpinBox
	next to each other.
	@see QTextOrientationWidget
*/
class QTextOrientationSpinBoxWidget : public QWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	QTextOrientationSpinBoxWidget(QWidget * = 0);
	virtual ~QTextOrientationSpinBoxWidget();
	private:
	QTextOrientationSpinBoxWidget(const QTextOrientationSpinBoxWidget &);
	
	signals:
	/**
		Signals emitted when users have finished editing the orientation.
	*/
	void editingFinished(double);
	void editingFinished();
	
	// methods
	public:
	QTextOrientationWidget *orientationWidget() const;
	QDoubleSpinBox *spinBox() const;
	double orientation() const;
	double value() const;
	bool isReadOnly() const;
	
	public slots:
	void setOrientation(const double &);
	void setValue(const double &);
	void setReadOnly(bool);
	
	private:
	void build();
	
	private slots:
	void emitChangeSignals();
	
	// attributes
	private:
	QTextOrientationWidget *orientation_widget_;
	QDoubleSpinBox *spin_box_;
};
#endif
