// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "conductorcolorcodes.h"

#include "conductorproperties.h"

#include <QCoreApplication>
#include <QPainter>
#include <QPixmap>

namespace {
		//The stripe length a two-colour wire gets when it has none of its
		//own: the length the example projects draw their earth wires with
	const int TWO_COLOUR_DASH_SIZE = 10;
}

/**
	@brief ConductorColorCodes::all
	@return every code, in the order an electrician reaches for them: the
	phases, neutral, earth, then the colours used for control and
	extra-low-voltage circuits, then the rest of IEC 60757. The nine
	colours the toolbar already offered keep their drawn colour.
*/
const QList<ConductorColorCodes::Code> &ConductorColorCodes::all()
{
	static const QList<Code> codes {
		{QStringLiteral("BK"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Black"),        QColor(0x00, 0x00, 0x00), QColor()},
		{QStringLiteral("BN"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Brown"),        QColor(0x7B, 0x3F, 0x00), QColor()},
		{QStringLiteral("GY"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Gray"),         QColor(0x80, 0x80, 0x80), QColor()},
		{QStringLiteral("BU"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Blue"),         QColor(0x00, 0x00, 0xFF), QColor()},
		{QStringLiteral("GNYE"), QT_TRANSLATE_NOOP("ConductorColorToolButton", "Green-yellow"), QColor(0x00, 0x80, 0x00), QColor(0xFF, 0xD8, 0x00)},
		{QStringLiteral("GN"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Green"),        QColor(0x00, 0x80, 0x00), QColor()},
		{QStringLiteral("RD"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Red"),          QColor(0xFF, 0x00, 0x00), QColor()},
		{QStringLiteral("OG"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Orange"),       QColor(0xFF, 0x80, 0x00), QColor()},
		{QStringLiteral("YE"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Yellow"),       QColor(0xFF, 0xD8, 0x00), QColor()},
		{QStringLiteral("VT"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Purple"),       QColor(0x80, 0x00, 0x80), QColor()},
		{QStringLiteral("WH"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "White"),        QColor(0xFF, 0xFF, 0xFF), QColor()},
		{QStringLiteral("PK"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Pink"),         QColor(0xFF, 0x80, 0xC0), QColor()},
		{QStringLiteral("TQ"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Turquoise"),    QColor(0x40, 0xE0, 0xD0), QColor()},
		{QStringLiteral("GD"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Gold"),         QColor(0xD4, 0xA0, 0x17), QColor()},
		{QStringLiteral("SR"),   QT_TRANSLATE_NOOP("ConductorColorToolButton", "Silver"),       QColor(0xC0, 0xC0, 0xC0), QColor()},
	};
	return codes;
}

/**
	@brief ConductorColorCodes::find
	@param text : a wire's colour text
	@return the code \p text is, ignoring case and surrounding spaces, or
	nullptr when it is free text
*/
const ConductorColorCodes::Code *ConductorColorCodes::find(const QString &text)
{
	const QString wanted = text.trimmed();
	for (const Code &code : all()) {
		if (code.code.compare(wanted, Qt::CaseInsensitive) == 0) {
			return &code;
		}
	}
	return nullptr;
}

/**
	@brief ConductorColorCodes::displayName
	@param code
	@return the code and its translated name, as a menu shows it:
	"GNYE – Green-yellow"
*/
QString ConductorColorCodes::displayName(const Code &code)
{
	return QStringLiteral("%1 – %2").arg(
				code.code,
				QCoreApplication::translate("ConductorColorToolButton", code.name));
}

/**
	@brief ConductorColorCodes::swatch
	@param code
	@return a square of the code's colour, its lower right half in the
	second colour for a two-colour code, outlined so that white is still
	visible against a light menu.
*/
QIcon ConductorColorCodes::swatch(const Code &code)
{
	QPixmap pix(16, 16);
	pix.fill(Qt::transparent);
	QPainter p(&pix);
	p.setRenderHint(QPainter::Antialiasing, false);
	p.fillRect(0, 0, 16, 16, code.color);
	if (code.isTwoColour()) {
		const QPolygon half({QPoint(16, 0), QPoint(16, 16), QPoint(0, 16)});
		p.setPen(Qt::NoPen);
		p.setBrush(code.color_2);
		p.drawPolygon(half);
	}
	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(QColor(0x40, 0x40, 0x40), 1));
	p.drawRect(0, 0, 15, 15);
	p.end();
	return QIcon(pix);
}

/**
	@brief ConductorColorCodes::apply
	Give \p properties the code: the colour text and how the wire is
	drawn, striped for a two-colour code and plain otherwise. A core of a
	cable keeps the colour text its cable gives it.
	@param code
	@param properties
*/
void ConductorColorCodes::apply(const Code &code, ConductorProperties &properties)
{
	properties.color = code.color;
	properties.m_bicolor = code.isTwoColour();
	if (code.isTwoColour())
	{
		properties.m_color_2 = code.color_2;
			//The default stripe (1) is a dotted line, not stripes
		if (properties.m_dash_size < 2) {
			properties.m_dash_size = TWO_COLOUR_DASH_SIZE;
		}
	}
	if (properties.m_cable_uuid.isNull()) {
		properties.m_wire_color = code.code;
	}
}
