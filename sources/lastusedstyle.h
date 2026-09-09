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
#ifndef LAST_USED_STYLE_H
#define LAST_USED_STYLE_H

#include <QBrush>
#include <QFont>
#include <QPen>

/**
	@brief The LastUsedStyle class
	Session-scoped "last used" style for new shapes and free text created
	on the diagram canvas: whatever pen/brush/font was last applied through
	the properties editors becomes the starting point for the next new
	item of that type, the way most drawing tools behave.

	Deliberately in-memory only, not QSettings-backed: this is a live
	"what did I just use" value for the current editing session, not an
	app-wide default (that's already covered by the Preferences dialog's
	font setting, read as the fallback when nothing has been set yet).
*/
class LastUsedStyle
{
	public:
		static bool hasShapePen();
		static QPen shapePen();
		static void setShapePen(const QPen &pen);

		static bool hasShapeBrush();
		static QBrush shapeBrush();
		static void setShapeBrush(const QBrush &brush);

		static bool hasTextFont();
		static QFont textFont();
		static void setTextFont(const QFont &font);

	private:
		LastUsedStyle() = delete;

		static QPen m_shape_pen;
		static bool m_has_shape_pen;
		static QBrush m_shape_brush;
		static bool m_has_shape_brush;
		static QFont m_text_font;
		static bool m_has_text_font;
};

#endif // LAST_USED_STYLE_H
