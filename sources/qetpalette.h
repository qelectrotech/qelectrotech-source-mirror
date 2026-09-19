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
#ifndef QET_PALETTE_H
#define QET_PALETTE_H

#include <QImage>
#include <QPalette>
#include <QPixmap>

class QStyle;

/**
	Palettes tuned for the Fusion style.

	main.cpp forces the Fusion style on macOS, but the palette still comes
	from Qt's macOS platform theme, which is built for the native style.
	That palette gives Window, Button and Base the same color, makes Dark
	lighter than Light, and in dark mode sets the Inactive ButtonText to
	black. Fusion derives its outlines, gradients and indicator colors from
	those roles, so under that palette controls lose their edges, radio
	indicators vanish, and combo box text turns black as soon as the window
	loses focus.

	This namespace provides a light and a dark palette with the role
	separation Fusion expects, and the WCAG 2 contrast helper the tests use
	to keep them readable.
*/
namespace QET {
	namespace Palette {
		/**
			@return true when the style draws with Fusion's rules,
			i.e. its object name is "fusion".
		*/
		bool styleIsFusion(const QStyle *style);

		/**
			@return true when the palette's Window color is dark
			(lightness below 128).
		*/
		bool isDark(const QPalette &palette);

		/**
			WCAG 2 contrast ratio between two opaque colors, from 1 (equal)
			to 21 (black on white). Normal text needs at least 4.5, large
			text and non-text controls at least 3.
		*/
		double contrastRatio(const QColor &a, const QColor &b);

		/**
			A light palette with the role separation Fusion expects: Window
			and Button a light grey, Base white, Text black, and Inactive
			equal to Active so nothing changes when the window loses focus.
		*/
		QPalette fusionLight();

		/**
			A dark palette with the same role separation: Window a mid
			grey, Button lighter than Window, Base darker than Window, Text
			light, and Inactive equal to Active.
		*/
		QPalette fusionDark();

		/**
			Copy the Highlight and HighlightedText colors of the platform
			palette into ours, so the user's accent color survives, but only
			when the pair reads at 4.5:1 or better. Otherwise ours is kept.
		*/
		QPalette withPlatformAccent(QPalette palette, const QPalette &platform);

		/**
			The palette QET should use under Fusion, given the palette the
			platform handed to the application: fusionDark() when the
			platform palette is dark, fusionLight() otherwise, with the
			platform's accent color kept when it is readable.
		*/
		QPalette forFusion(const QPalette &platform);

		/**
			True when fewer than a fifth of the visible pixels are
			saturated: black or gray line art, which is what element
			previews and most of QET's own icons are.
		*/
		bool isLineArt(const QImage &image);

		/**
			The image with its lightness inverted and hue, saturation and
			alpha kept: the darkest ink becomes light gray (220), white
			becomes black. The rule misc/make_icon_themes.py applies when
			it builds the dark icon theme.
		*/
		QImage invertedLightness(const QImage &image);

		/**
			A picture drawn for a white sheet, made to read on palette:
			returned as is on a light palette, and with its lightness
			inverted on a dark one when it is line art. Colored art is
			left alone either way.
		*/
		QPixmap forPalette(const QPixmap &pixmap, const QPalette &palette);
	}
}

#endif
