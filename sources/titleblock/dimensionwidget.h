/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_DIMENSION_WIDGET_H
#define TITLEBLOCK_SLASH_DIMENSION_WIDGET_H
#include <QtWidgets>
#include "dimension.h"

/**
	This class represents a dialog for the user to input a dimension: a row
	height, a column width, etc.
*/
class TitleBlockDimensionWidget : public QDialog {
	Q_OBJECT
	
	// constructors, destructor
	public:
	TitleBlockDimensionWidget(bool, QWidget * parent = 0);
	virtual ~TitleBlockDimensionWidget();
	private:
	TitleBlockDimensionWidget(const TitleBlockDimensionWidget &);
	
	// methods
	public:
	bool isComplete() const;
	QLabel *label() const;
	QSpinBox *spinbox() const;
	TitleBlockDimension value() const;
	void setValue(const TitleBlockDimension &);
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	private:
	void initWidgets();
	void initLayouts();
	
	private slots:
	void updateSpinBoxSuffix();
	
	// attributes
	private:
	bool complete_;                    ///< Whether or not this dialog is required to be complete, i.e. displaying also
	QSpinBox *spinbox_;                ///< Spinbox displaying the length
	QLabel *spinbox_label_;            ///< Label shown right before the spinbox
	QRadioButton *absolute_button_;    ///< Radio button to indicate the length is absolute
	QRadioButton *relative_button_;    ///< Radio button to indicate the length is relative to the total length
	QRadioButton *remaining_button_;   ///< Radio button to indicate the length is relative to the remaining length
	QButtonGroup *dimension_type_;     ///< QButtonGroup for the three radio buttons
	QDialogButtonBox *buttons_;        ///< Buttons to validate the dialog
	bool read_only_;                   ///< Whether or not this widget allow edition of the displayed dimension
};
#endif
