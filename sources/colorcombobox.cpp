/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "colorcombobox.h"

#include "colorcomboboxdelegate.h"

#include <QColorDialog>
#include <QPainter>
#include <QStylePainter>
#include <Qt>


/**
	@brief ColorComboBox::ColorComboBox
	Simple constructor
	@param parent QObject parent of the ColorComboBox
*/
ColorComboBox::ColorComboBox(QWidget *parent) : QComboBox(parent)
{
	connect(this, &QComboBox::activated, this, &ColorComboBox::colorSelected);
	setItemDelegate(new ColorComboBoxDelegate());
}

/**
	@brief ColorComboBox::setColors
	Sets the colors of the combo box. An item at the top will be added to allow selection
	of a custom color.
	@param colors Vector of colors to add to the combo box
*/
void ColorComboBox::setColors(const QVector<QColor> &colors)
{
	addItem(tr("[Custom color...]"), QColor(Qt::black));
	for(auto &color : colors) {
		addItem(color.name(), color);
	}
}

/**
	@brief ColorComboBox::colorSelected
	Opens a color selection dialog and lets the user select a color.
	@param checked Not used
*/
void ColorComboBox::colorSelected(int index)
{
	if (index == 0) {
		// Open color selection dialog if custom color is selected
		auto new_color = QColorDialog::getColor(itemData(index).value<QColor>(), this, tr("Select color"), QColorDialog::DontUseNativeDialog);

		// Validate and emit user input color
		if (new_color.isValid()) {
			setItemData(index, new_color);
			emit activated(new_color);
		}
	} else {
		// Emit color from selected combo box row
		emit activated(itemData(index).value<QColor>());
	}
}

/**
	@brief ColorComboBox::paintEvent
	Paints a filled rectangle with the current selected color on the combo box surface.
	@param e Paint event context
*/
void ColorComboBox::paintEvent(QPaintEvent *e) {
	// Padding for the color indicator
	const int padding_x = 5;
	const int padding_y = 5;

	// Create painter and draw a vanilla combobox
	QStylePainter painter(this);
	QStyleOptionComboBox opt;
    initStyleOption(&opt);
	painter.drawComplexControl(QStyle::CC_ComboBox, opt);
    
	// Get dimensions of the combo box paint surface
	auto r_width = painter.device()->width();
	auto r_height = painter.device()->height();

	// Paint a rectangle with a margin
	auto color_indicator = QRect(padding_x, padding_y, r_width - padding_x * 2, r_height - padding_y * 2);
	painter.fillRect(color_indicator, itemData(currentIndex()).value<QColor>());
}
