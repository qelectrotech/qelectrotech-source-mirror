/*
	Copyright 2006-2015 The QElectroTech Team
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
#include <QRegExp>
#include "numparteditorw.h"
#include "ui_numparteditorw.h"

/**
 * Constructor
 */
NumPartEditorW::NumPartEditorW(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::NumPartEditorW),
	intValidator (new QIntValidator(0,99999,this))
{
	ui -> setupUi(this);
	setType(NumPartEditorW::unit, true);
}

/**
 * Constructor
 * Build with value of @context at position i
 */
NumPartEditorW::NumPartEditorW (NumerotationContext &context, int i, QWidget *parent):
	QWidget(parent),
	ui(new Ui::NumPartEditorW),
	intValidator (new QIntValidator(0,99999,this))
{
	ui -> setupUi(this);

	//if @context contains nothing build with default value
	if(context.size()==0) setType(NumPartEditorW::unit, true);

	else {
		QStringList strl = context.itemAt(i);
		if (strl.at(0)=="unit") setType(NumPartEditorW::unit, true);
		else if (strl.at(0)=="ten") setType(NumPartEditorW::ten, true);
		else if (strl.at(0)=="hundred") setType(NumPartEditorW::hundred, true);
		else if (strl.at(0)=="string") setType(NumPartEditorW::string);
		else if (strl.at(0)== "folio") setType(NumPartEditorW::folio);
		ui -> value_field -> setText(strl.at(1));
		ui -> increase_spinBox -> setValue(strl.at(2).toInt());
	}
}

/**
 * Destructor
 */
NumPartEditorW::~NumPartEditorW()
{
	delete intValidator;
	delete ui;
}

/**
 * @brief NumPartEditorW::toNumContext
 * @return the display to NumerotationContext
 */
NumerotationContext NumPartEditorW::toNumContext() {
	NumerotationContext nc;
	QString type_str;
	switch (type_) {
		case unit:
			type_str = "unit";
			break;
		case ten:
			type_str = "ten";
			break;
		case hundred:
			type_str = "hundred";
			break;
		case string:
			type_str = "string";
			break;
		case folio:
			type_str = "folio";
			break;
	}
	nc.addValue(type_str, ui -> value_field -> displayText(), ui -> increase_spinBox -> value());
	return nc;
}

/**
 * @brief NumPartEditorW::isValid
 * @return true if value field isn't empty or if type is folio
 */
bool NumPartEditorW::isValid() {
	if (type_ != folio && ui -> value_field -> text().isEmpty()) return false;
	return true;
}

/**
 * @brief NumPartEditorW::on_type_combo_activated
 * Action when user change the type comboBox
 */
void NumPartEditorW::on_type_combo_activated(int index) {
	switch (index) {
		case unit:
			setType(unit);
			break;
		case ten:
			setType(ten);
			break;
		case hundred:
			setType(hundred);
			break;
		case string:
			setType(string);
			break;
		case folio:
			setType(folio);
			break;
	};
	emit changed();
}

/**
 * @brief NumPartEditorW::on_value_field_textChanged
 * emit changed when @value_field text changed
 */
void NumPartEditorW::on_value_field_textEdited() {
	emit changed();
}

/**
 * @brief NumPartEditorW::on_increase_spinBox_valueChanged
 * emit changed when @increase_spinBox value changed
 */
void NumPartEditorW::on_increase_spinBox_valueChanged() {
	if (!ui -> value_field -> text().isEmpty()) emit changed();
}

/**
 * @brief NumPartEditorW::setType
 * Set good behavior by type @t
 * @param t, type used
 * @param fnum, force the behavior of numeric type
 */
void NumPartEditorW::setType(NumPartEditorW::type t, bool fnum) {
	ui -> type_combo -> setCurrentIndex(t);

	//if @t is a numeric type and preview type @type_ isn't a numeric type
	//or @fnum is true, we set numeric behavior
	if ( ((t==unit || t==ten || t==hundred) && (type_==string || type_==folio)) || fnum) {
		ui -> value_field -> clear();
		ui -> value_field -> setEnabled(true);
		ui -> value_field -> setValidator(intValidator);
		ui -> increase_spinBox -> setEnabled(true);
		ui -> increase_spinBox -> setValue(1);
	}
	//@t isn't a numeric type
	else if (t==string || t==folio) {
		ui -> value_field -> clear();
		ui -> increase_spinBox -> setDisabled(true);
		if (t==string) {
			ui -> value_field -> setValidator(0);
			ui -> value_field -> setEnabled(true);
		}
		else if (t==folio) {
			ui -> value_field -> setDisabled(true);
			ui -> increase_spinBox -> setDisabled(true);
		}
	}
	type_= t;
}
