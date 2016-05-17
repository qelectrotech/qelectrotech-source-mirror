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
#ifndef NUMPARTEDITORW_H
#define NUMPARTEDITORW_H

#include <QWidget>
#include <QValidator>
#include "numerotationcontext.h"

/**
 *This class represent a single part num widget. By this widget, we can define and edit
 *how the num auto must work .
 *This widget is called by selectautonumw.
 */
namespace Ui {
	class NumPartEditorW;
}

class NumPartEditorW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
	explicit NumPartEditorW(QWidget *parent = 0);
	NumPartEditorW (NumerotationContext &, int, QWidget *parent=0);
	~NumPartEditorW();

	enum type {unit,ten,hundred,string,idfolio,folio};
	NumerotationContext toNumContext();
	bool isValid ();


	private slots:
	void on_type_combo_activated(int);
	void on_value_field_textEdited();
	void on_increase_spinBox_valueChanged();
	void setType (NumPartEditorW::type t, bool=false);

	signals:
	void changed ();
	
	private:
	Ui::NumPartEditorW *ui;
	QValidator *intValidator;
	type type_;

};

#endif // NUMPARTEDITORW_H
