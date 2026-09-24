/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "elementpreviewdelegate.h"

#include <QWidget>

#include "../qetpalette.h"

/**
	@brief ElementPreviewDelegate::initStyleOption
	After the base class has filled the option from the model, replace
	the icon with one that reads on a dark palette. The adapted icon is
	built from the pixmap the view is about to draw, at the view's
	decoration size and device pixel ratio.
*/
void ElementPreviewDelegate::initStyleOption(QStyleOptionViewItem *option,
                                             const QModelIndex &index) const
{
	QStyledItemDelegate::initStyleOption(option, index);
	if (option->icon.isNull() || !QET::Palette::isDark(option->palette))
		return;

	const qint64 key = option->icon.cacheKey();
	const auto it = m_dark_icons.constFind(key);
	if (it != m_dark_icons.constEnd())
	{
		option->icon = *it;
		return;
	}

	const qreal dpr = option->widget ? option->widget->devicePixelRatio() : 1.0;
	const QPixmap source = option->icon.pixmap(option->decorationSize, dpr);
	const QPixmap adapted = QET::Palette::forPalette(source, option->palette);
	QIcon icon = option->icon;
	if (adapted.cacheKey() != source.cacheKey())
		icon = QIcon(adapted);
	m_dark_icons.insert(key, icon);
	option->icon = icon;
}
