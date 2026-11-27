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
#include "textsizebox.h"

#include "../qetproject.h"
#include "../qetunits.h"

#include <QDoubleSpinBox>
#include <QFont>
#include <QObject>
#include <QtMath>

namespace {
	qreal round2(qreal v) { return qRound(v * 100.0) / 100.0; }
}

qreal TextSizeBox::pxPerMm(const QETProject *project)
{
	return project ? project->paperPxPerMm() : 0;
}

void TextSizeBox::configure(QDoubleSpinBox *sb, qreal px_per_mm, int min_pt,
							int max_pt, const QFont &font)
{
	if (px_per_mm > 0) {
		sb->setDecimals(2);
		sb->setSingleStep(0.1);
		sb->setSuffix(QObject::tr(" mm"));
		sb->setRange(
			qFloor(QetUnits::pointsToTextMm(min_pt, font, px_per_mm) * 100.0)
				/ 100.0,
			qCeil(QetUnits::pointsToTextMm(max_pt, font, px_per_mm) * 100.0)
				/ 100.0);
	} else {
		sb->setDecimals(0);
		sb->setSingleStep(1);
		sb->setSuffix(QString());
		sb->setRange(min_pt, max_pt);
	}
}

qreal TextSizeBox::shown(qreal px_per_mm, qreal pt, const QFont &font)
{
	return px_per_mm > 0
		? round2(QetUnits::pointsToTextMm(pt, font, px_per_mm))
		: qRound(pt);
}

void TextSizeBox::setPoints(QDoubleSpinBox *sb, qreal px_per_mm, qreal pt,
							const QFont &font)
{
	sb->setValue(shown(px_per_mm, pt, font));
}

bool TextSizeBox::isUnchanged(const QDoubleSpinBox *sb, qreal px_per_mm,
							  qreal pt, const QFont &font)
{
	return qAbs(sb->value() - shown(px_per_mm, pt, font))
			< (px_per_mm > 0 ? 0.001 : 0.5);
}

qreal TextSizeBox::points(const QDoubleSpinBox *sb, qreal px_per_mm,
						  const QFont &font)
{
	if (px_per_mm <= 0)
		return qRound(sb->value());
	return qRound(QetUnits::textMmToPoints(sb->value(), font, px_per_mm)
				  * 10.0) / 10.0;
}
