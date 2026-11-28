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
#include "qetunits.h"

#include <QFont>
#include <QHash>
#include <QFontMetricsF>

namespace {
	/// CSS pixels per point, as in the em size of a font set in points
	constexpr qreal pxPerPoint = 96.0 / 72.0;
}

/**
	@brief QetUnits::capHeightRatio
	Measured on \\p font at a large pixel size, so hinting does not
	round it. The measure builds a font and its metrics, and texts are
	measured in loops, so the ratio is kept for each family, style, weight,
	slant and stretch. Used from the main thread only, like the fonts of
	the drawing: the cache is not locked.
	@param font
	@return the cap height as a fraction of the em, 0.7 if the font has none
*/
qreal QetUnits::capHeightRatio(const QFont &font)
{
	static QHash<QString, qreal> cache;
	const QString key = QStringLiteral("%1|%2|%3|%4|%5")
			.arg(font.family(), font.styleName())
			.arg(int(font.weight()))
			.arg(int(font.style()))
			.arg(font.stretch());
	const auto cached = cache.constFind(key);
	if (cached != cache.constEnd())
		return cached.value();

	constexpr int measure_px = 1000;
	QFont f(font);
	f.setPixelSize(measure_px);
	const qreal measured = QFontMetricsF(f).capHeight() / measure_px;
	const qreal ratio = measured > 0 ? measured : 0.7;
	cache.insert(key, ratio);
	return ratio;
}

/**
	@brief QetUnits::pointsToTextMm
	em_px = pt * 96 / 72, h_mm = em_px * capHeightRatio / px_per_mm
	@param pt
	@param font
	@param px_per_mm : of the project, QETProject::pxPerMm()
	@return h in mm
*/
qreal QetUnits::pointsToTextMm(qreal pt, const QFont &font, qreal px_per_mm)
{
	return pt * pxPerPoint * capHeightRatio(font) / px_per_mm;
}

/**
	@brief QetUnits::textMmToPoints
	The inverse of pointsToTextMm(), not rounded.
	@param mm
	@param font
	@param px_per_mm : of the project, QETProject::pxPerMm()
	@return the point size
*/
qreal QetUnits::textMmToPoints(qreal mm, const QFont &font, qreal px_per_mm)
{
	return mm * px_per_mm / (capHeightRatio(font) * pxPerPoint);
}
