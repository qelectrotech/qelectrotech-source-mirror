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
#include "colorbutton.h"

#include <QColorDialog>
#include <QPainter>

/**
	@brief ColorButton::ColorButton
	Simple constructor
	@param parent QObject parent of the ColorButton
*/
ColorButton::ColorButton(QWidget *parent) : QPushButton(parent)
{
	connect(this, &QPushButton::clicked, this, &ColorButton::clicked);
}

/**
	@brief Getter for current color
	@return The current selected color
*/
const QColor ColorButton::color()
{
	return m_color;
}

/**
	@brief Setter for current color
*/
void ColorButton::setColor(const QColor &color)
{
    m_color = color;
	update();
}

/**
	@brief ColorButton::clicked
	Opens a color selection dialog and lets the user select a color.
	@param checked Not used
*/
void ColorButton::clicked(bool checked)
{
	// Open color selection dialog
	auto new_color = QColorDialog::getColor(m_color, this, tr("Select color"), QColorDialog::DontUseNativeDialog);

	// Validate user input
	if (new_color.isValid()) {
		m_color = new_color;
		emit changed(new_color);
		update();
	}
}

/**
	@brief ColorButton::paintEvent
	Paints a filled rectangle with the current selected color on the button surface.
	@param e Paint event context
*/
void ColorButton::paintEvent(QPaintEvent *e) {
	QPushButton::paintEvent(e);
	QPainter painter(this);

	// Get dimensions of the button paint surface
	auto r_width = painter.device()->width();
	auto r_height = painter.device()->height();

	// Paint a rectangle with a margin of 5
	auto color_indicator = QRect(5, 5, r_width - 10, r_height - 10);
	painter.fillRect(color_indicator, m_color);
}
