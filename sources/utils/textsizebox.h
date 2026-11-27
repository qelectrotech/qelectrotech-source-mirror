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
#ifndef TEXTSIZEBOX_H
#define TEXTSIZEBOX_H

#include <QtGlobal>

class QDoubleSpinBox;
class QFont;
class QETProject;

/**
	@brief TextSizeBox
	The text-size field of a property widget. Fonts are stored in points;
	in a project drawn to scale (millimetres) the field shows and takes
	the capital height h in mm (ISO 3098, see QetUnits::pointsToTextMm()).
	Anywhere else it shows whole points, as it always did. The functions
	work on a QDoubleSpinBox, with \p px_per_mm = the project's pixels per
	millimetre (QETProject::pxPerMm()) when it is drawn to scale, 0 for points.
*/
namespace TextSizeBox {
		/// The pixels per millimetre when text sizes of \p project are
		/// given in mm, else 0
	qreal pxPerMm(const QETProject *project);
		/// Decimals, suffix and range of \p sb for \p mm, from the points range
	void configure(QDoubleSpinBox *sb, qreal px_per_mm, int min_pt,
				   int max_pt, const QFont &font);
		/// Show \p pt of \p font in \p sb
	void setPoints(QDoubleSpinBox *sb, qreal px_per_mm, qreal pt,
				   const QFont &font);
		/// What \p sb shows for a text of \p pt points, as a number to compare
	qreal shown(qreal px_per_mm, qreal pt, const QFont &font);
		/// Whether \p sb still shows \p pt points of \p font
	bool isUnchanged(const QDoubleSpinBox *sb, qreal px_per_mm, qreal pt,
					 const QFont &font);
		/// The point size \p sb stands for, 0.1 pt in mm
	qreal points(const QDoubleSpinBox *sb, qreal px_per_mm,
				 const QFont &font);
}

#endif
