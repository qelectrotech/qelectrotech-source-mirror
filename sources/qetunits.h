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
#ifndef QETUNITS_H
#define QETUNITS_H

#include <QtGlobal>

class QFont;

/**
	@brief QetUnits
	What a scene pixel is on paper when a project is drawn to scale: the
	10 px grid step is the IEC 61082-1 module M (2.5 mm recommended, the
	project's choice), so 10 / M pixels per millimetre. Nothing here
	assumes a module: every conversion takes the pixels per millimetre of
	the project (QETProject::pxPerMm()). See docs/mm-scale.md.
*/
namespace QetUnits {
		/// The module M of IEC 61082-1 a project has unless it says
		/// otherwise, in millimetres
	constexpr qreal defaultModuleMm = 2.5;
		/// The pixels per millimetre of the default module: 10 px grid
		/// step = 2.5 mm = M. For a stored setting of its own (the
		/// realistic terminal strip), never for a project drawn to scale.
	constexpr qreal defaultPxPerMm = 10.0 / defaultModuleMm;
		/// The pixels per millimetre when the module M is \p module_mm
	constexpr qreal pxPerMmFor(qreal module_mm) { return 10.0 / module_mm; }
	constexpr qreal mmToPx(qreal mm, qreal px_per_mm) { return mm * px_per_mm; }
	constexpr qreal pxToMm(qreal px, qreal px_per_mm) { return px / px_per_mm; }

		/// The cap height of \p font as a fraction of its em
	qreal capHeightRatio(const QFont &font);
		/// The capital height h (ISO 3098) in mm of text of \p font at
		/// \p pt points
	qreal pointsToTextMm(qreal pt, const QFont &font, qreal px_per_mm);
		/// The point size that gives the capital height \p mm with \p font
	qreal textMmToPoints(qreal mm, const QFont &font, qreal px_per_mm);
}

#endif
