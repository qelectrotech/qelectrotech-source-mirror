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
#include "dimensionwidget.h"

/**
	Constructor
	@param complete True for this dialog to show the radio buttons that allow
	the user to specify whether the dimension is absolute, relative to the
	total width or relative to the remaining width.
	@param parent Parent QWidget
*/
TitleBlockDimensionWidget::TitleBlockDimensionWidget(bool complete, QWidget *parent) :
	QDialog(parent),
	complete_(complete),
	read_only_(false)
{
	initWidgets();
	initLayouts();
}

/**
	Destructor
*/
TitleBlockDimensionWidget::~TitleBlockDimensionWidget() {
}

/**
	@return true if this dialog shows the optional radio buttons
*/
bool TitleBlockDimensionWidget::isComplete() const {
	return(complete_);
}

/**
	@return a pointer to the label displayed right before the spinbox.
	Useful to specify a custom text.
*/
QLabel *TitleBlockDimensionWidget::label() const {
	return(spinbox_label_);
}

/**
	@return a pointer to the spinbox
	Useful to specify custom parameters, such as the minimum value
*/
QSpinBox *TitleBlockDimensionWidget::spinbox() const {
	return(spinbox_);
}

/**
	@return The dimension as currently shown by the dialog
*/
TitleBlockDimension TitleBlockDimensionWidget::value() const {
	QET::TitleBlockColumnLength type = QET::Absolute;
	if (complete_) {
		type = static_cast<QET::TitleBlockColumnLength>(dimension_type_ -> checkedId());
	}
	return(TitleBlockDimension(spinbox_ -> value(), type));
}

/**
	@param dim Dimension to be displayed and edited by this dialog
*/
void TitleBlockDimensionWidget::setValue(const TitleBlockDimension &dim) {
	if (complete_) {
		if (QAbstractButton *button = dimension_type_ -> button(dim.type)) {
			button -> setChecked(true);
		}
	}
	updateSpinBoxSuffix();
	spinbox_ -> setValue(dim.value);
}

/**
	@return Whether or not this widget should allow edition of the displayed
	dimension.
*/
bool TitleBlockDimensionWidget::isReadOnly() const {
	return(read_only_);
}

/**
	@param read_only Whether or not this widget should allow edition of the
	displayed dimension.
*/
void TitleBlockDimensionWidget::setReadOnly(bool read_only) {
	if (read_only_ == read_only) return;
	read_only_ = read_only;
	
	spinbox_ -> setReadOnly(read_only_);
	if (complete_) {
		absolute_button_  -> setEnabled(!read_only_);
		relative_button_  -> setEnabled(!read_only_);
		remaining_button_ -> setEnabled(!read_only_);
	}
}

/**
	Initialize the widgets composing the dialog.
*/
void TitleBlockDimensionWidget::initWidgets() {
	// basic widgets: label + spinbox
	spinbox_label_ = new QLabel(tr("Largeur :", "default dialog label"));
	
	spinbox_ = new QSpinBox();
	spinbox_ -> setValue(50);
	
	// extra widgets, for the user to specify whether the value is absolute, relative, etc.
	if (complete_) {
		absolute_button_  = new QRadioButton(tr("Absolu",             "a traditional, absolute measure"));
		relative_button_  = new QRadioButton(tr("Relatif au total",   "a percentage of the total width"));
		remaining_button_ = new QRadioButton(tr("Relatif au restant", "a percentage of what remains from the total width"));
		dimension_type_   = new QButtonGroup(this);
		dimension_type_ -> addButton(absolute_button_,  QET::Absolute);
		dimension_type_ -> addButton(relative_button_,  QET::RelativeToTotalLength);
		dimension_type_ -> addButton(remaining_button_, QET::RelativeToRemainingLength);
		absolute_button_ -> setChecked(true);
		connect(dimension_type_, SIGNAL(buttonClicked(int)), this, SLOT(updateSpinBoxSuffix()));
	}
	
	updateSpinBoxSuffix();
	
	// buttons, for the user to validate its input
	buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons_, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons_, SIGNAL(rejected()), this, SLOT(reject()));
}

/**
	Initialize the layout of the dialog.
*/
void TitleBlockDimensionWidget::initLayouts() {
	QHBoxLayout *hlayout0 = new QHBoxLayout();
	hlayout0 -> addWidget(spinbox_label_);
	hlayout0 -> addWidget(spinbox_);
	QVBoxLayout *vlayout0 = new QVBoxLayout();
	vlayout0 -> addLayout(hlayout0);
	if (complete_) {
		vlayout0 -> addWidget(absolute_button_);
		vlayout0 -> addWidget(relative_button_);
		vlayout0 -> addWidget(remaining_button_);
	}
	vlayout0 -> addWidget(buttons_);
	setLayout(vlayout0);
}

/**
	Ensure the suffix displayed by the spinbox matches the selected kind of length.
*/
void TitleBlockDimensionWidget::updateSpinBoxSuffix() {
	if (complete_ && dimension_type_ -> checkedId() != QET::Absolute) {
		spinbox_ -> setSuffix(tr("%", "spinbox suffix when changing the dimension of a row/column"));
		spinbox_ -> setMinimum(1);
		spinbox_ -> setMaximum(100);
	} else {
		spinbox_ -> setSuffix(tr("px", "spinbox suffix when changing the dimension of a row/column"));
		spinbox_ -> setMinimum(5);
		spinbox_ -> setMaximum(10000);
	}
	spinbox_ -> selectAll();
}
