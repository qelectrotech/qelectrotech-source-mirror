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
#include "lastusedstyle.h"

QPen  LastUsedStyle::m_shape_pen;
bool  LastUsedStyle::m_has_shape_pen = false;
QBrush LastUsedStyle::m_shape_brush;
bool  LastUsedStyle::m_has_shape_brush = false;
QFont LastUsedStyle::m_text_font;
bool  LastUsedStyle::m_has_text_font = false;

/**
	@return true if a shape pen was set this session
*/
bool LastUsedStyle::hasShapePen()
{
	return m_has_shape_pen;
}

/**
	@return the last pen applied to a shape this session
*/
QPen LastUsedStyle::shapePen()
{
	return m_shape_pen;
}

/**
	@brief LastUsedStyle::setShapePen
	Record @a pen as the last-used shape pen for this session
	@param pen
*/
void LastUsedStyle::setShapePen(const QPen &pen)
{
	m_shape_pen = pen;
	m_has_shape_pen = true;
}

/**
	@return true if a shape brush was set this session
*/
bool LastUsedStyle::hasShapeBrush()
{
	return m_has_shape_brush;
}

/**
	@return the last brush applied to a shape this session
*/
QBrush LastUsedStyle::shapeBrush()
{
	return m_shape_brush;
}

/**
	@brief LastUsedStyle::setShapeBrush
	Record @a brush as the last-used shape brush for this session
	@param brush
*/
void LastUsedStyle::setShapeBrush(const QBrush &brush)
{
	m_shape_brush = brush;
	m_has_shape_brush = true;
}

/**
	@return true if a text font was set this session
*/
bool LastUsedStyle::hasTextFont()
{
	return m_has_text_font;
}

/**
	@return the last font applied to a free text item this session
*/
QFont LastUsedStyle::textFont()
{
	return m_text_font;
}

/**
	@brief LastUsedStyle::setTextFont
	Record @a font as the last-used free text font for this session
	@param font
*/
void LastUsedStyle::setTextFont(const QFont &font)
{
	m_text_font = font;
	m_has_text_font = true;
}
