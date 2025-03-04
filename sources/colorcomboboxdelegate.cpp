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
#include "colorcomboboxdelegate.h"

#include <QColor>
#include <QPainter>
#include <QPalette>
#include <QtDebug>

/**
	@brief ColorComboBoxDelegate::paint
	Paints a filled rectangle on the drop down item with the items color.
	@param painter Painter context
	@param option Style options
	@param index Index of the item to paint

 */
void ColorComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// Padding for the color indicator
	const int padding_x = 5;
	const int padding_y = 2;

	painter->save();

	if (index.row() > 0)
	{
		auto rect = option.rect;

		// Draw mouseover background
		if (option.state & QStyle::State_MouseOver)
		{
			auto pal = option.widget->palette();
			painter->fillRect(rect, pal.color(QPalette::Highlight));
		}

		// Draw color indicator rectangle
		auto color = qvariant_cast<QColor>(index.data());
		rect.adjust(padding_x, padding_y, -padding_x, -padding_y);
		painter->fillRect(rect, color);
	} else {
		// Draw a normal drop down item for custom color
		QStyledItemDelegate::paint(painter, option, index);
	}

	painter->restore();
}
