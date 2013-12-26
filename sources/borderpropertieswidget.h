/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef BORDER_PROPERTIES_WIDGET_H
#define BORDER_PROPERTIES_WIDGET_H
#include <QWidget>
#include "borderproperties.h"
class QCheckBox;
class QSpinBox;
class QPushButton;
/**
	This class provides a widget to edit dimensions and display properties of a
	diagram, title block excluded.
	@see TitleBlockPropertiesWidget
*/
class BorderPropertiesWidget : public QWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	BorderPropertiesWidget(const BorderProperties &, QWidget * = 0);
	virtual ~BorderPropertiesWidget();
	private:
	BorderPropertiesWidget(const BorderPropertiesWidget &);
	
	// methods
	public:
	const BorderProperties &borderProperties();
	bool isReadOnly() const;
	void setReadOnly(bool);
	void setEditedBorder(const BorderProperties &);

	public slots:
	// to choose the back_ground color of diagram.
	void chooseColor();
	
	private:
	void build();
	
	// attributes
	private:
	BorderProperties border_;       ///< Edited properties
	QSpinBox *columns_count;        ///< Widget to edit the columns count
	QSpinBox *columns_width;        ///< Widget to edit the columns width
	QCheckBox *display_columns;     ///< Checkbox stating whether to display column headers
	QSpinBox *rows_count;           ///< Widget to edit the rows count
	QSpinBox *rows_height;          ///< Widget to edit the rows height
	QCheckBox *display_rows;        ///< Checkbox stating whether to display row headers
	QPushButton *pb_background_color; ///< Push button for selecting diagram background color
};
#endif
