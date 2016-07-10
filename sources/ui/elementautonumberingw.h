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
#ifndef ELEMENTAUTONUMBERINGW_H
#define ELEMENTAUTONUMBERINGW_H

#include <QWidget>

class QAbstractButton;

namespace Ui {
		class ElementAutonumberingW;
}

/**
	This class implements the element autonumbering widget.
	It loads the current formula applied to new elements and allows
	the user to overwrite it with a new formula. Formula is added
	while parsing label in customelement.cpp
*/
class ElementAutonumberingW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
		explicit ElementAutonumberingW(QWidget *parent = 0);
		~ElementAutonumberingW();
		QString formula();
		void setContext(QString);
		void clearContext();
		Ui::ElementAutonumberingW *ui;

	private:

	// SIGNALS
	signals:
		void applyPressed();
		void textChanged(QString);

	//SLOTS
	private slots:
		void on_m_formula_le_textChanged(QString);

	//ATTRIBUTES
	private:
		QString formula_;

};

#endif // ELEMENTAUTONUMBERINGW_H
