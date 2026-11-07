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
#include <QStyleOptionButton>
#include <QStylePainter>

#include <cmath>

namespace {
QColor fallbackColor()
{
	return QPalette{}.color(QPalette::Button);
}

	//The relative luminance of WCAG 2: 0 for black, 1 for white
qreal relativeLuminance(const QColor &color)
{
	const auto linear = [](qreal channel) {
		return channel <= 0.04045
			? channel / 12.92
			: std::pow((channel + 0.055) / 1.055, 2.4);
	};
	return 0.2126 * linear(color.redF())
		+ 0.7152 * linear(color.greenF())
		+ 0.0722 * linear(color.blueF());
}

	//Black or white, whichever contrasts more with @p background
QColor readableTextColor(const QColor &background)
{
		//Where black and white contrast equally: (L + 0.05)^2 = 0.0525
	return relativeLuminance(background) > 0.1791
		? QColor(Qt::black)
		: QColor(Qt::white);
}

	//@p from, moved by @p amount (0 to 1) towards @p to
QColor mixedColor(const QColor &from, const QColor &to, qreal amount)
{
	return QColor::fromRgbF(
		from.redF() + (to.redF() - from.redF()) * amount,
		from.greenF() + (to.greenF() - from.greenF()) * amount,
		from.blueF() + (to.blueF() - from.blueF()) * amount);
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
	const auto selected = QColorDialog::getColor(m_color, this, QString(),
											 QColorDialog::DontUseNativeDialog);
	if (!selected.isValid() || selected == m_color) {
		return;
	}

	m_color = selected;
	updateButton();
	emit changed(m_color);
}

void KColorButton::updateButton()
{
		//The text gives the button its size and its name for screen
		//readers; paintEvent() draws it on the colour
	setText(m_color.name());
	update();
}

/**
	@brief KColorButton::paintEvent
	Draws the button of the style, then the colour where its text would
	be, with the name of the colour in black or white on it. A style draws
	a button in its own colours, whatever the palette says, so the colour
	is painted here, the same in every style.
*/
void KColorButton::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	QStyleOptionButton option;
	initStyleOption(&option);
	option.text.clear();
	painter.drawControl(QStyle::CE_PushButton, option);

		//Inside the focus rectangle too: some styles draw the keyboard
		//focus within the button, where the colour would hide it
	const QRect swatch = style()->subElementRect(
		QStyle::SE_PushButtonContents, &option, this)
		.adjusted(1, 1, -1, -1)
		.intersected(style()->subElementRect(
			QStyle::SE_PushButtonFocusRect, &option, this)
			.adjusted(1, 1, -1, -1));
	const QColor fill = isEnabled()
		? m_color
		: mixedColor(m_color,
			palette().color(QPalette::Disabled, QPalette::Button), 0.6);
	const QColor ink = readableTextColor(fill);
	painter.fillRect(swatch, fill);

		//A faint edge, so a colour the button has itself still shows
	QColor edge = ink;
	edge.setAlphaF(0.3);
	QPen pen(edge, 0);
	painter.setPen(pen);
	painter.drawRect(swatch.adjusted(0, 0, -1, -1));

	painter.setPen(isEnabled() ? ink : mixedColor(ink, fill, 0.5));
	painter.drawText(swatch, Qt::AlignCenter,
		fontMetrics().elidedText(text(), Qt::ElideRight,
			swatch.width() - 4));
}
