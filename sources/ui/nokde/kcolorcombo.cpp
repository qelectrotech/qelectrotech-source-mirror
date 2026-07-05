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

#include <QVariant>

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
		addItem(color.name(), color);
	}
}

QColor KColorCombo::color(int index) const
{
	if (index < 0 || index >= count()) {
		return {};
	}

	return itemData(index).value<QColor>();
}
