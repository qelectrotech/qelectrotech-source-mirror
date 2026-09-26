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
#include "kcolorbutton.h"

#include <QColorDialog>
#include <QPalette>

namespace {
QColor fallbackColor()
{
	return QPalette{}.color(QPalette::Button);
}
}

KColorButton::KColorButton(QWidget *parent) :
	QPushButton{parent},
	m_color{fallbackColor()}
{
	connect(this, &QPushButton::clicked, this, &KColorButton::chooseColor);
	updateButton();
}

QColor KColorButton::color() const
{
	return m_color;
}

void KColorButton::setColor(const QColor &color)
{
	m_color = color.isValid() ? color : fallbackColor();
	updateButton();
}

void KColorButton::chooseColor()
{
	const auto selected = QColorDialog::getColor(m_color, this);
	if (!selected.isValid() || selected == m_color) {
		return;
	}

	m_color = selected;
	updateButton();
	emit changed(m_color);
}

void KColorButton::updateButton()
{
	setText(m_color.name());

	auto pal = palette();
	pal.setColor(QPalette::Button, m_color);
	setAutoFillBackground(true);
	setPalette(pal);
	update();
}
