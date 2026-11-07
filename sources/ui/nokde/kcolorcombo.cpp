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
#include "kcolorcombo.h"

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QVariant>

namespace {

	//A square of @p color, @p size large, drawn for normal and for
	//high-resolution screens. A faint edge, dark on a light colour and
	//light on a dark one, keeps white on white and black on black visible.
QIcon swatchIcon(const QColor &color, const QSize &size)
{
	QColor edge = color.lightnessF() > 0.5
		? QColor(Qt::black)
		: QColor(Qt::white);
	edge.setAlphaF(0.35);

	QIcon icon;
	for (const int scale : {1, 2}) {
		QPixmap pixmap(size * scale);
		pixmap.fill(color);
		QPainter painter(&pixmap);
		painter.setPen(QPen(edge, 1));
		painter.drawRect(
			QRectF(pixmap.rect()).adjusted(0.5, 0.5, -0.5, -0.5));
		painter.end();
		pixmap.setDevicePixelRatio(scale);
			//The same square when the colour is pointed at or
			//chosen: styles would tint it in the selection colour
		for (const auto mode : {QIcon::Normal, QIcon::Active,
				QIcon::Selected}) {
			icon.addPixmap(pixmap, mode);
		}
	}
	return icon;
}

}

KColorCombo::KColorCombo(QWidget *parent) :
	QComboBox{parent}
{
	connect(
		this,
		QOverload<int>::of(&QComboBox::activated),
		this,
		[this](int index) {
			emit activated(itemData(index).value<QColor>());
		});
}

void KColorCombo::setColors(const QList<QColor> &colors)
{
	clear();
	for (const auto &color : colors) {
		addItem(swatchIcon(color, iconSize()), color.name(), color);
	}
}

QColor KColorCombo::color(int index) const
{
	if (index < 0 || index >= count()) {
		return {};
	}

	return itemData(index).value<QColor>();
}

QColor KColorCombo::color() const
{
	return color(currentIndex());
}

void KColorCombo::setColor(const QColor &color)
{
	for (int i = 0; i < count(); ++i) {
		if (itemData(i).value<QColor>() == color) {
			setCurrentIndex(i);
			return;
		}
	}
	addItem(swatchIcon(color, iconSize()), color.name(), color);
	setCurrentIndex(count() - 1);
}
